namespace
{
	template<class PTRTYPE>
	struct FnBind
	{
		void* context;
		PTRTYPE type;

		FnBind() :
			context(nullptr), type(nullptr)
		{
		}

		FnBind(void* _context, PTRTYPE _type) :
			context(_context), type(_type)
		{
		}

		void Set(void* context, PTRTYPE type)
		{
			this->context = context;
			this->type = type;
		}
	};

	class WiredHandler : public IWiredWindowHandler
	{
	private:
		FnBind<FN_OnControlCommand> bindControlCommand;
		FnBind<FN_OnMenuCommand> bindMenuCommand;
		FnBind<FN_OnAcceleratorCommand> bindAcceleratorCommand;
		FnBind<FN_OnClose> bindClose;
		FnBind<FN_OnIdle> bindIdle;
		FnBind<FN_OnSize> bindOnSize;

	public:
		WiredHandler()
		{
		}

		~WiredHandler()
		{
		}

		void RouteIdle(void* context, FN_OnIdle fn)
		{
			bindIdle.Set(context, fn);
		}

		void RouteControlCommand(void* context, FN_OnControlCommand fn)
		{
			bindControlCommand.Set(context, fn);
		}

		virtual void RouteMenuCommand(void* context, FN_OnMenuCommand fn)
		{
			bindMenuCommand.Set(context, fn);
		}

		virtual void RouteSize(void* context, FN_OnSize fn)
		{
			bindOnSize.Set(context, fn);
		}

		virtual void RouteAcceleratorCommand(void* context, FN_OnAcceleratorCommand fn)
		{
			bindAcceleratorCommand.Set(context, fn);
		}

		virtual void RouteClose(void* context, FN_OnClose fn)
		{
			bindClose.Set(context, fn);
		}

		void OnControlCommand(HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode)
		{
			if (bindControlCommand.type != nullptr)
			{
				bindControlCommand.type(bindControlCommand.context, hWnd, notificationCode, id, hControlCode);
			}
		}

		DWORD OnIdle()
		{
			if (bindIdle.type != nullptr)
			{
				return bindIdle.type(bindIdle.context);
			}
			else
			{
				return 1000;
			}
		}

		void OnMenuCommand(HWND hWnd, DWORD id)
		{
			if (bindMenuCommand.type != nullptr)
			{
				bindMenuCommand.type(bindMenuCommand.context, hWnd, id);
			}
		}

		void OnAcceleratorCommand(HWND hWnd, DWORD id)
		{
			if (bindAcceleratorCommand.type != nullptr)
			{
				bindAcceleratorCommand.type(bindAcceleratorCommand.context, hWnd, id);
			}
		}

		void OnClose(HWND hWnd)
		{
			if (bindClose.type != nullptr)
			{
				bindClose.type(bindClose.context, hWnd);
			}
		}

		void OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			HWND hWndControl = (HWND)lParam;
			if (hWndControl != nullptr)
			{
				OnControlCommand(hWnd, HIWORD(wParam), LOWORD(wParam), hWndControl);
			}
			else
			{
				if (HIWORD(wParam) == 0)
				{
					OnMenuCommand(hWnd, LOWORD(wParam));
				}
				else
				{
					OnAcceleratorCommand(hWnd, LOWORD(wParam));
				}
			}
		}

		void OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			bindOnSize.type(bindOnSize.context, hWnd, Vec2i(LOWORD(lParam), HIWORD(lParam)), (RESIZE_TYPE)wParam);
		}

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{
			case WM_SIZE:
				OnSize(hWnd, wParam, lParam);
				break;
			case WM_COMMAND:
				OnCommand(hWnd, wParam, lParam);
				return 0L;
			case WM_CLOSE:
				OnClose(hWnd);
				return 0L;
			}

			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		virtual void Free()
		{
			delete this;
		}
	};
}