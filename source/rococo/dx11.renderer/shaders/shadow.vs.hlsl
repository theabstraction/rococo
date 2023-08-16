#include <mplat.api.hlsl>

struct ShadowOutVertex
{
	float4 position : SV_POSITION;
};

ShadowOutVertex main(ObjectVertex v)
{
	float4 instancePos = Transform_Instance_To_World(float4(v.position.xyz, 1.0f));

	ShadowOutVertex sv;
	sv.position = Transform_World_To_DepthBuffer(instancePos);
	return sv;
}