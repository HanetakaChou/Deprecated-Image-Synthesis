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

#ifndef _HAIR_SHADOWS_H_
#define _HAIR_SHADOWS_H_ 1

#include <DXUT.h>
#include <DXUTcamera.h>

#include "SimpleRT.h"

#define LIGHT_RES 1024

class HairShadows
{
	D3D11_VIEWPORT m_LViewport;
	CModelViewerCamera m_LCamera;
	SimpleRT *m_pDepthsRT;
	DepthRT *m_pDepthsDS;

	DirectX::XMFLOAT4X4 m_mWorldViewProj;
	DirectX::XMFLOAT3 m_vLightDirWorld;
	DirectX::XMFLOAT3 m_vLightCenterWorld;

public:
	HairShadows()
	{
		m_LViewport.Width = LIGHT_RES;
		m_LViewport.Height = LIGHT_RES;
		m_LViewport.MinDepth = 0;
		m_LViewport.MaxDepth = 1;
		m_LViewport.TopLeftX = 0;
		m_LViewport.TopLeftY = 0;

		m_pDepthsRT = NULL;
		m_pDepthsDS = NULL;
	}

	void OnD3D11CreateDevice(ID3D11Device *pd3dDevice, ID3D11DeviceContext *pd3dContext)
	{

		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = LIGHT_RES;
		texDesc.Height = LIGHT_RES;
		texDesc.ArraySize = 1;
		texDesc.MiscFlags = 0;
		texDesc.MipLevels = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Format = DXGI_FORMAT_R32_FLOAT;
		m_pDepthsRT = new SimpleRT(pd3dDevice, pd3dContext, &texDesc);

		texDesc.ArraySize = 1;
		texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = NULL;
		texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		texDesc.Width = LIGHT_RES;
		texDesc.Height = LIGHT_RES;
		texDesc.MipLevels = 1;
		texDesc.MiscFlags = NULL;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		m_pDepthsDS = new DepthRT(pd3dDevice, &texDesc);
	}

	void OnD3D11DestroyDevice()
	{
		SAFE_DELETE(m_pDepthsRT);
		SAFE_DELETE(m_pDepthsDS);
	}

	CModelViewerCamera &GetCamera()
	{
		return this->m_LCamera;
	}

	DirectX::XMFLOAT4X4 &GetLightWorldViewProj()
	{
		return this->m_mWorldViewProj;
	}

	DirectX::XMFLOAT3 &GetLightWorldDir()
	{
		return this->m_vLightDirWorld;
	}

	DirectX::XMFLOAT3 &GetLightCenterWorld()
	{
		return this->m_vLightCenterWorld;
	}

	void UpdateMatrices(DirectX::XMFLOAT3 bbCenter, DirectX::XMFLOAT3 bbExtents)
	{
		DirectX::XMMATRIX mTrans = DirectX::XMMatrixTranslation(-bbCenter.x, -bbCenter.y, -bbCenter.z);

		DirectX::XMMATRIX mWorld = DirectX::XMMatrixMultiply(mTrans, m_LCamera.GetWorldMatrix());

		DirectX::XMMATRIX mWorldI = DirectX::XMMatrixInverse(NULL, mWorld);

		DirectX::XMMATRIX mWorldIT = DirectX::XMMatrixTranspose(mWorldI);

		DirectX::XMMATRIX mView = this->m_LCamera.GetViewMatrix();

		DirectX::XMMATRIX mWorldView = DirectX::XMMatrixMultiply(mWorld, mView);

		DirectX::XMMATRIX mWorldViewI = DirectX::XMMatrixInverse(NULL, mWorldView);

		DirectX::XMMATRIX mWorldViewIT = DirectX::XMMatrixTranspose(mWorldViewI);

		DirectX::XMFLOAT3 vBox[2];
		vBox[0].x = bbCenter.x - bbExtents.x;
		vBox[0].y = bbCenter.y - bbExtents.y;
		vBox[0].z = bbCenter.z - bbExtents.z;
		vBox[1].x = bbCenter.x + bbExtents.x;
		vBox[1].y = bbCenter.y + bbExtents.y;
		vBox[1].z = bbCenter.z + bbExtents.z;

		DirectX::XMFLOAT3 BBox[2];
		BBox[0].x = BBox[0].y = BBox[0].z = FLT_MAX;
		BBox[1].x = BBox[1].y = BBox[1].z = -FLT_MAX;
		for (int i = 0; i < 8; ++i)
		{
			DirectX::XMFLOAT3 v;
			v.x = vBox[(i & 1) ? 0 : 1].x;
			v.y = vBox[(i & 2) ? 0 : 1].y;
			v.z = vBox[(i & 4) ? 0 : 1].z;

			DirectX::XMFLOAT3 v1;
			DirectX::XMStoreFloat3(&v1, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&v), mWorldView));

			BBox[0].x = std::min(BBox[0].x, v1.x);
			BBox[0].y = std::min(BBox[0].y, v1.y);
			BBox[0].z = std::min(BBox[0].z, v1.z);
			BBox[1].x = std::max(BBox[1].x, v1.x);
			BBox[1].y = std::max(BBox[1].y, v1.y);
			BBox[1].z = std::max(BBox[1].z, v1.z);
		}

		float ZNear = BBox[0].z;
		float ZFar = BBox[1].z;

		float w = (FLOAT)std::max(std::abs(BBox[0].x), (FLOAT)std::abs(BBox[1].x)) * 2;
		float h = (FLOAT)std::max(std::abs(BBox[0].y), (FLOAT)std::abs(BBox[1].y)) * 2;
		
		DirectX::XMMATRIX mProj = DirectX::XMMatrixOrthographicLH(w, h, ZNear, ZFar);

		DirectX::XMMATRIX mWorldViewProj = DirectX::XMMatrixMultiply(mWorldView, mProj);

		DirectX::XMMATRIX mWorldViewProjI = DirectX::XMMatrixInverse(NULL, mWorldViewProj);

		DirectX::XMFLOAT4X4 mClip2Tex =
			DirectX::XMFLOAT4X4(
				0.5, 0, 0, 0,
				0, -0.5, 0, 0,
				0, 0, 1, 0,
				0.5, 0.5, 0, 1);
		DirectX::XMMATRIX mLightViewProjClip2Tex = DirectX::XMMatrixMultiply(mWorldViewProj, DirectX::XMLoadFloat4x4(&mClip2Tex));

		DirectX::XMFLOAT3 vLightDirClip = DirectX::XMFLOAT3(0, 0, 1);
		DirectX::XMVECTOR vLightDirWorld = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&vLightDirClip), mWorldViewProjI);
		vLightDirWorld = DirectX::XMVectorScale(vLightDirWorld, -1.0F);
		vLightDirWorld = DirectX::XMVector3Normalize(vLightDirWorld);

		DirectX::XMFLOAT3 vLightCenterClip = DirectX::XMFLOAT3(0, 0, 0.5);
		DirectX::XMVECTOR vLightCenterWorld = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&vLightCenterClip), mWorldViewProjI);

		DirectX::XMStoreFloat4x4(&this->m_mWorldViewProj, mWorldViewProj);
		DirectX::XMStoreFloat3(&this->m_vLightDirWorld, vLightDirWorld);
		DirectX::XMStoreFloat3(&this->m_vLightCenterWorld, vLightCenterWorld);

		DirectX::XMFLOAT4X4 _mLightViewProjClip2Tex;
		DirectX::XMStoreFloat4x4(&_mLightViewProjClip2Tex, mLightViewProjClip2Tex);
		
		DirectX::XMFLOAT4X4 _mLightView;
		DirectX::XMStoreFloat4x4(&_mLightView, mWorldView);

		DirectX::XMFLOAT4X4 _mLightViewProj;
		DirectX::XMStoreFloat4x4(&_mLightViewProj, mWorldViewProj);

		HairEffect_SetLight(this->m_vLightDirWorld, _mLightViewProjClip2Tex, _mLightView, _mLightViewProj);
	}

	void BeginShadowMapRendering(ID3D11DeviceContext *context)
	{
		float ClearColor[4] = {FLT_MAX};
		context->ClearRenderTargetView(*m_pDepthsRT, ClearColor);

		context->ClearDepthStencilView(*m_pDepthsDS, D3D11_CLEAR_DEPTH, 1.0, 0);

		ID3D11ShaderResourceView *const NULLSRV = NULL;
		context->VSSetShaderResources(SLOT_TSHADOWMAP, 1, &NULLSRV);
		context->HSSetShaderResources(SLOT_TSHADOWMAP, 1, &NULLSRV);
		context->DSSetShaderResources(SLOT_TSHADOWMAP, 1, &NULLSRV);
		context->GSSetShaderResources(SLOT_TSHADOWMAP, 1, &NULLSRV);
		context->PSSetShaderResources(SLOT_TSHADOWMAP, 1, &NULLSRV);

		ID3D11RenderTargetView *pRTVs[1] = {*m_pDepthsRT};
		context->OMSetRenderTargets(1, pRTVs, *m_pDepthsDS);
		context->RSSetViewports(1, &m_LViewport);
	}

	void EndShadowMapRendering(ID3D11DeviceContext *context)
	{
		context->OMSetRenderTargets(0, NULL, NULL);
	}

	void SetHairShadowTexture(ID3D11DeviceContext *context)
	{
		ID3D11ShaderResourceView *const pDepthsRT_SRV = (*m_pDepthsRT);
		context->VSSetShaderResources(SLOT_TSHADOWMAP, 1, &pDepthsRT_SRV);
		context->HSSetShaderResources(SLOT_TSHADOWMAP, 1, &pDepthsRT_SRV);
		context->DSSetShaderResources(SLOT_TSHADOWMAP, 1, &pDepthsRT_SRV);
		context->GSSetShaderResources(SLOT_TSHADOWMAP, 1, &pDepthsRT_SRV);
		context->PSSetShaderResources(SLOT_TSHADOWMAP, 1, &pDepthsRT_SRV);
	}

	void UnSetHairShadowTexture(ID3D11DeviceContext *context)
	{
		ID3D11ShaderResourceView *const NULLSRV = NULL;
		context->VSSetShaderResources(SLOT_TSHADOWMAP, 1, &NULLSRV);
		context->HSSetShaderResources(SLOT_TSHADOWMAP, 1, &NULLSRV);
		context->DSSetShaderResources(SLOT_TSHADOWMAP, 1, &NULLSRV);
		context->GSSetShaderResources(SLOT_TSHADOWMAP, 1, &NULLSRV);
		context->PSSetShaderResources(SLOT_TSHADOWMAP, 1, &NULLSRV);
	}
};

#endif
