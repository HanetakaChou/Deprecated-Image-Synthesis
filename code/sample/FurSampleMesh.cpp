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

#include "FurSampleMesh.h"
#include "FurSampleVector.h"

using namespace DirectX;

HRESULT FurSampleSkinnedMeshes::Init(ID3D11Device *device)
{
	HRESULT hr;
	{
		// Create skinned mesh vertex shader
		ID3DBlob *blob = NULL;
		hr = FurSample_CreateVertexShader(device, "samples\\FurSampleCommon\\HairWorksSampleSkinnedVertexShader.hlsl", &m_skinnedVS, &blob);
		if (FAILED(hr))
			return hr;

		// Create input layout for skinned mesh
		const D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"BINORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 80, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 96, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 112, D3D11_INPUT_PER_VERTEX_DATA, 0},
			};
		hr = device->CreateInputLayout(layout, ARRAYSIZE(layout), blob->GetBufferPointer(), blob->GetBufferSize(), &m_skinnedInputLayout);
		if (FAILED(hr))
			return hr;
	}

	hr = FurSample_CreatePixelShader(device, "samples\\FurSampleCommon\\HairWorksSampleSkinnedPixelShader.hlsl", &m_skinnedPS);
	if (FAILED(hr))
		return hr;

	// Create constant buffer for skinning matrices
	{
		D3D11_BUFFER_DESC desc;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = sizeof(MySkinningMatrices);
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;
		desc.Usage = D3D11_USAGE_DYNAMIC;

		hr = device->CreateBuffer(&desc, NULL, &m_skinningMatricesCB);
		if (FAILED(hr))
			return hr;
	}

	// Create constant buffer for MVP matrices
	{
		D3D11_BUFFER_DESC desc;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = sizeof(MyModelViewProjMatrix);
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;
		desc.Usage = D3D11_USAGE_DYNAMIC;

		hr = device->CreateBuffer(&desc, NULL, &m_modelViewProjectionCB);
		if (FAILED(hr))
			return hr;
	}

	{
		D3D11_DEPTH_STENCIL_DESC desc;
		desc.DepthEnable = true;
		desc.DepthFunc = D3D11_COMPARISON_LESS;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.StencilEnable = false;
		desc.StencilReadMask = 0xff;
		desc.StencilWriteMask = 0xff;

		hr = device->CreateDepthStencilState(&desc, &m_depthStencilState);
		if (FAILED(hr))
			return hr;
	}

	// rasterizer state

	{
		D3D11_RASTERIZER_DESC desc;
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.AntialiasedLineEnable = false;
		desc.MultisampleEnable = true;
		desc.FrontCounterClockwise = true;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0;
		desc.SlopeScaledDepthBias = 0;
		desc.DepthClipEnable = true;
		desc.ScissorEnable = 0;

		hr = device->CreateRasterizerState(&desc, &m_rasterizerState);
		if (FAILED(hr))
			return hr;
	};

	return S_OK;
}

void FurSampleSkinnedMeshes::Release()
{
	SAFE_RELEASE(m_skinnedInputLayout);
	SAFE_RELEASE(m_skinnedVS);
	SAFE_RELEASE(m_skinnedPS);
	SAFE_RELEASE(m_skinningMatricesCB);
	SAFE_RELEASE(m_modelViewProjectionCB);

	SAFE_RELEASE(m_depthStencilState);
	SAFE_RELEASE(m_rasterizerState);

	for (SkinnedMeshVector::Iterator it = m_meshes.begin(); it != m_meshes.end(); ++it)
	{
		delete *it;
	}
}

HRESULT FurSampleSkinnedMeshes::LoadSkinnedMesh(ID3D11Device *device, const XMFLOAT3 *positions, int numPositions, const unsigned int *srcIndices, int numIndices, const XMVECTOR *boneIndices, const XMVECTOR *boneWeights)
{
	HRESULT hr;
	FurSample_Vector<MySkinnedVertexFormat> vertices;

	// Create on heap so a copy isn't needed to a vector
	MySkinnedMesh *mesh = new MySkinnedMesh;
	// Add to the array, such that it is freed if this goes out of scope
	m_meshes.pushBack(mesh);

	// Build vertices
	{
		vertices.setSize(numPositions);
		MySkinnedVertexFormat *dst = vertices.data();
		memset(dst, 0, sizeof(MySkinnedVertexFormat) * numPositions);

		for (int vertIdx = 0; vertIdx < numPositions; ++vertIdx)
		{
			MySkinnedVertexFormat &vert = dst[vertIdx];
			// Position
			vert.m_position[0] = positions[vertIdx].x;
			vert.m_position[1] = positions[vertIdx].y;
			vert.m_position[2] = positions[vertIdx].z;
			vert.m_position[3] = 1.0f;
			// Bone indices
			if (boneIndices)
			{
				XMStoreInt(vert.m_boneIndices, boneIndices[vertIdx]);
			}
			// Bone weights
			if (boneWeights)
			{
				XMStoreFloat(vert.m_boneWeights, boneWeights[vertIdx]);
			}
			// Set 1.0 weight for non-skinned meshes to use the same skinned vertex shader.
			if ((vert.m_boneWeights[0] == 0) &&
				(vert.m_boneWeights[1] == 0) &&
				(vert.m_boneWeights[2] == 0) &&
				(vert.m_boneWeights[3] == 0))
			{
				vert.m_boneWeights[0] = 1.0f;
			}
		}
	}
	// Build indices
	mesh->m_numIndicesPerMaterial.pushBack(numIndices);

	// Create vertex buffer with init data
	{
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.ByteWidth = sizeof(MySkinnedVertexFormat) * numPositions;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA subResourceDesc;
		subResourceDesc.pSysMem = vertices.data();
		subResourceDesc.SysMemPitch = 0;
		subResourceDesc.SysMemSlicePitch = 0;

		hr = device->CreateBuffer(&bufferDesc, &subResourceDesc, &mesh->m_vertexBuffer);
		if (FAILED(hr))
			return hr;
	}

	// Create index buffer with init data
	{
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.ByteWidth = UINT(numIndices * sizeof(UINT));
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;

		D3D11_SUBRESOURCE_DATA subResourceDesc;
		// init_data_desc.pSysMem = (const void*)&dstIndices[0];
		subResourceDesc.pSysMem = (const void *)srcIndices;
		subResourceDesc.SysMemPitch = 0;
		subResourceDesc.SysMemSlicePitch = 0;

		hr = device->CreateBuffer(&bufferDesc, &subResourceDesc, &mesh->m_indexBuffer);
		if (FAILED(hr))
			return hr;
	}

	// Initialize skinning matrices
	int numMaxBones = sizeof(m_skinning_matrices.m_boneTransforms) / sizeof(m_skinning_matrices.m_boneTransforms[0]);
	for (int idx = 0; idx < numMaxBones; ++idx)
	{
		m_skinning_matrices.m_boneTransforms[idx] = XMMatrixIdentity();
	}
	return S_OK;
}

HRESULT FurSampleSkinnedMeshes::RenderMeshes(ID3D11Device *device, ID3D11DeviceContext *context, const float *viewMatrix, const float *projectionMatrix)
{
	HRESULT hr;

	context->OMSetDepthStencilState(m_depthStencilState, 0);
	context->RSSetState(m_rasterizerState);

	for (SkinnedMeshVector::Iterator it = m_meshes.begin(); it != m_meshes.end(); ++it)
	{
		MySkinnedMesh *mesh = *it;

		// Set vertex buffer
		UINT strides[] = {sizeof(MySkinnedVertexFormat)};
		UINT offsets[] = {0};
		context->IASetVertexBuffers(0, 1, &mesh->m_vertexBuffer, strides, offsets);

		// Set index buffer
		context->IASetIndexBuffer(mesh->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Update Model/View/Projection matrices
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource;
			hr = context->Map(m_modelViewProjectionCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
			if (FAILED(hr))
				return hr;

			MyModelViewProjMatrix *mvp = reinterpret_cast<MyModelViewProjMatrix *>(mapped_resource.pData);

			mvp->m_worldMatrix = XMMatrixIdentity();

			memcpy(&mvp->m_projectionMatrix, projectionMatrix, sizeof(XMMATRIX));
			memcpy(&mvp->m_viewMatrix, viewMatrix, sizeof(XMMATRIX));

			context->Unmap(m_modelViewProjectionCB, 0);
		}

		// Update skinning matrices from the animation bones
		MySkinningMatrices *skinningMatrices = nullptr;
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource;
			hr = context->Map(m_skinningMatricesCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
			if (FAILED(hr))
				return hr;

			skinningMatrices = reinterpret_cast<MySkinningMatrices *>(mapped_resource.pData);
			memcpy(skinningMatrices->m_boneTransforms, m_skinning_matrices.m_boneTransforms, sizeof(m_skinning_matrices.m_boneTransforms));
			context->Unmap(m_skinningMatricesCB, 0);
		}

		// Set constant buffers
		context->VSSetConstantBuffers(0, 1, &m_modelViewProjectionCB);
		context->VSSetConstantBuffers(1, 1, &m_skinningMatricesCB);
		context->PSSetConstantBuffers(0, 1, &m_modelViewProjectionCB);

		// Draw all meshes
		int offset = 0;
		for (int matIdx = 0; matIdx < (int)mesh->m_numIndicesPerMaterial.getSize(); matIdx++)
		{
			int numIndices = mesh->m_numIndicesPerMaterial[matIdx];

			// Draw for each material
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context->DrawIndexed(numIndices, offset, 0);
			offset += numIndices;
		}
	}

	return S_OK;
}

void FurSampleSkinnedMeshes::UpdateMeshes(unsigned int numSkinBones, const XMMATRIX *skinningBoneMatrices)
{
	for (SkinnedMeshVector::SizeType meshIdx = 0; meshIdx < m_meshes.getSize(); ++meshIdx)
	{
		if (numSkinBones > 0)
		{
			memcpy(&m_skinning_matrices.m_boneTransforms[0], skinningBoneMatrices, sizeof(skinningBoneMatrices[0]) * numSkinBones);
		}
	}
}
