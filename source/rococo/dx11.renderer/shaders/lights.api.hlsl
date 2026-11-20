#include "mplat.types.hlsl"

// Defined in mplat.api.hlsl. Declared here so this file doesn't depend on include order
float GetClarity(ObjectPixelVertex v);
float GetClarityAcrossSpan(float3 cameraSpacePosition);

float3 GetLightToWorldPosition(ObjectPixelVertex p)
{
    return p.worldPosition.xyz - light.position.xyz;
}

float3 GetLightToWorldPositionG(GPixelSpec p, float3 worldPosition)
{
    return worldPosition - light.position.xyz;
}

float GetSpecular(ObjectPixelVertex p, float3 incident, float3 lightDirection)
{
    float3 N = p.worldNormal.xyz;
    float3 H = normalize(-lightDirection - incident);
    float shine = 48.0f;
    float NdotL = saturate(-dot(N, lightDirection));
    float spec = p.uv_and_gloss.z * pow(saturate(dot(N, H)), shine) * NdotL;
    return saturate(spec);   // remove the saturate and restore energy normalisation once you render to HDR and tonemap
}

float GetDiffuse(ObjectPixelVertex p, float3 lightToPixelVec, float3 lightToPixelDir)
{
    float R2 = dot(lightToPixelVec, lightToPixelVec);
    float incidence = -dot(lightToPixelDir, p.worldNormal.xyz);
    float i2 = saturate(incidence);
    return i2 * pow(R2, light.attenuationRate);
}

float GetSpecularG(GPixelSpec p, float gloss, float3 normal, float3 incident, float3 lightDirection)
{
    float3 H = normalize(-lightDirection - incident);
    float shine = 48.0f;
    float NdotL = saturate(-dot(normal, lightDirection));
    return saturate(gloss * pow(saturate(dot(normal, H)), shine) * NdotL);
}

float GetDiffuseG(GPixelSpec p, float3 normal, float3 lightToPixelVec, float3 lightToPixelDir)
{
    float R2 = dot(lightToPixelVec, lightToPixelVec);
    float incidence = -dot(lightToPixelDir, normal);
    float i2 = saturate(incidence);
    return i2 * pow(R2, light.attenuationRate);
}

float GetSpotlightIntensity(float3 lightToPixelDir)
{
    float incidence = saturate(dot(lightToPixelDir, light.direction.xyz));
    float radialAttenuation = saturate(light.cutoffCosAngle - incidence);
    float intensity = incidence * pow(1.0f - radialAttenuation, light.cutoffPower);
    return intensity;
}

// Fades light out as the geometric (unbumped) surface turns away from the light, so bumped normals cannot light the dark side
float GetTerminatorFade(float3 geometricNormal, float3 lightToPixelDir)
{
    return saturate(-dot(geometricNormal, lightToPixelDir) * 2.0f + 0.25f);
}

// Normal-aware ambient scale: sky/ground hemisphere plus cavity darkening where the bump bends away from the surface
float GetAmbientScale(float3 bumpedNormal, float3 geometricNormal)
{
    float hemi = 0.5f + 0.5f * dot(bumpedNormal, WORLD_UP);
    float cavity = saturate(dot(bumpedNormal, geometricNormal));
    cavity *= cavity;
    return lerp(0.6f, 1.0f, hemi) * cavity;
}

// Forward path. x = diffuse, y = specular. Both include spotlight cone, fog clarity and terminator fade
// p.worldNormal must already hold the bumped normal; Ng is the geometric normal captured before bumping
float2 GetDiffuseAndSpecular(ObjectPixelVertex p, float3 Ng)
{
    float3 incident = ComputeEyeToWorldDirection(p);
    float3 L = GetLightToWorldPosition(p);
    float3 Ldir = normalize(L);

    float k = GetSpotlightIntensity(Ldir) * GetClarity(p) * GetTerminatorFade(Ng, Ldir);

    return float2(GetDiffuse(p, L, Ldir), GetSpecular(p, incident, Ldir)) * k;
}

// Deferred path. x = diffuse, y = specular. Same terms as the forward path
// Fog uses eye distance, which equals length(cameraSpacePosition) in the forward path
float2 GetDiffuseAndSpecularG(GPixelSpec v, float3 normal, float3 Ng, float3 worldPosition, float3 incident, float gloss)
{
    float3 L = GetLightToWorldPositionG(v, worldPosition);
    float3 Ldir = normalize(L);

    float clarity = GetClarityAcrossSpan(worldPosition - global.eye.xyz);
    float k = GetSpotlightIntensity(Ldir) * clarity * GetTerminatorFade(Ng, Ldir);

    return float2(GetDiffuseG(v, normal, L, Ldir), GetSpecularG(v, gloss, normal, incident, Ldir)) * k;
}

// Legacy combined term, still used by fog, scroll and no-texture shaders
float GetDiffuseSpecularAndFoggedLighting(ObjectPixelVertex v)
{
    float3 incident = ComputeEyeToWorldDirection(v);

    // We dont apply the environment here, because by definition the environment is lit by ambient light only

    float3 lightToPixelVec = GetLightToWorldPosition(v);
    float3 lightToPixelDir = normalize(lightToPixelVec);

    float intensity = GetSpotlightIntensity(lightToPixelDir);
    float diffuse = GetDiffuse(v, lightToPixelVec, lightToPixelDir);
    float clarity = GetClarity(v);
    float specular = GetSpecular(v, incident, lightToPixelDir);
    float I = (diffuse + specular) * intensity * clarity;

    return I;
}