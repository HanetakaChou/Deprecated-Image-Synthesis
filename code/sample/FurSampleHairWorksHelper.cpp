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

#include "FurSampleHairWorksHelper.h"
#include "FurSampleCommon.h"

//---------------------------------------------------------------------------------------------
// Custom log handler
//---------------------------------------------------------------------------------------------
class CustomLogHandler : public GFSDK_HAIR_LogHandler
{
public:
	~CustomLogHandler()
	{
		OutputDebugStringA("GFSDK_HAIR_SAMPLE: log handler destroyed\n");
	}

	void Log(GFSDK_HAIR_LOG_TYPES logType, const char *message, const char *file, int line)
	{
		switch (logType)
		{
		case GFSDK_HAIR_LOG_ERROR:
			OutputDebugStringA("GFSDK_HAIR: ERROR: ");
			break;
		case GFSDK_HAIR_LOG_WARNING:
			OutputDebugStringA("GFSDK_HAIR: WARN: ");
			break;
		case GFSDK_HAIR_LOG_INFO:
			OutputDebugStringA("GFSDK_HAIR: INFO: ");
			break;
		};
		OutputDebugStringA(message);
		OutputDebugStringA("\n");
	}
};

static CustomLogHandler s_logger;

//---------------------------------------------------------------------------------------------
// Load hairworks runtime
//---------------------------------------------------------------------------------------------
GFSDK_HairSDK *FurSample_LoadHairWorksDLL(void)
{
	// Load hairworks dll
#ifdef _WIN64
	const char *coreDLLPath = "GFSDK_HairWorks.win64.dll";
#else
	const char *coreDLLPath = "GFSDK_HairWorks.win32.dll";
#endif

	return GFSDK_LoadHairSDK(coreDLLPath, GFSDK_HAIRWORKS_VERSION, 0, &s_logger);
}

//---------------------------------------------------------------------------------------------
// Create texture resource and set it to hairworks runtime
//---------------------------------------------------------------------------------------------
HRESULT FurSample_SetupTextureResource(ID3D11Device *device, GFSDK_HairSDK *hairSDK, GFSDK_HairAssetID assetID, GFSDK_HairInstanceID instanceID, const char *mediaPath)
{
	HRESULT hr;
	// load texture maps for hair and create shader resource views
	// Build absolute path for textures
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];

	_splitpath(mediaPath, drive, dir, NULL, NULL);

	for (int t = 0; t < GFSDK_HAIR_NUM_TEXTURES; ++t)
	{
		char textureFileName[1024];
		char textureFilePath[1024];

		if (GFSDK_HAIR_RETURN_OK != hairSDK->GetTextureName(assetID, (GFSDK_HAIR_TEXTURE_TYPE)t, textureFileName))
			continue;

		if (strlen(textureFileName) == 0)
			continue;

		_makepath(textureFilePath, drive, dir, textureFileName, NULL);

		ID3D11ShaderResourceView *textureSRV = nullptr;

		hr = FurSample_CreateTextureSRV(device, textureFilePath, &textureSRV);
		if (FAILED(hr))
			return hr;

		GFSDK_HAIR_RETURNCODES hrc = hairSDK->SetTextureSRV(instanceID, (GFSDK_HAIR_TEXTURE_TYPE)t, textureSRV);
		SAFE_RELEASE(textureSRV);

		if (GFSDK_HAIR_RETURN_OK != hrc)
			return E_FAIL;
	}

	return S_OK;
}
