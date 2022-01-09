#include <rococo.api.h>
#include <rococo.os.win32.h>
#include "rococo.dx11.h"
#include <rococo.auto-release.h>
#include <rococo.hashtable.h>
#include <rococo.os.h>

#include <d3d11_4.h>

#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace ANON
{
	class Pipeline : public IPipelineSupervisor, IPipelineBuilder
	{
	private:
		IDX11System& system;

		struct Stage
		{
			HString name;
			AutoRelease<IRenderStageSupervisor> supervisor;
		};

		std::vector<Stage> stages;

	public:
		Pipeline(IDX11System& ref_system): system(ref_system)
		{

		}

		virtual ~Pipeline()
		{

		}

		void Free() override
		{
			delete this;
		}

		IPipelineBuilder& GetBuilder() override
		{
			return *this;
		}

		void AddStage(cstr friendlyName, IRenderStageSupervisor* stage) override
		{
			stage->AddRef();
			stages.push_back({ friendlyName, stage });
		}

		void Clear() override
		{
			stages.clear();
		}

		void Execute() override
		{
			for (auto& s : stages)
			{
				s.supervisor->Execute();
			}
		}
	};
}

namespace Rococo::Graphics
{
	IPipelineSupervisor* CreatePipeline(IDX11System& system)
	{
		return new ANON::Pipeline(system);
	}
}