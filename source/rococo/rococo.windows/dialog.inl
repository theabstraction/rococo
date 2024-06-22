namespace Rococo::Windows::Impl
{
	class DialogWindowImpl : public IDialogSupervisor, private IWindowHandler
	{
	private:
		HWND hWnd;
		TWindows children;

		IWindowHandler* modalHandler;
		IWindowHandler* modelessHandler;

		DialogWindowImpl(IWindowHandler* _modelessHandler) :
			modalHandler(nullptr),
			modelessHandler(_modelessHandler),
			hWnd(nullptr)
		{
		}

		~DialogWindowImpl()
		{
			DestroyWindow(hWnd);
			DeleteAll(children);
		}

		void OnPretranslateMessage(MSG&) override
		{

		}

		void OnModal() override
		{

		}

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			if (modalHandler)
			{
				return modalHandler->OnMessage(hWnd, uMsg, wParam, lParam);
			}
			else if (modelessHandler)
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

		DWORD BlockModal(IModalControl& control, HWND ownerWindow, IWindowHandler* overrideHandler) override
		{
			this->modalHandler = overrideHandler;

			control.OnEnterModal();

			ShowWindow(hWnd, SW_SHOW);
			EnableWindow(ownerWindow, FALSE);
			SetForegroundWindow(hWnd);

			modalHandler->OnModal();

			while (control.IsRunning())
			{
				DWORD waitPeriod = control.OnIdle();

				MsgWaitForMultipleObjectsEx(0, nullptr, waitPeriod, QS_ALLINPUT, MWMO_ALERTABLE);
				MSG msg;
				while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
				{
					if (msg.hwnd == ownerWindow)
					{
						BringWindowToTop(hWnd);
					}
					overrideHandler->OnPretranslateMessage(msg);
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}

			EnableWindow(ownerWindow, TRUE);
			ShowWindow(hWnd, SW_HIDE);

			BringWindowToTop(ownerWindow);

			this->modalHandler = nullptr;

			return control.OnExitModal();
		}
	public:
		void ClearChildren() override
		{
			DeleteAll(children);
		}

		static DialogWindowImpl* Create(const WindowConfig& config, IWindowHandler* modelessHandler = nullptr)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			DialogWindowImpl* p = new DialogWindowImpl(modelessHandler);
			p->hWnd = CreateWindowIndirect(customClassName, config, &p->Handler());
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