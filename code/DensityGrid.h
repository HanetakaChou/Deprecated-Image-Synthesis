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

#ifndef _DENSITY_GRID_H_
#define _DENSITY_GRID_H_ 1

#include "Hair.h"

#define MAX_RTS_TOTAL 256
#define MAX_RT_GROUPS 8 // 256/32 = 8 -- we get 32 because in one render call we can render to 8 RTs and each RT can have 4 channels, leading to 8*4=32

struct GRID_TEXTURE_DISPLAY_STRUCT;

class HairGrid
{

public:
	enum RENDER_TARGET
	{
		// RENDER_TARGET_VELOCITY,
		RENDER_TARGET_DENSITY,
		RENDER_TARGET_DENSITY_DEMUX,
		RENDER_TARGET_DENSITY_BLUR_TEMP,
		RENDER_TARGET_DENSITY_BLUR,
		RENDER_TARGET_VOXELIZED_OBSTACLES,
		NUM_RENDER_TARGET_TYPES
	};

	HairGrid(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dContext);
	virtual ~HairGrid();
	HRESULT Initialize(int width, int height, int depth);
	void Reset(void);
	HRESULT AddHairDensity(DirectX::XMFLOAT4X4 &objToVolumeXForm, HRESULT (*DrawHair)(DirectX::XMFLOAT4X4 &, ID3D11Device *, ID3D11DeviceContext *, float, float, int));
	HRESULT demultiplexTexture(ID3D11DeviceContext* context);
	HRESULT setFluidObstacleTexture(ID3D11Texture3D *texture, DXGI_FORMAT format, int texWidth, int texHeight, int texDepth);
	HRESULT demultiplexTextureTo3DFluidObstacles(ID3D11DeviceContext* context);
	void ReloadShader();
	HRESULT voxelizeObstacles(ID3D11DeviceContext* context);

	void setDemuxTexture(ID3D11DeviceContext* context);
	void setObstacleVoxelizedTexture(ID3D11DeviceContext* context);
	void setBlurTexture(ID3D11DeviceContext* context);

	void unsetDemuxTexture(ID3D11DeviceContext* context);
	void unsetObstacleVoxelizedTexture(ID3D11DeviceContext* context);
	void unsetBlurTexture(ID3D11DeviceContext* context);

protected:
	HairGrid(void){};
	void InitScreenSlice(GRID_TEXTURE_DISPLAY_STRUCT **vertices, int z, int &index);
	void InitGridSlice(GRID_TEXTURE_DISPLAY_STRUCT **vertices);
	HRESULT CreateVertexBuffer(int ByteWidth, UINT bindFlags, ID3D11Buffer **vertexBuffer, GRID_TEXTURE_DISPLAY_STRUCT *vertices, int numVertices);
	void DrawSlicesToScreen(void);
	HRESULT CreateRTTextureAsShaderResource(RENDER_TARGET rtIndex, LPCSTR shaderTextureName, D3D11_SHADER_RESOURCE_VIEW_DESC *SRVDesc);

	int m_dim[3];
	int m_Width;
	int m_Height;
	int m_Depth;
	int m_numVerticesRenderQuad;
	int m_numVerticesRenderQuadsForGrid;
	int m_cols;
	int m_rows;
	int m_numRTVs;
	int m_totalDepth;

	int m_fluidGridWidth;
	int m_fluidGridHeight;
	int m_fluidGridDepth;

	// density
	ID3D11Texture2D *pDensity2DTextureArray;
	ID3D11RenderTargetView *pDensityRenderTargetViews[MAX_RT_GROUPS * 8];
	// demux
	ID3D11Texture2D *pDensityDemux2DTextureArray;
	ID3D11RenderTargetView *pDensityDemuxRenderTargetViews[MAX_RTS_TOTAL];
	// blurx
	ID3D11Texture2D *pDensityBlurTemp2DTextureArray;
	ID3D11RenderTargetView *pDensityBlurTempRenderTargetViews[MAX_RTS_TOTAL];
	// blury
	ID3D11Texture2D *pDensityBlur2DTextureArray;
	ID3D11RenderTargetView *pDensityBlurRenderTargetViews[MAX_RTS_TOTAL];
	// obstacles
	ID3D11Texture2D *pVoxelizedObstacle2DTextureArray;
	ID3D11RenderTargetView *pVoxelizedObstacleRenderTargetViews[MAX_RTS_TOTAL];

	// fluid obstacle texture
	ID3D11Texture3D *m_pFluidObstacleTexture3D;
	ID3D11RenderTargetView *m_pRenderTargetViewFluidObstacles;

	ID3D11ShaderResourceView *pRenderTargetShaderViews[NUM_RENDER_TARGET_TYPES];
	// ID3DX11EffectShaderResourceVariable *pShaderResourceVariables[NUM_RENDER_TARGET_TYPES];
	DXGI_FORMAT RenderTargetFormats[NUM_RENDER_TARGET_TYPES];

	ID3D11Device *m_pD3DDevice;
	ID3D11DeviceContext *m_pD3DContext;

	ID3D11Buffer *renderQuadBuffer;
	ID3D11Buffer *renderQuadBufferForGrid;
	ID3D11InputLayout *quad_layout;
};

#endif