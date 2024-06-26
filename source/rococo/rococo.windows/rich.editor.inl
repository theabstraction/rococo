namespace Rococo::Windows
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

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			switch (uMsg)
			{
			case WM_ERASEBKGND:
				return TRUE;
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_COMMAND:
				return eventHandler.RichEditor_OnCommand(hWnd, uMsg, wParam, lParam, *this);
			case WM_NOTIFY:
			{
				// Disabled in READONLY mode
				MSGFILTER* mf = (MSGFILTER*)lParam;
				if (mf->nmhdr.code == EN_MSGFILTER)
				{
					if (mf->msg == WM_NCRBUTTONUP)
					{
						Vec2i pos{ GET_X_LPARAM(mf->lParam), GET_Y_LPARAM(mf->lParam) };
						eventHandler.RichEditor_OnRightButtonUp(pos, *this);
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
			hilightStart.x = -1;
		}

		void AppendText(COLORREF foreground, COLORREF background, cstr text, size_t nChars) override
		{
			CHARFORMAT2 c;
			memset(&c, 0, sizeof(c));
			c.cbSize = sizeof(c);
			c.dwMask = CFM_COLOR | CFM_BACKCOLOR;
			c.crBackColor = background;
			c.crTextColor = foreground;

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

				CHARRANGE rangeBeforeAppend;
				rangeBeforeAppend.cpMin = -1; // This will give the starting character for the range of added characters
				rangeBeforeAppend.cpMax = -1;
				SendMessage(hWndEditor, EM_GETSEL, (WPARAM)&rangeBeforeAppend.cpMin, (LPARAM)&rangeBeforeAppend.cpMax);

				SendMessage(hWndEditor, EM_REPLACESEL, 0, (LPARAM)segmentBuffer);
				
				CHARRANGE rangeAfterAppend;
				rangeAfterAppend.cpMin = -1;
				rangeAfterAppend.cpMax = -1; // This will give the end character positiion for the range of added characters
				SendMessage(hWndEditor, EM_GETSEL, (WPARAM) &rangeAfterAppend.cpMin, (LPARAM) &rangeAfterAppend.cpMax);

				// Select everything we just added
				CHARRANGE cr4;
				cr4.cpMin = rangeBeforeAppend.cpMin;
				cr4.cpMax = rangeAfterAppend.cpMax;
				SendMessage(hWndEditor, EM_EXSETSEL, 0, (LPARAM)&cr4);

				// Then assign the colours
				SendMessage(hWndEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&c);
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
				auto* This = ((RichEditor*)dwRefData);
				This->eventHandler.RichEditor_OnRightButtonUp(Vec2i{ xPos, yPos }, *This);
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

		void OnModal() override
		{

		}

		void OnPretranslateMessage(MSG&) override
		{

		}

		ColourScheme scheme;
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

		void SetColourSchemeRecursive(const ColourScheme& scheme) override
		{
			this->scheme = scheme;

			CHARRANGE cr;
			cr.cpMin = 0;
			cr.cpMax = -1;

			SendMessage(hWndEditor, EM_EXSETSEL, 0, (LPARAM)&cr);

			CHARFORMAT2 c;
			memset(&c, 0, sizeof(c));
			c.cbSize = sizeof(c);
			c.dwMask = CFM_COLOR | CFM_BACKCOLOR;
			c.crBackColor = ToCOLORREF(scheme.backColour);
			c.crTextColor = ToCOLORREF(scheme.foreColour);
			SendMessage(hWndEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&c);
			SendMessage(hWndEditor, EM_SETBKGNDCOLOR, 0, c.crBackColor);

			if (hilightStart.x >= 0)
			{
				Hilight(hilightStart, hilightEnd);
			}

			InvalidateRect(hWndEditor, NULL, TRUE);
		}

		void SetTooltip(cstr name, cstr text) override 
		{
			UNUSED(name);
			UNUSED(text);
			// Not implemented
		}

		Vec2i hilightStart = { -1,-1 };
		Vec2i hilightEnd =   { -1, -1 };

		void Hilight(const Vec2i& start, const Vec2i& end) override
		{
			hilightStart = start;
			hilightEnd = end;

			int startIndex = (int)SendMessage(hWndEditor, EM_LINEINDEX, start.y, 0) + start.x;
			int endIndex = (int)SendMessage(hWndEditor, EM_LINEINDEX, end.y, 0) + end.x;

			SendMessage(hWndEditor, EM_SETSEL, startIndex, endIndex);

			CHARFORMAT2W format;
			ZeroMemory(&format, sizeof(format));
			format.cbSize = sizeof(format);
			format.dwMask = CFM_BOLD | CFM_COLOR | CFM_BACKCOLOR;
			format.dwEffects = CFE_BOLD;
			format.crTextColor = ToCOLORREF(scheme.foreSelectColour);
			format.crBackColor = ToCOLORREF(scheme.rowSelectBackColour);

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