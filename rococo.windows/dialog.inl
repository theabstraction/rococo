namespace
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

      virtual void OnPretranslateMessage(MSG& msg)
      {

      }

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

		virtual IWindowHandler& Handler()
		{
			return *this;
		}

		virtual DWORD BlockModal(IModalControl& control, HWND ownerWindow, IWindowHandler* overrideHandler)
		{
			this->modalHandler = overrideHandler;

			control.OnEnterModal();

			ShowWindow(hWnd, SW_SHOW);
			EnableWindow(ownerWindow, FALSE);
         SetForegroundWindow(hWnd);

			while (control.IsRunning())
			{
				DWORD waitPeriod = control.OnIdle();

				MsgWaitForMultipleObjectsEx(0, nullptr, waitPeriod, QS_ALLINPUT, MWMO_ALERTABLE);
				MSG msg;
				while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
				{
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
		static DialogWindowImpl* DialogWindowImpl::Create(const WindowConfig& config, bool autoDestroy, IWindowHandler* modelessHandler = nullptr)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			DialogWindowImpl* p = new DialogWindowImpl(modelessHandler);
			p->hWnd = CreateWindowIndirect(customClassName, config, &p->Handler());
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

		virtual IWindowSupervisor* AddChild(const WindowConfig& _childConfig, LPCWSTR className, ControlId id)
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