// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE,
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

#ifndef _COMMON_H_
#define _COMMON_H_ 1

#include <DXUT.h>

// Some common globals shared across
extern int g_Width;
extern int g_Height;

// Hair Effect

void HairEffect_Init(ID3D11Device *device);

void HairEffect_SetShaderVariables(float ksP, float ksS, float kd, float ka, float specPowerPrimary, float specPowerSecondary, float ksP_sparkles, float specPowerPrimarySparkles, DirectX::XMFLOAT4 const& baseColor, DirectX::XMFLOAT4 const& specColor);

void HairEffect_SetFirstPatchHair(int iFirstPatchHair);

void HairEffect_SetSubHairFirstVert(int iSubHairFirstVert);

void HairEffect_SetAdditionalTransformation(bool bApplyAdditionalRenderingTransform, DirectX::XMFLOAT4X4 const& AdditionalTransformation);

void HairEffect_SetLight(DirectX::XMFLOAT3 const& vLightDir, DirectX::XMFLOAT4X4 const& mLightViewProjClip2Tex, DirectX::XMFLOAT4X4 const& mLightView, DirectX::XMFLOAT4X4 const& mLightViewProj);

void HairEffect_SetDensityGrid(float textureWidth, float textureHeight, float textureDepth, int rowWidth, int colWidth);

void HairEffect_SetFluidObstacleTexture(int fluidTextureWidth, int fluidTextureHeight, int fluidTextureDepth);

void HairEffect_SetGridZIndex(int gridZIndex);

void HairEffect_Apply_RenderHair_InterpolateAndRenderM_HardwareTess(ID3D11DeviceContext *context);

void HairEffect_Apply_RenderHair_InterpolateAndRenderS_HardwareTess(ID3D11DeviceContext *context);

void HairEffect_Apply_RenderHair_InterpolateAndRenderCollisions_HardwareTess(ID3D11DeviceContext *context);

void HairEffect_Apply_TextureDemux(ID3D11DeviceContext* context);

void HairEffect_Apply_VoxelizeObstacles(ID3D11DeviceContext* context);

void HairEffect_Apply_DemuxTo3DFluidObstacles(ID3D11DeviceContext* context);

void FluidSimEffect_Init(ID3D11Device* device);

void FluidSimEffect_SetFluidType(int fluidType);

void FluidSimEffect_SetDrawTextureNumber(int drawTextureNumber);

void FluidSimEffect_SetFluidGrid(float textureWidth, float textureHeight, float textureDepth);

void FluidSimEffect_SetObstBoxcorner(DirectX::XMFLOAT3 const& obstBoxLBDcorner, DirectX::XMFLOAT3 const& obstBoxRTUcorner);

void FluidSimEffect_SetObstBoxVelocity(DirectX::XMFLOAT3 const& obstBoxVelocity);

void FluidSimEffect_Apply_DrawTexture(ID3D11DeviceContext* context);

#endif