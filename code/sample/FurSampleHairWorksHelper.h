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

// This file contains convenience functions that can be useful to simplify some HairWorks related tasks.
#include "GFSDK_HairWorks.h"
#include "DXUT.h"

//---------------------------------------------------------------------------------------------
// Load hairworks runtime
//---------------------------------------------------------------------------------------------
GFSDK_HairSDK *FurSample_LoadHairWorksDLL(void);

//---------------------------------------------------------------------------------------------
// Create texture resource and set it to hairworks runtime
//---------------------------------------------------------------------------------------------
HRESULT FurSample_SetupTextureResource(ID3D11Device *, GFSDK_HairSDK *, GFSDK_HairAssetID, GFSDK_HairInstanceID, const char *mediaPath);
