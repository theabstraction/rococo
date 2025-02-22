#include "..\mplat.api.hlsl"
#include "..\shadows.api.hlsl"
#include "..\lights.api.hlsl"

// normal element range is -1 to 1, so add + 1 and halve to normalize to [0,1]
float3 NormalizeSignedValues(float3 s)
{
    return 0.5f * (s + float3(1.0f, 1.0f, 1.0f));
}

float4 main(GPixelSpec spec) : SV_TARGET
{
    float4 rawColour = tx_GBuffer_Colour.Sample(spriteSampler, spec.uv);
	
    float shadowDensity = 0.15f; // GetShadowDensityDirect(spec.position);
		 
    float3 normal = tx_GBuffer_Normal.Sample(spriteSampler, spec.uv).xyz;       
   
    float I = 1.0f; // GetDiffuseSpecularAndFoggedLighting(spec, normal);
	
    return shadowDensity * rawColour;
	
    // The following computation requires global value 'light' to have been assigned to the shader
    return BlendColourWithLightAndShadow(rawColour, shadowDensity, I);
}
