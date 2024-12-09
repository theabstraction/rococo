#include <rococo.mplat.h>
#include <rococo.animation.h>
#include <components/rococo.components.animation.h>
#include <components/rococo.components.skeleton.h>
#include <vector>
#include <algorithm>

#include <rococo.textures.h>
#include "mplat.components.h"

namespace
{
   using namespace Rococo;
   using namespace Rococo::Entities;
   using namespace Rococo::Graphics;
   using namespace Rococo::Components;

   typedef std::vector<VertexTriangle> TTriangles;

   class Scene : public ISceneSupervisor, public ISceneBuilderSupervisor
   {
	  IECS& ecs;
      std::vector<ID_ENTITY> entities;
	  std::vector<ID_ENTITY> debugEntities;
	  std::vector<ID_ENTITY> dynamics;
	  std::vector<ID_ENTITY> statics;
      std::vector<ObjectInstance> drawQueue;
      Rococo::Graphics::ICameraSupervisor& camera;

	  GlobalState state;

      RGBA clearColour{ 0,0,0,1 };

	  enum { MAX_LIGHTS = 1 };
	  LightConstantBuffer lights[MAX_LIGHTS] = { 0 };

	  IScenePopulator* populator = nullptr;

	  ID_CUBE_TEXTURE skyboxId;
	  ID_CUBE_TEXTURE environmentMap;

	  IRigs& rigs;

	  AutoFree<IRodTesselatorSupervisor> debugTesselator;
   public:
      Scene(IECS& _ecs, ICameraSupervisor& _camera, IRigs& _rigs) :
         ecs(_ecs), camera(_camera), rigs(_rigs),
		 debugTesselator(CreateIsolatedRodTesselator())
      {
		  debugTesselator->SetUVScale(1.0f);
      }
      
      ~Scene()
      {
      }

	  void SetEnvironmentMap(ID_CUBE_TEXTURE mapId) override
	  {
		  this->environmentMap = mapId;
	  }

	  void SetSkyBox(ID_CUBE_TEXTURE cubeId) override
	  {
		  this->skyboxId = cubeId;
	  }

	  void GetCamera(Matrix4x4& worldToScreen, Matrix4x4& worldToCamera, Matrix4x4& proj, Vec4& eye, Vec4& viewDir) override
	  {
		  camera.GetWorld(worldToCamera);
		  camera.GetWorldAndProj(worldToScreen);
		  camera.GetPosition(eye);
		  camera.GetProjection(proj);
		  eye.w = 1.0f;
		  viewDir = Vec4::FromVec3(worldToCamera.GetForwardDirection(), 0);
	  };

	  ID_CUBE_TEXTURE GetEnvironmentMap() const override
	  {
		  return environmentMap;
	  }

	  ID_CUBE_TEXTURE GetSkyboxCubeId() const override
	  {
		  return skyboxId;
	  }

	  Lights GetLights() const override
	  {
		  return { lights, MAX_LIGHTS };
	  }

	  void SetPopulator(IScenePopulator* populator) override
	  {
		  this->populator = populator;
	  }

	  void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc, EShadowCasterFilter filter) override
	  {
		  rc.Gui3D().Clear3DGuiTriangles();

		  if (populator)
		  {
			  populator->PopulateShadowCasters(*this, drd);
		  }

		  RenderEntities_InternalAnyPhase(rc, filter);
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
		  lights[index].shadowFudge = spec.shadowFudge;
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
		  // auto& skeles = rigs.Skeles();

		  for (auto id : dynamics)
		  {
			  auto skeletonComponent = API::ForISkeletonComponent::Get(id);
			  auto animationComponent = API::ForIAnimationComponent::Get(id);
			  if (skeletonComponent && animationComponent)
			  {
				  auto* skele = skeletonComponent->Skeleton();
				  if (skele)
				  {
					  AnimationAdvanceArgs args
					  {
						  *skele,
						  poses,
						  dt
					  };
					  animationComponent->Advance(args);
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

	  void OnGuiResize(Vec2i) override
	  {

	  }

      void RenderGui(IGuiRenderContext&) override
      {
      }

      void RenderAndFlushDrawQueue(ID_SYS_MESH meshId, IRenderContext& rc)
      {
         if (drawQueue.empty()) return;
         rc.Draw(meshId, &drawQueue[0], (uint32)drawQueue.size());
         drawQueue.clear();
      }

	  void RenderAndFlushDrawQueue_NoTexture(ID_SYS_MESH meshId, IRenderContext& rc)
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
		  // Where some statics have a series bodies that share the same mesh, we want to render them as a batch using instancing.
		  for (auto i : statics)
		  {
			  auto body = API::ForIBodyComponent::Get(i);
			  if (!body)
			  {
				  Throw(0, "Scene: Unexpected missing entity with id #%llX", i.Value());
			  }

			  if (body->Mesh())
			  {
				  ObjectInstance instance{ body->Model(), body->Scale(), 0.0f, RGBA(0, 0, 0, 0) };
				  drawQueue.push_back(instance);
				  RenderAndFlushDrawQueue(body->Mesh(), r);
			  }
		  }
	  }

	  void RenderEntities_InternalDynamic(IRenderContext& r)
	  {
		  ID_SYS_MESH meshId;

		  for (auto i : debugEntities)
		  {
			  AddDebugBones(i, r, *debugTesselator);
		  }

		  for (auto i : dynamics)
		  {
			  auto body = API::ForIBodyComponent::Get(i);
			  if (!body)
			  {
				  Throw(0, "Scene: Unexpected missing entity with id #%lld", i.Value());
			  }

			  auto skeletonComponent = API::ForISkeletonComponent::Get(i);
			 
			  auto* skeleton = skeletonComponent ? skeletonComponent->Skeleton() : nullptr;

			  if (body->Mesh() != meshId)
			  {
				  RenderAndFlushDrawQueue(meshId, r);
				  meshId = body->Mesh();
			  }

			  ObjectInstance instance{ body->Model(), Vec3 {1.0f, 1.0f, 1.0f}, 1.0f, RGBA(0, 0, 0, 0) };
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
				  RenderAndFlushDrawQueue(meshId, r);
				  meshId = ID_SYS_MESH::Invalid();
			  }
		  }

		  RenderAndFlushDrawQueue(meshId, r);
	  }

	  void RenderEntities_InternalAnyPhase(IRenderContext& r, EShadowCasterFilter filter)
	  {
		  drawQueue.clear();

		  if (filter == EShadowCasterFilter::UnskinnedCastersOnly)
		  {
			  RenderEntities_InternalStatic(r);
		  }
		  else
		  {
			  RenderEntities_InternalDynamic(r);
		  }
	  }

      void RenderObjects(IRenderContext& rc, EShadowCasterFilter filter) override
      {
		  debugTesselator->Clear();

		  if (populator)
		  {
			  populator->PopulateScene(*this);
		  }

		  RenderEntities_InternalAnyPhase(rc, filter);
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
      ISceneSupervisor* CreateScene(IECS& ecs, ICameraSupervisor& camera, IRigs& rigs)
      {
         return new Scene(ecs, camera, rigs);
      }
   }
}