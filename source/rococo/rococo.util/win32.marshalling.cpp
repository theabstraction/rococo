#ifdef _WIN32

#include <rococo.types.h>

struct HWND
{
	void* internal;
};

extern "C" __declspec(dllimport) int GetWindowTextA(HWND hWnd, char* lpString, int MaxCount);
extern "C" __declspec(dllimport) int MessageBoxA(HWND hWnd, const char* lpText, const char* lpCaption, Rococo::uint32 uType);
extern "C" __declspec(dllimport) int MessageBoxW(HWND hWnd, const wchar_t* lpText, const wchar_t* lpCaption, Rococo::uint32 uType);

namespace Rococo::Windows
{
	ROCOCO_INTERFACE IWindow
	{
		 virtual operator HWND() const = 0;
	};
}

namespace MSWindows
{
	ROCOCO_API int GetWindowTitle(Rococo::Windows::IWindow& window, char* title, size_t capacity)
	{
		HWND hWnd = (HWND)window;
		return GetWindowTextA(hWnd, title, (int) capacity);
	}

	ROCOCO_API Rococo::Windows::IWindow& NullParent()
	{
		class NullParentClass : public Rococo::Windows::IWindow
		{
		public:
			virtual operator HWND () const
			{
				return { 0 };
			}
		};

		static NullParentClass s_null;
		return s_null;
	}

	int ShowMessageBox(Rococo::Windows::IWindow& window, const char* text, const char* caption, Rococo::uint32 type)
	{
		return MessageBoxA(window, text, caption, type);
	}
}

#endif