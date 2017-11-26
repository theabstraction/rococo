struct ParticleVertex
{
	float3 position : POSITION;
};

struct PixelVertex
{
	float4 position : SV_POSITION0;
	float2 uv: TEXCOORD0;
};

#pragma pack_matrix(row_major)

cbuffer camera: register(b0)
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	float4 eye;
	float4 viewDir;
	float4 aspect;
};

void EmitPoint(float3 p, inout TriangleStream<PixelVertex> output)
{
	PixelVertex outputVert;
	outputVert.position = mul(worldMatrixAndProj, float4(p, 1.0f));
	output.Append(outputVert);
}

void EmitPointEx(float4 p, float u, float v, inout TriangleStream<PixelVertex> output)
{
	PixelVertex outputVert;
	outputVert.position = p;
	outputVert.uv.x = u;
	outputVert.uv.y = v;
	output.Append(outputVert);
}

[maxvertexcount(6)]
void main (point ParticleVertex p[1], inout TriangleStream<PixelVertex> output)
{
	float scale = 0.1f;

	float4 pos = mul(worldMatrixAndProj, float4(p[0], 1.0f));

	float s = scale;
	float4 right = float4(s * aspect.x, 0, 0, 0);
	float4 up = float4(0, s, 0, 0);

	float4 billboard[4];
	billboard[0] = pos - right - up;
	billboard[1] = pos + right - up;
	billboard[2] = pos + right + up;
	billboard[3] = pos - right + up;

	EmitPointEx(billboard[0], -1, -1, output);
	EmitPointEx(billboard[1],  1, -1,  output);
	EmitPointEx(billboard[2],  1,  1, output);
	EmitPointEx(billboard[2],  1,  1, output);
	EmitPointEx(billboard[3], -1,  1, output);
	EmitPointEx(billboard[0], -1, -1, output);
}