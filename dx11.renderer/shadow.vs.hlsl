#include <mplat.api.hlsl>

struct ScreenVertex
{
	float4 position : SV_POSITION;
};

ScreenVertex main(ObjectVertex v)
{
	float4 instancePos = Transform_Instance_To_World(v.position);

	ScreenVertex sv;
	sv.position = Transform_World_To_DepthBuffer(instancePos);
	return sv;
}