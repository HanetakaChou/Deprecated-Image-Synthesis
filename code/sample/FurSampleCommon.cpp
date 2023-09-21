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

#include "FurSampleCommon.h"
#include "FurSampleVector.h"

#include <stdlib.h>

#include <DXUT.h>
#include <SDKmisc.h>

#include <D3Dcompiler.h>

#include "stb_image.h"

//--------------------------------------------------------------------------------------
// Given a filename, find path for media file location traversing up in the parent directories
//--------------------------------------------------------------------------------------
HRESULT FurSample_GetSampleMediaFilePath(const char *file, char *filePath)
{
	strcpy(filePath, file);
	// If path not exists,
	if (GetFileAttributesA(filePath) != INVALID_FILE_ATTRIBUTES)
	{
		return S_OK;
	}
	// Retry with its parent folders
	const int maxTries = 4;
	for (int i = 0; i < maxTries; ++i)
	{
		if ((strlen(filePath) > 2) && (filePath[1] != ':'))
		{
			char parentFolder[1024];
			sprintf(parentFolder, "..\\%s", filePath);
			strcpy(filePath, parentFolder);
			if (GetFileAttributesA(filePath) != INVALID_FILE_ATTRIBUTES)
			{
				// Found path
				return S_OK;
			}
		}
	}
	// Didn't find the path
	return E_FAIL;
}

//--------------------------------------------------------------------------------------
// Create custom hair shader from file
//--------------------------------------------------------------------------------------
HRESULT FurSample_CreatePixelShader(ID3D11Device *device, const char *shaderFile, ID3D11PixelShader **shaderOut)
{
	char shaderFilePath[1024];
	// get file path for the shader file
	HRESULT hr = FurSample_GetSampleMediaFilePath(shaderFile, shaderFilePath);
	if (FAILED(hr))
		return hr;

	ID3DBlob *blob = NULL;
	WCHAR buffer[MAX_PATH];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, shaderFile, -1, buffer, MAX_PATH);
	hr = DXUTCompileFromFile(buffer, NULL, "ps_main", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &blob);

	if (FAILED(hr))
		return hr;

	// Create custom pixel shader for HairWorks rendering
	hr = device->CreatePixelShader((void *)blob->GetBufferPointer(), blob->GetBufferSize(), NULL, shaderOut);
	blob->Release();
	return hr;
}

//--------------------------------------------------------------------------------------
// Create custom hair shader from file
//--------------------------------------------------------------------------------------
HRESULT FurSample_CreateVertexShader(ID3D11Device *device, const char *shaderFile, ID3D11VertexShader **shaderOut, ID3D10Blob **blobOut)
{
	char shaderFilePath[1024];

	// get file path for the shader file
	HRESULT hr = FurSample_GetSampleMediaFilePath(shaderFile, shaderFilePath);
	if (FAILED(hr))
		return hr;

	WCHAR buffer[MAX_PATH];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, shaderFile, -1, buffer, MAX_PATH);

	ID3D10Blob *blob = nullptr;
	hr = DXUTCompileFromFile(buffer, NULL, "vs_main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &blob);

	if (FAILED(hr))
		return hr;

	// Copy blob if specified
	if (blobOut)
	{
		blob->AddRef();
		*blobOut = blob;
	}

	// Create custom pixel shader for HairWorks rendering
	hr = device->CreateVertexShader((void *)blob->GetBufferPointer(), blob->GetBufferSize(), NULL, shaderOut);
	blob->Release();
	return hr;
}

//--------------------------------------------------------------------------------------
// Create constant buffer for use in hair shader
//--------------------------------------------------------------------------------------
HRESULT FurSample_CreateConstantBuffer(ID3D11Device *device, UINT byteWidth, ID3D11Buffer **bufferOut)
{
	// create constant buffer for hair rendering pixel shader
	D3D11_BUFFER_DESC desc;
	{
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = byteWidth;
		desc.StructureByteStride = 0;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.MiscFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	return device->CreateBuffer(&desc, nullptr, bufferOut);
}

//---------------------------------------------------------------------------------------------
// Create example texture sampler for hair shader texture sampling (linear)
//---------------------------------------------------------------------------------------------
HRESULT FurSample_CreateTextureSampler(ID3D11Device *device, ID3D11SamplerState **texSamplerOut)
{
	// example texture sampler (use any texture sampler of your own choice)
	D3D11_SAMPLER_DESC samplerDesc[1] = {
		D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		0.0,
		0,
		D3D11_COMPARISON_NEVER,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		D3D11_FLOAT32_MAX,
	};
	return device->CreateSamplerState(samplerDesc, texSamplerOut);
}

//---------------------------------------------------------------------------------------------
// Create example texture sampler for hair shader shadow map sampling (point clamp)
//---------------------------------------------------------------------------------------------
HRESULT FurSample_CreateTextureSamplerPointClamp(ID3D11Device *device, ID3D11SamplerState **texSamplerOut)
{
	D3D11_SAMPLER_DESC samplerDesc[1] = {
		D3D11_FILTER_MIN_MAG_MIP_POINT,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		0.0,
		0,
		D3D11_COMPARISON_NEVER,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		D3D11_FLOAT32_MAX,
	};
	return device->CreateSamplerState(samplerDesc, texSamplerOut);
}

HRESULT FurSample_StbImageCreateTexture(ID3D11Device *device, const char *textureFilePath, ID3D11Texture2D **textureOut)
{
	int width = 0;
	int height = 0;
	int numComponents = 0;
	unsigned char *origPixels = stbi_load(textureFilePath, &width, &height, &numComponents, 4);

	if (!origPixels)
		return E_FAIL;

	// Calculate the number of mips
	int numMipMaps = 0;
	size_t totalNumPixels = 0;
	{
		// Work out the size all the way do
		int mipWidth = width;
		int mipHeight = height;
		while (mipWidth > 1 || mipHeight > 1)
		{
			numMipMaps++;

			totalNumPixels += mipWidth * mipHeight;

			mipWidth >>= 1;
			mipHeight >>= 1;
		}
	}

	FurSample_Vector<D3D11_SUBRESOURCE_DATA> subData;
	subData.setSize(numMipMaps);
	ZeroMemory(subData.data(), sizeof(D3D11_SUBRESOURCE_DATA) * numMipMaps);

	// Pixels are stored in uint32 with 4 byte components
	typedef uint32_t Pixel;
	FurSample_Vector<Pixel> mipMaps;
	// Make space for all the mip maps bar the top one (we'll just use what we have for that)
	mipMaps.setSize(totalNumPixels - (width - height));

	// Initialize the top mip map
	{
		D3D11_SUBRESOURCE_DATA &topSub = subData[0];
		topSub.pSysMem = origPixels;
		topSub.SysMemPitch = width * 4;
	}

	// current mip level width and height
	int mipWidth = width;
	int mipHeight = height;

	const Pixel *srcMip = (const Pixel *)origPixels;
	Pixel *dstMip = mipMaps.data();

	// Generate the mips from the previous mips
	for (int idx = 1; idx < numMipMaps; ++idx)
	{
		int prevMipWidth = mipWidth;

		// generate the next mip level
		mipWidth = (mipWidth <= 1) ? 1 : (mipWidth >> 1);
		mipHeight = (mipHeight <= 1) ? 1 : (mipHeight >> 1);

		D3D11_SUBRESOURCE_DATA &curSubData = subData[idx];
		curSubData.pSysMem = dstMip;
		curSubData.SysMemPitch = mipWidth * 4;

		const Pixel *srcRow = srcMip;
		Pixel *dstRow = dstMip;

		// Average the 4 pixels of the larger mip
		for (int h = 0; h < mipHeight; h++)
		{
			for (int w = 0; w < mipWidth; w++)
			{
				const Pixel *src = srcRow + w * 2;
				const Pixel p00 = src[0];
				const Pixel p10 = src[1];
				const Pixel p01 = src[prevMipWidth];
				const Pixel p11 = src[prevMipWidth + 1];

				// Blend
				// This is a fast way to average two uint32 containing 4 bytes.
				// First averages horizontally, for two lines, and then averages those two averages.
				const Pixel p0 = (((p00 ^ p10) & 0xfefefefe) >> 1) + (p00 & p10);
				const Pixel p1 = (((p01 ^ p11) & 0xfefefefe) >> 1) + (p01 & p11);
				const Pixel p = (((p0 ^ p1) & 0xfefefefe) >> 1) + (p0 & p1);

				dstRow[w] = p;
			}
			srcRow += prevMipWidth * 2;
			dstRow += mipWidth;
		}

		// Next
		srcMip = dstMip;
		dstMip += (mipWidth * mipHeight);
	}

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Height = height;
	desc.Width = width;
	desc.MipLevels = numMipMaps;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;

	HRESULT hr = device->CreateTexture2D(&desc, subData.data(), textureOut);
	// now it's OK to delete the original
	stbi_image_free(origPixels);

	return hr;
}

//--------------------------------------------------------------------------------------
// Given texture file name, create a texture and its shader resource view (SRV)
//--------------------------------------------------------------------------------------
HRESULT FurSample_CreateTextureSRV(ID3D11Device *device, const char *textureFilePath, ID3D11ShaderResourceView **textureSRVOut)
{
	HRESULT hr;

	ID3D11Texture2D *texture = NULL;
	{
		// Try to read with DXUTCreateTextureFromFile first
		WCHAR buffer[MAX_PATH];
		mbtowc(buffer, textureFilePath, sizeof(buffer) / sizeof(buffer[0]));

		ID3D11Resource *resource;
		hr = DXUTCreateTextureFromFile(device, buffer, &resource);
		if (SUCCEEDED(hr))
		{
			hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), (LPVOID *)&texture);
			resource->Release();
		}
	}
	if (!texture)
	{
		// Try with Stb -> It handles Tgas amongst other formats
		hr = FurSample_StbImageCreateTexture(device, textureFilePath, &texture);
		if (FAILED(hr))
			return hr;
	}

	// Get a resourceView
	ID3D11ShaderResourceView *textureSRV = NULL;
	{
		D3D11_TEXTURE2D_DESC texDesc;
		texture->GetDesc(&texDesc);

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
		ZeroMemory(&viewDesc, sizeof(viewDesc));

		viewDesc.Format = texDesc.Format;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MostDetailedMip = 0;
		viewDesc.Texture2D.MipLevels = texDesc.MipLevels;
		device->CreateShaderResourceView(texture, &viewDesc, &textureSRV);
	}
	texture->Release();
	*textureSRVOut = textureSRV;
	return S_OK;
}
