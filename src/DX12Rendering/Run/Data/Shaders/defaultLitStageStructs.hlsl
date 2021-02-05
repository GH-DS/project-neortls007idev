//--------------------------------------------------------------------------------------
// Stream Input
// ------
// Stream Input is input that is walked by the vertex shader.  
// If you say "Draw(3,0)", you are telling to the GPU to expect '3' sets, or 
// elements, of input data.  IE, 3 vertices.  Each call of the VertxShader
// we be processing a different element. 
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
//                      PROGRAMMABLE SHADER STAGES STRUCTS
//--------------------------------------------------------------------------------------
//
// INPUT FROM VERTEX BUFFERS
//
//--------------------------------------------------------------------------------------
struct vs_input_t
{
   // we are not defining our own input data; 
    float3 position         : POSITION;
    float4 color            : COLOR;
    float2 uv               : TEXCOORD;

    float3 normal           : NORMAL;
    float4 tangent          : TANGENT;
};


//--------------------------------------------------------------------------------------
//
// INPUT FOR PASSING DATA FROM VERTEX STAGE TO FRAGMENT STAGE (V-2-F)
// 
//--------------------------------------------------------------------------------------

struct v2f_t
{
    float4 position         : SV_POSITION;
    float4 color            : COLOR;
    float2 uv               : UV;

    float3 world_position   : WORLD_POSITION;
    float3 world_normal     : WORLD_NORMAL;
    float4 world_tangent    : WORLD_TANGENT;
};

//--------------------------------------------------------------------------------------