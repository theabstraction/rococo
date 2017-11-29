#include <mplat.api.hlsl>

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float3 cameraSpacePosition: TEXCOORD1;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
};

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