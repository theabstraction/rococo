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
			case WM_ERASEBKGND:
				return TRUE;
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_COMMAND:
				return eventHandler.OnCommand(hWnd, uMsg, wParam, lParam);
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

		LRESULT OnSize(HWND, WPARAM wParam, LPARAM lParam)
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
			SetWindowTextA(hWndEditor, "");
		}

		virtual void AppendText(COLORREF foreground, COLORREF background, cstr text, size_t nChars)
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

			size_t len = min(rlen(text), nChars);

			const char* source = text;

			while (len > 0)
			{
				enum { SEGMENT_CAPACITY = 4096 };
				char segmentBuffer[SEGMENT_CAPACITY];

				size_t delta = (len >= SEGMENT_CAPACITY) ? SEGMENT_CAPACITY - 1 : len;

				memcpy(segmentBuffer, source, delta);
				segmentBuffer[delta] = 0;
				source += delta;
				len -= delta;

				SendMessage(hWndEditor, EM_EXSETSEL, 0, (LPARAM)&cr);
				SendMessage(hWndEditor, EM_REPLACESEL, 0, (LPARAM)segmentBuffer);
			}
		}

		static LRESULT Subclassproc(
			HWND hWnd,
			UINT uMsg,
			WPARAM wParam,
			LPARAM lParam,
			UINT_PTR uIdSubclass,
			DWORD_PTR dwRefData
		)
		{
			UNUSED(uIdSubclass);

			if (uMsg == WM_RBUTTONUP)
			{
				auto xPos = GET_X_LPARAM(lParam);
				auto yPos = GET_Y_LPARAM(lParam);
				((RichEditor*)dwRefData)->eventHandler.OnRightButtonUp(Vec2i{ xPos, yPos });
				return TRUE;
			}

			return DefSubclassProc(hWnd, uMsg, wParam, lParam);
		}

		void Construct(const WindowConfig& editorConfig, IWindow& parent)
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

			hWndEditor = CreateWindowIndirect("RichEdit20W", editorConfigCorrected, nullptr);
			SetDlgCtrlID(hWndEditor, 1001);

			SetControlFont(hWndEditor);

			if (!SetWindowSubclass(hWndEditor, Subclassproc, 1, (DWORD_PTR)this))
			{
				Throw(0, "Could not subclass richtext editor");
			}
		}

		virtual int32 LineCount() const
		{
			return (int32)SendMessage(hWndEditor, EM_LINEFROMCHAR, (WPARAM)-1, 0);
		}

		virtual int32 GetFirstVisibleLine() const
		{
			int32 y = (int32)SendMessage(hWndEditor, EM_GETFIRSTVISIBLELINE, 0, 0);
			return y;
		}

		virtual void ScrollTo(int32 row)
		{
			int32 y = GetFirstVisibleLine();
			SendMessage(hWndEditor, EM_LINESCROLL, 0, -y);
			SendMessage(hWndEditor, EM_LINESCROLL, 0, row);
		}

		~RichEditor()
		{
			DestroyWindow(hWndEditor);
			DestroyWindow(hWnd);
		}

		void OnPretranslateMessage(MSG&) override
		{

		}
	public:
		static RichEditor* Create(const WindowConfig& editorConfig, IWindow& parent, IRichEditorEvents& eventHandler)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			RichEditor* p = new RichEditor(eventHandler);
			p->Construct(editorConfig, parent);
			return p;
		}

		virtual void SetTooltip(cstr name, cstr text)
		{
			UNUSED(name);
			UNUSED(text);
			// Not implemented
		}

		virtual void Hilight(const Vec2i& start, const Vec2i& end, RGBAb background, RGBAb foreground)
		{
			int startIndex = (int)SendMessage(hWndEditor, EM_LINEINDEX, start.y, 0) + start.x;
			int endIndex = (int)SendMessage(hWndEditor, EM_LINEINDEX, end.y, 0) + end.x;

			SendMessage(hWndEditor, EM_SETSEL, startIndex, endIndex);

			CHARFORMAT2W format;
			ZeroMemory(&format, sizeof(format));
			format.cbSize = sizeof(format);
			format.dwMask = CFM_BOLD | CFM_COLOR | CFM_BACKCOLOR;
			format.dwEffects = CFE_BOLD;
			format.crTextColor = RGB(foreground.red, foreground.green, foreground.blue);
			format.crBackColor = RGB(background.red, background.green, background.blue);

			SendMessage(hWndEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);

			SendMessage(hWndEditor, EM_SETSEL, endIndex, endIndex);
			SendMessage(hWndEditor, EM_SCROLLCARET, 0, 0);
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

		void Free() override
		{
			delete this;
		}
	};
}