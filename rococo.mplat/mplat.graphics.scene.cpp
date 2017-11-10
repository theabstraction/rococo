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

	  enum { MAX_LIGHTS = 8 };
	  Light lights[MAX_LIGHTS] = { 0 };

	  IScenePopulator* populator = nullptr;
   public:
      Scene(IInstancesSupervisor& _instances, ICameraSupervisor& _camera) :
         instances(_instances), camera(_camera)
      {
      }
      
      ~Scene()
      {
      }

	  const Light* GetLights(size_t& nCount) const override
	  {
		  nCount = MAX_LIGHTS;
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

	  void ClearLights() override
	  {
		  memset(lights, 0, sizeof(lights));
	  }

	  virtual void SetLight(const LightSpec& spec,  int index)
	  {
		  if (index < 0 || index >= MAX_LIGHTS)
		  {
			  // Not enough lights, and so since index represents priority, it is silently dropped
			  return;
		  }

		  lights[index] = { 0 };
		  lights[index].colour = spec.diffuse;
		  lights[index].direction = Vec4::FromVec3(spec.direction,0.0f);
		  lights[index].fov = spec.fov;
		  lights[index].cosHalfFov = cosf(lights[index].fov * 0.5f);
		  lights[index].position = Vec4::FromVec3(spec.position, 1.0f);
		  lights[index].nearPlane = spec.nearPlane;
		  lights[index].farPlane = spec.farPlane;
		  lights[index].attenuationRate = spec.attenuation;
		  lights[index].cutoffPower = spec.cutoffPower;
		  lights[index].cutoffCosAngle = Cos(spec.cutoffAngle);
		  lights[index].ambient = spec.ambience;
		  lights[index].fogConstant = spec.fogConstant;
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

      void FlushDrawQueue(ID_SYS_MESH meshId, IRenderContext& rc)
      {
         if (drawQueue.empty()) return;
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

         for (auto i : entities)
         {
            IEntity* entity = instances.GetEntity(i);
            if (!entity)
            {
               Throw(0, "Unexpected missing entity");
            }

            if (entity->MeshId() != meshId)
            {
               FlushDrawQueue(meshId, rc);
               meshId = entity->MeshId();
            }
           
            ObjectInstance instance{ entity->Model(), RGBA(0, 0, 0, 0) };
            drawQueue.push_back(instance);
         }

         FlushDrawQueue(meshId, rc);
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