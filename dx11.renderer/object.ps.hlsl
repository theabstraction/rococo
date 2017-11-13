struct PixelVertex
{
	float4 position : SV_POSITION0;
	float4 uv_material_and_gloss: TEXCOORD;
    float4 worldPosition: TEXCOORD1;
	float4 normal : TEXCOORD2;
	float4 shadowPos: TEXCOORD3;
	float4 cameraSpacePosition: TEXCOORD4;
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
	float fogConstant;
	float3 randoms; // 4 random quotients 0.0 - 1.0
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

cbuffer globalState: register(b1)
{
	float4x4 worldMatrixAndProj;
	float4x4 worldMatrix;
	float4 eye;
	float4 viewDir;
};

Texture2DArray g_materials: register(t6);
SamplerState txSampler;

Texture2D g_ShadowMap: register(t2);

SamplerState shadowSamplerLookup
{
	// sampler state
	Filter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	AddressU = MIRROR;
	AddressV = MIRROR;
};

TextureCube g_cubeMap: register(t3);

float4 per_pixel_lighting(PixelVertex p)
{
	float4 shadowXYZW = p.shadowPos / p.shadowPos.w;
	float2 shadowUV = float2(1.0f + shadowXYZW.x, 1.0f - shadowXYZW.y) * 0.5f;

	float bias = -0.00001f;
	float shadowDepth = g_ShadowMap.Sample(shadowSamplerLookup, shadowUV).x + bias;

	float isLit = shadowDepth > shadowXYZW.z;

	if (isLit)
	{
		float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;
		float R2 = dot(lightToPixelVec, lightToPixelVec);

		float4 texel = g_materials.Sample(txSampler, p.uv_material_and_gloss.xyz);
		texel = lerp(p.colour, texel, p.colour.w);

		float3 incident = normalize(p.worldPosition.xyz - eye.xyz);
		float3 reflectionVector = reflect(incident.xyz, normalize(p.normal.xyz));
		float4 reflectionColor = g_cubeMap.Sample(txSampler, reflectionVector);

		texel.xyz = lerp(texel.xyz, reflectionColor.xyz, p.uv_material_and_gloss.w);

		float3 lightToPixelDir = normalize(lightToPixelVec);

		float f = dot(lightToPixelDir, normalize(light.direction.xyz));

		float falloff = 1.0f;

		if (f < light.cutoffCosAngle) falloff = pow(1.0f - (light.cutoffCosAngle - f), light.cutoffPower);

		if (f < 0) f = 0;

		float3 normal = normalize(p.normal.xyz);
		float g = -dot(lightToPixelDir, normal);

		float range = length(p.cameraSpacePosition.xyz);
		float fogging = exp(range * light.fogConstant);

		float3 r = reflect(lightToPixelDir.xyz, normal);

		float dotProduct = -dot(r, viewDir.xyz);
		float shine = 240.0f;
		float specular = p.uv_material_and_gloss.w * max(pow(dotProduct, shine), 0);
		
		float diffuse = pow(f,16.0f) * g * pow(R2, light.attenuationRate);
		float intensity = (diffuse + specular) * falloff * fogging;

		return float4 (texel.x * intensity * light.colour.x, texel.y * intensity * light.colour.y, texel.z * intensity * light.colour.z, 1.0f);
	}
	else
	{
		return float4(0,0,0,1);
	}
}

float4 no_lighting(PixelVertex p)
{
	float4 texel = g_materials.Sample(txSampler, p.uv_material_and_gloss.xyz).xyzw;
	texel = lerp(p.colour, texel, p.colour.w);

	float3 incident = normalize(p.worldPosition.xyz - eye.xyz);
	float3 reflectionVector = reflect(incident.xyz, normalize(p.normal.xyz));
	float4 reflectionColor = g_cubeMap.Sample(txSampler, reflectionVector);

	texel.xyz = lerp(texel.xyz, reflectionColor.xyz, p.uv_material_and_gloss.w);
	return texel;
}

float4 main(PixelVertex p) : SV_TARGET
{
	return per_pixel_lighting(p);
}
