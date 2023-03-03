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
// SHADER SAMPLE FOR FurSampleShading.cpp
//////////////////////////////////////////////////////////////////////////////

// common hairworks shader header must be included
#include "GFSDK_HairWorks_ShaderCommon.h" 

//////////////////////////////////////////////////////////////////////////////
// buffers and textures
//////////////////////////////////////////////////////////////////////////////
GFSDK_HAIR_DECLARE_SHADER_RESOURCES(t0, t1, t2); // shader resources

Texture2D	g_rootHairColorTexture	: register(t3); // texture map for hair root colors
Texture2D	g_tipHairColorTexture	: register(t4); // texture map for hair tip colors

//////////////////////////////////////////////////////////////////////////////
// constant buffer for render parameters
//////////////////////////////////////////////////////////////////////////////
cbuffer cbPerFrame : register(b0)
{
	// .... Add other cbuffer variables to get your own data

	GFSDK_Hair_ConstantBuffer	g_hairConstantBuffer; // hairworks portion of constant buffer data

	// .... Add other cbuffer variables to get your own data
}

//////////////////////////////////////////////////////////////////////////////
// sampler states
//////////////////////////////////////////////////////////////////////////////
SamplerState texSampler: register(s0); // we use a texture sampler for hair textures in this sample

//////////////////////////////////////////////////////////////////////////////////////////////
// Pixel shader for hair rendering
//////////////////////////////////////////////////////////////////////////////////////////////
[earlydepthstencil] float4 ps_main(GFSDK_Hair_PixelShaderInput input) : SV_Target
{   
	// get all the attibutes needed for hair shading with this function
	GFSDK_Hair_ShaderAttributes attr = GFSDK_Hair_GetShaderAttributes(input, g_hairConstantBuffer);
  
	// set up hair material.
	GFSDK_Hair_Material mat = g_hairConstantBuffer.defaultMaterial;

	float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);

	// use visualization?
	if (GFSDK_Hair_VisualizeColor(g_hairConstantBuffer, mat, attr, color.rgb))
		return color;

	// sample hair color from textures
	float3 hairColor = GFSDK_Hair_SampleHairColorTex(g_hairConstantBuffer, mat, texSampler, g_rootHairColorTexture, g_tipHairColorTexture, attr.texcoords);

#define NUM_LIGHTS 4

	// Replace with your own light iteration codes
	const float3 lightDir[NUM_LIGHTS] = 
	{
		float3(1, -2, -5),
		float3(1,-1,1),
		float3(-1,-1,1),
		float3(-1,-1,1)
	};

	const float3 lightColor[NUM_LIGHTS] = 
	{
		float3(1,1,1),
		float3(0.2,0.2,0.2),
		float3(0.1,0.1,0.1),
		float3(0.1,0.1,0.1)
	};

	// iterate through each light and sum all the shading contribution from the light

	[unroll]
	for (int i = 0; i < NUM_LIGHTS; i++)
	{
		float3 Lcolor = lightColor[i];
		float3 Ldir = normalize(lightDir[i]); // light direction defined from shaded point toward the light

		color.rgb += GFSDK_Hair_ComputeHairShading(Lcolor, Ldir, attr, mat, hairColor.rgb);
	}

    return color;
}

