struct ObjectVertex
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 emissiveColour: COLOR0;
	float4 diffuseColour: COLOR1;
	float2 uv: TEXCOORD;
};

struct ScreenVertex
{
	float4 position : SV_POSITION;
	float4 emissiveColour: COLOR0;
	float4 diffuseColour: COLOR1;
	float2 uv: TEXCOORD;
};

cbuffer globalState
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	float4 sunlightDirection;
};

cbuffer perInstanceData
{
	float4x4 instanceMatrix;
	float4 highlightColour;
}

ScreenVertex main(ObjectVertex v)
{
	ScreenVertex sv;

	float4 instancePos = mul(instanceMatrix, v.position);
	sv.position = mul(worldMatrixAndProj, instancePos);

	v.normal.w = 0;
	float4 normal = mul(instanceMatrix, v.normal);
	float f = -dot(normal.xyz, sunlightDirection.xyz);
	f = clamp(f, 0.0f, 1.0f);
	float illumination = 0.5f + 0.5f * f;
	sv.emissiveColour = highlightColour.w * (highlightColour + illumination * v.emissiveColour) + (1.0f - highlightColour.w) * illumination * v.emissiveColour;
	sv.diffuseColour = v.diffuseColour;
	
	sv.uv = v.uv;

	return sv;
}