#include "ShaderMathUtils.hlsl"
//#include "ConstantBuffers.hlsl"
#include "LightMathUtils.hlsl"
#include "defaultLitStageStructs.hlsl"

//--------------------------------------------------------------------------------------

    Texture2D<float4> tXDiffuse : register( t8 );
    Texture2D<float4> tXNormal  : register( t9 );
    Texture2D<float4> tYDiffuse : register( t10 );
    Texture2D<float4> tYNormal  : register( t11 );
    Texture2D<float4> tZDiffuse : register( t12 );
    Texture2D<float4> tZNormal  : register( t13 );

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
    float4 local_pos        = float4( input.position , 1.0f );                                  // passed in position is usually inferred to be "local position", ie, local to the object
    float4 world_pos        = mul( MODEL , local_pos );                                         // world pos is the object moved to its place int he world by the model, not used yet
    float4 camera_pos       = mul( VIEW , world_pos );                                          
    float4 clip_pos         = mul( PROJECTION , camera_pos );                                   
                                                                                                
   // normal is currently in model/local space                                                  
    float4 local_normal     = float4( input.normal , 0.0f );                                    
    float4 world_normal     = mul( MODEL , local_normal );                                      
                                                                                                
   // tangent & bitangent                                                                       
                                                                                                
    float4 local_tangent    = float4( input.tangent.xyz , 0.0f );                               
    float4 world_tangent    = mul( MODEL , local_tangent );                                     
                                                                                                
    v2f.position            = clip_pos;                                                         // we want to output the clip position to raster (a perspective point)
    v2f.color               = input.color * TINT;
    v2f.uv                  = input.uv;
    v2f.world_position      = world_pos.xyz;
    v2f.world_normal        = world_normal.xyz;
    v2f.world_tangent       = float4( world_tangent.xyz , input.tangent.w );
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
    
                                                                    
    float2      zUV              = frac( input.world_position.xy );                             // frac is used to sample correctly since we are not usinga wrapping/tiling sampler    
//  return      float4( zUV , 0.0f , 1.f );
    float2      xUV              = frac( input.world_position.zy );                             // frac is used to sample correctly since we are not usinga wrapping/tiling sampler
//  return      float4( xUV , 0.0f , 1.f );
    float2      yUV              = frac( input.world_position.xz);                              // frac is used to sample correctly since we are not usinga wrapping/tiling sampler
//  return      float4( yUV , 0.0f , 1.f );
    
    float4      xColor           = tXDiffuse.Sample( sSampler , xUV );
    float4      yColor           = tYDiffuse.Sample( sSampler , yUV );
    float4      zColor           = tZDiffuse.Sample( sSampler , zUV );
    
    float3      xNormal          = tXNormal.Sample( sSampler , xUV ).xyz;
    float3      yNormal          = tYNormal.Sample( sSampler , yUV ).xyz;
    float3      zNormal          = tZNormal.Sample( sSampler , zUV ).xyz;
    
    float3      weights          = normalize( input.world_normal );
                weights          = weights * weights;
                weights          = abs( weights );
    float       sum              = weights.x + weights.y + weights.z;
                weights         /= sum;   
    
    float4  finalColor          = 
                                   weights.x * xColor +
                                   weights.y * yColor +
                                   weights.z * zColor;

                xNormal          = NormalColorToVector3( xNormal );
                yNormal          = NormalColorToVector3( yNormal );
                zNormal          = NormalColorToVector3( zNormal );
    
                zNormal         *= sign( input.world_normal.z );
    
    float3x3 xTBN                = float3x3(
                                                0.0f , 0.0f ,  -1.0f , 
                                                0.0f , 1.0f ,   0.0f , 
                                                1.0f , 0.0f ,   0.0f 
                                    );
    
    xNormal                      = mul( xNormal , xTBN );
    xNormal                     *= sign( input.world_normal.x );
    
    float3x3 yTBN                = float3x3(
                                                1.0f , 0.0f ,  0.0f ,
                                                0.0f , 0.0f , -1.0f ,
                                                0.0f , 1.0f ,  0.0f
                                    );
  
    yNormal                      = mul( yNormal , yTBN );
    yNormal                     *= sign( input.world_normal.y );
    
   // return finalColor;
    
    float3 finalNormal           =
                                   weights.x * xNormal +
                                   weights.y * yNormal +
                                   weights.z * zNormal;
            
           finalNormal           = normalize( finalNormal );
    
           finalColor            = pow( finalColor , GAMMA.xxxx );
    
    PostLightingData result      = ComputeLightingAt( input.world_position , finalNormal , finalColor.xyz , float3( 0.0f.xxx ) , SPECULAR_FACTOR );
    float3 surfaceColor          = result.diffuse;
           surfaceColor          = pow( surfaceColor , INVERSE_GAMMA.xxx );
        
   return float4( surfaceColor , 1.f );
}

//--------------------------------------------------------------------------------------