//--------------------------------------------------------------------------------------
// Stream Input
// ------
// Stream Input is input that is walked by the vertex shader.
// If you say "Draw(3,0)", you are telling to the GPU to expect '3' sets, or
// elements, of input data.  IE, 3 vertices.  Each call of the VertxShader
// we be processing a different element.
//--------------------------------------------------------------------------------------

// inputs are made up of internal names (ie: uv) and semantic names
// (ie: TEXCOORD).  "uv" would be used in the shader file, where
// "TEXCOORD" is used from the client-side (cpp code) to attach ot.
// The semantic and internal names can be whatever you want,
// but know that semantics starting with SV_* usually denote special
// inputs/outputs, so probably best to avoid that naming.

//
// Grid shader
//

//Default precision qualifier
//min16float float;

//This represents the current texture on the mesh
sampler2D tex;

//The interpolated vertex color for this fragment
//min16uint float4 vColor;

//The interpolated texture coordinate for this fragment
//min16float float2 vTexCoord;


struct vs_input_t
{
   // we are not defining our own input data;
	// Name : SEMANTIC NAME  - SEMANTIC NAME NECESSARY to bind it with the input

   float3 position      : POSITION;
   float4 color         : COLOR;
   float2 uv            : TEXCOORD;
};

//----------------------------------------------------------------------------------------------------------------------------------------------------

float RangeMap( float val , float inMin , float inMax , float outMin , float outMax )
{
	float domain = inMax - inMin;
	float range = outMax - outMin;
	return ( ( val - inMin ) / domain ) * range + outMin;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
// constant built into the shader
static float SHIFT = 5.0f;
//

cbuffer time_constants : register( b0 ) // index 0 is now time
{
	float SYSTEM_TIME_SECONDS;
	float SYSTEM_TIME_DELTA_SECONDS;
}

cbuffer camera_constants : register( b1 ) // index 1 is now camera
{
	float4x4 CAMERA_TO_CLIP_TRANSFORM; // PROJECTION MATRIX
	float4x4 VIEW;
}

cbuffer model_constants : register( b2 ) // index 2 is now model
{
	float4x4 MODEL;
}
// Texture & Samplers are also a form of constants

Texture2D <float4> tDiffuse : register( t0 );			// Color of surface
SamplerState eSampler : register( s0 );					// Sampler are rules on hoe to sample

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
};

//--------------------------------------------------------------------------------------
// Vertex Shader
v2f_t VertexFunction( )
{
   v2f_t v2f = (v2f_t)0;

   // forward vertex input onto the next stage
   // v2f.position = float4( input.position, 1.0f );
   // v2f.color = input.color;
   // v2f.uv = input.uv;

   v2f.position = float4( 0.f, 0.f , 0.f, 1.0f );
   v2f.color = float4 ( 1.f, 1.f , 1.f , 1.f );
   v2f.uv = float2( 0.5f , 0.5f );

   float4 worldPos = mul( MODEL , v2f.position );
   float4 cameraPos = mul( VIEW , worldPos );
   float4 clipPos	= mul( CAMERA_TO_CLIP_TRANSFORM , cameraPos );

   v2f.position = clipPos;
   return v2f;
}

//--------------------------------------------------------------------------------------
// Fragment Shader
//
// SV_Target0 at the end means the float4 being returned
// is being drawn to the first bound color target.
float4 FragmentFunction( v2f_t input ) : SV_Target0
{
	// we'll outoupt our UV coordinates as color here
	// to make sure they're being passed correctly.
	// Very common rendering debugging method is to
	// use color to portray information;

	//texture2D mytex;
	/*min16uint*/ //float4 col = mytex.Sample( eSampler , input.uv , 0 ) * input.color;

	float x , y;
	
	x = frac( input.uv.x * 25.0 );
	y = frac( input.uv.y * 25.0 );

	float4 monotone;

	// Draw a black and white grid.
	if ( x > 0.9 || y > 0.9 )
	{
		monotone = float4( 1 , 1 , 1 , 1 );
	}
	else
	{
		monotone = float4( 0 , 0 , 0 , 0 );
	}

	float4 color = tDiffuse.Sample( eSampler , float2( x , y ) );
	return color * monotone ;
}



void main()
{
	//Sample the texture at the interpolated coordinate

}