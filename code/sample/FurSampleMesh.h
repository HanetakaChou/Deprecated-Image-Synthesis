// This code contains NVIDIA Confidential Information and is disclosed
// under the Mutual Non-Disclosure Agreement.
//
// Notice
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
//
// NVIDIA Corporation assumes no responsm_indexBufferility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. No third party distrm_indexBufferution is allowed unless
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
// Any use, reproduction, disclosure or distrm_indexBufferution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohm_indexBufferited.
//
#pragma once

#include "FurSampleCommon.h"
#include "FurSampleVector.h"
#include <DirectXMath.h>

// This should match the value in HairWorksSampleSkinnedVertexShader.hlsl
#define MAX_SKINNING_MATRICES 1024

struct MySkinningMatrices
{
	DirectX::XMMATRIX m_boneTransforms[MAX_SKINNING_MATRICES];
};

struct MyModelViewProjMatrix
{
	DirectX::XMMATRIX m_worldMatrix;
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projectionMatrix;
};

struct MySkinnedVertexFormat
{
	float m_position[4];
	float m_normal[4];
	float m_tangent[4];
	float m_binormal[4];
	float m_boneWeights[4];
	unsigned int m_boneIndices[4];
	float m_texCoord0[4];
	float m_texCoord1[4];
};

class MySkinnedMesh
{
public:
	// Overload operator new/delete to make sure allocations are 16 byte aligned
	// As type contains XMMATRIX which requires 16 byte alignment
	void *operator new(size_t size) { return _aligned_malloc(size, 16); }
	void operator delete(void *data) { _aligned_free(data); }

	ID3D11Buffer *m_vertexBuffer;
	ID3D11Buffer *m_indexBuffer;

	FurSample_Vector<unsigned int> m_numIndicesPerMaterial;

	int m_numFrames;
	int m_numSkinBones;

	DirectX::XMMATRIX *m_boneAnimMatrices; // m_numSkinBones * m_numFrames
	DirectX::XMMATRIX m_localTransform;
	DirectX::XMMATRIX m_worldTransform;
	DirectX::XMMATRIX m_objectTransform;
	DirectX::XMMATRIX m_binePoseTransform;	 // Bine-pose transform in model space
	DirectX::XMMATRIX *m_skeletonTransforms; // Skin bone transforms in world space

	MySkinnedMesh()
	{
		m_vertexBuffer = nullptr;
		m_indexBuffer = nullptr;
		m_numFrames = 0;
		m_boneAnimMatrices = nullptr;
		m_numSkinBones = 0;
		m_skeletonTransforms = nullptr;
		m_localTransform = DirectX::XMMatrixIdentity();
		m_worldTransform = DirectX::XMMatrixIdentity();
		m_objectTransform = DirectX::XMMatrixIdentity();
		m_binePoseTransform = DirectX::XMMatrixIdentity();
	}

	~MySkinnedMesh()
	{
		if (m_boneAnimMatrices)
		{
			delete[] m_boneAnimMatrices;
			m_boneAnimMatrices = nullptr;
		}
		if (m_skeletonTransforms)
		{
			delete[] m_skeletonTransforms;
			m_skeletonTransforms = nullptr;
		}
		if (m_vertexBuffer)
		{
			m_vertexBuffer->Release();
			m_vertexBuffer = nullptr;
		}
		if (m_indexBuffer)
		{
			m_indexBuffer->Release();
			m_indexBuffer = nullptr;
		}
	}
};

class FurSampleSkinnedMeshes
{
public:
	// Initialize
	HRESULT Init(ID3D11Device *d3dDevice);

	// Release
	void Release();

	// Load skinned meshs from the Hair asset
	HRESULT LoadSkinnedMesh(ID3D11Device *device, const DirectX::XMFLOAT3 *positions, int numPositions, const unsigned int *indices, int numIndices, const DirectX::XMVECTOR *boneIndices, const DirectX::XMVECTOR *boneWeights);

	// Render the meshes
	HRESULT RenderMeshes(ID3D11Device *device, ID3D11DeviceContext *context, const float *viewMatrix, const float *projectionMatrix);

	// Update mesh animations
	void UpdateMeshes(unsigned int numSkinBones, const DirectX::XMMATRIX *skinningBoneMatrices);

	typedef FurSample_Vector<MySkinnedMesh *> SkinnedMeshVector;
	SkinnedMeshVector m_meshes;

	MySkinningMatrices m_skinning_matrices;

	ID3D11InputLayout *m_skinnedInputLayout;
	ID3D11VertexShader *m_skinnedVS;
	ID3D11PixelShader *m_skinnedPS;
	ID3D11Buffer *m_skinningMatricesCB;
	ID3D11Buffer *m_modelViewProjectionCB;

	ID3D11DepthStencilState *m_depthStencilState;
	ID3D11RasterizerState *m_rasterizerState;
};
