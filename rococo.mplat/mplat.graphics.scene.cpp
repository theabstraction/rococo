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

	  Light lights[2] = { 0 };

	  IScenePopulator* populator = nullptr;
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

	  const Light* GetLights(size_t& nCount) const override
	  {
		  nCount = 2;
		  return lights;
	  }

	  void SetPopulator(IScenePopulator* populator) override
	  {
		  this->populator = populator;
	  }

	  void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc) override
	  {
		  if (populator)
		  {
			  populator->PopulateShadowCasters(*this, drd);
		  }

		  drawQueue.clear();

		  ID_SYS_MESH meshId;

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

			  if (entity->MeshId() != meshId)
			  {
				  FlushDrawQueue_NoTexture(meshId, rc);
				  meshId = entity->MeshId();
			  }

			  ObjectInstance instance{ entity->Model(), RGBA(0, 0, 0, 0) };
			  drawQueue.push_back(instance);
		  }

		  FlushDrawQueue_NoTexture(meshId, rc);
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
		  if (index < 0 || index >= 2)
		  {
			  // Not enough lights, and so since index represents priority, it is silently dropped
			  return;
		  }

		  state.lights[index] = GlobalLight{ Vec4 { dir.x, dir.y, dir.z, 0}, Vec4 {pos.x, pos.y, pos.z, 1.0f }  };

		  lights[index].colour = RGBAb(255, 255, 255);
		  lights[index].direction = dir;
		  lights[index].fov = 90_degrees;
		  lights[index].intensity = 1.0f;
		  lights[index].position = pos;
		  lights[index].nearPlane = 0.01_metres;
		  lights[index].farPlane = 25_metres;
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

	  void FlushDrawQueue_NoTexture(ID_SYS_MESH meshId , IRenderContext& rc)
	  {
		  if (drawQueue.empty()) return;
		  rc.Draw(meshId, &drawQueue[0], (uint32)drawQueue.size());
		  drawQueue.clear();
	  }

      virtual void RenderObjects(IRenderContext& rc)
      {
		  if (populator)
		  {
			  populator->PopulateScene(*this);
		  }

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