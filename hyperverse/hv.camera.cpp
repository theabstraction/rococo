#include "hv.h"
#include <rococo.maths.h>

namespace
{
   using namespace HV;
   using namespace HV::Graphics;

   class Camera : public ICameraSupervisor
   {
      Matrix4x4 world;
      Matrix4x4 projection;
      Quat orientation;
      Vec3 position;
      ID_ENTITY followingId;
      ID_ENTITY orientationGuideId;     
      int32 orientationFlags;
      IInstancesSupervisor& instances;
      IRenderer& renderer; 
      bool isDirty;
   public:
      Camera(IInstancesSupervisor& _instances, IRenderer& _renderer): 
         instances(_instances),
         renderer(_renderer)
      {
         Clear();
      }

      virtual void Clear()
      {
         projection = world = Matrix4x4::Identity();
         followingId = orientationGuideId = ID_ENTITY::Invalid();
         orientation = Quat{ { 0, 0, 0 }, 1.0 };
         position = Vec3 { 0, 0, 0 };
         orientationFlags = 0;
         isDirty = false;
      }

      virtual float AspectRatio()
      {
         GuiMetrics metrics;
         renderer.GetGuiMetrics(metrics);

         return metrics.screenSpan.x / (float)metrics.screenSpan.y;
      }

      virtual void GetPosition(Vec3& position)
      {
         position = this->position;
      }

      virtual void GetOrientation(Quat& orientation)
      {
         orientation = this->orientation;
      }

      virtual void SetPosition(const Vec3& position)
      {
         followingId = ID_ENTITY::Invalid();
         isDirty = true;
         this->position = position;
      }

      virtual void SetOrientation(const Quat& orientation)
      {
         orientationGuideId = ID_ENTITY::Invalid();
         isDirty = true;
         this->orientation = orientation;
      }

      void Update(const IUltraClock& clock)
      {
         // Generally this be called after entities are updated and just before scene is rendered
         if (followingId)
         {
            instances.ConcatenatePositionVectors(followingId, position);
            isDirty = true;
         }

         if (orientationGuideId)
         {
            Matrix4x4 model;
            instances.ConcatenateModelMatrices(orientationGuideId, model);
            
            model.row0.x = 0;
            model.row0.y = 0;
            model.row0.z = 0;

            Matrix4x4::GetRotationQuat(model, orientation);
            isDirty = true;
         }
      }

      virtual void SetRHProjection(Degrees fov, float near, float far)
      {
         this->projection = Matrix4x4::GetRHProjectionMatrix(fov, AspectRatio(), near, far);
      }

      virtual void SetProjection(const Matrix4x4& proj)
      {
         this->projection = proj;
      }

      virtual void GetWorld(Matrix4x4& worldToCamera)
      {
         if (isDirty)
         {
            isDirty = false;
            Matrix4x4 rot;
            Matrix4x4::FromQuatAndThenTranspose(orientation, rot);
            Matrix4x4 translate = Matrix4x4::Translate(-position);
            this->world = rot * translate;
         }
         worldToCamera = this->world;
      }

      virtual void GetWorldAndProj(Matrix4x4& worldAndProj)
      {
         Matrix4x4 world;
         GetWorld(world);
         worldAndProj = projection * world;
      }

      virtual void FollowEntity(ID_ENTITY id)
      {
         followingId = id;
      }

      virtual void MoveToEntity(ID_ENTITY id)
      {
         followingId = ID_ENTITY::Invalid();
         instances.ConcatenatePositionVectors(id, position);
      }

      virtual void OrientateWithEntity(ID_ENTITY id, int32 flags)
      {
         orientationGuideId = id;
      }

      virtual void OrientateToEntity(ID_ENTITY id, int32 flags)
      {
         orientationGuideId = ID_ENTITY::Invalid();

         Matrix4x4 model;
         instances.ConcatenateModelMatrices(orientationGuideId, model);

         model.row0.x = 0;
         model.row0.y = 0;
         model.row0.z = 0;

         Matrix4x4::GetRotationQuat(model, orientation);
      }

      virtual void Free()
      {
         delete this;
      }
   };
}


namespace HV
{
   namespace Graphics
   {
      ICameraSupervisor* CreateCamera(IInstancesSupervisor& instances, IRenderer& renderer)
      {
         return new Camera(instances, renderer);
      }
   }
}