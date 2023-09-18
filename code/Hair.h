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

#ifndef HAIR_H
#define HAIR_H 1

#pragma warning(disable : 4995) // avoid warning for "...was marked as #pragma deprecated"
#pragma warning(disable : 4244) // avoid warning for "conversion from 'float' to 'int'"

#include "../shaders/HairCommon.h"

extern int g_Width;
extern int g_Height;

//-------------------------------------------------------------------
#define SMALL_NUMBER 0.00001

#ifndef SAFE_ACQUIRE
#define SAFE_ACQUIRE(dst, p)   \
	{                          \
		if (dst)               \
		{                      \
			SAFE_RELEASE(dst); \
		}                      \
		if (p)                 \
		{                      \
			(p)->AddRef();     \
		}                      \
		dst = (p);             \
	}
#endif

#include "Fluid.h"
#include <string>
using namespace std;

extern Fluid *g_fluid;

typedef int int4[4];
typedef DirectX::XMFLOAT2 float2;
typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT4 float4;

struct coordinateFrame
{
	float3 x;
	float3 y;
	float3 z;
};

struct CFVertex
{
	float4 Position;
	float3 Color;
};

struct Attributes
{
	DirectX::XMFLOAT2 texcoord;
};

struct collisionImplicit
{
	float3 center;
	float3 rotation;
	float3 scale;
};

struct StrandRoot
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Texcoord;
	DirectX::XMFLOAT3 Tangent;
};

struct GRID_TEXTURE_DISPLAY_STRUCT
{
	DirectX::XMFLOAT3 Pos; // Clip space position for slice vertices
	DirectX::XMFLOAT3 Tex; // Cell coordinates in 0-"texture dimension" range
};

class hairShadingParameters
{
	float m_baseColor[4];
	float m_specColor[4];
	float m_ksP;
	float m_ksS;
	float m_kd;
	float m_specPowerPrimary;
	float m_specPowerSecondary;
	float m_ksP_sparkles;
	float m_specPowerPrimarySparkles;
	float m_ka;
	
public:
	hairShadingParameters()
	{

	};

	void assignValues(float baseColor[4], float specColor[4], float ksP, float ksS, float kd, float specPowerPrimary, float specPowerSecondary, float ksP_sparkles, float specPowerPrimarySparkles, float ka)
	{
		m_baseColor[0] = baseColor[0];
		m_baseColor[1] = baseColor[1];
		m_baseColor[2] = baseColor[2];
		m_baseColor[3] = baseColor[3];
		m_specColor[0] = specColor[0];
		m_specColor[1] = specColor[1];
		m_specColor[2] = specColor[2];
		m_specColor[3] = specColor[3];
		m_ksP = ksP;
		m_ksS = ksS;
		m_kd = kd;
		m_specPowerPrimary = specPowerPrimary;
		m_specPowerSecondary = specPowerSecondary;
		m_ksP_sparkles = ksP_sparkles;
		m_specPowerPrimarySparkles = specPowerPrimarySparkles;
		m_ka = ka;
	};

	void setShaderVariables()
	{
		DirectX::XMFLOAT4 const baseColor(this->m_baseColor[0], this->m_baseColor[1], this->m_baseColor[2], this->m_baseColor[3]);
		DirectX::XMFLOAT4 const specColor(this->m_specColor[0], this->m_specColor[1], this->m_specColor[2], this->m_specColor[3]);
		HairEffect_SetShaderVariables(this->m_ksP, this->m_ksS, this->m_kd, this->m_ka, this->m_specPowerPrimary, this->m_specPowerSecondary, this->m_ksP_sparkles, this->m_specPowerPrimarySparkles, baseColor, specColor);
	}
};

enum BLURTYPE
{
	GAUSSIAN_BLUR,
	BOX_BLUR,
};

enum IMPLICIT_TYPE
{
	SPHERE,
	CYLINDER,
	SPHERE_NO_MOVE_CONSTRAINT,
	NOT_AN_IMPLICIT
};

struct collisionObject
{
	bool isHead;						  // if this is the head it is also used to transform the hair
	string boneName;					  // the name of the bone that this object is attached. if there is no bone then we use the global transform for the object
	IMPLICIT_TYPE implicitType;			  // type of implicit: sphere or cylinder
	DirectX::XMFLOAT4X4 InitialTransform; // the total initial transform; this is the transform that takes a unit implicit to the hair base pose
	DirectX::XMFLOAT4X4 currentTransform; // the transform that takes the implicit from base pose to the current hair space (different from world space)
	DirectX::XMFLOAT4X4 objToMesh;		  // the transform that goes from the hair base pose to the mesh bind pose, such that after multiplying with the meshWorldXForm we end up at the correct real world position
};

enum RENDERTYPE
{
	INSTANCED_DEPTH, 
	INSTANCED_DENSITY, 
	INSTANCED_NORMAL_HAIR, 
	INSTANCED_INTERPOLATED_COLLISION, 
	SOATTRIBUTES,

	INSTANCED_COLLISION_RESULTS,
};
enum INTERPOLATE_MODEL
{
	HYBRID,
	MULTI_HYBRID,
	MULTISTRAND,
	SINGLESTRAND,
	NO_HAIR,

	NUM_INTERPOLATE_MODELS
};

typedef int Index;
typedef Index Wisp[4];

extern Attributes *g_MasterAttributes;
class CDXUTSDKMesh;
extern CDXUTSDKMesh *g_Scalp;
extern int g_NumWisps;
extern int g_NumMasterStrands;
extern unsigned int g_TessellatedMasterStrandLengthMax;
extern int g_NumMasterStrandControlVertices;
extern HairVertex *g_MasterStrandControlVertices;
extern Index *g_MasterStrandControlVertexOffsets;
extern Index *g_TessellatedMasterStrandVertexOffsets;
// extern StrandRoot (*g_StrandRoots)[NUM_STRANDS_PER_WISP];
//  Rendering
extern int g_NumTessellatedMasterStrandVertices;
extern int g_NumTessellatedWisps;
extern int *g_indices;
extern int g_MasterStrandLengthMax;

extern float *g_coordinateFrames;

extern float g_maxHairLength;
extern float g_scalpMaxRadius;

extern float vecLength(HairVertex v1, HairVertex v2);

bool LoadMayaHair(char *directory, bool b_shortHair);

extern void rotateVector(const float3 &rotationAxis, float theta, const float3 &prevVec, float3 &newVec);
extern void vectorMatrixMultiply(DirectX::XMFLOAT3 *vecOut, const DirectX::XMFLOAT4X4 matrix, const DirectX::XMFLOAT3 vecIn);
extern void vectorMatrixMultiply(DirectX::XMFLOAT4 *vecOut, const DirectX::XMFLOAT4X4 matrix, const DirectX::XMFLOAT4 vecIn);

#endif