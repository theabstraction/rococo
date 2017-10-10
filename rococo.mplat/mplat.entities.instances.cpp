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
      ID_TEXTURE textureId;
      Vec3 scale{ 1.0f, 1.0f, 1.0f };       
      std::vector<ID_ENTITY> children;

      virtual const Vec3 Position() const
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

      virtual ID_SYS_MESH MeshId() const { return meshId; }
      virtual ID_TEXTURE TextureId() const { return textureId;  }
   };

   typedef std::unordered_map<ID_ENTITY, EntityImpl*, ID_ENTITY> MapIdToEntity;

   struct Instances : public IInstancesSupervisor
   {      
      MapIdToEntity idToEntity;
      std::unordered_map<std::string, ID_TEXTURE> nameToTextureId;
      Rococo::Graphics::IMeshBuilderSupervisor& meshBuilder;
      IRenderer& renderer;

      int32 enumerationDepth{ 0 };

      Instances(Rococo::Graphics::IMeshBuilderSupervisor& _meshBuilder, IRenderer& _renderer) :
         meshBuilder(_meshBuilder), renderer(_renderer)
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

      ID_ENTITY Add(ID_SYS_MESH meshId, ID_TEXTURE textureId, const Matrix4x4& model, const Vec3& scale, ID_ENTITY parentId)
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
         e->textureId = textureId;
         e->scale = scale;

         idToEntity.insert(std::make_pair(id, e));

         return id;
      }

      virtual ID_ENTITY AddBody(const fstring& modelName, const fstring& texture, const Matrix4x4& model, const Vec3& scale, ID_ENTITY parentId)
      {
         ID_SYS_MESH meshId;
         if (!meshBuilder.TryGetByName(modelName, meshId))
         {
            Throw(0, "Cannot find model: %s", modelName.buffer);
         }

         ID_TEXTURE textureId = ReadyTexture(texture);

         return Add(meshId, textureId, model, scale, parentId);
      }

      virtual ID_ENTITY AddGhost(const Matrix4x4& model, const Vec3& scale, ID_ENTITY parentId)
      {
         return Add(ID_SYS_MESH::Invalid(), ID_TEXTURE::Invalid(), model, scale, parentId);
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

      ID_TEXTURE ReadyTexture(cstr pingName) override
      {
         ID_TEXTURE textureId;
         auto i = nameToTextureId.find(pingName);
         if (i == nameToTextureId.end())
         {
            AutoFree<IExpandingBuffer> fileImage = CreateExpandingBuffer(0);
            renderer.Installation().LoadResource(pingName, *fileImage, 64_megabytes);
            textureId = renderer.LoadTexture(*fileImage, pingName);
            nameToTextureId[pingName] = textureId;
         }
         else
         {
            textureId = i->second;
         }

         return textureId;
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

      virtual void SetScale(ID_ENTITY id, const Vec3& scale)
      {
         auto i = idToEntity.find(id);
         if (i == idToEntity.end())
         {
            Throw(0, "SetScale - no such entity");
         }

         i->second->scale = scale;
      }

      virtual void SetTexture(ID_ENTITY id, const fstring& texture)
      {
         auto i = idToEntity.find(id);
         if (i == idToEntity.end())
         {
            Throw(0, "SetScale - no such entity");
         }

         i->second->textureId = ReadyTexture(texture);
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
      IInstancesSupervisor* CreateInstanceBuilder(IMeshBuilderSupervisor& meshes, IRenderer& renderer)
      {
         return new Instances(meshes, renderer);
      }
   }
}