#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <rococo.target.h>
#include <Windows.h>

#undef min
#undef max

#include <rococo.api.h>
#include <rococo.strings.h>
#include <rococo.window.h>
#include <stdio.h>

namespace
{
	using namespace Rococo::Windows;

	
}

namespace Rococo
{
	namespace Windows
	{
		int ShowMessageBox(IWindow& window, cstr text, cstr caption, uint32 uType)
		{
			return MessageBoxA(window, text, caption, uType);
		}

		IWindow& NoParent()
		{
			class: public IWindow
			{
				virtual operator HWND () const
				{
					return nullptr;
				}
			} static noParent;

			return noParent;
		}
	}

   namespace OS
   {
      void ShowErrorBox(IWindow& parent, IException& ex, cstr caption)
      {
         if (ex.ErrorCode() == 0)
         {
            ShowMessageBox(parent, ex.Message(), caption, MB_ICONERROR);
         }
         else
         {
            rchar codeMsg[512];
            rchar bigMsg[512];
            if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, ex.ErrorCode(), 0, codeMsg, 512, nullptr) <= 0)
            {
               SafeFormat(bigMsg, sizeof(bigMsg), "%s. Code 0x%x", ex.Message(), ex.ErrorCode());
            }
            else
            {
               SafeFormat(bigMsg, sizeof(bigMsg), "%s\nCode 0x%x: %s", ex.Message(), ex.ErrorCode(), codeMsg);
            }

            ShowMessageBox(parent, bigMsg, caption, MB_ICONERROR);
         }
      }
   }//OS
}//Rococo