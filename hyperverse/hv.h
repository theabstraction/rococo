#ifndef ROCOCO_HV
#define ROCOCO_HV

#include <rococo.api.h>
#include <rococo.renderer.h>

using namespace Rococo;
using namespace Rococo::Events;

namespace HV
{
   ROCOCO_ID(ID_ENTITY, int64, 0);

   struct IPlayerSupervisor;
}

#include "hv.script.types.h"
#include "hv.sxh.h"

namespace HV
{
   struct ConfigText
   {
      const fstring key;
      const fstring value;

      operator const fstring() { return key; }
   };

   struct ConfigInt
   {
      const fstring key;
      int value;

      operator const fstring() { return key; }
   };

   struct ConfigFloat
   {
      const fstring key;
      float value;

      operator const fstring() { return key; }
   };

   struct ConfigBool
   {
      const fstring key;
      const boolean32 value;
      operator const fstring() { return key; }
   };

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
      IInstancesSupervisor* CreateInstanceBuilder(IMeshBuilderSupervisor& builder, IRenderer& renderer, IPublisher& publisher);
   }

   ROCOCOAPI IPlayerSupervisor
   {
      virtual void Free() = 0;
      virtual IPlayer* GetPlayer(int32 index) = 0;
      virtual void Update(const IUltraClock& clock) = 0;
   };

   IPlayerSupervisor* CreatePlayerSupervisor(Rococo::Events::IPublisher& publisher);

   struct Key
   {
      const wchar_t* KeyName;
      bool isPressed;
   };

   ROCOCOAPI IKeyboardSupervisor: public IKeyboard
   {
      virtual const wchar_t* GetAction(const wchar_t* keyName) = 0;
      virtual Key GetKeyFromEvent(const KeyboardEvent& ke) = 0;
      virtual void Free() = 0;
   };

   IKeyboardSupervisor* CreateKeyboardSupervisor();

   ROCOCOAPI IConfigSupervisor: public IConfig
   {
      virtual const wchar_t* GetText(const wchar_t* name) const = 0;
      virtual void Free() = 0;
   };

   IConfigSupervisor* CreateConfig();

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

      extern EventId OnPlayerAction;

      struct OnPlayerActionEvent : public Event
      {
         OnPlayerActionEvent() : Event(OnPlayerAction) {}
         const wchar_t* Name;
         bool start;
      };

      extern EventId OnPlayerDelta;

      struct OnPlayerDeltaEvent : public Event
      {
         OnPlayerDeltaEvent() : Event(OnPlayerDelta) {}
         const wchar_t* Name;
         float delta;
      };

      extern EventId OnPlayerTryMove;

      struct OnPlayerTryMoveEvent : public Event
      {
         OnPlayerTryMoveEvent() : Event(OnPlayerTryMove) {}
         ID_ENTITY playerEntityId;
         float fowardDelta;
         float straffeDelta;
      };
   }
  
   bool QueryYesNo(Windows::IWindow& ownerWindow, const wchar_t* message);

   struct Cosmos
   {
      IConfigSupervisor& config;
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
      IPlayerSupervisor& players;
      IKeyboardSupervisor& keyboard;
   };

   IApp* CreateHVApp(Cosmos& e);
   void RunEnvironmentScript(Cosmos& e, const wchar_t* name);
}


#endif