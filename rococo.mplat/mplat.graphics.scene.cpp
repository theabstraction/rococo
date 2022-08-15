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
	  std::vector<ID_ENTITY> statics;
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

	  void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc, bool skinned) override
	  {
		  rc.Gui3D().Clear3DGuiTriangles();

		  if (populator)
		  {
			  populator->PopulateShadowCasters(*this, drd);
		  }

		  RenderEntities_InternalAnyPhase(rc, skinned);
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
		 statics.clear();
      }

      void AddStatics(ID_ENTITY id) override
      { 
		  if (!id)
		  {
			  Throw(0, "Scene.AddStatics: id was zero/invalid");
		  }
          entities.push_back(id);
		  statics.push_back(id);
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

	  void AddBoneMatrix(cr_m4x4 parentMatrix, IBone& bone, IRenderContext& rc, int& index)
	  {
		  if (index >= BoneMatrices::BONE_MATRIX_CAPACITY)
		  {
			  BonePath bp;
			  bone.GetFullName(bp);
			  Throw(0, "Too many bones in bone: %s", bp.text);
		  }

		  Matrix4x4 boneMatrix = parentMatrix * bone.GetMatrix();
		  rc.SetBoneMatrix(index++, boneMatrix);

		  for (auto* child : bone)
		  {
			  AddBoneMatrix(boneMatrix, *child, rc, index);
		  }
	  }

	  void RenderEntities_InternalStatic(IRenderContext& r)
	  {
		  ID_SYS_MESH meshId;

		  for (auto i : statics)
		  {
			  IEntity* entity = instances.GetEntity(i);
			  if (!entity)
			  {
				  Throw(0, "Scene: Unexpected missing entity with id #%lld", i.value);
			  }

			  if (entity->MeshId() != meshId)
			  {
				  FlushDrawQueue(meshId, r);
				  meshId = entity->MeshId();
			  }

			  ObjectInstance instance{ entity->Model(), RGBA(0, 0, 0, 0) };
			  drawQueue.push_back(instance);
		  }

		  if (meshId) FlushDrawQueue(meshId, r);
	  }

	  void RenderEntities_InternalDynamic(IRenderContext& r)
	  {
		  ID_SYS_MESH meshId;

		  for (auto i : debugEntities)
		  {
			  IEntity* entity = instances.GetEntity(i);
			  if (!entity)
			  {
				  Throw(0, "Scene: Unexpected missing entity with id #%lld", i.value);
			  }

			  AddDebugBones(*entity, r, *debugTesselator, rigs);
		  }

		  for (auto i : dynamics)
		  {
			  IEntity* entity = instances.GetEntity(i);
			  if (!entity)
			  {
				  Throw(0, "Scene: Unexpected missing entity with id #%lld", i.value);
			  }

			  auto* skeleton = entity->GetSkeleton(rigs.Skeles());

			  if (entity->MeshId() != meshId)
			  {
				  FlushDrawQueue(meshId, r);
				  meshId = entity->MeshId();
			  }

			  ObjectInstance instance{ entity->Model(), RGBA(0, 0, 0, 0) };
			  drawQueue.push_back(instance);

			  if (skeleton)
			  {
				  auto* root = skeleton->Root();
				  if (root)
				  {
					  int index = 0;
					  for (auto* child : *root)
					  {
						  AddBoneMatrix(root->GetMatrix(), *child, r, index);
					  }
				  }
				  FlushDrawQueue(meshId, r);
				  meshId = ID_SYS_MESH::Invalid();
			  }
		  }

		  FlushDrawQueue(meshId, r);
	  }

	  void RenderEntities_InternalAnyPhase(IRenderContext& r, bool skinned)
	  {
		  drawQueue.clear();

		  if (!skinned)
		  {
			  RenderEntities_InternalStatic(r);
		  }
		  else
		  {
			  RenderEntities_InternalDynamic(r);
		  }
	  }

      void RenderObjects(IRenderContext& rc, bool skinned) override
      {
		  debugTesselator->Clear();

		  if (populator)
		  {
			  populator->PopulateScene(*this);
		  }

		  RenderEntities_InternalAnyPhase(rc, skinned);
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