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
#pragma once

#include "DXUT.h"
#include "DXUTcamera.h" // DXUT camera utility class

#include "DirectXMath.h"

#include "FurSampleShadowMap.h"

/// A very simple filter, used for time step smoothing
class FurTimeStepFilter
{
public:
	/// Add a value to the filter
	void addValue(float v);
	/// Get the filtered value
	float getFilteredValue() const { return m_value < 0.0f ? 0.01f : m_value; }

	FurTimeStepFilter() : m_percentageChange(0.15f), m_value(-1.0f) {}

protected:
	float m_value;			  ///< -ve means value not set, else holds current filtered value
	float m_percentageChange; ///< The maximum percentage delta from current
};

// Base class for all fur samples to encapsulate codes that are not directly related to HairWorks
// We assume that there are equivalent functionalities in host engine applications.
// Implementation of this class is for reference purpose only.
class FurSampleAppBase
{
public:
	// Window Dimensions
	static int m_width;
	static int m_height;

	// DXUT camera
	static CModelViewerCamera m_camera;
	static D3D11_VIEWPORT m_viewport;

	// Light matrices
	static DirectX::XMMATRIX m_lightView;		// view matrix for light camera
	static DirectX::XMMATRIX m_lightProjection; // projection matrix for light camera
	static DirectX::XMMATRIX m_lightWorldToTex; // transfroms world point into shadow texture coordinates

	// Texture Samplers
	static ID3D11SamplerState *m_texSamplerLinear;
	static ID3D11SamplerState *m_texSamplerPointClamp;

	// Utility shaders
	static ID3D11VertexShader *m_visualizeShadowVS;
	static ID3D11PixelShader *m_visualizeShadowPS;

	// shadow map resources
	static FurSampleShadowMap m_shadowMap;

	static FurTimeStepFilter m_timeStepFilter;

public:
	static HRESULT OnCreateDevice(ID3D11Device *pd3dDevice);
	static void OnDestroyDevice();

	static ID3D11DeviceContext *GetDeviceContext();
	static ID3D11Device *GetDevice();

	static void InitDefaultCamera(DirectX::FXMVECTOR eyePos, DirectX::FXMVECTOR at = DirectX::XMVectorZero());
	static void InitDefaultLight(DirectX::FXMVECTOR eyePos, DirectX::FXMVECTOR at = DirectX::XMVectorZero(), DirectX::FXMVECTOR up = DirectX::XMVectorSet(0, 1, 0, 0));
	static void SetupLightMatrices(DirectX::FXMVECTOR eye, DirectX::FXMVECTOR lookAt, DirectX::FXMVECTOR up, float sizeX, float sizeY, float zNear, float zFar);
	static void SetViewport();

	static void PrepareRenderTarget(ID3D11DeviceContext *context);

	static void PrepareShadowTarget(ID3D11DeviceContext *context);
	static void VisualizeShadowMap(ID3D11DeviceContext *context);
	static ID3D11ShaderResourceView *GetShadowSRV();

	static ID3D11SamplerState *GetSamplerLinear() { return m_texSamplerLinear; }
	static ID3D11SamplerState *GetSamplerPointClamp() { return m_texSamplerPointClamp; }

	static DirectX::XMMATRIX GetCameraProjection();
	static DirectX::XMMATRIX GetCameraViewMatrix();

	static const DirectX::XMMATRIX &GetLightProjection() { return m_lightProjection; }
	static const DirectX::XMMATRIX &GetLightViewMatrix() { return m_lightView; }
	static const DirectX::XMMATRIX &GetLightWorldToTex() { return m_lightWorldToTex; }

	static HRESULT OnFrameMove(double fTime, float fElapsedTime, void *);
	static HRESULT CALLBACK OnSwapChainResized(ID3D11Device *pd3dDevice, IDXGISwapChain *swapChain, const DXGI_SURFACE_DESC *backBufferSurfaceDesc, void *userContext);
	static LRESULT CALLBACK MsgProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam, bool *noFurtherProcessingInOut, void *userContext);
	static bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings *deviceSettings, void *userContext);

	static int WinMain(const WCHAR *title, int width = 1024, int height = 768);

	static float GetSimulationTimeStep() { return m_timeStepFilter.getFilteredValue(); }
};
