#include "dystopia.h"
#include <vector>
#include "rococo.renderer.h"

using namespace Rococo;
using namespace Dystopia;

namespace
{
	struct Pane
	{
		IUIPaneFactory& factory;
	};

	class UIStack: public IUIStackSupervisor, public ILock, public IScene
	{
	private:
		IUIPaneFactory* factory;
		std::vector<PaneBind> panes;
		bool isEnumerating;

	public:
		UIStack(): factory(nullptr), isEnumerating(false)
		{
		}

		~UIStack()
		{
			for (auto i : panes)
			{
				factory->FreeInstance(i.id, &i.pane);
			}
		}

		void Lock()
		{
			if (isEnumerating)
			{
				Throw(0, L"UI Stack is locked for enumeration");
			}
			isEnumerating = true;
		}

		void Unlock()
		{
			isEnumerating = false;
		}

		virtual void Free()
		{
			delete this;
		}

		virtual void OnFrameUpdated(const IUltraClock& clock)
		{
			Sync sync(*this);
			for (auto i = panes.rbegin(); isEnumerating &&  i != panes.rend(); ++i)
			{
				if (i->pane.OnFrameUpdated(clock) == PaneModality_Modal)
				{
					break;
				}
			}
		}

		virtual void OnKeyboardEvent(const KeyboardEvent& ke)
		{
			if (panes.empty()) return;
			Sync sync(*this);
			for (auto i = panes.rbegin(); isEnumerating && i != panes.rend(); ++i)
			{
				if (i->pane.OnKeyboardEvent(ke) == PaneModality_Modal)
				{
					break;
				}
			}
		}

		virtual void OnMouseEvent(const MouseEvent& me)
		{
			if (panes.empty()) return;
			Sync sync(*this);
			for (auto i = panes.rbegin(); isEnumerating && i != panes.rend(); ++i)
			{
				if (i->pane.OnMouseEvent(me) == PaneModality_Modal)
				{
					break;
				}
			}
		}

		virtual PaneBind PopTop()
		{
			Unlock();
			if (panes.empty()) Throw(0, L"UIStack::PopTop failed. No UI panes.");
			PaneBind top = panes.back();
			panes.pop_back();
			return top;
		}

		virtual PaneBind PushTop(ID_PANE id)
		{
			if (factory == nullptr) Throw(0, L"No factory has been assigned to the UI stack");

			auto* pane = factory->GetOrCreatePane(id);
			if (pane == nullptr)
			{
				Throw(0, L"UIStack::PushTop failed. PaneId %d yielded a null pointer.", id);
			}

			return PushTop(pane, id);
		}

		virtual PaneBind PushTop(IUIPaneSupervisor* pane, ID_PANE id)
		{
			Unlock();

			Sync sync(*this);
			panes.push_back(PaneBind{ *pane, id });
			return panes.back();
		}

		virtual PaneBind Top()
		{
			Sync sync(*this);
			if (panes.empty()) Throw(0, L"UIStack::Top failed. No UI panes.");
			return panes.back();
		}

		virtual RGBA GetClearColour() const
		{
			return RGBA(0.0f, 0.0f, 0.0f, 1.0f);
		}

		virtual void RenderGui(IGuiRenderContext& grc)
		{
			if (panes.empty()) return;
			Sync sync(*this);
			for (auto i = panes.begin(); isEnumerating && i != panes.end(); ++i)
			{
				i->pane.RenderGui(grc);
			}
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			if (panes.empty()) return;
			Sync sync(*this);
			for (auto i = panes.begin(); isEnumerating && i != panes.end(); ++i)
			{
				i->pane.RenderObjects(rc);
			}
		}

		virtual void SetFactory(IUIPaneFactory& factory)
		{
			this->factory = &factory;
		}

		virtual IScene& Scene()
		{
			return *this;
		}
	};
}

namespace Dystopia
{
	IUIStackSupervisor* CreateUIStack()
	{
		return new UIStack();
	}
}