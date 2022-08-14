#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include "rococo.hashtable.h"
#include <vector>
#include <comdef.h>

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

struct DX11PixelShader : public DX11Shader
{
	AutoRelease<ID3D11PixelShader> ps;
};

struct DX11Shaders : IDX11Shaders
{
	IInstallation& installation;
	ID3D11Device& device;
	ID3D11DeviceContext& dc;

	std::vector<DX11VertexShader*> vertexShaders;
	std::vector<DX11PixelShader*> pixelShaders;
	std::vector<DX11GeometryShader*> geometryShaders;

	AutoFree<IExpandingBuffer> shaderLoaderBuffer;

	stringmap<ID_PIXEL_SHADER> nameToPixelShader;
	stringmap<ID_VERTEX_SHADER> nameToVertexShader;

	DX11Shaders(IInstallation& _installation, ID3D11Device& _device, ID3D11DeviceContext& _dc) :
		installation(_installation),
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

				i->ps = nullptr;
				installation.LoadResource(pingPath, *shaderLoaderBuffer, 64_kilobytes);
				HRESULT hr = device.CreatePixelShader(shaderLoaderBuffer->GetData(), shaderLoaderBuffer->Length(), nullptr, &i->ps);
				if FAILED(hr)
				{
					SafeFormat(lastError, "device.CreatePixelShader for %s returned 0x%X", pingPath, hr);
				}
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

			shader->vs = nullptr;
			shader->inputLayout = nullptr;
			installation.LoadResource(pingPath, *shaderLoaderBuffer, 64_kilobytes);

			const D3D11_INPUT_ELEMENT_DESC* elements = nullptr;
			uint32 nElements;

			if (Eq(pingPath, "!gui.vs"))
			{
				elements = DX11::GetGuiVertexDesc();
				nElements = DX11::NumberOfGuiVertexElements();
			}
			else
			{
				elements = DX11::GetObjectVertexDesc();
				nElements = DX11::NumberOfObjectVertexElements();
			}

			HRESULT hr = device.CreateInputLayout(elements, nElements, shaderLoaderBuffer->GetData(), shaderLoaderBuffer->Length(), &shader->inputLayout);
			if SUCCEEDED(hr)
			{
				hr = device.CreateVertexShader(shaderLoaderBuffer->GetData(), shaderLoaderBuffer->Length(), nullptr, &shader->vs);
				if FAILED(hr)
				{
					SafeFormat(lastError, sizeof(lastError), "device.CreateVertexShader for %s returned 0x%X", pingPath, hr);
					shader->inputLayout = nullptr;
				}
			}
			else
			{
				shader->inputLayout = nullptr;
				SafeFormat(lastError, sizeof(lastError), "device.CreateInputLayout for %s returned 0x%X", pingPath, hr);
			}
		}
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
			const wchar_t* msg = e.ErrorMessage();
			Throw(e.Error(), "device.CreateInputLayout failed for shader %s: %ls. %s\n", name, msg, (cstr)e.Description());
		}

		if FAILED(hr)
		{
			delete shader;
			Throw(hr, "device.CreateInputLayout failed with shader %s", name);
			return ID_VERTEX_SHADER();
		}

		hr = device.CreateVertexShader(shaderCode, shaderLength, nullptr, &shader->vs);
		if FAILED(hr)
		{
			delete shader;
			Throw(hr, "device.CreateVertexShader failed with shader %s", name);
			return ID_VERTEX_SHADER::Invalid();
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

		DX11PixelShader* shader = new DX11PixelShader;
		HRESULT hr = device.CreatePixelShader(shaderCode, shaderLength, nullptr, &shader->ps);
		if FAILED(hr)
		{
			delete shader;
			Throw(hr, "device.CreatePixelShader failed with shader %s", name);
			return ID_PIXEL_SHADER::Invalid();
		}

		shader->name = name;
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
			return ID_GEOMETRY_SHADER::Invalid();
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
			dc.PSSetShader(ps.ps, nullptr, 0);
			currentVertexShaderId = vid;
			currentPixelShaderId = pid;
			return true;
		}
	}
};

namespace Rococo::DX11
{
	IDX11Shaders* CreateShaderManager(IInstallation& installation, ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new DX11Shaders(installation, device, dc);
	}
}