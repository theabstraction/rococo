namespace
{
	class RichEditor : public IRichEditor, private IWindowHandler
	{
	private:
		HWND hWnd;
		HWND hWndEditor;
		IRichEditorEvents& eventHandler;

		RichEditor(IRichEditorEvents& _eventHandler) :
			hWnd(nullptr),
			hWndEditor(nullptr),
			eventHandler(_eventHandler)
		{
		}

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_NOTIFY:
			{
							  // Disabled in READONLY mode
							  MSGFILTER* mf = (MSGFILTER*)lParam;
							  if (mf->nmhdr.code == EN_MSGFILTER)
							  {
								  if (mf->msg == WM_NCRBUTTONUP)
								  {
									  Vec2i pos{ GET_X_LPARAM(mf->lParam), GET_Y_LPARAM(mf->lParam) };
									  eventHandler.OnRightButtonUp(pos);
									  return 0L;
								  }
							  }
			}
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
				MoveWindow(hWndEditor, 0, 0, width, height, TRUE);
				break;
			}

			return 0L;
		}

		void ResetContent()
		{
			SetWindowText(hWndEditor, L"");
		}

		virtual void AppendText(COLORREF foreground, COLORREF background, const wchar_t* text, size_t nChars)
		{
			CHARFORMAT2 c;
			memset(&c, 0, sizeof(c));
			c.cbSize = sizeof(c);
			c.dwMask = CFM_COLOR | CFM_BACKCOLOR;
			c.crBackColor = background;
			c.crTextColor = foreground;
			SendMessage(hWndEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&c);

			CHARRANGE cr;
			cr.cpMin = -1;
			cr.cpMax = -1;

			size_t len = min(wcslen(text), nChars);
			wchar_t* segmentBuffer = (wchar_t*)_alloca(sizeof(wchar_t)* len + 2);
			SafeCopy(segmentBuffer, len + 1, text, len);

			// hwnd = rich edit hwnd
			SendMessage(hWndEditor, EM_EXSETSEL, 0, (LPARAM)&cr);
			SendMessage(hWndEditor, EM_REPLACESEL, 0, (LPARAM)segmentBuffer);
		}

		void Construct(const WindowConfig& editorConfig, IParentWindowSupervisor& parent)
		{
			WindowConfig containerConfig = editorConfig;
			containerConfig.style = WS_CHILD | WS_VISIBLE;
			containerConfig.exStyle = 0;
			containerConfig.hWndParent = parent;

			hWnd = CreateWindowIndirect(customClassName, containerConfig, static_cast<IWindowHandler*>(this));

			WindowConfig editorConfigCorrected = editorConfig;
			editorConfigCorrected.left = 0;
			editorConfigCorrected.top = 0;
			editorConfigCorrected.hWndParent = hWnd;
			editorConfigCorrected.style |= WS_CHILD | WS_VISIBLE;

			hWndEditor = CreateWindowIndirect(L"RichEdit20W", editorConfigCorrected, nullptr);
			SetDlgCtrlID(hWndEditor, 1001);

			SetControlFont(hWndEditor);
		}

		virtual size_t LineCount() const
		{
			return SendMessage(hWndEditor, EM_LINEFROMCHAR, -1, 0);
		}

		virtual void ScrollTo(size_t lineNumber)
		{
			SendMessage(hWndEditor, EM_LINESCROLL, 0, lineNumber - 4);
		}
	public:
		static RichEditor* Create(const WindowConfig& editorConfig, IParentWindowSupervisor& parent, IRichEditorEvents& eventHandler)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			RichEditor* p = new RichEditor(eventHandler);
			p->Construct(editorConfig, parent);
			return p;
		}

		virtual HWND EditorHandle() const
		{
			return hWndEditor;
		}

		virtual IWindowHandler& Handler()
		{
			return *this;
		}

		virtual operator HWND () const
		{
			return hWnd;
		}

		virtual HWND EditBoxHandle() const
		{
			return hWndEditor;
		}

		virtual void Free()
		{
			delete this;
		}
	};
}