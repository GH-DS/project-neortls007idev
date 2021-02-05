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

cbuffer material_constants : register( b8 ) // constant buffer slot 9
{
    float4x4 toneMapMatrix;
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

float4 ToneMap( float4 color )
{
    float4 toneMappedColor;
    toneMappedColor.r = toneMapMatrix[ 0 ][ 0 ] * color.r + toneMapMatrix[ 1 ][ 0 ] * color.g + toneMapMatrix[ 2 ][ 0 ] * color.b;
    toneMappedColor.g = toneMapMatrix[ 0 ][ 1 ] * color.r + toneMapMatrix[ 1 ][ 1 ] * color.g + toneMapMatrix[ 2 ][ 1 ] * color.b;
    toneMappedColor.b = toneMapMatrix[ 0 ][ 2 ] * color.r + toneMapMatrix[ 1 ][ 2 ] * color.g + toneMapMatrix[ 2 ][ 2 ] * color.b;
    toneMappedColor.a = toneMapMatrix[ 3 ][ 3 ];
    return toneMappedColor;
}


float4 FragmentFunction( VertexToFragment_t input ) : SV_Target0
{

//--------------------------------------------------------------------------------------
//              SAMPLE THE TEXTURES
//--------------------------------------------------------------------------------------    
  
    float4 imageColor   = tDiffuse.Sample( sSampler , input.uv );
            
    float4 finalColor   = mul( toneMapMatrix , imageColor );
           //finalColor.a = imageColor.a; 
    //float lum  = 0.2126f *  imageColor.r + 0.7152f * imageColor.g + 0.0722f * imageColor.b;
    //return float4( lum , lum , lum , imageColor.a );
    finalColor = ToneMap( imageColor );
    float4 imageFactor = float4( imageColor.xyz * ( 1 - finalColor.a ) , ( 1 - finalColor.a ) );
    finalColor = float4( finalColor.xyz * finalColor.a , finalColor.a ) + imageFactor;
    return finalColor;
}

//--------------------------------------------------------------------------------------