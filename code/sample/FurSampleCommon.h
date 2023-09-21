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

#include <DXUT.h>

//--------------------------------------------------------------------------------------
// Given a filename, find path for media file location traversing up in the parent directories
//--------------------------------------------------------------------------------------
HRESULT FurSample_GetSampleMediaFilePath(const char *file, char *filePath);

//--------------------------------------------------------------------------------------
// Given texture file name, create a texture and its shader resource view (SRV)
//--------------------------------------------------------------------------------------
HRESULT FurSample_CreateTextureSRV(ID3D11Device *device, const char *textureFilePath, ID3D11ShaderResourceView **srvOut);

//--------------------------------------------------------------------------------------
// Create custom hair shader from file
//--------------------------------------------------------------------------------------
HRESULT FurSample_CreatePixelShader(ID3D11Device *device, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11PixelShader **shaderOut);

//--------------------------------------------------------------------------------------
// Create custom hair shader from file
//--------------------------------------------------------------------------------------
HRESULT FurSample_CreateVertexShader(ID3D11Device *device, const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11VertexShader **shaderOut);

//--------------------------------------------------------------------------------------
// Create constant buffer for use in hair shader
//--------------------------------------------------------------------------------------
HRESULT FurSample_CreateConstantBuffer(ID3D11Device *device, UINT byteWidth, ID3D11Buffer **bufferOut);

//---------------------------------------------------------------------------------------------
// Create example texture sampler for hair shader
//---------------------------------------------------------------------------------------------
HRESULT FurSample_CreateTextureSampler(ID3D11Device *device, ID3D11SamplerState **texSamplerOut);

//---------------------------------------------------------------------------------------------
// Create example texture sampler for hair shadow map sampling
//---------------------------------------------------------------------------------------------
HRESULT FurSample_CreateTextureSamplerPointClamp(ID3D11Device *device, ID3D11SamplerState **texSamplerOut);