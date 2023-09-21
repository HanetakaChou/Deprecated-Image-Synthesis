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
	The sample loads a hair asset from .apx file and textures.
	It also creates a custom pixel shader to render hair with textures.
	Then it simulates and renders hairs with custom pixel shader.

	HIGHLIGHTS:
	This code sample shows
	- How to set up hair control textures and pass it to the custom pixel shader

	SAMPLE CODE WORKFLOW:
	1. First we load HairWorks dll (OnD3D11CreateDevice)
	2. After loading the HairWorks dll, we create a custom hair pixel shader and initialize the runtime. (OnD3D11CreateDevice)
	3. We load hair asset and create a hair instance out of it (OnD3D11CreateDevice)
	4. We create textures and the SRVs from texture file name from the asset, and set the SRVs for each hair instance (OnD3D11CreateDevice)
	5. At each frame, we run hair simulation (OnFrameMove)
	6. At each frame, we set camera, update rendering parameters, and render each hair (OnD3D11FrameRender)
	7. On program shutdown, we clean up all hair related resources (OnD3D11DestroyDevice)

	NOTE:
	The shader for this sample is located at samples\\FurSampleShading\\HairWorksSampleShader.hlsl
	Try to change the shader and see how it affects the hair rendering result.
	Note however that GFSDK_HairWorks_ShaderCommon.h is owned by HairWorks runtime and should not be modified.

*****************************************************************************************************************/

#include <DirectXMath.h>
#include "GFSDK_HairWorks.h" // hairworks main header file

#include "FurSampleAppBase.h"		  // application wrapper to hide non hair-related codes
#include "FurSampleCommon.h"		  // general DX utility functions shared among samples
#include "FurSampleHairWorksHelper.h" // convenience functions related to hairworks

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
}

//--------------------------------------------------------------------------------------
// This function contains all the steps necessary to prepare hair asset and resources
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device *pd3dDevice, const DXGI_SURFACE_DESC *pBackBufferSurfaceDesc, void *pUserContext)
{
	HRESULT hr;

	// initialize sample app
	FurSampleAppBase::OnCreateDevice(pd3dDevice);

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

	// copy default instance descriptor from the loaded asset
	if (GFSDK_HAIR_RETURN_OK != g_hairSDK->CopyInstanceDescriptorFromAsset(g_hairAssetID, g_hairInstanceDescriptor))
		return E_FAIL;

	// Create custom pixel shader for HairWorks rendering
	hr = FurSample_CreatePixelShader(pd3dDevice, "samples\\FurSampleShading\\HairWorksSampleShader.hlsl", &g_customHairWorksShader);
	if (FAILED(hr))
		return hr;

	// Initialize DirectX settings for hairworks runtime
	g_hairSDK->InitRenderResources(pd3dDevice);

	// create hair instance
	g_hairSDK->CreateHairInstance(g_hairAssetID, &g_hairInstanceID);

	// set up texture resources
	FurSample_SetupTextureResource(pd3dDevice, g_hairSDK, g_hairAssetID, g_hairInstanceID, g_apxFilePath);

	// set up default DXUT camera
	XMVECTOR modelCenter = XMVectorSet(0, 175, 0, 0);
	XMVECTOR camCenter = XMVectorAdd(modelCenter, XMVectorSet(0, 0, -50, 0));

	FurSampleAppBase::InitDefaultCamera(camCenter, modelCenter);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void *pUserContext)
{
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
void CALLBACK OnD3D11FrameRender(ID3D11Device *device, ID3D11DeviceContext *context, double fTime, float fElapsedTime, void *pUserContext)
{
	// prepare render target to render hair into
	FurSampleAppBase::PrepareRenderTarget(context);

	// modify instance descriptor.
	g_hairInstanceDescriptor.m_useBackfaceCulling = false;

	g_hairSDK->UpdateInstanceDescriptor(g_hairInstanceID, g_hairInstanceDescriptor);

	// Set render context for HairWorks
	g_hairSDK->SetCurrentContext(context);

	// Set view matrix and projection matrix
	XMMATRIX projection = FurSampleAppBase::GetCameraProjection();
	XMMATRIX view = FurSampleAppBase::GetCameraViewMatrix();
	g_hairSDK->SetViewProjection((const gfsdk_float4x4 *)&view, (const gfsdk_float4x4 *)&projection, GFSDK_HAIR_LEFT_HANDED);

	// set your hair pixel shader before rendering hairs
	context->PSSetShader(g_customHairWorksShader, NULL, 0);

	// set texture sampler for texture color in hair shader
	{
		ID3D11SamplerState *states[] = {FurSampleAppBase::GetSamplerLinear()};
		context->PSSetSamplers(0, 1, states);
	}

	// get standard shader resources for attribute interpolation
	ID3D11ShaderResourceView *ppSRV[GFSDK_HAIR_NUM_SHADER_RESOUCES];
	g_hairSDK->GetShaderResources(g_hairInstanceID, ppSRV);

	// set to resource slot for our shader
	context->PSSetShaderResources(0, GFSDK_HAIR_NUM_SHADER_RESOUCES, ppSRV);

	// set textures
	{
		ID3D11ShaderResourceView *textureSRVs[2];
		g_hairSDK->GetTextureSRV(g_hairInstanceID, GFSDK_HAIR_TEXTURE_ROOT_COLOR, &textureSRVs[0]);
		g_hairSDK->GetTextureSRV(g_hairInstanceID, GFSDK_HAIR_TEXTURE_TIP_COLOR, &textureSRVs[1]);
		// set to resource slot for our shader
		context->PSSetShaderResources(GFSDK_HAIR_NUM_SHADER_RESOUCES, 2, textureSRVs);
	}

	// Render the hair instance
	g_hairSDK->RenderHairs(g_hairInstanceID);

	// render visualization if any
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
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double time, float elapsedTime, void *userContext)
{
	if (S_OK != FurSampleAppBase::OnFrameMove(time, elapsedTime, userContext))
		return;

	ID3D11DeviceContext *pd3dContext = FurSampleAppBase::GetDeviceContext();

	// Set render context for HairWorks
	g_hairSDK->SetCurrentContext(pd3dContext);

	g_hairInstanceDescriptor.m_simulate = true;

	g_hairSDK->UpdateInstanceDescriptor(g_hairInstanceID, g_hairInstanceDescriptor);

	// run simulation for all hairs
	g_hairSDK->StepSimulation(FurSampleAppBase::GetSimulationTimeStep());
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
	const char *fileName = strlen(arg1) > 0 ? arg1 : "media\\HumanSamples\\Female\\Eve\\gm_main.apx";

	FurSample_GetSampleMediaFilePath(fileName, g_apxFilePath);

	// Set DXUT callbacks
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);
	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);

	return FurSampleAppBase::WinMain(L"FurSampleShading");
}
