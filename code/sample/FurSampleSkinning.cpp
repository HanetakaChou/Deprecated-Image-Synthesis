// This code contains NVIDIA Confidential Information and is disclosed
// under the Mutual Non-Disclosure Agreement.
//
// Notice
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
//
// NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless
// expressly authorized by NVIDIA.  Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

/************************************************************************************************************
	SUMMARY:
	This sample shows how to setup skinning matrices for HairWorks.

	HIGHLIGHTS:
	This sample updates the skinning matrices using UpdateSkinningMatrices() API.

	To visualize how the bone hierarchy is, set m_visualizeBones of GFSDK_HairInstanceDescriptor to true and call RenderVisualization().

	SAMPLE CODE WORKFLOW:
	1. First we load HairWorks dll (OnD3D11CreateDevice)
	2. After loading the HairWorks dll, we create a custom hair pixel shader and custom buffer.
	3. We initialize the HairWorks runtime with custom resource mapping enabled. (OnD3D11CreateDevice)
	4. We load hair asset and create a hair instance out of it (OnD3D11CreateDevice)
	5. We create textures and the SRVs from texture file name from the asset, and set the SRVs for each hair instance (OnD3D11CreateDevice)
	6. At each frame, we update skinning matrices and run hair simulation (OnFrameMove)
	7. At each frame, we set camera, update rendering parameters, update constant buffer, and render each hair (OnD3D11FrameRender)
	8. On program shutdown, we clean up all hair related resources (OnD3D11DestroyDevice)

	NOTE:

	INSTRUCTIONS:
	Camer Zoom : Mouse Wheel
	Camera Orbit : RMB
	'p' : toggle on/off animation
	'b' : toggle on/off visualizing bone hierarchy
	'g' : toggle on/off visualizing growth mesh

*****************************************************************************************************************/

#include "GFSDK_HairWorks.h" // hairworks main header file

#include "FurSampleAppBase.h"		  // application wrapper to hide non hair-related codes
#include "FurSampleCommon.h"		  // general DX utility functions shared among samples
#include "FurSampleHairWorksHelper.h" // convenience functions related to hairworks
#include "FurSampleMesh.h"			  // sample mesh loading/rendering functions

#include "FurSampleVector.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// global variables (used only in this file)
//--------------------------------------------------------------------------------------
namespace
{
	GFSDK_HairSDK *g_hairSDK = nullptr;								   // HairWorks SDK runtime
	GFSDK_HairAssetDescriptor g_hairAssetDescriptor;				   // hair asset descriptor
	GFSDK_HairAssetID g_hairAssetID = GFSDK_HairAssetID_NULL;		   // hair asset ID
	GFSDK_HairInstanceDescriptor g_hairInstanceDescriptor;			   // hair instance descriptor
	GFSDK_HairInstanceID g_hairInstanceID = GFSDK_HairInstanceID_NULL; // hair instance ID

	// apx file path
	char g_apxFilePath[1024];

	// custom pixel shader for hair rendering
	ID3D11PixelShader *g_customHairWorksShader = 0;

	// To use animation or not
	bool g_useAnimation = true;

	// To show bone hierarchy or not
	bool g_showBones = false;

	// To show growth mesh or not
	bool g_showGrowthMesh = false;

	// To show hairs or not
	bool g_showHairs = true;

	// To simulate hairs or not
	bool g_simulateHairs = true;

	// For mesh rendering
	FurSampleSkinnedMeshes g_meshes;

	// The following header includes g_numSkinningBones, g_numFrames and g_skinningMatrices for skinning
#include "SkinningMatrices.h"

	// Memory allocations for the mesh
	struct TempMeshData
	{
		gfsdk_U32 m_numFaces; // Number of triangle faces

		FurSample_Vector<XMFLOAT3> m_positions;	  // Positions
		FurSample_Vector<gfsdk_U32> m_indices;	  // Indices
		FurSample_Vector<XMVECTOR> m_boneWeights; // Bone weights
		FurSample_Vector<XMVECTOR> m_boneIndices; // Bone indices

		TempMeshData(gfsdk_U32 numPositions, gfsdk_U32 numFaces)
		{
			m_numFaces = numFaces;
			m_positions.setSize(numPositions);
			m_indices.setSize(numFaces * 3);
			m_boneWeights.setSize(numPositions);
			m_boneIndices.setSize(numPositions);
		}
	};

}

//--------------------------------------------------------------------------------------
// This function contains all the steps necessary to prepare hair asset and resources
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device *device, const DXGI_SURFACE_DESC *backBufferSurfaceDesc, void *userContext)
{
	HRESULT hr;
	// initialize sample app
	FurSampleAppBase::OnCreateDevice(device);

	// load hairworks dll
	g_hairSDK = FurSample_LoadHairWorksDLL();
	if (!g_hairSDK)
		return E_FAIL;

	// conversion settings to load apx file into.  We use l.h.s / y-up camera in this example.
	// note that actual asset might have used different coord system (e.g. r.h.s/z-up), so we need this setting to be properly set.
	GFSDK_HairConversionSettings conversionSettings;
	{
		conversionSettings.m_targetHandednessHint = GFSDK_HAIR_LEFT_HANDED;
		conversionSettings.m_targetUpAxisHint = GFSDK_HAIR_Y_UP;
		conversionSettings.m_targetSceneUnit = 1.0; // centimeter
	}

	// Load hair asset from .apx file
	if (GFSDK_HAIR_RETURN_OK != g_hairSDK->LoadHairAssetFromFile(g_apxFilePath, &g_hairAssetID, 0, &conversionSettings))
		return E_FAIL;

	if (GFSDK_HAIR_RETURN_OK != g_hairSDK->CopyInstanceDescriptorFromAsset(g_hairAssetID, g_hairInstanceDescriptor))
		return E_FAIL;

	// Create custom pixel shader for HairWorks rendering
	const char *shaderFile = "samples\\FurSampleSkinning\\HairWorksSampleShader.hlsl";
	hr = FurSample_CreatePixelShader(device, shaderFile, &g_customHairWorksShader);
	if (FAILED(hr))
		return hr;

	// Initialize DirectX settings for hairworks runtime
	g_hairSDK->InitRenderResources(device);

	// create hair instance
	g_hairSDK->CreateHairInstance(g_hairAssetID, &g_hairInstanceID);

	// set up texture resources
	FurSample_SetupTextureResource(device, g_hairSDK, g_hairAssetID, g_hairInstanceID, g_apxFilePath);

	// Create a mesh for the growth mesh visualization
	hr = g_meshes.Init(device);
	if (FAILED(hr))
		return hr;

	gfsdk_U32 numPositions = 0;
	gfsdk_U32 numFaces = 0;

	// Get the number of vertices from the hair growth mesh
	if (GFSDK_HAIR_RETURN_OK != g_hairSDK->GetNumHairVertices(g_hairAssetID, &numPositions))
	{
		return E_FAIL;
	}

	// Get the number of faces from the hair growth mesh
	if (GFSDK_HAIR_RETURN_OK != g_hairSDK->GetNumFaces(g_hairAssetID, &numFaces))
	{
		return E_FAIL;
	}

	// set up default DXUT camera
	XMVECTOR modelCenter = XMVectorSet(0, 40, 0, 0);
	XMVECTOR camCenter = XMVectorSet(-250, 40, 50, 0);

	FurSampleAppBase::InitDefaultCamera(camCenter, modelCenter);

	// Create mesh data.
	// Note that code below is only for illustration of hairworks query API.
	// In actual engine, you would have created this mesh elsewhere.
	// The mesh should have same bone structure as hair asset. (see SetBoneRemapping() API if that's not the case).
	if ((numPositions > 0) && (numFaces > 0))
	{
		TempMeshData meshData(numPositions, numFaces);

		// Fill the vertices info (root of each hair == vertex of the mesh)
		if (GFSDK_HAIR_RETURN_OK != g_hairSDK->GetRootVertices(g_hairAssetID, (gfsdk_float3 *)meshData.m_positions.data()))
		{
			return E_FAIL;
		}

		// Fill the bone indices info
		if (GFSDK_HAIR_RETURN_OK != g_hairSDK->GetBoneIndices(g_hairAssetID, (gfsdk_float4 *)meshData.m_boneIndices.data()))
		{
			return E_FAIL;
		}

		// Fill the bone weights info
		if (GFSDK_HAIR_RETURN_OK != g_hairSDK->GetBoneWeights(g_hairAssetID, (gfsdk_float4 *)meshData.m_boneWeights.data()))
		{
			return E_FAIL;
		}

		// Fill the face indices info
		if (GFSDK_HAIR_RETURN_OK != g_hairSDK->GetFaceIndices(g_hairAssetID, meshData.m_indices.data()))
		{
			return E_FAIL;
		}

		// Build a skinned mesh from the given geometry info
		hr = g_meshes.LoadSkinnedMesh(device,
									  (const XMFLOAT3 *)meshData.m_positions.data(), int(meshData.m_positions.getSize()),
									  (const unsigned int *)meshData.m_indices.data(), meshData.m_numFaces * 3,
									  (const XMVECTOR *)meshData.m_boneIndices.data(),
									  (const XMVECTOR *)meshData.m_boneWeights.data());

		if (FAILED(hr))
			return hr;
	}

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void *userContext)
{
	g_meshes.Release();

	FurSampleAppBase::OnDestroyDevice();

	SAFE_RELEASE(g_customHairWorksShader);

	g_hairSDK->FreeHairInstance(g_hairInstanceID);
	g_hairSDK->FreeHairAsset(g_hairAssetID);
	g_hairSDK->FreeRenderResources();
	g_hairSDK->Release();
}

//--------------------------------------------------------------------------------------
// Called at every frame. This is the main function for hair rendering
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device *device, ID3D11DeviceContext *context, double time, float elapsedTime, void *userContext)
{
	// prepare render target to render hair into
	FurSampleAppBase::PrepareRenderTarget(context);

	// Set render context for HairWorks
	g_hairSDK->SetCurrentContext(context);

	// Set view matrix and projection matrix
	XMMATRIX projection = FurSampleAppBase::GetCameraProjection();
	XMMATRIX view = FurSampleAppBase::GetCameraViewMatrix();
	g_hairSDK->SetViewProjection((const gfsdk_float4x4 *)&view, (const gfsdk_float4x4 *)&projection, GFSDK_HAIR_LEFT_HANDED);

	// Render polygonal skinned mesh (replace with your mesh code)
	{
		context->VSSetShader(g_meshes.m_skinnedVS, nullptr, 0);
		context->PSSetShader(g_meshes.m_skinnedPS, nullptr, 0);

		// Set input layout
		context->IASetInputLayout(g_meshes.m_skinnedInputLayout);

		// Render skinned meshes
		g_meshes.RenderMeshes(device, context, (const float *)&view, (const float *)&projection);
	}

	// set hair control options
	g_hairInstanceDescriptor.m_visualizeBones = g_showBones;
	g_hairInstanceDescriptor.m_visualizeGrowthMesh = g_showGrowthMesh;
	g_hairInstanceDescriptor.m_simulate = g_simulateHairs;
	g_hairInstanceDescriptor.m_drawRenderHairs = g_showHairs;
	g_hairInstanceDescriptor.m_useBackfaceCulling = false;

	g_hairSDK->UpdateInstanceDescriptor(g_hairInstanceID, g_hairInstanceDescriptor);

	// set your hair pixel shader before rendering hairs
	context->PSSetShader(g_customHairWorksShader, NULL, 0);

	// set texture sampler for texture color in hair shader
	{
		ID3D11SamplerState *states[1] = {FurSampleAppBase::GetSamplerLinear()};
		context->PSSetSamplers(0, 1, states);
	}

	// get standard shader resources for attribute interpolation
	{
		ID3D11ShaderResourceView *srvs[GFSDK_HAIR_NUM_SHADER_RESOUCES];
		g_hairSDK->GetShaderResources(g_hairInstanceID, srvs);
		context->PSSetShaderResources(0, GFSDK_HAIR_NUM_SHADER_RESOUCES, srvs);
	}

	// set textures
	{
		ID3D11ShaderResourceView *textureSRVs[2];
		g_hairSDK->GetTextureSRV(g_hairInstanceID, GFSDK_HAIR_TEXTURE_ROOT_COLOR, &textureSRVs[0]);
		g_hairSDK->GetTextureSRV(g_hairInstanceID, GFSDK_HAIR_TEXTURE_TIP_COLOR, &textureSRVs[1]);
		context->PSSetShaderResources(GFSDK_HAIR_NUM_SHADER_RESOUCES, 2, textureSRVs);
	}

	// render hairs with default shader settings
	g_hairSDK->RenderHairs(g_hairInstanceID);

	// Render visualization
	g_hairSDK->RenderVisualization(g_hairInstanceID);

	{
		// Unbind
		const int numResources = GFSDK_HAIR_NUM_SHADER_RESOUCES + 2;
		ID3D11ShaderResourceView *nullResources[numResources] = {nullptr};
		context->PSSetShaderResources(0, numResources, nullResources);
	}
}

//--------------------------------------------------------------------------------------
// Called once per frame when animation is on.
// We update user parameters, update skinning and call hair simulation hair
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double time, float elapsedTime, void *userContext)
{
	if (S_OK != FurSampleAppBase::OnFrameMove(time, elapsedTime, userContext))
		return;

	ID3D11DeviceContext *context = FurSampleAppBase::GetDeviceContext();

	// Set simulation context for HairWorks
	g_hairSDK->SetCurrentContext(context);

	static int s_frame = 0;

	if (g_useAnimation)
		s_frame++;

	if (s_frame >= g_numFrames)
		s_frame = 0;

	// Makes sure its 16 byte aligned
	const XMMATRIX *frameSkinningMatrices = (const XMMATRIX *)g_skinningMatrices[s_frame];
	assert((size_t(frameSkinningMatrices) & 0xf) == 0);

	// Update polygonal mesh animation
	g_meshes.UpdateMeshes(g_numSkinningBones, frameSkinningMatrices);

	// Update skinning matrices for the frame
	g_hairSDK->UpdateSkinningMatrices(g_hairInstanceID, g_numSkinningBones, g_skinningMatrices[s_frame]);

	// run simulation for all hairs
	g_hairSDK->StepSimulation(FurSampleAppBase::GetSimulationTimeStep());
}

#define TOGGLE(x) \
	{             \
		x = !x;   \
	}

void CALLBACK OnKeyboard(UINT character, bool keyDown, bool altDown, void *userContext)
{
	if (!keyDown)
		return;

	switch (character)
	{
	case ' ':
	case 'p':
	case 'P':
		TOGGLE(g_useAnimation);
		break;
	case 'b':
	case 'B':
		TOGGLE(g_showBones);
		break;
	case 'g':
	case 'G':
		TOGGLE(g_showGrowthMesh);
		break;
	case 'h':
	case 'H':
		TOGGLE(g_showHairs);
		break;
	case 's':
	case 'S':
		TOGGLE(g_simulateHairs);
		break;
	}
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
	// Get the apx file path from command line
	char arg1[1024];
	wcstombs(arg1, cmdLine, 1024);

	// find file path for sample apx file
	const char *fileName = strlen(arg1) > 0 ? arg1 : "media\\Manjaladon\\Maya\\Manjaladon_wFur.apx";

	FurSample_GetSampleMediaFilePath(fileName, g_apxFilePath);

	// Set DXUT callbacks
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);
	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);

	DXUTSetCallbackKeyboard(OnKeyboard);

	return FurSampleAppBase::WinMain(L"FurSampleSkinning");
}
