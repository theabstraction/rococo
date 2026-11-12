#include <3D\rococo.material-builder.h>
#include <mplat/mplat.events.h>
#include <rococo.renderer.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.strings.h>
#include <vector>
#include <rococo.hashtable.h>
#include <rococo.functional.h>
#include "..\rococo.mplat\code-gen\Rococo.Graphics.IShaderOptionsConfig.sxh.h"

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Entities;
using namespace Rococo::Events;
using namespace Rococo::Strings;

namespace Rococo::Graphics
{
	struct ShaderOption
	{
		HString interfaceName;
		HString className;
	};

	struct ShaderOptions : IShaderOptionsSupervisor, IShaderOptionsConfig
	{
		std::vector<ShaderOption> options;

		size_t NumberOfOptions() const override
		{
			return options.size();
		}

		void GetOption(size_t index, OUT cstr& interfaceName, OUT cstr& className) override
		{
			auto& opt = options[index];
			interfaceName = opt.interfaceName;
			className = opt.className;
		}

		void Set(const fstring& interfaceName, const fstring& className) override
		{
			for (auto& opt : options)
			{
				if (Eq(opt.interfaceName, interfaceName))
				{
					opt.className = className;
					return;
				}
			}

			ShaderOption opt;
			opt.interfaceName = interfaceName;
			opt.className = className;

			options.push_back(opt);
		}

		IShaderOptionsConfig& Config() override
		{
			return *this;
		}

		void Free() override
		{
			delete this;
		}
	};

	struct MaterialBuilder : public IMaterialBuilderSupervisor
	{
		IRenderer& renderer;
		IPublisher& publisher;

		// ecs - The Entity Component System

		int32 enumerationDepth{ 0 };

		MaterialBuilder(IRenderer& _renderer, IPublisher& _publisher) :
			renderer(_renderer), publisher(_publisher)
		{
		}

		ID_CUBE_TEXTURE CreateCubeTexture(const fstring& folder, const fstring& extension) override
		{
			return renderer.Textures().CubeTextures().CreateCubeTexture(folder, extension);
		}

		void BuildPaths(const wchar_t* ext, std::vector<HString>& paths, const fstring& folder)
		{
			auto& installation = renderer.Installation();
			
			struct PathBuilder : public IEventCallback<IO::FileItemData>
			{
				std::vector<HString>& filenames;
				IO::IInstallation& installation;
				const wchar_t* legalExt;

				void OnEvent(IO::FileItemData& item) override
				{
					if (item.isDirectory) return;

					const wchar_t* ext = GetFileExtension(item.itemRelContainer);
					if (Eq(ext, legalExt))
					{
						U8FilePath pingName;
						installation.ConvertSysPathToPingPath(item.fullPath, pingName);
						filenames.push_back((cstr)pingName);
					}
				}

				PathBuilder(std::vector<HString>& _paths, IO::IInstallation& _installation): filenames(_paths), installation(_installation)
				{

				}
			} addPathToBuilder (paths, installation);

			addPathToBuilder.legalExt = ext;

			WideFilePath sysPath;
			installation.ConvertPingPathToSysPath(folder, sysPath);
			IO::ForEachFileInDirectory(sysPath, addPathToBuilder, true);
		}

		void LoadMaterialArray(const fstring& folder) override
		{
			Events::BusyEvent beStart;
			beStart.isNowBusy = true;
			beStart.message = "Loading textures";
			Format(beStart.pingPath, "%s", folder.buffer);
			publisher.Publish(beStart, Rococo::Events::evBusy);

			std::vector<HString> materialPingPaths;
			BuildPaths(L".sexml", materialPingPaths, folder);

			renderer.Materials().ClearMaterials();
				
			for (auto& pingPath : materialPingPaths)
			{
				renderer.Materials().LoadMaterialTextureFile(pingPath);
			}

			renderer.Materials().CommitMaterials();

			Events::BusyEvent beEnd;
			beEnd.isNowBusy = false;
			beEnd.message = "";
			beEnd.pingPath.buf[0] = 0;
			publisher.Publish(beEnd, Rococo::Events::evBusy);

			RefreshCategories();
		}

		std::unordered_map<MaterialCategory, std::vector<MaterialId>> categories;

		stringmap<MaterialCategory> catEnum =
		{
			{ "wood", MaterialCategory::Wood },
			{ "stone", MaterialCategory::Stone },
			{ "rock", MaterialCategory::Rock },
			{ "metal", MaterialCategory::Metal },
			{ "marble", MaterialCategory::Marble }
		};

		MaterialCategory GetMaterialCateogry(MaterialId id)
		{
			cstr name = renderer.Materials().GetMaterialTextureName(id);
			for (auto& j : catEnum)
			{
				if (strstr(name, j.first))
				{
					return j.second;
				}
			}

			return MaterialCategory::Marble; // all that is not anythng is marble
		}

		void AddMaterialToCategory(MaterialId id)
		{
			cstr name = renderer.Materials().GetMaterialTextureName(id);
			if (!name)
			{
				return;
			}

			cstr firstDot = ForwardFind('.', Substring::ToSubstring(name));
			Substring scategory = { name, firstDot };

			char category[64];
			scategory.CopyWithTruncate(category, sizeof category);
			
			auto i = catEnum.find(category);
			if (i == catEnum.end())
			{
				return;
			}
				
			auto c = categories.find(i->second);
			if (c == categories.end())
			{
				c = categories.insert(std::make_pair(i->second, std::vector<MaterialId>())).first;
			}

			c->second.push_back(id);			
		}

		void RefreshCategories()
		{
			categories.clear();

			MaterialArrayMetrics metrics;
			renderer.Materials().GetMaterialArrayMetrics(metrics);

			auto content = renderer.Installation().Content();
			UNUSED(content);

			renderer.Materials().ForEachMaterial(
				[this](MaterialId id)
				{
					AddMaterialToCategory(id);
				}
			);
		}

		int32 CountMaterialsInCategory(Rococo::Graphics::MaterialCategory category) override
		{
			auto i = categories.find(category);
			return i == categories.end() ? 0 : (int32)i->second.size();
		}

		MaterialId GetMaterialId(Rococo::Graphics::MaterialCategory category, int32 index) override
		{
			auto i = categories.find(category);
			if (i == categories.end())
			{
				Throw(0, "Instances::GetMaterialId(...) -> No materials in category %d", category);
			}

			if (index < 0) index &= 0x4FFFFFFF; // Just do something to create a positive number

			int32 x = index % (int32)i->second.size();
			return i->second[x];
		}

		MaterialId GetMaterialDirect(const fstring& pingPath) override
		{
			return renderer.Materials().GetMaterialId(pingPath);
		}

		MaterialId GetRandomMaterialId(Rococo::Graphics::MaterialCategory category) override
		{
			auto i = categories.find(category);
			if (i == categories.end())
			{
				Throw(0, "Instances::GetRandomMaterialId(...) -> No materials in category %d", category);
			}

			int32 index = rand() % (int32)i->second.size();
			return i->second[index];
		}

		void SetMaterialMacro(const fstring& pingPath) override
		{
			renderer.Installation().Macro("#m", pingPath);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Graphics::Construction
{
	IMaterialBuilderSupervisor* CreateMaterialsBuilder(IRenderer& renderer, Events::IPublisher& publisher)
    {
        return new MaterialBuilder(renderer, publisher);
    }

	IShaderOptionsSupervisor* CreateShaderOptions()
	{
		return new ShaderOptions();
	}
}