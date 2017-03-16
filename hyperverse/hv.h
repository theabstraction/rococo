#ifndef ROCOCO_HV
#define ROCOCO_HV

#include <rococo.api.h>
#include <rococo.renderer.h>

using namespace Rococo;
using namespace Rococo::Events;

namespace HV
{
   ROCOCO_ID(ID_ENTITY, int64, 0);
}

#include "hv.script.types.h"
#include "hv.sxh.h"

namespace HV
{
   ROCOCOAPI IEntity
   {
      virtual const wchar_t* Name() const = 0;
      virtual const Vec3& Position() const = 0;
      virtual const Matrix4x4& Model() const = 0;
      virtual ID_ENTITY ParentId() const = 0;
      virtual const ID_ENTITY* begin() const = 0;
      virtual const ID_ENTITY* end() const = 0;
      virtual ID_SYS_MESH MeshId() const = 0;
      virtual ID_TEXTURE TextureId() const = 0;
   };

   ROCOCOAPI IEntityCallback
   {
      virtual void OnEntity(int64 index, IEntity& entity, ID_ENTITY id) = 0;
   };

   namespace Strings
   {
      enum { MAX_FQ_NAME_LEN = 127 };
      void ValidateFQNameIdentifier(const wchar_t* fqName);
   }

   namespace Graphics
   {
      ROCOCOAPI IMeshBuilderSupervisor: public IMeshBuilder
      {
         virtual void Free() = 0;
         virtual bool TryGetByName(const wchar_t* name, ID_SYS_MESH& id) = 0;
      };

      ROCOCOAPI IInstancesSupervisor: public IInstances
      {
         virtual void ForAll(IEntityCallback& cb) = 0;
         virtual void Free() = 0;
         virtual IEntity* GetEntity(ID_ENTITY id) = 0;
         virtual void ConcatenateModelMatrices(ID_ENTITY id, Matrix4x4& result) = 0;
         virtual void ConcatenatePositionVectors(ID_ENTITY id, Vec3& position) = 0;
      };

      ROCOCOAPI ISceneBuilderSupervisor: public ISceneBuilder
      {
         virtual void Free() = 0;
      };

      ROCOCOAPI ICameraSupervisor : public ICamera
      {
         virtual void Free() = 0;
         virtual void Update(const IUltraClock& clock) = 0;
      };

      ROCOCOAPI ISceneSupervisor : public IScene
      {
         virtual void Free() = 0;
         virtual ISceneBuilderSupervisor&  Builder() = 0;
      };

      ISceneSupervisor* CreateScene(IInstancesSupervisor& instances, ICameraSupervisor& camera);

      ICameraSupervisor* CreateCamera(IInstancesSupervisor& instances, IRenderer& render);

      IMeshBuilderSupervisor* CreateMeshBuilder(IRenderer& renderer);
      IInstancesSupervisor* CreateInstanceBuilder(IMeshBuilderSupervisor& builder, IRenderer& renderer);
   }

   namespace Events
   {
      extern EventId OnTick;

      struct OnTickEvent : public Event
      {
         OnTickEvent() : Event(OnTick) {}
         IUltraClock* clock;
         uint32 frameSleep{ 5 };
      };

      extern EventId OnFileChanged;

      struct OnFileChangedEvent : public Event
      {
         OnFileChangedEvent() : Event(OnFileChanged) {}
         FileModifiedArgs* args;
      };
   }
  
   bool QueryYesNo(Windows::IWindow& ownerWindow, const wchar_t* message);

   struct Cosmos
   {
      Rococo::Events::IPublisher& publisher;
      IInstallation& installation;
      ISourceCache& sources;
      IDebuggerWindow& debugger;
      Windows::IWindow& mainWindow;
      IRenderer& renderer;
      Graphics::ISceneSupervisor& scene;
      Graphics::IMeshBuilderSupervisor& meshes;
      Graphics::IInstancesSupervisor& instances;
      Graphics::ICameraSupervisor& camera;
   };

   IApp* CreateHVApp(Cosmos& e);
   void RunEnvironmentScript(Cosmos& e, const wchar_t* name);
}


#endif