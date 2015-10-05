struct ObjectVertex
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 emissiveColour: COLOR0;
	float4 diffuseColour: COLOR1;
	float4 row0: TEXCOORD0;
	float4 row1: TEXCOORD1;
	float4 row2: TEXCOORD2;
	float4 row3: TEXCOORD3;
};

struct ScreenVertex
{
	float4 position : SV_POSITION;
	float4 normal : NORMAL;
	float4 emissiveColour: COLOR0;
	float4 diffuseColour: COLOR1;
};

float4x4 worldMatrix;

ScreenVertex main(ObjectVertex v)
{
	ScreenVertex sv;

	v.position.w = 1.0f;
	v.normal.w = 0.0f;

	float4x4 instanceMatrix = float4x4(v.row0, v.row1, v.row2, v.row3);
	float4 instancePos = mul(instanceMatrix, v.position);
	sv.position = mul(worldMatrix, instancePos);
	sv.normal = mul(worldMatrix, v.normal);
	sv.emissiveColour = v.emissiveColour;
	sv.diffuseColour = v.diffuseColour;

	return sv;
}