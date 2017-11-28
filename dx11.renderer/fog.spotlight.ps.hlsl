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
	float4 aspect;
};

Texture2DArray g_materials: register(t6);
Texture2D g_ShadowMap: register(t2);
TextureCube g_cubeMap: register(t3);

SamplerState fontSampler: register(s0);
SamplerState spriteSampler: register(s1);
SamplerState matSampler: register(s2);
SamplerState envSampler: register(s3);
SamplerState shadowSampler: register(s4);

float4 main(PixelVertex p) : SV_TARGET
{
	float4 shadowXYZW = p.shadowPosition / p.shadowPosition.w;
	float2 shadowUV = float2(1.0f + shadowXYZW.x, 1.0f - shadowXYZW.y) * 0.5f;

	float bias = -0.00001f;
	float shadowDepth = g_ShadowMap.Sample(shadowSampler, shadowUV).x + bias;

	float isLit = shadowDepth > shadowXYZW.z;

	if (isLit)
	{
		float3 lightToPixelVec = p.worldPosition.xyz - light.position.xyz;
		float R2 = dot(lightToPixelVec, lightToPixelVec);

		float r = dot(p.uv, p.uv);
		float intensity = 0.2f * clamp(1 - r, 0, 1);
		float4 texel = float4(p.colour.xyz, intensity * p.colour.w);

		float3 lightToPixelDir = normalize(lightToPixelVec);

		float f = dot(lightToPixelDir, normalize(light.direction.xyz));

		float falloff = 1.0f;

		if (f < light.cutoffCosAngle) falloff = pow(1.0f - (light.cutoffCosAngle - f), light.cutoffPower);

		if (f < 0) f = 0;

		float range = length(p.cameraSpacePosition.xyz);
		float fogging = exp(range * light.fogConstant);

		float diffuse = pow(f, 16.0f) * pow(R2, light.attenuationRate);
		float I = diffuse * falloff * fogging;

		texel.xyz *= I;
		texel.xyz *= light.colour.xyz;

		return texel;
	}
	else
	{
		return float4(0, 0, 0, 0);
	}
}

