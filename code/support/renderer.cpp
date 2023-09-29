﻿//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#if defined(__GNUC__)

#if defined(__linux__) && defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR 1
#include <android/log.h>
#else
#error Unknown Platform
#endif

#elif defined(_MSC_VER)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#define VK_USE_PLATFORM_WIN32_KHR 1
#include <sdkddkver.h>
#include <windows.h>

#else
#error Unknown Compiler
#endif

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>
#include "../../thirdparty/vulkansdk/include/vulkan/vulkan.h"
#include "../../thirdparty/vma/vk_mem_alloc.h"
#include "streaming.h"
#include "frame_throttling.h"
#include "../demo.h"

static uint32_t const g_preferred_resolution_width = 1280U;
static uint32_t const g_preferred_resolution_height = 720U;

#ifndef NDEBUG
static VkBool32 VKAPI_PTR vulkan_debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);
#endif

static inline uint32_t __internal_find_lowest_memory_type_index(struct VkPhysicalDeviceMemoryProperties const *physical_device_memory_properties, VkDeviceSize memory_requirements_size, uint32_t memory_requirements_memory_type_bits, VkMemoryPropertyFlags required_property_flags);

static inline uint32_t __internal_find_lowest_memory_type_index(struct VkPhysicalDeviceMemoryProperties const *physical_device_memory_properties, VkDeviceSize memory_requirements_size, uint32_t memory_requirements_memory_type_bits, VkMemoryPropertyFlags required_property_flags, VkMemoryPropertyFlags preferred_property_flags);

class vulkan_renderer
{
	VkAllocationCallbacks *m_allocation_callbacks;

	PFN_vkGetInstanceProcAddr m_pfn_get_instance_proc_addr;
	VkInstance m_instance;

#ifndef NDEBUG
	VkDebugUtilsMessengerEXT m_messenge;
#endif

	VkPhysicalDevice m_physical_device;
	uint32_t m_min_uniform_buffer_offset_alignment;
	uint32_t m_min_storage_buffer_offset_alignment;
	uint32_t m_optimal_buffer_copy_offset_alignment;
	uint32_t m_optimal_buffer_copy_row_pitch_alignment;

	bool m_has_dedicated_transfer_queue;
	uint32_t m_queue_graphics_family_index;
	uint32_t m_queue_transfer_family_index;
	uint32_t m_queue_graphics_queue_index;
	uint32_t m_queue_transfer_queue_index;

	PFN_vkGetDeviceProcAddr m_pfn_get_device_proc_addr;
	bool m_physical_device_feature_texture_compression_ASTC_LDR;
	bool m_physical_device_feature_texture_compression_BC;
	VkDevice m_device;

	VkQueue m_queue_graphics;
	VkQueue m_queue_transfer;

	VkCommandPool m_command_pools[FRAME_THROTTLING_COUNT];
	VkCommandBuffer m_command_buffers[FRAME_THROTTLING_COUNT];
	VkSemaphore m_semaphores_acquire_next_image[FRAME_THROTTLING_COUNT];
	VkSemaphore m_semaphores_queue_submit[FRAME_THROTTLING_COUNT];
	VkFence m_fences[FRAME_THROTTLING_COUNT];

	VkDeviceSize m_upload_ring_buffer_size;
	VkBuffer m_upload_ring_buffer;
	VkDeviceMemory m_upload_ring_buffer_device_memory;
	void *m_upload_ring_buffer_device_memory_pointer;
	VkFormat m_depth_format;
	uint32_t m_depth_stencil_transient_attachment_memory_index;

	uint32_t m_upload_ring_buffer_begin[FRAME_THROTTLING_COUNT];
	uint32_t m_upload_ring_buffer_end[FRAME_THROTTLING_COUNT];
	uint32_t m_upload_ring_buffer_current[FRAME_THROTTLING_COUNT];

	VmaAllocator m_asset_allocator;

	PFN_vkWaitForFences m_pfn_wait_for_fences;
	PFN_vkResetFences m_pfn_reset_fences;
	PFN_vkResetCommandPool m_pfn_reset_command_pool;
	PFN_vkBeginCommandBuffer m_pfn_begin_command_buffer;
	PFN_vkEndCommandBuffer m_pfn_end_command_buffer;
	PFN_vkAcquireNextImageKHR m_pfn_acquire_next_image;
	PFN_vkQueueSubmit m_pfn_queue_submit;
	PFN_vkQueuePresentKHR m_pfn_queue_present;

	class Demo m_demo;

	VkSurfaceKHR m_surface;

	uint32_t m_framebuffer_width;
	uint32_t m_framebuffer_height;
	VkSwapchainKHR m_swapchain;
	uint32_t m_swapchain_image_count;
	std::vector<VkImage> m_swapchain_images;
	std::vector<VkImageView> m_swapchain_image_views;

	uint32_t m_frame_throtting_index;

	void destory_framebuffer();
	void create_framebuffer();

public:
	void init();
	void attach_window(void *window_void);
	void dettach_window();
	void draw();
	void destroy();
};

void vulkan_renderer::init()
{
#if defined(__GNUC__)

#if defined(__linux__) && defined(__ANDROID__)
	// Add the layer to the apk
#else
#error Unknown Platform
#endif

#elif defined(_MSC_VER)

#ifndef NDEBUG
	{
		WCHAR file_name[4096];
		DWORD res_get_file_name = GetModuleFileNameW(NULL, file_name, sizeof(file_name) / sizeof(file_name[0]));
		assert(0U != res_get_file_name);

		for (int i = (res_get_file_name - 1); i > 0; --i)
		{
			if (L'\\' == file_name[i])
			{
				file_name[i] = L'\0';
				break;
			}
		}

		BOOL res_set_environment_variable = SetEnvironmentVariableW(L"VK_LAYER_PATH", file_name);
		assert(FALSE != res_set_environment_variable);
	}
#endif
#else
#error Unknown Compiler
#endif

	this->m_allocation_callbacks = NULL;

	this->m_pfn_get_instance_proc_addr = vkGetInstanceProcAddr;

	uint32_t vulkan_api_version = VK_API_VERSION_1_0;

	this->m_instance = VK_NULL_HANDLE;
	{
		PFN_vkCreateInstance pfn_vk_create_instance = reinterpret_cast<PFN_vkCreateInstance>(this->m_pfn_get_instance_proc_addr(VK_NULL_HANDLE, "vkCreateInstance"));
		assert(NULL != pfn_vk_create_instance);

		VkApplicationInfo application_info = {
			VK_STRUCTURE_TYPE_APPLICATION_INFO,
			NULL,
			"Vulkan-Demo",
			0,
			"Vulkan-Demo",
			0,
			VK_API_VERSION_1_0};

#ifndef NDEBUG
		// Android
		// Include the following meta-data element within the <application> tag in the AndroidManifest.xml file
		// <application ...>
		//     <meta-data android:name="com.android.graphics.injectLayers.enable" android:value="true" />
		// ...
		// 	</application>
		char const *enabled_layer_names[] = {
			"VK_LAYER_KHRONOS_validation"};

		char const *enabled_extension_names[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(__GNUC__)

#if defined(__linux__) && defined(__ANDROID__)
			VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#else
#error Unknown Platform
#endif

#elif defined(_MSC_VER)

			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
#error Unknown Compiler
#endif
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
#else
		char const *enabled_extension_names[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(__GNUC__)

#if defined(__linux__) && defined(__ANDROID__)
			VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#else
#error Unknown Platform
#endif

#elif defined(_MSC_VER)

			VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#else
#error Unknown Compiler
#endif
		};
#endif

		VkInstanceCreateInfo instance_create_info = {
			VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			NULL,
			0U,
			&application_info,
#ifndef NDEBUG
			sizeof(enabled_layer_names) / sizeof(enabled_layer_names[0]),
			enabled_layer_names,
#else
			0U,
			NULL,
#endif
			sizeof(enabled_extension_names) / sizeof(enabled_extension_names[0]),
			enabled_extension_names};

		// TODO: validation layer will crash on Android
		VkResult res_create_instance = pfn_vk_create_instance(&instance_create_info, this->m_allocation_callbacks, &this->m_instance);
		assert(VK_SUCCESS == res_create_instance);
	}

	this->m_pfn_get_instance_proc_addr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetInstanceProcAddr"));
	assert(NULL != this->m_pfn_get_instance_proc_addr);

#ifndef NDEBUG
	this->m_messenge = VK_NULL_HANDLE;
	{
		PFN_vkCreateDebugUtilsMessengerEXT pfn_vk_create_debug_utils_messenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkCreateDebugUtilsMessengerEXT"));
		assert(NULL != pfn_vk_create_debug_utils_messenger);

		VkDebugUtilsMessengerCreateInfoEXT debug_utils_messenger_create_info =
			{
				VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
				0U,
				0U,
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
				vulkan_debug_utils_messenger_callback,
				NULL};

		VkResult res_create_debug_utils_messenger = pfn_vk_create_debug_utils_messenger(this->m_instance, &debug_utils_messenger_create_info, this->m_allocation_callbacks, &this->m_messenge);
		assert(VK_SUCCESS == res_create_debug_utils_messenger);
	}
#endif

	this->m_physical_device = VK_NULL_HANDLE;
	this->m_min_uniform_buffer_offset_alignment = -1;
	this->m_min_storage_buffer_offset_alignment = -1;
	this->m_optimal_buffer_copy_offset_alignment = -1;
	this->m_optimal_buffer_copy_row_pitch_alignment = -1;
	{
		PFN_vkEnumeratePhysicalDevices pfn_enumerate_physical_devices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkEnumeratePhysicalDevices"));
		assert(NULL != pfn_enumerate_physical_devices);
		PFN_vkGetPhysicalDeviceProperties pfn_get_physical_device_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceProperties"));
		assert(NULL != pfn_get_physical_device_properties);

		uint32_t physical_device_count_1 = uint32_t(-1);
		VkResult res_enumerate_physical_devices_1 = pfn_enumerate_physical_devices(this->m_instance, &physical_device_count_1, NULL);
		assert(VK_SUCCESS == res_enumerate_physical_devices_1 && 0U < physical_device_count_1);

		VkPhysicalDevice *physical_devices = static_cast<VkPhysicalDevice *>(malloc(sizeof(VkPhysicalDevice) * physical_device_count_1));
		assert(NULL != physical_devices);

		uint32_t physical_device_count_2 = physical_device_count_1;
		VkResult res_enumerate_physical_devices_2 = pfn_enumerate_physical_devices(this->m_instance, &physical_device_count_2, physical_devices);
		assert(VK_SUCCESS == res_enumerate_physical_devices_2 && physical_device_count_1 == physical_device_count_2);

		// The lower index may imply the user preference (e.g. VK_LAYER_MESA_device_select)
		uint32_t physical_device_index_first_discrete_gpu = uint32_t(-1);
		VkDeviceSize min_uniform_buffer_offset_alignment_first_discrete_gpu = VkDeviceSize(-1);
		VkDeviceSize min_storage_buffer_offset_alignment_first_discrete_gpu = VkDeviceSize(-1);
		VkDeviceSize optimal_buffer_copy_offset_alignment_first_discrete_gpu = VkDeviceSize(-1);
		VkDeviceSize optimal_buffer_copy_row_pitch_alignment_first_discrete_gpu = VkDeviceSize(-1);
		uint32_t physical_device_index_first_integrated_gpu = uint32_t(-1);
		VkDeviceSize min_uniform_buffer_offset_alignment_first_integrated_gpu = VkDeviceSize(-1);
		VkDeviceSize min_storage_buffer_offset_alignment_first_integrated_gpu = VkDeviceSize(-1);
		VkDeviceSize optimal_buffer_copy_offset_alignment_first_integrated_gpu = VkDeviceSize(-1);
		VkDeviceSize optimal_buffer_copy_row_pitch_alignment_first_integrated_gpu = VkDeviceSize(-1);
		for (uint32_t physical_device_index = 0; (uint32_t(-1) == physical_device_index_first_discrete_gpu) && (physical_device_index < physical_device_count_2); ++physical_device_index)
		{
			VkPhysicalDeviceProperties physical_device_properties;
			pfn_get_physical_device_properties(physical_devices[physical_device_index], &physical_device_properties);

			if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == physical_device_properties.deviceType)
			{
				physical_device_index_first_discrete_gpu = physical_device_index;
				min_uniform_buffer_offset_alignment_first_discrete_gpu = physical_device_properties.limits.minUniformBufferOffsetAlignment;
				min_storage_buffer_offset_alignment_first_discrete_gpu = physical_device_properties.limits.minStorageBufferOffsetAlignment;
				optimal_buffer_copy_offset_alignment_first_discrete_gpu = physical_device_properties.limits.optimalBufferCopyOffsetAlignment;
				optimal_buffer_copy_row_pitch_alignment_first_discrete_gpu = physical_device_properties.limits.optimalBufferCopyRowPitchAlignment;
			}
			else if (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU == physical_device_properties.deviceType)
			{
				physical_device_index_first_integrated_gpu = physical_device_index;
				min_uniform_buffer_offset_alignment_first_integrated_gpu = physical_device_properties.limits.minUniformBufferOffsetAlignment;
				min_storage_buffer_offset_alignment_first_integrated_gpu = physical_device_properties.limits.minStorageBufferOffsetAlignment;
				optimal_buffer_copy_offset_alignment_first_integrated_gpu = physical_device_properties.limits.optimalBufferCopyOffsetAlignment;
				optimal_buffer_copy_row_pitch_alignment_first_integrated_gpu = physical_device_properties.limits.optimalBufferCopyRowPitchAlignment;
			}
		}

		// The discrete gpu is preferred
		if (uint32_t(-1) != physical_device_index_first_discrete_gpu)
		{
			this->m_physical_device = physical_devices[physical_device_index_first_discrete_gpu];
			assert(min_uniform_buffer_offset_alignment_first_discrete_gpu < static_cast<VkDeviceSize>(UINT32_MAX));
			assert(min_storage_buffer_offset_alignment_first_discrete_gpu < static_cast<VkDeviceSize>(UINT32_MAX));
			assert(optimal_buffer_copy_offset_alignment_first_discrete_gpu < static_cast<VkDeviceSize>(UINT32_MAX));
			assert(optimal_buffer_copy_row_pitch_alignment_first_discrete_gpu < static_cast<VkDeviceSize>(UINT32_MAX));
			this->m_min_uniform_buffer_offset_alignment = static_cast<uint32_t>(min_uniform_buffer_offset_alignment_first_discrete_gpu);
			this->m_min_storage_buffer_offset_alignment = static_cast<uint32_t>(min_storage_buffer_offset_alignment_first_discrete_gpu);
			this->m_optimal_buffer_copy_offset_alignment = static_cast<uint32_t>(optimal_buffer_copy_offset_alignment_first_discrete_gpu);
			this->m_optimal_buffer_copy_row_pitch_alignment = static_cast<uint32_t>(optimal_buffer_copy_row_pitch_alignment_first_discrete_gpu);
		}
		else if (uint32_t(-1) != physical_device_index_first_integrated_gpu)
		{
			this->m_physical_device = physical_devices[physical_device_index_first_integrated_gpu];
			assert(min_uniform_buffer_offset_alignment_first_integrated_gpu < static_cast<VkDeviceSize>(UINT32_MAX));
			assert(min_storage_buffer_offset_alignment_first_integrated_gpu < static_cast<VkDeviceSize>(UINT32_MAX));
			assert(optimal_buffer_copy_offset_alignment_first_integrated_gpu < static_cast<VkDeviceSize>(UINT32_MAX));
			assert(optimal_buffer_copy_row_pitch_alignment_first_integrated_gpu < static_cast<VkDeviceSize>(UINT32_MAX));
			this->m_min_uniform_buffer_offset_alignment = static_cast<uint32_t>(min_uniform_buffer_offset_alignment_first_integrated_gpu);
			this->m_min_storage_buffer_offset_alignment = static_cast<uint32_t>(min_storage_buffer_offset_alignment_first_integrated_gpu);
			this->m_optimal_buffer_copy_offset_alignment = static_cast<uint32_t>(optimal_buffer_copy_offset_alignment_first_integrated_gpu);
			this->m_optimal_buffer_copy_row_pitch_alignment = static_cast<uint32_t>(optimal_buffer_copy_row_pitch_alignment_first_integrated_gpu);
		}
		else
		{
			assert(false);
		}

		free(physical_devices);
	}

	// https://github.com/ValveSoftware/dxvk
	// src/dxvk/dxvk_device.h
	// DxvkDevice::hasDedicatedTransferQueue
	this->m_has_dedicated_transfer_queue = false;
	// nvpro-samples/shared_sources/nvvk/context_vk.cpp
	// Context::initDevice
	// m_queue_GCT
	// m_queue_CT
	// m_queue_transfer
	this->m_queue_graphics_family_index = VK_QUEUE_FAMILY_IGNORED;
	this->m_queue_transfer_family_index = VK_QUEUE_FAMILY_IGNORED;
	this->m_queue_graphics_queue_index = uint32_t(-1);
	this->m_queue_transfer_queue_index = uint32_t(-1);
	{
		PFN_vkGetPhysicalDeviceQueueFamilyProperties pfn_vk_get_physical_device_queue_family_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
		assert(NULL != pfn_vk_get_physical_device_queue_family_properties);

#if defined(__GNUC__)

#if defined(__linux__) && defined(__ANDROID__)
		// Android always supported
#else
#error Unknown Platform
#endif

#elif defined(_MSC_VER)
		PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR pfn_vk_get_physical_device_win32_presentation_support = reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR"));
		assert(NULL != pfn_vk_get_physical_device_win32_presentation_support);
#else
#error Unknown Compiler
#endif

		uint32_t queue_family_property_count_1 = uint32_t(-1);
		pfn_vk_get_physical_device_queue_family_properties(this->m_physical_device, &queue_family_property_count_1, NULL);

		VkQueueFamilyProperties *queue_family_properties = static_cast<VkQueueFamilyProperties *>(malloc(sizeof(VkQueueFamilyProperties) * queue_family_property_count_1));
		assert(NULL != queue_family_properties);

		uint32_t queue_family_property_count_2 = queue_family_property_count_1;
		pfn_vk_get_physical_device_queue_family_properties(this->m_physical_device, &queue_family_property_count_2, queue_family_properties);
		assert(queue_family_property_count_1 == queue_family_property_count_2);

		// TODO: support seperated present queue
		// src/dxvk/dxvk_adapter.cpp
		// DxvkAdapter::findQueueFamilies
		// src/d3d11/d3d11_swapchain.cpp
		// D3D11SwapChain::CreatePresenter

		for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_property_count_2; ++queue_family_index)
		{
			VkQueueFamilyProperties queue_family_property = queue_family_properties[queue_family_index];

#if defined(__GNUC__)

#if defined(__linux__) && defined(__ANDROID__)
			if ((queue_family_property.queueFlags & VK_QUEUE_GRAPHICS_BIT))
#else
#error Unknown Platform
#endif

#elif defined(_MSC_VER)
			if ((queue_family_property.queueFlags & VK_QUEUE_GRAPHICS_BIT) && pfn_vk_get_physical_device_win32_presentation_support(this->m_physical_device, queue_family_index))
#else
#error Unknown Compiler
#endif
			{
				this->m_queue_graphics_family_index = queue_family_index;
				this->m_queue_graphics_queue_index = 0U;
				break;
			}
		}

		if (VK_QUEUE_FAMILY_IGNORED != this->m_queue_graphics_family_index)
		{
			// Find transfer queues
			for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_property_count_2; ++queue_family_index)
			{
				if ((this->m_queue_graphics_family_index != queue_family_index) && (VK_QUEUE_TRANSFER_BIT == (queue_family_properties[queue_family_index].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))))
				{
					this->m_queue_transfer_family_index = queue_family_index;
					this->m_queue_transfer_queue_index = 0U;
					this->m_has_dedicated_transfer_queue = true;
					break;
				}
			}

			// Fallback to other graphics/compute queues
			// By vkspec, "either GRAPHICS or COMPUTE implies TRANSFER". This means TRANSFER is optional.
			if (VK_QUEUE_FAMILY_IGNORED == this->m_queue_transfer_family_index)
			{
				for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_property_count_2; ++queue_family_index)
				{
					if ((this->m_queue_graphics_family_index != queue_family_index) && (0 != (queue_family_properties[queue_family_index].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))))
					{
						this->m_queue_transfer_family_index = queue_family_index;
						this->m_queue_transfer_queue_index = 0U;
						this->m_has_dedicated_transfer_queue = true;
						break;
					}
				}
			}

			// Try the same queue family
			if (VK_QUEUE_FAMILY_IGNORED == this->m_queue_transfer_family_index)
			{
				if (2U <= queue_family_properties[this->m_queue_graphics_family_index].queueCount)
				{
					this->m_queue_transfer_family_index = this->m_queue_graphics_family_index;
					this->m_queue_transfer_queue_index = 1U;
					this->m_has_dedicated_transfer_queue = true;
				}
				else
				{
					this->m_queue_transfer_family_index = VK_QUEUE_FAMILY_IGNORED;
					this->m_queue_transfer_queue_index = uint32_t(-1);
					this->m_has_dedicated_transfer_queue = false;
				}
			}
		}

		assert(VK_QUEUE_FAMILY_IGNORED != this->m_queue_graphics_family_index && (!this->m_has_dedicated_transfer_queue || VK_QUEUE_FAMILY_IGNORED != this->m_queue_transfer_family_index));

		free(queue_family_properties);
	}

	this->m_physical_device_feature_texture_compression_ASTC_LDR = false;
	this->m_physical_device_feature_texture_compression_BC = false;
	this->m_device = VK_NULL_HANDLE;
	{
		PFN_vkGetPhysicalDeviceFeatures pfn_vk_get_physical_device_features = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceFeatures"));
		assert(NULL != pfn_vk_get_physical_device_features);
		PFN_vkCreateDevice pfn_vk_create_device = reinterpret_cast<PFN_vkCreateDevice>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkCreateDevice"));
		assert(NULL != pfn_vk_create_device);

		VkPhysicalDeviceFeatures physical_device_features;
		pfn_vk_get_physical_device_features(this->m_physical_device, &physical_device_features);
		physical_device_features.robustBufferAccess = VK_FALSE;
		physical_device_features.fullDrawIndexUint32 = VK_FALSE;
		physical_device_features.imageCubeArray = VK_FALSE;
		physical_device_features.independentBlend = VK_FALSE;
		physical_device_features.geometryShader = VK_FALSE;
		physical_device_features.tessellationShader = VK_FALSE;
		physical_device_features.sampleRateShading = VK_FALSE;
		physical_device_features.dualSrcBlend = VK_FALSE;
		physical_device_features.logicOp = VK_FALSE;
		physical_device_features.multiDrawIndirect = VK_FALSE;
		physical_device_features.drawIndirectFirstInstance = VK_FALSE;
		physical_device_features.depthClamp = VK_FALSE;
		physical_device_features.depthBiasClamp = VK_FALSE;
		physical_device_features.fillModeNonSolid = VK_FALSE;
		physical_device_features.depthBounds = VK_FALSE;
		physical_device_features.wideLines = VK_FALSE;
		physical_device_features.largePoints = VK_FALSE;
		physical_device_features.alphaToOne = VK_FALSE;
		physical_device_features.multiViewport = VK_FALSE;
		physical_device_features.samplerAnisotropy = VK_FALSE;
		physical_device_features.textureCompressionETC2 = VK_FALSE;
		// this->m_physical_device_feature_texture_compression_ASTC_LDR = (physical_device_features.textureCompressionASTC_LDR != VK_FALSE) ? true : false;
		physical_device_features.textureCompressionASTC_LDR = VK_FALSE;
		// this->m_physical_device_feature_texture_compression_BC = (physical_device_features.textureCompressionBC != VK_FALSE) ? true : false;
		physical_device_features.textureCompressionBC = VK_FALSE;
		physical_device_features.occlusionQueryPrecise = VK_FALSE;
		physical_device_features.pipelineStatisticsQuery = VK_FALSE;
		physical_device_features.vertexPipelineStoresAndAtomics = VK_FALSE;
		physical_device_features.fragmentStoresAndAtomics = VK_FALSE;
		physical_device_features.shaderTessellationAndGeometryPointSize = VK_FALSE;
		physical_device_features.shaderImageGatherExtended = VK_FALSE;
		physical_device_features.shaderStorageImageExtendedFormats = VK_FALSE;
		physical_device_features.shaderStorageImageMultisample = VK_FALSE;
		physical_device_features.shaderStorageImageReadWithoutFormat = VK_FALSE;
		physical_device_features.shaderStorageImageWriteWithoutFormat = VK_FALSE;
		physical_device_features.shaderUniformBufferArrayDynamicIndexing = VK_FALSE;
		physical_device_features.shaderSampledImageArrayDynamicIndexing = VK_FALSE;
		physical_device_features.shaderStorageBufferArrayDynamicIndexing = VK_FALSE;
		physical_device_features.shaderStorageImageArrayDynamicIndexing = VK_FALSE;
		physical_device_features.shaderClipDistance = VK_FALSE;
		physical_device_features.shaderCullDistance = VK_FALSE;
		physical_device_features.shaderFloat64 = VK_FALSE;
		physical_device_features.shaderInt64 = VK_FALSE;
		physical_device_features.shaderInt16 = VK_FALSE;
		physical_device_features.shaderResourceResidency = VK_FALSE;
		physical_device_features.shaderResourceMinLod = VK_FALSE;
		physical_device_features.sparseBinding = VK_FALSE;
		physical_device_features.sparseResidencyBuffer = VK_FALSE;
		physical_device_features.sparseResidencyImage2D = VK_FALSE;
		physical_device_features.sparseResidencyImage3D = VK_FALSE;
		physical_device_features.sparseResidency2Samples = VK_FALSE;
		physical_device_features.sparseResidency4Samples = VK_FALSE;
		physical_device_features.sparseResidency8Samples = VK_FALSE;
		physical_device_features.sparseResidency16Samples = VK_FALSE;
		physical_device_features.sparseResidencyAliased = VK_FALSE;
		physical_device_features.variableMultisampleRate = VK_FALSE;
		physical_device_features.inheritedQueries = VK_FALSE;

		float queue_graphics_priority = 1.0F;
		float queue_transfer_priority = 1.0F;
		float queue_graphics_transfer_priorities[2] = {queue_graphics_priority, queue_transfer_priority};
		struct VkDeviceQueueCreateInfo device_queue_create_infos[2];
		uint32_t device_queue_create_info_count = -1;
		if (this->m_has_dedicated_transfer_queue)
		{
			if (this->m_queue_graphics_family_index != this->m_queue_transfer_family_index)
			{
				device_queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				device_queue_create_infos[0].pNext = NULL;
				device_queue_create_infos[0].flags = 0U;
				device_queue_create_infos[0].queueFamilyIndex = this->m_queue_graphics_family_index;
				assert(0U == this->m_queue_graphics_queue_index);
				device_queue_create_infos[0].queueCount = 1U;
				device_queue_create_infos[0].pQueuePriorities = &queue_graphics_priority;

				device_queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				device_queue_create_infos[1].pNext = NULL;
				device_queue_create_infos[1].flags = 0U;
				device_queue_create_infos[1].queueFamilyIndex = this->m_queue_transfer_family_index;
				assert(0U == this->m_queue_transfer_queue_index);
				device_queue_create_infos[1].queueCount = 1U;
				device_queue_create_infos[1].pQueuePriorities = &queue_transfer_priority;

				device_queue_create_info_count = 2U;
			}
			else
			{
				device_queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				device_queue_create_infos[0].pNext = NULL;
				device_queue_create_infos[0].flags = 0U;
				device_queue_create_infos[0].queueFamilyIndex = this->m_queue_graphics_family_index;
				assert(0U == this->m_queue_graphics_queue_index);
				assert(1U == this->m_queue_transfer_queue_index);
				device_queue_create_infos[0].queueCount = 2U;
				device_queue_create_infos[0].pQueuePriorities = queue_graphics_transfer_priorities;
				device_queue_create_info_count = 1U;
			}
		}
		else
		{
			device_queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			device_queue_create_infos[0].pNext = NULL;
			device_queue_create_infos[0].flags = 0U;
			device_queue_create_infos[0].queueFamilyIndex = this->m_queue_graphics_family_index;
			assert(0U == this->m_queue_graphics_queue_index);
			device_queue_create_infos[0].queueCount = 1U;
			device_queue_create_infos[0].pQueuePriorities = &queue_graphics_priority;
			device_queue_create_info_count = 1U;
		}

		struct VkDeviceCreateInfo device_create_info;
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.pNext = NULL;
		device_create_info.flags = 0U;
		device_create_info.pQueueCreateInfos = device_queue_create_infos;
		device_create_info.queueCreateInfoCount = device_queue_create_info_count;
		device_create_info.enabledLayerCount = 0U;
		device_create_info.ppEnabledLayerNames = NULL;
		char const* enabled_extension_names[2] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME };
		device_create_info.enabledExtensionCount = 2U;
		device_create_info.ppEnabledExtensionNames = enabled_extension_names;
		device_create_info.pEnabledFeatures = &physical_device_features;

		VkResult res_create_device = pfn_vk_create_device(this->m_physical_device, &device_create_info, this->m_allocation_callbacks, &this->m_device);
		assert(VK_SUCCESS == res_create_device);
	}

	this->m_pfn_get_device_proc_addr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetDeviceProcAddr"));
	assert(NULL != this->m_pfn_get_device_proc_addr);

	this->m_pfn_get_device_proc_addr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetDeviceProcAddr"));
	assert(NULL != this->m_pfn_get_device_proc_addr);

	this->m_queue_graphics = VK_NULL_HANDLE;
	this->m_queue_transfer = VK_NULL_HANDLE;
	{
		PFN_vkGetDeviceQueue vk_get_device_queue = reinterpret_cast<PFN_vkGetDeviceQueue>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetDeviceQueue"));
		assert(NULL != vk_get_device_queue);

		vk_get_device_queue(this->m_device, this->m_queue_graphics_family_index, this->m_queue_graphics_queue_index, &this->m_queue_graphics);

		if (this->m_has_dedicated_transfer_queue)
		{
			assert(VK_QUEUE_FAMILY_IGNORED != this->m_queue_transfer_family_index);
			assert(uint32_t(-1) != this->m_queue_transfer_queue_index);
			vk_get_device_queue(this->m_device, this->m_queue_transfer_family_index, this->m_queue_transfer_queue_index, &this->m_queue_transfer);
		}
	}
	assert(VK_NULL_HANDLE != this->m_queue_graphics && (!this->m_has_dedicated_transfer_queue || VK_NULL_HANDLE != this->m_queue_transfer));

	this->m_command_pools[0] = VK_NULL_HANDLE;
	this->m_command_buffers[0] = VK_NULL_HANDLE;
	this->m_semaphores_acquire_next_image[0] = VK_NULL_HANDLE;
	this->m_semaphores_queue_submit[0] = VK_NULL_HANDLE;
	this->m_fences[0] = VK_NULL_HANDLE;
	{
		PFN_vkCreateCommandPool pfn_create_command_pool = reinterpret_cast<PFN_vkCreateCommandPool>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateCommandPool"));
		assert(NULL != pfn_create_command_pool);
		PFN_vkAllocateCommandBuffers pfn_allocate_command_buffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(this->m_pfn_get_device_proc_addr(this->m_device, "vkAllocateCommandBuffers"));
		assert(NULL != pfn_allocate_command_buffers);
		PFN_vkCreateFence pfn_create_fence = reinterpret_cast<PFN_vkCreateFence>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateFence"));
		assert(NULL != pfn_create_fence);
		PFN_vkCreateSemaphore pfn_create_semaphore = reinterpret_cast<PFN_vkCreateSemaphore>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateSemaphore"));
		assert(NULL != pfn_create_semaphore);

		for (uint32_t frame_throtting_index = 0U; frame_throtting_index < FRAME_THROTTLING_COUNT; ++frame_throtting_index)
		{
			VkCommandPoolCreateInfo command_pool_create_info;
			command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			command_pool_create_info.pNext = NULL;
			command_pool_create_info.flags = 0U;
			command_pool_create_info.queueFamilyIndex = this->m_queue_graphics_family_index;
			VkResult res_create_command_pool = pfn_create_command_pool(this->m_device, &command_pool_create_info, this->m_allocation_callbacks, &this->m_command_pools[frame_throtting_index]);
			assert(VK_SUCCESS == res_create_command_pool);

			VkCommandBufferAllocateInfo command_buffer_allocate_info;
			command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			command_buffer_allocate_info.pNext = NULL;
			command_buffer_allocate_info.commandPool = this->m_command_pools[frame_throtting_index];
			command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			command_buffer_allocate_info.commandBufferCount = 1U;
			VkResult res_allocate_command_buffers = pfn_allocate_command_buffers(this->m_device, &command_buffer_allocate_info, &this->m_command_buffers[frame_throtting_index]);
			assert(VK_SUCCESS == res_allocate_command_buffers);

			VkSemaphoreCreateInfo semaphore_create_info_acquire_next_image;
			semaphore_create_info_acquire_next_image.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphore_create_info_acquire_next_image.pNext = NULL;
			semaphore_create_info_acquire_next_image.flags = 0U;
			VkResult res_create_semaphore_acquire_next_image = pfn_create_semaphore(this->m_device, &semaphore_create_info_acquire_next_image, this->m_allocation_callbacks, &this->m_semaphores_acquire_next_image[frame_throtting_index]);
			assert(VK_SUCCESS == res_create_semaphore_acquire_next_image);

			VkSemaphoreCreateInfo semaphore_create_info_queue_submit;
			semaphore_create_info_queue_submit.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semaphore_create_info_queue_submit.pNext = NULL;
			semaphore_create_info_queue_submit.flags = 0U;
			VkResult res_create_semaphore_queue_submit = pfn_create_semaphore(this->m_device, &semaphore_create_info_queue_submit, this->m_allocation_callbacks, &this->m_semaphores_queue_submit[frame_throtting_index]);
			assert(VK_SUCCESS == res_create_semaphore_queue_submit);

			VkFenceCreateInfo fence_create_info;
			fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_create_info.pNext = NULL;
			fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			VkResult res_create_fence = pfn_create_fence(this->m_device, &fence_create_info, this->m_allocation_callbacks, &this->m_fences[frame_throtting_index]);
			assert(VK_SUCCESS == res_create_fence);
		}
	}

	this->m_upload_ring_buffer_size = -1;
	this->m_upload_ring_buffer = VK_NULL_HANDLE;
	this->m_upload_ring_buffer_device_memory = VK_NULL_HANDLE;
	this->m_upload_ring_buffer_device_memory_pointer = NULL;
	this->m_depth_format = VK_FORMAT_UNDEFINED;
	this->m_depth_stencil_transient_attachment_memory_index = -1;
	{
		PFN_vkGetPhysicalDeviceMemoryProperties pfn_get_physical_device_memory_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceMemoryProperties"));
		PFN_vkCreateBuffer pfn_create_buffer = reinterpret_cast<PFN_vkCreateBuffer>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateBuffer"));
		PFN_vkGetBufferMemoryRequirements pfn_get_buffer_memory_requirements = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetBufferMemoryRequirements"));
		PFN_vkAllocateMemory pfn_allocate_memory = reinterpret_cast<PFN_vkAllocateMemory>(this->m_pfn_get_device_proc_addr(this->m_device, "vkAllocateMemory"));
		PFN_vkMapMemory pfn_map_memory = reinterpret_cast<PFN_vkMapMemory>(this->m_pfn_get_device_proc_addr(this->m_device, "vkMapMemory"));
		PFN_vkBindBufferMemory pfn_bind_buffer_memory = reinterpret_cast<PFN_vkBindBufferMemory>(this->m_pfn_get_device_proc_addr(this->m_device, "vkBindBufferMemory"));
		PFN_vkGetPhysicalDeviceFormatProperties pfn_get_physical_device_format_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceFormatProperties"));
		PFN_vkCreateImage pfn_create_image = reinterpret_cast<PFN_vkCreateImage>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateImage"));
		PFN_vkGetImageMemoryRequirements pfn_get_image_memory_requirements = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetImageMemoryRequirements"));
		PFN_vkDestroyImage pfn_destroy_image = reinterpret_cast<PFN_vkDestroyImage>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyImage"));

		VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
		pfn_get_physical_device_memory_properties(this->m_physical_device, &physical_device_memory_properties);

		// upload ring buffer
		// NVIDIA Driver 128 MB
		// \[Gruen 2015\] [Holger Gruen. "Constant Buffers without Constant Pain." NVIDIA GameWorks Blog 2015.](https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0)
		// AMD Special Pool 256MB
		// \[Sawicki 2018\] [Adam Sawicki. "Memory Management in Vulkan and DX12." GDC 2018.](https://gpuopen.com/events/gdc-2018-presentations)
		this->m_upload_ring_buffer_size = (224ULL * 1024ULL * 1024ULL); // 224MB
		{
			this->m_upload_ring_buffer = VK_NULL_HANDLE;
			VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
			uint32_t memory_requirements_memory_type_bits = 0U;
			{
				struct VkBufferCreateInfo buffer_create_info;
				buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				buffer_create_info.pNext = NULL;
				buffer_create_info.flags = 0U;
				buffer_create_info.size = this->m_upload_ring_buffer_size;
				buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
				buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				buffer_create_info.queueFamilyIndexCount = 0U;
				buffer_create_info.pQueueFamilyIndices = NULL;

				VkResult res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &this->m_upload_ring_buffer);
				assert(VK_SUCCESS == res_create_buffer);

				struct VkMemoryRequirements memory_requirements;
				pfn_get_buffer_memory_requirements(this->m_device, this->m_upload_ring_buffer, &memory_requirements);
				memory_requirements_size = memory_requirements.size;
				memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;
			}

			// VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			// We use the "AMD Special Pool" for upload ring buffer
			uint32_t vulkan_upload_ring_buffer_memory_index = __internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			assert(VK_MAX_MEMORY_TYPES > vulkan_upload_ring_buffer_memory_index);
			assert(physical_device_memory_properties.memoryTypeCount > vulkan_upload_ring_buffer_memory_index);

			this->m_upload_ring_buffer_device_memory = VK_NULL_HANDLE;
			{
				VkMemoryAllocateInfo memory_allocate_info;
				memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memory_allocate_info.pNext = NULL;
				memory_allocate_info.allocationSize = memory_requirements_size;
				memory_allocate_info.memoryTypeIndex = vulkan_upload_ring_buffer_memory_index;
				VkResult res_allocate_memory = pfn_allocate_memory(this->m_device, &memory_allocate_info, this->m_allocation_callbacks, &this->m_upload_ring_buffer_device_memory);
				assert(VK_SUCCESS == res_allocate_memory);
			}

			this->m_upload_ring_buffer_device_memory_pointer = NULL;
			VkResult res_map_memory = pfn_map_memory(this->m_device, this->m_upload_ring_buffer_device_memory, 0U, this->m_upload_ring_buffer_size, 0U, &this->m_upload_ring_buffer_device_memory_pointer);
			assert(VK_SUCCESS == res_map_memory);

			VkResult res_bind_buffer_memory = pfn_bind_buffer_memory(this->m_device, this->m_upload_ring_buffer, this->m_upload_ring_buffer_device_memory, 0U);
			assert(VK_SUCCESS == res_bind_buffer_memory);
		}

		// depth attachment
		this->m_depth_format = VK_FORMAT_UNDEFINED;
		{
			struct VkFormatProperties format_properties;
			this->m_depth_format = VK_FORMAT_D32_SFLOAT;
			pfn_get_physical_device_format_properties(this->m_physical_device, this->m_depth_format, &format_properties);
			if (0 == (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
			{
				this->m_depth_format = VK_FORMAT_X8_D24_UNORM_PACK32;
				pfn_get_physical_device_format_properties(this->m_physical_device, this->m_depth_format, &format_properties);
				assert(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
			}
		}
		this->m_depth_stencil_transient_attachment_memory_index = VK_MAX_MEMORY_TYPES;
		{

			VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
			uint32_t memory_requirements_memory_type_bits = 0U;
			{
				struct VkImageCreateInfo image_create_info_depth_stencil_transient_tiling_optimal;
				image_create_info_depth_stencil_transient_tiling_optimal.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				image_create_info_depth_stencil_transient_tiling_optimal.pNext = NULL;
				image_create_info_depth_stencil_transient_tiling_optimal.flags = 0U;
				image_create_info_depth_stencil_transient_tiling_optimal.imageType = VK_IMAGE_TYPE_2D;
				image_create_info_depth_stencil_transient_tiling_optimal.format = this->m_depth_format;
				image_create_info_depth_stencil_transient_tiling_optimal.extent.width = 8U;
				image_create_info_depth_stencil_transient_tiling_optimal.extent.height = 8U;
				image_create_info_depth_stencil_transient_tiling_optimal.extent.depth = 1U;
				image_create_info_depth_stencil_transient_tiling_optimal.mipLevels = 1U;
				image_create_info_depth_stencil_transient_tiling_optimal.arrayLayers = 1U;
				image_create_info_depth_stencil_transient_tiling_optimal.samples = VK_SAMPLE_COUNT_1_BIT;
				image_create_info_depth_stencil_transient_tiling_optimal.tiling = VK_IMAGE_TILING_OPTIMAL;
				image_create_info_depth_stencil_transient_tiling_optimal.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
				image_create_info_depth_stencil_transient_tiling_optimal.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				image_create_info_depth_stencil_transient_tiling_optimal.queueFamilyIndexCount = 0U;
				image_create_info_depth_stencil_transient_tiling_optimal.pQueueFamilyIndices = NULL;
				image_create_info_depth_stencil_transient_tiling_optimal.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

				VkImage dummy_img;
				VkResult res_create_image = pfn_create_image(this->m_device, &image_create_info_depth_stencil_transient_tiling_optimal, this->m_allocation_callbacks, &dummy_img);
				assert(VK_SUCCESS == res_create_image);

				struct VkMemoryRequirements memory_requirements;
				pfn_get_image_memory_requirements(this->m_device, dummy_img, &memory_requirements);
				memory_requirements_size = memory_requirements.size;
				memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

				pfn_destroy_image(this->m_device, dummy_img, this->m_allocation_callbacks);
			}

			// The lower index indicates the more performance
			this->m_depth_stencil_transient_attachment_memory_index = __internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
			assert(VK_MAX_MEMORY_TYPES > this->m_depth_stencil_transient_attachment_memory_index);
			assert(physical_device_memory_properties.memoryTypeCount > this->m_depth_stencil_transient_attachment_memory_index);
		}
	}

	this->m_upload_ring_buffer_begin[0] = VK_NULL_HANDLE;
	this->m_upload_ring_buffer_end[0] = VK_NULL_HANDLE;
	this->m_upload_ring_buffer_current[0] = VK_NULL_HANDLE;
	{
		assert(this->m_upload_ring_buffer_size < static_cast<VkDeviceSize>(UINT32_MAX));

		for (uint32_t frame_throtting_index = 0U; frame_throtting_index < FRAME_THROTTLING_COUNT; ++frame_throtting_index)
		{
			this->m_upload_ring_buffer_begin[frame_throtting_index] = (static_cast<uint32_t>(this->m_upload_ring_buffer_size) * frame_throtting_index) / FRAME_THROTTLING_COUNT;
			this->m_upload_ring_buffer_end[frame_throtting_index] = (static_cast<uint32_t>(this->m_upload_ring_buffer_size) * (frame_throtting_index + 1U)) / FRAME_THROTTLING_COUNT;
		}
	}

	this->m_asset_allocator = VK_NULL_HANDLE;
	{
		VmaVulkanFunctions vulkan_functions = {};
		vulkan_functions.vkGetInstanceProcAddr = this->m_pfn_get_instance_proc_addr;
		vulkan_functions.vkGetDeviceProcAddr = this->m_pfn_get_device_proc_addr;

		VmaAllocatorCreateInfo allocator_create_info = {};
		allocator_create_info.vulkanApiVersion = vulkan_api_version;
		allocator_create_info.physicalDevice = this->m_physical_device;
		allocator_create_info.device = this->m_device;
		allocator_create_info.instance = this->m_instance;
		allocator_create_info.pAllocationCallbacks = this->m_allocation_callbacks;
		allocator_create_info.pVulkanFunctions = &vulkan_functions;

		vmaCreateAllocator(&allocator_create_info, &this->m_asset_allocator);
	}

	this->m_pfn_wait_for_fences = reinterpret_cast<PFN_vkWaitForFences>(this->m_pfn_get_device_proc_addr(this->m_device, "vkWaitForFences"));
	assert(NULL != this->m_pfn_wait_for_fences);
	this->m_pfn_reset_fences = reinterpret_cast<PFN_vkResetFences>(this->m_pfn_get_device_proc_addr(this->m_device, "vkResetFences"));
	assert(NULL != this->m_pfn_reset_fences);
	this->m_pfn_reset_command_pool = reinterpret_cast<PFN_vkResetCommandPool>(this->m_pfn_get_device_proc_addr(this->m_device, "vkResetCommandPool"));
	assert(NULL != this->m_pfn_reset_command_pool);
	this->m_pfn_begin_command_buffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(this->m_pfn_get_device_proc_addr(this->m_device, "vkBeginCommandBuffer"));
	assert(NULL != this->m_pfn_begin_command_buffer);
	this->m_pfn_end_command_buffer = reinterpret_cast<PFN_vkEndCommandBuffer>(this->m_pfn_get_device_proc_addr(this->m_device, "vkEndCommandBuffer"));
	assert(NULL != this->m_pfn_end_command_buffer);
	this->m_pfn_acquire_next_image = reinterpret_cast<PFN_vkAcquireNextImageKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkAcquireNextImageKHR"));
	assert(NULL != this->m_pfn_acquire_next_image);
	this->m_pfn_queue_submit = reinterpret_cast<PFN_vkQueueSubmit>(this->m_pfn_get_device_proc_addr(this->m_device, "vkQueueSubmit"));
	assert(NULL != this->m_pfn_queue_submit);
	this->m_pfn_queue_present = reinterpret_cast<PFN_vkQueuePresentKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkQueuePresentKHR"));
	assert(NULL != this->m_pfn_queue_present);

	// Demo Init
	{
		// Streaming
		VkCommandPool vulkan_streaming_transfer_command_pool = VK_NULL_HANDLE;
		VkCommandBuffer vulkan_streaming_transfer_command_buffer = VK_NULL_HANDLE;
		VkCommandPool vulkan_streaming_graphics_command_pool = VK_NULL_HANDLE;
		VkCommandBuffer vulkan_streaming_graphics_command_buffer = VK_NULL_HANDLE;
		VkSemaphore vulkan_streaming_semaphore = VK_NULL_HANDLE;
		VkFence vulkan_streaming_fence = VK_NULL_HANDLE;
		{
			PFN_vkCreateCommandPool pfn_create_command_pool = reinterpret_cast<PFN_vkCreateCommandPool>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateCommandPool"));
			assert(NULL != pfn_create_command_pool);
			PFN_vkAllocateCommandBuffers pfn_allocate_command_buffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(this->m_pfn_get_device_proc_addr(this->m_device, "vkAllocateCommandBuffers"));
			assert(NULL != pfn_allocate_command_buffers);
			PFN_vkCreateFence pfn_create_fence = reinterpret_cast<PFN_vkCreateFence>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateFence"));
			assert(NULL != pfn_create_fence);
			PFN_vkCreateSemaphore pfn_create_semaphore = reinterpret_cast<PFN_vkCreateSemaphore>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateSemaphore"));
			assert(NULL != pfn_create_semaphore);

			if (this->m_has_dedicated_transfer_queue)
			{
				if (this->m_queue_graphics_family_index != this->m_queue_transfer_family_index)
				{

					VkCommandPoolCreateInfo transfer_command_pool_create_info;
					transfer_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
					transfer_command_pool_create_info.pNext = NULL;
					transfer_command_pool_create_info.flags = 0U;
					transfer_command_pool_create_info.queueFamilyIndex = this->m_queue_transfer_family_index;
					VkResult res_transfer_create_command_pool = pfn_create_command_pool(this->m_device, &transfer_command_pool_create_info, this->m_allocation_callbacks, &vulkan_streaming_transfer_command_pool);
					assert(VK_SUCCESS == res_transfer_create_command_pool);

					VkCommandBufferAllocateInfo transfer_command_buffer_allocate_info;
					transfer_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					transfer_command_buffer_allocate_info.pNext = NULL;
					transfer_command_buffer_allocate_info.commandPool = vulkan_streaming_transfer_command_pool;
					transfer_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					transfer_command_buffer_allocate_info.commandBufferCount = 1U;
					VkResult res_transfer_allocate_command_buffers = pfn_allocate_command_buffers(this->m_device, &transfer_command_buffer_allocate_info, &vulkan_streaming_transfer_command_buffer);
					assert(VK_SUCCESS == res_transfer_allocate_command_buffers);

					VkCommandPoolCreateInfo graphics_command_pool_create_info;
					graphics_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
					graphics_command_pool_create_info.pNext = NULL;
					graphics_command_pool_create_info.flags = 0U;
					graphics_command_pool_create_info.queueFamilyIndex = this->m_queue_graphics_family_index;
					VkResult res_graphics_create_command_pool = pfn_create_command_pool(this->m_device, &graphics_command_pool_create_info, this->m_allocation_callbacks, &vulkan_streaming_graphics_command_pool);
					assert(VK_SUCCESS == res_graphics_create_command_pool);

					VkCommandBufferAllocateInfo graphics_command_buffer_allocate_info;
					graphics_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					graphics_command_buffer_allocate_info.pNext = NULL;
					graphics_command_buffer_allocate_info.commandPool = vulkan_streaming_graphics_command_pool;
					graphics_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					graphics_command_buffer_allocate_info.commandBufferCount = 1U;
					VkResult res_graphics_allocate_command_buffers = pfn_allocate_command_buffers(this->m_device, &graphics_command_buffer_allocate_info, &vulkan_streaming_graphics_command_buffer);
					assert(VK_SUCCESS == res_graphics_allocate_command_buffers);

					VkSemaphoreCreateInfo semaphore_create_info;
					semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
					semaphore_create_info.pNext = NULL;
					semaphore_create_info.flags = 0U;
					VkResult res_create_semaphore = pfn_create_semaphore(this->m_device, &semaphore_create_info, this->m_allocation_callbacks, &vulkan_streaming_semaphore);
					assert(VK_SUCCESS == res_create_semaphore);
				}
				else
				{

					VkCommandPoolCreateInfo transfer_command_pool_create_info;
					transfer_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
					transfer_command_pool_create_info.pNext = NULL;
					transfer_command_pool_create_info.flags = 0U;
					transfer_command_pool_create_info.queueFamilyIndex = this->m_queue_transfer_family_index;
					VkResult res_transfer_create_command_pool = pfn_create_command_pool(this->m_device, &transfer_command_pool_create_info, this->m_allocation_callbacks, &vulkan_streaming_transfer_command_pool);
					assert(VK_SUCCESS == res_transfer_create_command_pool);

					VkCommandBufferAllocateInfo transfer_command_buffer_allocate_info;
					transfer_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					transfer_command_buffer_allocate_info.pNext = NULL;
					transfer_command_buffer_allocate_info.commandPool = vulkan_streaming_transfer_command_pool;
					transfer_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					transfer_command_buffer_allocate_info.commandBufferCount = 1U;
					VkResult res_transfer_allocate_command_buffers = pfn_allocate_command_buffers(this->m_device, &transfer_command_buffer_allocate_info, &vulkan_streaming_transfer_command_buffer);
					assert(VK_SUCCESS == res_transfer_allocate_command_buffers);
				}
			}
			else
			{

				VkCommandPoolCreateInfo graphics_command_pool_create_info;
				graphics_command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				graphics_command_pool_create_info.pNext = NULL;
				graphics_command_pool_create_info.flags = 0U;
				graphics_command_pool_create_info.queueFamilyIndex = this->m_queue_graphics_family_index;
				VkResult res_graphics_create_command_pool = pfn_create_command_pool(this->m_device, &graphics_command_pool_create_info, this->m_allocation_callbacks, &vulkan_streaming_graphics_command_pool);
				assert(VK_SUCCESS == res_graphics_create_command_pool);

				VkCommandBufferAllocateInfo graphics_command_buffer_allocate_info;
				graphics_command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				graphics_command_buffer_allocate_info.pNext = NULL;
				graphics_command_buffer_allocate_info.commandPool = vulkan_streaming_graphics_command_pool;
				graphics_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				graphics_command_buffer_allocate_info.commandBufferCount = 1U;
				VkResult res_graphics_allocate_command_buffers = pfn_allocate_command_buffers(this->m_device, &graphics_command_buffer_allocate_info, &vulkan_streaming_graphics_command_buffer);
				assert(VK_SUCCESS == res_graphics_allocate_command_buffers);
			}

			VkFenceCreateInfo fence_create_info;
			fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_create_info.pNext = NULL;
			fence_create_info.flags = 0U;
			VkResult res_create_fence = pfn_create_fence(this->m_device, &fence_create_info, this->m_allocation_callbacks, &vulkan_streaming_fence);
			assert(VK_SUCCESS == res_create_fence);
		}

		VkDeviceSize vulkan_staging_buffer_size;
		VkBuffer vulkan_staging_buffer;
		VkDeviceMemory vulkan_staging_buffer_device_memory;
		void *vulkan_staging_buffer_device_memory_pointer;
		uint32_t vulkan_asset_vertex_buffer_memory_index;
		uint32_t vulkan_asset_index_buffer_memory_index;
		uint32_t vulkan_asset_uniform_buffer_memory_index;
		uint32_t vulkan_asset_image_memory_index;
		uint32_t vulkan_depth_stencil_sampled_memory_index;
		{
			PFN_vkGetPhysicalDeviceMemoryProperties pfn_get_physical_device_memory_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceMemoryProperties"));
			PFN_vkCreateBuffer pfn_create_buffer = reinterpret_cast<PFN_vkCreateBuffer>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateBuffer"));
			PFN_vkGetBufferMemoryRequirements pfn_get_buffer_memory_requirements = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetBufferMemoryRequirements"));
			PFN_vkAllocateMemory pfn_allocate_memory = reinterpret_cast<PFN_vkAllocateMemory>(this->m_pfn_get_device_proc_addr(this->m_device, "vkAllocateMemory"));
			PFN_vkMapMemory pfn_map_memory = reinterpret_cast<PFN_vkMapMemory>(this->m_pfn_get_device_proc_addr(this->m_device, "vkMapMemory"));
			PFN_vkBindBufferMemory pfn_bind_buffer_memory = reinterpret_cast<PFN_vkBindBufferMemory>(this->m_pfn_get_device_proc_addr(this->m_device, "vkBindBufferMemory"));
			PFN_vkDestroyBuffer pfn_destroy_buffer = reinterpret_cast<PFN_vkDestroyBuffer>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyBuffer"));
			PFN_vkGetPhysicalDeviceFormatProperties pfn_get_physical_device_format_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceFormatProperties"));
			PFN_vkCreateImage pfn_create_image = reinterpret_cast<PFN_vkCreateImage>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateImage"));
			PFN_vkGetImageMemoryRequirements pfn_get_image_memory_requirements = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetImageMemoryRequirements"));
			PFN_vkDestroyImage pfn_destroy_image = reinterpret_cast<PFN_vkDestroyImage>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyImage"));

			VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
			pfn_get_physical_device_memory_properties(this->m_physical_device, &physical_device_memory_properties);

			// staging buffer
			vulkan_staging_buffer_size = 64ULL * 1024ULL * 1024ULL;
			{
				vulkan_staging_buffer = VK_NULL_HANDLE;
				VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
				uint32_t memory_requirements_memory_type_bits = 0U;
				{
					struct VkBufferCreateInfo buffer_create_info;
					buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					buffer_create_info.pNext = NULL;
					buffer_create_info.flags = 0U;
					buffer_create_info.size = vulkan_staging_buffer_size;
					buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
					buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					buffer_create_info.queueFamilyIndexCount = 0U;
					buffer_create_info.pQueueFamilyIndices = NULL;

					VkResult res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &vulkan_staging_buffer);
					assert(VK_SUCCESS == res_create_buffer);

					struct VkMemoryRequirements memory_requirements;
					pfn_get_buffer_memory_requirements(this->m_device, vulkan_staging_buffer, &memory_requirements);
					memory_requirements_size = memory_requirements.size;
					memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;
				}

				// Do NOT use "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT"
				// We leave the "AMD Special Pool" for upload ring buffer
				uint32_t vulkan_staging_buffer_memory_index = __internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				assert(VK_MAX_MEMORY_TYPES > vulkan_staging_buffer_memory_index);
				assert(physical_device_memory_properties.memoryTypeCount > vulkan_staging_buffer_memory_index);

				vulkan_staging_buffer_device_memory = VK_NULL_HANDLE;
				{
					VkMemoryAllocateInfo memory_allocate_info;
					memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
					memory_allocate_info.pNext = NULL;
					memory_allocate_info.allocationSize = memory_requirements_size;
					memory_allocate_info.memoryTypeIndex = vulkan_staging_buffer_memory_index;
					VkResult res_allocate_memory = pfn_allocate_memory(this->m_device, &memory_allocate_info, this->m_allocation_callbacks, &vulkan_staging_buffer_device_memory);
					assert(VK_SUCCESS == res_allocate_memory);
				}

				VkResult res_map_memory = pfn_map_memory(this->m_device, vulkan_staging_buffer_device_memory, 0U, vulkan_staging_buffer_size, 0U, &vulkan_staging_buffer_device_memory_pointer);
				assert(VK_SUCCESS == res_map_memory);

				VkResult res_bind_buffer_memory = pfn_bind_buffer_memory(this->m_device, vulkan_staging_buffer, vulkan_staging_buffer_device_memory, 0U);
				assert(VK_SUCCESS == res_bind_buffer_memory);
			}

			// https://www.khronos.org/registry/vulkan/specs/1.0/html/chap13.html#VkMemoryRequirements
			// If buffer is a VkBuffer not created with the VK_BUFFER_CREATE_SPARSE_BINDING_BIT bit set, or if image is linear image,
			// then the memoryTypeBits member always contains at least one bit set corresponding to a VkMemoryType with a
			// propertyFlags that has both the VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT bit and the
			// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit set. In other words, mappable coherent memory can always be attached to
			// these objects.

			// https://www.khronos.org/registry/vulkan/specs/1.0/html/chap13.html#VkMemoryRequirements
			// The memoryTypeBits member is identical for all VkBuffer objects created with the same value for the flags and usage
			// members in the VkBufferCreateInfo structure passed to vkCreateBuffer. Further, if usage1 and usage2 of type
			// VkBufferUsageFlags are such that the bits set in usage2 are a subset of the bits set in usage1, and they have the same flags,
			// then the bits set in memoryTypeBits returned for usage1 must be a subset of the bits set in memoryTypeBits returned for
			// usage2, for all values of flags.

			// asset vertex buffer
			vulkan_asset_vertex_buffer_memory_index = VK_MAX_MEMORY_TYPES;
			{
				VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
				uint32_t memory_requirements_memory_type_bits = 0U;
				{
					struct VkBufferCreateInfo buffer_create_info;
					buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					buffer_create_info.pNext = NULL;
					buffer_create_info.flags = 0U;
					buffer_create_info.size = 8U;
					buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
					buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					buffer_create_info.queueFamilyIndexCount = 0U;
					buffer_create_info.pQueueFamilyIndices = NULL;

					VkBuffer dummy_buf;
					VkResult res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
					assert(VK_SUCCESS == res_create_buffer);

					struct VkMemoryRequirements memory_requirements;
					pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
					memory_requirements_size = memory_requirements.size;
					memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

					pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
				}

				// NOT HOST_VISIBLE
				// The UMA driver may compress the buffer/texture to boost performance
				vulkan_asset_vertex_buffer_memory_index = __internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				assert(VK_MAX_MEMORY_TYPES > vulkan_asset_vertex_buffer_memory_index);
				assert(physical_device_memory_properties.memoryTypeCount > vulkan_asset_vertex_buffer_memory_index);
			}

			// asset index buffer
			vulkan_asset_index_buffer_memory_index = VK_MAX_MEMORY_TYPES;
			{
				VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
				uint32_t memory_requirements_memory_type_bits = 0U;
				{
					struct VkBufferCreateInfo buffer_create_info;
					buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					buffer_create_info.pNext = NULL;
					buffer_create_info.flags = 0U;
					buffer_create_info.size = 8U;
					buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
					buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					buffer_create_info.queueFamilyIndexCount = 0U;
					buffer_create_info.pQueueFamilyIndices = NULL;

					VkBuffer dummy_buf;
					VkResult res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
					assert(VK_SUCCESS == res_create_buffer);

					struct VkMemoryRequirements memory_requirements;
					pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
					memory_requirements_size = memory_requirements.size;
					memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

					pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
				}

				// NOT HOST_VISIBLE
				// The UMA driver may compress the buffer/texture to boost performance
				vulkan_asset_index_buffer_memory_index = __internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				assert(VK_MAX_MEMORY_TYPES > vulkan_asset_index_buffer_memory_index);
				assert(physical_device_memory_properties.memoryTypeCount > vulkan_asset_index_buffer_memory_index);
			}

			// asset uniform buffer
			vulkan_asset_uniform_buffer_memory_index = VK_MAX_MEMORY_TYPES;
			{
				VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
				uint32_t memory_requirements_memory_type_bits = 0U;
				{
					struct VkBufferCreateInfo buffer_create_info;
					buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					buffer_create_info.pNext = NULL;
					buffer_create_info.flags = 0U;
					buffer_create_info.size = 8U;
					buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
					buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					buffer_create_info.queueFamilyIndexCount = 0U;
					buffer_create_info.pQueueFamilyIndices = NULL;

					VkBuffer dummy_buf;
					VkResult res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
					assert(VK_SUCCESS == res_create_buffer);

					struct VkMemoryRequirements memory_requirements;
					pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
					memory_requirements_size = memory_requirements.size;
					memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

					pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
				}

				// NOT HOST_VISIBLE
				// The UMA driver may compress the buffer/texture to boost performance
				vulkan_asset_uniform_buffer_memory_index = __internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				assert(VK_MAX_MEMORY_TYPES > vulkan_asset_uniform_buffer_memory_index);
				assert(physical_device_memory_properties.memoryTypeCount > vulkan_asset_uniform_buffer_memory_index);
			}

			// https://www.khronos.org/registry/vulkan/specs/1.0/html/chap13.html#VkMemoryRequirements
			// For images created with a color format, the memoryTypeBits member is identical for all VkImage objects created with the
			// same combination of values for the tiling member, the VK_IMAGE_CREATE_SPARSE_BINDING_BIT bit of the flags member, and
			// the VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT of the usage member in the VkImageCreateInfo structure passed to
			// vkCreateImage.

			// vulkaninfo
			// https://github.com/KhronosGroup/Vulkan-Tools/tree/master/vulkaninfo/vulkaninfo/vulkaninfo.h
			// GetImageCreateInfo
			// FillImageTypeSupport
			// https://github.com/KhronosGroup/Vulkan-Tools/tree/master/vulkaninfo/vulkaninfo.cpp
			// GpuDumpMemoryProps //"usable for"

			vulkan_asset_image_memory_index = VK_MAX_MEMORY_TYPES;
			{
				VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
				uint32_t memory_requirements_memory_type_bits = 0U;
				{
					VkFormat color_format = VK_FORMAT_R8G8B8A8_UNORM;

					struct VkFormatProperties format_properties;
					pfn_get_physical_device_format_properties(this->m_physical_device, color_format, &format_properties);
					assert(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
					assert(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT);
					assert(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
					assert(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

					struct VkImageCreateInfo image_create_info_regular_tiling_optimal;
					image_create_info_regular_tiling_optimal.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
					image_create_info_regular_tiling_optimal.pNext = NULL;
					image_create_info_regular_tiling_optimal.flags = 0U;
					image_create_info_regular_tiling_optimal.imageType = VK_IMAGE_TYPE_2D;
					image_create_info_regular_tiling_optimal.format = color_format;
					image_create_info_regular_tiling_optimal.extent.width = 8U;
					image_create_info_regular_tiling_optimal.extent.height = 8U;
					image_create_info_regular_tiling_optimal.extent.depth = 1U;
					image_create_info_regular_tiling_optimal.mipLevels = 1U;
					image_create_info_regular_tiling_optimal.arrayLayers = 1U;
					image_create_info_regular_tiling_optimal.samples = VK_SAMPLE_COUNT_1_BIT;
					image_create_info_regular_tiling_optimal.tiling = VK_IMAGE_TILING_OPTIMAL;
					image_create_info_regular_tiling_optimal.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
					image_create_info_regular_tiling_optimal.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					image_create_info_regular_tiling_optimal.queueFamilyIndexCount = 0U;
					image_create_info_regular_tiling_optimal.pQueueFamilyIndices = NULL;
					image_create_info_regular_tiling_optimal.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

					VkImage dummy_img;
					VkResult res_create_image = pfn_create_image(this->m_device, &image_create_info_regular_tiling_optimal, this->m_allocation_callbacks, &dummy_img);
					assert(VK_SUCCESS == res_create_image);

					struct VkMemoryRequirements memory_requirements;
					pfn_get_image_memory_requirements(this->m_device, dummy_img, &memory_requirements);
					memory_requirements_size = memory_requirements.size;
					memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

					pfn_destroy_image(this->m_device, dummy_img, this->m_allocation_callbacks);
				}

				vulkan_asset_image_memory_index = __internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				assert(VK_MAX_MEMORY_TYPES > vulkan_asset_image_memory_index);
				assert(physical_device_memory_properties.memoryTypeCount > vulkan_asset_image_memory_index);
			}

			vulkan_depth_stencil_sampled_memory_index = VK_MAX_MEMORY_TYPES;
			{

				VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
				uint32_t memory_requirements_memory_type_bits = 0U;
				{
					struct VkImageCreateInfo image_create_info_depth_stencil_sampled_tiling_optimal;
					image_create_info_depth_stencil_sampled_tiling_optimal.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
					image_create_info_depth_stencil_sampled_tiling_optimal.pNext = NULL;
					image_create_info_depth_stencil_sampled_tiling_optimal.flags = 0U;
					image_create_info_depth_stencil_sampled_tiling_optimal.imageType = VK_IMAGE_TYPE_2D;
					image_create_info_depth_stencil_sampled_tiling_optimal.format = this->m_depth_format;
					image_create_info_depth_stencil_sampled_tiling_optimal.extent.width = 8U;
					image_create_info_depth_stencil_sampled_tiling_optimal.extent.height = 8U;
					image_create_info_depth_stencil_sampled_tiling_optimal.extent.depth = 1U;
					image_create_info_depth_stencil_sampled_tiling_optimal.mipLevels = 1U;
					image_create_info_depth_stencil_sampled_tiling_optimal.arrayLayers = 1U;
					image_create_info_depth_stencil_sampled_tiling_optimal.samples = VK_SAMPLE_COUNT_1_BIT;
					image_create_info_depth_stencil_sampled_tiling_optimal.tiling = VK_IMAGE_TILING_OPTIMAL;
					image_create_info_depth_stencil_sampled_tiling_optimal.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
					image_create_info_depth_stencil_sampled_tiling_optimal.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					image_create_info_depth_stencil_sampled_tiling_optimal.queueFamilyIndexCount = 0U;
					image_create_info_depth_stencil_sampled_tiling_optimal.pQueueFamilyIndices = NULL;
					image_create_info_depth_stencil_sampled_tiling_optimal.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

					VkImage dummy_img;
					VkResult res_create_image = pfn_create_image(this->m_device, &image_create_info_depth_stencil_sampled_tiling_optimal, this->m_allocation_callbacks, &dummy_img);
					assert(VK_SUCCESS == res_create_image);

					struct VkMemoryRequirements memory_requirements;
					pfn_get_image_memory_requirements(this->m_device, dummy_img, &memory_requirements);
					memory_requirements_size = memory_requirements.size;
					memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

					pfn_destroy_image(this->m_device, dummy_img, this->m_allocation_callbacks);
				}

				// The lower index indicates the more performance
				vulkan_depth_stencil_sampled_memory_index = __internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				assert(VK_MAX_MEMORY_TYPES > vulkan_depth_stencil_sampled_memory_index);
				assert(physical_device_memory_properties.memoryTypeCount > vulkan_depth_stencil_sampled_memory_index);
			}
		}

		if (this->m_has_dedicated_transfer_queue)
		{
			if (this->m_queue_graphics_family_index != this->m_queue_transfer_family_index)
			{
				VkResult res_reset_transfer_command_pool = this->m_pfn_reset_command_pool(this->m_device, vulkan_streaming_transfer_command_pool, 0U);
				assert(VK_SUCCESS == res_reset_transfer_command_pool);

				VkCommandBufferBeginInfo vulkan_transfer_command_buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL};
				VkResult res_begin_transfer_command_buffer = this->m_pfn_begin_command_buffer(vulkan_streaming_transfer_command_buffer, &vulkan_transfer_command_buffer_begin_info);
				assert(VK_SUCCESS == res_begin_transfer_command_buffer);

				VkResult res_reset_graphics_command_pool = this->m_pfn_reset_command_pool(this->m_device, vulkan_streaming_graphics_command_pool, 0U);
				assert(VK_SUCCESS == res_reset_graphics_command_pool);

				VkCommandBufferBeginInfo vulkan_graphics_command_buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL};
				VkResult res_begin_graphics_command_buffer = this->m_pfn_begin_command_buffer(vulkan_streaming_graphics_command_buffer, &vulkan_graphics_command_buffer_begin_info);
				assert(VK_SUCCESS == res_begin_graphics_command_buffer);
			}
			else
			{
				VkResult res_reset_transfer_command_pool = this->m_pfn_reset_command_pool(this->m_device, vulkan_streaming_transfer_command_pool, 0U);
				assert(VK_SUCCESS == res_reset_transfer_command_pool);

				VkCommandBufferBeginInfo vulkan_transfer_command_buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL};
				VkResult res_begin_transfer_command_buffer = this->m_pfn_begin_command_buffer(vulkan_streaming_transfer_command_buffer, &vulkan_transfer_command_buffer_begin_info);
				assert(VK_SUCCESS == res_begin_transfer_command_buffer);
			}
		}
		else
		{
			VkResult res_reset_graphics_command_pool = this->m_pfn_reset_command_pool(this->m_device, vulkan_streaming_graphics_command_pool, 0U);
			assert(VK_SUCCESS == res_reset_graphics_command_pool);

			VkCommandBufferBeginInfo vulkan_graphics_command_buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL};
			VkResult res_begin_graphics_command_buffer = this->m_pfn_begin_command_buffer(vulkan_streaming_graphics_command_buffer, &vulkan_graphics_command_buffer_begin_info);
			assert(VK_SUCCESS == res_begin_graphics_command_buffer);
		}

		assert(vulkan_staging_buffer_size < static_cast<VkDeviceSize>(UINT32_MAX));
		this->m_demo.init(
			this->m_instance, this->m_pfn_get_instance_proc_addr, this->m_device, this->m_pfn_get_device_proc_addr, this->m_allocation_callbacks,
			this->m_asset_allocator, 0U, static_cast<uint32_t>(vulkan_staging_buffer_size), vulkan_staging_buffer_device_memory_pointer, vulkan_staging_buffer,
			vulkan_asset_vertex_buffer_memory_index, vulkan_asset_index_buffer_memory_index, vulkan_asset_uniform_buffer_memory_index, vulkan_asset_image_memory_index, this->m_depth_format, vulkan_depth_stencil_sampled_memory_index,
			this->m_optimal_buffer_copy_offset_alignment, this->m_optimal_buffer_copy_row_pitch_alignment,
			this->m_has_dedicated_transfer_queue, this->m_queue_transfer_family_index, this->m_queue_graphics_family_index, vulkan_streaming_transfer_command_buffer, vulkan_streaming_graphics_command_buffer,
			this->m_upload_ring_buffer_size, this->m_upload_ring_buffer);

		if (this->m_has_dedicated_transfer_queue)
		{
			if (this->m_queue_graphics_family_index != this->m_queue_transfer_family_index)
			{
				VkResult res_end_transfer_command_buffer = this->m_pfn_end_command_buffer(vulkan_streaming_transfer_command_buffer);
				assert(VK_SUCCESS == res_end_transfer_command_buffer);

				VkResult res_end_graphics_command_buffer = this->m_pfn_end_command_buffer(vulkan_streaming_graphics_command_buffer);
				assert(VK_SUCCESS == res_end_graphics_command_buffer);

				VkSubmitInfo submit_info_transfer;
				submit_info_transfer.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info_transfer.pNext = NULL;
				submit_info_transfer.waitSemaphoreCount = 0U;
				submit_info_transfer.pWaitSemaphores = NULL;
				submit_info_transfer.pWaitDstStageMask = NULL;
				submit_info_transfer.commandBufferCount = 1U;
				submit_info_transfer.pCommandBuffers = &vulkan_streaming_transfer_command_buffer;
				submit_info_transfer.signalSemaphoreCount = 1U;
				submit_info_transfer.pSignalSemaphores = &vulkan_streaming_semaphore;
				VkResult res_transfer_queue_submit = this->m_pfn_queue_submit(this->m_queue_transfer, 1U, &submit_info_transfer, VK_NULL_HANDLE);
				assert(VK_SUCCESS == res_transfer_queue_submit);

				VkSubmitInfo submit_info_graphics;
				submit_info_graphics.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info_graphics.pNext = NULL;
				submit_info_graphics.waitSemaphoreCount = 1U;
				submit_info_graphics.pWaitSemaphores = &vulkan_streaming_semaphore;
				VkPipelineStageFlags wait_dst_stage_mask[1] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
				submit_info_graphics.pWaitDstStageMask = wait_dst_stage_mask;
				submit_info_graphics.commandBufferCount = 1U;
				submit_info_graphics.pCommandBuffers = &vulkan_streaming_graphics_command_buffer;
				submit_info_graphics.signalSemaphoreCount = 0U;
				submit_info_graphics.pSignalSemaphores = NULL;
				// queue family ownership transfer - acquire operation
				VkResult res_graphics_queue_submit = this->m_pfn_queue_submit(this->m_queue_graphics, 1U, &submit_info_graphics, vulkan_streaming_fence);
				assert(VK_SUCCESS == res_graphics_queue_submit);
			}
			else
			{
				VkResult res_end_transfer_command_buffer = this->m_pfn_end_command_buffer(vulkan_streaming_transfer_command_buffer);
				assert(VK_SUCCESS == res_end_transfer_command_buffer);

				VkSubmitInfo submit_info_transfer;
				submit_info_transfer.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit_info_transfer.pNext = NULL;
				submit_info_transfer.waitSemaphoreCount = 0U;
				submit_info_transfer.pWaitSemaphores = NULL;
				submit_info_transfer.pWaitDstStageMask = NULL;
				submit_info_transfer.commandBufferCount = 1U;
				submit_info_transfer.pCommandBuffers = &vulkan_streaming_transfer_command_buffer;
				submit_info_transfer.signalSemaphoreCount = 0U;
				submit_info_transfer.pSignalSemaphores = NULL;
				VkResult res_transfer_queue_submit = this->m_pfn_queue_submit(this->m_queue_transfer, 1U, &submit_info_transfer, vulkan_streaming_fence);
				assert(VK_SUCCESS == res_transfer_queue_submit);
			}
		}
		else
		{
			VkResult res_end_graphics_command_buffer = this->m_pfn_end_command_buffer(vulkan_streaming_graphics_command_buffer);
			assert(VK_SUCCESS == res_end_graphics_command_buffer);

			VkSubmitInfo submit_info_graphics;
			submit_info_graphics.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info_graphics.pNext = NULL;
			submit_info_graphics.waitSemaphoreCount = 0U;
			submit_info_graphics.pWaitSemaphores = NULL;
			submit_info_graphics.pWaitDstStageMask = NULL;
			submit_info_graphics.commandBufferCount = 1U;
			submit_info_graphics.pCommandBuffers = &vulkan_streaming_graphics_command_buffer;
			submit_info_graphics.signalSemaphoreCount = 0U;
			submit_info_graphics.pSignalSemaphores = NULL;
			VkResult res_graphics_queue_submit = this->m_pfn_queue_submit(this->m_queue_graphics, 1U, &submit_info_graphics, vulkan_streaming_fence);
			assert(VK_SUCCESS == res_graphics_queue_submit);
		}

		VkResult res_wait_for_fences = this->m_pfn_wait_for_fences(this->m_device, 1U, &vulkan_streaming_fence, VK_TRUE, UINT64_MAX);
		assert(VK_SUCCESS == res_wait_for_fences);

		// free staging buffer
		{
			PFN_vkUnmapMemory pfn_unmap_memory = reinterpret_cast<PFN_vkUnmapMemory>(this->m_pfn_get_device_proc_addr(this->m_device, "vkUnmapMemory"));
			PFN_vkDestroyBuffer pfn_destroy_buffer = reinterpret_cast<PFN_vkDestroyBuffer>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyBuffer"));
			PFN_vkFreeMemory pfn_free_memory = reinterpret_cast<PFN_vkFreeMemory>(this->m_pfn_get_device_proc_addr(this->m_device, "vkFreeMemory"));

			pfn_unmap_memory(this->m_device, vulkan_staging_buffer_device_memory);
			pfn_destroy_buffer(this->m_device, vulkan_staging_buffer, this->m_allocation_callbacks);
			pfn_free_memory(this->m_device, vulkan_staging_buffer_device_memory, this->m_allocation_callbacks);
		}

		// destory fence/semaphore/commandbuffer/commandpool
		{
			PFN_vkDestroyCommandPool pfn_destroy_command_pool = reinterpret_cast<PFN_vkDestroyCommandPool>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyCommandPool"));
			assert(NULL != pfn_destroy_command_pool);
			PFN_vkFreeCommandBuffers pfn_free_command_buffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(this->m_pfn_get_device_proc_addr(this->m_device, "vkFreeCommandBuffers"));
			assert(NULL != pfn_free_command_buffers);
			PFN_vkDestroyFence pfn_destroy_fence = reinterpret_cast<PFN_vkDestroyFence>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyFence"));
			assert(NULL != pfn_destroy_fence);
			PFN_vkDestroySemaphore pfn_destroy_semaphore = reinterpret_cast<PFN_vkDestroySemaphore>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroySemaphore"));
			assert(NULL != pfn_destroy_semaphore);

			pfn_destroy_fence(this->m_device, vulkan_streaming_fence, this->m_allocation_callbacks);

			if (this->m_has_dedicated_transfer_queue)
			{
				if (this->m_queue_graphics_family_index != this->m_queue_transfer_family_index)
				{
					pfn_free_command_buffers(this->m_device, vulkan_streaming_graphics_command_pool, 1U, &vulkan_streaming_graphics_command_buffer);

					pfn_destroy_command_pool(this->m_device, vulkan_streaming_graphics_command_pool, this->m_allocation_callbacks);

					pfn_free_command_buffers(this->m_device, vulkan_streaming_transfer_command_pool, 1U, &vulkan_streaming_transfer_command_buffer);

					pfn_destroy_command_pool(this->m_device, vulkan_streaming_transfer_command_pool, this->m_allocation_callbacks);

					pfn_destroy_semaphore(this->m_device, vulkan_streaming_semaphore, this->m_allocation_callbacks);
				}
				else
				{
					pfn_free_command_buffers(this->m_device, vulkan_streaming_transfer_command_pool, 1U, &vulkan_streaming_transfer_command_buffer);

					pfn_destroy_command_pool(this->m_device, vulkan_streaming_transfer_command_pool, this->m_allocation_callbacks);
				}
			}
			else
			{
				pfn_free_command_buffers(this->m_device, vulkan_streaming_graphics_command_pool, 1U, &vulkan_streaming_graphics_command_buffer);

				pfn_destroy_command_pool(this->m_device, vulkan_streaming_graphics_command_pool, this->m_allocation_callbacks);
			}
		}
	}

	// window
	this->m_surface = VK_NULL_HANDLE;

	// FrameBuffer
	this->m_framebuffer_width = 0U;
	this->m_framebuffer_height = 0U;
	this->m_swapchain = VK_NULL_HANDLE;
	this->m_swapchain_image_count = 0U;
	assert(0U == this->m_swapchain_image_views.size());

	// Rendering
	this->m_frame_throtting_index = 0U;
};

void vulkan_renderer::attach_window(void *window_void)
{
	assert(VK_NULL_HANDLE == this->m_surface);
#if defined(__GNUC__)
#if defined(__linux__) && defined(__ANDROID__)
	{
		ANativeWindow *native_window = static_cast<ANativeWindow *>(window_void);

		PFN_vkCreateAndroidSurfaceKHR pfn_create_android_surface = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkCreateAndroidSurfaceKHR"));
		assert(NULL != pfn_create_android_surface);

		VkAndroidSurfaceCreateInfoKHR android_surface_create_info;
		android_surface_create_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
		android_surface_create_info.pNext = NULL;
		android_surface_create_info.flags = 0U;
		android_surface_create_info.window = native_window;

		VkResult res_create_android_surface = pfn_create_android_surface(this->m_instance, &android_surface_create_info, this->m_allocation_callbacks, &this->m_surface);
		assert(VK_SUCCESS == res_create_android_surface);
	}
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)

	{
		HWND hWnd = static_cast<HWND>(window_void);

		PFN_vkCreateWin32SurfaceKHR pfn_create_win32_surface = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkCreateWin32SurfaceKHR"));

		VkWin32SurfaceCreateInfoKHR win32_surface_create_info;
		win32_surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		win32_surface_create_info.pNext = NULL;
		win32_surface_create_info.flags = 0U;
		win32_surface_create_info.hinstance = reinterpret_cast<HINSTANCE>(GetClassLongPtrW(hWnd, GCLP_HMODULE));
		win32_surface_create_info.hwnd = hWnd;
		VkResult res_create_win32_surface = pfn_create_win32_surface(this->m_instance, &win32_surface_create_info, this->m_allocation_callbacks, &this->m_surface);
		assert(VK_SUCCESS == res_create_win32_surface);
	}
#else
#error Unknown Compiler
#endif
	{
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR pfn_get_physical_device_surface_support = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
		VkBool32 supported;
		VkResult res_get_physical_device_surface_support = pfn_get_physical_device_surface_support(this->m_physical_device, this->m_queue_graphics_queue_index, this->m_surface, &supported);
		assert(VK_SUCCESS == res_get_physical_device_surface_support);
		assert(VK_FALSE != supported);
	}
}

void vulkan_renderer::dettach_window()
{
	assert(VK_NULL_HANDLE != this->m_surface);

	this->destory_framebuffer();

	PFN_vkDestroySurfaceKHR pfn_destroy_surface = reinterpret_cast<PFN_vkDestroySurfaceKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkDestroySurfaceKHR"));
	pfn_destroy_surface(this->m_instance, this->m_surface, this->m_allocation_callbacks);

	this->m_surface = VK_NULL_HANDLE;
}

void vulkan_renderer::destory_framebuffer()
{
	VkResult res_wait_for_fences = this->m_pfn_wait_for_fences(this->m_device, FRAME_THROTTLING_COUNT, this->m_fences, VK_TRUE, UINT64_MAX);
	assert(VK_SUCCESS == res_wait_for_fences);

	this->m_demo.destroy_main_camera_frame_buffer(
		this->m_device, this->m_pfn_get_device_proc_addr, this->m_allocation_callbacks,
		this->m_swapchain_image_count);

	// Destroy Swapchain
	{
		// Swapchain ImageView
		{
			PFN_vkDestroyImageView pfn_destroy_image_view = reinterpret_cast<PFN_vkDestroyImageView>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyImageView"));

			assert(static_cast<uint32_t>(this->m_swapchain_image_views.size()) == this->m_swapchain_image_count);

			for (uint32_t vulkan_swapchain_image_index = 0U; vulkan_swapchain_image_index < this->m_swapchain_image_count; ++vulkan_swapchain_image_index)
			{
				pfn_destroy_image_view(this->m_device, this->m_swapchain_image_views[vulkan_swapchain_image_index], this->m_allocation_callbacks);
			}
			this->m_swapchain_image_views.clear();
		}

		// SwapChain
		{
			PFN_vkDestroySwapchainKHR pfn_destroy_swapchain = reinterpret_cast<PFN_vkDestroySwapchainKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroySwapchainKHR"));
			pfn_destroy_swapchain(this->m_device, this->m_swapchain, this->m_allocation_callbacks);
			this->m_swapchain = VK_NULL_HANDLE;
		}

		// Count and Extent
		{
			this->m_swapchain_image_count = 0U;
			this->m_framebuffer_width = 0U;
			this->m_framebuffer_height = 0U;
		}
	}
}

void vulkan_renderer::create_framebuffer()
{
	VkFormat vulkan_swapchain_image_format;
	VkCompositeAlphaFlagBitsKHR vulkan_swapchain_composite_alpha;
	{
		// Query Frame Buffer Extent
		vulkan_swapchain_image_format = VK_FORMAT_UNDEFINED;
		{
			PFN_vkGetPhysicalDeviceSurfaceFormatsKHR pfn_get_physical_device_surface_formats = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
			assert(NULL != pfn_get_physical_device_surface_formats);

			uint32_t surface_format_count_1 = uint32_t(-1);
			pfn_get_physical_device_surface_formats(this->m_physical_device, this->m_surface, &surface_format_count_1, NULL);

			VkSurfaceFormatKHR *surface_formats = static_cast<VkSurfaceFormatKHR *>(malloc(sizeof(VkQueueFamilyProperties) * surface_format_count_1));
			assert(NULL != surface_formats);

			uint32_t surface_format_count_2 = surface_format_count_1;
			pfn_get_physical_device_surface_formats(this->m_physical_device, this->m_surface, &surface_format_count_2, surface_formats);
			assert(surface_format_count_1 == surface_format_count_2);

			assert(surface_format_count_2 >= 1U);
			if (VK_FORMAT_UNDEFINED != surface_formats[0].format)
			{
				vulkan_swapchain_image_format = surface_formats[0].format;
			}
			else
			{
				vulkan_swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;
			}

			assert(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == surface_formats[0].colorSpace);

			free(surface_formats);
		}

		assert(0U == this->m_framebuffer_width);
		assert(0U == this->m_framebuffer_height);
		{
			PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pfn_get_physical_device_surface_capabilities = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
			assert(NULL != pfn_get_physical_device_surface_capabilities);

			VkSurfaceCapabilitiesKHR surface_capabilities;
			VkResult res_get_physical_device_surface_capablilities = pfn_get_physical_device_surface_capabilities(this->m_physical_device, this->m_surface, &surface_capabilities);
			assert(VK_SUCCESS == res_get_physical_device_surface_capablilities);

			if (surface_capabilities.currentExtent.width != 0XFFFFFFFFU)
			{
				this->m_framebuffer_width = surface_capabilities.currentExtent.width;
			}
			else
			{
				this->m_framebuffer_width = g_preferred_resolution_width;
			}
			this->m_framebuffer_width = (this->m_framebuffer_width < surface_capabilities.minImageExtent.width) ? surface_capabilities.minImageExtent.width : (this->m_framebuffer_width > surface_capabilities.maxImageExtent.width) ? surface_capabilities.maxImageExtent.width
																																																									  : this->m_framebuffer_width;

			if (surface_capabilities.currentExtent.height != 0XFFFFFFFFU)
			{
				this->m_framebuffer_height = surface_capabilities.currentExtent.height;
			}
			else
			{
				this->m_framebuffer_height = g_preferred_resolution_height;
			}
			this->m_framebuffer_height = (this->m_framebuffer_height < surface_capabilities.minImageExtent.height) ? surface_capabilities.minImageExtent.height : (this->m_framebuffer_height > surface_capabilities.maxImageExtent.height) ? surface_capabilities.maxImageExtent.height
																																																											: this->m_framebuffer_height;

			// Test on Android
			// We should use identity even if the surface is rotation 90
			assert(VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR == (VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR & surface_capabilities.supportedTransforms));

			VkCompositeAlphaFlagBitsKHR composite_alphas[] = {
				VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
				VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
				VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};

			for (VkCompositeAlphaFlagBitsKHR composite_alpha : composite_alphas)
			{
				if (0 != (composite_alpha & surface_capabilities.supportedCompositeAlpha))
				{
					vulkan_swapchain_composite_alpha = composite_alpha;
					break;
				}
			}
		}
	}

	if (0U != this->m_framebuffer_width && 0U != this->m_framebuffer_height)
	{
		// Create Swap Chain
		assert(VK_NULL_HANDLE == this->m_swapchain);
		{
			PFN_vkCreateSwapchainKHR pfn_create_swapchain = reinterpret_cast<PFN_vkCreateSwapchainKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateSwapchainKHR"));
			assert(NULL != pfn_create_swapchain);

			VkSwapchainCreateInfoKHR swapchain_create_info;
			swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchain_create_info.pNext = NULL;
			swapchain_create_info.flags = 0U;
			swapchain_create_info.surface = this->m_surface;
			swapchain_create_info.minImageCount = FRAME_THROTTLING_COUNT;
			swapchain_create_info.imageFormat = vulkan_swapchain_image_format;
			swapchain_create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			swapchain_create_info.imageExtent.width = this->m_framebuffer_width;
			swapchain_create_info.imageExtent.height = this->m_framebuffer_height;
			swapchain_create_info.imageArrayLayers = 1U;
			swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchain_create_info.queueFamilyIndexCount = 0U;
			swapchain_create_info.pQueueFamilyIndices = NULL;
			swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			swapchain_create_info.compositeAlpha = vulkan_swapchain_composite_alpha;
			swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
			swapchain_create_info.clipped = VK_FALSE;
			swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

			VkResult res_create_swapchain = pfn_create_swapchain(this->m_device, &swapchain_create_info, this->m_allocation_callbacks, &this->m_swapchain);
			assert(VK_SUCCESS == res_create_swapchain);
		}

		assert(0U == this->m_swapchain_image_count);
		{
			PFN_vkGetSwapchainImagesKHR pfn_get_swapchain_images = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetSwapchainImagesKHR"));
			assert(NULL != pfn_get_swapchain_images);

			uint32_t swapchain_image_count_1 = uint32_t(-1);
			VkResult res_get_swapchain_images_1 = pfn_get_swapchain_images(this->m_device, this->m_swapchain, &swapchain_image_count_1, NULL);
			assert(VK_SUCCESS == res_get_swapchain_images_1);

			this->m_swapchain_images.resize(swapchain_image_count_1);

			uint32_t swapchain_image_count_2 = swapchain_image_count_1;
			VkResult res_get_swapchain_images_2 = pfn_get_swapchain_images(this->m_device, this->m_swapchain, &swapchain_image_count_2, &this->m_swapchain_images[0]);
			assert(VK_SUCCESS == res_get_swapchain_images_2 && swapchain_image_count_2 == swapchain_image_count_1);

			this->m_swapchain_image_count = swapchain_image_count_2;
		}

		assert(0U == this->m_swapchain_image_views.size());
		{
			this->m_swapchain_image_views.resize(this->m_swapchain_image_count);

			PFN_vkCreateImageView pfn_create_image_view = reinterpret_cast<PFN_vkCreateImageView>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateImageView"));
			assert(NULL != pfn_create_image_view);

			for (uint32_t vulkan_swapchain_image_index = 0U; vulkan_swapchain_image_index < this->m_swapchain_image_count; ++vulkan_swapchain_image_index)
			{
				VkImageViewCreateInfo image_view_create_info = {
					VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					NULL,
					0U,
					this->m_swapchain_images[vulkan_swapchain_image_index],
					VK_IMAGE_VIEW_TYPE_2D,
					vulkan_swapchain_image_format,
					{VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
					{VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U}};

				VkResult res_create_image_view = pfn_create_image_view(this->m_device, &image_view_create_info, this->m_allocation_callbacks, &this->m_swapchain_image_views[vulkan_swapchain_image_index]);
				assert(VK_SUCCESS == res_create_image_view);
			}
		}

		this->m_demo.create_main_camera_frame_buffer(
			this->m_instance, this->m_pfn_get_instance_proc_addr, this->m_physical_device, this->m_device, this->m_pfn_get_device_proc_addr, this->m_allocation_callbacks,
			this->m_depth_format, this->m_depth_stencil_transient_attachment_memory_index,
			vulkan_swapchain_image_format,
			this->m_framebuffer_width, this->m_framebuffer_height,
			this->m_swapchain,
			this->m_swapchain_image_count,
			this->m_swapchain_image_views);
	}
}

void vulkan_renderer::draw()
{
	if (0U == this->m_framebuffer_width || 0U == this->m_framebuffer_height)
	{
		if (VK_NULL_HANDLE != this->m_surface)
		{
			this->create_framebuffer();
		}

		// skip current frame
		return;
	}

	VkCommandPool vulkan_command_pool = this->m_command_pools[this->m_frame_throtting_index];
	VkCommandBuffer vulkan_command_buffer = this->m_command_buffers[this->m_frame_throtting_index];
	VkFence vulkan_fence = this->m_fences[this->m_frame_throtting_index];
	VkSemaphore vulkan_semaphore_acquire_next_image = this->m_semaphores_acquire_next_image[this->m_frame_throtting_index];
	VkSemaphore vulkan_semaphore_queue_submit = this->m_semaphores_queue_submit[this->m_frame_throtting_index];

	uint32_t vulkan_swapchain_image_index = uint32_t(-1);
	VkResult res_acquire_next_image = this->m_pfn_acquire_next_image(this->m_device, this->m_swapchain, UINT64_MAX, vulkan_semaphore_acquire_next_image, VK_NULL_HANDLE, &vulkan_swapchain_image_index);
	bool sub_optimal_acquire_next_image = false;
	if (VK_SUCCESS != res_acquire_next_image)
	{
		assert(VK_SUBOPTIMAL_KHR == res_acquire_next_image || VK_ERROR_OUT_OF_DATE_KHR == res_acquire_next_image);

		if (VK_SUBOPTIMAL_KHR != res_acquire_next_image)
		{
			this->destory_framebuffer();

			if (VK_NULL_HANDLE != this->m_surface)
			{
				this->create_framebuffer();
			}

			// skip current frame
			return;
		}
		else
		{
			// we still render this frame, but we will update the framebuffer after present
			sub_optimal_acquire_next_image = true;
		}
	}

	VkResult res_wait_for_fences = this->m_pfn_wait_for_fences(this->m_device, 1U, &vulkan_fence, VK_TRUE, UINT64_MAX);
	assert(VK_SUCCESS == res_wait_for_fences);

	VkResult res_reset_command_pool = this->m_pfn_reset_command_pool(this->m_device, vulkan_command_pool, 0U);
	assert(VK_SUCCESS == res_reset_command_pool);

	VkCommandBufferBeginInfo vulkan_command_buffer_begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, NULL};
	VkResult res_begin_command_buffer = this->m_pfn_begin_command_buffer(vulkan_command_buffer, &vulkan_command_buffer_begin_info);
	assert(VK_SUCCESS == res_begin_command_buffer);

	this->m_upload_ring_buffer_current[this->m_frame_throtting_index] = this->m_upload_ring_buffer_begin[this->m_frame_throtting_index];

	this->m_demo.tick(vulkan_command_buffer, vulkan_swapchain_image_index, this->m_framebuffer_width, this->m_framebuffer_height,
					  this->m_upload_ring_buffer_device_memory_pointer, this->m_upload_ring_buffer_current[this->m_frame_throtting_index], this->m_upload_ring_buffer_end[this->m_frame_throtting_index],
					  this->m_min_uniform_buffer_offset_alignment);

	VkResult res_end_command_buffer = this->m_pfn_end_command_buffer(vulkan_command_buffer);
	assert(VK_SUCCESS == res_end_command_buffer);

	VkResult res_reset_fences = this->m_pfn_reset_fences(this->m_device, 1U, &vulkan_fence);
	assert(VK_SUCCESS == res_reset_fences);

	VkSubmitInfo submit_info;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.waitSemaphoreCount = 1U;
	submit_info.pWaitSemaphores = &vulkan_semaphore_acquire_next_image;
	VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
	submit_info.commandBufferCount = 1U;
	submit_info.pCommandBuffers = &vulkan_command_buffer;
	submit_info.signalSemaphoreCount = 1U;
	submit_info.pSignalSemaphores = &vulkan_semaphore_queue_submit;
	VkResult res_queue_submit = this->m_pfn_queue_submit(this->m_queue_graphics, 1U, &submit_info, vulkan_fence);
	assert(VK_SUCCESS == res_queue_submit);

	VkPresentInfoKHR present_info;
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = 1U;
	present_info.pWaitSemaphores = &vulkan_semaphore_queue_submit;
	present_info.swapchainCount = 1U;
	present_info.pSwapchains = &this->m_swapchain;
	present_info.pImageIndices = &vulkan_swapchain_image_index;
	present_info.pResults = NULL;
	VkResult res_queue_present = this->m_pfn_queue_present(this->m_queue_graphics, &present_info);
	if (sub_optimal_acquire_next_image || VK_SUCCESS != res_queue_present)
	{
		assert(VK_SUBOPTIMAL_KHR == res_queue_present || VK_ERROR_OUT_OF_DATE_KHR == res_queue_present);

		this->destory_framebuffer();

		if (VK_NULL_HANDLE != this->m_surface)
		{
			this->create_framebuffer();
		}
	}

	++this->m_frame_throtting_index;
	this->m_frame_throtting_index %= FRAME_THROTTLING_COUNT;
}

void vulkan_renderer::destroy()
{
	VkResult res_wait_for_fences = this->m_pfn_wait_for_fences(this->m_device, FRAME_THROTTLING_COUNT, this->m_fences, VK_TRUE, UINT64_MAX);
	assert(VK_SUCCESS == res_wait_for_fences);

	this->m_demo.destroy(this->m_device, this->m_pfn_get_device_proc_addr, this->m_allocation_callbacks, &this->m_asset_allocator);

	// TODO:
	// destory
}

void *renderer_init()
{
	class vulkan_renderer *renderer = new (malloc(sizeof(class vulkan_renderer))) vulkan_renderer();
	renderer->init();
	return renderer;
}

void renderer_attach_window(void *renderer_void, void *window_void)
{
	class vulkan_renderer *renderer = static_cast<class vulkan_renderer *>(renderer_void);
	renderer->attach_window(window_void);
}

void renderer_dettach_window(void *renderer_void)
{
	class vulkan_renderer *renderer = static_cast<class vulkan_renderer *>(renderer_void);
	renderer->dettach_window();
}

void renderer_draw(void *renderer_void)
{
	class vulkan_renderer *renderer = static_cast<class vulkan_renderer *>(renderer_void);
	renderer->draw();
}

void renderer_destory(void *renderer_void)
{
	class vulkan_renderer *renderer = static_cast<class vulkan_renderer *>(renderer_void);
	renderer->destroy();
	renderer->~vulkan_renderer();
	free(renderer);
}

#ifndef NDEBUG
static VkBool32 VKAPI_PTR vulkan_debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
#if defined(__GNUC__)

#if defined(__linux__) && defined(__ANDROID__)
	__android_log_write(ANDROID_LOG_DEBUG, "Vulkan-Demo", pCallbackData->pMessage);
#else
#error Unknown Platform
#endif

#elif defined(_MSC_VER)
	OutputDebugStringA(pCallbackData->pMessage);
	OutputDebugStringA("\n");
#else
#error Unknown Compiler
#endif
	return VK_FALSE;
}
#endif

static inline uint32_t __internal_find_lowest_memory_type_index(struct VkPhysicalDeviceMemoryProperties const *physical_device_memory_properties, VkDeviceSize memory_requirements_size, uint32_t memory_requirements_memory_type_bits, VkMemoryPropertyFlags required_property_flags)
{
	uint32_t memory_type_count = physical_device_memory_properties->memoryTypeCount;
	assert(VK_MAX_MEMORY_TYPES >= memory_type_count);

	// The lower memory_type_index indicates the more performance
	for (uint32_t memory_type_index = 0; memory_type_index < memory_type_count; ++memory_type_index)
	{
		uint32_t memory_type_bits = (1U << memory_type_index);
		bool is_required_memory_type = ((memory_requirements_memory_type_bits & memory_type_bits) != 0) ? true : false;

		VkMemoryPropertyFlags property_flags = physical_device_memory_properties->memoryTypes[memory_type_index].propertyFlags;
		bool has_required_property_flags = ((property_flags & required_property_flags) == required_property_flags) ? true : false;

		uint32_t heap_index = physical_device_memory_properties->memoryTypes[memory_type_index].heapIndex;
		VkDeviceSize heap_budget = physical_device_memory_properties->memoryHeaps[heap_index].size;
		// The application is not alone and there may be other applications which interact with the Vulkan as well.
		// The allocation may success even if the budget has been exceeded. However, this may result in performance issue.
		bool is_within_budget = (memory_requirements_size <= heap_budget) ? true : false;

		if (is_required_memory_type && has_required_property_flags && is_within_budget)
		{
			return memory_type_index;
		}
	}

	return VK_MAX_MEMORY_TYPES;
}

static inline uint32_t __internal_find_lowest_memory_type_index(struct VkPhysicalDeviceMemoryProperties const *physical_device_memory_properties, VkDeviceSize memory_requirements_size, uint32_t memory_requirements_memory_type_bits, VkMemoryPropertyFlags required_property_flags, VkMemoryPropertyFlags preferred_property_flags)
{
	VkMemoryPropertyFlags optimal_property_flags = (required_property_flags | preferred_property_flags);
	uint32_t memory_type_index = __internal_find_lowest_memory_type_index(physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, optimal_property_flags);
	if (VK_MAX_MEMORY_TYPES != memory_type_index)
	{
		assert(VK_MAX_MEMORY_TYPES > memory_type_index);
		return memory_type_index;
	}
	else
	{
		return __internal_find_lowest_memory_type_index(physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, required_property_flags);
	}
}
