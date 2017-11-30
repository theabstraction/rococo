#include "mplat.types.hlsl"

cbuffer GlobalState: register(b0)
{
	GlobalState global;
}

cbuffer Spotlight: register(b1)
{
	Light light;
};

cbuffer AmbienceState: register(b2)
{
	AmbientData ambience;
}

cbuffer DepthRenderDesc : register(b3)
{
	DepthRenderDesc drd;
}

cbuffer InstanceState: register(b4)
{
	ObjectInstance instance;
}

cbuffer textureState : register(b5)
{
	TextureDescState state;
}

SamplerState fontSampler: register(s0);
SamplerState spriteSampler: register(s1);
SamplerState matSampler: register(s2);
SamplerState envSampler: register(s3);
SamplerState shadowSampler: register(s4);

Texture2D tx_FontSprite: register(t0);
Texture2D tx_ShadowMap: register(t2);
TextureCube tx_cubeMap: register(t3);
Texture2D tx_SelectedTexture : register(t4);
Texture2DArray tx_materials: register(t6);
Texture2DArray tx_BitmapSprite: register(t7);

float4 Transform_Instance_To_World(float4 v)
{
	return mul(instance.modelToWorldMatrix, v);
}

float4 Transform_World_To_Screen(float4 v)
{
	return mul(global.worldToScreenMatrix, v);
}

float4 Transform_World_To_Camera(float4 v)
{
	return mul(global.worldToCameraMatrix, v);
}

float4 Transform_World_To_ShadowBuffer(float4 v)
{
	return mul(light.worldToShadowBufferMatrix, v);
}

float4 Transform_World_To_DepthBuffer(float4 v)
{
	return mul(drd.worldToScreen, v);
}

bool IsInShadow(float4 shadowPos)
{
	float4 shadowXYZW = shadowPos / shadowPos.w;
	float2 shadowUV = float2(1.0f + shadowXYZW.x, 1.0f - shadowXYZW.y) * 0.5f;

	float bias = -0.00001f;
	float shadowDepth = tx_ShadowMap.Sample(shadowSampler, shadowUV).x + bias;

	return shadowDepth <= shadowXYZW.z;
}

float4 SampleMaterial(float3 materialVertex, float4 colour)
{
	float4 texel = tx_materials.Sample(matSampler, materialVertex);
	return float4(lerp(colour.xyz, texel.xyz, colour.w), 1.0f);
}

float4 ModulateWithEnvMap(float4 texel, float3 incident, float3 worldNormal, float gloss)
{
	float3 reflectionVector = reflect(incident, worldNormal);
	float4 reflectionColor = tx_cubeMap.Sample(envSampler, reflectionVector);
	return float4( lerp(texel.xyz, reflectionColor.xyz, gloss), 1.0f );
}

float4 GetPointSpriteTexel(float2 uv, float4 colour)
{
	float r = dot(uv, uv);
	float intensity = 0.2f * clamp(1 - r, 0, 1);
	float4 texel = float4(colour.xyz, intensity * colour.w);
	return texel;
}

float GetSpotlightIntensity(float3 lightToPixelDir)
{
	float f = dot(lightToPixelDir, normalize(light.direction.xyz));

	float intensity = 1.0f;

	if (f < light.cutoffCosAngle) intensity = pow(1.0f - (light.cutoffCosAngle - f), light.cutoffPower);

	if (f < 0) f = 0;

	return pow(f, 16.0f);
}

float GetClarity(float3 cameraSpacePosition)
{
	float range = length(cameraSpacePosition.xyz);
	return exp(range * light.fogConstant);
}

float4 GetFontPixel(float3 uv_blend, float4 vertexColour)
{
	float fontIntensity = lerp(1.0f, tx_FontSprite.Sample(fontSampler, uv_blend.xy).x, uv_blend.z);
	return float4(vertexColour.xyz, fontIntensity);
}

float GetSpecular(ObjectPixelVertex p, float3 incident, float3 lightDirection)
{
	float shine = 240.0f;
	float3 r = reflect(lightDirection, p.normal.xyz);
	float dotProduct = dot(r, incident);
	float specular = p.uv_material_and_gloss.w * max(pow(dotProduct, shine), 0);
	return specular;
}

float GetDiffuse(ObjectPixelVertex p, float3 lightToPixelVec, float3 lightToPixelDir)
{
	float R2 = dot(lightToPixelVec, lightToPixelVec);
	float incidence = -dot(lightToPixelDir, p.normal.xyz);
	float i2 = clamp(incidence, 0, 1);
	return i2 * pow(R2, light.attenuationRate);
}