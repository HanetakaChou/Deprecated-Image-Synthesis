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
//////////////////////////////////////////////////////////////////////////////
// SAMPLE SHADER FOR FurSampleShadow.cpp
//////////////////////////////////////////////////////////////////////////////

// common hairworks shader header must be included
#include "GFSDK_HairWorks_ShaderCommon.h"

//////////////////////////////////////////////////////////////////////////////
// buffers and textures
//////////////////////////////////////////////////////////////////////////////
// declare standard resources to retrieve shader attributes (index, tangent, normal)
GFSDK_HAIR_DECLARE_SHADER_RESOURCES(t0, t1, t2);

Texture2D g_rootHairColorTexture : register(t3); // texture map for hair root colors
Texture2D g_tipHairColorTexture : register(t4);	 // texture map for hair tip colors
Texture2D g_shadowTexture : register(t5);		 // shadow map

//////////////////////////////////////////////////////////////////////////////
// constant buffer for render parameters.
// Here, we use our own constant buffer definition where HairWorks portion (g_hairConstantBuffer)
// is contained within our buffer.
//////////////////////////////////////////////////////////////////////////////
cbuffer cbPerFrame : register(b0)
{
	// user side definition of light projection matrices added for shadow map sampling
	row_major float4x4 g_lightView;
	row_major float4x4 g_lightWorldToTex;

	// .... Add other cbuffer variables to get your own data ....

	// hairworks portion of constant buffer data
	GFSDK_Hair_ConstantBuffer g_hairConstantBuffer;
}

//////////////////////////////////////////////////////////////////////////////
// sampler states
//////////////////////////////////////////////////////////////////////////////
SamplerState texSampler : register(s0);	   // texture sampler used to sample color from hair textures in this sample
SamplerState shadowSampler : register(s1); // texture sample used to sample depth from shadow texture in this sample

//////////////////////////////////////////////////////////////////////////////////////////////
// Example shadow computation code.
// If you have equivalent code in your engine, we can use it instead
//////////////////////////////////////////////////////////////////////////////////////////////
inline float GetShadowFactor(GFSDK_Hair_ConstantBuffer hairConstantBuffer, GFSDK_Hair_Material mat, GFSDK_Hair_ShaderAttributes attr)
{
	// texcoord of sample point in shadow texture
	float2 shadowTexcoords = mul(float4(attr.P, 1), g_lightWorldToTex).xy;

	// depth of this point in light's view space
	float viewSpaceDepth = mul(float4(attr.P, 1), g_lightView).z;

	// apply PCF filtering of absorption depth from linear depth texture
	// use gain = 1.0f for right handed light camera.
	float gain = -1.0f; // left handed light camera.

	float filteredDepth = GFSDK_Hair_ShadowFilterDepth(
		g_shadowTexture, shadowSampler, shadowTexcoords, viewSpaceDepth, gain);

	// convert absorption depth into lit factor
	float lit = GFSDK_Hair_ShadowLitFactor(hairConstantBuffer, mat, filteredDepth);

	return lit;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Pixel shader for hair rendering
//////////////////////////////////////////////////////////////////////////////////////////////
[earlydepthstencil] float4 ps_main(GFSDK_Hair_PixelShaderInput input) : SV_Target
{
	// get all the attibutes needed for hair shading with this function
	GFSDK_Hair_ShaderAttributes attr = GFSDK_Hair_GetShaderAttributes(input, g_hairConstantBuffer);

	// set up hair material.
	GFSDK_Hair_Material mat = g_hairConstantBuffer.defaultMaterial;

	// light direction is -z of light view matrix (left handed), pointing toward the light away from hair
	float3 Ldir = -1.0f * float3(g_lightView._13, g_lightView._23, g_lightView._33);

	// compute shadow factor
	float litFactor = GetShadowFactor(g_hairConstantBuffer, mat, attr);

	float3 Lcolor = litFactor * float3(1, 1, 1);

	// sample hair color from textures
	float3 hairColor = GFSDK_Hair_SampleHairColorTex(g_hairConstantBuffer, mat, texSampler, g_rootHairColorTexture, g_tipHairColorTexture, attr.texcoords);

	// shade hair fragment
	float3 shadedColor = GFSDK_Hair_ComputeHairShading(Lcolor, Ldir, attr, mat, hairColor.rgb);

	return float4(shadedColor, 1.0f);
}
