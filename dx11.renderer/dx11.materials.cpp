#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include "rococo.hashtable.h"
#include "rococo.imaging.h"

using namespace Rococo::DX11;

struct DX11Materials : IDX11Materials
{
	IO::IInstallation& installation;
	stringmap<MaterialId> nameToMaterialId;
	std::vector<HString> idToMaterialName;
	AutoFree<IDX11TextureArray> materialArray;

	DX11Materials(IO::IInstallation& _installation, ID3D11Device& device, ID3D11DeviceContext& dc):
		installation(_installation),
		materialArray(CreateDX11TextureArray(device, dc))
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

	void LoadMaterialTextureArray(IMaterialTextureArrayBuilder& builder)  override
	{
		int32 txWidth = builder.TexelWidth();
		materialArray->ResetWidth(builder.TexelWidth());
		materialArray->Resize(builder.Count());
		nameToMaterialId.clear();
		idToMaterialName.clear();

		idToMaterialName.resize(builder.Count());

		for (size_t i = 0; i < builder.Count(); ++i)
		{
			struct ANON : IEventCallback<MaterialTextureArrayBuilderArgs>, Imaging::IImageLoadEvents
			{
				DX11Materials* This;
				size_t i;
				int32 txWidth;
				cstr name; // valid for duration of OnEvent callback

				void OnEvent(MaterialTextureArrayBuilderArgs& args) override
				{
					name = args.name;

					auto ext = GetFileExtension(args.name);
					if (EqI(ext, ".tif") || EqI(ext, ".tiff"))
					{
						Rococo::Imaging::DecompressTiff(*this, args.buffer.GetData(), args.buffer.Length());
					}
					else if (EqI(ext, ".jpg") || EqI(ext, ".jpeg"))
					{
						Rococo::Imaging::DecompressJPeg(*this, args.buffer.GetData(), args.buffer.Length());
					}
					else
					{
						Throw(0, "Error loading material texture: %s: Only extensions allowed are tif, tiff, jpg and jpeg", name);
					}

					This->nameToMaterialId[args.name] = (MaterialId)i;
					auto t = This->nameToMaterialId.find(args.name);
					This->idToMaterialName[i] = t->first;
				}

				void OnError(const char* message) override
				{
					Throw(0, "Error loading material texture: %s: %s", name, message);
				}

				void OnRGBAImage(const Vec2i& span, const RGBAb* pixels) override
				{
					if (span.x != txWidth || span.y != txWidth)
					{
						Throw(0, "Error loading texture %s. Only %d x %d dimensions supported", name, txWidth, txWidth);
					}

					This->materialArray->WriteSubImage(i, pixels, GuiRect{ 0, 0, txWidth, txWidth });
				}

				void OnAlphaImage(const Vec2i& span, const uint8* data) override
				{
					Throw(0, "Error loading texture %s. Only RGB and ARGB formats supported", name);
				}

			} onTexture;
			onTexture.This = this;
			onTexture.i = i;
			onTexture.txWidth = txWidth;

			builder.LoadTextureForIndex(i, onTexture);
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
				return -1;
			}
		}

		auto i = nameToMaterialId.find(name);
		return i != nameToMaterialId.end() ? i->second : -1.0f;
	}

	IDX11TextureArray& Textures() override
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