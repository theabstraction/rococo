#include <rococo.mplat.h>
#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Entities;

namespace Mplat::CameraCode
{
    class Camera : public ICameraSupervisor, public IMathsVenue
    {
        mutable Matrix4x4 worldToCamera;
        mutable Matrix4x4 cameraToWorld;

        struct Projection
        {
            Degrees fov;
            Metres near;
            Metres far;
        } projectionParameters = { { 60 }, {0.1f}, {1000.0f} };

        mutable Matrix4x4 projection;
        Quat orientation;
        Vec3 position;
        int32 orientationFlags;
        ID_ENTITY followingId;
        ID_ENTITY orientationGuideId;
        IMobiles& mobiles;
        IRenderer& renderer;
        Degrees elevation{ 0 };
        Degrees heading{ 0 };
        mutable bool isDirty{ false };
        bool isFPSlinked{ false };
        Vec3 relativePos;
    public:
        Camera(IMobiles& _mobiles, IRenderer& _renderer) :
            mobiles(_mobiles),
            renderer(_renderer)
        {
            Clear();
        }

        ~Camera()
        {
        }

        void GetProjection(Matrix4x4& proj) const override
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
            visitor.Show("World->Camera", worldToCamera);
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
            visitor.ShowDecimal("Following", followingId.Value());
            visitor.ShowDecimal("Guide", orientationGuideId.Value());

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
            projection = worldToCamera = cameraToWorld = Matrix4x4::Identity();
            followingId = orientationGuideId = ID_ENTITY::Invalid();
            orientation = Quat{ { 0, 0, 0 }, 1.0 };
            position = Vec3{ 0, 0, 0 };
            orientationFlags = 0;
            isDirty = false;
            elevation = Degrees{ 0 };
        }

        float AspectRatio() const override
        {
            GuiMetrics metrics;
            renderer.GetGuiMetrics(metrics);

            return metrics.screenSpan.x / (float)metrics.screenSpan.y;
        }

        void GetPosition(Vec3& position) const override
        {
            position = this->position + this->relativePos;
        }

        void GetOrientation(Quat& orientation) const override
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

        void Update(const IUltraClock&)
        {
            // Generally this be called after entities are updated and just before scene is rendered
            if (followingId)
            {
                isDirty = true;
            }

            if (orientationGuideId)
            {
                Matrix4x4 model;
                auto body = API::ForIBodyComponent::Get(orientationGuideId);
                if (body)
                {
                    body->GetModel(OUT model);
                }
                else
                {
                    model = Matrix4x4::Identity();
                }

                model.row0.x = 0;
                model.row0.y = 0;
                model.row0.z = 0;

                position = model.GetPosition();

                if (isFPSlinked)
                {
                    FPSAngles fpsAngles;
                    mobiles.GetAngles(orientationGuideId, OUT fpsAngles);

                    heading = fpsAngles.heading;

                    FPSAngles cameraOrientation;
                    cameraOrientation.elevation = elevation;
                    cameraOrientation.heading = heading;
                    cameraOrientation.tilt = fpsAngles.tilt;

                    FPS::SetWorldToCameraTransformToFPSRHMapSystem(OUT worldToCamera, cameraOrientation, position + relativePos);
                    cameraToWorld = InvertMatrix(worldToCamera);
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

        void GetWorld(Matrix4x4& worldToCamera) const override
        {
            if (isDirty)
            {
                isDirty = false;
                Matrix4x4 rot;
                Matrix4x4::FromQuatAndThenTranspose(orientation, rot);
                Matrix4x4 translate = Matrix4x4::Translate(-position);
                this->worldToCamera = rot * translate;
                this->cameraToWorld = InvertMatrix(this->worldToCamera);
            }
            worldToCamera = this->worldToCamera;
        }

        void GetWorldAndProj(Matrix4x4& worldAndProj) const override
        {
            Matrix4x4 worldToCamera;
            GetWorld(worldToCamera);

            projection = Matrix4x4::GetRHProjectionMatrix(projectionParameters.fov, AspectRatio(), projectionParameters.near, projectionParameters.far);

            worldAndProj = projection * worldToCamera;
        }

        void GetPixelDirection(const Vec2& zeroCentredFrustumPos, OUT Vec3& worldSpaceRayDirectionFromCameraEye) const override
        {
            // Grab the two FOV elements, A and B from the projection matrix
            float A = projection.row0.x;
            float B = projection.row1.y;

            Vec3 cameraSpacePixelPos = Vec4{ projectionParameters.near * zeroCentredFrustumPos.x / A, projectionParameters.near * zeroCentredFrustumPos.y / B, -projectionParameters.near };

            // Note, that if we use the projection matrix on this 'cameraSpacePixelPos' vector, then following the A and B multiplication and the z divide, then we arrive at { zeroCentredFrustumPos.x, zeroCentredFrustumPos.y, 0.0f }

            // The camera eye is at the origin, so cameraSpacePixelPos serves as the direction vector in camera space of the ray from the eye through the screen
           
            Vec3 rawPos;
            TransformDirection(cameraToWorld, cameraSpacePixelPos, OUT rawPos);
            
            OUT worldSpaceRayDirectionFromCameraEye = Normalize(rawPos);
        }

        void FollowEntity(ID_ENTITY id) override
        {
            followingId = id;
        }

        void MoveToEntity(ID_ENTITY id) override
        {
            followingId = ID_ENTITY::Invalid();

            auto entity = API::ForIBodyComponent::Get(id);
            if (entity)
            {
                position += entity->Model().GetPosition();
            }
        }

        void OrientateWithEntity(ID_ENTITY id, int32 flags) override
        {
            UNUSED(flags);
            orientationGuideId = id;

            isFPSlinked = true;
        }

        void OrientateToEntity(ID_ENTITY id, int32 flags) override
        {
            UNUSED(id);
            UNUSED(flags);
            orientationGuideId = ID_ENTITY::Invalid();

            Matrix4x4 model;
            auto body = API::ForIBodyComponent::Get(orientationGuideId);
            if (body)
            {
                body->GetModel(OUT model);
            }
            else
            {
                model = Matrix4x4::Identity();
            }

            model.row0.x = 0;
            model.row0.y = 0;
            model.row0.z = 0;

            isFPSlinked = false;

            Matrix4x4::GetRotationQuat(model, orientation);
        }

        float32 Far() const override
        {
            return projectionParameters.far;
        }

        float32 Near() const override
        {
            return projectionParameters.near;
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
      ICameraSupervisor* CreateCamera(IMobiles& mobiles, IRenderer& renderer)
      {
         return new Mplat::CameraCode::Camera(mobiles, renderer);
      }
   }
}