#include <mplat.api.hlsl>

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float2 uv: TEXCOORD0;
	float4 colour: COLOR0;
};

void EmitPoint(float3 p, inout TriangleStream<PixelVertex> output)
{
	PixelVertex outputVert;
	outputVert.position = Project_World_To_Screen(float4(p, 1.0f));
	output.Append(outputVert);
}

void EmitPointEx(float4 p, float4 colour, float u, float v, inout TriangleStream<PixelVertex> output)
{
	PixelVertex outputVert;
	outputVert.position = p;
	outputVert.uv.x = u;
	outputVert.uv.y = v;
	outputVert.colour = colour;
	output.Append(outputVert);
}

[maxvertexcount(6)]
void main (point ParticleVertex p[1], inout TriangleStream<PixelVertex> output)
{
	float scale = p[0].geometry.x;

	float4 pos = Project_World_To_Screen(float4(p[0].position, 1.0f));

	float s = scale;
	float4 right = float4(s * global.aspect.x, 0, 0, 0);
	float4 up = float4(0, s, 0, 0);

	float4 billboard[4];
	billboard[0] = pos - right - up;
	billboard[1] = pos + right - up;
	billboard[2] = pos + right + up;
	billboard[3] = pos - right + up;

	EmitPointEx(billboard[0], p[0].colour, -1, -1, output);
	EmitPointEx(billboard[1], p[0].colour,  1, -1, output);
	EmitPointEx(billboard[3], p[0].colour, -1,  1, output);
	EmitPointEx(billboard[2], p[0].colour,  1,  1, output);
}