#include "hv.events.h"
#include <rococo.maths.h>

namespace
{
   using namespace HV;
   using namespace HV::Graphics;
   using namespace HV::Entities;

   class Camera : public ICameraSupervisor, public IObserver
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
      IPublisher& publisher;
      bool isDirty{ false };
      bool isFPSlinked{ false };
      FPSAngles fps{ 0, 0, 0 };
   public:
      Camera(IInstancesSupervisor& _instances, IRenderer& _renderer, IPublisher& _publisher) :
         instances(_instances),
         renderer(_renderer),
         publisher(_publisher)
      {
         Clear();

         publisher.Attach(this);
      }

      ~Camera()
      {
         publisher.Detach(this);
      }

      virtual void OnEvent(Event& ev)
      {
         if (ev == HV::Events::Player::OnPlayerViewChange)
         {
            auto& pvce = Rococo::Events::As <HV::Events::Player::OnPlayerViewChangeEvent>(ev);
            if (pvce.elevationDelta != 0 && orientationGuideId == pvce.playerEntityId)
            {
               float newElevation = fps.elevation + (float)pvce.elevationDelta;
               newElevation = min(89.0f, newElevation);
               newElevation = max(-89.0f, newElevation);
               fps.elevation = Degrees{ newElevation };
            }
         }
         else if (ev == HV::Events::Input::OnMouseMoveRelative)
         {
            auto& mmre = Rococo::Events::As <HV::Events::Input::OnMouseMoveRelativeEvent>(ev);
            float newHeading = fps.heading + mmre.dx;
            fps.heading = Degrees{ fmodf(newHeading, 360.0f) };
         }
      }

      virtual void Clear()
      {
         projection = world = Matrix4x4::Identity();
         followingId = orientationGuideId = ID_ENTITY::Invalid();
         orientation = Quat{ { 0, 0, 0 }, 1.0 };
         position = Vec3 { 0, 0, 0 };
         orientationFlags = 0;
         isDirty = false;
         fps = { 0,0,0 };
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
         isFPSlinked = false;
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

            /*

            (Ax  Bx  Cx)(1)       (Ax)
            (Ay  By  Cy)(0)   ->  (Ay)
            (Az  Bz  Cz)(0)       (Az)

            (Ax  Bx  Cx)(0)       (Bx)
            (Ay  By  Cy)(1)   ->  (By)
            (Az  Bz  Cz)(0)       (Bz)

            (Ax  Bx  Cx)(0)       (Cx)
            (Ay  By  Cy)(0)   ->  (Cy)
            (Az  Bz  Cz)(1)       (Cz)

            Now, if ABC is the model matrix,
            Then ABC transforms i to A, j to B and k to C

            In an untransformed state (1 0 0) is right
                                      (0 1 0) is forward
                                      (0 0 1) is up

            So A gives the right vector, B gives the forward vector, and C is the up vector

            // In the camera 
            */

            if (isFPSlinked)
            {
               // With the identify world matrix, the camera is facing up, and x is to the right, and up is to the south
               // If a rotation to 0 elevation faces the camera forward with up vertical and x still to the right
               // We rotate the camera 90 degrees clockwise around the x-axis to point it so.
               // If the camera is viewing a particle at point P in the world, we can transform the point P into camera space
               // by rotating it 90 degress anticlockwise around the x-axis

               float cameraToWorldElevation = 90.0f - fps.elevation;
               float worldToCameraElevation = -cameraToWorldElevation;

               // The heading gives us compass direction with 0 = North and 90 = East
               // Heading is thus clockwise when positive, but our rotation matrix has anticlockwise for positive angles
               // So switch signs

               float cameraToWorldYaw = -fps.heading;
               float worldToCameraYaw = -cameraToWorldYaw;

               Matrix4x4 Rz = Matrix4x4::RotateRHAnticlockwiseZ(Degrees{ worldToCameraYaw });
               Matrix4x4 Rx = Matrix4x4::RotateRHAnticlockwiseX(Degrees{ worldToCameraElevation });

               // Lean is not yet implemented
               world = Rz * Rx;

               isDirty = false;
            }
            else
            {
               Matrix4x4::GetRotationQuat(model, orientation);
               isDirty = true;
            }        
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

         isFPSlinked = true;
      }

      virtual void OrientateToEntity(ID_ENTITY id, int32 flags)
      {
         orientationGuideId = ID_ENTITY::Invalid();

         Matrix4x4 model;
         instances.ConcatenateModelMatrices(orientationGuideId, model);

         model.row0.x = 0;
         model.row0.y = 0;
         model.row0.z = 0;

         isFPSlinked = false;

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
      ICameraSupervisor* CreateCamera(IInstancesSupervisor& instances, IRenderer& renderer, IPublisher& publisher)
      {
         return new Camera(instances, renderer, publisher);
      }
   }
}