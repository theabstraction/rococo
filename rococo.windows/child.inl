namespace
{
	class ChildWindowImpl : public IParentWindowSupervisor, private IWindowHandler
	{
	public:
		HWND hWnd;
		TWindows children;
	private:
		IWindowHandler* modelessHandler;

		ChildWindowImpl(IWindowHandler* _modelessHandler) :
			modelessHandler(_modelessHandler),
			hWnd(nullptr)
		{
		}

		~ChildWindowImpl()
		{
			DeleteAll(children);
		}

      virtual void OnPretranslateMessage(MSG& msg)
      {

      }

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			if (modelessHandler)
			{
				return modelessHandler->OnMessage(hWnd, uMsg, wParam, lParam);
			}
			else
			{
				return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}
		}

		virtual IWindowHandler& Handler()
		{
			return *this;
		}

	public:
		static ChildWindowImpl* Create(const WindowConfig& config, IWindowHandler* modelessHandler = nullptr)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			ChildWindowImpl* p = new ChildWindowImpl(modelessHandler);
			p->hWnd = CreateWindowIndirect(customClassName, config, &p->Handler());
			return p;
		}

		static ChildWindowImpl* Create(const WindowConfig& config, cstr childClassName)
		{
			ChildWindowImpl* p = new ChildWindowImpl(false);
			p->hWnd = CreateWindowIndirect(childClassName, config, nullptr);
			return p;
		}

		virtual operator HWND () const
		{
			return hWnd;
		}

		virtual void Free()
		{
			delete this;
		}

		virtual IWindowSupervisor* AddChild(const WindowConfig& _childConfig, cstr className, ControlId id)
		{
			WindowConfig childConfig = _childConfig;
			childConfig.hWndParent = hWnd;

			ChildWindowImpl* p = ChildWindowImpl::Create(childConfig, className);
			SetWindowLongPtr(p->hWnd, GWLP_ID, static_cast<LONG_PTR>(static_cast<DWORD_PTR>(id)));
			children.push_back(p);
			return p;
		}

		virtual IParentWindowSupervisor* AddChild(const WindowConfig& _childConfig, ControlId id, IWindowHandler* modelessHandler)
		{
			WindowConfig childConfig = _childConfig;
			childConfig.hWndParent = hWnd;

			ChildWindowImpl* p = ChildWindowImpl::Create(childConfig, modelessHandler);
			SetWindowLongPtr(p->hWnd, GWLP_ID, static_cast<LONG_PTR>(static_cast<DWORD_PTR>(id)));
			children.push_back(p);
			return p;
		}
	};
}