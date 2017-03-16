#include "hv.h"
#include <unordered_map>
#include <string>
#include <vector>

#include <rococo.strings.h>
#include <rococo.maths.h>

namespace
{
   using namespace HV;
   using namespace HV::Graphics;

   int64 nextId = 1;

   struct EntityImpl : public IEntity
   {
      Quat orientation{ {1.0f, 0.0f, 0.0f}, 0.0f };
      Vec3 position{ 0, 0, 0 };
      Vec3 scale{ 1.0f, 1.0f, 1.0f };
      mutable Matrix4x4 model;
      ID_SYS_MESH meshId;
      ID_TEXTURE textureId;
      ID_ENTITY parentId; 
      std::wstring name;
      std::vector<ID_ENTITY> children;
      mutable bool isDirty{ true };

      virtual const wchar_t* Name() const 
      {
         return name.c_str();
      }

      virtual const Vec3& Position() const
      {
         return position;
      }

      virtual const Matrix4x4& Model() const
      {
         if (isDirty)
         {
            isDirty = false;
            Matrix4x4 rotation;
            Matrix4x4::FromQuat(orientation, rotation);
            Matrix4x4 translation = Matrix4x4::Translate(position);
            model = translation * rotation;
         }
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
      std::unordered_map<std::wstring, ID_ENTITY> nameToEntityId;
      std::unordered_map<std::wstring, ID_TEXTURE> nameToTextureId;

      wchar_t name[Strings::MAX_FQ_NAME_LEN + 1]{ 0 };
      Matrix4x4 model;
      ID_SYS_MESH meshId;
      ID_TEXTURE textureId;
      IMeshBuilderSupervisor& meshBuilder;
      EntityImpl* parent{ nullptr };
      ID_ENTITY parentId;
      int32 enumerationDepth{ 0 };
      IRenderer& renderer;

      Instances(IMeshBuilderSupervisor& _meshBuilder, IRenderer& _renderer) :
         meshBuilder(_meshBuilder), renderer(_renderer) {}

      virtual void Begin(const fstring& fqName)
      {
         if (*name != 0)
         {
            Throw(0, L"Call InstanceBuilder.End() first");
         }

         Strings::ValidateFQNameIdentifier(fqName);
         SafeCopy(name, fqName, _TRUNCATE);
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
               Throw(0, L"Missing entity");
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
               Throw(0, L"Missing entity");
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
            Throw(0, L"SetOrientation - no such entity");
         }

         scale = i->second->scale;
      }

      virtual void GetPosition(ID_ENTITY entityId, Vec3& position)
      {
         auto i = idToEntity.find(entityId);
         if (i == idToEntity.end())
         {
            Throw(0, L"SetOrientation - no such entity");
         }

         position = i->second->position;
      }

      virtual void GetOrientation(ID_ENTITY entityId, Quat& orientation)
      {
         auto i = idToEntity.find(entityId);
         if (i == idToEntity.end())
         {
            Throw(0, L"SetOrientation - no such entity");
         }

         orientation = i->second->orientation;
      }

      virtual void SetOrientation(ID_ENTITY id, const Quat& orientation)
      {
         auto i = idToEntity.find(id);
         if (i == idToEntity.end())
         {
            Throw(0, L"SetOrientation - no such entity");
         }

         i->second->isDirty = true;
         i->second->orientation = orientation;
      }

      virtual void SetScale(ID_ENTITY id, const Vec3& scale)
      {
         auto i = idToEntity.find(id);
         if (i == idToEntity.end())
         {
            Throw(0, L"SetScale - no such entity");
         }


         i->second->isDirty = true;
         i->second->scale = scale;
      }

      virtual void SetPosition(ID_ENTITY id, const Vec3& position)
      {
         auto i = idToEntity.find(id);
         if (i == idToEntity.end())
         {
            Throw(0, L"SetPosition - no such entity");
         }

         i->second->isDirty = true;
         i->second->position = position;
      }
      
      virtual void SetMeshByName(const fstring& modelName, const fstring& textureFile)
      {
         if (*name == 0)
         {
            Throw(0, L"Call InstanceBuilder.Begin(...) first");
         }

         if (!meshBuilder.TryGetByName(modelName, meshId))
         {
            Throw(0, L"Mesh %s not found", modelName);
         }

         auto i = nameToTextureId.find(textureFile.buffer);
         if (i == nameToTextureId.end())
         {
            AutoFree<IExpandingBuffer> fileImage = CreateExpandingBuffer(0);
            renderer.Installation().LoadResource(textureFile, *fileImage, 64_megabytes);
            textureId = renderer.LoadTexture(*fileImage, textureFile);
            nameToTextureId[textureFile.buffer] = textureId;
         }
         else
         {
            textureId = i->second;
         }
      }

      virtual void SetParent(ID_ENTITY parentId)
      {
         if (*name == 0)
         {
            Throw(0, L"Call InstanceBuilder.Begin(...) first");
         }

         if (parentId != ID_ENTITY::Invalid())
         {
            this->parentId = parentId;
            auto i = idToEntity.find(parentId);
            if (i == idToEntity.end())
            {
               Throw(0, L"SetParent(...) Could not find parent");
            }
            else
            {
               parent = i->second;
            }
         }
      }

      virtual ID_ENTITY End()
      {
         if (enumerationDepth > 0)
         {
            Throw(0, L"Cannot create entity at this point. An enumeration is in progress");
         }

         if (*name == 0)
         {
            Throw(0, L"Call InstanceBuilder.Begin(...) first");
         }

         if (!meshId)
         {
            Throw(0, L"No valid mesh set for this instance");
         }

         if (!textureId)
         {
            Throw(0, L"No valid texture set for this instance");
         }

         auto i = nameToEntityId.find(name);
         if (i == nameToEntityId.end())
         {
            i = nameToEntityId.insert(std::make_pair(std::wstring(name), ID_ENTITY(nextId++))).first;
            EntityImpl* entity = new EntityImpl();
            entity->name = name;
            entity->model = model;
            entity->meshId = meshId;
            entity->parentId = parentId;
            entity->textureId = textureId;
            idToEntity.insert(std::make_pair(i->second, entity));
            if (parent != nullptr)
            {
               parent->children.push_back(i->second);
            }
            Clear();
            return i->second;
         }
         else
         {
            auto et = idToEntity.find(i->second);
            if (et == idToEntity.end())
            {
               Throw(0, L"Expecting entity for %s", name);
            }

            et->second->parentId = parentId;
            et->second->meshId = meshId;
            et->second->textureId = textureId;
            et->second->isDirty = false;

            if (parent != nullptr)
            {
               parent->children.push_back(i->second);
            }
            Clear();
            return i->second;
         }
      }

      virtual void Clear()
      {
         *name = 0;
         meshId = ID_SYS_MESH::Invalid();
         parent = nullptr;
         parentId = ID_ENTITY::Invalid();
         textureId = ID_TEXTURE::Invalid();
      }

      virtual void Free()
      {
         delete this;
      }
   };
}

namespace HV
{
   namespace Graphics
   {
      IInstancesSupervisor* CreateInstanceBuilder(IMeshBuilderSupervisor& builder, IRenderer& renderer)
      {
         return new Instances(builder, renderer);
      }
   }
}