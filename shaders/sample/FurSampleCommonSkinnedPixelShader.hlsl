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
// Vertex shader for skinned mesh rendering
//////////////////////////////////////////////////////////////////////////////

cbuffer cb0 : register(b0)
{
	row_major float4x4 worldMatrix;
	row_major float4x4 viewMatrix;
	row_major float4x4 projectionMatrix;
}

#define MAX_SKINNING_MATRICES 1024

cbuffer cb1 : register(b1)
{
	row_major float4x4 skinBoneMatrices[MAX_SKINNING_MATRICES];
};

struct VertexInputType
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 binormal : BINORMAL;
	float4 boneWeights : BLENDWEIGHT;
	uint4 boneIndices : BLENDINDICES;
	float4 texcoord0 : TEXCOORD0;
	float4 texcoord1 : TEXCOORD1;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 binormal : BINORMAL;
	float4 texcoord0 : TEXCOORD0;
	float4 texcoord1 : TEXCOORD1;
};

// Apply vertex skinning
void VertexSkinning(float4 boneWeights, uint4 boneIndices, inout float4 position, inout float3 normal)
{
	row_major float4x4 skinTM = skinBoneMatrices[boneIndices.x] * boneWeights.x +
								skinBoneMatrices[boneIndices.y] * boneWeights.y +
								skinBoneMatrices[boneIndices.z] * boneWeights.z +
								skinBoneMatrices[boneIndices.w] * boneWeights.w;
	position = mul(position, skinTM);
	// Assumming that it is uniform scaling
	normal = mul(normal, (float3x3)skinTM);
}

PixelInputType vs_main(VertexInputType input)
{
	PixelInputType output;

	float4 position = input.position;
	float3 normal = input.normal.xyz;

	// Transform vertex position and normal with bone influences
	VertexSkinning(input.boneWeights, input.boneIndices, position, normal);

	// Apply Model/View/Projection matrix
	position = mul(position, worldMatrix);
	position = mul(position, viewMatrix);
	position = mul(position, projectionMatrix);

	// Get view space normal
	normal = normalize(mul(normal, (float3x3)worldMatrix));
	normal = normalize(mul(normal, (float3x3)viewMatrix));

	// Output
	output.position = position;
	output.normal.xyz = normal.xyz;
	output.tangent.xyz = input.tangent.xyz;
	output.binormal.xyz = input.binormal.xyz;
	output.texcoord0.xy = input.texcoord0.xy;
	output.texcoord1.xy = input.texcoord1.xy;

	return output;
}
