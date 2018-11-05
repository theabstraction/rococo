#ifndef SEXY_WINDOWS_H
#define SEXY_WINDOWS_H

#ifdef SEXY_WINDOWS_IMP
#define SEXY_WINDOWS_API __declspec(dllexport)
#else
#define SEXY_WINDOWS_API __declspec(dllimport)
#endif

namespace Rococo
{
	struct ILog;

	namespace SexyWindows
	{
		struct StringPtrArray
		{
			const char** pArray;
		};

		struct SysWindowHandle
		{
			void* internalPtr;

#ifdef _WIN32
# ifdef _WIN32_WINNT
			operator HWND () const { return (HWND)internalPtr; }
# endif
#endif
			static SysWindowHandle None() { return SysWindowHandle{ nullptr }; }
		};

		struct ErrorDialogSpec
		{
			const char* title;
			StringPtrArray errorMessages;
			StringPtrArray responseButtons;
			int systemError;
			SysWindowHandle parent;
		};

		struct ScriptedDialogSpec
		{
			const char* scriptFile;			   // The sxy file to execute to populate the dialog
			const char* indicatorFile;         // The marker file for the installation folder
			SysWindowHandle parent;            // Parent window handle. In Win32 is the HWND handle
			size_t maxProgSize;                // Maximum size of a sxy script binary
		};

		struct DialogResult
		{
			const char* buttonName;
			size_t returnValue;
		};

		const char* DialogThrewException();

		struct ISexyWindows
		{
			virtual DialogResult ShowErrorDialog(const ErrorDialogSpec& info) = 0;
			virtual DialogResult ShowScriptedDialog(const ScriptedDialogSpec& info) = 0;
			virtual void Free() = 0;
		};

		struct IRollingLog
		{
			virtual void AppendRaw(char c) = 0;
			virtual ILog& SexyLog() = 0;
			virtual operator StringPtrArray() = 0;
			virtual void Free() = 0;
		};

		IRollingLog* CreateRollingLogger(int _lineLength, int _rowCount);

		struct ISexyWindowsSupervisor : public ISexyWindows
		{
			virtual void Free() = 0;
		};

		SEXY_WINDOWS_API ISexyWindowsSupervisor* CreateSexyWindows(void* hResourceInstance);

		class SexyWindowsSupervisor
		{
		public:
			SexyWindowsSupervisor(void* hResourceInstance)
			{
				impl = CreateSexyWindows(hResourceInstance);
			}

			~SexyWindowsSupervisor()
			{
				impl->Free();
			}

			operator ISexyWindows& ()
			{
				return *impl;
			}

			ISexyWindows& operator ()()
			{
				return *impl;
			}
		private:
			ISexyWindowsSupervisor* impl;
		};
	}
}

#ifdef IMPORT_SEXY_WINDOWS_LIB
#  ifdef _DEBUG
#    pragma comment(lib, "rococo.sexy.windows.debug.lib")
#  else
#    pragma comment(lib, "rococo.sexy.windows.lib")
#  endif
#endif

#endif // SEXY_WINDOWS_H