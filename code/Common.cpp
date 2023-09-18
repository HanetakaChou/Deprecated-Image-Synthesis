#include "Common.h"
#include "../shaders/HairCommon.h"
#include "../shaders/FluidCommon.h"

#include "../dxbc/Hair_InterpolateVS_DUMMY11_bytecode.inl"
#include "../dxbc/Hair_InterpolateHSMultiStrand_bytecode.inl"
#include "../dxbc/Hair_InterpolateHSSingleStrand_bytecode.inl"
#include "../dxbc/Hair_InterpolateHSMultiStrandCollisions_bytecode.inl"
#include "../dxbc/Hair_InterpolateDSMultiStrand_bytecode.inl"
#include "../dxbc/Hair_InterpolateDSSingleStrand_NORMAL_bytecode.inl"
#include "../dxbc/Hair_InterpolateDSMultiStrandCollisionsWithGS_bytecode.inl"
#include "../dxbc/Hair_InterpolateGS_bytecode.inl"
#include "../dxbc/Hair_InterpolateGSMultiStrandCollisions_bytecode.inl"
#include "../dxbc/Hair_RenderPS_bytecode.inl"
#include "../dxbc/Hair_InterpolatePSMultiStrandCollisions_bytecode.inl"

#include "../dxbc/Hair_VS_GRID_bytecode.inl"
#include "../dxbc/Hair_VS_GRID_FLUIDSIM_bytecode.inl"
#include "../dxbc/Hair_GS_ARRAY_bytecode.inl"
#include "../dxbc/Hair_PS_DEMUX_bytecode.inl"
#include "../dxbc/Hair_PS_VOXELIZE_OBSTACLES_bytecode.inl"
#include "../dxbc/Hair_PS_DEMUX_TO_3D_OBSTACLE_TEX_bytecode.inl"

struct CB_HAIR_PER_DRAW
{
    float g_ScreenWidth;
    float g_ScreenHeight;
    float g_bApplyAdditionalTransform;
    float g_bApplyAdditionalRenderingTransform;

    DirectX::XMFLOAT4X4 ViewProjection;
    DirectX::XMFLOAT4X4 WorldView;
    DirectX::XMFLOAT4X4 WorldViewProjection;
    DirectX::XMFLOAT4X4 WorldToGrid;
    DirectX::XMFLOAT4X4 GridToWorld;
    DirectX::XMFLOAT4X4 RootTransformation;
    DirectX::XMFLOAT4X4 additionalTransformation;
    DirectX::XMFLOAT4X4 TotalTransformation;
    DirectX::XMFLOAT4X4 HairToWorldTransform;
    DirectX::XMFLOAT4X4 currentHairTransformation;
    DirectX::XMFLOAT4X4 currentHairTransformationInverse;

    // for plane:
    DirectX::XMFLOAT3 vLightPos;
    float _Unused_Padding_0;

    float SStextureWidth;
    float SStextureHeight;
    float _Unused_Padding_1;
    float _Unused_Padding_2;

    DirectX::XMFLOAT3 EyePosition;
    float _Unused_Padding_3;
    DirectX::XMFLOAT4 arrowColor;
    DirectX::XMFLOAT3 TransformedEyePosition;
    float _Unused_Padding_4;

    // parameters for shading
    float g_ksP;
    float g_ksS;
    float g_kd;
    float g_ka;
    float g_specPowerPrimary;
    float g_specPowerSecondary;
    float g_ksP_sparkles;
    float g_specPowerPrimarySparkles;
    float g_fNumHairsLOD;
    float g_fWidthHairsLOD;
    float _Unused_Padding_5;
    float _Unused_Padding_6;

    DirectX::XMFLOAT4 g_baseColor;
    DirectX::XMFLOAT4 g_specColor;

    float g_maxLengthToRoot; // this is the maximum length of any strand in the whole hairstyle
    float g_useScalpTexture;
    float _Unused_Padding_7;
    float _Unused_Padding_8;

    // parameters for shadow:
    int g_NumTotalWisps;
    int _Unused_Padding_9;
    int _Unused_Padding_10;
    int _Unused_Padding_11;
    float g_blendAxis; // for coordinate frame correction
    float doCurlyHair;
    float g_angularStiffness;
    float g_bApplyHorizontalForce;
    float g_bAddGravity;
    float TimeStep;
    float g_gravityStrength;
    float g_bSimulate;

    // wind
    DirectX::XMFLOAT3 windForce;
    float _Unused_Padding_12;

    // wind fluid simulation
    int fluidTextureWidth;
    int fluidTextureHeight;
    int fluidTextureDepth;
    int _Unused_Padding_13;

    // body collisions
    int g_NumSphereImplicits;
    int g_NumCylinderImplicits;
    int _Unused_Padding_14;
    int _Unused_Padding_15;

    int g_NumSphereNoMoveImplicits;
    int _Unused_Padding_16;
    int _Unused_Padding_17;
    int _Unused_Padding_18;
    int _Unused_Padding_19;

    DirectX::XMFLOAT4X4 CollisionSphereTransformations[MAX_IMPLICITS];
    DirectX::XMFLOAT4X4 CollisionSphereInverseTransformations[MAX_IMPLICITS];
    DirectX::XMFLOAT4X4 CollisionCylinderTransformations[MAX_IMPLICITS];
    DirectX::XMFLOAT4X4 CollisionCylinderInverseTransformations[MAX_IMPLICITS];

    DirectX::XMFLOAT4X4 SphereNoMoveImplicitTransform[MAX_IMPLICITS];
    DirectX::XMFLOAT4X4 SphereNoMoveImplicitInverseTransform[MAX_IMPLICITS];

    int g_UseSphereForBarycentricInterpolant[MAX_IMPLICITS];
    int _Unused_Padding_20;
    int _Unused_Padding_21;

    int g_UseCylinderForBarycentricInterpolant[MAX_IMPLICITS];
    int _Unused_Padding_22;
    int _Unused_Padding_23;

    int g_UseSphereNoMoveForBarycentricInterpolant[MAX_IMPLICITS];
    int _Unused_Padding_24;
    int _Unused_Padding_25;

    // render the collision spheres
    int currentCollisionSphere;
    int g_TessellatedMasterStrandLengthMax;
    int _Unused_Padding_26;
    int _Unused_Padding_27;

    float textureHeight;
    float textureWidth;
    float textureDepth;
    float _Unused_Padding_28;

    int rowWidth;
    int colWidth;
    int textureIndex;
    int gridZIndex;

    float useBlurTexture;
    float g_bClearForces;
    float useShadows;
    float _Unused_Padding_29;

    DirectX::XMFLOAT4X4 mLightView;
    DirectX::XMFLOAT4X4 mLightViewProj;
    DirectX::XMFLOAT4X4 mLightViewProjClip2Tex;

    DirectX::XMFLOAT3 vLightDir; // TO DO SHADOWS: if world matrices (like HairToWorldTransform) are incorporated into shadows
                                 //                this vector might change to be in world space rather than hair space.
                                 //                in that case calculations using it will have to change, since they are currently
                                 //                happening in hair space
    float _Unused_Padding_30;

    float g_SigmaA;
    float _Unused_Padding_31;
    float _Unused_Padding_32;
    float _Unused_Padding_33;

    DirectX::XMFLOAT3 vLightDirObjectSpace;
    float _Unused_Padding_34;

    int g_iFirstPatchHair;
    // this is index of first subshair vertex inside hair
    int g_iSubHairFirstVert;
    int _Unused_Padding_35;
    int _Unused_Padding_36;
};

static struct CB_HAIR_PER_DRAW g_cbHairPerDrawData;

static ID3D11Buffer* g_pcbHairPerDraw = NULL;

static ID3D11RasterizerState* g_SolidCull = NULL;
static ID3D11RasterizerState* g_SolidNoCull = NULL;
static ID3D11DepthStencilState* g_DepthTest = NULL;
static ID3D11DepthStencilState* g_NoDepthStencilTest = NULL;
static ID3D11BlendState* g_AlphaBlending = NULL;
static ID3D11BlendState* g_MinBlending = NULL;
static ID3D11BlendState* g_NoBlending = NULL;

static ID3D11VertexShader* g_Hair_InterpolateVS_DUMMY11 = NULL;
static ID3D11HullShader* g_Hair_InterpolateHSMultiStrand = NULL;
static ID3D11HullShader* g_Hair_InterpolateHSSingleStrand = NULL;
static ID3D11HullShader* g_Hair_InterpolateHSMultiStrandCollisions = NULL;
static ID3D11DomainShader* g_Hair_InterpolateDSMultiStrand = NULL;
static ID3D11DomainShader* g_Hair_InterpolateDSSingleStrand_NORMAL = NULL;
static ID3D11DomainShader* g_Hair_InterpolateDSMultiStrandCollisionsWithGS = NULL;
static ID3D11GeometryShader* g_Hair_InterpolateGS = NULL;
static ID3D11GeometryShader* g_Hair_InterpolateGSMultiStrandCollisions = NULL;
static ID3D11PixelShader* g_Hair_RenderPS = NULL;
static ID3D11PixelShader* g_Hair_InterpolatePSMultiStrandCollisions = NULL;

static ID3D11VertexShader* g_Hair_VS_GRID = NULL;
static ID3D11VertexShader* g_Hair_VS_GRID_FLUIDSIM = NULL;
static ID3D11GeometryShader* g_Hair_GS_ARRAY = NULL;
static ID3D11PixelShader* g_Hair_PS_DEMUX = NULL;
static ID3D11PixelShader* g_Hair_PS_VOXELIZE_OBSTACLES = NULL;
static ID3D11PixelShader* g_Hair_PS_DEMUX_TO_3D_OBSTACLE_TEX = NULL;

void HairEffect_Init(ID3D11Device* device)
{
    g_cbHairPerDrawData.g_ScreenWidth = g_Width;
    g_cbHairPerDrawData.g_ScreenHeight = g_Height;
    g_cbHairPerDrawData.g_fNumHairsLOD = 1.0F;
    g_cbHairPerDrawData.g_fWidthHairsLOD = 1.0F;
    g_cbHairPerDrawData.g_maxLengthToRoot = 12.0F; // this is the maximum length of any strand in the whole hairstyle
    g_cbHairPerDrawData.g_bApplyHorizontalForce = -1.0F;
    g_cbHairPerDrawData.g_bAddGravity = -1.0F;
    g_cbHairPerDrawData.TimeStep = 0.1F;
    g_cbHairPerDrawData.g_gravityStrength = 0.1F;
    g_cbHairPerDrawData.g_bSimulate = -1.0F;
    g_cbHairPerDrawData.windForce = DirectX::XMFLOAT3(-1.0F, 0.0F, 0.0F);

    HRESULT hr;

    D3D11_BUFFER_DESC HairPerDrawConstantBufferDesc;
    HairPerDrawConstantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    HairPerDrawConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    HairPerDrawConstantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HairPerDrawConstantBufferDesc.MiscFlags = 0;
    HairPerDrawConstantBufferDesc.ByteWidth = sizeof(struct CB_HAIR_PER_DRAW);
    V(device->CreateBuffer(&HairPerDrawConstantBufferDesc, NULL, &g_pcbHairPerDraw));

    D3D11_RASTERIZER_DESC SolidCullDesc;
    SolidCullDesc.FillMode = D3D11_FILL_SOLID;
    SolidCullDesc.CullMode = D3D11_CULL_BACK;
    SolidCullDesc.FrontCounterClockwise = FALSE;
    SolidCullDesc.DepthBias = 0;
    SolidCullDesc.SlopeScaledDepthBias = 0.0f;
    SolidCullDesc.DepthBiasClamp = 0.0f;
    SolidCullDesc.DepthClipEnable = TRUE;
    SolidCullDesc.ScissorEnable = FALSE;
    SolidCullDesc.MultisampleEnable = FALSE;
    SolidCullDesc.AntialiasedLineEnable = FALSE;
    //--------------------------------------------------------------------------------------------------
    SolidCullDesc.FillMode = D3D11_FILL_SOLID;
    SolidCullDesc.CullMode = D3D11_CULL_BACK;
    SolidCullDesc.AntialiasedLineEnable = FALSE;
    SolidCullDesc.MultisampleEnable = FALSE;
    V(device->CreateRasterizerState(&SolidCullDesc, &g_SolidCull));

    D3D11_RASTERIZER_DESC SolidNoCullDesc;
    SolidNoCullDesc.FillMode = D3D11_FILL_SOLID;
    SolidNoCullDesc.CullMode = D3D11_CULL_BACK;
    SolidNoCullDesc.FrontCounterClockwise = FALSE;
    SolidNoCullDesc.DepthBias = 0;
    SolidNoCullDesc.SlopeScaledDepthBias = 0.0f;
    SolidNoCullDesc.DepthBiasClamp = 0.0f;
    SolidNoCullDesc.DepthClipEnable = TRUE;
    SolidNoCullDesc.ScissorEnable = FALSE;
    SolidNoCullDesc.MultisampleEnable = FALSE;
    SolidNoCullDesc.AntialiasedLineEnable = FALSE;
    //--------------------------------------------------------------------------------------------------
    SolidNoCullDesc.FillMode = D3D11_FILL_SOLID;
    SolidNoCullDesc.CullMode = D3D11_CULL_NONE;
    SolidNoCullDesc.AntialiasedLineEnable = FALSE;
    SolidNoCullDesc.MultisampleEnable = FALSE;
    V(device->CreateRasterizerState(&SolidNoCullDesc, &g_SolidNoCull));

    D3D11_DEPTH_STENCIL_DESC DepthTestDesc = {};
    DepthTestDesc.DepthEnable = TRUE;
    DepthTestDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    DepthTestDesc.DepthFunc = D3D11_COMPARISON_LESS;
    DepthTestDesc.StencilEnable = FALSE;
    DepthTestDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    DepthTestDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    DepthTestDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    DepthTestDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    DepthTestDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    DepthTestDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    DepthTestDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    DepthTestDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    DepthTestDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    DepthTestDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    //--------------------------------------------------------------------------------------------------
    DepthTestDesc.DepthEnable = TRUE;
    DepthTestDesc.DepthFunc = D3D11_COMPARISON_LESS;
    V(device->CreateDepthStencilState(&DepthTestDesc, &g_DepthTest));

    D3D11_DEPTH_STENCIL_DESC NoDepthStencilTestDesc = {};
    NoDepthStencilTestDesc.DepthEnable = TRUE;
    NoDepthStencilTestDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    NoDepthStencilTestDesc.DepthFunc = D3D11_COMPARISON_LESS;
    NoDepthStencilTestDesc.StencilEnable = FALSE;
    NoDepthStencilTestDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    NoDepthStencilTestDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    NoDepthStencilTestDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    NoDepthStencilTestDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    NoDepthStencilTestDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    NoDepthStencilTestDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    NoDepthStencilTestDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    NoDepthStencilTestDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    NoDepthStencilTestDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    NoDepthStencilTestDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    //--------------------------------------------------------------------------------------------------
    NoDepthStencilTestDesc.DepthEnable = FALSE;
    NoDepthStencilTestDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    NoDepthStencilTestDesc.StencilEnable = FALSE;
    NoDepthStencilTestDesc.StencilReadMask = 0;
    NoDepthStencilTestDesc.StencilWriteMask = 0;
    V(device->CreateDepthStencilState(&DepthTestDesc, &g_NoDepthStencilTest));

    D3D11_BLEND_DESC AlphaBlendingDesc = {};
    AlphaBlendingDesc.AlphaToCoverageEnable = FALSE;
    AlphaBlendingDesc.IndependentBlendEnable = FALSE;
    AlphaBlendingDesc.RenderTarget[0].BlendEnable = FALSE;
    AlphaBlendingDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    AlphaBlendingDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    AlphaBlendingDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    AlphaBlendingDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    AlphaBlendingDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    AlphaBlendingDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    AlphaBlendingDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    AlphaBlendingDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    AlphaBlendingDesc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    AlphaBlendingDesc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    //--------------------------------------------------------------------------------------------------
    AlphaBlendingDesc.AlphaToCoverageEnable = FALSE;
    AlphaBlendingDesc.RenderTarget[0].BlendEnable = TRUE;
    AlphaBlendingDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    AlphaBlendingDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    AlphaBlendingDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    AlphaBlendingDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    AlphaBlendingDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    AlphaBlendingDesc.RenderTarget[0].RenderTargetWriteMask = 0X0F;
    V(device->CreateBlendState(&AlphaBlendingDesc, &g_AlphaBlending));

    D3D11_BLEND_DESC MinBlendingDesc = {};
    MinBlendingDesc.AlphaToCoverageEnable = FALSE;
    MinBlendingDesc.IndependentBlendEnable = FALSE;
    MinBlendingDesc.RenderTarget[0].BlendEnable = FALSE;
    MinBlendingDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    MinBlendingDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    MinBlendingDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    MinBlendingDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    MinBlendingDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    MinBlendingDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    MinBlendingDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    MinBlendingDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    MinBlendingDesc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    MinBlendingDesc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    //--------------------------------------------------------------------------------------------------
    MinBlendingDesc.AlphaToCoverageEnable = FALSE;
    MinBlendingDesc.RenderTarget[0].BlendEnable = TRUE;
    MinBlendingDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    MinBlendingDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    MinBlendingDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MIN;
    MinBlendingDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    MinBlendingDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    MinBlendingDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MIN;
    MinBlendingDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    V(device->CreateBlendState(&MinBlendingDesc, &g_MinBlending));

    D3D11_BLEND_DESC NoBlendingDesc = {};
    NoBlendingDesc.AlphaToCoverageEnable = FALSE;
    NoBlendingDesc.IndependentBlendEnable = FALSE;
    NoBlendingDesc.RenderTarget[0].BlendEnable = FALSE;
    NoBlendingDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    NoBlendingDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    NoBlendingDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    NoBlendingDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    NoBlendingDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    NoBlendingDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    NoBlendingDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    NoBlendingDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    NoBlendingDesc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    NoBlendingDesc.RenderTarget[3].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    //--------------------------------------------------------------------------------------------------
    NoBlendingDesc.AlphaToCoverageEnable = FALSE;
    NoBlendingDesc.RenderTarget[0].BlendEnable = FALSE;
    V(device->CreateBlendState(&NoBlendingDesc, &g_NoBlending));

    V(device->CreateVertexShader(Hair_InterpolateVS_DUMMY11_bytecode, sizeof(Hair_InterpolateVS_DUMMY11_bytecode), NULL, &g_Hair_InterpolateVS_DUMMY11));

    V(device->CreateHullShader(Hair_InterpolateHSMultiStrand_bytecode, sizeof(Hair_InterpolateHSMultiStrand_bytecode), NULL, &g_Hair_InterpolateHSMultiStrand));
    V(device->CreateHullShader(Hair_InterpolateHSSingleStrand_bytecode, sizeof(Hair_InterpolateHSSingleStrand_bytecode), NULL, &g_Hair_InterpolateHSSingleStrand));
    V(device->CreateHullShader(Hair_InterpolateHSMultiStrandCollisions_bytecode, sizeof(Hair_InterpolateHSMultiStrandCollisions_bytecode), NULL, &g_Hair_InterpolateHSMultiStrandCollisions));

    V(device->CreateDomainShader(Hair_InterpolateDSMultiStrand_bytecode, sizeof(Hair_InterpolateDSMultiStrand_bytecode), NULL, &g_Hair_InterpolateDSMultiStrand));
    V(device->CreateDomainShader(Hair_InterpolateDSSingleStrand_NORMAL_bytecode, sizeof(Hair_InterpolateDSSingleStrand_NORMAL_bytecode), NULL, &g_Hair_InterpolateDSSingleStrand_NORMAL));
    V(device->CreateDomainShader(Hair_InterpolateDSMultiStrandCollisionsWithGS_bytecode, sizeof(Hair_InterpolateDSMultiStrandCollisionsWithGS_bytecode), NULL, &g_Hair_InterpolateDSMultiStrandCollisionsWithGS));

    V(device->CreateGeometryShader(Hair_InterpolateGS_bytecode, sizeof(Hair_InterpolateGS_bytecode), NULL, &g_Hair_InterpolateGS));
    V(device->CreateGeometryShader(Hair_InterpolateGSMultiStrandCollisions_bytecode, sizeof(Hair_InterpolateGSMultiStrandCollisions_bytecode), NULL, &g_Hair_InterpolateGSMultiStrandCollisions));

    V(device->CreatePixelShader(Hair_RenderPS_bytecode, sizeof(Hair_RenderPS_bytecode), NULL, &g_Hair_RenderPS));
    V(device->CreatePixelShader(Hair_InterpolatePSMultiStrandCollisions_bytecode, sizeof(Hair_InterpolatePSMultiStrandCollisions_bytecode), NULL, &g_Hair_InterpolatePSMultiStrandCollisions));

    V(device->CreateVertexShader(Hair_VS_GRID_bytecode, sizeof(Hair_VS_GRID_bytecode), NULL, &g_Hair_VS_GRID));
    V(device->CreateVertexShader(Hair_VS_GRID_FLUIDSIM_bytecode, sizeof(Hair_VS_GRID_FLUIDSIM_bytecode), NULL, &g_Hair_VS_GRID_FLUIDSIM));

    V(device->CreateGeometryShader(Hair_GS_ARRAY_bytecode, sizeof(Hair_GS_ARRAY_bytecode), NULL, &g_Hair_GS_ARRAY));

    V(device->CreatePixelShader(Hair_PS_DEMUX_bytecode, sizeof(Hair_PS_DEMUX_bytecode), NULL, &g_Hair_PS_DEMUX));
    V(device->CreatePixelShader(Hair_PS_VOXELIZE_OBSTACLES_bytecode, sizeof(Hair_PS_VOXELIZE_OBSTACLES_bytecode), NULL, &g_Hair_PS_VOXELIZE_OBSTACLES));
    V(device->CreatePixelShader(Hair_PS_DEMUX_TO_3D_OBSTACLE_TEX_bytecode, sizeof(Hair_PS_DEMUX_TO_3D_OBSTACLE_TEX_bytecode), NULL, &g_Hair_PS_DEMUX_TO_3D_OBSTACLE_TEX));
}

void HairEffect_SetShaderVariables(float ksP, float ksS, float kd, float ka, float specPowerPrimary, float specPowerSecondary, float ksP_sparkles, float specPowerPrimarySparkles, DirectX::XMFLOAT4 const& baseColor, DirectX::XMFLOAT4 const& specColor)
{
    g_cbHairPerDrawData.g_ksP = ksP;
    g_cbHairPerDrawData.g_ksS = ksS;
    g_cbHairPerDrawData.g_kd = kd;
    g_cbHairPerDrawData.g_ka = ka;
    g_cbHairPerDrawData.g_specPowerPrimary = specPowerPrimary;
    g_cbHairPerDrawData.g_specPowerSecondary = specPowerSecondary;
    g_cbHairPerDrawData.g_ksP_sparkles = ksP_sparkles;
    g_cbHairPerDrawData.g_specPowerPrimarySparkles = specPowerPrimarySparkles;

    g_cbHairPerDrawData.g_baseColor = baseColor;
    g_cbHairPerDrawData.g_specColor = specColor;
}

void HairEffect_SetFirstPatchHair(int iFirstPatchHair)
{
    g_cbHairPerDrawData.g_iFirstPatchHair = iFirstPatchHair;
}

void HairEffect_SetSubHairFirstVert(int iSubHairFirstVert)
{
    g_cbHairPerDrawData.g_iSubHairFirstVert = iSubHairFirstVert;
}

void HairEffect_SetAdditionalTransformation(bool bApplyAdditionalRenderingTransform, DirectX::XMFLOAT4X4 const& AdditionalTransformation)
{
    g_cbHairPerDrawData.g_bApplyAdditionalRenderingTransform = bApplyAdditionalRenderingTransform ? 1.0F : -1.0F;
    g_cbHairPerDrawData.additionalTransformation = AdditionalTransformation;
}

void HairEffect_SetLight(DirectX::XMFLOAT3 const& vLightDir, DirectX::XMFLOAT4X4 const& mLightViewProjClip2Tex, DirectX::XMFLOAT4X4 const& mLightView, DirectX::XMFLOAT4X4 const& mLightViewProj)
{
    g_cbHairPerDrawData.vLightDir = vLightDir;
    g_cbHairPerDrawData.mLightViewProjClip2Tex = mLightViewProjClip2Tex;
    g_cbHairPerDrawData.mLightView = mLightView;
    g_cbHairPerDrawData.mLightViewProj = mLightViewProj;
}

void HairEffect_SetDensityGrid(float textureWidth, float textureHeight, float textureDepth, int rowWidth, int colWidth)
{
    g_cbHairPerDrawData.textureWidth = textureWidth;
    g_cbHairPerDrawData.textureHeight = textureHeight;
    g_cbHairPerDrawData.textureDepth = textureDepth;
    g_cbHairPerDrawData.rowWidth = rowWidth;
    g_cbHairPerDrawData.colWidth = colWidth;
}

void HairEffect_SetFluidObstacleTexture(int fluidTextureWidth, int fluidTextureHeight, int fluidTextureDepth)
{

    g_cbHairPerDrawData.fluidTextureWidth = fluidTextureWidth;
    g_cbHairPerDrawData.fluidTextureHeight = fluidTextureHeight;
    g_cbHairPerDrawData.fluidTextureDepth = fluidTextureDepth;
}

void HairEffect_SetGridZIndex(int gridZIndex)
{
    g_cbHairPerDrawData.gridZIndex = gridZIndex;
}

static void HairEffect_Apply_Common(ID3D11DeviceContext* context)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    context->Map(g_pcbHairPerDraw, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, &g_cbHairPerDrawData, sizeof(struct CB_HAIR_PER_DRAW));
    context->Unmap(g_pcbHairPerDraw, 0);

    context->VSSetConstantBuffers(0U, 1U, &g_pcbHairPerDraw);
    context->HSSetConstantBuffers(0U, 1U, &g_pcbHairPerDraw);
    context->DSSetConstantBuffers(0U, 1U, &g_pcbHairPerDraw);
    context->GSSetConstantBuffers(0U, 1U, &g_pcbHairPerDraw);
    context->PSSetConstantBuffers(0U, 1U, &g_pcbHairPerDraw);
}

void HairEffect_Apply_RenderHair_InterpolateAndRenderM_HardwareTess(ID3D11DeviceContext* context)
{
    HairEffect_Apply_Common(context);

    context->VSSetShader(g_Hair_InterpolateVS_DUMMY11, NULL, 0);
    context->HSSetShader(g_Hair_InterpolateHSMultiStrand, NULL, 0);
    context->DSSetShader(g_Hair_InterpolateDSMultiStrand, NULL, 0);
    context->GSSetShader(g_Hair_InterpolateGS, NULL, 0);
    context->PSSetShader(g_Hair_RenderPS, NULL, 0);
    FLOAT const BlendFactor[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
    context->OMSetBlendState(g_AlphaBlending, BlendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(g_DepthTest, 0);
    context->RSSetState(g_SolidNoCull);
}

void HairEffect_Apply_RenderHair_InterpolateAndRenderS_HardwareTess(ID3D11DeviceContext* context)
{
    HairEffect_Apply_Common(context);

    context->VSSetShader(g_Hair_InterpolateVS_DUMMY11, NULL, 0);
    context->HSSetShader(g_Hair_InterpolateHSSingleStrand, NULL, 0);
    context->DSSetShader(g_Hair_InterpolateDSSingleStrand_NORMAL, NULL, 0);
    context->GSSetShader(g_Hair_InterpolateGS, NULL, 0);
    context->PSSetShader(g_Hair_RenderPS, NULL, 0);
    FLOAT const BlendFactor[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
    context->OMSetBlendState(g_AlphaBlending, BlendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(g_DepthTest, 0);
    context->RSSetState(g_SolidNoCull);
}

void HairEffect_Apply_RenderHair_InterpolateAndRenderCollisions_HardwareTess(ID3D11DeviceContext* context)
{
    HairEffect_Apply_Common(context);

    context->VSSetShader(g_Hair_InterpolateVS_DUMMY11, NULL, 0);
    context->HSSetShader(g_Hair_InterpolateHSMultiStrandCollisions, NULL, 0);
    context->DSSetShader(g_Hair_InterpolateDSMultiStrandCollisionsWithGS, NULL, 0);
    context->GSSetShader(g_Hair_InterpolateGSMultiStrandCollisions, NULL, 0);
    context->PSSetShader(g_Hair_InterpolatePSMultiStrandCollisions, NULL, 0);
    FLOAT const BlendFactor[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
    context->OMSetBlendState(g_MinBlending, BlendFactor, 0xFFFFFFFF);
    context->RSSetState(g_SolidNoCull);
    context->OMSetDepthStencilState(g_NoDepthStencilTest, 0);
}

void HairEffect_Apply_TextureDemux(ID3D11DeviceContext* context)
{
    HairEffect_Apply_Common(context);

    context->VSSetShader(g_Hair_VS_GRID, NULL, 0);
    context->HSSetShader(NULL, NULL, 0);
    context->DSSetShader(NULL, NULL, 0);
    context->GSSetShader(NULL, NULL, 0);
    context->PSSetShader(g_Hair_PS_DEMUX, NULL, 0);
    FLOAT const BlendFactor[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
    context->OMSetBlendState(g_NoBlending, BlendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(g_NoDepthStencilTest, 0);
    context->RSSetState(g_SolidCull);
}

void HairEffect_Apply_VoxelizeObstacles(ID3D11DeviceContext* context)
{
    HairEffect_Apply_Common(context);

    context->VSSetShader(g_Hair_VS_GRID, NULL, 0);
    context->HSSetShader(NULL, NULL, 0);
    context->DSSetShader(NULL, NULL, 0);
    context->GSSetShader(NULL, NULL, 0);
    context->PSSetShader(g_Hair_PS_VOXELIZE_OBSTACLES, NULL, 0);
    FLOAT const BlendFactor[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
    context->OMSetBlendState(g_NoBlending, BlendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(g_NoDepthStencilTest, 0);
    context->RSSetState(g_SolidCull);
}

void HairEffect_Apply_DemuxTo3DFluidObstacles(ID3D11DeviceContext* context)
{
    HairEffect_Apply_Common(context);

    context->VSSetShader(g_Hair_VS_GRID_FLUIDSIM, NULL, 0);
    context->HSSetShader(NULL, NULL, 0);
    context->DSSetShader(NULL, NULL, 0);
    context->GSSetShader(g_Hair_GS_ARRAY, NULL, 0);
    context->PSSetShader(g_Hair_PS_DEMUX_TO_3D_OBSTACLE_TEX, NULL, 0);
    FLOAT const BlendFactor[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
    context->OMSetBlendState(g_NoBlending, BlendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(g_NoDepthStencilTest, 0);
    context->RSSetState(g_SolidCull);
}

struct CB_FLUIDSIM_PER_DRAW
{
    int fluidType;
    float advectAsTemperature;
    float treatAsLiquidVelocity;
    int drawTextureNumber;

    float textureWidth;
    float textureHeight;
    float textureDepth;
    float _Unused_Padding_0;

    // NOTE: The spacing between simulation grid cells is \delta x  = 1, so it is omitted everywhere
    float timestep;
    float decay;                // this is the (1.0 - dissipation_rate). dissipation_rate >= 0 ==> decay <= 1
    float viscosity;            // kinematic viscosity
    float vortConfinementScale; // this is typically a small value >= 0
    DirectX::XMFLOAT3 gravity;  // note this is assumed to be given as pre-multiplied by the timestep, so it's really velocity: cells per step
    float _Unused_Padding_1;

    float radius;
    float _Unused_Padding_2;
    float _Unused_Padding_3;
    float _Unused_Padding_4;
    DirectX::XMFLOAT3 center;
    float _Unused_Padding_5;
    DirectX::XMFLOAT4 color;

    DirectX::XMFLOAT3 obstBoxVelocity;
    float _Unused_Padding_6;
    DirectX::XMFLOAT3 obstBoxLBDcorner;
    float _Unused_Padding_7;
    DirectX::XMFLOAT3 obstBoxRTUcorner;
    float _Unused_Padding_8;
};

static struct CB_FLUIDSIM_PER_DRAW g_cbFluidSimPerDrawData;

void FluidSimEffect_Init(ID3D11Device* device)
{
    g_cbFluidSimPerDrawData.fluidType = FT_SMOKE;
    g_cbFluidSimPerDrawData.advectAsTemperature = -1.0F;
    g_cbFluidSimPerDrawData.treatAsLiquidVelocity = -1.0F;
    g_cbFluidSimPerDrawData.drawTextureNumber = 1;

    g_cbFluidSimPerDrawData.timestep = 1.0F;
    g_cbFluidSimPerDrawData.decay = 1.0F;
    g_cbFluidSimPerDrawData.viscosity = 5e-6F;
    g_cbFluidSimPerDrawData.vortConfinementScale = 0.0F;
    g_cbFluidSimPerDrawData.gravity = DirectX::XMFLOAT3(0.0F, 0.0F, 0.0F);
    g_cbFluidSimPerDrawData.obstBoxVelocity = DirectX::XMFLOAT4(0.0F, 0.0F, 0.0F, 0.0F);

    HRESULT hr;
}

void FluidSimEffect_SetFluidType(int fluidType)
{
    g_cbFluidSimPerDrawData.fluidType = fluidType;
}

void FluidSimEffect_SetDrawTextureNumber(int drawTextureNumber)
{
    g_cbFluidSimPerDrawData.drawTextureNumber = drawTextureNumber;
}

void FluidSimEffect_SetFluidGrid(float textureWidth, float textureHeight, float textureDepth)
{
    g_cbFluidSimPerDrawData.textureWidth = textureWidth;
    g_cbFluidSimPerDrawData.textureHeight = textureHeight;
    g_cbFluidSimPerDrawData.textureDepth = textureDepth;
}

void FluidSimEffect_SetObstBoxcorner(DirectX::XMFLOAT3 const& obstBoxLBDcorner, DirectX::XMFLOAT3 const& obstBoxRTUcorner)
{
    g_cbFluidSimPerDrawData.obstBoxLBDcorner = obstBoxLBDcorner;
    g_cbFluidSimPerDrawData.obstBoxRTUcorner = obstBoxRTUcorner;
}

void FluidSimEffect_SetObstBoxVelocity(DirectX::XMFLOAT3 const& obstBoxVelocity)
{
    g_cbFluidSimPerDrawData.obstBoxVelocity = obstBoxVelocity;
}

void FluidSimEffect_Apply_DrawTexture(ID3D11DeviceContext* context)
{

}