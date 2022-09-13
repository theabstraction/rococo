#include <rococo.gui.retained.h>
#include <vector>
#include <stdio.h>
#include <string>
#include <algorithm>

using namespace Rococo;
using namespace Rococo::Gui;

namespace Rococo::Gui
{
	IGRLayoutSupervisor* CreateFullScreenLayout();
	IGRMainFrameSupervisor* CreateGRMainFrame(IGRPanel& panel);
	IGRPanelSupervisor* CreatePanel(IGRPanelRoot& root, IGRLayoutSupervisor* layout);

	bool operator == (const GuiRect& a, const GuiRect& b)
	{
		return a.left == b.left && a.right == b.right && a.top == b.top && a.bottom == b.bottom;
	}
}

namespace ANON
{
	struct GuiRetained: IGuiRetainedSupervisor, IGRPanelRoot
	{
		IGuiRetainedCustodian& custodian;
		GuiRetainedConfig config;

		AutoFree<ISchemeSupervisor> scheme = CreateScheme();

		struct FrameDesc
		{
			IGRPanelSupervisor* panel;
			IGRMainFrameSupervisor* frame;
			std::string id;

			bool Eq(IdWidget other) const
			{
				return other.Name && strcmp(other.Name, id.c_str()) == 0;
			}
		};

		using TFrames = std::vector<FrameDesc>;

		TFrames frameDescriptors;

		GuiRetained(GuiRetainedConfig& _config, IGuiRetainedCustodian& _custodian) : config(_config), custodian(_custodian)
		{

		}

		virtual ~GuiRetained()
		{
			for (auto d : frameDescriptors)
			{
				d.frame->Free();
				d.panel->Free();
			}
		}

		void Free() override
		{
			delete this;
		}

		IGRMainFrame& BindFrame(IdWidget id) override
		{
			IGRMainFrame* oldFrame = TryGetFrame(id);
			if (oldFrame)
			{
				return *oldFrame;
			}

			auto* newRootPanel = CreatePanel(*this, CreateFullScreenLayout());
			auto* newFrame = CreateGRMainFrame(*newRootPanel);
			newRootPanel->SetWidget(*newFrame);
			frameDescriptors.push_back(FrameDesc{ newRootPanel, newFrame, std::string(id.Name) });
			return *newFrame;
		}

		void DeleteFrame(IdWidget id) override
		{
			auto matchesId = [&id](const FrameDesc& desc)
			{
				return desc.Eq(id);
			};

			auto d = std::remove_if(frameDescriptors.begin(), frameDescriptors.end(), matchesId);
			frameDescriptors.erase(d, frameDescriptors.end());
		}

		IScheme& Scheme()
		{
			return *scheme;
		}

		IGRMainFrame* TryGetFrame(IdWidget id) override
		{
			for (auto& d : frameDescriptors)
			{
				if (d.Eq(id) == 0)
				{
					return d.frame;
				}
			}

			return nullptr;
		}

		GuiRect lastLayedOutScreenDimensions { 0,0,0,0 };

		void LayoutFrames()
		{
			for (auto& d : frameDescriptors)
			{
				d.frame->Layout(lastLayedOutScreenDimensions);
			}
		}

		void RenderGui(IGRRenderContext& g) override
		{
			for (auto& d : frameDescriptors)
			{
				auto screenDimensions = g.ScreenDimensions();
				g.SetOrigin({ screenDimensions.left, screenDimensions.top });

				if (lastLayedOutScreenDimensions != screenDimensions)
				{
					lastLayedOutScreenDimensions = screenDimensions;
					LayoutFrames();
				}

				d.frame->Render(g);
			}
		}

		void MakeFirstToRender(IdWidget id) override
		{
			auto matchesId = [&id](const FrameDesc& desc)
			{
				return desc.Eq(id);
			};

			auto i = std::find_if(frameDescriptors.begin(), frameDescriptors.end(), matchesId);
			if (i != frameDescriptors.end())
			{
				TFrames::iterator firstOne = frameDescriptors.begin();
				std::swap(i, firstOne);
			}
		}

		void MakeLastToRender(IdWidget id) override
		{
			auto matchesId = [&id](const FrameDesc& desc)
			{
				return desc.Eq(id);
			};
			
			auto i = std::find_if(frameDescriptors.begin(), frameDescriptors.end(), matchesId);
			if (i != frameDescriptors.end())
			{
				TFrames::iterator lastOne = frameDescriptors.begin();
				std::advance(i, frameDescriptors.size() - 1);
				std::swap(i, lastOne);
			}
		}

		IGRPanelRoot& Root() override
		{
			return *this;
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGuiRetainedSupervisor* CreateGuiRetained(GuiRetainedConfig& config, IGuiRetainedCustodian& custodian)
	{
		return new ANON::GuiRetained(config, custodian);
	}
}