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

   void RenderTest(IGuiRenderContext& grc)
   {
      GuiMetrics metrics;
      grc.Renderer().GetGuiMetrics(metrics);

      auto c = metrics.screenSpan;
      Rococo::Graphics::RenderCentredText(grc, L"Hello World!", RGBAb(255, 255, 255, 255), 0, { c.x >> 1,c.y >> 1 });
   }

   class Scene : public ISceneSupervisor, public HV::Graphics::ISceneBuilderSupervisor, public IObserver
   {
      HV::Entities::IInstancesSupervisor& instances;
      std::vector<ID_ENTITY> entities;
      std::vector<ObjectInstance> drawQueue;
      HV::Graphics::ICameraSupervisor& camera;

      IPublisher& publisher;

      RGBA clearColour{ 0,0,0,1 };
      Vec3 sun{ 0, 0, -1 };

      Vec2i spritePos{ 0, 0 };
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
         if (ev == HV::Events::Entities::OnTryMoveMobile)
         {
            auto& tmm = As<HV::Events::Entities::OnTryMoveMobileEvent>(ev);
            int32 dy = (int32) (50.0f * tmm.fowardDelta);
            int32 dx = (int32) (50.0f * tmm.straffeDelta);

            spritePos.x += dx;
            spritePos.y -= dy;
         }
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
         Textures::BitmapLocation location;
         if (grc.Renderer().SpriteBuilder().TryGetBitmapLocation(L"!textures/walls/metal1.jpg", location))
         {
            Rococo::Graphics::DrawSprite(Vec2i{ 0, 0 } - spritePos, location, grc, false);
         }
         else
         {
            Throw(0, L"Metal1 not loaded!");
         }

         if (grc.Renderer().SpriteBuilder().TryGetBitmapLocation(L"!textures/characters/player1.small.tif", location))
         {
            GuiMetrics metrics;
            grc.Renderer().GetGuiMetrics(metrics);

            auto span = Span(location.txUV);

            Vec2i delta{ span.x >> 1, span.y >> 1 };

            Vec2i centre{ metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1 };
            Rococo::Graphics::DrawSprite(centre - delta, location, grc, true);
         }
         else
         {
            Throw(0, L"!textures/characters/player1.small.tif not loaded!");
         }

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