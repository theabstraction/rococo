#include "dystopia.h"
#include <vector>
#include "rococo.renderer.h"
#include "rococo.ui.h"
#include "dystopia.post.h"
#include "dystopia.ui.h"

using namespace Rococo;
using namespace Rococo::Post;
using namespace Dystopia;

namespace
{
	struct Pane
	{
		IUIPaneFactory& factory;
	};

	using namespace Rococo::Post;

	class UIStack: public IUIStackSupervisor, public ILock, public IScene, public IRecipient
	{
	private:
		IUIPaneFactory* factory;
		std::vector<PaneBind> panes;
		bool isEnumerating;
		Post::IPostbox& postbox;
	public:
		UIStack(Post::IPostbox& _postbox): postbox(_postbox), factory(nullptr), isEnumerating(false)
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

		void OnTimestep(const TimestepEvent& timestep)
		{
			Sync sync(*this);
			for (auto i = panes.rbegin(); isEnumerating &&  i != panes.rend(); ++i)
			{
				if (i->pane.OnTimestep(timestep) == Relay_None)
				{
					break;
				}
			}
		}

		void OnKeyboardEvent(const KeyboardEvent& ke)
		{
			if (panes.empty()) return;
			Sync sync(*this);
			for (auto i = panes.rbegin(); isEnumerating && i != panes.rend(); ++i)
			{
				if (i->pane.OnKeyboardEvent(ke) == Relay_None)
				{
					break;
				}
			}
		}

		void OnMouseEvent(const MouseEvent& me)
		{
			if (panes.empty()) return;
			Sync sync(*this);
			for (auto i = panes.rbegin(); isEnumerating && i != panes.rend(); ++i)
			{
				if (i->pane.OnMouseEvent(me) == Relay_None)
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
			pane->OnTop();
			return panes.back();
		}

		virtual PaneBind Top()
		{
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

		virtual void OnCreated()
		{
			postbox.Subscribe(POST_TYPE_KEYBOARD_EVENT, this);
			postbox.Subscribe(POST_TYPE_MOUSE_EVENT, this);
			postbox.Subscribe(POST_TYPE_TIMESTEP, this);
		}

		virtual void OnPost(const Mail& mail)
		{
			auto* k = Post::InterpretAs<KeyboardEvent>(mail);
			if (k) OnKeyboardEvent(*k);
			
			auto* m = Post::InterpretAs<MouseEvent>(mail);
			if (m) OnMouseEvent(*m);
			
			auto* t = Post::InterpretAs<TimestepEvent>(mail);
			if (t) OnTimestep(*t);
		}
	};
}

namespace Dystopia
{
	IUIStackSupervisor* CreateUIStack(IPostbox& postbox)
	{
		return new UIStack(postbox);
	}
}