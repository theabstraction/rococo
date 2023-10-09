#include <RAL\RAL.h>
#include <rococo.graphics.types.h>
#include <rococo.renderer.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::RAL;

namespace Rococo::RAL
{
	ID_TEXTURE GeneratePermuationTexture(IRAL& ral)
	{
		ID_TEXTURE noisePermutationTable = ral.RALTextures().CreateRenderTarget("NoisePermutationTable", 256, 1, TextureFormat_8_BIT_UFLOAT);
		ID_PIXEL_SHADER shaderId = ral.Shaders().CreatePixelShader("!shaders/compiled/GeneratePermutationTexture.ps");
		ral.Fill(noisePermutationTable, shaderId);
		return noisePermutationTable;
	}

	ID_TEXTURE GenerateGradientTexture(IRAL& ral)
	{
		ID_TEXTURE gradientTable = ral.RALTextures().CreateRenderTarget("NoiseGradientTable", 16, 1, TextureFormat_8_BIT_UFLOAT);
		ID_PIXEL_SHADER shaderId = ral.Shaders().CreatePixelShader("!shaders/compiled/GenerateGradientTexture.ps");
		ral.Fill(gradientTable, shaderId);
		return gradientTable;
	}
}

/*
sampler permSampler = sampler_state 
{   
	texture = <permTexture>; 
	AddressU  = Wrap; 
	AddressV  = Clamp; 
	MAGFILTER = POINT;
	MINFILTER = POINT;
	MIPFILTER = NONE; 
}; 

sampler gradSampler = sampler_state  
{   
	texture = <gradTexture>;  
	AddressU  = Wrap;
	AddressV  = Clamp;
	MAGFILTER = POINT;
	MINFILTER = POINT; 
	MIPFILTER = NONE;
}; 
*/