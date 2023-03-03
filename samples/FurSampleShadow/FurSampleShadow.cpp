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
	- How to use custom constant buffer 
	- How to combine user's own constant buffer with constant buffer needed for hair shader
	- How to render hairs to user's own shadow map
	- How to compute hair shadows in the hair shader

	SAMPLE CODE WORKFLOW:
	1. First we load HairWorks dll (OnD3D11CreateDevice)
	2. After loading the HairWorks dll, we create a custom hair pixel shader and custom buffer.
	3. We initialize the HairWorks runtime with custom resource mapping enabled. (OnD3D11CreateDevice)
	4. We load hair asset and create a hair instance out of it (OnD3D11CreateDevice)
	5. We create textures and the SRVs from texture file name from the asset, and set the SRVs for each hair instance (OnD3D11CreateDevice)
	6. We create a camera (OnD3D11CreateDevice)
	7. At each frame, we run hair simulation (OnFrameMove)
	8. At each frame, we set camera, update rendering parameters, update constant buffer, and render each hair (OnD3D11FrameRender)
	9. On program shutdown, we clean up all hair related resources (OnD3D11DestroyDevice)

	NOTE:
	To simplify samples, we use DXUT for camera control and DX device management.
	However, DXUT is not needed when HairWorks is used in your own DirectX 11 based application.
	All the HairWorks data types start with GFSDK_* (classes) or gfsdk_* (simple types) prefix.
	Please see HairWorksSampleShader.hlsl and the header file HairShaderAttribute.h to see more details on hair shading.

	INSTRUCTIONS:
	Camer Zoom : Mouse Wheel
	Camera Orbit : RMB
	'v' : toggle on/off shadow map visualization

*****************************************************************************************************************/

#include "GFSDK_HairWorks.h" // hairworks main header file

#include "FurSampleAppBase.h" // application wrapper to hide non hair-related codes
#include "FurSampleCommon.h" // general DX utility functions shared among samples
#include "FurSampleHairWorksHelper.h" // convenience functions related to hairworks

using namespace DirectX;

//--------------------------------------------------------------------------------------
// global variables (used only in this file)
//--------------------------------------------------------------------------------------
namespace {
	GFSDK_HairSDK*					g_hairSDK = nullptr; // HairWorks SDK runtime 
	GFSDK_HairAssetDescriptor		g_hairAssetDescriptor;	// hair asset descriptor
	GFSDK_HairAssetID				g_hairAssetID = GFSDK_HairAssetID_NULL;	// hair asset ID
	GFSDK_HairInstanceDescriptor	g_hairInstanceDescriptor; // hair instance descriptor
	GFSDK_HairInstanceID			g_hairInstanceID = GFSDK_HairInstanceID_NULL; // hair instance ID
	
	// apx file path
	char g_apxFilePath[1024];

	// custom pixel shader for hair rendering
	ID3D11PixelShader*	g_customHairWorksShader = 0;

	// custom pixel shader for shadowmap rendering pass
	ID3D11PixelShader*	g_customHairWorksShadowShader = 0;

	// custom constant buffer for use with custom pixel shader
	ID3D11Buffer*		g_hairShaderConstantBuffer = 0;

	// visualize shadow map?
	bool				g_visualizeShadowMap = false;

	// we add additional info about light matrices in our custom constant bufffer
	struct MyConstantBuffer
	{
		XMMATRIX	lightView;
		XMMATRIX	lightWorldToTex;

		GFSDK_HairShaderConstantBuffer	hairShaderConstantBuffer;
	};
}

//--------------------------------------------------------------------------------------
// This function contains all the steps necessary to prepare hair asset and resources
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* device, const DXGI_SURFACE_DESC* backBufferSurfaceDesc, void* userContext)
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
		conversionSettings.m_targetHandednessHint	= GFSDK_HAIR_LEFT_HANDED;
		conversionSettings.m_targetUpAxisHint		= GFSDK_HAIR_Y_UP;
		conversionSettings.m_targetSceneUnit		= 1.0 ; // centimeter
	}

	// Load hair asset from .apx file
	if (GFSDK_HAIR_RETURN_OK != g_hairSDK->LoadHairAssetFromFile(g_apxFilePath, &g_hairAssetID, 0, &conversionSettings))
		return E_FAIL;

	// copy default hair instance descriptor from the loaded asset
	if (GFSDK_HAIR_RETURN_OK != g_hairSDK->CopyInstanceDescriptorFromAsset(g_hairAssetID, g_hairInstanceDescriptor))
		return E_FAIL;

	// Create custom pixel shader for hair rendering
	hr = FurSample_CreatePixelShader(device, "samples\\FurSampleShadow\\HairWorksSampleShader.hlsl", &g_customHairWorksShader);
	if (FAILED(hr))
		return hr;

	// Create custom pixel shader for hair shadow pass
	hr = FurSample_CreatePixelShader(device, "samples\\FurSampleShadow\\HairWorksSampleShadowShader.hlsl", &g_customHairWorksShadowShader);
	if (FAILED(hr))
		return hr;

	// create constant buffer for custom hair shader
	hr = FurSample_CreateConstantBuffer(device, sizeof(MyConstantBuffer), &g_hairShaderConstantBuffer);
	if (FAILED(hr))
		return hr;

	// Initialize DirectX settings for hairworks runtime
	g_hairSDK->InitRenderResources(device);

	// create hair instance
	g_hairSDK->CreateHairInstance(g_hairAssetID, &g_hairInstanceID);

	// set up texture resources
	FurSample_SetupTextureResource(device, g_hairSDK, g_hairAssetID, g_hairInstanceID, g_apxFilePath);

	// set up default DXUT camera
	XMVECTOR modelCenter = XMVectorSet(0,175, 0, 0);
	XMVECTOR camCenter = XMVectorAdd(modelCenter, XMVectorSet(0, 0, -50, 0));

	FurSampleAppBase::InitDefaultCamera(camCenter, modelCenter);

	// set up default light camera
	XMVECTOR up = XMVectorSet(0.0, 1.0, 0.0, 0);
	XMVECTOR lightCenter = XMVectorAdd(modelCenter, XMVectorSet(0.0f, 20.0f, -50.0f, 0));
	FurSampleAppBase::InitDefaultLight(lightCenter, modelCenter, up);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* userContext)
{
	FurSampleAppBase::OnDestroyDevice();

	SAFE_RELEASE(g_hairShaderConstantBuffer);
	SAFE_RELEASE(g_customHairWorksShader);
	SAFE_RELEASE(g_customHairWorksShadowShader);

	g_hairSDK->FreeHairInstance(g_hairInstanceID);
	g_hairSDK->FreeHairAsset(g_hairAssetID);
	g_hairSDK->FreeRenderResources();
	g_hairSDK->Release();
}

//--------------------------------------------------------------------------------------
// Called at every frame. This is the main function for hair rendering
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device* device, ID3D11DeviceContext* context, double time, float elapsedTime, void* userContext)
{
	// Set render context for HairWorks
	g_hairSDK->SetCurrentContext(context);

	// update all the rendering control variables
	g_hairInstanceDescriptor.m_shadowSigma = 2.0f;
	g_hairInstanceDescriptor.m_useBackfaceCulling = false;

	g_hairSDK->UpdateInstanceDescriptor(g_hairInstanceID, g_hairInstanceDescriptor);

	// prepare shadow map render target, etc. (usually happens in your app/engine)
	FurSampleAppBase::PrepareShadowTarget(context);

	// shadow rendering pass ..
	{
		// 1. ... render other scene objects to shadow map...

		// 2. get shadow camera's view projection matrix and set to HairWorks
		{
			XMMATRIX projection	= FurSampleAppBase::GetLightProjection();
			XMMATRIX view			= FurSampleAppBase::GetLightViewMatrix();
			g_hairSDK->SetViewProjection((const gfsdk_float4x4*)&view,(const gfsdk_float4x4*)&projection, GFSDK_HAIR_LEFT_HANDED);
		}

		// 3. set shader settings and render hairs with our custom hair shadow shader
		{
			// use custom shadow pass shader
			context->PSSetShader(g_customHairWorksShadowShader, NULL, 0);

			// render hairs to shadow buffer
			GFSDK_HairShaderSettings settings;

			settings.m_useCustomConstantBuffer = false;
			settings.m_shadowPass = true;

			g_hairSDK->RenderHairs(g_hairInstanceID, &settings);
		}
	}

	// prepare render target for scene rendering
	FurSampleAppBase::PrepareRenderTarget(context);

	// ... render other scene objects ...

	// hair rendering pass
	{
		// 1. get view and projection matrix from your app, set to HairWorks
		{
			XMMATRIX projection	= FurSampleAppBase::GetCameraProjection();
			XMMATRIX view		= FurSampleAppBase::GetCameraViewMatrix();
			g_hairSDK->SetViewProjection((const gfsdk_float4x4*)&view,(const gfsdk_float4x4*)&projection, GFSDK_HAIR_LEFT_HANDED);
		}

		// 2. prepare HairWorks constant buffer and copy shadow matrices to our custom buffer
		{
			D3D11_MAPPED_SUBRESOURCE mappedSubResource;		
			context->Map( g_hairShaderConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource);
			MyConstantBuffer* constantBuffer = ( MyConstantBuffer* )mappedSubResource.pData;
			{
				// copy additional data we defined (light matrices)
				constantBuffer->lightView			= FurSampleAppBase::GetLightViewMatrix();
				constantBuffer->lightWorldToTex	= FurSampleAppBase::GetLightWorldToTex();
				// use HairWorks API to fill the HairWorks portion of constant buffer
				g_hairSDK->PrepareShaderConstantBuffer(g_hairInstanceID, &constantBuffer->hairShaderConstantBuffer);

			}	
			context->Unmap(g_hairShaderConstantBuffer,0);
			context->PSSetConstantBuffers(0, 1, &g_hairShaderConstantBuffer);
		}
	
		// 3. set resources for hair shader (color textures, shadow tex, attribute resources)
		// The resource mapping here should match decalartions in the shader.
		{
			// get standard shader resources for attribute interpolation
			ID3D11ShaderResourceView* srvs[GFSDK_HAIR_NUM_SHADER_RESOUCES];
			g_hairSDK->GetShaderResources(g_hairInstanceID, srvs);
			context->PSSetShaderResources( 0, GFSDK_HAIR_NUM_SHADER_RESOUCES, srvs);

			// set texture resource
			ID3D11ShaderResourceView* textureSRVs[3];

			g_hairSDK->GetTextureSRV(g_hairInstanceID, GFSDK_HAIR_TEXTURE_ROOT_COLOR, &textureSRVs[0]);
			g_hairSDK->GetTextureSRV(g_hairInstanceID, GFSDK_HAIR_TEXTURE_TIP_COLOR, &textureSRVs[1]);

			// shadow srv, which is user side creation
			textureSRVs[2] = FurSampleAppBase::GetShadowSRV();

			context->PSSetShaderResources( GFSDK_HAIR_NUM_SHADER_RESOUCES, 3, textureSRVs);
		}

		// 4. set texture sampler for color sampling and shadow PCF sampling
		{
			ID3D11SamplerState *states[2] = {
				FurSampleAppBase::GetSamplerLinear(), 
				FurSampleAppBase::GetSamplerPointClamp() 
			};
			context->PSSetSamplers( 0, 2, states );
		}

		// 5. set hair shader and render hairs
		{
			// set your hair pixel shader before rendering hairs
			context->PSSetShader(g_customHairWorksShader, NULL, 0);

			// set shader settings.  
			GFSDK_HairShaderSettings settings;
			settings.m_useCustomConstantBuffer = true; // we use our own constant buffer

			// Render the hair instance
			g_hairSDK->RenderHairs(g_hairInstanceID, &settings);
		}

		// 6. clear shader resource references
		{
			const int numResources = GFSDK_HAIR_NUM_SHADER_RESOUCES + 3;
			ID3D11ShaderResourceView* nullResources[numResources] = { NULL };
			context->PSSetShaderResources(0, numResources, nullResources);
		}
	}

	if (g_visualizeShadowMap)
		FurSampleAppBase::VisualizeShadowMap(context);
}

//--------------------------------------------------------------------------------------
// Called once per frame when animation is on.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double time, float elapsedTime, void* userContext)
{
	if (S_OK != FurSampleAppBase::OnFrameMove(time, elapsedTime, userContext))
		return;

	ID3D11DeviceContext* context = FurSampleAppBase::GetDeviceContext();

	// update simulation parameters
	g_hairSDK->UpdateInstanceDescriptor(g_hairInstanceID, g_hairInstanceDescriptor);

	// Set simulation context for HairWorks 
	g_hairSDK->SetCurrentContext(context);

	// run simulation for all hairs
	g_hairSDK->StepSimulation(FurSampleAppBase::GetSimulationTimeStep());
}

//--------------------------------------------------------------------------------------
// Process key events
//--------------------------------------------------------------------------------------
#define TOGGLE(x) { x = !x; }

void CALLBACK OnKeyboard(UINT character, bool keyDown, bool altDown, void* userContext)
{	
	if (!keyDown)
		return;

	switch(character)
	{
		case 'v': case 'V':  
			TOGGLE(g_visualizeShadowMap); 
			break;
		case 's': case 'S':  
			TOGGLE(g_hairInstanceDescriptor.m_receiveShadows); 
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
	const char* fileName = strlen(arg1) > 0 ? arg1 : "media\\HumanSamples\\Female\\Eve\\gm_main.apx";

	FurSample_GetSampleMediaFilePath(fileName, g_apxFilePath);

	// Set DXUT callbacks
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);
	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);

	DXUTSetCallbackKeyboard(OnKeyboard);

	return FurSampleAppBase::WinMain(L"FurSampleShadow");
}

