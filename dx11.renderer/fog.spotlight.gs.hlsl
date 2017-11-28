struct ParticleVertex
{
	float3 position : POSITION;
	float4 colour: COLOR;
	float4 geometry: TEXCOORD;
};

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float3 cameraSpacePosition: TEXCOORD3;
	float4 shadowPosition: TEXCOORD2;
	float3 worldPosition: TEXCOORD1;
	float2 uv: TEXCOORD0;
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

cbuffer light: register(b0)
{
	Light light;
};

cbuffer camera: register(b1)
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	float4 eye;
	float4 viewDir;
	float4 aspect;
};

void EmitPointEx(float4 p, ParticleVertex source, float4 cameraSpacePos, float4 shadowPos, float u, float v, inout TriangleStream<PixelVertex> output)
{
	PixelVertex outputVert;
	outputVert.position = p;
	outputVert.uv.x = u;
	outputVert.uv.y = v;
	outputVert.colour = source.colour;
	outputVert.shadowPosition = shadowPos;
	outputVert.cameraSpacePosition = cameraSpacePos.xyz;
	outputVert.worldPosition = source.position,
	output.Append(outputVert);
}

[maxvertexcount(6)]
void main (point ParticleVertex p[1], inout TriangleStream<PixelVertex> output)
{
	float scale = p[0].geometry.x;

	float4 worldPosition = float4(p[0].position, 1.0f);

	float4 preScreenTransformPosition = mul(worldMatrixAndProj, worldPosition);

	float4 cameraSpacePosition = mul(worldMatrix, worldPosition);

	float4 shadowPos = mul(light.worldToShadowBuffer, worldPosition);

	float s = scale;
	float4 right = float4(s * aspect.x, 0, 0, 0);
	float4 up = float4(0, s, 0, 0);

	float4 billboard[4];
	billboard[0] = preScreenTransformPosition - right - up;
	billboard[1] = preScreenTransformPosition + right - up;
	billboard[2] = preScreenTransformPosition + right + up;
	billboard[3] = preScreenTransformPosition - right + up;

	EmitPointEx(billboard[0], p[0], cameraSpacePosition, shadowPos, -1, -1, output);
	EmitPointEx(billboard[1], p[0], cameraSpacePosition, shadowPos,  1, -1, output);
	EmitPointEx(billboard[2], p[0], cameraSpacePosition, shadowPos,  1,  1, output);
	EmitPointEx(billboard[2], p[0], cameraSpacePosition, shadowPos,  1,  1, output);
	EmitPointEx(billboard[3], p[0], cameraSpacePosition, shadowPos, -1,  1, output);
	EmitPointEx(billboard[0], p[0], cameraSpacePosition, shadowPos, -1, -1, output);
}