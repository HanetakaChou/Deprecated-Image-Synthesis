//
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

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <cmath>
#include <algorithm>
#include <assert.h>
#include "demo.h"
#include "support/streaming.h"
#include "support/utils_align_up.h"
#include "support/camera_controller.h"
#include "../shaders/forward_shading_pipeline_layout.h"
#include "../shaders/point_light_shadow_pipeline_layout.h"
#include "../shaders/mesh_vertex.h"
#include "../assets/cube.h"

static uint32_t const g_point_light_shadow_mapping_width_height_size = 1024U;
static uint32_t const g_point_light_shadow_mapping_array_size = 2U;

static inline uint32_t linear_allocate(uint32_t &buffer_current, uint32_t buffer_end, uint32_t size, uint32_t alignment);

static inline DirectX::XMMATRIX XM_CALLCONV DirectX_Math_Matrix_PerspectiveFovRH_ReversedZ(float FovAngleY, float AspectRatio, float NearZ, float FarZ);

Demo::Demo() : m_main_camera_render_pass(VK_NULL_HANDLE),
			   m_vulkan_depth_image(VK_NULL_HANDLE),
			   m_vulkan_depth_device_memory(VK_NULL_HANDLE),
			   m_vulkan_depth_image_view(VK_NULL_HANDLE)
{
}

void Demo::init(
	VkInstance vulkan_instance, PFN_vkGetInstanceProcAddr pfn_get_instance_proc_addr, VkDevice vulkan_device, PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkAllocationCallbacks *vulkan_allocation_callbacks,
	VmaAllocator vulkan_asset_allocator, uint32_t staging_buffer_current, uint32_t vulkan_staging_buffer_end, void *vulkan_staging_buffer_device_memory_pointer, VkBuffer vulkan_staging_buffer,
	uint32_t vulkan_asset_vertex_buffer_memory_index, uint32_t vulkan_asset_index_buffer_memory_index, uint32_t vulkan_asset_uniform_buffer_memory_index, uint32_t vulkan_asset_image_memory_index, VkFormat vulkan_depth_format, uint32_t vulkan_depth_stencil_sampled_memory_index,
	uint32_t vulkan_optimal_buffer_copy_offset_alignment, uint32_t vulkan_optimal_buffer_copy_row_pitch_alignment,
	bool vulkan_has_dedicated_transfer_queue, uint32_t vulkan_queue_transfer_family_index, uint32_t vulkan_queue_graphics_family_index, VkCommandBuffer vulkan_streaming_transfer_command_buffer, VkCommandBuffer vulkan_streaming_graphics_command_buffer,
	VkDeviceSize vulkan_upload_ring_buffer_size, VkBuffer vulkan_upload_ring_buffer)
{
	// Point Light Shadow (Immutable) Sampler
	{
		PFN_vkCreateSampler pfn_create_sampler = reinterpret_cast<PFN_vkCreateSampler>(pfn_get_device_proc_addr(vulkan_device, "vkCreateSampler"));

		VkSamplerCreateInfo sampler_create_info;
		sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_create_info.pNext = NULL;
		sampler_create_info.flags = 0U;
		sampler_create_info.magFilter = VK_FILTER_NEAREST;
		sampler_create_info.minFilter = VK_FILTER_NEAREST;
		sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_create_info.mipLodBias = 0.0F;
		sampler_create_info.anisotropyEnable = VK_FALSE;
		sampler_create_info.maxAnisotropy = 1U;
		sampler_create_info.compareEnable = VK_FALSE;
		sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
		sampler_create_info.minLod = 0.0F;
		sampler_create_info.maxLod = VK_LOD_CLAMP_NONE;
		sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		sampler_create_info.unnormalizedCoordinates = VK_FALSE;
		VkResult res_create_sampler = pfn_create_sampler(vulkan_device, &sampler_create_info, vulkan_allocation_callbacks, &this->m_point_light_shadow_nearest_sampler);
		assert(VK_SUCCESS == res_create_sampler);
	}

	// Descriptor Layout
	{
		PFN_vkCreateDescriptorSetLayout pfn_create_descriptor_set_layout = reinterpret_cast<PFN_vkCreateDescriptorSetLayout>(pfn_get_device_proc_addr(vulkan_device, "vkCreateDescriptorSetLayout"));
		PFN_vkCreatePipelineLayout pfn_create_pipeline_layout = reinterpret_cast<PFN_vkCreatePipelineLayout>(pfn_get_device_proc_addr(vulkan_device, "vkCreatePipelineLayout"));

		VkDescriptorSetLayoutBinding point_light_shadow_global_set_layout_bindings[2] = {
			{0U,
			 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			 1U,
			 VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			 VK_NULL_HANDLE},
			{1U,
			 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			 1U,
			 VK_SHADER_STAGE_VERTEX_BIT,
			 VK_NULL_HANDLE}};

		VkDescriptorSetLayoutCreateInfo point_light_shadow_global_set_layout_create_info = {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			NULL,
			0U,
			sizeof(point_light_shadow_global_set_layout_bindings) / sizeof(point_light_shadow_global_set_layout_bindings[0]),
			point_light_shadow_global_set_layout_bindings,
		};

		VkResult res_create_point_light_shadow_global_set_layout = pfn_create_descriptor_set_layout(vulkan_device, &point_light_shadow_global_set_layout_create_info, vulkan_allocation_callbacks, &this->m_point_light_shadow_global_set_layout);
		assert(VK_SUCCESS == res_create_point_light_shadow_global_set_layout);

		VkDescriptorSetLayout point_light_shadow_set_layouts[1] = {this->m_point_light_shadow_global_set_layout};

		VkPipelineLayoutCreateInfo point_light_shadow_pipeline_layout_create_info = {
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			NULL,
			0U,
			sizeof(point_light_shadow_set_layouts) / sizeof(point_light_shadow_set_layouts[0]),
			point_light_shadow_set_layouts,
			0U,
			NULL};
		VkResult res_create_point_light_shadow_pipeline_layout = pfn_create_pipeline_layout(vulkan_device, &point_light_shadow_pipeline_layout_create_info, vulkan_allocation_callbacks, &this->m_point_light_shadow_pipeline_layout);
		assert(VK_SUCCESS == res_create_point_light_shadow_pipeline_layout);

		VkDescriptorSetLayoutBinding forward_shading_global_set_layout_bindings[3] = {
			{0U,
			 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			 1U,
			 VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			 VK_NULL_HANDLE},
			{1U,
			 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			 1U,
			 VK_SHADER_STAGE_VERTEX_BIT,
			 VK_NULL_HANDLE},
			{2U,
			 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			 1U,
			 VK_SHADER_STAGE_FRAGMENT_BIT,
			 &this->m_point_light_shadow_nearest_sampler}};

		VkDescriptorSetLayoutCreateInfo forward_shading_global_set_layout_create_info = {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			NULL,
			0U,
			sizeof(forward_shading_global_set_layout_bindings) / sizeof(forward_shading_global_set_layout_bindings[0]),
			forward_shading_global_set_layout_bindings,
		};

		VkResult res_create_forward_shading_global_set_layout = pfn_create_descriptor_set_layout(vulkan_device, &forward_shading_global_set_layout_create_info, vulkan_allocation_callbacks, &this->m_forward_shading_global_set_layout);
		assert(VK_SUCCESS == res_create_forward_shading_global_set_layout);

		VkDescriptorSetLayout forward_shading_set_layouts[1] = {this->m_forward_shading_global_set_layout};

		VkPipelineLayoutCreateInfo forward_shading_pipeline_layout_create_info = {
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			NULL,
			0U,
			sizeof(forward_shading_set_layouts) / sizeof(forward_shading_set_layouts[0]),
			forward_shading_set_layouts,
			0U,
			NULL};
		VkResult res_create_forward_shading_pipeline_layout = pfn_create_pipeline_layout(vulkan_device, &forward_shading_pipeline_layout_create_info, vulkan_allocation_callbacks, &this->m_forward_shading_pipeline_layout);
		assert(VK_SUCCESS == res_create_forward_shading_pipeline_layout);
	}

	// Point Light Shadow Render Pass and Pipeline
	{
		this->m_point_light_shadow_pass_index = 0;

		// Render Pass
		{
			PFN_vkCreateRenderPass pfn_create_render_pass = reinterpret_cast<PFN_vkCreateRenderPass>(pfn_get_device_proc_addr(vulkan_device, "vkCreateRenderPass"));

			uint32_t depth_attachment_index = 0U;
			VkAttachmentDescription render_pass_attachments[] = {
				{
					0U,
					vulkan_depth_format,
					VK_SAMPLE_COUNT_1_BIT,
					VK_ATTACHMENT_LOAD_OP_CLEAR,
					VK_ATTACHMENT_STORE_OP_STORE,
					VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					VK_ATTACHMENT_STORE_OP_DONT_CARE,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				}};

			VkAttachmentReference point_light_shadow_pass_depth_attachment_reference = {
				depth_attachment_index,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

			VkSubpassDescription render_pass_subpasses[] = {
				{0U,
				 VK_PIPELINE_BIND_POINT_GRAPHICS,
				 0U,
				 NULL,
				 0U,
				 NULL,
				 NULL,
				 &point_light_shadow_pass_depth_attachment_reference,
				 0U,
				 NULL}};

			VkRenderPassCreateInfo render_pass_create_info = {
				VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
				NULL,
				0U,
				sizeof(render_pass_attachments) / sizeof(render_pass_attachments[0]),
				render_pass_attachments,
				sizeof(render_pass_subpasses) / sizeof(render_pass_subpasses[0]),
				render_pass_subpasses,
				0U,
				NULL,
			};

			VkResult res_create_render_pass = pfn_create_render_pass(vulkan_device, &render_pass_create_info, NULL, &this->m_point_light_shadow_render_pass);
			assert(VK_SUCCESS == res_create_render_pass);
		}

		// Pipeline
		{
			PFN_vkCreateShaderModule pfn_create_shader_module = reinterpret_cast<PFN_vkCreateShaderModule>(pfn_get_device_proc_addr(vulkan_device, "vkCreateShaderModule"));
			PFN_vkCreateGraphicsPipelines pfn_create_graphics_pipelines = reinterpret_cast<PFN_vkCreateGraphicsPipelines>(pfn_get_device_proc_addr(vulkan_device, "vkCreateGraphicsPipelines"));
			PFN_vkDestroyShaderModule pfn_destory_shader_module = reinterpret_cast<PFN_vkDestroyShaderModule>(pfn_get_device_proc_addr(vulkan_device, "vkDestroyShaderModule"));

			VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
			{
				constexpr uint32_t const code[] = {
#include "../spirv/point_light_shadow_vertex.inl"
				};

				VkShaderModuleCreateInfo shader_module_create_info;
				shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				shader_module_create_info.pNext = NULL;
				shader_module_create_info.flags = 0U;
				shader_module_create_info.codeSize = sizeof(code);
				shader_module_create_info.pCode = code;

				VkResult res_create_shader_module = pfn_create_shader_module(vulkan_device, &shader_module_create_info, NULL, &vertex_shader_module);
				assert(VK_SUCCESS == res_create_shader_module);
			}

			VkShaderModule fragment_shader_module = VK_NULL_HANDLE;
			{
				constexpr uint32_t const code[] = {
#include "../spirv/point_light_shadow_fragment.inl"
				};

				VkShaderModuleCreateInfo shader_module_create_info;
				shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				shader_module_create_info.pNext = NULL;
				shader_module_create_info.flags = 0U;
				shader_module_create_info.codeSize = sizeof(code);
				shader_module_create_info.pCode = code;

				VkResult res_create_shader_module = pfn_create_shader_module(vulkan_device, &shader_module_create_info, NULL, &fragment_shader_module);
				assert(VK_SUCCESS == res_create_shader_module);
			}

			VkPipelineShaderStageCreateInfo stages[2] =
				{
					{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					 NULL,
					 0U,
					 VK_SHADER_STAGE_VERTEX_BIT,
					 vertex_shader_module,
					 "main",
					 NULL},
					{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					 NULL,
					 0U,
					 VK_SHADER_STAGE_FRAGMENT_BIT,
					 fragment_shader_module,
					 "main",
					 NULL}};

			VkPipelineVertexInputStateCreateInfo vertex_input_state = {
				VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				NULL,
				0U,
				sizeof(mesh_vertex_binding_descriptions) / sizeof(mesh_vertex_binding_descriptions[0]),
				mesh_vertex_binding_descriptions,
				sizeof(mesh_vertex_attribute_descriptions) / sizeof(mesh_vertex_attribute_descriptions[0]),
				mesh_vertex_attribute_descriptions};

			VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
				VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				NULL,
				0U,
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				VK_FALSE};

			VkPipelineViewportStateCreateInfo viewport_state = {
				VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
				NULL,
				0U,
				1U,
				NULL,
				1U,
				NULL};

			VkPipelineRasterizationStateCreateInfo rasterization_state = {
				VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				NULL,
				0U,
				VK_FALSE,
				VK_FALSE,
				VK_POLYGON_MODE_FILL,
				VK_CULL_MODE_BACK_BIT,
				VK_FRONT_FACE_COUNTER_CLOCKWISE,
				VK_FALSE,
				0.0F,
				0.0F,
				0.0F,
				1.0F};

			VkPipelineMultisampleStateCreateInfo multisample_state = {
				VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				NULL,
				0U,
				VK_SAMPLE_COUNT_1_BIT,
				VK_FALSE,
				0.0F,
				NULL,
				VK_FALSE,
				VK_FALSE};

			VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
				VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				NULL,
				0U,
				VK_TRUE,
				VK_TRUE,
				VK_COMPARE_OP_LESS,
				VK_FALSE,
				VK_FALSE,
				{VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 255, 255, 255},
				{VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 255, 255, 255},
				0.0F,
				1.0F};

			VkPipelineColorBlendAttachmentState attachments[1] = {
				{VK_FALSE, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT}};

			VkPipelineColorBlendStateCreateInfo color_blend_state = {
				VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
				NULL,
				0U,
				VK_FALSE,
				VK_LOGIC_OP_CLEAR,
				1U,
				attachments,
				{0.0F, 0.0F, 0.0F, 0.0F}};

			VkDynamicState dynamic_states[2] = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR};

			VkPipelineDynamicStateCreateInfo dynamic_state = {
				VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				NULL,
				0U,
				sizeof(dynamic_states) / sizeof(dynamic_states[0]),
				dynamic_states};

			VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
				VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
				NULL,
				0U,
				sizeof(stages) / sizeof(stages[0]),
				stages,
				&vertex_input_state,
				&input_assembly_state,
				NULL,
				&viewport_state,
				&rasterization_state,
				&multisample_state,
				&depth_stencil_state,
				&color_blend_state,
				&dynamic_state,
				this->m_point_light_shadow_pipeline_layout,
				this->m_point_light_shadow_render_pass,
				this->m_point_light_shadow_pass_index,
				VK_NULL_HANDLE,
				0U};

			VkResult res_create_graphics_pipelines = pfn_create_graphics_pipelines(vulkan_device, VK_NULL_HANDLE, 1U, &graphics_pipeline_create_info, vulkan_allocation_callbacks, &this->m_point_light_shadow_pipeline);
			assert(VK_SUCCESS == res_create_graphics_pipelines);

			pfn_destory_shader_module(vulkan_device, vertex_shader_module, vulkan_allocation_callbacks);
			pfn_destory_shader_module(vulkan_device, fragment_shader_module, vulkan_allocation_callbacks);
		}
	}

	// Point Light Shadow Image
	{
		PFN_vkCreateImage pfn_create_image = reinterpret_cast<PFN_vkCreateImage>(pfn_get_device_proc_addr(vulkan_device, "vkCreateImage"));
		PFN_vkGetImageMemoryRequirements pfn_get_image_memory_requirements = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(pfn_get_device_proc_addr(vulkan_device, "vkGetImageMemoryRequirements"));
		PFN_vkAllocateMemory pfn_allocate_memory = reinterpret_cast<PFN_vkAllocateMemory>(pfn_get_device_proc_addr(vulkan_device, "vkAllocateMemory"));
		PFN_vkBindImageMemory pfn_bind_image_memory = reinterpret_cast<PFN_vkBindImageMemory>(pfn_get_device_proc_addr(vulkan_device, "vkBindImageMemory"));
		PFN_vkCreateImageView pfn_create_image_view = reinterpret_cast<PFN_vkCreateImageView>(pfn_get_device_proc_addr(vulkan_device, "vkCreateImageView"));

		// image
		{
			struct VkImageCreateInfo image_create_info;
			image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			image_create_info.pNext = NULL;
			image_create_info.flags = 0U;
			image_create_info.imageType = VK_IMAGE_TYPE_2D;
			image_create_info.format = vulkan_depth_format;
			image_create_info.extent.width = g_point_light_shadow_mapping_width_height_size;
			image_create_info.extent.height = g_point_light_shadow_mapping_width_height_size;
			image_create_info.extent.depth = 1U;
			image_create_info.mipLevels = 1U;
			image_create_info.arrayLayers = g_point_light_shadow_mapping_array_size;
			image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
			image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
			image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			image_create_info.queueFamilyIndexCount = 0U;
			image_create_info.pQueueFamilyIndices = NULL;
			image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			VkResult res_create_image = pfn_create_image(vulkan_device, &image_create_info, vulkan_allocation_callbacks, &this->m_point_light_shadow_image);
			assert(VK_SUCCESS == res_create_image);
		}

		// device memory
		{
			struct VkMemoryRequirements memory_requirements;
			pfn_get_image_memory_requirements(vulkan_device, this->m_point_light_shadow_image, &memory_requirements);
			assert(0U != (memory_requirements.memoryTypeBits & (1U << vulkan_depth_stencil_sampled_memory_index)));

			VkMemoryAllocateInfo memory_allocate_info;
			memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memory_allocate_info.pNext = NULL;
			memory_allocate_info.allocationSize = memory_requirements.size;
			memory_allocate_info.memoryTypeIndex = vulkan_depth_stencil_sampled_memory_index;
			VkResult res_allocate_memory = pfn_allocate_memory(vulkan_device, &memory_allocate_info, vulkan_allocation_callbacks, &this->m_point_light_shadow_device_memory);
			assert(VK_SUCCESS == res_allocate_memory);

			VkResult res_bind_image_memory = pfn_bind_image_memory(vulkan_device, this->m_point_light_shadow_image, this->m_point_light_shadow_device_memory, 0U);
			assert(VK_SUCCESS == res_bind_image_memory);
		}

		// image view
		{
			VkImageViewCreateInfo image_view_create_info;
			image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			image_view_create_info.pNext = NULL;
			image_view_create_info.flags = 0U;
			image_view_create_info.image = this->m_point_light_shadow_image;
			image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			image_view_create_info.format = vulkan_depth_format;
			image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			image_view_create_info.subresourceRange.baseMipLevel = 0U;
			image_view_create_info.subresourceRange.levelCount = 1U;
			image_view_create_info.subresourceRange.baseArrayLayer = 0U;
			image_view_create_info.subresourceRange.layerCount = g_point_light_shadow_mapping_array_size;

			VkResult res_create_image_view = pfn_create_image_view(vulkan_device, &image_view_create_info, vulkan_allocation_callbacks, &this->m_point_light_shadow_image_view);
			assert(VK_SUCCESS == res_create_image_view);
		}
	}

	// Point Light Shadow Pass Framebuffer
	{
		PFN_vkCreateFramebuffer pfn_create_framebuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(pfn_get_device_proc_addr(vulkan_device, "vkCreateFramebuffer"));

		VkImageView framebuffer_attachments[] = {
			this->m_point_light_shadow_image_view};

		VkFramebufferCreateInfo framebuffer_create_info = {
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			NULL,
			0U,
			this->m_point_light_shadow_render_pass,
			sizeof(framebuffer_attachments) / sizeof(framebuffer_attachments[0]),
			framebuffer_attachments,
			g_point_light_shadow_mapping_width_height_size,
			g_point_light_shadow_mapping_width_height_size,
			g_point_light_shadow_mapping_array_size};

		VkResult res_create_framebuffer = pfn_create_framebuffer(vulkan_device, &framebuffer_create_info, vulkan_allocation_callbacks, &this->m_point_light_shadow_frame_buffer);
		assert(VK_SUCCESS == res_create_framebuffer);
	}

	// Main Camera Render Pass and Pipeline
	this->create_main_camera_render_pass_and_pipeline(vulkan_device, pfn_get_device_proc_addr, VK_FORMAT_D32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM, vulkan_allocation_callbacks);

	// Descriptor Pool
	{
		PFN_vkCreateDescriptorPool pfn_create_descriptor_pool = reinterpret_cast<PFN_vkCreateDescriptorPool>(pfn_get_device_proc_addr(vulkan_device, "vkCreateDescriptorPool"));

		VkDescriptorPoolSize pool_sizes[2] = {

			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2U + 2U},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1U}};

		VkDescriptorPoolCreateInfo create_info = {
			VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			NULL,
			0U,
			2U,
			sizeof(pool_sizes) / sizeof(pool_sizes[0]),
			pool_sizes};

		VkResult res_create_descriptor_pool = pfn_create_descriptor_pool(vulkan_device, &create_info, vulkan_allocation_callbacks, &this->m_descriptor_pool);
		assert(VK_SUCCESS == res_create_descriptor_pool);
	}

	// Descriptors - Global Set
	{
		PFN_vkAllocateDescriptorSets pfn_allocate_descriptor_sets = reinterpret_cast<PFN_vkAllocateDescriptorSets>(pfn_get_device_proc_addr(vulkan_device, "vkAllocateDescriptorSets"));

		VkDescriptorSetLayout set_layouts[2] = {this->m_point_light_shadow_global_set_layout, this->m_forward_shading_global_set_layout};

		VkDescriptorSetAllocateInfo allocate_info = {
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			NULL,
			this->m_descriptor_pool,
			sizeof(set_layouts) / sizeof(set_layouts[0]),
			set_layouts};

		VkDescriptorSet descriptor_sets[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
		VkResult res_allocate_descriptor_sets = pfn_allocate_descriptor_sets(vulkan_device, &allocate_info, descriptor_sets);
		assert(VK_SUCCESS == res_allocate_descriptor_sets);

		this->m_point_light_shadow_global_set = descriptor_sets[0];
		this->m_forward_shading_global_set = descriptor_sets[1];

		PFN_vkUpdateDescriptorSets pfn_update_descriptor_sets = reinterpret_cast<PFN_vkUpdateDescriptorSets>(pfn_get_device_proc_addr(vulkan_device, "vkUpdateDescriptorSets"));

		VkDescriptorBufferInfo point_light_shadow_buffer_info[2] = {
			{vulkan_upload_ring_buffer,
			 0U,
			 sizeof(point_light_shadow_layout_global_set_per_frame_uniform_buffer_binding)},
			{vulkan_upload_ring_buffer,
			 0U,
			 sizeof(point_light_shadow_layout_global_set_per_object_uniform_buffer_binding)}};

		VkDescriptorBufferInfo forward_shading_buffer_info[2] = {
			{vulkan_upload_ring_buffer,
			 0U,
			 sizeof(forward_shading_layout_global_set_per_frame_uniform_buffer_binding)},
			{vulkan_upload_ring_buffer,
			 0U,
			 sizeof(forward_shading_layout_global_set_per_object_uniform_buffer_binding)}};

		VkDescriptorImageInfo forward_shading_image_info[1] = {
			{VK_NULL_HANDLE,
			 this->m_point_light_shadow_image_view,
			 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};

		VkWriteDescriptorSet set_writes[5] = {
			{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			 NULL,
			 this->m_point_light_shadow_global_set,
			 0U,
			 0U,
			 1U,
			 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			 NULL,
			 &point_light_shadow_buffer_info[0],
			 NULL},
			{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			 NULL,
			 this->m_point_light_shadow_global_set,
			 1U,
			 0U,
			 1U,
			 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			 NULL,
			 &point_light_shadow_buffer_info[1],
			 NULL},
			{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			 NULL,
			 this->m_forward_shading_global_set,
			 0U,
			 0U,
			 1U,
			 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			 NULL,
			 &forward_shading_buffer_info[0],
			 NULL},
			{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			 NULL,
			 this->m_forward_shading_global_set,
			 1U,
			 0U,
			 1U,
			 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			 NULL,
			 &forward_shading_buffer_info[1],
			 NULL},
			{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			 NULL,
			 this->m_forward_shading_global_set,
			 2U,
			 0U,
			 1U,
			 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			 &forward_shading_image_info[0],
			 NULL,
			 NULL}};
		pfn_update_descriptor_sets(vulkan_device, sizeof(set_writes) / sizeof(set_writes[0]), set_writes, 0U, NULL);
	}

	// Assets
	{
		PFN_vkCmdPipelineBarrier pfn_cmd_pipeline_barrier = reinterpret_cast<PFN_vkCmdPipelineBarrier>(pfn_get_device_proc_addr(vulkan_device, "vkCmdPipelineBarrier"));
		PFN_vkCmdCopyBuffer pfn_cmd_copy_buffer = reinterpret_cast<PFN_vkCmdCopyBuffer>(pfn_get_device_proc_addr(vulkan_device, "vkCmdCopyBuffer"));
		PFN_vkCmdCopyBufferToImage pfn_cmd_copy_buffer_to_image = reinterpret_cast<PFN_vkCmdCopyBufferToImage>(pfn_get_device_proc_addr(vulkan_device, "vkCmdCopyBufferToImage"));
		PFN_vkCreateImageView pfn_create_image_view = reinterpret_cast<PFN_vkCreateImageView>(pfn_get_device_proc_addr(vulkan_device, "vkCreateImageView"));

		// vertex count
		this->m_cube_vertex_count = 6U * 6U;

		// asset
		std::vector<mesh_vertex_position> mesh_vertex_positions;
		mesh_vertex_positions.resize(this->m_cube_vertex_count);
		for (uint32_t vertex_index = 0U; vertex_index < this->m_cube_vertex_count; ++vertex_index)
		{
			mesh_vertex_positions[vertex_index].position_x = DirectX::PackedVector::XMConvertFloatToHalf(cube_vertex_postion[3U * vertex_index + 0U]);
			mesh_vertex_positions[vertex_index].position_y = DirectX::PackedVector::XMConvertFloatToHalf(cube_vertex_postion[3U * vertex_index + 1U]);
			mesh_vertex_positions[vertex_index].position_z = DirectX::PackedVector::XMConvertFloatToHalf(cube_vertex_postion[3U * vertex_index + 2U]);
		}

		// vertex position buffer
		{
			VkBufferCreateInfo buffer_create_info = {
				VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				NULL,
				0U,
				sizeof(mesh_vertex_position) * this->m_cube_vertex_count,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_SHARING_MODE_EXCLUSIVE,
				0U,
				NULL};

			VmaAllocationCreateInfo allocation_create_info = {};
			allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocation_create_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			allocation_create_info.memoryTypeBits = 1 << vulkan_asset_vertex_buffer_memory_index;

			VkResult res_vma_create_buffer = vmaCreateBuffer(vulkan_asset_allocator, &buffer_create_info, &allocation_create_info, &this->m_cube_vertex_position_buffer, &this->m_cube_vertex_position_allocation, NULL);
			assert(VK_SUCCESS == res_vma_create_buffer);

			uint32_t position_staging_buffer_offset = linear_allocate(staging_buffer_current, vulkan_staging_buffer_end, sizeof(mesh_vertex_position) * this->m_cube_vertex_count, 1U);

			// write to staging buffer
			memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(vulkan_staging_buffer_device_memory_pointer) + position_staging_buffer_offset), &mesh_vertex_positions[0], sizeof(mesh_vertex_position) * this->m_cube_vertex_count);

			VkBufferCopy region_position[1] = {
				{position_staging_buffer_offset,
				 0U,
				 sizeof(mesh_vertex_position) * this->m_cube_vertex_count}};

			streaming_staging_to_asset_buffer(
				pfn_cmd_pipeline_barrier, pfn_cmd_copy_buffer,
				vulkan_has_dedicated_transfer_queue, vulkan_queue_transfer_family_index, vulkan_queue_graphics_family_index, vulkan_streaming_transfer_command_buffer, vulkan_streaming_graphics_command_buffer,
				vulkan_staging_buffer, this->m_cube_vertex_position_buffer, 1U, region_position, ASSET_VERTEX_BUFFER);
		}
	}

	// Proc Addr
	this->m_pfn_cmd_begin_render_pass = reinterpret_cast<PFN_vkCmdBeginRenderPass>(pfn_get_device_proc_addr(vulkan_device, "vkCmdBeginRenderPass"));
	this->m_pfn_cmd_end_render_pass = reinterpret_cast<PFN_vkCmdEndRenderPass>(pfn_get_device_proc_addr(vulkan_device, "vkCmdEndRenderPass"));
	this->m_pfn_cmd_bind_pipeline = reinterpret_cast<PFN_vkCmdBindPipeline>(pfn_get_device_proc_addr(vulkan_device, "vkCmdBindPipeline"));
	this->m_pfn_cmd_set_viewport = reinterpret_cast<PFN_vkCmdSetViewport>(pfn_get_device_proc_addr(vulkan_device, "vkCmdSetViewport"));
	this->m_pfn_cmd_set_scissor = reinterpret_cast<PFN_vkCmdSetScissor>(pfn_get_device_proc_addr(vulkan_device, "vkCmdSetScissor"));
	this->m_pfn_cmd_bind_descriptor_sets = reinterpret_cast<PFN_vkCmdBindDescriptorSets>(pfn_get_device_proc_addr(vulkan_device, "vkCmdBindDescriptorSets"));
	this->m_pfn_cmd_bind_vertex_buffers = reinterpret_cast<PFN_vkCmdBindVertexBuffers>(pfn_get_device_proc_addr(vulkan_device, "vkCmdBindVertexBuffers"));
	this->m_pfn_cmd_draw = reinterpret_cast<PFN_vkCmdDraw>(pfn_get_device_proc_addr(vulkan_device, "vkCmdDraw"));
#ifndef NDEBUG
	this->m_pfn_cmd_begin_debug_utils_label = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(pfn_get_instance_proc_addr(vulkan_instance, "vkCmdBeginDebugUtilsLabelEXT"));
	this->m_pfn_cmd_end_debug_utils_label = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(pfn_get_instance_proc_addr(vulkan_instance, "vkCmdEndDebugUtilsLabelEXT"));
#endif

	// Init Camera
	g_camera_controller.m_eye_position = DirectX::XMFLOAT3(-14.0F, 14.0F, 14.0F);
	g_camera_controller.m_eye_direction = DirectX::XMFLOAT3(0.577F, -0.577F, -0.577F);
	g_camera_controller.m_up_direction = DirectX::XMFLOAT3(0.0F, 1.0F, 0.0F);

	// Init Box and Plane
	this->m_box_spin_angle = 0.0F;
	DirectX::XMStoreFloat4x4(&this->m_plane_model_transform, DirectX::XMMatrixMultiply(DirectX::XMMatrixScaling(25.0F, 1.0F, 25.0F), DirectX::XMMatrixTranslation(0.0F, -5.0F, -25.0F)));
	this->m_point_light_shadow_bias = 0.000075F;

	// Init Point Light
	this->m_point_light_position_and_radius.x = 0.0F;
	this->m_point_light_position_and_radius.y = 0.0F;
	this->m_point_light_position_and_radius.z = 5.0F;
	this->m_point_light_position_and_radius.w = 50.0F;
}

void Demo::create_main_camera_render_pass_and_pipeline(VkDevice vulkan_device, PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkFormat vulkan_depth_format, VkFormat vulkan_swapchain_image_format, VkAllocationCallbacks *vulkan_allocation_callbacks)
{
	this->m_forward_shading_pass_index = 0;

	// Render Pass
	{
		PFN_vkCreateRenderPass pfn_create_render_pass = reinterpret_cast<PFN_vkCreateRenderPass>(pfn_get_device_proc_addr(vulkan_device, "vkCreateRenderPass"));

		uint32_t swapchain_image_attachment_index = 0U;
		uint32_t depth_attachment_index = 1U;
		VkAttachmentDescription render_pass_attachments[] = {
			{
				0U,
				vulkan_swapchain_image_format,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			},
			{
				0U,
				vulkan_depth_format,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			}};

		VkAttachmentReference forward_shading_pass_color_attachments_reference[] = {
			{swapchain_image_attachment_index,
			 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

		VkAttachmentReference forward_shading_pass_depth_attachment_reference = {
			depth_attachment_index,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

		VkSubpassDescription render_pass_subpasses[] = {
			{0U,
			 VK_PIPELINE_BIND_POINT_GRAPHICS,
			 0U,
			 NULL,
			 sizeof(forward_shading_pass_color_attachments_reference) / sizeof(forward_shading_pass_color_attachments_reference[0]),
			 forward_shading_pass_color_attachments_reference,
			 NULL,
			 &forward_shading_pass_depth_attachment_reference,
			 0U,
			 NULL}};

		VkRenderPassCreateInfo render_pass_create_info = {
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			NULL,
			0U,
			sizeof(render_pass_attachments) / sizeof(render_pass_attachments[0]),
			render_pass_attachments,
			sizeof(render_pass_subpasses) / sizeof(render_pass_subpasses[0]),
			render_pass_subpasses,
			0U,
			NULL,
		};

		VkResult res_create_render_pass = pfn_create_render_pass(vulkan_device, &render_pass_create_info, NULL, &this->m_main_camera_render_pass);
		assert(VK_SUCCESS == res_create_render_pass);
	}

	// Pipeline
	{
		PFN_vkCreateShaderModule pfn_create_shader_module = reinterpret_cast<PFN_vkCreateShaderModule>(pfn_get_device_proc_addr(vulkan_device, "vkCreateShaderModule"));
		PFN_vkCreateGraphicsPipelines pfn_create_graphics_pipelines = reinterpret_cast<PFN_vkCreateGraphicsPipelines>(pfn_get_device_proc_addr(vulkan_device, "vkCreateGraphicsPipelines"));
		PFN_vkDestroyShaderModule pfn_destory_shader_module = reinterpret_cast<PFN_vkDestroyShaderModule>(pfn_get_device_proc_addr(vulkan_device, "vkDestroyShaderModule"));

		VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
		{
			constexpr uint32_t const code[] = {
#include "../spirv/forward_shading_vertex.inl"
			};

			VkShaderModuleCreateInfo shader_module_create_info;
			shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shader_module_create_info.pNext = NULL;
			shader_module_create_info.flags = 0U;
			shader_module_create_info.codeSize = sizeof(code);
			shader_module_create_info.pCode = code;

			VkResult res_create_shader_module = pfn_create_shader_module(vulkan_device, &shader_module_create_info, NULL, &vertex_shader_module);
			assert(VK_SUCCESS == res_create_shader_module);
		}

		VkShaderModule fragment_shader_module = VK_NULL_HANDLE;
		{
			constexpr uint32_t const code[] = {
#include "../spirv/forward_shading_fragment.inl"
			};

			VkShaderModuleCreateInfo shader_module_create_info;
			shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shader_module_create_info.pNext = NULL;
			shader_module_create_info.flags = 0U;
			shader_module_create_info.codeSize = sizeof(code);
			shader_module_create_info.pCode = code;

			VkResult res_create_shader_module = pfn_create_shader_module(vulkan_device, &shader_module_create_info, NULL, &fragment_shader_module);
			assert(VK_SUCCESS == res_create_shader_module);
		}

		VkPipelineShaderStageCreateInfo stages[2] =
			{
				{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				 NULL,
				 0U,
				 VK_SHADER_STAGE_VERTEX_BIT,
				 vertex_shader_module,
				 "main",
				 NULL},
				{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				 NULL,
				 0U,
				 VK_SHADER_STAGE_FRAGMENT_BIT,
				 fragment_shader_module,
				 "main",
				 NULL}};

		VkPipelineVertexInputStateCreateInfo vertex_input_state = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			NULL,
			0U,
			sizeof(mesh_vertex_binding_descriptions) / sizeof(mesh_vertex_binding_descriptions[0]),
			mesh_vertex_binding_descriptions,
			sizeof(mesh_vertex_attribute_descriptions) / sizeof(mesh_vertex_attribute_descriptions[0]),
			mesh_vertex_attribute_descriptions};

		VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			NULL,
			0U,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			VK_FALSE};

		VkPipelineViewportStateCreateInfo viewport_state = {
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			NULL,
			0U,
			1U,
			NULL,
			1U,
			NULL};

		VkPipelineRasterizationStateCreateInfo rasterization_state = {
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			NULL,
			0U,
			VK_FALSE,
			VK_FALSE,
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_BACK_BIT,
			VK_FRONT_FACE_COUNTER_CLOCKWISE,
			VK_FALSE,
			0.0F,
			0.0F,
			0.0F,
			1.0F};

		VkPipelineMultisampleStateCreateInfo multisample_state = {
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			NULL,
			0U,
			VK_SAMPLE_COUNT_1_BIT,
			VK_FALSE,
			0.0F,
			NULL,
			VK_FALSE,
			VK_FALSE};

		VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			NULL,
			0U,
			VK_TRUE,
			VK_TRUE,
			VK_COMPARE_OP_GREATER,
			VK_FALSE,
			VK_FALSE,
			{VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 255, 255, 255},
			{VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 255, 255, 255},
			0.0F,
			1.0F};

		VkPipelineColorBlendAttachmentState attachments[1] = {
			{VK_FALSE, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT}};

		VkPipelineColorBlendStateCreateInfo color_blend_state = {
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			NULL,
			0U,
			VK_FALSE,
			VK_LOGIC_OP_CLEAR,
			1U,
			attachments,
			{0.0F, 0.0F, 0.0F, 0.0F}};

		VkDynamicState dynamic_states[2] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR};

		VkPipelineDynamicStateCreateInfo dynamic_state = {
			VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			NULL,
			0U,
			sizeof(dynamic_states) / sizeof(dynamic_states[0]),
			dynamic_states};

		VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			NULL,
			0U,
			sizeof(stages) / sizeof(stages[0]),
			stages,
			&vertex_input_state,
			&input_assembly_state,
			NULL,
			&viewport_state,
			&rasterization_state,
			&multisample_state,
			&depth_stencil_state,
			&color_blend_state,
			&dynamic_state,
			this->m_forward_shading_pipeline_layout,
			this->m_main_camera_render_pass,
			this->m_forward_shading_pass_index,
			VK_NULL_HANDLE,
			0U};

		VkResult res_create_graphics_pipelines = pfn_create_graphics_pipelines(vulkan_device, VK_NULL_HANDLE, 1U, &graphics_pipeline_create_info, vulkan_allocation_callbacks, &this->m_forward_shading_pipeline);
		assert(VK_SUCCESS == res_create_graphics_pipelines);

		pfn_destory_shader_module(vulkan_device, vertex_shader_module, vulkan_allocation_callbacks);
		pfn_destory_shader_module(vulkan_device, fragment_shader_module, vulkan_allocation_callbacks);
	}
}

void Demo::create_main_camera_frame_buffer(
	VkInstance vulkan_instance, PFN_vkGetInstanceProcAddr pfn_get_instance_proc_addr, VkPhysicalDevice vulkan_physical_device, VkDevice vulkan_device, PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkAllocationCallbacks *vulkan_allocation_callbacks,
	VkFormat vulkan_depth_format, uint32_t vulkan_depth_stencil_transient_attachment_memory_index,
	VkFormat vulkan_swapchain_image_format,
	uint32_t vulkan_framebuffer_width, uint32_t vulkan_framebuffer_height,
	VkSwapchainKHR vulkan_swapchain,
	uint32_t vulkan_swapchain_image_count,
	std::vector<VkImageView> const &vulkan_swapchain_image_views)
{
	if (vulkan_depth_format != this->m_depth_format || vulkan_swapchain_image_format != this->m_swapchain_image_format)
	{
		assert(VK_NULL_HANDLE != this->m_main_camera_render_pass);
		PFN_vkDestroyRenderPass pfn_destroy_render_pass = reinterpret_cast<PFN_vkDestroyRenderPass>(pfn_get_device_proc_addr(vulkan_device, "vkDestroyRenderPass"));
		pfn_destroy_render_pass(vulkan_device, this->m_main_camera_render_pass, vulkan_allocation_callbacks);

		assert(VK_NULL_HANDLE != this->m_forward_shading_pipeline);
		PFN_vkDestroyPipeline pfn_destroy_pipeline = reinterpret_cast<PFN_vkDestroyPipeline>(pfn_get_device_proc_addr(vulkan_device, "vkDestroyPipeline"));
		pfn_destroy_pipeline(vulkan_device, this->m_forward_shading_pipeline, vulkan_allocation_callbacks);

		this->create_main_camera_render_pass_and_pipeline(vulkan_device, pfn_get_device_proc_addr, vulkan_depth_format, vulkan_swapchain_image_format, vulkan_allocation_callbacks);

		this->m_depth_format = vulkan_depth_format;
		this->m_swapchain_image_format = vulkan_swapchain_image_format;
	}

	assert(0U != vulkan_framebuffer_width);
	assert(0U != vulkan_framebuffer_height);

	assert(VK_NULL_HANDLE == this->m_vulkan_depth_image);
	assert(VK_NULL_HANDLE == this->m_vulkan_depth_device_memory);
	assert(VK_NULL_HANDLE == this->m_vulkan_depth_image_view);
	{
		PFN_vkCreateImage pfn_create_image = reinterpret_cast<PFN_vkCreateImage>(pfn_get_device_proc_addr(vulkan_device, "vkCreateImage"));
		PFN_vkGetImageMemoryRequirements pfn_get_image_memory_requirements = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(pfn_get_device_proc_addr(vulkan_device, "vkGetImageMemoryRequirements"));
		PFN_vkAllocateMemory pfn_allocate_memory = reinterpret_cast<PFN_vkAllocateMemory>(pfn_get_device_proc_addr(vulkan_device, "vkAllocateMemory"));
		PFN_vkBindImageMemory pfn_bind_image_memory = reinterpret_cast<PFN_vkBindImageMemory>(pfn_get_device_proc_addr(vulkan_device, "vkBindImageMemory"));
		PFN_vkCreateImageView pfn_create_image_view = reinterpret_cast<PFN_vkCreateImageView>(pfn_get_device_proc_addr(vulkan_device, "vkCreateImageView"));

		// depth
		{
			struct VkImageCreateInfo image_create_info;
			image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			image_create_info.pNext = NULL;
			image_create_info.flags = 0U;
			image_create_info.imageType = VK_IMAGE_TYPE_2D;
			image_create_info.format = vulkan_depth_format;
			image_create_info.extent.width = vulkan_framebuffer_width;
			image_create_info.extent.height = vulkan_framebuffer_height;
			image_create_info.extent.depth = 1U;
			image_create_info.mipLevels = 1U;
			image_create_info.arrayLayers = 1U;
			image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
			image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
			image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
			image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			image_create_info.queueFamilyIndexCount = 0U;
			image_create_info.pQueueFamilyIndices = NULL;
			image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			VkResult res_create_image = pfn_create_image(vulkan_device, &image_create_info, vulkan_allocation_callbacks, &this->m_vulkan_depth_image);
			assert(VK_SUCCESS == res_create_image);
		}

		// depth memory
		{
			struct VkMemoryRequirements memory_requirements;
			pfn_get_image_memory_requirements(vulkan_device, this->m_vulkan_depth_image, &memory_requirements);
			assert(0U != (memory_requirements.memoryTypeBits & (1U << vulkan_depth_stencil_transient_attachment_memory_index)));

			VkMemoryAllocateInfo memory_allocate_info;
			memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memory_allocate_info.pNext = NULL;
			memory_allocate_info.allocationSize = memory_requirements.size;
			memory_allocate_info.memoryTypeIndex = vulkan_depth_stencil_transient_attachment_memory_index;
			VkResult res_allocate_memory = pfn_allocate_memory(vulkan_device, &memory_allocate_info, vulkan_allocation_callbacks, &this->m_vulkan_depth_device_memory);
			assert(VK_SUCCESS == res_allocate_memory);

			VkResult res_bind_image_memory = pfn_bind_image_memory(vulkan_device, this->m_vulkan_depth_image, this->m_vulkan_depth_device_memory, 0U);
			assert(VK_SUCCESS == res_bind_image_memory);
		}

		// depth view
		{
			VkImageViewCreateInfo image_view_create_info;
			image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			image_view_create_info.pNext = NULL;
			image_view_create_info.flags = 0U;
			image_view_create_info.image = this->m_vulkan_depth_image;
			image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			image_view_create_info.format = vulkan_depth_format;
			image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			image_view_create_info.subresourceRange.baseMipLevel = 0U;
			image_view_create_info.subresourceRange.levelCount = 1U;
			image_view_create_info.subresourceRange.baseArrayLayer = 0U;
			image_view_create_info.subresourceRange.layerCount = 1U;

			VkResult res_create_image_view = pfn_create_image_view(vulkan_device, &image_view_create_info, vulkan_allocation_callbacks, &this->m_vulkan_depth_image_view);
			assert(VK_SUCCESS == res_create_image_view);
		}
	}

	assert(0U == this->m_vulkan_main_camera_framebuffers.size());
	{
		this->m_vulkan_main_camera_framebuffers.resize(vulkan_swapchain_image_count);

		PFN_vkCreateFramebuffer pfn_create_framebuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(pfn_get_device_proc_addr(vulkan_device, "vkCreateFramebuffer"));

		for (uint32_t vulkan_swapchain_image_index = 0U; vulkan_swapchain_image_index < vulkan_swapchain_image_count; ++vulkan_swapchain_image_index)
		{
			VkImageView framebuffer_attachments[] = {
				vulkan_swapchain_image_views[vulkan_swapchain_image_index],
				this->m_vulkan_depth_image_view};

			VkFramebufferCreateInfo framebuffer_create_info = {
				VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				NULL,
				0U,
				this->m_main_camera_render_pass,
				sizeof(framebuffer_attachments) / sizeof(framebuffer_attachments[0]),
				framebuffer_attachments,
				vulkan_framebuffer_width,
				vulkan_framebuffer_height,
				1U};

			VkResult res_create_framebuffer = pfn_create_framebuffer(vulkan_device, &framebuffer_create_info, vulkan_allocation_callbacks, &this->m_vulkan_main_camera_framebuffers[vulkan_swapchain_image_index]);
			assert(VK_SUCCESS == res_create_framebuffer);
		}
	}
}

void Demo::destroy_main_camera_frame_buffer(
	VkDevice vulkan_device, PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkAllocationCallbacks *vulkan_allocation_callbacks, uint32_t vulkan_swapchain_image_count)
{
	// Framebuffer
	{
		PFN_vkDestroyFramebuffer pfn_destroy_framebuffer = reinterpret_cast<PFN_vkDestroyFramebuffer>(pfn_get_device_proc_addr(vulkan_device, "vkDestroyFramebuffer"));

		assert(static_cast<uint32_t>(this->m_vulkan_main_camera_framebuffers.size()) == vulkan_swapchain_image_count);

		for (uint32_t vulkan_swapchain_image_index = 0U; vulkan_swapchain_image_index < vulkan_swapchain_image_count; ++vulkan_swapchain_image_index)
		{
			pfn_destroy_framebuffer(vulkan_device, this->m_vulkan_main_camera_framebuffers[vulkan_swapchain_image_index], vulkan_allocation_callbacks);
		}
		this->m_vulkan_main_camera_framebuffers.clear();
	}

	// Depth
	{
		PFN_vkDestroyImageView pfn_destroy_image_view = reinterpret_cast<PFN_vkDestroyImageView>(pfn_get_device_proc_addr(vulkan_device, "vkDestroyImageView"));
		PFN_vkDestroyImage pfn_destroy_image = reinterpret_cast<PFN_vkDestroyImage>(pfn_get_device_proc_addr(vulkan_device, "vkDestroyImage"));
		PFN_vkFreeMemory pfn_free_memory = reinterpret_cast<PFN_vkFreeMemory>(pfn_get_device_proc_addr(vulkan_device, "vkFreeMemory"));

		pfn_destroy_image_view(vulkan_device, this->m_vulkan_depth_image_view, vulkan_allocation_callbacks);
		pfn_destroy_image(vulkan_device, this->m_vulkan_depth_image, vulkan_allocation_callbacks);
		pfn_free_memory(vulkan_device, this->m_vulkan_depth_device_memory, vulkan_allocation_callbacks);

		this->m_vulkan_depth_image = VK_NULL_HANDLE;
		this->m_vulkan_depth_device_memory = VK_NULL_HANDLE;
		this->m_vulkan_depth_image_view = VK_NULL_HANDLE;
	}
}

void Demo::tick(
	VkCommandBuffer vulkan_command_buffer, uint32_t vulkan_swapchain_image_index, uint32_t vulkan_framebuffer_width, uint32_t vulkan_framebuffer_height,
	void *vulkan_upload_ring_buffer_device_memory_pointer, uint32_t vulkan_upload_ring_buffer_current, uint32_t vulkan_upload_ring_buffer_end,
	uint32_t vulkan_min_uniform_buffer_offset_alignment)
{
	DirectX::XMFLOAT4X4 box_model_transform;
	{
		// update box and plane
		this->m_box_spin_angle += (0.7F / 180.0F * DirectX::XM_PI);

		DirectX::XMStoreFloat4x4(&box_model_transform, DirectX::XMMatrixRotationY(this->m_box_spin_angle));
	}

	{
		// Point Light Shadow Pass
#ifndef NDEBUG
		VkDebugUtilsLabelEXT debug_utils_label = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, NULL, "Point Light Shadow Pass", {1.0F, 1.0F, 1.0F, 1.0F}};
		this->m_pfn_cmd_begin_debug_utils_label(vulkan_command_buffer, &debug_utils_label);
#endif

		VkClearValue clear_values[1] = {{{1.0F, 0U}}};
		VkRenderPassBeginInfo render_pass_begin_info = {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			NULL,
			this->m_point_light_shadow_render_pass,
			this->m_point_light_shadow_frame_buffer,
			{{0U, 0U}, {g_point_light_shadow_mapping_width_height_size, g_point_light_shadow_mapping_width_height_size}},
			sizeof(clear_values) / sizeof(clear_values[0]),
			clear_values,
		};
		this->m_pfn_cmd_begin_render_pass(vulkan_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		this->m_pfn_cmd_bind_pipeline(vulkan_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_point_light_shadow_pipeline);

		VkViewport viewport = {0.0, 0.0, static_cast<float>(g_point_light_shadow_mapping_width_height_size), static_cast<float>(g_point_light_shadow_mapping_width_height_size), 0.0F, 1.0F};
		this->m_pfn_cmd_set_viewport(vulkan_command_buffer, 0U, 1U, &viewport);

		VkRect2D scissor = {{0, 0}, {static_cast<uint32_t>(g_point_light_shadow_mapping_width_height_size), static_cast<uint32_t>(g_point_light_shadow_mapping_width_height_size)}};
		this->m_pfn_cmd_set_scissor(vulkan_command_buffer, 0U, 1U, &scissor);

		// update constant buffer
		uint32_t global_set_per_frame_binding_offset = linear_allocate(vulkan_upload_ring_buffer_current, vulkan_upload_ring_buffer_end, sizeof(point_light_shadow_layout_global_set_per_frame_uniform_buffer_binding), vulkan_min_uniform_buffer_offset_alignment);
		{
			point_light_shadow_layout_global_set_per_frame_uniform_buffer_binding *global_set_per_frame_binding = reinterpret_cast<point_light_shadow_layout_global_set_per_frame_uniform_buffer_binding *>(reinterpret_cast<uintptr_t>(vulkan_upload_ring_buffer_device_memory_pointer) + global_set_per_frame_binding_offset);

			global_set_per_frame_binding->point_light_position_and_radius = this->m_point_light_position_and_radius;
		}

		// update constant buffer
		uint32_t global_set_box_per_object_binding_offset = linear_allocate(vulkan_upload_ring_buffer_current, vulkan_upload_ring_buffer_end, sizeof(point_light_shadow_layout_global_set_per_object_uniform_buffer_binding), vulkan_min_uniform_buffer_offset_alignment);
		{
			point_light_shadow_layout_global_set_per_object_uniform_buffer_binding *global_set_box_per_object_binding = reinterpret_cast<point_light_shadow_layout_global_set_per_object_uniform_buffer_binding *>(reinterpret_cast<uintptr_t>(vulkan_upload_ring_buffer_device_memory_pointer) + global_set_box_per_object_binding_offset);
			global_set_box_per_object_binding->model_transform = box_model_transform;
		}

		// update constant buffer
		uint32_t global_set_plane_per_object_binding_offset = linear_allocate(vulkan_upload_ring_buffer_current, vulkan_upload_ring_buffer_end, sizeof(point_light_shadow_layout_global_set_per_object_uniform_buffer_binding), vulkan_min_uniform_buffer_offset_alignment);
		{
			point_light_shadow_layout_global_set_per_object_uniform_buffer_binding *global_set_plane_per_object_binding = reinterpret_cast<point_light_shadow_layout_global_set_per_object_uniform_buffer_binding *>(reinterpret_cast<uintptr_t>(vulkan_upload_ring_buffer_device_memory_pointer) + global_set_plane_per_object_binding_offset);
			global_set_plane_per_object_binding->model_transform = this->m_plane_model_transform;
		}

		// draw box
		{
			VkDescriptorSet descriptor_sets[1] = {this->m_point_light_shadow_global_set};
			uint32_t dynamic_offsets[2] = {global_set_per_frame_binding_offset, global_set_box_per_object_binding_offset};
			this->m_pfn_cmd_bind_descriptor_sets(vulkan_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_point_light_shadow_pipeline_layout, 0U, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);

			VkBuffer buffers[1] = {this->m_cube_vertex_position_buffer};
			VkDeviceSize offsets[1] = {0U};
			this->m_pfn_cmd_bind_vertex_buffers(vulkan_command_buffer, 0U, sizeof(buffers) / sizeof(buffers[0]), buffers, offsets);

			this->m_pfn_cmd_draw(vulkan_command_buffer, this->m_cube_vertex_count, 2U, 0U, 0U);
		}

		// draw plane
		{
			VkDescriptorSet descriptor_sets[1] = {this->m_point_light_shadow_global_set};
			uint32_t dynamic_offsets[2] = {global_set_per_frame_binding_offset, global_set_plane_per_object_binding_offset};
			this->m_pfn_cmd_bind_descriptor_sets(vulkan_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_point_light_shadow_pipeline_layout, 0U, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);

			VkBuffer buffers[1] = {this->m_cube_vertex_position_buffer};
			VkDeviceSize offsets[1] = {0U};
			this->m_pfn_cmd_bind_vertex_buffers(vulkan_command_buffer, 0U, sizeof(buffers) / sizeof(buffers[0]), buffers, offsets);

			this->m_pfn_cmd_draw(vulkan_command_buffer, this->m_cube_vertex_count, 2U, 0U, 0U);
		}

		this->m_pfn_cmd_end_render_pass(vulkan_command_buffer);

#ifndef NDEBUG
		this->m_pfn_cmd_end_debug_utils_label(vulkan_command_buffer);
#endif
	}

	{
		// Main Camera Pass
#ifndef NDEBUG
		VkDebugUtilsLabelEXT debug_utils_label = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, NULL, "Main Camera Pass", {1.0F, 1.0F, 1.0F, 1.0F}};
		this->m_pfn_cmd_begin_debug_utils_label(vulkan_command_buffer, &debug_utils_label);
#endif
		VkFramebuffer vulkan_framebuffer = this->m_vulkan_main_camera_framebuffers[vulkan_swapchain_image_index];
		VkClearValue clear_values[2] = {
			{{0.0F, 0.0F, 0.0F, 0.0F}},
			{{0.0F, 0U}}};
		VkRenderPassBeginInfo render_pass_begin_info = {
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			NULL,
			this->m_main_camera_render_pass,
			vulkan_framebuffer,
			{{0U, 0U}, {vulkan_framebuffer_width, vulkan_framebuffer_height}},
			sizeof(clear_values) / sizeof(clear_values[0]),
			clear_values,
		};
		this->m_pfn_cmd_begin_render_pass(vulkan_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		this->m_pfn_cmd_bind_pipeline(vulkan_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_forward_shading_pipeline);

		VkViewport viewport = {0.0, 0.0, static_cast<float>(vulkan_framebuffer_width), static_cast<float>(vulkan_framebuffer_height), 0.0F, 1.0F};
		this->m_pfn_cmd_set_viewport(vulkan_command_buffer, 0U, 1U, &viewport);

		VkRect2D scissor = {{0, 0}, {static_cast<uint32_t>(vulkan_framebuffer_width), static_cast<uint32_t>(vulkan_framebuffer_height)}};
		this->m_pfn_cmd_set_scissor(vulkan_command_buffer, 0U, 1U, &scissor);

		// update constant buffer
		uint32_t global_set_per_frame_binding_offset = linear_allocate(vulkan_upload_ring_buffer_current, vulkan_upload_ring_buffer_end, sizeof(forward_shading_layout_global_set_per_frame_uniform_buffer_binding), vulkan_min_uniform_buffer_offset_alignment);
		{
			forward_shading_layout_global_set_per_frame_uniform_buffer_binding *global_set_per_frame_binding = reinterpret_cast<forward_shading_layout_global_set_per_frame_uniform_buffer_binding *>(reinterpret_cast<uintptr_t>(vulkan_upload_ring_buffer_device_memory_pointer) + global_set_per_frame_binding_offset);

			// update camera
			DirectX::XMFLOAT3 eye_position = g_camera_controller.m_eye_position;
			DirectX::XMFLOAT3 eye_direction = g_camera_controller.m_eye_direction;
			DirectX::XMFLOAT3 up_direction = g_camera_controller.m_up_direction;

			DirectX::XMFLOAT4X4 view_transform;
			DirectX::XMStoreFloat4x4(&view_transform, DirectX::XMMatrixLookToRH(DirectX::XMLoadFloat3(&eye_position), DirectX::XMLoadFloat3(&eye_direction), DirectX::XMLoadFloat3(&up_direction)));

			DirectX::XMFLOAT4X4 projection_transform;
			// vulkan viewport flip y
			DirectX::XMFLOAT4X4 mat_vk_y;
			mat_vk_y.m[0][0] = 1.0F;
			mat_vk_y.m[0][1] = 0.0F;
			mat_vk_y.m[0][2] = 0.0F;
			mat_vk_y.m[0][3] = 0.0F;
			mat_vk_y.m[1][0] = 0.0F;
			mat_vk_y.m[1][1] = -1.0F;
			mat_vk_y.m[1][2] = 0.0F;
			mat_vk_y.m[1][3] = 0.0F;
			mat_vk_y.m[2][0] = 0.0F;
			mat_vk_y.m[2][1] = 0.0F;
			mat_vk_y.m[2][2] = 1.0F;
			mat_vk_y.m[2][3] = 0.0F;
			mat_vk_y.m[3][0] = 0.0F;
			mat_vk_y.m[3][1] = 0.0F;
			mat_vk_y.m[3][2] = 0.0F;
			mat_vk_y.m[3][3] = 1.0F;
			DirectX::XMStoreFloat4x4(&projection_transform, DirectX::XMMatrixMultiply(DirectX_Math_Matrix_PerspectiveFovRH_ReversedZ(0.785F, static_cast<float>(vulkan_framebuffer_width) / static_cast<float>(vulkan_framebuffer_height), 0.1F, 100.0F), DirectX::XMLoadFloat4x4(&mat_vk_y)));

			global_set_per_frame_binding->view_transform = view_transform;
			global_set_per_frame_binding->projection_transform = projection_transform;
			global_set_per_frame_binding->point_light_position_and_radius = this->m_point_light_position_and_radius;
			global_set_per_frame_binding->point_light_shadow_bias = this->m_point_light_shadow_bias;
		}

		// update constant buffer
		uint32_t global_set_box_per_object_binding_offset = linear_allocate(vulkan_upload_ring_buffer_current, vulkan_upload_ring_buffer_end, sizeof(forward_shading_layout_global_set_per_object_uniform_buffer_binding), vulkan_min_uniform_buffer_offset_alignment);
		{
			forward_shading_layout_global_set_per_object_uniform_buffer_binding *global_set_box_per_object_binding = reinterpret_cast<forward_shading_layout_global_set_per_object_uniform_buffer_binding *>(reinterpret_cast<uintptr_t>(vulkan_upload_ring_buffer_device_memory_pointer) + global_set_box_per_object_binding_offset);
			global_set_box_per_object_binding->model_transform = box_model_transform;
		}

		// update constant buffer
		uint32_t global_set_plane_per_object_binding_offset = linear_allocate(vulkan_upload_ring_buffer_current, vulkan_upload_ring_buffer_end, sizeof(forward_shading_layout_global_set_per_object_uniform_buffer_binding), vulkan_min_uniform_buffer_offset_alignment);
		{
			forward_shading_layout_global_set_per_object_uniform_buffer_binding *global_set_plane_per_object_binding = reinterpret_cast<forward_shading_layout_global_set_per_object_uniform_buffer_binding *>(reinterpret_cast<uintptr_t>(vulkan_upload_ring_buffer_device_memory_pointer) + global_set_plane_per_object_binding_offset);
			global_set_plane_per_object_binding->model_transform = this->m_plane_model_transform;
		}

		// draw box
		{
			VkDescriptorSet descriptor_sets[1] = {this->m_forward_shading_global_set};
			uint32_t dynamic_offsets[2] = {global_set_per_frame_binding_offset, global_set_box_per_object_binding_offset};
			this->m_pfn_cmd_bind_descriptor_sets(vulkan_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_forward_shading_pipeline_layout, 0U, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);

			VkBuffer buffers[1] = {this->m_cube_vertex_position_buffer};
			VkDeviceSize offsets[1] = {0U};
			this->m_pfn_cmd_bind_vertex_buffers(vulkan_command_buffer, 0U, sizeof(buffers) / sizeof(buffers[0]), buffers, offsets);

			this->m_pfn_cmd_draw(vulkan_command_buffer, this->m_cube_vertex_count, 1U, 0U, 0U);
		}

		// draw plane
		{
			VkDescriptorSet descriptor_sets[1] = {this->m_forward_shading_global_set};
			uint32_t dynamic_offsets[2] = {global_set_per_frame_binding_offset, global_set_plane_per_object_binding_offset};
			this->m_pfn_cmd_bind_descriptor_sets(vulkan_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_forward_shading_pipeline_layout, 0U, sizeof(descriptor_sets) / sizeof(descriptor_sets[0]), descriptor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);

			VkBuffer buffers[1] = {this->m_cube_vertex_position_buffer};
			VkDeviceSize offsets[1] = {0U};
			this->m_pfn_cmd_bind_vertex_buffers(vulkan_command_buffer, 0U, sizeof(buffers) / sizeof(buffers[0]), buffers, offsets);

			this->m_pfn_cmd_draw(vulkan_command_buffer, this->m_cube_vertex_count, 1U, 0U, 0U);
		}

		this->m_pfn_cmd_end_render_pass(vulkan_command_buffer);

#ifndef NDEBUG
		this->m_pfn_cmd_end_debug_utils_label(vulkan_command_buffer);
#endif
	}
}

void Demo::destroy(VkDevice vulkan_device, PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkAllocationCallbacks *vulkan_allocation_callbacks, VmaAllocator *vulkan_asset_allocator)
{
	// TODO:
	// destory resources
}

static inline uint32_t linear_allocate(uint32_t &buffer_current, uint32_t buffer_end, uint32_t size, uint32_t alignment)
{
	uint32_t buffer_offset = utils_align_up(buffer_current, alignment);
	buffer_current = (buffer_offset + size);
	assert(buffer_current < buffer_end);
	return buffer_offset;
}

static inline DirectX::XMMATRIX XM_CALLCONV DirectX_Math_Matrix_PerspectiveFovRH_ReversedZ(float FovAngleY, float AspectRatio, float NearZ, float FarZ)
{
	// [Reversed-Z](https://developer.nvidia.com/content/depth-precision-visualized)
	//
	// _  0  0  0
	// 0  _  0  0
	// 0  0  b -1
	// 0  0  a  0
	//
	// _  0  0  0
	// 0  _  0  0
	// 0  0 zb  -z
	// 0  0  a
	//
	// z' = -b - a/z
	//
	// Standard
	// 0 = -b + a/nearz // z=-nearz
	// 1 = -b + a/farz  // z=-farz
	// a = farz*nearz/(nearz - farz)
	// b = farz/(nearz - farz)
	//
	// Reversed-Z
	// 1 = -b + a/nearz // z=-nearz
	// 0 = -b + a/farz  // z=-farz
	// a = farz*nearz/(farz - nearz)
	// b = nearz/(farz - nearz)

	// __m128 _mm_shuffle_ps(__m128 lo,__m128 hi, _MM_SHUFFLE(hi3,hi2,lo1,lo0))
	// Interleave inputs into low 2 floats and high 2 floats of output. Basically
	// out[0]=lo[lo0];
	// out[1]=lo[lo1];
	// out[2]=hi[hi2];
	// out[3]=hi[hi3];

	// DirectX::XMMatrixPerspectiveFovRH

	float SinFov;
	float CosFov;
	DirectX::XMScalarSinCos(&SinFov, &CosFov, 0.5F * FovAngleY);

	float Height = CosFov / SinFov;
	float Width = Height / AspectRatio;
	float b = NearZ / (FarZ - NearZ);
	float a = (FarZ / (FarZ - NearZ)) * NearZ;
#if defined(_XM_SSE_INTRINSICS_)
	// Note: This is recorded on the stack
	DirectX::XMVECTOR rMem = {
		Width,
		Height,
		b,
		a};

	// Copy from memory to SSE register
	DirectX::XMVECTOR vValues = rMem;
	DirectX::XMVECTOR vTemp = _mm_setzero_ps();
	// Copy x only
	vTemp = _mm_move_ss(vTemp, vValues);
	// CosFov / SinFov,0,0,0
	DirectX::XMMATRIX M;
	M.r[0] = vTemp;
	// 0,Height / AspectRatio,0,0
	vTemp = vValues;
	vTemp = _mm_and_ps(vTemp, DirectX::g_XMMaskY);
	M.r[1] = vTemp;
	// x=b,y=a,0,-1.0f
	vTemp = _mm_setzero_ps();
	vValues = _mm_shuffle_ps(vValues, DirectX::g_XMNegIdentityR3, _MM_SHUFFLE(3, 2, 3, 2));
	// 0,0,b,-1.0f
	vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(3, 0, 0, 0));
	M.r[2] = vTemp;
	// 0,0,a,0.0f
	vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(2, 1, 0, 0));
	M.r[3] = vTemp;
	return M;
#elif defined(_XM_ARM_NEON_INTRINSICS_)
	const float32x4_t Zero = vdupq_n_f32(0);

	DirectX::XMMATRIX M;
	M.r[0] = vsetq_lane_f32(Width, Zero, 0);
	M.r[1] = vsetq_lane_f32(Height, Zero, 1);
	M.r[2] = vsetq_lane_f32(b, DirectX::g_XMNegIdentityR3.v, 2);
	M.r[3] = vsetq_lane_f32(a, Zero, 2);
	return M;
#endif
}
