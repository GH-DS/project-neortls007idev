//#ifndef CONSTANT_BUFFERS
    #include "ConstantBuffers.hlsl"
//#define CONSTANT_BUFFERS
//--------------------------------------------------------------------------------------------------------------------------------------------

float2 ComputeLightFactor( light_t light , float3 worldPosition , float3 worldNormal , float3 directionToEye )
{
    float3      vectorToLight           = light.worldPosition - worldPosition;
        
    float       distanceToLight         = length( vectorToLight );
    float3      directionToLight        = normalize( vectorToLight );
        
    float3      lightDirection          = lerp( -directionToLight , light.direction , light.directionFactor );
    float3      attenuationVector       = float3( 1.0f , distanceToLight , distanceToLight * distanceToLight );
    
    // how much do these two coincide (cone lights)
    float       dotAngle                = dot( -directionToLight , light.direction );
    float coneAttenuation               = smoothstep( light.dotOuterAngle , light.dotInnerAngle , dotAngle);
    
    float       diffuseAttenuation      = light.intensity / dot( attenuationVector , light.attenuation ) * coneAttenuation;
    float       specularAttenuation     = light.intensity / dot( attenuationVector , light.specularAttenuation )* coneAttenuation;

   // specularAttenuation = light.intensity;
    
   // compute diffuse
   // max prevents lighting from the "back", which would subtract light
    float       dotIncident             = dot( -lightDirection , worldNormal );
    float       facingDirection         = smoothstep( -0.4f , 0.1f , dotIncident );
    float       diffuse                 = max( 0.0f , dotIncident );
      
   // BLINN-PHONG LIGHTING COMPUTATION
    float3      hv                      = normalize( -lightDirection + directionToEye );
    float       specular                = max( 0.0f , dot( normalize( worldNormal ) , hv ) );
    
    // SPECULAR LIGHTING
                specular                = facingDirection * specular;   
                specular                = SPECULAR_FACTOR * pow( specular , SPECULAR_POWER );
 
    return float2( diffuseAttenuation * diffuse , specularAttenuation * specular );
}

//--------------------------------------------------------------------------------------------------------------------------------------------

struct PostLightingData
{
    float3 diffuse;
    float3 specularEmmisive;
};

PostLightingData ComputeLightingAt( float3 worldPos , float3 worldNormal , float3 surfaceColor , float3 emmisiveColor , float specularFactor )
{
    float3 directionToEye           = normalize( CAMERA_POSITION - worldPos );
 
    //float3 diffuse                  = AMBIENT.xyz * AMBIENT.w;                                                          // assumes ambient is set from a user - so sRGB space
    float3 diffuse                  = pow( AMBIENT.xyz * AMBIENT.w , GAMMA.xxx );                                       // assumes ambient is set from a user - so sRGB space
    float3 specular                 = float3( 0.0f.xxx );

   // add up contribution of all lights
    for( uint index = 0 ; index < TOTAL_LIGHTS ; index++ )
    {
        float3 lightColor           = LIGHTS[ index ].color.xyz;
               lightColor           = pow( lightColor , GAMMA.xxx );                                                    // assumes light color is set by a user - so sRGB space
        
        float2 lightFactors         = ComputeLightFactor( LIGHTS[ index ] , worldPos , worldNormal , directionToEye );

               diffuse             += lightFactors.x * lightColor;
               specular            += lightFactors.y * lightColor;
    }
   
   // limit it
    diffuse                         = min( DIFFUSE_FACTOR * diffuse , float3( 1.f.xxx ) );
    specular                       *= specularFactor;                                                                   // scale back specular based on spec factor

   // returns light color (in linear space)
    PostLightingData result;
    result.specularEmmisive = specular + emmisiveColor;
    result.diffuse = diffuse * surfaceColor + result.specularEmmisive;
    return result;
}