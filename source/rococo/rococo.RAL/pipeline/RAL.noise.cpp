#include <RAL\RAL.h>
#include <rococo.graphics.types.h>
#include <rococo.renderer.h>
#include <rococo.renderer.formats.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::RAL;

namespace Rococo::RAL
{
	ID_TEXTURE GeneratePermuationTexture(IRAL& ral)
	{
		ID_TEXTURE noisePermutationTable = ral.RALTextures().CreateRenderTarget("NoisePermutationTable", 256, 1, TextureFormat::F_8_BIT_UINT);
		ID_PIXEL_SHADER shaderId = ral.Shaders().CreatePixelShader("!shaders/compiled/GeneratePermutationTexture.ps");
		ral.Fill(noisePermutationTable, shaderId);
		return noisePermutationTable;
	}

	ID_TEXTURE GenerateGradientTexture(IRAL& ral)
	{
		ID_TEXTURE gradientTable = ral.RALTextures().CreateRenderTarget("NoiseGradientTable", 16, 1, TextureFormat::F_24_BIT_BUMPMAP);
		ID_PIXEL_SHADER shaderId = ral.Shaders().CreatePixelShader("!shaders/compiled/GenerateGradientTexture.ps");
		ral.Fill(gradientTable, shaderId);
		return gradientTable;
	}
}
