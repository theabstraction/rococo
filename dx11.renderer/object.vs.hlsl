struct ObjectVertex
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 emissiveColour: COLOR0;
	float4 diffuseColour: COLOR1;
};

struct ScreenVertex
{
	float4 position : SV_POSITION;
	float4 normal : NORMAL;
	float4 emissiveColour: COLOR0;
	float4 diffuseColour: COLOR1;
};

cbuffer globalState
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
};

cbuffer perInstanceData
{
	float4x4 instanceMatrix;
	float4 highlightColour;
}

ScreenVertex main(ObjectVertex v)
{
	ScreenVertex sv;

	float4 instancePos = mul(v.position, instanceMatrix);
	sv.position = mul(instancePos, worldMatrixAndProj);
	sv.normal = mul(v.normal, worldMatrix);
	sv.emissiveColour = highlightColour.w * (highlightColour + v.emissiveColour) + (1.0f - highlightColour.w) * v.emissiveColour;
	sv.diffuseColour = v.diffuseColour;

	return sv;
}