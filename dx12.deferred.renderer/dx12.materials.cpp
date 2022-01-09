#include <rococo.os.win32.h>
#include <rococo.strings.h>
#include <rococo.api.h>
#include <rococo.window.h>
#include <rococo.maths.h>
#include <rococo.dx12.h>
#include <rococo.auto-release.h>
#include <d3d12.h>
#include "rococo.dx12-msoft.h" // If this does not compile update your Windows 10 kit thing
#include <vector>
#include <rococo.renderer.h>
#include <rococo.textures.h>
#include <rococo.hashtable.h>
#include <rococo.imaging.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Textures;

namespace ANON
{
	class DX12MaterialList: public IDX12MaterialList
	{
	private:
		DX12WindowInternalContext ic;
		AutoFree<IDX12TextureArray> materialArray;
		stringmap<MaterialId> nameToMaterialId;
		std::vector<std::string> idToMaterialName;
		ITextureMemory& txMemory;
		IInstallation& installation;
	public:
		DX12MaterialList(DX12WindowInternalContext& ref_ic, ITextureMemory& ref_txMemory, IInstallation& ref_installation) :
			ic(ref_ic), installation(ref_installation), txMemory(ref_txMemory)
		{
			DX12TextureArraySpec spec{ txMemory };
			materialArray = CreateDX12TextureArray("materials", true, spec, ic);
		}

		MaterialId GetMaterialId(cstr name) const override
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

		void LoadMaterialTextureArray(IMaterialTextureArrayBuilder& builder) override
		{
			int32 txWidth = builder.TexelWidth();
			materialArray->SetDimensions(txWidth, txWidth, (int)builder.Count());
			nameToMaterialId.clear();
			idToMaterialName.clear();

			idToMaterialName.resize(builder.Count());

			for (size_t i = 0; i < builder.Count(); ++i)
			{
				struct ANON : IEventCallback<MaterialTextureArrayBuilderArgs>, Imaging::IImageLoadEvents
				{
					DX12MaterialList* This;
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

						This->materialArray->WriteSubImage((int)i, pixels, GuiRect{ 0, 0, txWidth, txWidth });
					}

					virtual void OnAlphaImage(const Vec2i& span, const uint8* data)
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

		virtual ~DX12MaterialList()
		{

		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Graphics
{
	IDX12MaterialList* CreateMaterialList(DX12WindowInternalContext& ic, ITextureMemory& txMemory, IInstallation& installation)
	{
		return new ANON::DX12MaterialList(ic, txMemory, installation);
	}
}