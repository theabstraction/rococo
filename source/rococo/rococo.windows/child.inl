namespace Rococo::Windows
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
			CloseWindow(hWnd);
		}

		void OnModal() override
		{

		}

		void OnPretranslateMessage(MSG&) override
		{

		}

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
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

		IWindowHandler& Handler() override
		{
			return *this;
		}

	public:
		void ClearChildren() override
		{
			DeleteAll(children);
		}

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
			ChildWindowImpl* p = new ChildWindowImpl(nullptr);
			p->hWnd = CreateWindowIndirect(childClassName, config, nullptr);
			return p;
		}

		operator HWND () const override
		{
			return hWnd;
		}

		void Free() override
		{
			delete this;
		}

		IWindowSupervisor* AddChild(const WindowConfig& _childConfig, cstr className, ControlId id) override
		{
			WindowConfig childConfig = _childConfig;
			childConfig.hWndParent = hWnd;

			ChildWindowImpl* p = ChildWindowImpl::Create(childConfig, className);
			SetWindowLongPtr(p->hWnd, GWLP_ID, static_cast<LONG_PTR>(static_cast<DWORD_PTR>(id)));
			children.push_back(p);
			return p;
		}

		IParentWindowSupervisor* AddChild(const WindowConfig& _childConfig, ControlId id, IWindowHandler* modelessHandler) override
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