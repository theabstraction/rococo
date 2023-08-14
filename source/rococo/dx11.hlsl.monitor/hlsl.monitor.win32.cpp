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

struct HLSL_Monitor: IO::IShaderMonitor, ID3DInclude, IEventCallback<FileModifiedArgs>
{
	std::vector<D3D_SHADER_MACRO> macros_not_nullterminated;

	bool isDebugging = false;
	bool rowMajor = false;

	std::vector<HString> includePaths;
	HString targetPath;

	AutoFree<IO::IOSSupervisor> io;

	stringmap<Time::ticks> pathToCompileTime;

	Strings::IStringPopulator& logger;

	HLSL_Monitor(Strings::IStringPopulator& _logger, cstr targetDirectory): io(IO::GetIOS()), logger(_logger)
	{
		this->targetPath = targetDirectory;
	}

	void Free()
	{
		delete this;
	}

	void OnEvent(FileModifiedArgs& args) noexcept
	{
		U8FilePath changedFilename;
		Assign(changedFilename, args.sysPath);

		auto i = pathToCompileTime.insert(changedFilename, Time::TickCount());
		if (i.second)
		{
			// A fresh insert, just fine
		}
		else
		{
			// A renewal
			Time::ticks delta = Time::TickCount() - i.first->second;
			if (delta > Time::TickHz() * 5)
			{
				// 5 secs minimum time between recompiles
				i.second = Time::TickCount();
			}
		}

		CompileWithFilter(changedFilename);
	}

	void CompileWithFilter(cstr filename) noexcept
	{
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
	}

	void CompileDirectory(cstr path)
	{
		try
		{
			AutoFree<IO::IPathCacheSupervisor> allHlSLfiles = IO::CreatePathCache();
			allHlSLfiles->AddPathsFromDirectory(path, true);
			allHlSLfiles->Sort();

			for (size_t i = 0; i < allHlSLfiles->NumberOfFiles(); ++i)
			{
				cstr filename = allHlSLfiles->GetFileName(i);
				CompileWithFilter(filename);
			}
		}
		catch (IException& ex)
		{
			char message[1024];
			SafeFormat(message, "Error enumerating directory %s: %0x8.8X:\n%s", path, ex.ErrorCode(), ex.Message());
			Log(message);
		}
	}

	void SetMonitorDirectory(cstr path)
	{
		WideFilePath wPath;
		Assign(wPath, path);
		io->Monitor(wPath);
	}

	void DoHousekeeping()
	{
		io->EnumerateModifiedFiles(*this);
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
		fprintf(stderr, "%s", message);
		logger.Populate(message);
	}

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
		HRESULT hr = D3DCompileFromFile(wPath, macros.data(), this, "main", target, flags, flags2, &blobCode, &errorMessages);
		
		if (errorMessages)
		{
			cstr messages = (cstr) errorMessages->GetBufferPointer();

			char intro[512];
			SafeFormat(intro, "Error compiling %s. Code 0x%8.8X\n", filename, hr);
			Log(intro);
			Log(messages);
			Log("\n\n\n");
			errorMessages->Release();
		}

		if FAILED(hr)
		{
			if (!errorMessages)
			{
				char intro[512];
				SafeFormat(intro, "Unkown Error compiling %s. Code 0x%8.8X\n", filename, hr);;
				Log(intro);
			}

			return;
		}

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
				char message[1024];
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
};

extern "C" ROCOCO_API_EXPORT IO::IShaderMonitor* RococoGraphics_CreateShaderMonitor(Rococo::Strings::IStringPopulator & logger, cstr targetDirectory)
{
	return new HLSL_Monitor(logger, targetDirectory);
}