#include "mplat.types.hlsl"

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

float4 Project_World_To_DepthBuffer(float4 v)
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

float4 SampleMaterialByVectors(float3 uvw, float4 colour)
{
	float4 texel = tx_materials.Sample(matSampler, uvw);
	float colourToTexelBlendFactor = colour.w;
	return float4(lerp(colour.xyz, texel.xyz, colourToTexelBlendFactor), 1.0f);
}

float4 SampleMaterial(ObjectPixelVertex v)
{
    return SampleMaterialByVectors(v.uv_material_and_gloss.xyz, v.colour);
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

float GetClarityAcrossSpan(float3 cameraSpacePosition)
{
	float range = length(cameraSpacePosition.xyz);
	return exp(range * light.fogConstant);
}

float GetClarity(ObjectPixelVertex v)
{
    return GetClarityAcrossSpan(v.cameraSpacePosition.xyz);
}

float4 GetFontPixel(float3 uv_blend, float4 vertexColour)
{
	float fontIntensity = lerp(1.0f, tx_FontSprite.Sample(fontSampler, uv_blend.xy).x, uv_blend.z);
	return float4(vertexColour.xyz, fontIntensity);
}

float3 fade(float3 t)
{
    // f = 6t^5 - 15t^4 + 10t^3
    // if (t == 0 then f == 0)
    // if (t == 1 then f == 1)
    
    // df/dt = 30t^4 - 60t^3 + 30t^2. When t == 0 df/dt == 0 and when t = 1 df/dt = 0
    
    // f''(t) = df2/dt^2 = 120t^3 - 180t^2 + 60t. f''(0) =  0 and f''(1) = 0
    
    // Thus f has the property that it interpolates between 0 and 1 with t going from 0 to 1 and its first and second order derivatives are zero at the bounds    
    
    return t * t * t * (t * (t * 6 - 15) + 10);
}

// Convert an x into a 0 to 1 value, the permSampler contains a pseudorandom vector
float perm(float x)
{
    return tx_noisePermutationArray.Sample(noisePermutationSampler, x / 256.0).x * 256;
}

// The gradient array consists of unit length components, so the dot product effectively flips, preserves or nullifies components from p
float grad(float x, float3 p)
{
    return dot(tx_noiseGradientArray.Sample(noiseGradientSampler, x).xyz, p);
}

// 3D version 
float inoise(float3 p)
{
    float3 P = fmod(floor(p), 256.0);
    p -= floor(p);
    float3 f = fade(p);
	// HASH COORDINATES FOR 6 OF THE 8 CUBE CORNERS 
    float A = perm(P.x) + P.y;
    float AA = perm(A) + P.z;
    float AB = perm(A + 1) + P.z;
    float B = perm(P.x + 1) + P.y;
    float BA = perm(B) + P.z;
    float BB = perm(B + 1) + P.z;
	// AND ADD BLENDED RESULTS FROM 8 CORNERS OF CUBE
    return lerp(lerp(lerp(grad(perm(AA), p), grad(perm(BA), p + float3(-1, 0, 0)), f.x), lerp(grad(perm(AB), p + float3(0, -1, 0)), grad(perm(BB), p + float3(-1, -1, 0)), f.x), f.y), lerp(lerp(grad(perm(AA + 1), p + float3(0, 0, -1)), grad(perm(BA + 1), p + float3(-1, 0, -1)), f.x), lerp(grad(perm(AB + 1), p + float3(0, -1, -1)), grad(perm(BB + 1), p + float3(-1, -1, -1)), f.x), f.y), f.z);
}