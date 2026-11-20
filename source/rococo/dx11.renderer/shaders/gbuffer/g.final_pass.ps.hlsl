#include "..\mplat.api.hlsl"
#include "..\shadows.api.hlsl"
#include "..\lights.api.hlsl"

// normal element range is -1 to 1, so add + 1 and halve to normalize to [0,1]
float3 NormalizeSignedValues(float3 s)
{
    return 0.5f * (s + float3(1.0f, 1.0f, 1.0f));
}

float3 ReconstructGeometricNormal(int2 pixel, float3 P, float3 bumped)
{
    float3 pR = tx_GBuffer_Position.Load(int3(pixel + int2( 1,  0), 0)).xyz;
    float3 pL = tx_GBuffer_Position.Load(int3(pixel + int2(-1,  0), 0)).xyz;
    float3 pD = tx_GBuffer_Position.Load(int3(pixel + int2( 0,  1), 0)).xyz;
    float3 pU = tx_GBuffer_Position.Load(int3(pixel + int2( 0, -1), 0)).xyz;

    // choose the shorter difference on each axis so we never span a silhouette
    float3 dx = (dot(pR - P, pR - P) < dot(P - pL, P - pL)) ? (pR - P) : (P - pL);
    float3 dy = (dot(pD - P, pD - P) < dot(P - pU, P - pU)) ? (pD - P) : (P - pU);

    float3 n = cross(dy, dx);
    float len2 = dot(n, n);
    if (len2 < 1e-12f) return bumped;          // degenerate, so fall back
    n *= rsqrt(len2);
    if (dot(n, bumped) < 0) n = -n;            // flip before blending, so the lerp never cancels out
    return normalize(lerp(n, bumped, 0.5f));
}

float4 main(GPixelSpec spec) : SV_TARGET
{
    float4 rawColourWithGloss = tx_GBuffer_Colour.Sample(spriteSampler, spec.uv);
    float3 albedo = rawColourWithGloss.xyz;

    float4 depthAndNg = tx_GBuffer_Depth.Sample(spriteSampler, spec.uv);
    float depth = depthAndNg.x;

    if (depth == 1.0f)
    {
        // We sampled a skybox pixel, so we only use the skybox pixel in raw form
        return float4(albedo, 1.0f);
    }

    float gloss = rawColourWithGloss.w;
    float3 worldPosition = tx_GBuffer_Position.Sample(spriteSampler, spec.uv).xyz;
    float3 normal = normalize(tx_GBuffer_Normal.Sample(spriteSampler, spec.uv).xyz);   // bumped
	
    float3 Ng = ReconstructGeometricNormal(int2(spec.position.xy), worldPosition, normal);

    float4 shadowPos = Transform_World_To_ShadowBuffer(float4(worldPosition, 1.0f));
    float shadowDensity = GetShadowDensity_16Sample(shadowPos);

    float3 incident = ComputeEyeToWorldDirectionG(spec, worldPosition);

    // The following computation requires global value 'light' to have been assigned to the shader
    float2 ds = GetDiffuseAndSpecularG(spec, normal, Ng, worldPosition, incident, gloss);   // x = diffuse, y = specular

    // Diffuse is tinted by the surface colour, specular is not
    float3 lit = (albedo * ds.x + ds.y) * light.colour.xyz;
    lit = lerp(lit, float3(0.0f, 0.0f, 0.0f), shadowDensity);

    float3 reflected = ModulateWithEnvMap(float4(lit, 1.0f), incident, normal, gloss).xyz;

    float3 ambient = light.ambient.xyz * albedo * GetAmbientScale(normal, Ng);

    return float4(reflected + ambient, 1.0f);
}