#include <rococo.mplat.h>
#include <unordered_map>
#include <string>
#include <vector>

#include <rococo.strings.h>
#include <rococo.maths.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Graphics;
   using namespace Rococo::Entities;

   int64 nextId = 1;

   struct EntityImpl : public IEntity
   {
      Matrix4x4 model;
      ID_ENTITY parentId; 
      ID_SYS_MESH meshId;
      Vec3 scale{ 1.0f, 1.0f, 1.0f };       
      std::vector<ID_ENTITY> children;

      virtual Vec3 Position() const
      {
         return model.GetPosition();
      }

      virtual Matrix4x4& Model()
      {       
         return model;
      }

      virtual ID_ENTITY ParentId() const
      {
         return parentId;
      }

      virtual const ID_ENTITY* begin() const 
      { 
         return children.empty() ? ((ID_ENTITY*) nullptr) : &children[0]; 
      }
      
      virtual const ID_ENTITY* end() const
      {
         return children.empty() ? ((ID_ENTITY*) nullptr) : &children[children.size()];
      }

      virtual ID_SYS_MESH MeshId() const 
	  {
		  return meshId;
	  }

	  virtual void SetMesh(ID_SYS_MESH id)
	  {
		  meshId = id;
	  }

   };

   typedef std::unordered_map<ID_ENTITY, EntityImpl*, ID_ENTITY> MapIdToEntity;

   struct Instances : public IInstancesSupervisor
   {      
      MapIdToEntity idToEntity;
      Rococo::Graphics::IMeshBuilderSupervisor& meshBuilder;
      IRenderer& renderer;
	  Events::IPublisher& publisher;

      int32 enumerationDepth{ 0 };

      Instances(Rococo::Graphics::IMeshBuilderSupervisor& _meshBuilder, IRenderer& _renderer, Events::IPublisher& _publisher) :
         meshBuilder(_meshBuilder), renderer(_renderer), publisher(_publisher)
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
         float d = Determinant(model);
         if (d < 0.975f || d > 1.025f)
         {
            Throw(0, "Bad model matrix. Determinant was %f", d);
         }
         
         ID_ENTITY id(nextId++);

         if (parentId != 0)
         {
            auto i = idToEntity.find(parentId);
            if (i == idToEntity.end()) Throw(0, "Cannot find parent entity with id #%d", parentId.value);
            i->second->children.push_back(id);
         }

         auto* e = new EntityImpl;
         e->model = model;
         e->parentId = parentId;
         e->meshId = meshId;
         e->scale = scale;

         idToEntity.insert(std::make_pair(id, e));

         return id;
      }

      virtual ID_ENTITY AddBody(const fstring& modelName, const Matrix4x4& model, const Vec3& scale, ID_ENTITY parentId)
      {
         ID_SYS_MESH meshId;
         if (!meshBuilder.TryGetByName(modelName, meshId))
         {
            Throw(0, "Cannot find model: %s", modelName.buffer);
         }

         return Add(meshId, model, scale, parentId);
      }

      virtual ID_ENTITY AddGhost(const Matrix4x4& model, const Vec3& scale, ID_ENTITY parentId)
      {
         return Add(ID_SYS_MESH::Invalid(), model, scale, parentId);
      }

      virtual void Delete(ID_ENTITY id)
      {
         auto i = idToEntity.find(id);
         if (i != idToEntity.end())
         {
            delete i->second;
            idToEntity.erase(i);
         }
      }

      virtual boolean32 TryGetModelToWorldMatrix(ID_ENTITY entityId, Matrix4x4& model)
      {
         auto i = idToEntity.find(entityId);
         if (i == idToEntity.end())
         {
            model = Matrix4x4::Identity();
            return false;
         }
         else
         {
            model = i->second->model;
            return true;
         }
      }

      virtual IEntity* GetEntity(ID_ENTITY id)
      {
         auto i = idToEntity.find(id);
         return i == idToEntity.end() ? nullptr : i->second;
      }

      std::vector<const Matrix4x4*> modelStack;

      virtual void ConcatenatePositionVectors(ID_ENTITY leafId, Vec3& position)
      {
         position = Vec3{ 0,0,0 };

         ID_ENTITY i = leafId;
         while (i != ID_ENTITY::Invalid())
         {
            auto* entity = GetEntity(i);
            if (entity == nullptr)
            {
               Throw(0, "Missing entity");
            }

            position += entity->Position();

            i = entity->ParentId();
         }
      }

      virtual void ConcatenateModelMatrices(ID_ENTITY leafId, Matrix4x4& m)
      {
         modelStack.clear();

         ID_ENTITY i = leafId;
         while(i != ID_ENTITY::Invalid())
         {
            auto* entity = GetEntity(i);
            if (entity == nullptr)
            {
               Throw(0, "Missing entity");
            }

            modelStack.push_back(&entity->Model());

            i = entity->ParentId();
         }

         m = Matrix4x4::Identity();
         for (auto j = modelStack.rbegin(); j != modelStack.rend(); ++j)
         {
            Matrix4x4 mPrimed = m * **j;
            m = mPrimed;
         }

         float Dm = Determinant(m);
         if (Dm < 0.9f || Dm > 1.1f)
         {
            Throw(0, "Bad model matrix for entity %lld. Det M = %f", leafId.value, Dm);
         }
      }

      void ForAll(IEntityCallback& cb)
      {
         RecursionGuard guard(enumerationDepth);

         int64 count = 0;
         for (auto& i : idToEntity)
         {
            cb.OnEntity(count++, *i.second, i.first);
         }
      }

      virtual void GetScale(ID_ENTITY entityId, Vec3& scale)
      {
         auto i = idToEntity.find(entityId);
         if (i == idToEntity.end())
         {
            Throw(0, "SetOrientation - no such entity");
         }

         scale = i->second->scale;
      }

      virtual void GetPosition(ID_ENTITY entityId, Vec3& position)
      {
         auto i = idToEntity.find(entityId);
         if (i == idToEntity.end())
         {
            Throw(0, "GetPosition - no such entity");
         }

         position = i->second->model.GetPosition();
      }

	  virtual void LoadMaterialArray(const fstring& folder, int32 txWidth)
	  {
		  struct: public IEventCallback<cstr>, IMaterialTextureArrayBuilder
		  {
			  int32 txWidth;
			  std::vector<std::string> filenames;
			  IInstallation* installation;
			  AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(4_megabytes);
			  Events::IPublisher* publisher;
			  Instances* This;

			  char absPath[IO::MAX_PATHLEN];

			  void OnEvent(cstr name) override
			  {
				  auto ext = GetFileExtension(name);
				  if (Eq(ext, ".jpeg") || Eq(ext, ".jpg") || Eq(ext, "tif") || Eq(ext, "tiff"))
				  {
					  char absName[IO::MAX_PATHLEN];
					  SecureFormat(absName, IO::MAX_PATHLEN, "%s%s", absPath, name);
					  filenames.push_back(absName);
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
				  be.resourceName = path;
				  publisher->Publish(be);

				  installation->LoadResource(path, *buffer, 64_megabytes);
				  MaterialTextureArrayBuilderArgs args{ *buffer, path };
				  onLoad.OnEvent(args);
			  }
		  } z;

		  z.txWidth = txWidth;
		  z.installation = &renderer.Installation();
		  z.publisher = &publisher;
		  z.This = this;

		  z.installation->ConvertPingPathToSysPath(folder, z.absPath, IO::MAX_PATHLEN);
	
		  IO::ForEachFileInDirectory(z.absPath, z);

		  if (z.filenames.empty()) return;

		  Events::BusyEvent be;
		  be.isNowBusy = true;
		  be.message = "Loading textures";
		  be.resourceName = "";
		  publisher.Publish(be);

		  renderer.LoadMaterialTextureArray(z);

		  be.isNowBusy = false;
		  be.message = "";
		  be.resourceName = "";
		  publisher.Publish(be);

		  RefreshCategories();
	  }

	  std::unordered_map<MaterialCategory, std::vector<MaterialId>> categories;

	  std::unordered_map<std::string, MaterialCategory> subdirToCatEnum =
	  {
		  { "/wood/", MaterialCategory_Wood },
		  { "/stone/", MaterialCategory_Stone },
		  { "/rock/", MaterialCategory_Rock },
		  { "/metal/", MaterialCategory_Metal },
		  { "/marble/", MaterialCategory_Marble },
	  };

	  void RefreshCategories()
	  {
		  categories.clear();

		  MaterialArrayMetrics metrics;
		  renderer.GetMaterialArrayMetrics(metrics);

		  auto content = renderer.Installation().Content();

		  for (size_t i = 0; i < metrics.NumberOfElements; ++i)
		  {
			  auto id = (MaterialId)i;
			  cstr name = renderer.GetMaterialTextureName(id);
			  if (StartsWith(name, content))
			  {
				  char fullname[IO::MAX_PATHLEN];
				  SafeFormat(fullname, IO::MAX_PATHLEN, "%s", name);
				  OS::ToUnixPath(fullname);
				  cstr subpath = fullname + content.length;

				  for (auto& j : subdirToCatEnum)
				  {
					  if (strstr(subpath, j.first.c_str()))
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
		  char sysPath[IO::MAX_PATHLEN];
		  renderer.Installation().ConvertPingPathToSysPath(pingPath, sysPath, IO::MAX_PATHLEN);
		  return renderer.GetMaterialId(sysPath);
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
         auto i = idToEntity.find(id);
         if (i == idToEntity.end())
         {
            Throw(0, "SetScale - no such entity");
         }

         i->second->scale = scale;
      }
   
      virtual void Clear()
      {
         for (auto& i : idToEntity)
         {
            delete i.second;
         }

         idToEntity.clear();
      }

      virtual void Free()
      {
         delete this;
      }
   };
}

namespace Rococo
{
   namespace Entities
   {
      IInstancesSupervisor* CreateInstanceBuilder(IMeshBuilderSupervisor& meshes, IRenderer& renderer, Events::IPublisher& publisher)
      {
         return new Instances(meshes, renderer, publisher);
      }
   }
}