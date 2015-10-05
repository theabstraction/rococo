namespace
{
	class TrackBarSupervisor : public ITrackBarSupervisor, private IWindowHandler
	{
	private:
		HWND hWnd;
		HWND hWndBar;
		ITrackBarHandler& handler;

		TrackBarSupervisor(ITrackBarHandler& _handler) :
			hWnd(nullptr),
			hWndBar(nullptr),
			handler(_handler)
		{
		}

		void OnScroll(SCROLL_BAR_MESSAGE msg)
		{
			LRESULT pos = SendMessage(hWndBar, TBM_GETPOS, 0, 0);
			handler.OnMove((int) pos);
		}

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_HSCROLL:
				OnScroll((SCROLL_BAR_MESSAGE)LOWORD(wParam));
				return 0L;
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		LRESULT OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			DWORD width = LOWORD(lParam);
			DWORD height = HIWORD(lParam);

			switch (wParam)
			{
			case SIZE_RESTORED:
			case SIZE_MAXSHOW:
			case SIZE_MAXIMIZED:
			case SIZE_MAXHIDE:
				MoveWindow(hWndBar, 0, 0, width, height, TRUE);
				break;
			}

			return 0L;
		}

	public:
		static TrackBarSupervisor* Create(const WindowConfig& barConfig, IParentWindowSupervisor& parent, ITrackBarHandler& handler)
		{
			TrackBarSupervisor* p = new TrackBarSupervisor(handler);

			WindowConfig containerConfig = barConfig;
			containerConfig.style = WS_CHILD | WS_VISIBLE;
			containerConfig.exStyle = 0;
			containerConfig.hWndParent = parent;
			p->hWnd = CreateWindowIndirect(customClassName, containerConfig, static_cast<IWindowHandler*>(p));

			WindowConfig configCorrected = barConfig;
			configCorrected.left = 0;
			configCorrected.top = 0;
			configCorrected.hWndParent = p->hWnd;
			configCorrected.style |= WS_CHILD | WS_VISIBLE;

			p->hWndBar = CreateWindowIndirect(TRACKBAR_CLASS, configCorrected, nullptr);
			return p;
		}

		virtual IWindowHandler& Handler()
		{
			return *this;
		}

		virtual operator HWND () const
		{
			return hWnd;
		}

		virtual void Free()
		{
			delete this;
		}

		virtual void SetRange(int mininma, int maxima)
		{
			SendMessage(hWndBar, TBM_SETRANGE, TRUE, MAKELONG(mininma, maxima));
		}

		virtual void SetPageSize(int pageSize)
		{
			SendMessage(hWndBar, TBM_SETPAGESIZE, 0, pageSize);
		}

		virtual void SetPosition(int pos)
		{
			SendMessage(hWndBar, TBM_SETPOS, TRUE, pos);
		}

		virtual HWND TrackerHandle() const
		{
			return hWndBar;
		}
	};
}