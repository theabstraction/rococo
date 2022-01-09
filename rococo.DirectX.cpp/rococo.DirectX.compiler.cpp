#include <rococo.DirectX.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <rococo.auto-release.h>

#include <d3dcompiler.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;

#ifdef _DEBUG
#define DEBUG_SHADERS
#endif

#ifdef DEBUG_SHADERS
# define SHADER_COMPILE_DEBUG_FLAGS D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
# define SHADER_COMPILE_DEBUG_FLAGS 0
#endif

namespace ANON
{
	struct KeyValue
	{
		HString key;
		HString value;
	};

	typedef std::vector<KeyValue> TKeyValuePairs;

	HRESULT CompileGenericShaderFromString(const fstring& srcCode, cstr target, cstr resourceName, ID3DBlob** pShaderBlob, ID3DBlob** pErrorBlob, ID3DInclude& includer, TKeyValuePairs& kvPairs) noexcept
	{
		auto* macros = reinterpret_cast<D3D_SHADER_MACRO*>(_alloca(sizeof(D3D_SHADER_MACRO) * (kvPairs.size() + 1)));
		for (size_t i = 0; i < kvPairs.size(); ++i)
		{
			macros[i] = { kvPairs[i].key, kvPairs[i].value };
		}

		macros[kvPairs.size()] = { nullptr,nullptr };

		UINT flags = SHADER_COMPILE_DEBUG_FLAGS | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
		UINT fxFlags = 0;
		HRESULT hr = D3DCompile2(
			srcCode.buffer,
			srcCode.length,
			resourceName,
			macros,
			&includer,
			"main",
			target,
			flags, fxFlags, 0, NULL, 0, pShaderBlob, pErrorBlob);
		return hr;
	}

	class D3DCompiler : public ID3DShaderCompiler, public ID3DInclude
	{
		IInstallation& installation;
		AutoFree<IExpandingBuffer> scratch = CreateExpandingBuffer(128_kilobytes);

		HRESULT lastHResult = S_OK;
		char lastError[4096] = "";

		TKeyValuePairs kvPairs;

	public:
		D3DCompiler(IInstallation& ref_installation): installation(ref_installation)
		{

		}

		virtual ~D3DCompiler()
		{

		}

		void AddShaderMacro(cstr key, cstr value) override
		{
			kvPairs.push_back( { key, value });
		}

		IExpandingBuffer* CompileShader(fstring& srcCode, cstr name, cstr target)
		{
			AutoRelease<ID3DBlob> shaderBlob;
			AutoRelease<ID3DBlob> errorBlob;
			auto hr = CompileGenericShaderFromString(srcCode, target, name, &shaderBlob, &errorBlob, *this, kvPairs);
			
			auto msg = errorBlob ? (cstr) errorBlob->GetBufferPointer() : "";
			
			if FAILED(hr)
			{
				Throw(hr, "%s", msg);
			}

			constexpr uint32 minBlobSize = 20;
			if (!shaderBlob || shaderBlob->GetBufferSize() < minBlobSize)
			{
				Throw(0, "The compiler did not generate shader binary code. But it also did not say what the problem was.");
			}

			size_t blobSize = shaderBlob->GetBufferSize();
			const void* blobData = shaderBlob->GetBufferPointer();
			auto* buffer = CreateExpandingBuffer(blobSize);
			memcpy(buffer->GetData(), blobData, blobSize);
			return buffer;	
		}

		IExpandingBuffer* CompilePS(fstring& srcCode, cstr name)
		{
			return CompileShader(srcCode, name, "ps_5_0");
		}

		IExpandingBuffer* CompileVS(fstring& srcCode, cstr name)
		{
			return CompileShader(srcCode, name, "vs_5_0");
		}

		IExpandingBuffer* Compile(ShaderType type, fstring& srcCode, cstr name) override
		{
			lastHResult = S_OK;
			lastError[0] = 0;

			try
			{
				switch (type)
				{
				case ShaderType::PIXEL:
					return CompilePS(srcCode, name);
				case ShaderType::VERTEX:
					return CompileVS(srcCode, name);
				default:
					Throw(E_NOTIMPL, "Shader type %u unknown", type);
				}
			}
			catch (IException& ex)
			{
				// If there is a problem with an include file then we need to add it into the exception info
				if (FAILED(lastHResult) || *lastError != 0)
				{
					Throw(lastHResult, "%s\n%s\n", lastError, ex.Message());
				}
				else
				{
					throw;
				}
			}
		}

		void Free() override
		{
			delete this;
		}

		cstr ReadFile_Create_And_Fill_Buffer(UINT& nBytes, cstr resourceName)
		{
			scratch->Resize(0);

			if (*resourceName != '!' && *resourceName != '#')
			{
				// relativePath, assume child of "!shaders/"
				U8FilePath fullPingPath;
				Format(fullPingPath, "!shaders/%s", resourceName);
				installation.LoadResource(fullPingPath, *scratch, 1_megabytes);
			}
			else
			{
				// pingPath
				installation.LoadResource(resourceName, *scratch, 1_megabytes);
			}

			nBytes = (uint32) scratch->Length() + 1;
			auto* s = new char[nBytes + 1];
			memcpy(s, scratch->GetData(), nBytes);
			s[nBytes] = 0;
			return s;
		}

		STDMETHOD(Open)(D3D_INCLUDE_TYPE type, cstr pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* len) noexcept
		{
			if (ppData == nullptr || len == nullptr) return E_POINTER;

			try
			{
				// The D3DInclude::Open method is noexcept, we must not allow an exception to leave the function
				cstr fileData = ReadFile_Create_And_Fill_Buffer(*len, pFileName);
				*ppData = fileData;
				*len = (uint32) strlen(fileData);
			}
			catch (IException& ex)
			{
				SafeFormat(lastError, "Error #include \"%s\":\n\t%s\n", pFileName, ex.Message());
				lastHResult = ex.ErrorCode();
			}

			return S_OK; // According to the Docs, custom include handlers' Open methods always return S_OK. Great design! ;pD
		}

		STDMETHOD(Close)(LPCVOID pData)
		{
			const char* fileData = (const char*) pData;
			delete[] fileData;
			return S_OK;
		}
	};
}

namespace Rococo::Graphics::DirectX
{
	ID3DShaderCompiler* CreateD3DCompiler(IInstallation& installation)
	{
		return new ANON::D3DCompiler(installation);
	}
}

namespace Rococo::Graphics::GL
{
	IVulcanShaderCompiler* CreateVulcanCompiler(IInstallation& installation)
	{
		Throw(0, "%s: Not implemented", __FUNCTION__);
	}
}