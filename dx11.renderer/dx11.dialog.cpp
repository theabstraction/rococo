#include "dx11.renderer.h"
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
            char txt[64];
            SafeFormat(txt, sizeof(txt), "Confirming in %d seconds", secsLeft);
            SetWindowTextA(*countdownTimerLabel, txt);
         }

         return 0;
      }

      void PostConstruct()
      {
         WindowConfig config;
         SetOverlappedWindowConfig(config, Vec2i{ 500, 350 }, SW_SHOW, nullptr, "Confirm Title", WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU, 0);
         dialogWindow = Windows::CreateDialogWindow(config, this);
         RECT rect;
         GetClientRect(*dialogWindow, &rect);
         label = AddLabel(*dialogWindow, GuiRect(rect.left + 10, rect.top + 10, rect.right - 10, rect.bottom - 100), "Hint", 0, WS_BORDER);
         countdownTimerLabel = AddLabel(*dialogWindow, GuiRect(rect.left + 10, rect.bottom - 90, rect.right - 10, rect.bottom - 50), "Timer", 0, WS_BORDER);
         okButton = AddPushButton(*dialogWindow, GuiRect(210, rect.bottom - 40, 290, rect.bottom - 10), "OK", IDOK, BS_DEFPUSHBUTTON);
         cancelButton = AddPushButton(*dialogWindow, GuiRect(310, rect.bottom - 40, 390, rect.bottom - 10), "Cancel", IDCANCEL, 0);

         SetTimer(*dialogWindow, (UINT_PTR)dialogWindow, 100, nullptr);
      }

   public:
      static CountdownConfirmationDialog* Create()
      {
         auto m = new CountdownConfirmationDialog();
         m->PostConstruct();
         return m;
      }

      DWORD DoModal(HWND owner /* the owner is greyed out during modal operation */, cstr title, cstr hint, int countdown)
      {
         LARGE_INTEGER hz;
         QueryPerformanceFrequency(&hz);

         LARGE_INTEGER start;
         QueryPerformanceCounter(&start);

         confirmAt.QuadPart = hz.QuadPart * countdown + start.QuadPart;

         SetWindowTextA(*dialogWindow, title);
         SetWindowTextA(*label, hint);
         return dialogWindow->BlockModal(modalHandler.ModalControl(), owner, this);
      }

      void Free()
      {
         delete this;
      }
   };
}

namespace Rococo::DX11
{
    ICountdownConfirmationDialog* CreateCountdownConfirmationDialog()
    {
        return CountdownConfirmationDialog::Create();
    }
}
