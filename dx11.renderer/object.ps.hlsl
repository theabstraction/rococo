struct PixelVertex
{
	float4 position : SV_POSITION0;
	float4 normal : TEXCOORD2;
	float3 uv_material: TEXCOORD;
    float4 worldPosition: TEXCOORD1;
	float4 shadowPos: TEXCOORD3;
	float4 colour: COLOR0;	// w component gives lerpColourToTexture
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

cbuffer light
{
	Light light;
};

Texture2DArray g_materials: register(t6);
SamplerState txSampler;

Texture2D g_ShadowMap: register(t2);
SamplerState shadowSampler;

float4 per_pixel_lighting(PixelVertex p)
{
	float4 texel = g_materials.Sample(txSampler, p.uv_material);

	texel = lerp(p.colour, texel, p.colour.w);

	float oow = 1.0f / p.shadowPos.w;

	float bias = -0.001f;

	float4 shadowXYZW = p.shadowPos;
	shadowXYZW.z += bias;

	shadowXYZW = shadowXYZW * oow;
	float depth = shadowXYZW.z;

	float2 shadowUV = float2(1.0f + shadowXYZW.x, 1.0f - shadowXYZW.y) * 0.5f;

	float shadowDepth = g_ShadowMap.Sample(shadowSampler, shadowUV).x;
	
	float normalizedDepth = (depth + 1.0f) * 0.5f;

	float4 ambience = float4(texel.x * light.ambient.x, texel.y * light.ambient.y, texel.z * light.ambient.z, 1.0f);

	if (shadowDepth > depth)
	{
		float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;

		float R2 = dot(lightToPixelVec, lightToPixelVec);

		float3 lightToPixelDir = normalize(lightToPixelVec);

		float f = dot(lightToPixelDir, light.direction.xyz);

		float falloff = 1.0f;

		if (f < light.cutoffCosAngle) falloff = pow(1.0f - (light.cutoffCosAngle - f), light.cutoffPower);

		if (f < 0) f = 0;

		float g = -dot(light.direction.xyz, normalize(p.normal.xyz));

		float intensity = clamp(f * g, 0, 1.0f) * pow(R2, light.attenuationRate) * falloff;

		float4 diffCol = float4 (texel.x * intensity * light.colour.x, texel.y * intensity * light.colour.y, texel.z * intensity * light.colour.z, 0.0f);
	//	float4 diffCol = float4 (f, texel.y * intensity * light.colour.y, texel.z * intensity * light.colour.z, 0.0f);
		return diffCol + ambience;
	}
	else
	{
		return ambience;
	}
}

float4 no_lighting(PixelVertex p)
{
	float4 texel = g_materials.Sample(txSampler, p.uv_material).xyzw;
	return texel;
}

float4 visualize_vector(PixelVertex p, float3 vec)
{
	float3 v = normalize(vec);
	return float4 (v.x, v.y, v.z, 1.0f);
}

float4 visualize_time(PixelVertex p)
{
	return float4 (light.time / 60.0f, 0.0f, 0.0f, 1.0f);
}

float4 visualize_red(PixelVertex p)
{
	return float4 (1.0f, 0, 0, 1.0f);
}

float4 visualize_blue(PixelVertex p)
{
	return float4 (0, 0, 1.0f, 1.0f);
}

float4 visualize_light_colour(PixelVertex p)
{
	return light.colour;
}

float4 main(PixelVertex p) : SV_TARGET
{
	return per_pixel_lighting(p);
}
