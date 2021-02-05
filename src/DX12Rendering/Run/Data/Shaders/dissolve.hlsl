#include "ShaderMathUtils.hlsl"
#include "LightMathUtils.hlsl"
#include "defaultLitStageStructs.hlsl"

//--------------------------------------------------------------------------------------

cbuffer material_constants : register( b8 )                                                     // constant buffer slot 9
{                                                                                               
    float3 burnStartColor;
    float  burnEdgeWidth;
    
    float3 burnEndColor;
    float  burnAmount;
};

Texture2D<float4> tDissolvePattern : register( t8 );

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
            
//--------------------------------------------------------------------------------------
//              SAMPLE THE TEXTURES
//--------------------------------------------------------------------------------------    
    
    float4 diffuseColor            = tDiffuse.Sample( sSampler , input.uv );
    float4 normalColor             = tNormal.Sample( sSampler , input.uv );    
    float burnValue                = tDissolvePattern.Sample( sSampler , input.uv ).x;
    
    float burnMin                  = lerp( -burnEdgeWidth , 1.0f , burnAmount );
    float burnMax                  = burnMin + burnEdgeWidth;
    
   // if( burnValue < burnMin )
   // {
   //     discard;                                                                                // like an exit - short circuits the pixel completely
   // }

    clip( burnValue - burnMin );
    
    float  burnMix                 = smoothstep( burnMin , burnMax , burnValue );
    float3 burnColor               = lerp( burnStartColor , burnEndColor , burnMix );

    float3 tangent                 = normalize( input.world_tangent.xyz );
    float3 normal                  = normalize( input.world_normal );    
    float3 bitangent               = normalize( cross( normal , tangent ) ) * input.world_tangent.w;
    float3x3 TBN                   = float3x3( tangent, bitangent, normal );

    float3 surfaceColor            = pow( diffuseColor.xyz , GAMMA.xxx );
    
          surfaceColor             = surfaceColor * input.color.xyz;
   // float3 surfaceColor             = input.color.xyz;
    float alpha                    = diffuseColor.w * input.color.w;

    float3 surfaceNormal           = NormalColorToVector3( normalColor.xyz );
    float3 worldNormal             = mul( surfaceNormal , TBN );

    PostLightingData lightResult    = ComputeLightingAt( input.world_position , worldNormal , surfaceColor , float3( 0.0f.xxx ) , SPECULAR_FACTOR );
    
    surfaceColor                    = lightResult.diffuse;

   // compute final color; 
    float3 finalColor              = pow( surfaceColor.xyz , INVERSE_GAMMA.xxx );
           finalColor              = lerp( burnColor , finalColor , burnMix );
    //float3    finalColor              = lerp( burnColor , surfaceColor , burnMix );
  //            finalColor              = lerp( burnColor , finalColor , burnMix );

    return float4( finalColor , alpha );
}

//--------------------------------------------------------------------------------------