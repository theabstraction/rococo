#include <rococo.mplat.h>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace ANON
{
	struct RendererConfig : public IRendererConfigSupervisor
	{
		IRenderer& renderer;

		RendererConfig(IRenderer& _renderer) : renderer(_renderer) 
		{
			static_assert(sizeof(SampleStateDef) == 32, "Expecting someone taller");
		}

		void Free() override
		{
			delete this;
		}

		void SetSampler(const SampleStateDef& ssd, SampleIndex index) override
		{
			renderer.SetSampler(
				index,
				(Samplers::Filter) ssd.method,
				(Samplers::AddressMode) ssd.u,
				(Samplers::AddressMode) ssd.v,
				(Samplers::AddressMode) ssd.w,
				ssd.borderColour);
		}
	};
}

namespace Rococo
{
	namespace Graphics
	{
		IRendererConfigSupervisor* CreateRendererConfig(IRenderer& renderer)
		{
			return new ANON::RendererConfig(renderer);
		}
	}
}