#include <rococo.mplat.h>

#include <vector>
#include <algorithm>

#include <rococo.textures.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Entities;
   using namespace Rococo::Graphics;

   typedef std::vector<VertexTriangle> TTriangles;

   struct NullMeshBuilder : IMeshBuilder
   {
	   void AddMesh(const Matrix4x4& transform, const fstring& sourceName) override {}
	   void AddTriangleEx(const VertexTriangle& t) override  {}
	   void AddTriangle(const ObjectVertex& a, const ObjectVertex& b, const ObjectVertex& c) override  {}
	   void AddPhysicsHull(const Triangle& t) override  {}
	   void Begin(const fstring& meshName) override {}
	   void End(boolean32 preserveCopy, boolean32 invisible) override  {}
	   void Clear() override {}
	   void Delete(const fstring& fqName) override {}
	   void SetShadowCasting(const fstring& fqName, boolean32 isActive) override {}
	   void SetSpecialShader(const fstring& fqName, const fstring& psSpotlightPingPath, const fstring& psAmbientPingPath, boolean32 alphaBlending) override  {}
	   void Span(Vec3& span, const fstring& fqName) override {}
   } s_NullMeshBuilder;

   class Scene : public ISceneSupervisor, public ISceneBuilderSupervisor
   {
      IInstancesSupervisor& instances;
      std::vector<ID_ENTITY> entities;
      std::vector<ObjectInstance> drawQueue;
      Rococo::Graphics::ICameraSupervisor& camera;

	  GlobalState state;

      RGBA clearColour{ 0,0,0,1 };

	  enum { MAX_LIGHTS = 1 };
	  Light lights[MAX_LIGHTS] = { 0 };

	  IScenePopulator* populator = nullptr;

	  ID_CUBE_TEXTURE skyboxId;

	  ISkeletons& skeletons;

	  AutoFree<IRodTesselatorSupervisor> debugTesselator;
   public:
      Scene(IInstancesSupervisor& _instances, ICameraSupervisor& _camera, ISkeletons& _skeletons) :
         instances(_instances), camera(_camera), skeletons(_skeletons)
      {
		  debugTesselator = CreateRodTesselator(s_NullMeshBuilder);
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

	  void AddDebugBone(const IBone& bone, const Matrix4x4& model, IRenderContext& rc)
	  {
		  if (bone.Length() > 0)
		  {
			  MaterialVertexData mat;
			  mat.colour = RGBAb(255, 255, 255, 255);
			  mat.gloss = 0;
			  mat.materialId = 0;

			  debugTesselator->SetMaterialTop(mat);
			  debugTesselator->SetMaterialMiddle(mat);
			  debugTesselator->SetMaterialBottom(mat);

			  debugTesselator->AddTube(bone.Length(), 0.05_metres, 0.05_metres, 8);
			  debugTesselator->TransformVertices(model);
			  rc.Add3DGuiTriangles(debugTesselator->begin(), debugTesselator->end());
			  debugTesselator->Clear();
		  }

		  for (auto& child : bone)
		  {
			  Matrix4x4 childModelMatrix = child->GetMatrix() * model;
			  AddDebugBone(*child, childModelMatrix, rc);
		  }
	  }

	  void AddDebugBones(IEntity& e, IRenderContext& rc)
	  {
		  auto skeleton = e.GetSkeleton(skeletons);
		  if (skeleton)
		  {
			  auto* root = skeleton->Root();
			  if (root)
			  {
				  Matrix4x4 m = e.Model();
				  Matrix4x4 rY = Matrix4x4::RotateRHAnticlockwiseY(0_degrees);
				  Matrix4x4 R = m * rY;
				  AddDebugBone(*root, R, rc);
			  }
		  }
	  }

	  void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc) override
	  {
		  debugTesselator->Clear();
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

			  AddDebugBones(*entity, rc);

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

      void Clear() override
      {
         entities.clear();
      }

      void AddStatics(ID_ENTITY id) override
      { 
		  if (!id)
		  {
			  Throw(0, "Scene.AddStatics: id was zero/invalid");
		  }
          entities.push_back(id);
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
      ISceneSupervisor* CreateScene(IInstancesSupervisor& instances, ICameraSupervisor& camera, ISkeletons& skeletons)
      {
         return new Scene(instances, camera, skeletons);
      }
   }
}