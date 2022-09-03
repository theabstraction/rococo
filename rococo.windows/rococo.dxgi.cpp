#include <rococo.api.h>

#include <dxgi.h>
#include <rococo.auto-release.h>
#include <vector>
#include <algorithm>
#include <rococo.strings.h>
#include <rococo.window.h>

namespace Rococo::Graphics
{
	using namespace Rococo::Strings;
	using namespace Rococo::Windows;

	void GetDisplayModes(std::vector<DXGI_MODE_DESC>& modeList, IDXGIOutput& output)
	{
		UINT numberOfModes = 0;
		output.GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numberOfModes, nullptr);

		if (numberOfModes == 0) return;

		DXGI_MODE_DESC* modes = (DXGI_MODE_DESC*)_alloca(numberOfModes * sizeof(DXGI_MODE_DESC));
		if (S_OK != output.GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numberOfModes, modes))
		{
			return;
		}

		SIZE lastSize = { 0 };
		UINT lastHz = 0;

		for (UINT i = 0; i < numberOfModes; ++i)
		{
			const DXGI_MODE_DESC& m = modes[i];
			UINT Hz = (UINT)(m.RefreshRate.Numerator / (float)m.RefreshRate.Denominator);

			if (lastSize.cx == m.Width && lastSize.cy == m.Height && lastHz == Hz)
			{
				continue; // skip duplicate modes
			}

			lastSize.cx = m.Width;
			lastSize.cy = m.Height;
			lastHz = Hz;

			modeList.push_back(m);
		}
	}

	void FormatModeString(const DXGI_MODE_DESC& mode, char modeDesc[64])
	{
		UINT hz = (UINT)(mode.RefreshRate.Numerator / (float)mode.RefreshRate.Denominator);
		SafeFormat(modeDesc, 64, "%d x %d - %d Hz", mode.Width, mode.Height, hz);
	}

	bool FormatOutputString(IDXGIOutput& output, char outputString[64], UINT index)
	{
		DXGI_OUTPUT_DESC odesc;
		if (S_OK != output.GetDesc(&odesc))
		{
			return false;
		}

		SafeFormat(outputString, 64, "Output #%d", index);
		return true;
	}

	class AdapterDialog : public StandardWindowHandler, private IListItemHandler
	{
	private:
		std::vector<DXGI_MODE_DESC> modeCache;
		ModalDialogHandler modalHandler;
		IDialogSupervisor* dialogWindow;
		IListWindowSupervisor* adapterList;
		IListWindowSupervisor* outputList;
		IListWindowSupervisor* modeList;
		IButton* okButton;
		IButton* cancelButton;

		IDXGIFactory& f;

		DXGI_MODE_DESC outputDesc;

		AdapterDialog(IDXGIFactory& _f) : f(_f), dialogWindow(nullptr), adapterList(nullptr), modeList(nullptr)
		{
			ZeroMemory(&outputDesc, sizeof(outputDesc));
		}

		~AdapterDialog()
		{
			Rococo::Free(dialogWindow);
		}

		virtual void OnClose(HWND hWnd)
		{
			modalHandler.TerminateDialog(IDCANCEL);
		}

		virtual void OnMenuCommand(HWND hWnd, DWORD id)
		{
			modalHandler.TerminateDialog(IDOK);
		}

		void PostConstruct()
		{
			WindowConfig config;
			SetOverlappedWindowConfig(config, Vec2i{ 800, 435 }, SW_SHOW, nullptr, "Choose screen resolution", WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU, 0);
			dialogWindow = Windows::CreateDialogWindow(config, this);
			adapterList = AddListbox(*dialogWindow, GuiRect(10, 10, 390, 150), "Select Graphics Adapter", *this, LBS_NOTIFY, WS_BORDER, 0);
			outputList = AddListbox(*dialogWindow, GuiRect(410, 10, 770, 150), "Select Screen", *this, WS_VSCROLL | LBS_NOTIFY, WS_BORDER, 0);
			modeList = AddListbox(*dialogWindow, GuiRect(10, 160, 390, 380), "Display Modes", *this, WS_VSCROLL | LBS_NOTIFY, WS_BORDER, 0);
			okButton = AddPushButton(*dialogWindow, GuiRect(410, 350, 490, 380), "OK", IDOK, BS_DEFPUSHBUTTON);
			cancelButton = AddPushButton(*dialogWindow, GuiRect(510, 350, 590, 380), "Cancel", IDCANCEL, 0);
		}

		void PopulateAdapterList()
		{
			adapterList->ResetContent();

			for (UINT i = 0; i < 64; ++i)
			{
				IDXGIAdapter* adapter = nullptr;
				if (DXGI_ERROR_NOT_FOUND == f.EnumAdapters(i, &adapter))
				{
					break;
				}

				DXGI_ADAPTER_DESC desc;
				adapter->GetDesc(&desc);
				adapter->Release();

				char buf[256];
				SafeFormat(buf, "%ls", desc.Description);
				adapterList->AddString(buf);
			}

			adapterList->SetCurrentSelection(0);
			PopulateOutputList();
		}

		void PopulateOutputList()
		{
			outputList->ResetContent();
			int index = adapterList->GetCurrentSelection();
			if (index >= 0)
			{
				IDXGIAdapter* adapter = nullptr;
				if (DXGI_ERROR_NOT_FOUND == f.EnumAdapters(index, &adapter))
				{
					return;
				}

				HRESULT hro;
				IDXGIOutput* output = nullptr;
				for (UINT i = 0; (hro = adapter->EnumOutputs(i, &output)) == S_OK; ++i)
				{
					DXGI_OUTPUT_DESC odesc;
					output->GetDesc(&odesc);

					char outputString[64];
					if (FormatOutputString(*output, outputString, i + 1))
					{
						outputList->AddString(outputString);
					}
				}

				adapter->Release();
			}

			PopulateModeList();
		}

		void PopulateModeList()
		{
			modeCache.clear();
			modeList->ResetContent();
			int adapterIndex = adapterList->GetCurrentSelection();
			int outputIndex = outputList->GetCurrentSelection();
			if (adapterIndex >= 0 && outputIndex >= 0)
			{
				IDXGIAdapter* adapter = nullptr;
				if (S_OK == f.EnumAdapters(adapterIndex, &adapter))
				{
					AutoRelease<IDXGIAdapter> ar_adapter(adapter);

					IDXGIOutput* output = nullptr;
					if (S_OK == adapter->EnumOutputs(outputIndex, &output))
					{
						AutoRelease<IDXGIOutput> ar_output(output);

						GetDisplayModes(modeCache, *output);

						for (auto& i : modeCache)
						{
							char modeString[64];
							FormatModeString(i, modeString);
							modeList->AddString(modeString);
						}
					}
				}
			}
		}

		void OnAdapterChanged()
		{
			PopulateOutputList();
		}

		void OnItemSelectionChanged(IListWindowSupervisor& listWindow)
		{
			if (&listWindow == adapterList)
			{
				OnAdapterChanged();
			}

			if (&listWindow == outputList)
			{
				PopulateModeList();
			}

			EnableWindow(*okButton, modeList->GetCurrentSelection() >= 0 ? TRUE : FALSE);
		}

		virtual void OnDrawItem(DRAWITEMSTRUCT& dis)
		{

		}

		virtual void OnMeasureItem(MEASUREITEMSTRUCT& mis)
		{

		}
	public:
		static AdapterDialog* Create(IDXGIFactory& _f)
		{
			auto m = new AdapterDialog(_f);
			m->PostConstruct();
			return m;
		}

		DWORD DoModal(HWND owner /* the owner is greyed out during modal operation */)
		{
			return dialogWindow->BlockModal(modalHandler.ModalControl(), owner, this);
		}

		void EnumAdapters()
		{
			PopulateAdapterList();
			EnableWindow(*okButton, FALSE);
		}

		void Free()
		{
			delete this;
		}
	};

	void EnumerateAdapters(IDXGIFactory& f, HWND hWndOwner)
	{
		AutoFree<AdapterDialog> modalDialog(AdapterDialog::Create(f));
		modalDialog->EnumAdapters();
		modalDialog->DoModal(hWndOwner);
	}
}//Rococo::Graphics