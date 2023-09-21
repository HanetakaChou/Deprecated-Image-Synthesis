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

#include "FurSampleShadowMap.h"

namespace // anonymous
{
	// used only in this file
	//////////////////////////////////////////////////////////////////////////////////////////////////
	D3D11_TEXTURE2D_DESC CreateD3D11TextureDesc(
		DXGI_FORMAT format, UINT width, UINT height,
		UINT bindFlags, UINT sampleCount = 1, D3D11_USAGE usage = D3D11_USAGE_DEFAULT, UINT cpuAccessFlags = 0,
		UINT miscFlags = 0, UINT arraySize = 1, UINT mipLevels = 1)
	{
		D3D11_TEXTURE2D_DESC desc;
		desc.Format                 = format;
		desc.Width					= width;
		desc.Height					= height;

		desc.ArraySize				= arraySize;
		desc.MiscFlags				= miscFlags;
		desc.MipLevels				= mipLevels;

		desc.SampleDesc.Count		= sampleCount;
		desc.SampleDesc.Quality		= 0;
		desc.BindFlags				= bindFlags;
		desc.Usage					= usage;
		desc.CPUAccessFlags			= cpuAccessFlags;
		return desc;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	D3D11_DEPTH_STENCIL_VIEW_DESC CreateD3D11DSVDesc(
		DXGI_FORMAT format, D3D11_DSV_DIMENSION viewDimension,  
		UINT flags = 0, UINT mipSlice = 0)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		desc.Format                    = format;
		desc.ViewDimension             = viewDimension;
		desc.Flags					   = flags;
		desc.Texture2D.MipSlice        = mipSlice;
		return desc;
	}

}

//////////////////////////////////////////////////////////////////////////////
FurSampleShadowMap::FurSampleShadowMap()
{
	m_shadowTexture = nullptr;
	m_shadowRTV = nullptr;
	m_shadowSRV = nullptr;

	m_depthTexture = nullptr;
	m_depthDSV = nullptr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT 
FurSampleShadowMap::Init(ID3D11Device* device, int resolution)
{	
	HRESULT hr;
	// set viewport
	{
		m_viewport.Width  = float(resolution);
		m_viewport.Height = float(resolution);
		m_viewport.MinDepth = 0;
		m_viewport.MaxDepth = 1;
		m_viewport.TopLeftX = 0;
		m_viewport.TopLeftY = 0;
	}

	// create shadow render target
	{
		D3D11_TEXTURE2D_DESC texDesc = CreateD3D11TextureDesc(
			DXGI_FORMAT_R32_FLOAT, UINT(resolution), UINT(resolution),
			D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
	
		hr =  device->CreateTexture2D( &texDesc, nullptr, &m_shadowTexture);
		if (FAILED(hr))
			return hr;

		hr = device->CreateShaderResourceView( m_shadowTexture, nullptr, &m_shadowSRV );
		if (FAILED(hr))
			return hr;

		hr = device->CreateRenderTargetView(m_shadowTexture, nullptr, &m_shadowRTV);
		if (FAILED(hr))
			return hr;
	}

	// create shadow depth stencil
	{
		D3D11_TEXTURE2D_DESC texDesc = CreateD3D11TextureDesc(
			DXGI_FORMAT_R32_TYPELESS, UINT(resolution), UINT(resolution),
			D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE); 

		hr = device->CreateTexture2D( &texDesc, NULL, &m_depthTexture );
		if (FAILED(hr))
			return hr;

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = CreateD3D11DSVDesc(
			DXGI_FORMAT_D32_FLOAT, D3D11_DSV_DIMENSION_TEXTURE2D);

		hr = device->CreateDepthStencilView(m_depthTexture, &dsvDesc, &m_depthDSV);
		if (FAILED(hr))
			return hr;
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT 
FurSampleShadowMap::Release()
{
	SAFE_RELEASE(m_shadowTexture);
	SAFE_RELEASE(m_shadowRTV);
	SAFE_RELEASE(m_shadowSRV);
	SAFE_RELEASE(m_depthTexture);
	SAFE_RELEASE(m_depthDSV);

	m_shadowTexture = nullptr;
	m_shadowRTV = nullptr;
	m_shadowSRV = nullptr;

	m_depthTexture = nullptr;
	m_depthDSV = nullptr;
	return S_OK;
}

