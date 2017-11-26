struct PixelVertex
{
	float4 position : SV_POSITION0;
	float2 uv: TEXCOORD0;
};

float4 main(PixelVertex p) : SV_TARGET
{
	float r = dot(p.uv, p.uv);
	float intensity = 0.2f * clamp(1 - r, 0, 1);
	return float4(1,1,1, intensity);
}
