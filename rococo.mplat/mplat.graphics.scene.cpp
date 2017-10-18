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

	  GlobalState state;

      RGBA clearColour{ 0,0,0,1 };

   public:
      Scene(IInstancesSupervisor& _instances, ICameraSupervisor& _camera) :
         instances(_instances), camera(_camera)
      {
		  state.lights[0].lightDir = Vec4{ 1, 0, 0, 0 };
		  state.lights[0].lightPos = Vec4{ 1, 0, 0, 1 };
		  state.lights[1].lightDir = Vec4{ 1, 0, 0, 0 };
		  state.lights[1].lightPos = Vec4{ 0, 0, 0, 1 };
      }
      
      ~Scene()
      {
      }

      virtual void SetClearColour(float32 red, float32 green, float32 blue, float alpha)
      {
         clearColour.red = red;
         clearColour.green = green;
         clearColour.blue = blue;
		 clearColour.alpha = alpha;
      }

	  virtual void SetLight(const Vec3& dir, const Vec3& pos, int index)
	  {
		  if (index < 0 || index >= (sizeof(GlobalState::lights) / sizeof(GlobalLight)))
		  {
			  Throw(0, "Bad light index %d. Domain is [0,%d]", index, (sizeof(GlobalState::lights) / sizeof(GlobalLight)));
		  }

		  state.lights[index] = GlobalLight{ Vec4 { dir.x, dir.y, dir.z, 0}, Vec4 {pos.x, pos.y, pos.z, 1.0f }  };
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