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
	float4 position : SV_POSITION0;
	float4 normal : TEXCOORD2;
	float2 uv: TEXCOORD;
	float4 worldPosition: TEXCOORD1;
	float4 shadowPos: TEXCOORD3;
};

#pragma pack_matrix(row_major)

struct Light
{
	float4x4 worldToShadowBuffer;
	float4 position;
	float4 direction;
	float4 right;
	float4 up;
	float4 colour;
	float4 ambient;
	float4 randoms; // 4 random quotients 0.0 - 1.0
	float cosHalfFov;
	float fov;
	float nearPlane;
	float farPlane;
	float time; // Can be used for animation 0 - ~59.99, cycles every minute
	float cutoffCosAngle; // What angle to trigger cutoff of light
	float cutoffPower; // Exponent of cutoff rate. Range 1 to 64 is cool
	float attenuationRate; // Point lights vary as inverse square, so 0.5 ish
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

cbuffer light
{
	Light light;
};

ScreenVertex main(ObjectVertex v)
{
	ScreenVertex sv;

	float4 instancePos = mul(instanceMatrix, v.position);
	sv.position = mul(worldMatrixAndProj, instancePos);
	sv.normal = mul(instanceMatrix, v.normal);
	sv.worldPosition = instancePos;
	sv.shadowPos = mul(light.worldToShadowBuffer, instancePos);
	sv.uv = v.uv;
	return sv;
}