#include "Common.hlsl"

// #DXR Extra - Another ray type
struct ShadowHitInfo
{
	bool isHit;
};

// #DXR Extra - Another ray type
// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t2);

struct STriVertex {
  float3 vertex;
  float4 color;
  float2 uvs;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
// #DXR Extra - Another ray type
// Raytracing acceleration structure, accessed as a SRV

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
  float3 barycentrics =
      float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

  uint vertId = 3 * PrimitiveIndex();
  
  float3 vert0Color = float3( 1.f , 0.f , 0.f );
  float3 vert1Color = float3( 0.f , 1.f , 0.f );
  float3 vert2Color = float3( 0.f , 0.f , 1.f );
    
    float3 hitColor =   vert0Color * barycentrics.x +
                        vert1Color * barycentrics.y +
                        vert2Color * barycentrics.z;

  payload.colorAndDistance = float4(hitColor, RayTCurrent());
 // payload.colorAndDistance = float4( 0.f , 1.f , 0.f , RayTCurrent() );
}

// #DXR Extra - Another ray type
[shader( "closesthit" )]
void PlaneClosestHit( inout HitInfo payload , Attributes attrib )
{
    float3 lightPos = float3( 2 , 2 , -2 );

  // Find the world - space hit position
    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

    float3 lightDir = normalize( lightPos - worldOrigin );

  // Fire a shadow ray. The direction is hard-coded here, but can be fetched
  // from a constant-buffer
    RayDesc ray;
    ray.Origin = worldOrigin;
    ray.Direction = lightDir;
    ray.TMin = 0.01;
    ray.TMax = 100000;
    bool hit = true;

  // Initialize the ray payload
    ShadowHitInfo shadowPayload;
    shadowPayload.isHit = false;

  // Trace the ray
    TraceRay(
      // Acceleration structure
      SceneBVH ,
      // Flags can be used to specify the behavior upon hitting a surface
      RAY_FLAG_NONE ,
      // Instance inclusion mask, which can be used to mask out some geometry to
      // this ray by and-ing the mask with a geometry mask. The 0xFF flag then
      // indicates no geometry will be masked
      0xFF ,
      // Depending on the type of ray, a given object can have several hit
      // groups attached (ie. what to do when hitting to compute regular
      // shading, and what to do when hitting to compute shadows). Those hit
      // groups are specified sequentially in the SBT, so the value below
      // indicates which offset (on 4 bits) to apply to the hit groups for this
      // ray. In this sample we only have one hit group per object, hence an
      // offset of 0.
      1 ,
      // The offsets in the SBT can be computed from the object ID, its instance
      // ID, but also simply by the order the objects have been pushed in the
      // acceleration structure. This allows the application to group shaders in
      // the SBT in the same order as they are added in the AS, in which case
      // the value below represents the stride (4 bits representing the number
      // of hit groups) between two consecutive objects.
      0 ,
      // Index of the miss shader to use in case several consecutive miss
      // shaders are present in the SBT. This allows to change the behavior of
      // the program when no geometry have been hit, for example one to return a
      // sky color for regular rendering, and another returning a full
      // visibility value for shadow rays. This sample has only one miss shader,
      // hence an index 0
      1 ,
      // Ray information to trace
      ray ,
      // Payload associated to the ray, which will be used to communicate
      // between the hit/miss shaders and the raygen
      shadowPayload );

    float factor = shadowPayload.isHit ? 0.3 : 1.0;

    float3 barycentrics =
      float3( 1.f - attrib.bary.x - attrib.bary.y , attrib.bary.x , attrib.bary.y );
    float4 hitColor = float4( float3( 0.7 , 0.7 , 0.3 ) * factor , RayTCurrent() );
   // float4 hitColor = float4( 1.f , 0.f , 1.f , 1.f );
    payload.colorAndDistance = float4( hitColor );
}