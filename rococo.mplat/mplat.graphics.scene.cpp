#include <rococo.mplat.h>
#include <rococo.animation.h>

#include <vector>
#include <algorithm>

#include <rococo.textures.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Entities;
   using namespace Rococo::Graphics;

   typedef std::vector<VertexTriangle> TTriangles;

   class Scene : public ISceneSupervisor, public ISceneBuilderSupervisor
   {
      IInstancesSupervisor& instances;
      std::vector<ID_ENTITY> entities;
	  std::vector<ID_ENTITY> debugEntities;
	  std::vector<ID_ENTITY> dynamics;
      std::vector<ObjectInstance> drawQueue;
      Rococo::Graphics::ICameraSupervisor& camera;

	  GlobalState state;

      RGBA clearColour{ 0,0,0,1 };

	  enum { MAX_LIGHTS = 1 };
	  Light lights[MAX_LIGHTS] = { 0 };

	  IScenePopulator* populator = nullptr;

	  ID_CUBE_TEXTURE skyboxId;

	  IRigs& rigs;

	  AutoFree<IRodTesselatorSupervisor> debugTesselator;
   public:
      Scene(IInstancesSupervisor& _instances, ICameraSupervisor& _camera, IRigs& _rigs) :
         instances(_instances), camera(_camera), rigs(_rigs),
		 debugTesselator(CreateIsolatedRodTesselator())
      {
		  debugTesselator->SetUVScale(1.0f);
      }
      
      ~Scene()
      {
      }

	  void SetSkyBox(ID_CUBE_TEXTURE cubeId) override
	  {
		  this->skyboxId = cubeId;
	  }

	  void GetCamera(Matrix4x4& worldToScreen, Matrix4x4& world, Matrix4x4& proj, Vec4& eye, Vec4& viewDir) override
	  {
		  camera.GetWorld(world);
		  camera.GetWorldAndProj(worldToScreen);
		  camera.GetPosition(eye);
		  camera.GetProjection(proj);
		  eye.w = 1.0f;
		  viewDir = Vec4::FromVec3(world.GetForwardDirection(), 0);
	  };

	  ID_CUBE_TEXTURE GetSkyboxCubeId() const override
	  {
		  return skyboxId;
	  }

	  const Light* GetLights(uint32& nCount) const override
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
		  rc.Clear3DGuiTriangles();

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

      void SetClearColour(float32 red, float32 green, float32 blue, float alpha) override
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

	  void SetLight(const LightSpec& spec,  int index) override
	  {
		  if (index < 0 || index >= MAX_LIGHTS)
		  {
			  // Not enough lights, and so since index represents priority, request is silently ignored
			  return;
		  }

		  lights[index] = { 0 };
		  lights[index].colour = spec.diffuse;
		  lights[index].direction = Vec4::FromVec3(Normalize(spec.direction),0.0f);
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

      void Free() override
      {
         delete this;
      }

      ISceneBuilderSupervisor& Builder() override
      {
         return *this;
      }

	  void AdvanceAnimations(Seconds dt) override
	  {
		  auto& poses = rigs.Poses();
		  auto& skeles = rigs.Skeles();

		  for (auto id : dynamics)
		  {
			  auto e = instances.GetEntity(id);
			  if (e)
			  {
				  auto a = e->GetAnimation();
				  if (a)
				  {
					  auto* skele = e->GetSkeleton(skeles);
					  if (skele)
					  {
						  AnimationAdvanceArgs args
						  {
							   *skele,
							   poses,
							   dt
						  };
						  a->Advance(args);
					  }
				  }
			  }
		  }
	  }

      void Clear() override
      {
         entities.clear();
		 debugEntities.clear();
		 dynamics.clear();
      }

      void AddStatics(ID_ENTITY id) override
      { 
		  if (!id)
		  {
			  Throw(0, "Scene.AddStatics: id was zero/invalid");
		  }
          entities.push_back(id);
      }

	  void AddDebugObject(ID_ENTITY id) override
	  {
		  if (!id)
		  {
			  Throw(0, "Scene.AddDebugMeshes: id was zero/invalid");
		  }
		  debugEntities.push_back(id);
	  }

	  void AddDynamicObject(ID_ENTITY id) override
	  {
		  if (!id)
		  {
			  Throw(0, "Scene.AddDynamicObject: id was zero/invalid");
		  }
		  entities.push_back(id);
		  dynamics.push_back(id);
	  }

	  void OnGuiResize(Vec2i span) override
	  {

	  }

      void RenderGui(IGuiRenderContext& grc) override
      {
      }

      void FlushDrawQueue(ID_SYS_MESH meshId, IRenderContext& rc)
      {
         if (drawQueue.empty()) return;
         rc.Draw(meshId, &drawQueue[0], (uint32)drawQueue.size());
         drawQueue.clear();
      }

	  void FlushDrawQueue_NoTexture(ID_SYS_MESH meshId, IRenderContext& rc)
	  {
		  if (drawQueue.empty()) return;
		  rc.Draw(meshId, &drawQueue[0], (uint32)drawQueue.size());
		  drawQueue.clear();
	  }

      void RenderObjects(IRenderContext& rc) override
      {
		  debugTesselator->Clear();

		  if (populator)
		  {
			  populator->PopulateScene(*this);
		  }

         drawQueue.clear();

         ID_SYS_MESH meshId;

		 for (auto i : debugEntities)
		 {
			 IEntity* entity = instances.GetEntity(i);
			 if (!entity)
			 {
				 Throw(0, "Scene: Unexpected missing entity with id #%lld", i.value);
			 }

			 AddDebugBones(*entity, rc, *debugTesselator, rigs);
		 }

         for (auto i : entities)
         {
            IEntity* entity = instances.GetEntity(i);
            if (!entity)
            {
               Throw(0, "Scene: Unexpected missing entity with id #%lld", i.value);
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

      RGBA GetClearColour() const override
      {
         return clearColour;
      }
   };
}

namespace Rococo
{
   namespace Graphics
   {
      ISceneSupervisor* CreateScene(IInstancesSupervisor& instances, ICameraSupervisor& camera, IRigs& rigs)
      {
         return new Scene(instances, camera, rigs);
      }
   }
}