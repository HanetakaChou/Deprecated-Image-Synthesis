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

/************************************************************************************************************
	SUMMARY:
	This sample creates a simple hair asset made of two triangles (a quad), and create a hair instance out of the asset.
	A very simple hair asset made of a quad with 2 triangles and 4 guide hairs are created.
	Once hair assets are created, we use HairWorks to render hairs on the faces of the growth mesh.

	Each interpolated hair is drawn in white, and the guide hairs are visualized in red.
	The growth mesh is drawn as yellow triangles, using default visualization features of HairWorks.

	HIGHLIGHTS:
	This code sample shows
	- How to initialize and cleanup the HairWorks runtime
	- How to create a hair asset and modify the parameter (instance descriptor)
	- How to pass camera information to HairWorks runtime.
	- How to draw hairs with minimal rendering.
	- How to draw visualization for guide hairs and the growth mesh.

	NOTE:
	For simplicity, we provide a wrapper class FurSampleAppBase to handle general DX framework management.
	Use this as a reference to see how different components of the HairWorks API can be integrated to a program that
	has initialization (OnD3D11CreateDevice), animation/simulation loop (OnFrameMove), rendering Loop (OnFrameRender), and
	shutdown (OnD3D11DestroyDevice) processes.
	Actual integration codes may vary depending on how your application is structured.

*****************************************************************************************************************/

#include "../GFSDK_HairWorks.h" // HairWorks main header file

#include "FurSampleAppBase.h"		  // application wrapper to hide non hair-related codes
#include "FurSampleCommon.h"		  // general DX utility functions shared among samples
#include "FurSampleHairWorksHelper.h" // convenience functions related to HairWorks

#include "../../dxbc/FurSampleSimpleRenderingPixelShader_bytecode.inl"

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global variables (used only in this file)
//--------------------------------------------------------------------------------------
namespace
{
	GFSDK_HairSDK *g_hairSDK = NULL;								   // HairWorks SDK runtime
	GFSDK_HairAssetDescriptor g_hairAssetDescriptor;				   // hair asset descriptor
	GFSDK_HairAssetID g_hairAssetID = GFSDK_HairAssetID_NULL;		   // hair asset ID
	GFSDK_HairInstanceDescriptor g_hairInstanceDescriptor;			   // hair instance descriptor
	GFSDK_HairInstanceID g_hairInstanceID = GFSDK_HairInstanceID_NULL; // hair instance ID

	ID3D11PixelShader *g_customHairWorksShader = NULL; // custom pixel shader

#ifndef NDEBUG
	ID3DUserDefinedAnnotation* g_d3dUserDefinedAnnotation = NULL;
#endif
}

//--------------------------------------------------------------------------------------
// Initialize an example hair geometry, etc.
//--------------------------------------------------------------------------------------
void InitHairAssetDescriptor(GFSDK_HairAssetDescriptor &assetDesc)
{
	// We create a quad with two triangles and 4 verts.
	// A guide hair is created on each of the vertex, and HairWorks generates interpolated hairs
	// from the triangles.

	static gfsdk_float3 s_vertices[] = {
		// Vertices of 4 guide hairs.  Each guide hair has 5 cvs
		{-0.5, -0.5, 0},
		{-0.5, -0.499999553, 0.0449976847},
		{-0.5, -0.499999106, 0.0899953693},
		{-0.5, -0.499998659, 0.134993076},
		{-0.5, -0.499998212, 0.179990783},
		{0.5, -0.5, 0},
		{0.5, -0.499999553, 0.0449976847},
		{0.5, -0.499999106, 0.0899953693},
		{0.5, -0.499998659, 0.134993076},
		{0.5, -0.499998212, 0.179990783},
		{-0.5, 0.5, 0},
		{-0.5, 0.500000417, 0.0449976847},
		{-0.5, 0.500000894, 0.0899953693},
		{-0.5, 0.500001371, 0.134993076},
		{-0.5, 0.500001788, 0.179990783},
		{0.5, 0.5, 0},
		{0.5, 0.500000417, 0.0449976847},
		{0.5, 0.500000894, 0.0899953693},
		{0.5, 0.500001371, 0.134993076},
		{0.5, 0.500001788, 0.179990783}};

	static unsigned int s_endIndices[4] = {4, 9, 14, 19};											 // index of last cv for each guide hair from above s_vertices vertex table.
	static unsigned int s_faceIndices[6] = {2, 0, 3, 1, 3, 0};										 // we have 4 guide hairs, and the first (root) vertex of each guide hair defines vertex for the growth mesh face.
	static gfsdk_float2 s_faceUVs[6] = {{0, 1}, {0, 0}, {1, 1}, {1, 0}, {1, 1}, {0, 0}};			 // UVs used for textures etc.  Defined per triangle vertex of the growth mesh.
	static gfsdk_float4 s_boneIndices[4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}; // bone index for each guide hair (we have only 1 bone in this example)
	static gfsdk_float4 s_boneWeights[4] = {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}; // bone weights
	static gfsdk_float4x4 s_bindPoses[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};			 // bind pose matrix for each bone.
	static int boneParents = -1;																	 // index to parent bone of each bone.

	assetDesc.m_NumGuideHairs = 4;
	assetDesc.m_NumVertices = 20;
	assetDesc.m_pVertices = s_vertices;
	assetDesc.m_pEndIndices = s_endIndices;
	assetDesc.m_NumFaces = 2;
	assetDesc.m_pFaceIndices = s_faceIndices;
	assetDesc.m_pFaceUVs = s_faceUVs;
	assetDesc.m_NumBones = 1;
	assetDesc.m_pBoneIndices = s_boneIndices;
	assetDesc.m_pBoneWeights = s_boneWeights;
	assetDesc.m_pBoneNames = "Bone001";
	assetDesc.m_pBindPoses = s_bindPoses;
	assetDesc.m_pBoneParents = &boneParents;
}

//--------------------------------------------------------------------------------------
// Initialize instance descriptor (control parameters) used in this sample
//--------------------------------------------------------------------------------------
void InitHairInstanceDescriptor(GFSDK_HairInstanceDescriptor &instanceDesc)
{
	// change some of parameters for hair instance descriptor
	// for full list of available parameters, refer to GFSDK_HairWorks.h

	instanceDesc.m_width = 0.2;	  // thickness/width of each hair
	instanceDesc.m_density = 1.0; // density per triangle.  A value of 1.0 corresponds to 64 hairs per triangle.
	instanceDesc.m_lengthNoise = 0.0f;

	instanceDesc.m_visualizeGuideHairs = true;
	instanceDesc.m_visualizeGrowthMesh = true;

	instanceDesc.m_simulate = false; // turn off simulation
}

//--------------------------------------------------------------------------------------
// This function contains all the steps necessary to prepare hair asset and resources
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device *device, const DXGI_SURFACE_DESC *backBufferSurfaceDesc, void *userContext)
{
	HRESULT hr;
	// set up default DXUT camera
	FurSampleAppBase::InitDefaultCamera(XMVectorSet(0, -2.0f, 2.0f, 0));

	// load HairWorks dll
	g_hairSDK = FurSample_LoadHairWorksDLL();
	if (!g_hairSDK)
		return E_FAIL;

	// Initialize hair asset descriptor and hair instance descriptor
	InitHairAssetDescriptor(g_hairAssetDescriptor);
	InitHairInstanceDescriptor(g_hairInstanceDescriptor);

	// Create the hair asset
	g_hairSDK->CreateHairAsset(g_hairAssetDescriptor, &g_hairAssetID);

	// create custom hair shader
	hr = FurSample_CreatePixelShader(device, FurSampleSimpleRenderingPixelShader_bytecode, sizeof(FurSampleSimpleRenderingPixelShader_bytecode), &g_customHairWorksShader);
	if (FAILED(hr))
		return hr;

	// Initialize DirectX settings for HairWorks runtime
	g_hairSDK->InitRenderResources(device);

	// Create hair instance.  A valid d3d device must be set before an instance can be created.
	g_hairSDK->CreateHairInstance(g_hairAssetID, &g_hairInstanceID);

	// Update instance descriptor
	g_hairSDK->UpdateInstanceDescriptor(g_hairInstanceID, g_hairInstanceDescriptor);

#ifndef NDEBUG
	DXUTGetD3D11DeviceContext()->QueryInterface(IID_PPV_ARGS(&g_d3dUserDefinedAnnotation));
#endif

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void *userContext)
{
#ifndef NDEBUG
	SAFE_RELEASE(g_d3dUserDefinedAnnotation);
#endif

	SAFE_RELEASE(g_customHairWorksShader);

	g_hairSDK->FreeHairInstance(g_hairInstanceID);
	g_hairSDK->FreeHairAsset(g_hairAssetID);
	g_hairSDK->FreeRenderResources();
	g_hairSDK->Release();
}

//--------------------------------------------------------------------------------------
// Called at every frame. This is the main function for hair rendering
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender(ID3D11Device *device, ID3D11DeviceContext *context, double time, float elapsedTime, void *userContext)
{
	// prepare render target to render hair into
	FurSampleAppBase::PrepareRenderTarget(context);

	// Set render context for HairWorks
	g_hairSDK->SetCurrentContext(context);

	// Set view matrix and projection matrix
	XMMATRIX projection = FurSampleAppBase::GetCameraProjection();
	XMMATRIX view = FurSampleAppBase::GetCameraViewMatrix();
	g_hairSDK->SetViewProjection((const gfsdk_float4x4 *)&view, (const gfsdk_float4x4 *)&projection, GFSDK_HAIR_LEFT_HANDED);

	// set your hair pixel shader before rendering hairs
	context->PSSetShader(g_customHairWorksShader, nullptr, 0);

	// Render the hair instance
	{
#ifndef NDEBUG
		g_d3dUserDefinedAnnotation->BeginEvent(L"Render Hairs");
#endif
		g_hairSDK->RenderHairs(g_hairInstanceID);

#ifndef NDEBUG
		g_d3dUserDefinedAnnotation->EndEvent();
#endif
	}

	// Render visualization the hair instance
	{
#ifndef NDEBUG
		g_d3dUserDefinedAnnotation->BeginEvent(L"Render Visualization");
#endif

		g_hairSDK->RenderVisualization(g_hairInstanceID);

#ifndef NDEBUG
		g_d3dUserDefinedAnnotation->EndEvent();
#endif
	}
}

//--------------------------------------------------------------------------------------
// Called once per frame when animation is on.
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove(double time, float elapsedTime, void *userContext)
{
	if (S_OK != FurSampleAppBase::OnFrameMove(time, elapsedTime, userContext))
		return;

	ID3D11DeviceContext *context = FurSampleAppBase::GetDeviceContext();
	// Set simulation context for HairWorks
	g_hairSDK->SetCurrentContext(context);
	// run simulation for all hairs
	g_hairSDK->StepSimulation();
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
	// Set DXUT callbacks
	DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
	DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);
	DXUTSetCallbackFrameMove(OnFrameMove);
	DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);

	return FurSampleAppBase::WinMain(L"FurSampleSimple");
}
