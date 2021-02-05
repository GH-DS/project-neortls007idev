#include "ConstantBuffers.hlsl"
// input to the vertex shader - for now, a special input that is the index of the vertex we're drawing

//--------------------------------------------------------------------------------------
// constants
//--------------------------------------------------------------------------------------
// The term 'static' refers to this an built into the shader, and not coming
// from a contsant buffer - which we'll get into later (if you remove static, you'll notice
// this stops working).

static float3 POSITIONS[ 3 ] =
{
   float3( -1.0f , -1.0f , 0.0f ) ,
   float3(  3.0f , -1.0f , 0.0f ) ,
   float3( -1.0f ,  3.0f , 0.0f )
};

//--------------------------------------------------------------------------------------

static float2 UVS[ 3 ] =
{
   float2( 0.0f ,  1.0f ) ,
   float2( 2.0f ,  1.0f ) ,
   float2( 0.0f , -1.0f )
};

//--------------------------------------------------------------------------------------

struct vs_input_t
{
   uint vidx : SV_VERTEXID;                                 // SV_* stands for System Variable (ie, built-in by D3D11 and has special meaning)
                                                            // in this case, is the the index of the vertex coming in.
};

//--------------------------------------------------------------------------------------

struct VertexToFragment_t
{
   // SV_POSITION is a semantic - or a name identifying this variable.
   // Usually a semantic can be any name we want - but the SV_* family
   // usually denote special meaning (SV = System Variable I believe)
   // SV_POSITION denotes that this is output in clip space, and will be
   // use for rasterization.  When it is the input (pixel shader stage), it will
   // actually hold the pixel coordinates.

   float4 position : SV_POSITION;
   float2 uv : UV;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

VertexToFragment_t VertexFunction( vs_input_t input )
{
   VertexToFragment_t v2f = (VertexToFragment_t)0;

   // The output of a vertex shader is in clip-space, which is a 4D vector
   // so we need to convert out input to a 4D vector.
    
   v2f.position = float4( POSITIONS[input.vidx], 1.0f );
   v2f.uv = UVS[input.vidx];

   // And return - this will pass it on to the next stage in the pipeline;
    
   return v2f;
}

Texture2D<float4> blurDiffuse : register( t8 );
Texture2D<float4> colorDiffuse : register( t9 );

float4 FragmentFunction( VertexToFragment_t input ) : SV_Target0
{

//--------------------------------------------------------------------------------------
//              SAMPLE THE TEXTURES
//--------------------------------------------------------------------------------------    
  
    float4 imageColor   = colorDiffuse.Sample( sSampler , input.uv );
    float4 blurColor    = blurDiffuse.Sample( sSampler , input.uv );
    
    //if( blurColor.a == 0.0 )
    //{
    //    return imageColor;
    //}
    
    float4 finalColor;
    //finalColor = lerp( blurColor , imageColor , imageColor.a - blurColor.a );
    //finalColor = blurColor * ( blurColor.a ) + imageColor * imageColor.a;
    finalColor = blurColor + imageColor;
    //finalColor = blurColor * ( 1 - imageColor.a ) + imageColor * imageColor.a;
    //return float4( blurColor * ( 1 - imageColor.a ) );
    //return float4( ( 1 - imageColor.a ).xxx , 1 );
    //return float4( ( 1 - imageColor.a ).xxx , 1 );
    return finalColor;
}

//--------------------------------------------------------------------------------------