#include "ShaderMathUtils.hlsl"
#include "LightMathUtils.hlsl"
#include "defaultLitStageStructs.hlsl"

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
    float4 local_pos        = float4( input.position , 1.0f );                                      // passed in position is usually inferred to be "local position", ie, local to the object
    float4 world_pos        = mul( MODEL , local_pos );                                             // world pos is the object moved to its place int he world by the model, not used yet
    float4 camera_pos       = mul( VIEW , world_pos );                                          
    float4 clip_pos         = mul( PROJECTION , camera_pos );                                   
                                                                                                
   // normal is currently in model/local space                                                  
    float4 local_normal     = float4( input.normal , 0.0f );                                    
    float4 world_normal     = mul( MODEL , local_normal );                                      
                                                                                                
   // tangent & bitangent                                                                       
                                                                                                
    float4 local_tangent    = float4( input.tangent.xyz , 0.0f );                               
    float4 world_tangent    = mul( MODEL , local_tangent );                                     
                                                                                                
    v2f.position            = clip_pos;                                                             // we want to output the clip position to raster (a perspective point)
    v2f.color               = input.color * TINT;
    v2f.uv                  = input.uv;
    v2f.world_position      = world_pos.xyz;
    v2f.world_normal        = world_normal.xyz;
    v2f.world_tangent       = float4( world_tangent.xyz , input.tangent.w );
    return v2f;
}

//--------------------------------------------------------------------------------------
//                  FRAGMENT SHADER
//--------------------------------------------------------------------------------------
// SV_Target0 at the end means the float4 being returned
// is being drawn to the first bound color target.
//--------------------------------------------------------------------------------------

struct fragmentFunctionOutput
{
    float4 color    : SV_Target0;
    float4 bloom    : SV_Target1;
  //  float4 normal   : SV_Target2;
  //  float4 albedo   : SV_Target3;
  //  float4 tangent  : SV_Target4;
};


fragmentFunctionOutput FragmentFunction( v2f_t input )
{

//--------------------------------------------------------------------------------------
//              SAMPLE THE TEXTURES
//--------------------------------------------------------------------------------------    
    float4 diffuseColor            = tDiffuse.Sample( sSampler , input.uv );
    float4 normalColor             = tNormal.Sample( sSampler , input.uv );
//--------------------------------------------------------------------------------------
//              COMPUTE SURFACE COLOR
//--------------------------------------------------------------------------------------
    
    float3 surfaceColor            = diffuseColor.xyz * input.color.xyz;
           surfaceColor            = pow( surfaceColor , GAMMA.xxx );
    float alpha                    = diffuseColor.w * input.color.w;
      
    float3 tangent                 = normalize( input.world_tangent.xyz );
    float3 normal                  = normalize( input.world_normal );
    float3 bitangent               = normalize( cross( normal , tangent ) ) * input.world_tangent.w;
    float3x3 TBN                   = float3x3( tangent , bitangent , normal );
    float3 directionToCamera       = normalize( CAMERA_POSITION - input.world_position ); // As the Camera IS OUR EYE
    
    float3 surfaceNormal           = NormalColorToVector3( normalColor.xyz );
    float3 worldNormal             = mul( surfaceNormal , TBN );
     
//--------------------------------------------------------------------------------------
//              COMPUTE LIGHT FACTOR
//--------------------------------------------------------------------------------------
    PostLightingData lightResult   = ComputeLightingAt( input.world_position , worldNormal , surfaceColor , float3( 0.0f.xxx ) , SPECULAR_FACTOR );
    float3 finalColor              = lightResult.diffuse + lightResult.specularEmmisive;
   
    float3 bloom                    = max( float3( 0.f , 0.f , 0.f ) , finalColor - float3( 1.f , 1.f , 1.f ) );
           bloom                    = pow( bloom.xyz , INVERSE_GAMMA.xxx );
   // gamma correct back, and output
           finalColor              = pow( finalColor.xyz , INVERSE_GAMMA.xxx );

    finalColor                      = ApplyLinearFog( input.world_position , finalColor );
            
   // return float4( finalColor , alpha );
    
    fragmentFunctionOutput output;
    output.color    = float4( finalColor.xyz , alpha );
    output.bloom    = float4( bloom , 1 );
    //output.tangent  = float4( ( tangent     + float3( 1 , 1 , 1 ) ) * .5f , 1);
    //output.normal   = float4( ( worldNormal + float3( 1 , 1 , 1 ) ) * .5f , 1);
    //output.albedo   = diffuseColor;
    
    return output;
}

//--------------------------------------------------------------------------------------