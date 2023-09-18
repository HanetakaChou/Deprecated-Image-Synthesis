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
#define HAIR_H

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
#define CSM_ZMAX 1.0
#define CSM_HALF_NUM_TEX 4

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

#ifdef __cplusplus

struct HairVertex
{
	float4 Position;
};

#else

struct HairVertex
{
	float4 Position : Position;
};

#endif

#define MAX_IMPLICITS 10

#define USETEXTURES
#define USE_TEXTURE_COLOR

#endif