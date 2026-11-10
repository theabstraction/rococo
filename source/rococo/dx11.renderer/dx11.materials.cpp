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

struct TxTableId
{
	enum class Type : int32 { Colour, Normal, Roughness } type;
	int32 span;
	DXGI_FORMAT format;

	static inline uint64_t Mix64(uint64_t x) {           // splitmix64 finaliser
		x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
		x ^= x >> 27; x *= 0x94d049bb133111ebULL;
		x ^= x >> 31; return x;
	}

	static inline uint64_t Hash3(int32_t a, int32_t b, int32_t c)
	{
		uint64_t h = Mix64((uint32_t)a);
		h = Mix64(h ^ (uint32_t)b);
		h = Mix64(h ^ (uint32_t)c);
		return h;
	}

	bool operator==(const TxTableId& item) const
	{
		return item.type == type && item.span == span && item.format == format;
	}
};

template<> struct std::hash<TxTableId>
{
	size_t operator()(const TxTableId& k) const noexcept
	{
		return (size_t)TxTableId::Hash3((int32)k.type, (int32)k.span, (int32)k.format);
	}
};

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

	size_t colourIndex;
	TxTableId colourTableId;

	IDX11BitmapArray* colourTextureArray;
};


struct DX11Materials : IDX11MaterialsSupervisor
{
	IO::IInstallation& installation;
	stringmap<MaterialId> nameToMaterialId;
	std::vector<MaterialDef> materialDefs;
	stringmap<int> mapMaterialNameToIndex;
	std::unordered_map<TxTableId, AutoFree<IDX11BitmapArray>> mapTextureTableIdToTextureArray;

	ID3D11Device& device;
	ID3D11DeviceContext& dc;

	DX11Materials(IO::IInstallation& _installation, ID3D11Device& _device, ID3D11DeviceContext& _dc): device(_device), dc(_dc),
		installation(_installation)
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
		size_t index = (size_t)(id - 1.0f);
		if (index >= materialDefs.size()) return nullptr;
		return materialDefs[index].name.c_str();
	}

	void GetMaterialArrayMetrics(MaterialArrayMetrics& metrics) const override
	{
		int sum = 0;
		int maxWidth = 0;
		for (auto& i : mapTextureTableIdToTextureArray)
		{
			sum += (int) i.second->TextureCount();
			maxWidth = max(maxWidth, i.first.span);
		}

		metrics.NumberOfElements = sum;
		metrics.Width = maxWidth;
	}

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

	AutoFree<IExpandingBuffer> matBufferFile = CreateExpandingBuffer(32_kilobytes);

	void ClearMaterials() override
	{
		materialDefs.clear();
		mapMaterialNameToIndex.clear();
		mapTextureTableIdToTextureArray.clear();
	}

	using ColourCallbackFunction = Rococo::Function<void(const Vec2i& span, const RGBAb* data)>;
	using AlphaCallbackFunction = Rococo::Function<void(const Vec2i& span, const uint8* data)>;

	struct ImageLoader : Imaging::IImageLoadEvents
	{
		HString err;

		AlphaCallbackFunction onAlpha;
		ColourCallbackFunction onRGBAb;

		void OnError(const char* message) override
		{
			err = message;
		}

		void OnRGBAImage(const Vec2i& span, const RGBAb* data) override
		{
			onRGBAb.Invoke(span, data);
		}

		void OnAlphaImage(const Vec2i& span, const uint8* data) override
		{
			onAlpha.Invoke(span, data);
		}
	};

	void DecompressImageFile(const IExpandingBuffer& buffer, cstr pingPath, ColourCallbackFunction onColour, AlphaCallbackFunction onAlpha)
	{
		ImageLoader loader;
		loader.onRGBAb = onColour;
		loader.onAlpha = onAlpha;

		cstr ext = GetFileExtension(pingPath);

		if (EqI(ext, ".jpg") || EqI(ext, ".jpeg"))
		{
			Imaging::DecompressJPeg(loader, buffer.GetData(), buffer.Length());
			if (loader.err.length() > 0)
			{
				Throw(0, "Bad file %s for %s from %s line %d", loader.err.c_str(), pingPath);
			}
		}
		else if (EqI(ext, "*.tif") || EqI(ext, ".tiff"))
		{
			Imaging::DecompressTiff(loader, buffer.GetData(), buffer.Length());
			if (loader.err.length() > 0)
			{
				Throw(0, "Bad file %s for %s from %s line %d", loader.err.c_str(), pingPath);
			}
		}
		else
		{
			Throw(0, "Bad file extension %s for %s from %s line %d", ext, pingPath);
		}
	}

	void LoadMaterialTextureFile(cstr pingPath) override
	{
		matBufferFile->Resize(0);
		installation.LoadResource(pingPath, *matBufferFile, 1_megabytes);

		if (matBufferFile->Length() == 0)
		{
			Throw(0, "Empty file: %s", pingPath);
		}

		cstr s = (cstr)matBufferFile->GetData();

		currentSrcPath = pingPath;

		OS::ParseSXMLFromString(pingPath, s, [this](const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives) 
			{
				for (size_t index = 0; index < topLevelDirectives.NumberOfDirectives(); ++index)
				{
					auto& dir = topLevelDirectives[index];
					if (Eq(dir.FQName(), "Material"))
					{
						// Add all the material entries to materialDefs and mapMaterialNameToIndex
						this->AddMaterialFromMaterialDirective(dir);
					}
				}
			}
		);
	}

	IDX11BitmapArray* GetTextureTable(Vec2i span, TxTableId::Type type, DXGI_FORMAT format, OUT TxTableId& id)
	{
		if (span.x != span.y)
		{
			Throw(0, "Span.x != Span.y (%d vs %d)", span.x, span.y);
		}

		enum { MAX_TEXTURE_SPAN = 2048 };

		if (span.x > MAX_TEXTURE_SPAN)
		{
			Throw(0, "Span.x > %d", span.x, MAX_TEXTURE_SPAN);
		}

		id.type = type;
		id.span = span.x;
		id.format = format;
		auto i = mapTextureTableIdToTextureArray.insert(std::make_pair(id, AutoFree<IDX11BitmapArray>(nullptr)));
		if (i.second)
		{
			// Fresh insert
			i.first->second = CreateDX11BitmapArray(device, dc);
		}

		return i.first->second;
	}

	void ReserveTexture(MaterialDef& def, TxTableId::Type type, Vec2i span, DXGI_FORMAT format)
	{
		TxTableId id;
		IDX11BitmapArray* textures = GetTextureTable(span, type, format, OUT id);
		textures->AddTexture();

		def.colourTableId = id;
		def.colourIndex = textures->TextureCount() - 1;
		def.colourTextureArray = textures;
	}

	void CommitTexture(MaterialDef& def, TxTableId::Type type, Vec2i span, DXGI_FORMAT format, const void* data)
	{
		TxTableId id;
		IDX11BitmapArray* textures = GetTextureTable(span, type, format, OUT id);

		// Commit
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			textures->WriteSubImage(def.colourIndex, (const RGBAb*)data, GuiRect{ 0,0,span.x, span.y });
			break;
		case DXGI_FORMAT_A8_UNORM:
			textures->WriteSubImage(def.colourIndex, (const uint8*)data, span);
			break;
		default:
			Throw(0, "Bad DXGI format: %d", format);
		}
	}

	void ReserveMaterial(MaterialDef& def)
	{
		cstr colourPingPath = def.colourMap.c_str();
		cstr normalPingPath = def.normalMap.c_str();
		cstr roughnessPath = def.roughnessMap.c_str();
		UNUSED(roughnessPath);
		UNUSED(normalPingPath);

		if (*colourPingPath)
		{
			installation.LoadResource(colourPingPath, *matBufferFile, 32_megabytes);

			// TODO -> have our material def file record spans of each texture, so we don't have to read image data twice

			DecompressImageFile(*matBufferFile, colourPingPath,
				[this, &def](const Vec2i& span, const RGBAb* data)
				{
					UNUSED(data);
					this->ReserveTexture(def, TxTableId::Type::Colour, span, DXGI_FORMAT_R8G8B8A8_UNORM);
				},

				[this, &def](const Vec2i& span, const uint8* data)
				{
					UNUSED(data);
					this->ReserveTexture(def, TxTableId::Type::Colour, span, DXGI_FORMAT_A8_UNORM);
				}
			);
		}
	}

	void CommitMaterial(MaterialDef& def)
	{
		cstr colourPingPath = def.colourMap.c_str();
		cstr normalPingPath = def.normalMap.c_str();
		cstr roughnessPath = def.roughnessMap.c_str();
		UNUSED(roughnessPath);
		UNUSED(normalPingPath);

		if (*colourPingPath)
		{
			installation.LoadResource(colourPingPath, *matBufferFile, 32_megabytes);
			DecompressImageFile(*matBufferFile, colourPingPath,
				[this, &def](const Vec2i& span, const RGBAb* data)
				{
					this->CommitTexture(def, TxTableId::Type::Colour, span, DXGI_FORMAT_R8G8B8A8_UNORM, data);
				},

				[this, &def](const Vec2i& span, const uint8* data)
				{
					this->CommitTexture(def, TxTableId::Type::Colour, span, DXGI_FORMAT_A8_UNORM, data);
				}
			);
		}
	}

	void CommitMaterials() override
	{
		for (auto& def : materialDefs)
		{
			try
			{
				ReserveMaterial(def);
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "Error with material %s defined in %s line %d: %s", def.name.c_str(), def.srcPath.c_str(), def.srcIndex, ex.Message());
			}
		}

		for (auto& def : materialDefs)
		{
			try
			{
				CommitMaterial(def);
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "Error with material %s defined in %s line %d: %s", def.name.c_str(), def.srcPath.c_str(), def.srcIndex, ex.Message());
			}
		}
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

		auto i = mapMaterialNameToIndex.find(name);
		return i != mapMaterialNameToIndex.end() ? MaterialId(i->second) : MaterialId(-1);
	}

	IDX11BitmapArray& GetTextureArrayForMaterial(MaterialId id) override
	{
		size_t index = (size_t)(id - 1.0f);
		if (index >= materialDefs.size())
		{
			Throw(0, "Bad material index");
		}

		return *materialDefs[index].colourTextureArray;
	}

	void ShowVenue(IMathsVisitor& visitor)
	{
		int index = 0;
		for (auto& m : this->materialDefs)
		{
			index++;

			char id[64];
			SafeFormat(id, "MatId %u", index);
			visitor.ShowSelectableString("overlay.select.material", id, "  %s", m.name.c_str());
		}
	}
};

namespace Rococo::DX11
{
	IDX11MaterialsSupervisor* CreateMaterials(IO::IInstallation& installation, ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new DX11Materials(installation, device, dc);
	}
}