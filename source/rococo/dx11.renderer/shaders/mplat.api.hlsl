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
SamplerState shadowSampler: register(s1);
SamplerState envSampler: register(s2);
SamplerState selectSampler: register(s3);
SamplerState matSampler: register(s4);
SamplerState spriteSampler: register(s5);
SamplerState glyphSampler: register(s6);

Texture2D tx_FontSprite: register(t0);
Texture2D tx_ShadowMap: register(t1);
TextureCube tx_cubeMap: register(t2);
Texture2D tx_SelectedTexture : register(t3);
Texture2DArray tx_materials: register(t4);
Texture2DArray tx_BitmapSprite: register(t5);
Texture2DArray tx_GlyphArray: register(t6);

float4 ConvertGuiScreenPixelCoordinatesToDX11Coordinates(GuiVertexOpaque v)
{
    float4 position;
	
	// (left and top) 0,0 maps to -1,1, and (right and bottom) screenWidth, screenHeight maps to 1, -1
    position.x = 2.0f * v.pos.x * global.guiScale.OOScreenWidth - 1.0f;
    position.y = -2.0f * v.pos.y * global.guiScale.OOScreenHeight + 1.0f;
    position.z = 0;
    position.w = 1.0f;
    return position;
}

BaseVertexData GetBaseVertexDataFromFloat3(float3 v)
{
    BaseVertexData base;
    base.uv = v.xy;
    base.fontBlend = v.z;
    return base;
}

BaseVertexData GetBaseVertexData(GuiVertexOpaque v)
{
    return GetBaseVertexDataFromFloat3(v.base);
}

float4 GetGuiVertexColour(GuiVertexOpaque v)
{
    return v.colour;
}

SpriteVertexData GetSpriteVertexDataFromFloat4(float4 v)
{
    SpriteVertexData sd;
    sd.saturation = v.x;
    sd.spriteIndex = v.y;
    sd.matIndex = v.z;
    sd.spriteToMatLerpFactor = v.w;
    return sd;
}

SpriteVertexData GetSpriteVertexData(GuiVertexOpaque v)
{
    return GetSpriteVertexDataFromFloat4(v.sd);
}

GuiPixelVertex ToGuiPixelVertex(GuiPixelVertexOpaque p_opaque)
{
	GuiPixelVertex p;
    p.colour = p_opaque.colour;
    p.base = GetBaseVertexDataFromFloat3(p_opaque.base);
    p.sd = GetSpriteVertexDataFromFloat4(p_opaque.sd);
    p.position = p_opaque.colour;
    return p;
}

float2 GetGuiTextureUV(GuiVertexOpaque v)
{
    float2 uv = lerp(v.base.xy, v.base.xy * global.guiScale.OOFontWidth, v.base.z);
    uv = lerp(uv * global.guiScale.OOSpriteWidth, uv, v.sd.w);
    return uv;
}

float3 GetEyePosition()
{
	return global.eye.xyz;
}

float4 Transform_Instance_To_World(float4 v)
{
	return mul(instance.modelToWorldMatrix, v);
}

float4 Transform_Instance_To_World_Scaled(float3 v)
{
    float4 hSv = float4(instance.scale.x * v.x, instance.scale.x * v.y,  instance.scale.x * v.z, 1.0f);
	return mul(instance.modelToWorldMatrix, hSv);
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

float4 Project_World_To_Screen(float4 v)
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

float4 ComputeWorldPosition(ObjectVertex v)
{
    return Transform_Instance_To_World_Scaled(v.modelPosition);
}

float3 ComputeWorldNormal(ObjectVertex v)
{
    return Transform_Instance_To_World(float4(v.modelNormal.xyz, 0.0f)).xyz;
}

float SampleShadowWithDelta(float4 pos, float2 offset)
{
	float2 scaledOffset = offset * global.OOShadowTxWidth * light.shadowFudge;
	float3 shadowXYZ = pos.xyz / pos.w;
	float2 shadowUV = (scaledOffset + (float2(1.0f + shadowXYZ.x, 1.0f - shadowXYZ.y))) * 0.5f;

	float bias = -0.00001f;
	float shadowDepth = tx_ShadowMap.Sample(shadowSampler, shadowUV).x + bias;
	
	if (shadowDepth <= shadowXYZ.z)
	{
		return 1.0f;
	}
	else
	{
		return 0.0f;
	}
}

float GetShadowDensity_16Sample(float4 shadowPos)
{
    float shadowDensity = 0.0f;
    float2 delta;
	
    for (delta.y = -1.5f; delta.y <= 1.5f; delta.y += 1.0f)
    {
        for (delta.x = -1.5f; delta.x <= 1.5f; delta.x += 1.0f)
        {
            shadowDensity += SampleShadowWithDelta(shadowPos, delta);
        }
    }
	
    return shadowDensity / 4.0f;
}

float GetShadowDensity_4Sample(float4 shadowPos)
{
	float shadowDensity = 0.0f;
	float2 delta;
	
    float f1 = SampleShadowWithDelta(shadowPos, float2(-1.5f, 0.5f));
    float f2 = SampleShadowWithDelta(shadowPos, float2(0.5f, 0.5f));
    float f3 = SampleShadowWithDelta(shadowPos, float2(-1.5f, -1.5f));
    float f4 = SampleShadowWithDelta(shadowPos, float2(0.5f, -1.5f));
	
    return (f1 + f2 + f3 + f4) * 0.25f;
}

float GetShadowDensity_1Sample(float4 shadowPos)
{
	float3 shadowXYZ = shadowPos.xyz / shadowPos.w;
	float2 shadowUV = float2(1.0f + shadowXYZ.x, 1.0f - shadowXYZ.y) * 0.5f;

	float bias = -0.00001f;
	float shadowDepth = tx_ShadowMap.Sample(shadowSampler, shadowUV).x + bias;

	if (shadowDepth <= shadowXYZ.z)
	{
		return 1.0f;
	}
	else
	{
		return 0.0f;
	}
}

float GetShadowDensity(float4 shadowPos)
{
    return GetShadowDensity_16Sample(shadowPos);
}

float4 SampleMaterial(float3 materialVertex, float4 colour)
{
	float4 texel = tx_materials.Sample(matSampler, materialVertex);
	return float4(lerp(colour.xyz, texel.xyz, colour.w), 1.0f);
}

float SignedToUnsigned (float x)
{
	return (x + 1.0f) * 0.5f;
}

float3 SignedToUnsignedV3 (float3 p)
{
	return float3(SignedToUnsigned(p.x), SignedToUnsigned(p.y), SignedToUnsigned(p.z));
}

float4 ModulateWithEnvMap(float4 texel, float3 incident, float3 worldNormal, float gloss)
{
	float3 reflectionVector = normalize(reflect(incident, worldNormal.xyz));
	float3 r = float3(reflectionVector.x, reflectionVector.z, reflectionVector.y);
	float4 reflectionColor = tx_cubeMap.Sample(envSampler, r);
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
	float3 r = reflect(lightDirection, p.worldNormal.xyz);
	float dotProduct = dot(r, incident);
	float specular = p.uv_material_and_gloss.w * max(pow(abs(dotProduct), shine), 0);
	return clamp(specular, 0, 1);
}

float GetDiffuse(ObjectPixelVertex p, float3 lightToPixelVec, float3 lightToPixelDir)
{
	float R2 = dot(lightToPixelVec, lightToPixelVec);
	float incidence = -dot(lightToPixelDir, p.worldNormal.xyz);
	float i2 = clamp(incidence, 0, 1);
	return i2 * pow(R2, light.attenuationRate);
}