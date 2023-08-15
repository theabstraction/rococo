#include "mplat.api.hlsl"

struct ConeVertex
{
	float4 position : SV_POSITION0;
	float4 colour: COLOR0;
	float2 coneData : TEXCOORD;
};

ConeVertex main(ObjectVertex v)
{
	ConeVertex sv;
	sv.position = Transform_World_To_Screen(v.position);
	sv.coneData.x = v.uv.x;
	sv.coneData.y = v.uv.y;
	sv.colour = v.colour;
	return sv;
}
