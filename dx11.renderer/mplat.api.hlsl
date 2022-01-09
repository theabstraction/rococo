#include "mplat.types.hlsl"

// b registers are mapped by CBUFFER_INDEX in dx11.renderer.cpp
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

cbuffer SunlightState : register(b6)
{
	Sunlight sunlight;
};

cbuffer BoneMatricesState: register(b7)
{
	float4x4 boneMatrices[16];
};

SamplerState fontSampler: register(s0);
SamplerState shadowSampler: register(s2);
SamplerState envSampler: register(s3);
SamplerState selectSampler: register(s4);
SamplerState matSampler: register(s6);
SamplerState spriteSampler: register(s7);
SamplerState glyphSampler: register(s8);

Texture2D tx_FontSprite: register(t0);
Texture2D tx_ShadowMap: register(t2);
TextureCube tx_cubeMap: register(t3);
Texture2D tx_SelectedTexture : register(t4);
Texture2DArray tx_materials: register(t6);
Texture2DArray tx_BitmapSprite: register(t7);
Texture2DArray tx_GlyphArray: register(t8);

float4 Transform_Instance_To_World(float4 v)
{
	return mul(instance.modelToWorldMatrix, v);
}

float4x4 GetBoneMatrix(float index)
{
	return boneMatrices[(int)index];
}

float4 Transform_Model_Vertices_Via_Bone_Weights(float4 v, BoneWeight_2Bones w)
{
	float4x4 skin0 = GetBoneMatrix(w.index0);
	float4 v0 = mul(skin0, v);
	float4x4 skin1 = GetBoneMatrix(w.index1);
	float4 v1 = mul(skin1, v);
	return lerp(v0, v1, w.weight1);
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
	float incidence = clamp(dot(lightToPixelDir, light.direction.xyz), 0, 1);
	float radialAttenuation = clamp(light.cutoffCosAngle - incidence, 0, 1);	
	float intensity = incidence * pow(1.0f - radialAttenuation, light.cutoffPower);
	return intensity;
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
	return clamp(specular, 0, 1);
}

float GetDiffuse(ObjectPixelVertex p, float3 lightToPixelVec, float3 lightToPixelDir)
{
	float R2 = dot(lightToPixelVec, lightToPixelVec);
	float incidence = -dot(lightToPixelDir, p.normal.xyz);
	float i2 = clamp(incidence, 0, 1);
	return i2 * pow(R2, light.attenuationRate);
}