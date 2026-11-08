#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include "rococo.hashtable.h"
#include "rococo.imaging.h"
#include "rococo.io.h"
#include "rococo.sexml.h"
#include "sexy.types.h"

using namespace Rococo::DX11;
using namespace Rococo::Graphics;
using namespace Rococo::Strings;
using namespace Rococo::Sex::SEXML;

struct DX11Materials : IDX11Materials
{
	IO::IInstallation& installation;
	stringmap<MaterialId> nameToMaterialId;
	std::vector<HString> idToMaterialName;
	AutoFree<IDX11BitmapArray> materialArray;

	DX11Materials(IO::IInstallation& _installation, ID3D11Device& device, ID3D11DeviceContext& dc):
		installation(_installation),
		materialArray(CreateDX11BitmapArray(device, dc))
	{

	}

	virtual ~DX11Materials()
	{
	}

	void Free() override
	{
		delete this;
	}

	cstr GetMaterialTextureName(MaterialId id) const override
	{
		size_t index = (size_t)id;
		if (index >= materialArray->TextureCount()) return nullptr;
		return idToMaterialName[index].c_str();
	}

	void GetMaterialArrayMetrics(MaterialArrayMetrics& metrics) const override
	{
		metrics.NumberOfElements = (int32)materialArray->TextureCount();
		metrics.Width = materialArray->MaxWidth();
	}

	struct MaterialDef
	{
		// Ping path where material is defined
		HString srcPath;

		// Line number where material is defined
		int srcIndex;

		HString name;
		HString colourMap;
		HString normalMap;
		HString roughnessMap;
		RGBAb diffuse;
		RGBAb emission;
	};

	std::vector<MaterialDef> materialDefs;
	stringmap<int> mapMaterialNameToIndex;

	HString currentSrcPath;

	void AddMaterialFromMaterialDirective(const Rococo::Sex::SEXML::ISEXMLDirective& mat)
	{
		/*
		(Material
			(Name <unique-id>)
			(ColourMap <pingpath>)
			(NormalMap <pingpath>)
			(RoughnessMap <pingpath>)
			(Diffuse 0xRGBA)
			(Emission 0xRGBA)
		)
		*/

		auto& aMatName = mat["Name"];
		cstr name = AsString(aMatName).c_str();
		
		auto& aColourMap = mat["ColourMap"];
		cstr colourMap = AsString(aColourMap).c_str();

		auto& aNormalMap = mat["NormalMap"];
		cstr normalMap = AsString(aNormalMap).c_str();

		auto& aRoughness = mat["RoughnessMap"];
		cstr roughnessMap = AsString(aRoughness).c_str();

		auto& aDiffuse = mat["Diffuse"];
		RGBAb diffuse = AsAtomicRGBA(aDiffuse);

		auto& aEmission = mat["Emission"];
		RGBAb emission = AsAtomicRGBA(aEmission);

		auto i = mapMaterialNameToIndex.insert(name, 0);
		if (!i.second)
		{
			int index = i.first->second - 1;
			if (index <= 0 || index >= (int)materialDefs.size())
			{
				Throw(mat.S(), "Duplicate material name '%s'", name);
			}

			const auto& original = materialDefs[index];
			
			// Already exists
			Throw(mat.S(), "Duplicate material name '%s'. Original defined in %s line %d", original.srcPath.c_str(), original.srcIndex);
		}

		materialDefs.emplace_back(MaterialDef{ currentSrcPath, mat.S().Start().y + 1, name, colourMap, normalMap, roughnessMap, diffuse, emission});

		i.first->second = (int) materialDefs.size();
	}

	void LoadMaterialTextureArray(IMaterialTextureArrayBuilder& builder)  override
	{
		nameToMaterialId.clear();
		idToMaterialName.clear();

		AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(32_kilobytes);

		for (size_t i = 0; i < builder.Count(); ++i)
		{
			cstr pingPath = builder.GetMaterialDef(i);

			buffer->Resize(0);
			installation.LoadResource(pingPath, *buffer, 1_megabytes);

			if (buffer->Length() == 0)
			{
				Throw(0, "Empty file: %s", pingPath);
			}

			cstr s = (cstr) buffer->GetData();

			currentSrcPath = pingPath;

			OS::ParseSXMLFromString(pingPath, s, [this](const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives) 
				{
					for (size_t index = 0; index < topLevelDirectives.NumberOfDirectives(); ++index)
					{
						auto& dir = topLevelDirectives[index];
						if (Eq(dir.FQName(), "Material"))
						{
							this->AddMaterialFromMaterialDirective(dir);
						}
					}
				}
			);
		}


		/*

		idToMaterialName.resize(builder.Count());

		for (size_t i = 0; i < builder.Count(); ++i)
		{
			struct ANON : IEventCallback<MaterialTextureArrayBuilderArgs>
			{
				DX11Materials* This;
				size_t i;
				int32 txWidth;
				cstr name; // valid for duration of OnEvent callback

				void OnEvent(MaterialTextureArrayBuilderArgs& args) override
				{
					name = args.name;

					auto ext = GetFileExtension(args.name);
					if (EqI(ext, ".sxymat"))
					{
						Throw(0, "Error loading material texture: %s: Only extension allowed is .sxymat", name);
					}

					This->nameToMaterialId[args.name] = MaterialId((int32)i);
					auto t = This->nameToMaterialId.find(args.name);
					This->idToMaterialName[i] = t->first;
				}
			} onMaterial;
			onMaterial.This = this;
			onMaterial.i = i;

			builder.LoadMaterialForIndex(i, onMaterial);
		}

		*/
	}

	MaterialId GetMaterialId(cstr name) const
	{
		if (name[0] == '#')
		{
			// Macro, so expand
			U8FilePath expandedPath;
			if (installation.TryExpandMacro(name, expandedPath))
			{
				return GetMaterialId(expandedPath);
			}
			else
			{
				return MaterialId(-1);
			}
		}

		auto i = nameToMaterialId.find(name);
		return i != nameToMaterialId.end() ? i->second : MaterialId(-1);
	}

	IDX11BitmapArray& Textures() override
	{
		return *materialArray;
	}

	void ShowVenue(IMathsVisitor& visitor) override
	{
		for (size_t i = 0; i < materialArray->TextureCount(); ++i)
		{
			char name[64];
			SafeFormat(name, "MatId %u", i);
			visitor.ShowSelectableString("overlay.select.material", name, "  %s", idToMaterialName[i].c_str());
		}
	}
};

namespace Rococo::DX11
{
	IDX11Materials* CreateMaterials(IO::IInstallation& installation, ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new DX11Materials(installation, device, dc);
	}
}