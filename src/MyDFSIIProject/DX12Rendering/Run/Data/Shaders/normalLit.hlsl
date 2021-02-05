#include "ShaderMathUtils.hlsl"
#include "ConstantBuffers.hlsl"

//--------------------------------------------------------------------------------------
// Stream Input
// ------
// Stream Input is input that is walked by the vertex shader.  
// If you say "Draw(3,0)", you are telling to the GPU to expect '3' sets, or 
// elements, of input data.  IE, 3 vertices.  Each call of the VertxShader
// we be processing a different element. 
//--------------------------------------------------------------------------------------

// Input from Vertex Buffers
struct vs_input_t
{
   // we are not defining our own input data; 
    float3 position     : POSITION;
    float4 color        : COLOR;
    float2 uv           : TEXCOORD;

    float3 normal       : NORMAL;
};

//--------------------------------------------------------------------------------------
// Programmable Shader Stages
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// for passing data from vertex to fragment (v-2-f)
struct v2f_t
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;

    float3 world_position : WORLD_POSITION;
    float3 world_normal : WORLD_NORMAL;
};

//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
//                      PROGRAMMABLE SHADER STAGES FUNCTIONS
//--------------------------------------------------------------------------------------
//
// VERTEX SHADER
//
//--------------------------------------------------------------------------------------

v2f_t VertexFunction(vs_input_t input)
{
    v2f_t v2f = (v2f_t) 0;

   // move the vertex through the spaces
    float4 local_pos = float4( input.position , 1.0f ); // passed in position is usually inferred to be "local position", ie, local to the object
    float4 world_pos = mul( MODEL , local_pos ); // world pos is the object moved to its place int he world by the model, not used yet
    float4 camera_pos = mul( VIEW , world_pos );
    float4 clip_pos = mul( PROJECTION , camera_pos );

   // normal is currently in model/local space
    float4 local_normal = float4( input.normal , 0.0f );
    float4 world_normal = mul( MODEL , local_normal );
    
   // tangent & bitangent

    v2f.position        = clip_pos; // we want to output the clip position to raster (a perspective point)
    v2f.color           = input.color * TINT;
    v2f.uv              = input.uv;
    v2f.world_position  = world_pos.xyz;
    v2f.world_normal    = world_normal.xyz;

    return v2f;
}

//--------------------------------------------------------------------------------------
//
// FRAGMENT SHADER
//
//--------------------------------------------------------------------------------------
// SV_Target0 at the end means the float4 being returned
// is being drawn to the first bound color target.
//--------------------------------------------------------------------------------------

float4 FragmentFunction(v2f_t input) : SV_Target0
{
    float3 normal       = normalize( input.world_normal );
    float4 normalColor  = ConvertNormalizedVector3ToColor( normal );
    return normalColor;
}

//--------------------------------------------------------------------------------------