#include <mplat.api.hlsl>

struct ScreenVertex
{
	float4 position : SV_POSITION;
};

ScreenVertex main(ObjectVertex v)
{
	float4 instancePos = mul(instanceMatrix, v.position);

	ScreenVertex sv;
	sv.position = mul(drd.worldToScreen, instancePos);
	return sv;
}