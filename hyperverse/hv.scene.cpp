#include "hv.h"

#include <vector>

namespace
{
   using namespace Rococo;
   using namespace HV;
   using namespace HV::Graphics;

   void RenderTest(IGuiRenderContext& grc)
   {
      GuiMetrics metrics;
      grc.Renderer().GetGuiMetrics(metrics);

      auto c = metrics.screenSpan;
      Rococo::Graphics::RenderCentredText(grc, L"Hello World!", RGBAb(255, 255, 255, 255), 0, { c.x >> 1,c.y >> 1 });
   }

   class Scene : public ISceneSupervisor, public HV::Graphics::ISceneBuilderSupervisor
   {
      HV::Graphics::IInstancesSupervisor& instances;
      std::vector<ID_ENTITY> entities;
      std::vector<ObjectInstance> drawQueue;
      HV::Graphics::ICameraSupervisor& camera;
   public:
      Scene(HV::Graphics::IInstancesSupervisor& _instances, HV::Graphics::ICameraSupervisor& _camera) : 
         instances(_instances), camera(_camera)
      {

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

      virtual void AddAllStatics(const fstring& prefix)
      {
         struct ANON : public IEntityCallback
         {
            Scene* This;
            fstring prefix;

            virtual void OnEntity(int64 index, IEntity& entity, ID_ENTITY id)
            {
               if (wcsncmp(entity.Name(), prefix, prefix.length) == 0)
               {
                  This->AddStatics(id);
               }
            }
         } addByPrefix;

         addByPrefix.This = this;
         addByPrefix.prefix = prefix;

         instances.ForAll(addByPrefix);
      }

      virtual void RenderGui(IGuiRenderContext& grc)
      {
         RenderTest(grc);
      }

      void FlushDrawQueue(ID_SYS_MESH meshId, IRenderContext& rc)
      {
         if (drawQueue.empty()) return;
         rc.Draw(meshId, &drawQueue[0], (uint32)drawQueue.size());
         drawQueue.clear();
      }

      virtual void RenderObjects(IRenderContext& rc)
      {
         drawQueue.clear();

         GlobalState state;
         state.sunlightDirection = Vec4{ 0, 0, 1.0f, 0.0f };

         camera.GetWorld(state.worldMatrix);
         camera.GetWorldAndProj(state.worldMatrixAndProj);

         rc.SetGlobalState(state);

         ID_SYS_MESH meshId;

         for (auto i : entities)
         {
            IEntity* entity = instances.GetEntity(i);
            if (!entity)
            {
               Throw(0, L"Unexpected missing entity");
            }

            if (entity->MeshId() != meshId)
            {
               FlushDrawQueue(meshId, rc);
               meshId = entity->MeshId();
            }
            else
            {
               ObjectInstance instance{ entity->Model(), RGBA(0, 0, 0, 0) };
               drawQueue.push_back(instance);
            }
         }

         FlushDrawQueue(meshId, rc);
      }

      virtual RGBA GetClearColour() const
      {
         return RGBA(0.0f, 0.0f, 0.0f, 1.0f);
      }
   };
}

namespace HV
{
   namespace Graphics
   {
      ISceneSupervisor* CreateScene(IInstancesSupervisor& instances, ICameraSupervisor& camera)
      {
         return new  Scene(instances, camera);
      }
   }
}