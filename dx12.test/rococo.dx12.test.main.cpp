#include <rococo.api.h>
#include <rococo.os.win32.h>
#include <rococo.libs.inl>
#include <dxgi.h>
#include <rococo.window.h>
#include <rococo.dx12.h>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>
#include <vector>
#include <rococo.mplat.h>
#include <rococo.fonts.hq.h>

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>
#include <sexy.lib.script.h>

#ifdef _DEBUG
# pragma comment(lib, "rococo.windows.debug.lib")
# pragma comment(lib, "rococo.fonts.debug.lib")
# pragma comment(lib, "rococo.mplat.debug.lib")
# pragma comment(lib, "rococo.sexy.ide.debug.lib")
#else
# pragma comment(lib, "rococo.windows.lib")
# pragma comment(lib, "rococo.fonts.lib")
# pragma comment(lib, "rococo.mplat.lib")
# pragma comment(lib, "rococo.sexy.ide.lib")
#endif

#pragma comment(lib, "dxgi.lib")

using namespace Rococo;
using namespace Rococo::Graphics;

void Main(HINSTANCE hInstance)
{
	struct : IDX12ResourceResolver
	{
		void ConvertResourceNameToPath(const char* resourceName, wchar_t* sysPath, size_t charsInSysPath)
		{
			if (strlen(resourceName) < 4)
			{
				Throw(0, "The resource name was too short");
			}

			if (resourceName[0] != '!')
			{
				Throw(0, "The resource must begin with a ping character");
			}

			SecureFormat(sysPath, charsInSysPath, L"\\work\\rococo\\content\\%hs", resourceName + 1);
			Rococo::OS::ToSysPath(sysPath);
		}

		void LoadResource_FreeThreaded(const wchar_t* filename, IEventCallback<const fstring>& onLoad)
		{
			enum { MAX_LEN = 64_kilobytes };
			char buffer[MAX_LEN];
			DWORD len;

			{
				struct AutoClose
				{
					HANDLE hFile;
					~AutoClose()
					{
						if (hFile != INVALID_HANDLE_VALUE)
						{
							CloseHandle(hFile);
						}
					}
				};

				AutoClose F{ CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL) };
				if (F.hFile == INVALID_HANDLE_VALUE)
				{
					Throw(GetLastError(), "CreateFileW failed"); // The error handlers will add the filename
				}

				len = SetFilePointer(F.hFile, 0, NULL, FILE_END);
				SetFilePointer(F.hFile, 0, NULL, FILE_BEGIN);

				if (len >= MAX_LEN - 1)
				{
					Throw(E_NOT_SUFFICIENT_BUFFER, "Maximum length %d kb exceeded.", MAX_LEN / 1024);
				}

				DWORD bytesRead;
				if (!ReadFile(F.hFile, buffer, MAX_LEN, &bytesRead, NULL))
				{
					Throw(GetLastError(), "ReadFile failed");
				}

				if (bytesRead != len)
				{
					Throw(E_UNEXPECTED, "bytesRead != len");
				}

				buffer[len] = 0;
			}

			onLoad.OnEvent(fstring{ buffer, (int32) len });
		}
	} resourceResolver;
	AutoFree<IDX12FactoryContext> fcc = CreateDX12FactoryContext(0, 0, resourceResolver);
	AutoFree<IDX12RendererFactory> factory = fcc->CreateFactory(1024_megabytes);
	AutoFree<IOSSupervisor> os = GetOS();
	AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *os);
	AutoFree<IPipelineBuilder> pipelineBuilder = CreatePipelineBuilder(factory->IC(), factory->Shaders());

	struct : Rococo::Graphics::IDX12RendererWindowEventHandler
	{
		bool isRunning = true;
		char msg[1024] = { 0 };
		int errCode = 0;

		void OnCloseRequested(IDX12RendererWindow& window) override
		{
			isRunning = false;
		}

		void OnMessageQueueException(IDX12RendererWindow& window, IException& ex) override
		{
			SafeFormat(msg, "A window message handler threw an exception:\n%s", ex.Message());
			errCode = ex.ErrorCode();
			isRunning = false;
		}

		void ThrowOnError()
		{
			if (msg[0] != 0)
			{
				Throw(errCode, "%s", msg);
			}
		}
	} appState;

	DX12WindowCreateContext wcc
	{
		"Rococo DX12 Test Dialog",
		NULL,
		hInstance,
		GuiRect { 0, 0, 800, 600 },
		appState
	};

	AutoFree<IDX12RendererWindow> window = factory->CreateDX12Window(wcc);
	BringWindowToTop(window->Window());
	window->WaitForNextRenderAndDisplay("Initializing DirectX12...");
	AutoFree<ITextureMemory> textureMemory = Create_MPlat_Standard_TextureMemory(factory->IC());
	AutoFree<IDX12Renderer> renderer = CreateDX12Renderer(*installation, factory->IC(), *textureMemory, factory->Shaders(), *pipelineBuilder);
	window->SetText("");

	WideFilePath shaderDir;
	installation->ConvertPingPathToSysPath("!shaders/", shaderDir);
	os->Monitor(shaderDir);

	auto guiPS = factory->Shaders().AddPixelShader("!shaders/gui.ps.hlsl");
	auto guiVS = factory->Shaders().AddVertexShader("!shaders/gui.vs.hlsl");

	renderer->SetTargetWindow(window);
	auto& r = renderer->Renderer();

	AutoFree<IHQFontsSupervisor> fonts = CreateHQFonts(r);
	fonts->Build(HQFont_EditorFont);
	fonts->AddRange(32, 120);
	fonts->SetFaceName("Consolas"_fstring);
	fonts->SetHeight(-11);
	fonts->MakeBold();
	fonts->Commit();

	Rococo::OS::ticks start = Rococo::OS::CpuTicks();

	struct CLOSURE : IShaderViewGrabber
	{
		struct BadShader
		{
			HString msg;
			HRESULT hr;
			HString resourceName;
		};
		std::vector<BadShader> badShaders;

		void OnGrab(const ShaderView& e) override
		{
			char msg[1024];
			char err[128];
			Rococo::OS::FormatErrorMessage(err, 128, e.hr);
			SafeFormat(msg, "Shader %s did not compile (code 0x%X - %s):\n%s", e.resourceName, e.hr, err, e.errorString ? e.errorString : "???");
			badShaders.push_back({ msg, e.hr, e.resourceName });
		}
	} errorHandler;

	AutoFree<IStringBuilder> psb = CreateDynamicStringBuilder(1024);
	auto& sb = psb->Builder();

	MSG msg;
	while (appState.isRunning)
	{
		MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT);

		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}

		Rococo::OS::ticks now = Rococo::OS::CpuTicks();

		enum { TIME_OUT_SECONDS = 120 };
		if (now - start > Rococo::OS::CpuHz() * TIME_OUT_SECONDS)
		{
			break;
		}

		struct CLOSURE2 : IEventCallback<FileModifiedArgs>
		{
			std::vector<WideFilePath> updates;
			void OnEvent(FileModifiedArgs& args) override
			{
				if (EndsWith(args.sysPath, L".hlsl"))
				{
					WideFilePath update;
					Format(update, L"%ls", args.sysPath);
					updates.push_back(update);
				}
			}
		} onFileChanged;

		os->EnumerateModifiedFiles(onFileChanged);

		if (!onFileChanged.updates.empty())
		{
			U8FilePath pingPath;
			installation->ConvertSysPathToPingPath(onFileChanged.updates.back(), pingPath);
			factory->Shaders().ReloadShader(pingPath);
		}

		factory->Shaders().TryGrabAndPopNextError(errorHandler);

		if (!errorHandler.badShaders.empty())
		{
			sb.Clear();
			for (auto& bad : errorHandler.badShaders)
			{
				sb << bad.msg.c_str() << "\n";
			}
			window->SetText(*sb);

			auto& s = factory->Shaders();

			auto i = std::remove_if(errorHandler.badShaders.begin(), errorHandler.badShaders.end(), 
				[&s](CLOSURE::BadShader& shader)
				{
					struct CLOSURE : IShaderViewGrabber
					{
						bool isOK = false;
						void OnGrab(const ShaderView& e) override
						{
							isOK = SUCCEEDED(e.hr);
						}
					} grabber;
					s.GrabShaderObject(shader.resourceName, grabber);
					return grabber.isOK;
				}
			);

			errorHandler.badShaders.erase(i, errorHandler.badShaders.end());

			if (errorHandler.badShaders.empty())
			{
				window->SetText("");
			}
		}
	}

	appState.ThrowOnError();
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	try
	{
		Rococo::Windows::InitRococoWindows(hInstance, NULL, NULL, NULL, NULL);
		Main(hInstance);
		return 0;
	}
	catch (IException& ex)
	{
		Rococo::OS::ShowErrorBox(Windows::NoParent(), ex, "Rococo.DX12.TestDialog crashed");
		return ex.ErrorCode();
	}
}