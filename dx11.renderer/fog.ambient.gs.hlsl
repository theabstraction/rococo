struct ParticleVertex
{
	float3 position : POSITION;
	float4 colour: COLOR;
	float4 geometry: TEXCOORD;
};

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float3 cameraSpacePosition: TEXCOORD1;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
};

#pragma pack_matrix(row_major)

struct AmbientData
{
	float4 localLight;
	float fogConstant; // light = e(Rk). Where R is distance. k = -0.2218 gives modulation of 1/256 at 25 metres, reducing full brightness to dark
	float a;
	float b;
	float c;
	float4 eye;
};

struct GuiScale
{
	float OOScreenWidth;
	float OOScreenHeight;
	float OOFontWidth;
	float OOSpriteWidth;
};

cbuffer GlobalState: register(b0)
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	GuiScale guiScale;
	float4 eye;
	float4 viewDir;
	float4 aspect;
}

cbuffer AmbienceState : register(b2)
{
	AmbientData ambience;
}

void EmitPointEx(float4 p, ParticleVertex source, float4 cameraSpacePos, float u, float v, inout TriangleStream<PixelVertex> output)
{
	PixelVertex outputVert;
	outputVert.position = p;
	outputVert.uv.x = u;
	outputVert.uv.y = v;
	outputVert.colour = source.colour;
	outputVert.cameraSpacePosition = cameraSpacePos.xyz;
	output.Append(outputVert);
}

[maxvertexcount(6)]
void main (point ParticleVertex p[1], inout TriangleStream<PixelVertex> output)
{
	float scale = p[0].geometry.x;

	float4 worldPosition = float4(p[0].position, 1.0f);

	float4 preScreenTransformPosition = mul(worldMatrixAndProj, worldPosition);

	float4 cameraSpacePosition = mul(worldMatrix, worldPosition);

	float s = scale;
	float4 right = float4(s * aspect.x, 0, 0, 0);
	float4 up = float4(0, s, 0, 0);

	float4 billboard[4];
	billboard[0] = preScreenTransformPosition - right - up;
	billboard[1] = preScreenTransformPosition + right - up;
	billboard[2] = preScreenTransformPosition + right + up;
	billboard[3] = preScreenTransformPosition - right + up;

	EmitPointEx(billboard[0], p[0], cameraSpacePosition, -1, -1, output);
	EmitPointEx(billboard[1], p[0], cameraSpacePosition,  1, -1, output);
	EmitPointEx(billboard[3], p[0], cameraSpacePosition, -1,  1, output);
	EmitPointEx(billboard[2], p[0], cameraSpacePosition,  1,  1, output);
}