#include "Common.hlsl"

// #DXR Extra - Another ray type
struct ShadowHitInfo
{ bool isHit;
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

[shader("closesthit")] void ClosestHit(inout HitInfo payload,
                                       Attributes attrib) {
  float3 barycentrics =
      float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

  uint vertId = 3 * PrimitiveIndex();
  float3 hitColor = BTriVertex[vertId + 0].color * barycentrics.x +
                    BTriVertex[vertId + 1].color * barycentrics.y +
                    BTriVertex[vertId + 2].color * barycentrics.z;

	payload.colorAndDistance = float4(hitColor, RayTCurrent());
//  payload.colorAndDistance = float4( 1.f , 0.f , 0.f , RayTCurrent() );
}
