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

#include "FurSampleAppBase.h"
#include "FurSampleCommon.h" // simple utility functions shared among fur samples

using namespace DirectX;

//--------------------------------------------------------------------------------------
int FurSampleAppBase::m_width = 1024;
int FurSampleAppBase::m_height = 768;
CModelViewerCamera FurSampleAppBase::m_camera;
D3D11_VIEWPORT FurSampleAppBase::m_viewport;

DirectX::XMMATRIX FurSampleAppBase::m_lightView;
DirectX::XMMATRIX FurSampleAppBase::m_lightProjection;
DirectX::XMMATRIX FurSampleAppBase::m_lightWorldToTex;

ID3D11SamplerState *FurSampleAppBase::m_texSamplerLinear = 0;
ID3D11SamplerState *FurSampleAppBase::m_texSamplerPointClamp = 0;

ID3D11VertexShader *FurSampleAppBase::m_visualizeShadowVS = 0;
ID3D11PixelShader *FurSampleAppBase::m_visualizeShadowPS = 0;

FurSampleShadowMap FurSampleAppBase::m_shadowMap;

FurTimeStepFilter FurSampleAppBase::m_timeStepFilter;

//--------------------------------------------------------------------------------------
HRESULT FurSampleAppBase::OnCreateDevice(ID3D11Device *device)
{
	HRESULT hr;
	// create an example texture sampler
	FurSample_CreateTextureSampler(device, &m_texSamplerLinear);
	FurSample_CreateTextureSampler(device, &m_texSamplerPointClamp);

	// create utility shaders
	hr = FurSample_CreatePixelShader(device, "samples\\FurSampleCommon\\HairWorksSampleVisualizeShadow.hlsl", &m_visualizeShadowPS);
	if (FAILED(hr))
		return hr;

	// Create custom pixel shader for HairWorks shadow pass
	hr = FurSample_CreateVertexShader(device, "samples\\FurSampleCommon\\HairWorksSampleVisualizeShadow.hlsl", &m_visualizeShadowVS);
	if (FAILED(hr))
		return hr;

	// initialize shadow map
	m_shadowMap.Init(device, 1024);

	return S_OK;
}

//--------------------------------------------------------------------------------------
void FurSampleAppBase::OnDestroyDevice()
{
	SAFE_RELEASE(m_texSamplerLinear);
	SAFE_RELEASE(m_texSamplerPointClamp);

	SAFE_RELEASE(m_visualizeShadowVS);
	SAFE_RELEASE(m_visualizeShadowPS);

	m_shadowMap.Release();
}

//--------------------------------------------------------------------------------------
ID3D11DeviceContext *
FurSampleAppBase::GetDeviceContext()
{
	return DXUTGetD3D11DeviceContext();
}

//--------------------------------------------------------------------------------------
ID3D11Device *
FurSampleAppBase::GetDevice()
{
	return DXUTGetD3D11Device();
}

//--------------------------------------------------------------------------------------
void FurSampleAppBase::InitDefaultCamera(FXMVECTOR eyePos, FXMVECTOR at)
{
	m_camera.SetViewParams(eyePos, at);
	m_camera.SetEnablePositionMovement(true);
	m_camera.SetScalers(0.005f, 0.002f);
}

//--------------------------------------------------------------------------------------
void FurSampleAppBase::InitDefaultLight(FXMVECTOR eye, FXMVECTOR at, FXMVECTOR up)
{
	float sizeX = 50.0f;
	float sizeY = 50.0f;
	float znear = -200.0f;
	float zfar = 200.0f;

	SetupLightMatrices(eye, at, up, sizeX, sizeY, znear, zfar);
}

//--------------------------------------------------------------------------------------
void FurSampleAppBase::SetupLightMatrices(FXMVECTOR eye, FXMVECTOR lookAt, FXMVECTOR up, float sizeX, float sizeY, float zNear, float zFar)
{
	m_lightView = XMMatrixLookAtLH(eye, lookAt, up);

	m_lightProjection = XMMatrixOrthographicLH(sizeX, sizeY, zNear, zFar);

	DirectX::XMMATRIX clip2Tex(0.5, 0, 0, 0,
							   0, -0.5, 0, 0,
							   0, 0, 1, 0,
							   0.5, 0.5, 0, 1);

	DirectX::XMMATRIX viewProjection = m_lightView * m_lightProjection;
	m_lightWorldToTex = viewProjection * clip2Tex;
}

//--------------------------------------------------------------------------------------
void FurSampleAppBase::SetViewport()
{
	m_viewport.Width = float(m_width);
	m_viewport.Height = float(m_height);
	m_viewport.MinDepth = 0;
	m_viewport.MaxDepth = 1;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
}

//--------------------------------------------------------------------------------------
void FurSampleAppBase::PrepareRenderTarget(ID3D11DeviceContext *pd3dContext)
{
	// Get the depth stencil and render target
	ID3D11RenderTargetView *pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView *pDSV = DXUTGetD3D11DepthStencilView();

	pd3dContext->OMSetRenderTargets(1, &pRTV, pDSV);

	const XMVECTOR color = XMVectorSet(0.5f, 0.5f, 0.5f, 1.0f);
	pd3dContext->ClearRenderTargetView(pRTV, (const FLOAT *)&color);
	pd3dContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0, 0);

	UINT numViewports = 1;
	pd3dContext->RSSetViewports(numViewports, &m_viewport);
}

//--------------------------------------------------------------------------------------
void FurSampleAppBase::PrepareShadowTarget(ID3D11DeviceContext *context)
{
	context->OMSetRenderTargets(1, &m_shadowMap.m_shadowRTV, m_shadowMap.m_depthDSV);

	context->RSSetViewports(1, &m_shadowMap.m_viewport);

	float clearDepth = FLT_MAX;
	float ClearColor[4] = {clearDepth, clearDepth, clearDepth, clearDepth};

	context->ClearRenderTargetView(m_shadowMap.m_shadowRTV, ClearColor);
	context->ClearDepthStencilView(m_shadowMap.m_depthDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
}

//--------------------------------------------------------------------------------------
ID3D11ShaderResourceView *FurSampleAppBase::GetShadowSRV()
{
	return m_shadowMap.m_shadowSRV;
}

//--------------------------------------------------------------------------------------
void FurSampleAppBase::VisualizeShadowMap(ID3D11DeviceContext *context)
{
	// render shadow map
	context->IASetInputLayout(NULL);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	context->VSSetShader(m_visualizeShadowVS, NULL, 0);
	context->PSSetShader(m_visualizeShadowPS, NULL, 0);

	// set texture sampler for texture color in hair shader
	ID3D11SamplerState *states[1] = {m_texSamplerPointClamp};
	context->PSSetSamplers(0, 1, states);

	{
		ID3D11ShaderResourceView *srvs[] = {GetShadowSRV()};
		context->PSSetShaderResources(0, 1, srvs);
	}

	context->Draw(3, 0);

	{
		ID3D11ShaderResourceView *srvs[] = {nullptr};
		context->PSSetShaderResources(0, 1, srvs);
	}
}

//--------------------------------------------------------------------------------------
XMMATRIX
FurSampleAppBase::GetCameraProjection()
{
	return m_camera.GetProjMatrix();
}

//--------------------------------------------------------------------------------------
XMMATRIX
FurSampleAppBase::GetCameraViewMatrix()
{
	return m_camera.GetViewMatrix();
}

//--------------------------------------------------------------------------------------
HRESULT
FurSampleAppBase::OnFrameMove(double time, float elapsedTime, void *)
{
	m_camera.FrameMove(elapsedTime);

	ID3D11Device *pd3dDevice = DXUTGetD3D11Device();
	ID3D11DeviceContext *pd3dContext = DXUTGetD3D11DeviceContext();

	if (!pd3dDevice || !pd3dContext || (elapsedTime <= 0.0f))
		return S_FALSE;

	m_timeStepFilter.addValue(elapsedTime);
	return S_OK;
}

//--------------------------------------------------------------------------------------
HRESULT
FurSampleAppBase::OnSwapChainResized(ID3D11Device *device, IDXGISwapChain *swapChain, const DXGI_SURFACE_DESC *backBufferSurfaceDesc, void *userContext)
{
	m_width = backBufferSurfaceDesc->Width;
	m_height = backBufferSurfaceDesc->Height;

	// Setup the camera's projection parameters
	float aspectRatio = backBufferSurfaceDesc->Width / float(backBufferSurfaceDesc->Height);
	m_camera.SetProjParams(XM_PI / 3, aspectRatio, 0.1f, 10000.0f);
	m_camera.SetButtonMasks(MOUSE_LEFT_BUTTON);
	SetViewport();
	return S_OK;
}

//--------------------------------------------------------------------------------------
LRESULT CALLBACK FurSampleAppBase::MsgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam, bool *noFurtherProcessingInOut, void *userContext)
{
	m_camera.HandleMessages(wnd, msg, wparam, lparam);
	return 0;
}

//--------------------------------------------------------------------------------------
bool CALLBACK FurSampleAppBase::ModifyDeviceSettings(DXUTDeviceSettings *deviceSettings, void *userContext)
{
#ifdef _DEBUG
	deviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	{
		deviceSettings->d3d11.SyncInterval = 0;		   // Turn off vsync
		deviceSettings->d3d11.sd.SampleDesc.Count = 8; // set MSAA samples to 8
		deviceSettings->d3d11.sd.SampleDesc.Quality = 0;
	}
	return true;
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int FurSampleAppBase::WinMain(const WCHAR *title, int width, int height)
{
	m_width = width;
	m_height = height;

	SetViewport();

	DXUTSetCallbackDeviceChanging(FurSampleAppBase::ModifyDeviceSettings);
	DXUTSetCallbackMsgProc(FurSampleAppBase::MsgProc);
	DXUTSetCallbackD3D11SwapChainResized(FurSampleAppBase::OnSwapChainResized);

	DXUTSetIsInGammaCorrectMode(false);

	DXUTInit(true, true, NULL);		   // Parse the command line, show msgboxes on error, no extra command line params
	DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen
	DXUTCreateWindow(title, 0, 0, 0, 0, 0);

	// Easiest way to do would be...
	// DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, m_Width, m_Height);
	// But doesn't work as DXUTCreateDevice doesn't honor disabling gamma (ie non SRGB video mode)
	// thus the longer  DXUTCreateDeviceFromSettings
	{
		DXUTDeviceSettings deviceSettings;
		DXUTApplyDefaultDeviceSettings(&deviceSettings);
		deviceSettings.MinimumFeatureLevel = D3D_FEATURE_LEVEL_11_0;
		deviceSettings.d3d11.sd.BufferDesc.Width = m_width;
		deviceSettings.d3d11.sd.BufferDesc.Height = m_height;
		deviceSettings.d3d11.sd.Windowed = true;

		deviceSettings.d3d11.sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		DXUTCreateDeviceFromSettings(&deviceSettings);
	}
	DXUTMainLoop(); // Enter into the DXUT render loop

	return DXUTGetExitCode();
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!! FurTimeStepFilter !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
//--------------------------------------------------------------------------------------
void FurTimeStepFilter::addValue(float v)
{
	const float minDelta = 1e-4f;
	if (v > 0.0f)
	{
		// If there is no value set, just set it
		if (m_value < 0.0f)
		{
			m_value = v;
			return;
		}
		// Work out the maximum delta, from the current value
		float maxDelta = m_value * m_percentageChange;
		// Disable delta limiting if values get very small
		maxDelta = (maxDelta < minDelta) ? minDelta : maxDelta;

		float delta = v - m_value;
		delta = (delta > maxDelta) ? maxDelta : delta;
		delta = (delta < -maxDelta) ? -maxDelta : delta;

		// Alter the value
		m_value += delta;
	}
}