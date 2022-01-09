namespace
{
	class ButtonSupervisor : public IButton, public ICheckbox, private IWindowHandler
	{
	private:
		HWND hWnd;
	
		ButtonSupervisor() :
			hWnd(nullptr)
		{
		}

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

      virtual void OnPretranslateMessage(MSG& msg)
      {

      }
	public:
		static ButtonSupervisor* Create(const WindowConfig& config, IParentWindowSupervisor& parent)
		{
			ButtonSupervisor* p = new ButtonSupervisor();
			p->hWnd = CreateWindowIndirect("BUTTON", config, nullptr);
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

		virtual Visitors::CheckState GetCheckState()  const
		{
			switch (SendMessage(hWnd, BM_GETCHECK, 0, 0))
			{
			case BST_CHECKED: return Visitors::CheckState_Ticked;
			case BST_UNCHECKED: return Visitors::CheckState_Clear;
			default: return Visitors::CheckState_NoCheckBox;
			}
		}

		virtual void SetCheckState(Visitors::CheckState state)
		{
			switch (state)
			{
			case Visitors::CheckState_NoCheckBox:
				SendMessage(hWnd, BM_SETCHECK, BST_INDETERMINATE, 0); break;
			case Visitors::CheckState_Clear:
				SendMessage(hWnd, BM_SETCHECK, BST_UNCHECKED, 0); break;
			case Visitors::CheckState_Ticked:
				SendMessage(hWnd, BM_SETCHECK, BST_CHECKED, 0); break;
			}
		}
	};
}