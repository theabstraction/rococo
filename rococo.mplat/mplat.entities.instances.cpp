#include <rococo.mplat.h>
#include <vector>
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.animation.h>
#include <rococo.handles.h>
#include <rococo.hashtable.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Graphics;
   using namespace Rococo::Entities;

   struct EntityImpl : public IEntityDeprecated
   {
      Matrix4x4 model = Matrix4x4::Identity();
      ID_ENTITY parentId; 
      ID_SYS_MESH meshId;
      Vec3 scale{ 1.0f, 1.0f, 1.0f };   
      HString skeletonName;
      ID_SKELETON idSkeleton;

      EntityImpl() {}

      EntityImpl(const EntityImpl& src):
          model(src.model),
          parentId(src.parentId),
          meshId(src.meshId),
          scale(src.scale),
          skeletonName(src.skeletonName),
          idSkeleton(src.idSkeleton)
      {

      }

      AutoFree<IAnimation> animation;

      // Lazy init the animation object
      IAnimation& LazyInitAndGetAnimation()
      {
          if (!animation)
          {
              animation = CreateAnimation();
          }

          return *animation;
      }

      IAnimation* GetAnimation() override
      {
          return animation;
      }

      Vec3 Position() const override
      {
         return model.GetPosition();
      }

      Matrix4x4& Model() override
      {       
         return model;
      }

      ID_SYS_MESH MeshId() const  override
	  {
		  return meshId;
	  }

	  void SetMesh(ID_SYS_MESH id) override
	  {
		  meshId = id;
	  }

      ISkeleton* GetSkeleton(ISkeletons& skeletons) override
      {
          if (skeletonName.length() == 0)
          {
              return nullptr;
          }

          ISkeleton* skeleton = nullptr;

          if (skeletons.TryGet(idSkeleton, &skeleton))
          {
              return skeleton;
          }

          idSkeleton = skeletons.TryGet(skeletonName, &skeleton);
          return skeleton;
      }
   };

   enum { ENTITY_SALT_BITCOUNT = 8 };
   typedef HandleTable<EntityImpl, ENTITY_SALT_BITCOUNT> MapIdToEntity;
   typedef THandle<ENTITY_SALT_BITCOUNT> H_ENTITY;

   struct Instances : public IInstancesSupervisor
   {      
      MapIdToEntity idToEntity;
      IMeshBuilderSupervisor& meshBuilder;
      IRenderer& renderer;
	  Events::IPublisher& publisher;

      int32 enumerationDepth{ 0 };

      Instances(IMeshBuilderSupervisor& _meshBuilder, IRenderer& _renderer, Events::IPublisher& _publisher, size_t maxEntities) :
          meshBuilder(_meshBuilder), renderer(_renderer), publisher(_publisher), idToEntity("Instances-idToEntity", maxEntities)
      {
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

          H_ENTITY hEntity = idToEntity.CreateNew();

          EntityImpl* e;
          if (!idToEntity.TryGetRef(hEntity, &e))
          {
              Throw(0, "%s: !idToEntity.TryGetRef returned false!", __FUNCTION__);
          }

          e->model = model;
          e->parentId = parentId;
          e->meshId = meshId;

          e->scale = (scale.x != 0) ? scale : Vec3{ 1.0f, 1.0f, 1.0f };

          return ID_ENTITY{ hEntity.Value() };
      }

      ID_ENTITY AddSkeleton(const fstring& skeleton, const Matrix4x4& model) override
      {
            float d = Determinant(model);
            if (d < 0.975f || d > 1.025f)
            {
                Throw(0, "Bad model matrix. Determinant was %f", d);
            }

            H_ENTITY hEntity = idToEntity.CreateNew();

            EntityImpl* e;
            if (!idToEntity.TryGetRef(hEntity, &e))
            {
                Throw(0, "%s: !idToEntity.TryGetRef returned false!", __FUNCTION__);
            }

            e->model = model;
            e->parentId = ID_ENTITY{ 0 };
            e->meshId = ID_SYS_MESH::Invalid();
            e->skeletonName = skeleton;
            e->scale = Vec3{ 1.0f, 1.0f, 1.0f };

            return ID_ENTITY{ hEntity.Value() };
      }

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

      ID_ENTITY AddGhost(const Matrix4x4& model, ID_ENTITY parentId) override
      {
		  return Add(ID_SYS_MESH::Invalid(), model, { 1,1,1 }, parentId);
      }

      void BindSkeletonToBody(const fstring& skeleton, ID_ENTITY idBody) override
      {
          if (skeleton.length < 1)
          {
              Throw(0, "%s: skeleton name was blank", __FUNCTION__);
          }

          H_ENTITY hBody{ idBody.Value()};

          EntityImpl* body;
          if (!idToEntity.TryGetRef(hBody, &body))
          {
              Throw(0, "%s: !idToEntity.TryGetRef returned false! Failed to find body with id %llu", __FUNCTION__, idBody.Value());
          }

          body->skeletonName = skeleton;
      }

      void Delete(ID_ENTITY id) override
      {
          idToEntity.Destroy(H_ENTITY(id.Value()), EntityImpl());
      }

      boolean32 TryGetModelToWorldMatrix(ID_ENTITY id, Matrix4x4& model) override
      {
          EntityImpl* e;
          if (idToEntity.TryGetRef(H_ENTITY(id.Value()), &e))
          {
              model = e->model;
              return true;
          }
          else
          {
            model = Matrix4x4::Identity();
            return false;
          }
      }

      IEntityDeprecated* GetEntity(ID_ENTITY id) override
      {
          EntityImpl* e;
          if (idToEntity.TryGetRef(H_ENTITY(id.Value()), &e))
          {
              return static_cast<IEntityDeprecated*>(e);
          }
          else
          {
              return nullptr;
          }
      }

      std::vector<const Matrix4x4*> modelStack;

      void ConcatenatePositionVectors(ID_ENTITY leafId, Vec3& position) override
      {
        position = Vec3{ 0,0,0 };

        auto* entity = GetEntity(leafId);
        if (entity == nullptr)
        {
            Throw(0, "Missing entity");
        }

        position += entity->Position();
      }

	  ID_CUBE_TEXTURE CreateCubeTexture(const fstring& folder, const fstring& extension) override
	  {
		  return renderer.Textures().CubeTextures().CreateCubeTexture(folder, extension);
	  }

      void ConcatenateModelMatrices(ID_ENTITY leafId, Matrix4x4& m) override
      {
        auto* entity = GetEntity(leafId);
        if (entity == nullptr)
        {
            Throw(0, "Missing entity");
        }

        m = entity->Model();

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

         H_ENTITY id = idToEntity.GetFirstHandle();

         while(id)
         {
             EntityImpl* e;
             if (idToEntity.TryGetRef(id, &e))
             {
                 cb.OnEntity(count++, *static_cast<IEntityDeprecated*>(e), ID_ENTITY(id.Value()));
             }
             id = idToEntity.GetNextHandle(id);
         }
      }

      void GetScale(ID_ENTITY id, Vec3& scale)
      {
          EntityImpl* e;
          if (idToEntity.TryGetRef(H_ENTITY(id.Value()), &e))
          {
              scale = e->scale;
          }
          else
          {
              scale = { 0,0,0 };
          }
      }

      void GetPosition(ID_ENTITY id, Vec3& position) 
      {
          EntityImpl* e;
          if (idToEntity.TryGetRef(H_ENTITY(id.Value()), &e))
          {
              position = e->model.GetPosition();
          }
          else
          {
              position = { 0,0,0 };
          }
      }

      void AddAnimationFrame(ID_ENTITY id, const fstring& frameName, Seconds duration, boolean32 loop) override
      {
          EntityImpl* e;
          if (!idToEntity.TryGetRef(H_ENTITY(id.Value()), &e))
          {
              Throw(0, "%s no such entity with id %llu", __FUNCTION__, id.Value());
          }

          e->LazyInitAndGetAnimation().AddKeyFrame(frameName, duration, loop);
      }

	  void LoadMaterialArray(const fstring& folder, int32 txWidth) override
	  {
		  struct: public IEventCallback<IO::FileItemData>, IMaterialTextureArrayBuilder
		  {
			  int32 txWidth;
			  std::vector<HString> filenames;
			  IInstallation* installation;
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

      void SetScale(ID_ENTITY id, const Vec3& scale) override
      {
          EntityImpl* e;
          if (idToEntity.TryGetRef(H_ENTITY(id.Value()), &e))
          {
              e->scale = scale;
          }
      }
   
      void Clear() override
      {
          idToEntity.Clear(EntityImpl());
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
      IInstancesSupervisor* CreateInstanceBuilder(IMeshBuilderSupervisor& meshes, IRenderer& renderer, Events::IPublisher& publisher, size_t maxEntities)
      {
         return new Instances(meshes, renderer, publisher, maxEntities);
      }
   }
}