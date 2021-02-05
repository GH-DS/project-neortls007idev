
//--------------------------------------------------------------------------------------------------------------------------------------------

float RangeMap( float val , float inMin , float inMax , float outMin , float outMax )
{
	float domain = inMax - inMin;
	float range = outMax - outMin;
	return ( ( val - inMin ) / domain ) * range + outMin;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

float4 ConvertNormalizedVector3ToColor( float3 vec3 )
{
    float4 color;
    color.x = RangeMap( vec3.x , -1.f , 1.f , 0.f , 1.f );
    color.y = RangeMap( vec3.y , -1.f , 1.f , 0.f , 1.f );
    color.z = RangeMap( vec3.z , -1.f , 1.f , 0.f , 1.f );
    color.w = 1;
    return color;
}

//--------------------------------------------------------------------------------------------------------------------------------------------

float3 NormalColorToVector3( float3 color )
{
    return normalize( color * float3( 2.0f , 2.0f , 1.0f ) - float3( 1.0f , 1.0f , 0.0f ) );
}

//--------------------------------------------------------------------------------------------------------------------------------------------
