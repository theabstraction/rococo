#include <rococo.mplat.h>

#include <vector>
#include <algorithm>

#include <rococo.textures.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Entities;
   using namespace Rococo::Graphics;

   class Scene : public ISceneSupervisor, public ISceneBuilderSupervisor
   {
      IInstancesSupervisor& instances;
      std::vector<ID_ENTITY> entities;
      std::vector<ObjectInstance> drawQueue;
      Rococo::Graphics::ICameraSupervisor& camera;

      enum { MAX_LIGHTS = 1 };

      struct Light
      {
         Vec3 dir;
         Vec3 pos;
      } lights[MAX_LIGHTS] =
      { 
         Vec3 { 1, 0, 0 }, Vec3{0, 0, 1}
      };

      RGBA clearColour{ 0,0,0,1 };

   public:
      Scene(IInstancesSupervisor& _instances, ICameraSupervisor& _camera) :
         instances(_instances), camera(_camera)
      {
      }
      
      ~Scene()
      {
      }

      virtual void SetClearColour(float32 red, float32 green, float32 blue)
      {
         clearColour.red = red;
         clearColour.green = green;
         clearColour.blue = blue;
      }

      virtual void SetLight(const Vec3& dir, const Vec3& pos, int index)
      {
         if (index < 0 || index >= MAX_LIGHTS)
         {
            Throw(0, "Bad light index %d. Domain is [0,%d]", index, MAX_LIGHTS);
         }

         lights[index] = Light{ dir, pos };
      }

      virtual void Free()
      {
         delete this;
      }

      virtual ISceneBuilderSupervisor& Builder()
      {
         return *this;
      }

      virtual void Clear()
      {
         entities.clear();
      }

      virtual void AddStatics(ID_ENTITY id)
      {
         entities.push_back(id);
      }

      virtual void RenderGui(IGuiRenderContext& grc)
      {
      }

      void FlushDrawQueue(ID_SYS_MESH meshId, ID_TEXTURE textureId, IRenderContext& rc)
      {
         if (drawQueue.empty()) return;
         rc.SetMeshTexture(textureId, 1);
         rc.Draw(meshId, &drawQueue[0], (uint32)drawQueue.size());
         drawQueue.clear();
      }

      virtual void RenderObjects(IRenderContext& rc)
      {
         drawQueue.clear();

         GlobalState state;
         state.lightDir = { lights[0].dir.x, lights[0].dir.y, lights[0].dir.z, 0.0f };
         state.lightPos = { lights[0].pos.x, lights[0].pos.y, lights[0].pos.z, 1.0f };

         camera.GetWorld(state.worldMatrix);
         camera.GetWorldAndProj(state.worldMatrixAndProj);

         rc.SetGlobalState(state);

         ID_SYS_MESH meshId;
         ID_TEXTURE textureId;

         for (auto i : entities)
         {
            IEntity* entity = instances.GetEntity(i);
            if (!entity)
            {
               Throw(0, "Unexpected missing entity");
            }

            if (!entity->TextureId())
            {
               continue;
            }

            if (entity->MeshId() != meshId || entity->TextureId() != textureId)
            {
               FlushDrawQueue(meshId, textureId, rc);
               meshId = entity->MeshId();
               textureId = entity->TextureId();
            }
           
            ObjectInstance instance{ entity->Model(), RGBA(0, 0, 0, 0) };
            drawQueue.push_back(instance);
         }

         FlushDrawQueue(meshId, textureId, rc);
      }

      virtual RGBA GetClearColour() const
      {
         return clearColour;
      }
   };
}

namespace Rococo
{
   namespace Graphics
   {
      ISceneSupervisor* CreateScene(IInstancesSupervisor& instances, ICameraSupervisor& camera)
      {
         return new  Scene(instances, camera);
      }
   }
}