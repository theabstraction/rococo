#include <rococo.mplat.h>
#include <rococo.maths.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Graphics;
   using namespace Rococo::Entities;

   class Camera : public ICameraSupervisor, public IMathsVenue
   {
	   Matrix4x4 world;

	   struct Projection
	   {
		  Degrees fov;
		  Metres near;
		  Metres far;
	   } projectionParameters = { { 60 }, {0.1f}, {1000.0f} };

	  Matrix4x4 projection;
      Quat orientation;
      Vec3 position;
      int32 orientationFlags;
      ID_ENTITY followingId;
      ID_ENTITY orientationGuideId;
      IInstancesSupervisor& instances;
      IMobiles& mobiles;
      IRenderer& renderer;
      Degrees elevation{ 0 };
      Degrees heading{ 0 };
      bool isDirty{ false };
      bool isFPSlinked{ false };
      Vec3 relativePos;
   public:
      Camera(IInstancesSupervisor& _instances, IMobiles& _mobiles, IRenderer& _renderer) :
         instances(_instances),
         mobiles(_mobiles),
         renderer(_renderer)
      {
         Clear();
      }

      ~Camera()
      {
      }

      void GetProjection(Matrix4x4& proj)
      {
          proj = this->projection;
      }

      IMathsVenue& Venue()
      {
         return *this;
      }

      void ShowVenue(IMathsVisitor& visitor)
      {
         visitor.Clear();
         visitor.Show("World->Camera", world);
         visitor.Show("Camera->Screen", projection);
         visitor.ShowString("", "");

         visitor.ShowRow("Position", &position.x, 3);

         if (!isFPSlinked)
         {
            visitor.ShowRow("Orientation", &orientation.v.x, 4);
         }

         if (isFPSlinked)
         {
            visitor.Show("Heading", heading);
            visitor.Show("Elevation", elevation);
         }

         visitor.ShowString("", "");

         visitor.ShowHex("OrientFlags", orientationFlags);
         visitor.ShowDecimal("Following", followingId.value);
         visitor.ShowDecimal("Guide", orientationGuideId.value);

         visitor.ShowString("", "");

         visitor.ShowBool("Dirty", isDirty);
         visitor.ShowBool("FPSLinked", isFPSlinked);

         visitor.ShowPointer("this", this);
      }

      void ElevateView(ID_ENTITY entityId, Degrees delta, cr_vec3 relativePos) override
      {
         if (delta != 0 && orientationGuideId == entityId)
         {
            float newElevation = elevation + (float)delta * 0.25f;
            newElevation = min(89.0f, newElevation);
            newElevation = max(-89.0f, newElevation);
            elevation = Degrees{ newElevation };           
            isFPSlinked = true;
         }
         
         this->relativePos = relativePos;
      }

      void Clear() override
      {
         relativePos = Vec3{ 0,0,0 };
         projection = world = Matrix4x4::Identity();
         followingId = orientationGuideId = ID_ENTITY::Invalid();
         orientation = Quat{ { 0, 0, 0 }, 1.0 };
         position = Vec3{ 0, 0, 0 };
         orientationFlags = 0;
         isDirty = false;
         elevation = Degrees{ 0 };
      }

      float AspectRatio() override
      {
         GuiMetrics metrics;
         renderer.GetGuiMetrics(metrics);

         return metrics.screenSpan.x / (float)metrics.screenSpan.y;
      }

      void GetPosition(Vec3& position) override
      {
         position = this->position + this->relativePos;
      }

      void GetOrientation(Quat& orientation) override
      {
         orientation = this->orientation;
      }

      void SetPosition(const Vec3& position) override
      {
         followingId = ID_ENTITY::Invalid();
         isDirty = true;
         this->position = position;
      }

      void SetOrientation(const Quat& orientation) override
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
            isDirty = true;
         }

         if (orientationGuideId)
         {
            Matrix4x4 model;
            instances.ConcatenateModelMatrices(orientationGuideId, model);

            model.row0.x = 0;
            model.row0.y = 0;
            model.row0.z = 0;

            position = model.GetPosition();

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
               // With the identity world matrix, the camera is facing up, and x is to the right, and up is to the south
               // If a rotation to 0 elevation faces the camera forward with up vertical and x still to the right
               // We rotate the camera 90 degrees clockwise around the x-axis to point it so.
               // If the camera is viewing a particle at point P in the world, we can transform the point P into camera space
               // by rotating it 90 degress anticlockwise around the x-axis

               float cameraToWorldElevation = 90.0f - elevation;
               float worldToCameraElevation = -cameraToWorldElevation;

               // The heading gives us compass direction with 0 = North and 90 = East
               // Heading is thus clockwise when positive, but our rotation matrix has anticlockwise for positive angles
               // So switch signs

               FPSAngles angles;
               mobiles.GetAngles(orientationGuideId, angles);

               heading = angles.heading;

               Matrix4x4 Rz = Matrix4x4::RotateRHAnticlockwiseZ(Degrees{ angles.heading });
               Matrix4x4 Rx = Matrix4x4::RotateRHAnticlockwiseX(Degrees{ worldToCameraElevation });

               Matrix4x4 T = Matrix4x4::Translate(-position-relativePos);

               // Lean is not yet implemented
               world = Rx * Rz * T;

               float detW = Determinant(world);
               if (detW < 0.9f || detW > 1.1f)
               {
                  Throw(0, "Bad world-to-camera determinant: %f", detW);
               }

               isDirty = false;
            }
            else
            {
               Matrix4x4::GetRotationQuat(model, orientation);
               isDirty = true;
            }
         }
      }

      void SetRHProjection(Degrees fov, float near, float far) override
      {
		  projectionParameters.fov = fov;
		  projectionParameters.near = Metres{ near };
		  projectionParameters.far = Metres{ far };
      }

      void GetWorld(Matrix4x4& worldToCamera) override
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

      void GetWorldAndProj(Matrix4x4& worldAndProj) override
      {
         Matrix4x4 world;
         GetWorld(world);

		 projection = Matrix4x4::GetRHProjectionMatrix(projectionParameters.fov, AspectRatio(), projectionParameters.near, projectionParameters.far);

         worldAndProj = projection * world;
      }

      void FollowEntity(ID_ENTITY id) override
      {
         followingId = id;
      }

      void MoveToEntity(ID_ENTITY id) override
      {
         followingId = ID_ENTITY::Invalid();
         instances.ConcatenatePositionVectors(id, position);
      }

      void OrientateWithEntity(ID_ENTITY id, int32 flags) override
      {
         orientationGuideId = id;

         isFPSlinked = true;
      }

      void OrientateToEntity(ID_ENTITY id, int32 flags) override
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

      void Free() override
      {
         delete this;
      }
   };
}


namespace Rococo
{
   namespace Graphics
   {
      ICameraSupervisor* CreateCamera(IInstancesSupervisor& instances, IMobiles& mobiles, IRenderer& renderer)
      {
         return new Camera(instances, mobiles, renderer);
      }
   }
}