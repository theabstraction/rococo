#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.strings.h>

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#include <vector>
#include <rococo.hashtable.h>
#include <rococo.time.h>

using namespace Rococo;
using namespace Rococo::Strings;

struct HLSL_Monitor: IO::IShaderMonitor, ID3DInclude, IEventCallback<FileModifiedArgs>, OS::IThreadJob
{
	std::vector<D3D_SHADER_MACRO> macros_not_nullterminated;

	bool isDebugging = false;
	bool rowMajor = false;

	std::vector<HString> includePaths;
	HString targetPath;

	AutoFree<IO::IOSSupervisor> io;

	stringmap<Time::ticks> pathToCompileTime;

	IO::IShaderMonitorEvents& eventHandler;

	AutoFree<OS::IThreadSupervisor> thread;
	AutoFree<OS::ICriticalSection> sync;

	struct Job
	{
		HString filename;
		bool wasResultOfModifiedFile;
	};

	std::vector<Job> jobs;
	std::vector<HString> logs;
	std::vector<Job> skippedJobs;

	HLSL_Monitor(IO::IShaderMonitorEvents& _eventHandler, cstr targetDirectory): io(IO::GetIOS()), eventHandler(_eventHandler)
	{
		this->targetPath = targetDirectory;
		thread = OS::CreateRococoThread(this, 0);
		sync = OS::CreateCriticalSection();
		thread->Resume();
	}

	~HLSL_Monitor()
	{
		thread = nullptr;
	}

	void Free()
	{
		char message[256];
		SafeFormat(message, "Error count: %llu. Message count: %llu\n", errorCount, messageCount);
		eventHandler.OnLog(*this, message);
		delete this;
	}

	void OnEvent(FileModifiedArgs& args) noexcept
	{
		if (!EndsWith(args.sysPath, L".hlsl"))
		{
			return;
		}

		U8FilePath changedFilename;
		Assign(changedFilename, args.sysPath);

		auto i = pathToCompileTime.insert(changedFilename, Time::TickCount());
		if (i.second)
		{
			// A fresh insert, just fine
			{
				OS::Lock lock(sync);
				jobs.push_back({ changedFilename.buf, true });
			}
			OS::WakeUp(*thread);
		}
		else
		{
			// A renewal
			Time::ticks delta = Time::TickCount() - i.first->second;
			if (delta > Time::TickHz() * 5)
			{
				// 5 secs minimum time between recompiles
				i.second = Time::TickCount();
				{
					OS::Lock lock(sync);
					jobs.push_back({ changedFilename.buf, true });
				}
				OS::WakeUp(*thread);
			}
		}
	}

	void CompileWithFilter(Job& job) noexcept
	{
		cstr filename = job.filename;

		if (EndsWith(filename, ".ps.hlsl"))
		{
			// A pixel shader
			Compile(filename, "ps_5_0");
			return;
		}

		if (EndsWith(filename, ".vs.hlsl"))
		{
			// A vertex shader
			Compile(filename, "vs_5_0");
			return;
		}

		if (EndsWith(filename, ".gs.hlsl"))
		{
			// A geometry shader
			Compile(filename, "gs_5_0");
			return;
		}

		if (EndsWith(filename, ".hs.hlsl"))
		{
			// A hull shader
			Compile(filename, "hs_5_0");
			return;
		}

		if (EndsWith(filename, ".ds.hlsl"))
		{
			// A domain shader
			Compile(filename, "ds_5_0");
			return;
		}

		char message[256];
		SafeFormat(message, "Skipping %80.80s: file appears to be an intermediary\n", filename);
		Log(message);


		OS::Lock lock(sync);
		skippedJobs.push_back(job);

		queueLength--;
	}

	void CompileDirectory(cstr path)
	{
		if (path == nullptr) path = this->monitorDirectory;

		try
		{
			AutoFree<IO::IPathCacheSupervisor> allHlSLfiles = IO::CreatePathCache();
			allHlSLfiles->AddPathsFromDirectory(path, true);
			allHlSLfiles->Sort();

			bool atLeastOne = false;
			for (size_t i = 0; i < allHlSLfiles->NumberOfFiles(); ++i)
			{
				cstr filename = allHlSLfiles->GetFileName(i);
				if (EndsWith(filename, ".hlsl"))
				{
					OS::Lock lock(sync);
					jobs.push_back({ filename, false });
					queueLength = jobs.size();
					atLeastOne = true;
				}
			}

			if (!atLeastOne)
			{
				char msg[256];
				SafeFormat(msg, sizeof msg, "No HLSL files were detected in % s\n", path);
				Log(msg);
			}

			OS::WakeUp(*thread);
		}
		catch (IException& ex)
		{
			char message[1024];
			SafeFormat(message, "Error enumerating directory %s: %0x8.8X:\n%s", path, ex.ErrorCode(), ex.Message());
			Log(message);
		}
	}

	volatile size_t queueLength = 0;

	size_t QueueLength() const
	{
		return queueLength;
	}

	HString monitorDirectory = R"(\work\rococo\source\rococo\dx11.renderer\shaders\)";

	void SetMonitorDirectory(cstr path)
	{
		WideFilePath wPath;
		Assign(wPath, path);
		io->Monitor(wPath);
		monitorDirectory = path;
	}

	void DoHousekeeping()
	{
		{
			OS::Lock lock(sync);

			for (auto& log : logs)
			{
				eventHandler.OnLog(*this, log);
			}

			logs.clear();
		}

		while (!skippedJobs.empty())
		{
			Job skippedJob;
			{
				OS::Lock lock(sync);
				skippedJob = skippedJobs.back();
				skippedJobs.pop_back();
			}

			if (skippedJob.wasResultOfModifiedFile)
			{
				eventHandler.OnModifiedFileSkipped(*this, skippedJob.filename);
			}
		}		
	}

	void AddMacro(cstr name, cstr value)
	{
		macros_not_nullterminated.push_back({ name, value });
	}

	void AddIncludePath(cstr path)
	{
		for (auto& i : includePaths)
		{
			if (Eq(i, path))
			{
				return;
			}
		}

		includePaths.push_back(path);
	}

	void Log(cstr message)
	{
		OutputDebugStringA(message);
		OS::Lock lock(sync);
		logs.push_back(message);
	}

	size_t compileCountThisSession = 0;
	size_t errorCount = 0;
	size_t messageCount = 0;

	void Compile(cstr filename, cstr target) noexcept
	{
		std::vector<D3D_SHADER_MACRO> macros = macros_not_nullterminated;
		macros.push_back({ nullptr, nullptr });

		UINT flags = 0;
		if (isDebugging)
		{
			flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		}
		else
		{
			flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
		}

		if (rowMajor)
		{
			flags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
		}
		else
		{
			flags |= D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
		}

		flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;

		UINT flags2 = 0;

		WideFilePath wPath;
		Assign(wPath, filename);
		
		ID3DBlob* blobCode = nullptr;
		ID3DBlob* errorMessages = nullptr;

		compileCountThisSession++;

		char message[1024];
		SafeFormat(message, "%llu: Compiling %80.80ws", compileCountThisSession, wPath.buf);
		Log(message);

		Time::ticks now = Time::TickCount();

		HRESULT hr = D3DCompileFromFile(wPath, macros.data(), this, "main", target, flags, flags2, &blobCode, &errorMessages);
		
		if (errorMessages)
		{
			cstr messages = (cstr) errorMessages->GetBufferPointer();

			messageCount++;

			char intro[512];
			SafeFormat(intro, "Error compiling %s. Code 0x%8.8X\n", filename, hr);
			Log(intro);
			Log(messages);
			Log("\n\n\n");
			errorMessages->Release();
		}

		if FAILED(hr)
		{
			errorCount++;

			if (!errorMessages)
			{
				char intro[512];
				SafeFormat(intro, "Unkown Error compiling %s. Code 0x%8.8X\n", filename, hr);;
				Log(intro);
			}

			return;
		}

		Time::ticks end = Time::TickCount();


		SafeFormat(message, ". Done in %g ms\n", (end - now) / ((double)Time::TickHz() * 0.0001f));
		Log(message);

		U8FilePath targetFullname;
		Format(targetFullname, "%hs", filename);

		cstr ext = GetFileExtension(targetFullname);
		if (Eq(ext, ".hlsl"))
		{
			targetFullname.buf[strlen(filename) - 5] = 0;

			Substring s = ToSubstring(targetFullname);
			cstr finalSlash = Strings::ReverseFind('\\', s);
			U8FilePath transformedPath;
			if (EndsWith(targetPath, "\\"))
			{
				finalSlash++;
			}
			
			Format(transformedPath, "%s%s", targetPath.c_str(), finalSlash);
			
			try
			{
				IO::SaveBinaryFile(transformedPath, (uint8*)blobCode->GetBufferPointer(), blobCode->GetBufferSize());
			}
			catch (IException& ex)
			{
				SafeFormat(message, "Error saving %s. Code 0x%8.8X.\n%s", filename, ex.ErrorCode(), ex.Message());
				Log(message);
			}
		}

		blobCode->Release();
	}

	STDMETHOD(Open)(D3D_INCLUDE_TYPE IncludeType, cstr pFileName, LPCVOID pParentData, OUT LPCVOID* ppData, OUT UINT* pBytes) override
	{
		cstr parentData = (cstr)pParentData;
		UNUSED(parentData);
		UNUSED(IncludeType);

		try
		{
			for (auto& i : includePaths)
			{
				U8FilePath fullPath;
				Format(fullPath, "%s\\%s", i.c_str(), pFileName);
				if (!IO::IsFileExistant(fullPath))
				{
					continue;
				}

				struct Anon : IO::IBinaryFileLoader
				{
					char* data = 0;
					size_t len = 0;

					uint8* LockWriter(size_t length) override
					{
						len = length;
						data = new char[length + 1];
						return (uint8*)data;
					}

					void Unlock() override
					{
						if (len) data[len] = 0;
					}
				} onLoading;

				IO::LoadBinaryFile(onLoading, fullPath, 1_megabytes);

				*ppData = onLoading.data;
				*pBytes = (UINT)onLoading.len;

				return S_OK;
			}

			return D3D11_ERROR_FILE_NOT_FOUND;
		}
		catch (IException& ex)
		{
			char message[1024];
			SafeFormat(message, "Error loading % s: %s\n", pFileName, ex.Message());
			Log(message);
			return ex.ErrorCode() == 0 ? E_FAIL : ex.ErrorCode();
		}
		catch (...)
		{
			return E_FAIL;
		}
	}

	STDMETHOD(Close)(THIS_ LPCVOID pData) override
	{
		if (pData)
		{
			uint8* buf = (uint8*) pData;
			delete[] buf;
		}

		return S_OK;
	}

	uint32 RunThread(OS::IThreadControl& control) override
	{
		while (control.IsRunning())
		{
			io->EnumerateModifiedFiles(*this);

			while (!jobs.empty() && control.IsRunning())
			{
				Job back;

				{
					OS::Lock lock(sync);
					back = jobs.back();
					jobs.pop_back();					

					queueLength = jobs.size() + 1;
				}

				CompileWithFilter(back);
			}

			queueLength = 0;

			control.SleepUntilAysncEvent(1000);
		}

		return 0;
	}
};

extern "C" ROCOCO_API_EXPORT IO::IShaderMonitor* RococoGraphics_CreateShaderMonitor(Rococo::IO::IShaderMonitorEvents& eventHandler, cstr targetDirectory)
{
	return new HLSL_Monitor(eventHandler, targetDirectory);
}