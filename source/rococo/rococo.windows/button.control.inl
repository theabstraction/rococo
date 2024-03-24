namespace Rococo::Windows
{
	class ButtonSupervisor : public IButton, public ICheckbox, private IWindowHandler
	{
	private:
		HWND hWnd;

		ButtonSupervisor() :
			hWnd(nullptr)
		{
		}

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		void OnModal() override
		{

		}

		void OnPretranslateMessage(MSG&) override
		{

		}
	public:
		static ButtonSupervisor* Create(const WindowConfig& config, IParentWindowSupervisor&)
		{
			ButtonSupervisor* p = new ButtonSupervisor();
			p->hWnd = CreateWindowIndirect("BUTTON", config, nullptr);
			return p;
		}

		IWindowHandler& Handler() override
		{
			return *this;
		}

		operator HWND () const override
		{
			return hWnd;
		}

		void Free() override
		{
			delete this;
		}

		Visitors::CheckState GetCheckState()  const override
		{
			switch (SendMessage(hWnd, BM_GETCHECK, 0, 0))
			{
			case BST_CHECKED: return Visitors::CheckState_Ticked;
			case BST_UNCHECKED: return Visitors::CheckState_Clear;
			default: return Visitors::CheckState_NoCheckBox;
			}
		}

		void SetCheckState(Visitors::CheckState state) override
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