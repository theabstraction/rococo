#include <rococo.mplat.h>
#include <unordered_map>
#include "mplat.components.h"

using namespace Rococo;
using namespace Rococo::Components;
using namespace Rococo::Entities;

namespace
{
   struct Mobiles : public IMobilesSupervisor
   {
      IInstancesSupervisor& instances;
      IRCObjectTable& ecs;

      Mobiles(IInstancesSupervisor& _instances) : instances(_instances), ecs(instances.ECS())
      {
      }

      ~Mobiles()
      {
      }

      bool TryMoveMobile(const MoveMobileArgs& tmm) override
      {
          auto mobile = ecs.GetSkeletonComponent(tmm.entityId);
          if (mobile)
          {
              auto angles = mobile->FPSOrientation();
              angles.elevation.degrees += fmodf(tmm.delta.elevation, 360.0f);
              angles.heading.degrees += fmodf(tmm.delta.heading, 360.0f);
              angles.tilt.degrees += tmm.delta.tilt;
          
              if (angles.tilt.degrees < -90.0f) angles.tilt.degrees = -90.0f;
              if (angles.tilt.degrees > 90.0f) angles.tilt.degrees = 90.0f;

              mobile->SetFPSOrientation(angles);

              auto body = ecs.GetBodyComponent(tmm.entityId);

              if (body)
              {
                  auto& modelRef = body->Model();
                  Vec3 pos = modelRef.GetPosition();

                  Matrix4x4 model = Matrix4x4::RotateRHAnticlockwiseZ(Degrees{ -angles.heading.degrees });

                  auto forward = model.GetForwardDirection();
                  auto right = model.GetRightDirection();

                  Vec3 df = forward * tmm.fowardDelta;
                  Vec3 dr = right * tmm.straffeDelta;

                  model.SetPosition(pos + df + dr);

                  body->SetModel(model);
                  return true;
              }
          }

         return false;
      }
         
      void Link(ID_ENTITY id) override
      {
          auto skeleton = ecs.GetSkeletonComponent(id);
          if (!skeleton)
          {
              skeleton = ecs.AddSkeletonComponent(id);
          }

          auto body = ecs.GetBodyComponent(id);
          if (!body)
          {
              body = ecs.GetBodyComponent(id);
          }
      }

      void GetAngles(ID_ENTITY id, FPSAngles& angles) override
      {
         auto skeleton = ecs.GetSkeletonComponent(id);
         if (!skeleton)
         {
            Throw(0, "No such entity");
         }

         angles = skeleton->FPSOrientation();
      }

      void SetAngles(ID_ENTITY id, const FPSAngles& angles) override
      {
          auto skeleton = ecs.GetSkeletonComponent(id);
          if (!skeleton)
          {
              Throw(0, "No such entity");
          }

          skeleton->SetFPSOrientation(angles);
      }

      void Free() override
      {
         delete this;
      }
   };
}

namespace Rococo
{
   namespace Entities
   {
      IMobilesSupervisor* CreateMobilesSupervisor(IInstancesSupervisor& instances)
      {
         return new Mobiles(instances);
      }
   }
}