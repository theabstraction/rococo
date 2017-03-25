#include "rococo.dx11.api.h"
#include <rococo.window.h>
#include <rococo.strings.h>
#include <vector>
#include <algorithm>

#include "dx11helpers.inl"

namespace
{
   using namespace Rococo;
   using namespace Rococo::Windows;

   class CountdownConfirmationDialog : public StandardWindowHandler, public Rococo::DX11::ICountdownConfirmationDialog
   {
   private:
      ModalDialogHandler modalHandler;
      IDialogSupervisor* dialogWindow;

      IButton* okButton;
      IButton* cancelButton;
      IWindowSupervisor* label;
      IWindowSupervisor* countdownTimerLabel;

      LARGE_INTEGER confirmAt;

      CountdownConfirmationDialog() : dialogWindow(nullptr)
      {
      }

      ~CountdownConfirmationDialog()
      {
         Rococo::Free(dialogWindow);
      }

      virtual void OnClose(HWND hWnd)
      {
         modalHandler.TerminateDialog(IDCANCEL);
      }

      virtual void OnDestroy(HWND hWnd)
      {
         KillTimer(hWnd, (UINT_PTR)dialogWindow);
      }

      virtual void OnMenuCommand(HWND hWnd, DWORD id)
      {
         modalHandler.TerminateDialog(id);
      }

      virtual LRESULT OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam)
      {
         LARGE_INTEGER hz;
         QueryPerformanceFrequency(&hz);

         LARGE_INTEGER now;
         QueryPerformanceCounter(&now);

         LARGE_INTEGER delta;
         delta.QuadPart = confirmAt.QuadPart - now.QuadPart;

         if (delta.QuadPart < 0)
         {
            modalHandler.TerminateDialog(IDOK);
         }
         else
         {
            int secsLeft = (int)((float)delta.QuadPart / (float)hz.QuadPart);
            wchar_t txt[64];
            SafeFormat(txt, _TRUNCATE, L"Confirming in %d seconds", secsLeft);
            SetWindowText(*countdownTimerLabel, txt);
         }

         return 0;
      }

      void PostConstruct()
      {
         WindowConfig config;
         SetOverlappedWindowConfig(config, Vec2i{ 500, 350 }, SW_SHOW, nullptr, L"Confirm Title", WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU, 0);
         dialogWindow = Windows::CreateDialogWindow(config, this);
         RECT rect;
         GetClientRect(*dialogWindow, &rect);
         label = AddLabel(*dialogWindow, GuiRect(rect.left + 10, rect.top + 10, rect.right - 10, rect.bottom - 100), L"Hint", 0, WS_BORDER);
         countdownTimerLabel = AddLabel(*dialogWindow, GuiRect(rect.left + 10, rect.bottom - 90, rect.right - 10, rect.bottom - 50), L"Timer", 0, WS_BORDER);
         okButton = AddPushButton(*dialogWindow, GuiRect(210, rect.bottom - 40, 290, rect.bottom - 10), L"OK", IDOK, BS_DEFPUSHBUTTON);
         cancelButton = AddPushButton(*dialogWindow, GuiRect(310, rect.bottom - 40, 390, rect.bottom - 10), L"Cancel", IDCANCEL, 0);

         SetTimer(*dialogWindow, (UINT_PTR)dialogWindow, 100, nullptr);
      }

   public:
      static CountdownConfirmationDialog* Create()
      {
         auto m = new CountdownConfirmationDialog();
         m->PostConstruct();
         return m;
      }

      DWORD DoModal(HWND owner /* the owner is greyed out during modal operation */, LPCWSTR title, LPCWSTR hint, int countdown)
      {
         LARGE_INTEGER hz;
         QueryPerformanceFrequency(&hz);

         LARGE_INTEGER start;
         QueryPerformanceCounter(&start);

         confirmAt.QuadPart = hz.QuadPart * countdown + start.QuadPart;

         SetWindowText(*dialogWindow, title);
         SetWindowText(*label, hint);
         return dialogWindow->BlockModal(modalHandler.ModalControl(), owner, this);
      }

      void Free()
      {
         delete this;
      }
   };
}

namespace Rococo
{
   namespace DX11
   {
      using namespace Rococo::Windows;

      ICountdownConfirmationDialog* CreateCountdownConfirmationDialog()
      {
         return CountdownConfirmationDialog::Create();
      }

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

      void FormatModeString(const DXGI_MODE_DESC& mode, wchar_t modeDesc[64])
      {
         UINT hz = (UINT)(mode.RefreshRate.Numerator / (float)mode.RefreshRate.Denominator);
         SafeFormat(modeDesc, 64, _TRUNCATE, L"%d x %d - %d Hz", mode.Width, mode.Height, hz);
      }

      bool FormatOutputString(IDXGIOutput& output, wchar_t outputString[64], UINT index)
      {
         DXGI_OUTPUT_DESC odesc;
         if (S_OK != output.GetDesc(&odesc))
         {
            return false;
         }

         if (odesc.AttachedToDesktop)
         {
            int width = odesc.DesktopCoordinates.right - odesc.DesktopCoordinates.left;
            int height = odesc.DesktopCoordinates.bottom - odesc.DesktopCoordinates.top;

            SafeFormat(outputString, 64, _TRUNCATE, L"Output #%d: %d x %d", index, width, height);
            return true;
         }
         else
         {
            std::vector<DXGI_MODE_DESC> modeList;
            GetDisplayModes(modeList, output);
            if (modeList.empty()) return false;

            std::sort(modeList.begin(), modeList.end(), [](const DXGI_MODE_DESC & a, const DXGI_MODE_DESC & b) -> bool
            {
               if (a.Width == b.Width)
               {
                  if (a.Height == b.Height)
                  {
                     return a.RefreshRate.Numerator > b.RefreshRate.Numerator;
                  }

                  return a.Height > b.Height;
               }
               return a.Width > b.Width;
            });

            int width = modeList[0].Width;
            int height = modeList[0].Height;

            SafeFormat(outputString, 64, _TRUNCATE, L"Output #%d: %d x %d", index, width, height);
            return true;
         }
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
            SetOverlappedWindowConfig(config, Vec2i{ 800, 435 }, SW_SHOW, nullptr, L"Choose screen resolution", WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU, 0);
            dialogWindow = Windows::CreateDialogWindow(config, this);
            adapterList = AddListbox(*dialogWindow, GuiRect(10, 10, 390, 150), L"Graphics Adapter", *this, LBS_NOTIFY, WS_BORDER, 0);
            outputList = AddListbox(*dialogWindow, GuiRect(410, 10, 770, 150), L"Screens", *this, WS_VSCROLL | LBS_NOTIFY, WS_BORDER, 0);
            modeList = AddListbox(*dialogWindow, GuiRect(10, 160, 390, 380), L"Display Modes", *this, WS_VSCROLL | LBS_NOTIFY, WS_BORDER, 0);
            okButton = AddPushButton(*dialogWindow, GuiRect(410, 350, 490, 380), L"OK", IDOK, BS_DEFPUSHBUTTON);
            cancelButton = AddPushButton(*dialogWindow, GuiRect(510, 350, 590, 380), L"Cancel", IDCANCEL, 0);
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

               adapterList->AddString(desc.Description);
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
               for (UINT i = 0; hro = adapter->EnumOutputs(i, &output) == S_OK; ++i)
               {
                  DXGI_OUTPUT_DESC odesc;
                  output->GetDesc(&odesc);

                  wchar_t outputString[64];
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
                        wchar_t modeString[64];
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
   }//DX11
}//Rococo
