#include "mplat.types.hlsl"

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
    return saturate(spec);   // remove the saturate and restore norm once you render to HDR and tonemap
}

float GetDiffuse(ObjectPixelVertex p, float3 lightToPixelVec, float3 lightToPixelDir)
{
	float R2 = dot(lightToPixelVec, lightToPixelVec);
	float incidence = -dot(lightToPixelDir, p.worldNormal.xyz);
	float i2 = clamp(incidence, 0, 1);
	return i2 * pow(R2, light.attenuationRate);
}

float GetSpecularG(GPixelSpec p, float gloss, float3 normal, float3 incident, float3 lightDirection)
{
    float3 H = normalize(-lightDirection - incident);
    float shine = 48.0f;
    float NdotL = saturate(-dot(normal, lightDirection));
    return saturate(gloss * pow(saturate(dot(normal, H)), shine) * NdotL);
}

float GetSpotlightIntensity(float3 lightToPixelDir)
{
	float incidence = clamp(dot(lightToPixelDir, light.direction.xyz), 0, 1);
	float radialAttenuation = clamp(light.cutoffCosAngle - incidence, 0, 1);	
	float intensity = incidence * pow(1.0f - radialAttenuation, light.cutoffPower);
	return intensity;
}

float2 GetDiffuseAndSpecular(ObjectPixelVertex v)   // x = diffuse, y = specular
{
    float3 incident = ComputeEyeToWorldDirection(v);
    float3 L = GetLightToWorldPosition(v);
    float3 Ldir = normalize(L);
    float k = GetSpotlightIntensity(Ldir) * GetClarity(v);
    return float2(GetDiffuse(v, L, Ldir), GetSpecular(v, incident, Ldir)) * k;
}

float GetDiffuseG(GPixelSpec p, float3 normal, float3 lightToPixelVec, float3 lightToPixelDir)
{
    float R2 = dot(lightToPixelVec, lightToPixelVec);
    float incidence = -dot(lightToPixelDir, normal);
    float i2 = clamp(incidence, 0, 1);
    return i2 * pow(R2, light.attenuationRate);
}

float GetClarity(ObjectPixelVertex v);

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

float GetClarityAcrossSpan(float3 cameraSpacePosition);

float GetDiffuseSpecularAndFoggedLighting(GPixelSpec v, float3 normal, float3 worldPosition, float3 incident, float gloss)
{
	// We dont apply the environment here, because by definition the environment is lit by ambient light only
	
    float3 lightToPixelVec = GetLightToWorldPositionG(v, worldPosition);
    float3 lightToPixelDir = normalize(lightToPixelVec);

    float intensity = GetSpotlightIntensity(lightToPixelDir);
    
    float diffuse = GetDiffuseG(v, normal, lightToPixelVec, lightToPixelDir);
    
    float3 cameraSpacePosition = { 0, 0, 0 };
    float clarity = GetClarityAcrossSpan(cameraSpacePosition);
    float specular = GetSpecularG(v, gloss, normal, incident, lightToPixelDir);
    float I = (diffuse + specular) * intensity * clarity;
	
    return I;
}