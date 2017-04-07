#include "hv.h"
#include "hv.events.h"

#include <vector>
#include <algorithm>

#include <rococo.textures.h>

namespace
{
   using namespace Rococo;
   using namespace HV;
   using namespace HV::Graphics;

   class Scene : public ISceneSupervisor, public HV::Graphics::ISceneBuilderSupervisor, public IObserver
   {
      HV::Entities::IInstancesSupervisor& instances;
      std::vector<ID_ENTITY> entities;
      std::vector<ObjectInstance> drawQueue;
      HV::Graphics::ICameraSupervisor& camera;

      IPublisher& publisher;

      RGBA clearColour{ 0,0,0,1 };
      Vec3 sun{ 0, 0, -1 };

   public:
      Scene(HV::Entities::IInstancesSupervisor& _instances, HV::Graphics::ICameraSupervisor& _camera, IPublisher& _publisher) :
         instances(_instances), camera(_camera), publisher(_publisher)
      {
         publisher.Attach(this);
      }
      
      ~Scene()
      {
         publisher.Detach(this);
      }

      virtual void OnEvent(Event& ev)
      {
      }

      virtual void SetClearColour(float32 red, float32 green, float32 blue)
      {
         clearColour.red = red;
         clearColour.green = green;
         clearColour.blue = blue;
      }

      virtual void SetSunDirection(const Vec3& sun)
      {
         this->sun = sun;
         if (!TryNormalize(sun, this->sun))
         {
            this->sun = { 0,0,0 }; // night
         }
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
         Clear();
         struct ANON: public IEntityCallback
         {
            std::vector<ID_ENTITY>* entities;
            virtual void OnEntity(int64 index, IEntity& entity, ID_ENTITY id)
            {
               entities->push_back(id);
            }
         } addToScene;
         addToScene.entities = &entities;
         instances.ForAll(addToScene);

         drawQueue.clear();

         GlobalState state;
         state.sunlightDirection = HomogenizeNormal(sun);

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
               Throw(0, L"Unexpected missing entity");
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

namespace HV
{
   namespace Graphics
   {
      ISceneSupervisor* CreateScene(Entities::IInstancesSupervisor& instances, ICameraSupervisor& camera, IPublisher& publisher)
      {
         return new  Scene(instances, camera, publisher);
      }
   }
}