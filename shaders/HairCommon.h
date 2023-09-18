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

#ifndef SHADERS_HAIRCOMMON_H
#define SHADERS_HAIRCOMMON_H 1

#define NUM_MRTS 8
#define NUM_PASSES 1
#define ARRAY_SIZE (NUM_PASSES * NUM_MRTS)

// demo---------------------------------------------------------------

#define NUM_TESSELLATION_SEGMENTS 2
// #define HIGH_RES_MODEL
#define ADAPTIVE_HIGH_RES_MODEL

#ifdef HIGH_RES_MODEL
#define NUM_STRANDS_PER_WISP 10
#elif defined(ADAPTIVE_HIGH_RES_MODEL)
#define NUM_STRANDS_PER_WISP 1
#else
#define NUM_STRANDS_PER_WISP 20
#endif

// #define USE_CSM
// #define CSM_ZMAX 1.0
// #define CSM_HALF_NUM_TEX 4

//-------------------------------------------------------------------

/*
//testing -----------------------------------------------------------

#define NUM_TESSELLATION_SEGMENTS 1
//#define HIGH_RES_MODEL
#define ADAPTIVE_HIGH_RES_MODEL

#ifdef HIGH_RES_MODEL
	#define NUM_STRANDS_PER_WISP 10
#elif defined (ADAPTIVE_HIGH_RES_MODEL)
	#define NUM_STRANDS_PER_WISP 1
#else
	#define NUM_STRANDS_PER_WISP 20
#endif
*/

//-------------------------------------------------------------------
#define MAX_INTERPOLATED_HAIR_PER_WISP 200 // note the g_NumMStrandsPerWisp variable cannot/should not be larger than this

#define g_NumMStrandsPerWisp 45
#define g_NumSStrandsPerWisp 80

#define g_thinning 0.5

//-------------------------------------------------------------------
#define SLOT_MASTERSTRANDSB 57
#define SLOT_MASTERSTRAND 10
#define SLOT_TESSELLATEDMASTERSTRAND 9
#define SLOT_TESSELLATEDCOORDINATEFRAMES 19
#define SLOT_TESSELLATEDLENGTHSTOROOTS 20
#define SLOT_TESSELLATEDTANGENTS 22
#define SLOT_COLLISIONSTEXTURE 44
#define SLOT_INTERPOLATEDPOSITIONANDWIDTH 29
#define SLOT_INTERPOLATEDIDALPHATEX 30
#define SLOT_INTERPOLATEDTANGENT 31
#define SLOT_INTERPOLATEDPOSITIONANDWIDTHCLUMP 32
#define SLOT_INTERPOLATEDIDALPHATEXCLUMP 33
#define SLOT_INTERPOLATEDTANGENTCLUMP 34
#define SLOT_SSTRANDSPERMASTERSTRANDCUMULATIVE_OR_MSTRANDPERWISPCUMULATIVE 35

#define SLOT_TEXTURE_DENSITY 48
#define SLOT_TEXTURE_DENSITY_DEMUX 49
#define SLOT_TEXTURE_DENSITY_BLUR_TEMP 50
#define SLOT_TEXTURE_DENSITY_BLUR 51
#define SLOT_TEXTURE_TO_BLUR 52
#define SLOT_TEXTURE_VOXELIZED_OBSTACLES 53

#define SLOT_TSHADOWMAP 54

#if defined(__STDC__) || defined(__cplusplus)

struct HairVertex
{
	DirectX::XMFLOAT4 Position;
};

#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)

struct HairVertex
{
	float4 Position : Position;
};

#define TEXREG(n) register(t##n)

#else
#error Unknown Compiler
#endif

#define MAX_IMPLICITS 10

#define USETEXTURES
#define USE_TEXTURE_COLOR

#endif