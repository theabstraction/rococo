#include <..\sexy\Common\sexy.script.h>
#include <..\sexy\Common\sexy.s-parser.h>
#include <rococo.strings.h>

#include "sexy.windows.internal.h"
#include "resource.h"

#include <unordered_map>

#include <rococo.sexy.ide.h>
#include <rococo.io.h>
#include <rococo.os.win32.h>

#include <rococo.window.h>

namespace ANON
{
	using namespace Rococo::SexyWindows;
	using namespace Rococo::Script;
	using namespace Rococo;
	using namespace Rococo::Windows::IDE;

	struct DefaultExceptionHandler : public IScriptExceptionHandler
	{
		DefaultExceptionHandler(HWND _hParent): hParent(_hParent)
		{

		}

		HWND hParent;

		void Free() override
		{

		}

		virtual EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message) override
		{
			char txt[1024];
			SafeFormat(txt, 1024, "Source: %s\nMessage: %s\nThe script system threw an exception. Debug?", source, message);
			int result = MessageBoxA(hParent, "Sexy Script Exception", txt, MB_YESNO | MB_ICONQUESTION);
			if (result == IDYES)
			{
				return EScriptExceptionFlow_Retry;
			}
			else
			{
				return EScriptExceptionFlow_Terminate;
			}
		}
	};

	struct ScriptObjects
	{
		AutoFree<IRollingLog> logger;
		AutoFree<IOSSupervisor> os;
		AutoFree<IInstallationSupervisor> installation;
		AutoFree<ISourceCache> sourceCache;
		AutoFree<IPublicScriptSystem> pss;

		ScriptObjects(size_t maxProgramSize, cstr indicatorFile)
		{
			logger = CreateRollingLogger(128, 200);
			os = Rococo::GetOS();

			installation = CreateInstallation(indicatorFile, *os);

			sourceCache = CreateSourceCache(*installation);

			Rococo::Compiler::ProgramInitParameters pip;
			pip.MaxProgramBytes = 1024 * 1024;

			char scriptPath[Rococo::IO::MAX_PATHLEN];
			installation->ConvertPingPathToSysPath("!scripts", scriptPath, Rococo::IO::MAX_PATHLEN);
			pip.NativeSourcePath = scriptPath;

			pss = CreateScriptV_1_2_0_0(pip, logger->SexyLog());
		}

		~ScriptObjects()
		{
			if (pss)
			{
				pss->Free();
			}
		}

		void ExecuteScript(HWND hParent, cstr scriptPingPathFile, IEventCallback<ScriptCompileArgs>& onCompile)
		{
			struct : public Rococo::Windows::IWindow
			{
				HWND hWnd;

				virtual operator HWND() const
				{
					return hWnd;
				}
			} parent;

			parent.hWnd = hParent;

			char scriptFile[Rococo::IO::MAX_PATHLEN];
			installation->ConvertPingPathToSysPath(scriptPingPathFile, scriptFile, Rococo::IO::MAX_PATHLEN);

			AutoFree<IDebuggerWindow> debuggerWindow = Rococo::Windows::IDE::CreateDebuggerWindow(parent);

			DefaultExceptionHandler exHandler(hParent);

			ScriptPerformanceStats stats;
			Rococo::Windows::IDE::ExecuteSexyScriptLoop(
				stats,
				1048576,
				*sourceCache,
				*debuggerWindow,
				scriptFile,
				0,
				131072,
				onCompile,
				exHandler
			);
		}
	};

	struct ScriptedDialogScriptSession: public IEventCallback<ScriptCompileArgs>
	{
		HWND hDlg;

		ScriptedDialogScriptSession(HWND _hDlg): hDlg(_hDlg)
		{

		}

		void OnEvent(ScriptCompileArgs& onCompile) override
		{
			auto& ss = onCompile.ss;
		}
	};

	struct ScriptedDialogContext
	{
		const ScriptedDialogSpec& spec;
		std::unordered_map<UINT, IMessageHandler*> messageHandlers;
		ScriptObjects objects;

		ScriptedDialogContext(const ScriptedDialogSpec& _spec): 
			spec(_spec), 
			objects(spec.maxProgSize, spec.indicatorFile)
		{
			Bind(WM_CLOSE, Handler::NewEndDialog(IDCANCEL));
		}

		~ScriptedDialogContext()
		{
			for (auto& i : messageHandlers)
			{
				i.second->Free();
			}
		}

		void Bind(UINT message, IMessageHandler* handler)
		{
			messageHandlers[message] = handler;
		}

		INT_PTR OnMessage(Message& message)
		{
			auto i = messageHandlers.find(message.uMsg);
			if (i != messageHandlers.end())
			{
				auto* handler = i->second;
				handler->Handle(message);
				return message.result;
			}

			return FALSE;
		}

		void OnInitDialog(Message& message)
		{
			RefreshChildren(message.hDlg);
		}

		void RefreshChildren(HWND hDlg)
		{
			ScriptedDialogScriptSession session(hDlg);
			objects.ExecuteScript(hDlg, spec.scriptFile, session);
		}
	};

	INT_PTR CALLBACK ScriptedDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		Message message { hDlg, uMsg, wParam, lParam };
		if (uMsg == WM_INITDIALOG)
		{
			auto* context = (ScriptedDialogContext*)lParam;
			SetWindowLongPtrA(hDlg, GWLP_USERDATA, (LPARAM) context);
			context->OnInitDialog(message);
			return TRUE;
		}
		else
		{
			auto* context = (ScriptedDialogContext*)GetWindowLongPtrA(hDlg, GWLP_USERDATA);
			return context ? context->OnMessage(message) : FALSE;
		}
	}
}

namespace Rococo
{
	namespace SexyWindows
	{
		const char* DialogThrewException()
		{
			return "Dialog threw exception";
		}

		DialogResult ShowScriptedDialog(const ScriptedDialogSpec& spec, HINSTANCE hDllInstance)
		{
			try
			{
				ANON::ScriptedDialogContext context(spec);
				if (context.objects.pss == nullptr)
				{
					context.objects.logger->AppendRaw(0);

					ErrorDialogSpec errSpec;
					errSpec.errorMessages = *context.objects.logger;

					const char* responseButtons[] = { "OK", nullptr };
					errSpec.responseButtons.pArray = responseButtons;
					errSpec.systemError = 0;
					errSpec.title = "SexyWindows::ShowScriptedDialog failed. Script Initialization Error";
					errSpec.parent = spec.parent;
					ShowErrorDialog(errSpec, hDllInstance);

					DialogResult throw_result{ DialogThrewException(), IDCANCEL };
					return throw_result;
				}
				INT_PTR result = DialogBoxParamA(hDllInstance, (LPCSTR)IDD_SCRIPTED_DIALOG, spec.parent, ANON::ScriptedDialogProc, (LPARAM)&context);
				return DialogResult{ nullptr, (size_t)result };
			}
			catch (IException& ex)
			{
				ErrorDialogSpec errSpec;

				const char* errorMessage[] = { ex.Message(), nullptr };
				errSpec.errorMessages.pArray = errorMessage;
				errSpec.systemError = ex.ErrorCode();

				const char* responseButtons[] = { "OK", nullptr };
				errSpec.responseButtons.pArray = responseButtons;

				errSpec.title = "SexyWindows::ShowScriptedDialog failed. Script Initialization Error";
				errSpec.parent = spec.parent;
				ShowErrorDialog(errSpec, hDllInstance);
				DialogResult throw_result{ DialogThrewException(), IDCANCEL };
				return throw_result;
			}
		}
	}
}