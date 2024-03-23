namespace Rococo::Windows
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
		FnBind<FN_OnPreTranslate> bindPreTranslate;
		FnBind<FN_OnMessage> bindOnMessage;

	public:
		WiredHandler()
		{
		}

		~WiredHandler()
		{
		}

		void RouteIdle(void* context, FN_OnIdle f)
		{
			bindIdle.Set(context, f);
		}

		void RouteControlCommand(void* context, FN_OnControlCommand f)
		{
			bindControlCommand.Set(context, f);
		}

		virtual void RouteMenuCommand(void* context, FN_OnMenuCommand f)
		{
			bindMenuCommand.Set(context, f);
		}

		virtual void RouteSize(void* context, FN_OnSize f)
		{
			bindOnSize.Set(context, f);
		}

		virtual void RouteAcceleratorCommand(void* context, FN_OnAcceleratorCommand f)
		{
			bindAcceleratorCommand.Set(context, f);
		}

		virtual void RouteClose(void* context, FN_OnClose f)
		{
			bindClose.Set(context, f);
		}

		void OnControlCommand(HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode)
		{
			if (bindControlCommand.type != nullptr)
			{
				bindControlCommand.type(bindControlCommand.context, hWnd, notificationCode, id, hControlCode);
			}
		}

		void RoutePreTranslate(void* context, FN_OnPreTranslate f)
		{
			bindPreTranslate.Set(context, f);
		}

		void RouteMessage(void* context, FN_OnMessage f)
		{
			bindOnMessage.Set(context, f);
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
			bindOnSize.type(bindOnSize.context, hWnd, Vec2i{ LOWORD(lParam), HIWORD(lParam) }, (RESIZE_TYPE)wParam);
		}

		void OnPretranslateMessage(MSG& msg) override
		{
			if (bindPreTranslate.type != nullptr)
			{
				bindPreTranslate.type(bindPreTranslate.context, msg);
			}
		}

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			if (bindOnMessage.type != nullptr)
			{
				bool wasHandled = false;
				LRESULT result = bindOnMessage.type(bindOnMessage.context, hWnd, uMsg, wParam, lParam, OUT wasHandled);
				if (wasHandled)
				{
					return result;
				}
			}

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