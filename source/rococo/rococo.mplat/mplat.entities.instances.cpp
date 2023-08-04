#include <rococo.mplat.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.animation.h>
#include <rococo.handles.h>
#include <rococo.hashtable.h>
#include "mplat.components.h"
#include <rococo.maths.h>
#include <components/rococo.components.body.h>
#include <components/rococo.components.animation.h>
#include <components/rococo.components.skeleton.h>
#include <vector>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Graphics;
   using namespace Rococo::Entities;

   struct Instances : public IInstancesSupervisor
   {      
      IMeshBuilderSupervisor& meshBuilder;
      IRenderer& renderer;
	  Events::IPublisher& publisher;

      // ecs - The Entity Component System
      IECS& ecs;

      int32 enumerationDepth{ 0 };

      Instances(IMeshBuilderSupervisor& _meshBuilder, IRenderer& _renderer, Events::IPublisher& _publisher, IECS& _ecs, size_t maxEntities) :
          meshBuilder(_meshBuilder), renderer(_renderer), publisher(_publisher), ecs(_ecs)
      {
          UNUSED(maxEntities);
      }

      ~Instances()
      {
         Clear();
      }

      Rococo::Graphics::IMeshBuilder& MeshBuilder() override
      {
         return meshBuilder;
      }

      ID_ENTITY Add(ID_SYS_MESH meshId, const Matrix4x4& model, const Vec3& scale, ID_ENTITY parentId)
      {
          // 0 scale means scale is passed as the model matrix

          if (scale.x != 0)
          {
              float d = Determinant(model);
              if (d < 0.975f || d > 1.025f)
              {
                  Throw(0, "Bad model matrix. Determinant was %f", d);
              }
          }

          auto id = ecs.NewROID();

          auto body = API::ForIBodyComponent::Add(id);
          body->SetModelMatrix(model);
          body->SetParent(parentId);
          body->SetMesh(meshId);
          body->SetScale((scale.x != 0) ? scale : Vec3{ 1.0f, 1.0f, 1.0f });

          return id;
      }

      ID_ENTITY AddSkeleton(const fstring& skeletonName, const Matrix4x4& model) override
      {
            float d = Determinant(model);
            if (d < 0.975f || d > 1.025f)
            {
                Throw(0, "Bad model matrix. Determinant was %f", d);
            }

            auto id = ecs.NewROID();

            auto body = API::ForIBodyComponent::Add(id);
            body->SetModelMatrix(model);
            body->SetParent(ID_ENTITY::Invalid());
            body->SetMesh(ID_SYS_MESH::Invalid());
            body->SetScale(Vec3{ 1.0f, 1.0f, 1.0f });

            auto skeleton = API::ForISkeletonComponent::Add(id);
            skeleton->SetSkeleton(skeletonName);

            return id;
      }

      /*
      ID_ENTITY AddBody(const fstring& modelName, const Matrix4x4& model, const Vec3& scale, ID_ENTITY parentId) override
      {
         ID_SYS_MESH meshId;
		 AABB bounds;
         if (!meshBuilder.TryGetByName(modelName, meshId, bounds))
         {
            Throw(0, "Cannot find model: %s", modelName.buffer);
         }

         return Add(meshId, model, scale, parentId);
      }
      */

      ID_ENTITY AddGhost(const Matrix4x4& model, ID_ENTITY parentId) override
      {
		  return Add(ID_SYS_MESH::Invalid(), model, { 1,1,1 }, parentId);
      }

      void BindSkeletonToBody(const fstring& skeletonName, ID_ENTITY idBody) override
      {
          if (skeletonName.length < 1)
          {
              Throw(0, "%s: skeleton name was blank", __FUNCTION__);
          }

          auto body = API::ForISkeletonComponent::Get(idBody);
          if (!body)
          {
              body = API::ForISkeletonComponent::Add(idBody);
          }

          body->SetSkeleton(skeletonName);
      }

      void Delete(ID_ENTITY id) override
      {
          ecs.Deprecate(id);
      }

      boolean32 TryGetModelToWorldMatrix(ID_ENTITY id, Matrix4x4& model) override
      {
          auto body = API::ForIBodyComponent::Get(id);
          if (!body)
          {
              model = Matrix4x4::Identity();
              return false;
          }

          model = body->Model();
          return true;
      }

      std::vector<const Matrix4x4*> modelStack;

      void ConcatenatePositionVectors(ID_ENTITY leafId, Vec3& position) override
      {
          auto body = API::ForIBodyComponent::Get(leafId);
          if (!body)
          {
              Throw(0, "Missing entity");
          }

          position += body->Model().GetPosition();
      }

	  ID_CUBE_TEXTURE CreateCubeTexture(const fstring& folder, const fstring& extension) override
	  {
		  return renderer.Textures().CubeTextures().CreateCubeTexture(folder, extension);
	  }

      void ConcatenateModelMatrices(ID_ENTITY leafId, Matrix4x4& m) override
      {
          auto body = API::ForIBodyComponent::Get(leafId);
          if (!body)
          {
              Throw(0, "Missing entity");
          }

          m = body->Model();

          float Dm = Determinant(m);
          if (Dm < 0.9f || Dm > 1.1f)
          {
              Throw(0, "Bad model matrix for entity %lld. Det M = %f", leafId.Value(), Dm);
          }
      }

      void ForAll(IEntityCallback& cb)
      {
         RecursionGuard guard(enumerationDepth);

         int64 count = 0;

         API::ForIBodyComponent::ForEach(
             [&count,&cb](Components::ROID roid, Components::IBodyComponent& body) 
             {
                 cb.OnEntity(count++, body, roid);
                 return EFlowLogic::CONTINUE;
             }
         );
      }

      void GetPosition(ID_ENTITY id, Vec3& position)
      {
          auto body = API::ForIBodyComponent::Get(id);
          if (body)
          {
              position = body->Model().GetPosition();
          }
          else
          {
              position = { 0,0,0 };
          }
      }

      void EnableAnimation(ID_ENTITY id) override
      {
          auto animationComponent = API::ForIAnimationComponent::Add(id);
      }

      void AddAnimationFrame(ID_ENTITY id, const fstring& frameName, Seconds duration, boolean32 loop) override
      {
          auto a = API::ForIAnimationComponent::Get(id);
          if (!a)
          {
              Throw(0, "%s: no animation component for ID_ENTITY [%d v%d]. Call (instances.EnableAnimation <bodyId>) after object creation", __FUNCTION__, id.index, id.salt);
          }

          a->AddKeyFrame(frameName, duration, loop);
      }

	  void LoadMaterialArray(const fstring& folder, int32 txWidth) override
	  {
		  struct: public IEventCallback<IO::FileItemData>, IMaterialTextureArrayBuilder
		  {
			  int32 txWidth;
			  std::vector<HString> filenames;
              IO::IInstallation* installation;
			  AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(4_megabytes);
			  Events::IPublisher* publisher;
			  Instances* This;

			  char pingPath[IO::MAX_PATHLEN];
			  wchar_t sysSearchPath[IO::MAX_PATHLEN];

			  void OnEvent(IO::FileItemData& item) override
			  {
                  if (item.isDirectory) return;

				  auto ext = GetFileExtension(item.itemRelContainer);
				  if (Eq(ext, L".jpeg") || Eq(ext, L".jpg") || Eq(ext, L"tif") || Eq(ext, L"tiff"))
				  {
					  U8FilePath pingName;
					  installation->ConvertSysPathToPingPath(item.fullPath, pingName);
					  filenames.push_back((cstr)pingName);
				  }
			  }

			  size_t Count() const override
			  {
				  return filenames.size();
			  }

			  int32 TexelWidth() const override
			  {
				  return txWidth;
			  }

			  void LoadTextureForIndex(size_t index, IEventCallback<MaterialTextureArrayBuilderArgs>& onLoad) override
			  {
				  auto path = filenames[index].c_str();

				  Events::BusyEvent be;
				  be.isNowBusy = true;
				  be.message = "Loading textures";
				  Format(be.pingPath, "%s", path);
				  publisher->Publish(be, Rococo::Events::evBusy);

				  installation->LoadResource(path, *buffer, 64_megabytes);
				  MaterialTextureArrayBuilderArgs args{ *buffer, path };
				  onLoad.OnEvent(args);
			  }
		  } materialIndex;

          // First thing we do is build up a list of filenames for each material
          materialIndex.txWidth = txWidth;
          materialIndex.installation = &renderer.Installation();
          materialIndex.publisher = &publisher;
          materialIndex.This = this;

		  SafeFormat(materialIndex.pingPath, IO::MAX_PATHLEN, "%s", folder.buffer);

		  WideFilePath sysPath;
          materialIndex.installation->ConvertPingPathToSysPath(materialIndex.pingPath, sysPath);

		  swprintf_s(materialIndex.sysSearchPath, IO::MAX_PATHLEN, L"%s", sysPath.buf);
	
		  IO::ForEachFileInDirectory(sysPath, materialIndex, true);

		  if (materialIndex.filenames.empty()) return;

		  Events::BusyEvent be;
		  be.isNowBusy = true;
		  be.message = "Loading textures";
		  be.pingPath.buf[0] = 0;
		  publisher.Publish(be, Rococo::Events::evBusy);

          // Then we tell the renderer to open the files by index
		  renderer.Materials().LoadMaterialTextureArray(materialIndex);

		  be.isNowBusy = false;
		  be.message = "";
		  be.pingPath.buf[0] = 0;
		  publisher.Publish(be, Rococo::Events::evBusy);

		  RefreshCategories();
	  }

	  std::unordered_map<MaterialCategory, std::vector<MaterialId>> categories;

	  stringmap<MaterialCategory> subdirToCatEnum =
	  {
		  { "/wood/", MaterialCategory::Wood },
		  { "/stone/", MaterialCategory::Stone },
		  { "/rock/", MaterialCategory::Rock },
		  { "/metal/", MaterialCategory::Metal },
		  { "/marble/", MaterialCategory::Marble }
	  };

      MaterialCategory GetMaterialCateogry(MaterialId id)
      {
          cstr name = renderer.Materials().GetMaterialTextureName(id);
          for (auto& j : subdirToCatEnum)
          {
              if (strstr(name, j.first))
              {
                  return j.second;
              }
          }

          return MaterialCategory::Marble; // all that is not anythng is marble
      }

	  void RefreshCategories()
	  {
		  categories.clear();

		  MaterialArrayMetrics metrics;
		  renderer.Materials().GetMaterialArrayMetrics(metrics);

		  auto content = renderer.Installation().Content();
          UNUSED(content);

		  for (size_t i = 0; i < metrics.NumberOfElements; ++i)
		  {
			  auto id = (MaterialId)i;
			  cstr name = renderer.Materials().GetMaterialTextureName(id);
			  if (name)
			  {
				  char fullname[IO::MAX_PATHLEN];
				  SafeFormat(fullname, IO::MAX_PATHLEN, "%s", name);
				  OS::ToUnixPath(fullname);
				  cstr subpath = fullname;

				  for (auto& j : subdirToCatEnum)
				  {
					  if (strstr(subpath, j.first))
					  {
						  auto c = categories.find(j.second);
						  if (c == categories.end())
						  {
							  c = categories.insert(std::make_pair(j.second, std::vector<MaterialId>())).first;
						  }

						  c->second.push_back(id);
						  break;
					  }
				  }
			  }
		  }
	  }

	  int32 CountMaterialsInCategory(Rococo::Graphics::MaterialCategory category) override
	  {
		  auto i = categories.find(category);
		  return i == categories.end() ? 0 : (int32) i->second.size();
	  }

	  MaterialId GetMaterialId(Rococo::Graphics::MaterialCategory category, int32 index) override
	  {
		  auto i = categories.find(category);
		  if (i == categories.end())
		  {
			  Throw(0, "Instances::GetMaterialId(...) -> No materials in category %d", category);
		  }

		  if (index < 0) Throw(0, "Instances::GetMaterialId(...) -> Index negative");

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
   
      void Clear() override
      {
          ecs.DeprecateAll();
          ecs.CollectGarbage();
      }

      Rococo::IECS& ECS() override
      {
          return ecs;
      }

      void Free() override
      {
         delete this;
      }
   };
}

namespace Rococo
{
   namespace Entities
   {
      IInstancesSupervisor* CreateInstanceBuilder(IMeshBuilderSupervisor& meshes, IRenderer& renderer, Events::IPublisher& publisher, IECS& ecs, size_t maxEntities)
      {
         return new Instances(meshes, renderer, publisher, ecs, maxEntities);
      }
   }
}