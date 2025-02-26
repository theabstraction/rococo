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

float GetSpecularG(GPixelSpec p, float gloss, float3 normal, float3 incident, float3 lightDirection)
{
    float shine = 240.0f;
    float3 r = reflect(lightDirection, normal);
    float dotProduct = dot(r, incident);
    float specular = gloss * max(pow(abs(dotProduct), shine), 0);
    return clamp(specular, 0, 1);
}

float GetDiffuseG(GPixelSpec p, float3 normal, float3 lightToPixelVec, float3 lightToPixelDir)
{
    float R2 = dot(lightToPixelVec, lightToPixelVec);
    float incidence = -dot(lightToPixelDir, normal);
    float i2 = clamp(incidence, 0, 1);
    return i2 * pow(R2, light.attenuationRate);
}

float GetSpotlightIntensity(float3 lightToPixelDir)
{
	float incidence = clamp(dot(lightToPixelDir, light.direction.xyz), 0, 1);
	float radialAttenuation = clamp(light.cutoffCosAngle - incidence, 0, 1);	
	float intensity = incidence * pow(1.0f - radialAttenuation, light.cutoffPower);
	return intensity;
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

float GetDiffuseSpecularAndFoggedLighting(GPixelSpec v, float3 normal, float3 worldPosition, float3 incident)
{
	// We dont apply the environment here, because by definition the environment is lit by ambient light only
	
    float3 lightToPixelVec = GetLightToWorldPositionG(v, worldPosition);
    float3 lightToPixelDir = normalize(lightToPixelVec);

    float intensity = GetSpotlightIntensity(lightToPixelDir);
    
    float diffuse = GetDiffuseG(v, normal, lightToPixelVec, lightToPixelDir);
    
    float3 cameraSpacePosition = { 0, 0, 0 };
    float gloss = 0.5f;
    float clarity = GetClarityAcrossSpan(cameraSpacePosition);
    float specular = GetSpecularG(v, gloss, normal, incident, lightToPixelDir);
    float I = (diffuse + specular) * intensity * clarity;
	
    return I;
}