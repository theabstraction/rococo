#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include "rococo.hashtable.h"
#include "rococo.io.h"
#include <vector>
#include <comdef.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>

#pragma comment(lib, "D3dcompiler.lib")

using namespace Rococo::DX11;

struct DX11Shader
{
	HString name;
};

struct DX11VertexShader : public DX11Shader
{
	AutoRelease<ID3D11InputLayout> inputLayout;
	AutoRelease<ID3D11VertexShader> vs;
};

struct DX11GeometryShader : public DX11Shader
{
	AutoRelease<ID3D11InputLayout> inputLayout;
	AutoRelease<ID3D11GeometryShader> gs;
};

/*
ROCOCO_INTERFACE IShaderOptions
{
	virtual size_t NumberOfOptions() const = 0;

	// Retrieve the options. Do not cache the pointers, consume them before significant API calls that may change the options
	virtual void GetOption(size_t index, OUT cstr& interfaceName, OUT cstr& className) = 0;
};
*/

struct DX11PixelShader : public DX11Shader
{
	ID3D11Device& device;
	AutoRelease<ID3D11PixelShader> ps;
	AutoRelease<ID3D11ShaderReflection> reflection;
	AutoRelease<ID3D11ClassLinkage> classLinkage;
	std::vector<ID3D11ClassInstance*> classInstances;

	DX11PixelShader(ID3D11Device& _device) : device(_device)
	{
		ResetLinkage();
	}

	void ResetLinkage()
	{
		AutoRelease<ID3D11ClassLinkage> classLinkage;
		HRESULT hr = device.CreateClassLinkage(&classLinkage);
		if FAILED(hr)
		{
			Throw(hr, "UpdatePixelShaderLinkage failed. device.CreateClassLinkage(&classLinkage); returned an error code.");
		}

		ClearInstances();

		this->classLinkage = classLinkage;
	}

	~DX11PixelShader()
	{
		ClearInstances();
	}

	void ClearInstances()
	{
		for (auto* i : classInstances)
		{
			if (i) i->Release();
		}

		classInstances.clear();
	}

	void UpdatePixelShaderLinkage(IShaderOptions& options, const IExpandingBuffer& buffer) 
	{
		AutoRelease<ID3D11ShaderReflection> reflection;
		HRESULT hr = D3DReflect(buffer.GetData(), buffer.Length(), IID_ID3D11ShaderReflection, (void**)&reflection);
		if (FAILED(hr) || reflection == nullptr)
		{
			Throw(hr, "UpdatePixelShaderLinkage: D3DReflect for %s returned 0x%X", name.c_str(), hr);
		}

		enum { MAX_SLOTS = 128 };

		UINT nInterfaceSlots = reflection->GetNumInterfaceSlots();
		if (nInterfaceSlots > MAX_SLOTS)
		{
			Throw(hr, "UpdatePixelShaderLinkage: %s shader->reflection->GetNumInterfaceSlots() returned %u > %u ", name.c_str(), nInterfaceSlots, MAX_SLOTS);
		}

		std::vector<ID3D11ClassInstance*> classInstances;
		classInstances.resize(nInterfaceSlots);
		std::fill(classInstances.begin(), classInstances.end(), nullptr);

		try
		{
			for (size_t i = 0; i < options.NumberOfOptions(); ++i)
			{
				cstr interfaceName, interfaceClass;
				options.GetOption(i, OUT interfaceName, OUT interfaceClass);

				ID3D11ShaderReflectionVariable* var = reflection->GetVariableByName(interfaceName);
				
				if (var != nullptr)
				{
					UINT iSlot = var->GetInterfaceSlot(0);
					if (iSlot < MAX_SLOTS)
					{
						AutoRelease<ID3D11ClassInstance> instance;
						hr = classLinkage->CreateClassInstance(interfaceClass,0, 0, 0, 0, &instance);
						if (hr != S_OK)
						{
							Throw(hr, "UpdatePixelShaderLinkage: %s classLinkage->CreateClassInstance(%s, ...) returned an error code for class name %s", name.c_str(), interfaceClass);
						}

						D3D11_CLASS_INSTANCE_DESC desc;
						instance->GetDesc(&desc);
						if (!desc.Created)
						{
							Throw(0, "UpdatePixelShaderLinkage: %s classLinkage->CreateClassInstance(%s, ...) returned a duff class ref to %s", name.c_str(), interfaceClass);
						}
						
						instance->AddRef();
						classInstances[iSlot] = instance.Detach();						
					}
				}
			}
		}
		catch (...)
		{
			for (auto* l : classInstances)
			{
				if (l) l->Release();
			}
			throw;
		}

		for (size_t i = 0; i < classInstances.size(); ++i)
		{
			if (classInstances[i] == nullptr)
			{
				// TODO - figure out which and splice into the message
				Throw(0, "UpdatePixelShaderLinkage: %s shader is missing class instances from the shader options. Use ShaderOptionsConfig() to add the missing options", name.c_str());
			}
		}

		reflection->AddRef();

		ClearInstances();

		this->reflection = reflection.Detach();
		this->classInstances = classInstances;
	}
};

struct DX11Shaders : IDX11Shaders
{
	IO::IInstallation& installation;
	ID3D11Device& device;
	ID3D11DeviceContext& dc;
	IShaderOptions& shaderOptions;

	std::vector<DX11VertexShader*> vertexShaders;
	std::vector<DX11PixelShader*> pixelShaders;
	std::vector<DX11GeometryShader*> geometryShaders;

	AutoFree<IExpandingBuffer> shaderLoaderBuffer;

	stringmap<ID_PIXEL_SHADER> nameToPixelShader;
	stringmap<ID_VERTEX_SHADER> nameToVertexShader;

	DX11Shaders(IO::IInstallation& _installation, IShaderOptions& _shaderOptions, ID3D11Device& _device, ID3D11DeviceContext& _dc) :
		installation(_installation),
		shaderOptions(_shaderOptions),
		device(_device),
		dc(_dc),
		shaderLoaderBuffer(CreateExpandingBuffer(64_kilobytes))
	{
		
	}

	virtual ~DX11Shaders()
	{
		for (auto& x : vertexShaders)
		{
			delete x;
		}

		for (auto& x : pixelShaders)
		{
			delete x;
		}

		for (auto& x : geometryShaders)
		{
			delete x;
		}
	}

	void Free() override
	{
		delete this;
	}

	char lastError[256] = { 0 };

	void UpdatePixelShader(cstr pingPath) override
	{
		for (auto& i : pixelShaders)
		{
			if (Eq(pingPath, i->name.c_str()))
			{
				if (!i->ps)
				{
					lastError[0] = 0;
				}

				installation.LoadResource(pingPath, *shaderLoaderBuffer, 64_kilobytes);
				if (shaderLoaderBuffer->Length() == 0)
				{
					Throw(0, "UpdatePixelShader LoadResource for %s returned 0 length object", pingPath);
				}

				i->ResetLinkage();

				AutoRelease<ID3D11PixelShader> replacementPS;
				HRESULT hr = device.CreatePixelShader(shaderLoaderBuffer->GetData(), shaderLoaderBuffer->Length(), i->classLinkage, &replacementPS);

				if (FAILED(hr) || replacementPS == nullptr)
				{
					Throw(hr, "device.UpdatePixelShader for %s returned 0x%X", pingPath, hr);
				}

				i->ps = replacementPS;
				
				i->UpdatePixelShaderLinkage(shaderOptions, *shaderLoaderBuffer);

				break;
			}
		}
	}

	void UpdateVertexShader(cstr pingPath) override
	{
		auto i = nameToVertexShader.find(pingPath);
		if (i != nameToVertexShader.end())
		{
			auto id = i->second;
			auto* shader = vertexShaders[id.value];

			if (!shader->vs)
			{
				lastError[0] = 0;
			}

			installation.LoadResource(pingPath, *shaderLoaderBuffer, 64_kilobytes);

			if (shaderLoaderBuffer->Length() == 0)			
			{
				SafeFormat(lastError, "LoadResource for %s returned 0 length object", pingPath);
				Throw(0, "%s", lastError);
			}

			AutoRelease<ID3D11VertexShader> replacementShader;
			HRESULT hr = device.CreateVertexShader(shaderLoaderBuffer->GetData(), shaderLoaderBuffer->Length(), nullptr, &replacementShader);

			if (FAILED(hr) || !replacementShader)
			{
				SafeFormat(lastError, sizeof(lastError), "device.CreateVertexShader %s failed and returned returned 0x%X", pingPath, hr);
				Throw(hr, "%s", lastError);
			}

			shader->vs = replacementShader;
		}
	}

	ID_VERTEX_SHADER CreateVertexShader(cstr pingPath, const VertexElement* vertexElements)
	{
		enum { MAX_ELEMENTS = 16 };
		D3D11_INPUT_ELEMENT_DESC elements[MAX_ELEMENTS];

		uint32 i = 0;
		for (; vertexElements[i].SemanticName != nullptr; ++i)
		{
			if (i == MAX_ELEMENTS)
			{
				Throw(0, "%s(%s, ...): vertexElements had more than the MAX_ELEMENTS %u", __FUNCTION__, pingPath, MAX_ELEMENTS);
			}

			const auto& v = vertexElements[i];

			elements[i].SemanticName = v.SemanticName;
			elements[i].SemanticIndex = v.semanticIndex;
			
			switch (v.format)
			{
			case VertexElementFormat::Float1:
				elements[i].Format = DXGI_FORMAT_R32_FLOAT;
				break;
			case VertexElementFormat::Float2:
				elements[i].Format = DXGI_FORMAT_R32G32_FLOAT;
				break;
			case VertexElementFormat::Float3:
				elements[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
				break;
			case VertexElementFormat::Float4:
				elements[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				break;
			case VertexElementFormat::RGBA8U:
				elements[i].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			default:
				Throw(0, "%s(%s, ...): vertexElements[%d] had unhandled format %u", __FUNCTION__, pingPath, i, (uint32) v.format);
			}

			elements[i].InputSlot = 0;
			elements[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			elements[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			elements[i].InstanceDataStepRate = 0;
		}

		if (i == 0)
		{
			Throw(0, "%s(%s, ...): vertexElements terminated early with a null semantic", __FUNCTION__, pingPath);
		}

		return CreateVertexShader(pingPath, elements, i);
	}

	ID_VERTEX_SHADER CreateVertexShader(cstr name, const byte* shaderCode, size_t shaderLength, const D3D11_INPUT_ELEMENT_DESC* vertexDesc, UINT nElements)
	{
		if (name == nullptr || rlen(name) > 1024) Throw(0, "Bad <name> for vertex shader");
		if (shaderCode == nullptr || shaderLength < 4 || shaderLength > 65536) Throw(0, "Bad shader code for vertex shader %s", name);

		DX11VertexShader* shader = new DX11VertexShader;
		HRESULT hr;

		try
		{
			hr = device.CreateInputLayout(vertexDesc, nElements, shaderCode, shaderLength, &shader->inputLayout);
		}
		catch (_com_error& e)
		{
			cstr msg = e.ErrorMessage();
			Throw(e.Error(), "device.CreateInputLayout failed for shader %s: %s. %s\n", name, msg, (cstr)e.Description());
		}

		if FAILED(hr)
		{
			delete shader;
			Throw(hr, "device.CreateInputLayout failed with shader %s", name);
		}

		hr = device.CreateVertexShader(shaderCode, shaderLength, nullptr, &shader->vs);
		if FAILED(hr)
		{
			delete shader;
			Throw(hr, "device.CreateVertexShader failed with shader %s", name);
		}

		shader->name = name;
		vertexShaders.push_back(shader);
		return ID_VERTEX_SHADER(vertexShaders.size() - 1);
	}

	ID_VERTEX_SHADER CreateVertexShader(cstr pingPath, const D3D11_INPUT_ELEMENT_DESC* vertexDesc, UINT nElements) override
	{
		auto i = nameToVertexShader.find(pingPath);
		if (i == nameToVertexShader.end())
		{
			installation.LoadResource(pingPath, *shaderLoaderBuffer, 64_kilobytes);
			auto id = CreateVertexShader(pingPath, shaderLoaderBuffer->GetData(), shaderLoaderBuffer->Length(), vertexDesc, nElements);
			nameToVertexShader.insert(pingPath, id);
			return id;
		}
		else
		{
			return i->second;
		}
	}

	ID_VERTEX_SHADER CreateObjectVertexShader(cstr pingPath) override
	{
		return CreateVertexShader(pingPath, DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
	}

	ID_VERTEX_SHADER CreateParticleVertexShader(cstr pingPath) override
	{
		return CreateVertexShader(pingPath, DX11::GetParticleVertexDesc(), DX11::NumberOfParticleVertexElements());
	}

	ID_PIXEL_SHADER CreatePixelShader(cstr name, const byte* shaderCode, size_t shaderLength)
	{
		if (name == nullptr || rlen(name) > 1024) Throw(0, "Bad <name> for pixel shader");
		if (shaderCode == nullptr || shaderLength < 4 || shaderLength > 65536) Throw(0, "Bad shader code for pixel shader %s", name);

		DX11PixelShader* shader = new DX11PixelShader(device);
		HRESULT hr = device.CreatePixelShader(shaderCode, shaderLength, shader->classLinkage, &shader->ps);
		if FAILED(hr)
		{
			delete shader;
			Throw(hr, "CreatePixelShader failed with shader %s", name);
		}

		AutoRelease<ID3D11ShaderReflection> reflection;
		hr = D3DReflect(shaderLoaderBuffer->GetData(), shaderLoaderBuffer->Length(), IID_ID3D11ShaderReflection, (void**)&reflection);
		if (FAILED(hr) || reflection == nullptr)
		{
			delete shader;
			Throw(hr, "CreatePixelShader: D3DReflect for %s returned 0x%X", name, hr);
		}

		shader->name = name;

		try
		{
			shader->UpdatePixelShaderLinkage(shaderOptions, *shaderLoaderBuffer);
		}
		catch (...)
		{
			delete shader;
			throw;
		}

		pixelShaders.push_back(shader);
		return ID_PIXEL_SHADER(pixelShaders.size() - 1);
	}

	ID_PIXEL_SHADER CreatePixelShader(cstr pingPath) override
	{
		auto i = nameToPixelShader.find(pingPath);
		if (i == nameToPixelShader.end())
		{
			installation.LoadResource(pingPath, *shaderLoaderBuffer, 64_kilobytes);
			auto id = CreatePixelShader(pingPath, shaderLoaderBuffer->GetData(), shaderLoaderBuffer->Length());
			nameToPixelShader.insert(pingPath, id);
			return id;
		}
		else
		{
			return i->second;
		}
	}

	ID_GEOMETRY_SHADER CreateGeometryShader(cstr pingPath) override
	{
		if (pingPath == nullptr || rlen(pingPath) > 1024) Throw(0, "Bad <pingPath> for geometry shader");

		installation.LoadResource(pingPath, *shaderLoaderBuffer, 64_kilobytes);

		if (shaderLoaderBuffer->Length() == 0) Throw(0, "Bad shader code for geometry shader %s", pingPath);

		DX11GeometryShader* shader = new DX11GeometryShader;
		HRESULT hr = device.CreateGeometryShader(shaderLoaderBuffer->GetData(), shaderLoaderBuffer->Length(), nullptr, &shader->gs);
		if FAILED(hr)
		{
			delete shader;
			Throw(hr, "device.CreateGeometryShader failed with shader %s", pingPath);
		}

		shader->name = pingPath;
		geometryShaders.push_back(shader);
		return ID_GEOMETRY_SHADER(geometryShaders.size() - 1);
	}

	void ShowVenue(IMathsVisitor& visitor) override
	{
		visitor.ShowString("Last shader error", "%s", *lastError ? lastError : "- none -");
	}

	bool UseGeometryShader(ID_GEOMETRY_SHADER gid) override
	{
		if (!gid)
		{
			dc.GSSetShader(nullptr, nullptr, 0);
			return true;
		}

		if (gid.value >= geometryShaders.size()) Throw(0, "Bad shader Id in call to UseGeometryShader");

		auto& gs = *geometryShaders[gid.value];
		if (gs.gs == nullptr)
		{
			Throw(0, "Geometry Shader null for %s", gs.name.c_str());
		}

		dc.GSSetShader(gs.gs, nullptr, 0);

		return true;
	}

	ID_PIXEL_SHADER currentPixelShaderId;
	ID_VERTEX_SHADER currentVertexShaderId;

	bool UseShaders(ID_VERTEX_SHADER vid, ID_PIXEL_SHADER pid) override
	{
		if (vid.value >= vertexShaders.size()) Throw(0, "Bad vertex shader Id in call to UseShaders");
		if (pid.value >= pixelShaders.size()) Throw(0, "Bad pixel shader Id in call to UseShaders");

		auto& vs = *vertexShaders[vid.value];
		auto& ps = *pixelShaders[pid.value];

		if (vs.vs == nullptr)
		{
			Throw(0, "Vertex Shader null for %s", vs.name.c_str());
		}

		if (ps.ps == nullptr)
		{
			Throw(0, "Pixel Shader null for %s", ps.name.c_str());
		}

		if (vs.vs == nullptr || ps.ps == nullptr)
		{
			dc.IASetInputLayout(nullptr);
			dc.VSSetShader(nullptr, nullptr, 0);
			dc.PSSetShader(nullptr, nullptr, 0);
			currentVertexShaderId = ID_VERTEX_SHADER();
			currentPixelShaderId = ID_PIXEL_SHADER();
			return false;
		}
		else
		{
			dc.IASetInputLayout(vs.inputLayout);
			dc.VSSetShader(vs.vs, nullptr, 0);

			cstr pixelShaderName = ps.name;
			size_t len = ps.classInstances.size();
			auto* pInstances = len > 0 ? ps.classInstances.data() : nullptr;
			// An error here means not every class interface required by the shader was matched in the shader options
			dc.PSSetShader(ps.ps, pInstances, (UINT) len);
			UNUSED(pixelShaderName);
			currentVertexShaderId = vid;
			currentPixelShaderId = pid;
			return true;
		}
	}
};

namespace Rococo::DX11
{
	IDX11Shaders* CreateShaderManager(IO::IInstallation& installation, IShaderOptions& shaderOptions, ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new DX11Shaders(installation, shaderOptions, device, dc);
	}
}