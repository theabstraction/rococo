#include <rococo.gui.retained.h>
#include <vector>
#include <stdio.h>
#include <string>
#include <algorithm>
#include <rococo.maths.h>
#include <unordered_map>

using namespace Rococo;
using namespace Rococo::Gui;

namespace Rococo::Gui
{
	IGRLayoutSupervisor* CreateFullScreenLayout();
	IGRMainFrameSupervisor* CreateGRMainFrame(IGRPanel& panel);
	IGRPanelSupervisor* CreatePanel(IGRPanelRoot& root, IGRPanelSupervisor* parent);

	bool operator == (const GuiRect& a, const GuiRect& b)
	{
		return a.left == b.left && a.right == b.right && a.top == b.top && a.bottom == b.bottom;
	}
}

namespace ANON
{
	struct GuiRetained: IGuiRetainedSupervisor, IGRPanelRoot
	{
		IGRCustodian& custodian;
		GRConfig config;
		AutoFree<ISchemeSupervisor> scheme = CreateScheme();
		std::unordered_map<int64, IGRPanel*> mapIdToPanel;
		int queryDepth = 0;

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

		GuiRetained(GRConfig& _config, IGRCustodian& _custodian) : config(_config), custodian(_custodian)
		{

		}

		virtual ~GuiRetained()
		{
			for (auto d : frameDescriptors)
			{
				d.panel->Free();
			}
		}

		IGRWidget* FindWidget(int64 panelId)
		{
			auto i = mapIdToPanel.find(panelId);
			return i != mapIdToPanel.end() ? &i->second->Widget() : nullptr;
		}

		void NotifyPanelDeleted(int64 uniqueId)
		{
			mapIdToPanel.erase(uniqueId);
		}

		void Free() override
		{
			delete this;
		}

		IGRMainFrame& BindFrame(IdWidget id) override
		{
			if (queryDepth > 0)
			{
				custodian.RaiseError(GRErrorCode::RecursionLocked, __FUNCTION__, "The GUI Retained API is locked for a recursive query. BindFrame cannot be executed at this time");
				IGRMainFrame* frame = nullptr;
				return *frame;
			}

			IGRMainFrame* oldFrame = TryGetFrame(id);
			if (oldFrame)
			{
				return *oldFrame;
			}

			auto* newFramePanel = CreatePanel(*this, nullptr);
			auto* newFrame = CreateGRMainFrame(*newFramePanel);
			newFramePanel->SetWidget(*newFrame);
			frameDescriptors.push_back(FrameDesc{ newFramePanel, newFrame, std::string(id.Name) });
			mapIdToPanel.try_emplace(newFramePanel->Id(), newFramePanel);
			return *newFrame;
		}

		void DeleteFrame(IdWidget id) override
		{
			if (queryDepth > 0)
			{
				custodian.RaiseError(GRErrorCode::RecursionLocked, __FUNCTION__, "The GUI Retained API is locked for a recursive query. FrameDelete cannot be executed at this time");
				return;
			}

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

		struct RecursionGuard
		{
			GuiRetained& This;

			RecursionGuard(GuiRetained& _This) : This(_This)
			{
				This.queryDepth++;
			}
			~RecursionGuard()
			{
				This.queryDepth--;
			}
		};

		void LayoutFrames()
		{
			RecursionGuard guard(*this);
			for (auto& d : frameDescriptors)
			{
				d.frame->Layout(lastLayedOutScreenDimensions);
				d.panel->InvalidateLayout(false);
				d.panel->LayoutRecursive({ 0,0 });
			}
		}

		void RenderGui(IGRRenderContext& g) override
		{
			for (auto& d : frameDescriptors)
			{
				auto screenDimensions = g.ScreenDimensions();
				Vec2i topLeft = { screenDimensions.left, screenDimensions.top };

				if (lastLayedOutScreenDimensions != screenDimensions || d.panel->RequiresLayout())
				{
					lastLayedOutScreenDimensions = screenDimensions;
					LayoutFrames();
				}

				RecursionGuard guard(*this);
				d.panel->RenderRecursive(g);
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

		IGRWidget& AddWidget(IGRPanel& parent, IGRWidgetFactory& factory)
		{
			auto& panel = parent.AddChild();
			auto& widget = factory.CreateWidget(panel);
			auto& superPanel = static_cast<IGRPanelSupervisor&>(panel);
			superPanel.SetWidget(widget);
			mapIdToPanel.try_emplace(panel.Id(), &panel);
			return widget;
		}

		IGuiRetained& GR()
		{
			return *this;
		}

		bool isVisible = true;

		void SetVisible(bool isVisible) override
		{
			this->isVisible = isVisible;
		}

		bool IsVisible() const override
		{
			return isVisible && !frameDescriptors.empty();
		}

		EventRouting RouteCursorClickEvent(CursorEvent& ev) override
		{
			RecursionGuard guard(*this);

			if (captureId >= 0)
			{
				auto* widget = FindWidget(captureId);
				if (widget)
				{
					auto& panelSupervisor = static_cast<IGRPanelSupervisor&>(widget->Panel());
					return panelSupervisor.RouteCursorClickEvent(ev, false);
				}
			}

			for (auto d = frameDescriptors.rbegin(); d != frameDescriptors.rend(); ++d)
			{
				auto routing = d->panel->RouteCursorClickEvent(ev, true);
				if (routing == EventRouting::Terminate)
				{
					return EventRouting::Terminate;
				}
			}

			return EventRouting::NextHandler;
		}

		int64 captureId = -1;

		void CaptureCursor(IGRPanel& panel) override
		{
			captureId = panel.Id();
		}

		int64 CapturedPanelId() const override
		{
			return captureId;
		}

		void ReleaseCursor() override
		{
			captureId = -1;
		}

		EventRouting RouteCursorMoveEvent(CursorEvent& ev) override
		{
			RecursionGuard guard(*this);

			if (captureId >= 0)
			{
				auto* widget = FindWidget(captureId);
				if (widget)
				{
					auto& panelSupervisor = static_cast<IGRPanelSupervisor&>(widget->Panel());
					auto routing = panelSupervisor.RouteCursorMoveEvent(ev);
					if (routing == EventRouting::Terminate)
					{
						return routing;
					}
				}
			}

			for (auto d = frameDescriptors.rbegin(); d != frameDescriptors.rend(); ++d)
			{
				auto routing = d->panel->RouteCursorMoveEvent(ev);
				if (routing == EventRouting::Terminate)
				{
					return EventRouting::Terminate;
				}
			}

			return EventRouting::NextHandler;
		}

		IGRCustodian& Custodian() override
		{
			return custodian;
		}
	};
}

namespace Rococo
{
	// Copied from the maths lib. We want our DLL to depend on as few libraries as possible to make it easier to re-use in third party apps
	bool IsPointInRect(Vec2i p, const GuiRect& rect)
	{
		return (p.x >= rect.left && p.x <= rect.right && p.y >= rect.top && p.y <= rect.bottom);
	}
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGuiRetainedSupervisor* CreateGuiRetained(GRConfig& config, IGRCustodian& custodian)
	{
		return new ANON::GuiRetained(config, custodian);
	}
}