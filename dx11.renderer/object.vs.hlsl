struct ObjectVertex
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
	float2 materialIndexAndGloss : TEXCOORD1;
};

struct ScreenVertex
{
	float4 position : SV_POSITION0;
	float4 uv_material_and_gloss: TEXCOORD;
	float4 worldPosition: TEXCOORD1;
	float4 normal : TEXCOORD2;
	float4 shadowPos: TEXCOORD3;
	float4 cameraSpacePosition: TEXCOORD4;
	float4 colour: COLOR0;
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

cbuffer globalState: register(b0)
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	float4 eye;
};

cbuffer perInstanceData: register(b1)
{
	float4x4 instanceMatrix;
	float4 highlightColour;
}

cbuffer light: register(b2)
{
	Light light;
};

ScreenVertex main(ObjectVertex v)
{
	ScreenVertex sv;



	float4 instancePos = mul(instanceMatrix, float4(v.position,1.0f));
	sv.position = mul(worldMatrixAndProj, instancePos);
	sv.normal = mul(instanceMatrix, float4(v.normal.xyz,0.0f));
	sv.worldPosition = instancePos;
	sv.shadowPos = mul(light.worldToShadowBuffer, instancePos);
	sv.cameraSpacePosition = mul(worldMatrix, instancePos);
	sv.uv_material_and_gloss.xy = v.uv.xy;
	sv.uv_material_and_gloss.z = v.materialIndexAndGloss.x;
	sv.uv_material_and_gloss.w = v.materialIndexAndGloss.y;
	sv.colour = v.colour;
	return sv;
}
