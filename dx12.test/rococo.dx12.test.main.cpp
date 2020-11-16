#include <rococo.api.h>
#include <rococo.os.win32.h>
#include <rococo.libs.inl>
#include <dxgi.h>
#include <rococo.window.h>
#include <rococo.dx12.h>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <rococo.renderer.h>

#ifdef _DEBUG
# pragma comment(lib, "rococo.windows.debug.lib")
# pragma comment(lib, "rococo.fonts.debug.lib")
#else
# pragma comment(lib, "rococo.windows.lib")
# pragma comment(lib, "rococo.fonts.lib")
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
	window->WaitForNextRenderAndDisplay("Starting game...");

	auto id = factory->Shaders().AddPixelShader("!shaders/gui.ps.hlsl");

	Rococo::OS::ticks start = Rococo::OS::CpuTicks();

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

		enum { TIME_OUT_SECONDS = 10 };
		if (now - start > Rococo::OS::CpuHz() * TIME_OUT_SECONDS)
		{
			break;
		}

		struct CLOSURE : IShaderViewGrabber
		{
			void OnGrab(const ShaderView& e) override
			{
				Throw(e.hr, "Shader %s did not compile:\n %s", e.resourceName, e.errorString ? e.errorString : "???");
			}
		} errorHandler;

		factory->Shaders().TryGrabAndPopNextError(errorHandler);
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