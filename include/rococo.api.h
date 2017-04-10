#ifndef ROCOCO_API_H
#define ROCOCO_API_H

#include <rococo.types.h>

// The following could be done with a template, but that results in bloated error messages, and our ids are ubiquitous
#define ROCOCO_ID(DEFINED_ID_NAME,TYPE,INVALID_VALUE)										      \
struct DEFINED_ID_NAME																		            \
{																							                  \
	DEFINED_ID_NAME() : value(INVALID_VALUE) {}												      \
	explicit DEFINED_ID_NAME(TYPE _value) : value(_value) {}								      \
	TYPE value;																				               \
   static DEFINED_ID_NAME Invalid() { return DEFINED_ID_NAME(); }							   \
	size_t operator()(const DEFINED_ID_NAME& obj) const { return size_t(obj.value); }	\
   operator bool () const { return value != INVALID_VALUE; }                           \
};																							                  \
inline bool operator == (const DEFINED_ID_NAME& a, const DEFINED_ID_NAME& b) { return a.value == b.value; }				\
inline bool operator != (const DEFINED_ID_NAME& a, const DEFINED_ID_NAME& b) { return !(a == b); }                   \
inline bool operator <  (const DEFINED_ID_NAME& a, const DEFINED_ID_NAME& b) { return a.value < b.value; }

namespace DirectX
{
	struct XMFLOAT4X4;
	struct XMFLOAT4;
}

namespace Sexy
{
   struct IStringPopulator;
}

namespace Rococo
{
	typedef int64 ticks;

	ticks CpuClock();

	template<class T> struct Segment
	{
		T a;
		T b;
	};

	struct Quat;

	ROCOCO_ID(ID_BITMAP, uint64, -1)

	struct Sphere
	{
		Vec3 centre;
		float radius;
	};

	struct Gravity;
	struct Metres;
	struct Quat;

	namespace Windows
	{
		struct IWindow;
		IWindow& NoParent();
		int ShowMessageBox(IWindow& window, const wchar_t* text, const wchar_t* caption, uint32 uType);
	}

	namespace Visitors
	{
		enum CheckState : int32;
		struct TREE_NODE_ID;
		struct IUITree;
		struct IUIList;
      struct ITreePopulator;
      struct IListPopulator;
	}

	void ShowErrorBox(Windows::IWindow& parent, IException& ex, const wchar_t* caption);

	struct IRenderer;
	struct IInstallation;
	struct IOS;
	struct IRenderContext;
	struct IBuffer;

   ROCOCOAPI IUltraClock
   {
      virtual ticks Hz() const = 0;			// Number of ticks per seconds
      virtual ticks FrameStart() const = 0;	// The time of the current render frame
      virtual ticks Start() const = 0;		// The time at which the mainloop started
      virtual ticks FrameDelta() const = 0;	// The time between the previous frame and the current frame.
      virtual Seconds DT() const = 0; // Get a sanitized timestep in seconds
   };

	struct IStringBuilder;
	struct IRandom;
	struct KeyboardEvent;
	struct MouseEvent;

	void SplitString(const wchar_t* text, size_t length, const wchar_t* seperators, IEventCallback<const wchar_t*>& onSubString);

	struct IGuiRenderContext;

	ROCOCOAPI IBitmapCache
	{
		virtual ID_BITMAP Cache(const wchar_t* resourceName) = 0;
		virtual void DrawBitmap(IGuiRenderContext& gc, const GuiRect& targetRect, ID_BITMAP id) = 0;
		virtual void SetCursorBitmap(ID_BITMAP id, Vec2i hotspotOffset) = 0;
		virtual void SetMeshBitmap(IRenderContext& rc, ID_BITMAP id) = 0;
	};

	ROCOCOAPI IBitmapCacheSupervisor : public IBitmapCache
	{
		virtual void Free() = 0;
	};

	IBitmapCacheSupervisor* CreateBitmapCache(IInstallation& installation, IRenderer& renderer);

	namespace Post
	{
		struct IPostbox;
	}

	// ID_MESH are artist defined indices. The artist chooses a number to associate with a mesh.
	// Rough convention:
	//    id  0          invalid
	//    ids 1          to 0x1FFFFFFF are defined in script files and level editors
	//    ids 0x20000000 to 0x20FFFFFF are procedurally generated paths, roads and rivers created in C++
	//    ids 0x21000000 t0 0x21FFFFFF are procedurally generated houses created in C++.
	//    ids 0x40000000 to 0x41000000 are gameplay generated meshes such as explosions created in C++.
	//	  ids 0x41000001 to 0x42000000 are skeletal animation meshes
   ROCOCO_ID(ID_MESH, int32, 0)

   // ID_SYS_MESH are renderer defined indices that are generated when meshes are loaded into the renderer
   ROCOCO_ID(ID_SYS_MESH, size_t, (size_t)-1)

   ROCOCO_ID(ID_WIDGET, int32, 0);
   ROCOCO_ID(ID_UI_EVENT_TYPE, int64, 0);

	bool operator == (const fstring& a, const fstring& b);

	uint32 FastHash(const wchar_t* text);

	fstring to_fstring(const wchar_t* const msg);

	inline size_t operator "" _megabytes(size_t mb)
	{
		return mb * 1024 * 1024;
	}

	inline size_t operator "" _kilobytes(size_t kb)
	{
		return kb * 1024;
	}

	typedef const Matrix4x4& cr_m4x4;

	ROCOCOAPI IDebugControl
	{
		virtual void Continue() = 0;
		virtual void StepOut() = 0;
		virtual void StepOver() = 0;
		virtual void StepNextSymbol() = 0;
		virtual void StepNext() = 0;
      virtual void PopulateAPITree(Visitors::IUITree& tree) = 0;
      virtual void RefreshAtDepth(int stackDepth) = 0; // Refresh source and disassembly, but do not refresh the stack view
	};

	ROCOCOAPI ILogger
	{
      virtual void AddLogSection(RGBAb colour, const wchar_t* format, ...) = 0;
      virtual void ClearLog() = 0;
		virtual int Log(const wchar_t* format, ...) = 0;
	};

   namespace IO
   {
      struct IUnicode16Writer;
      bool ChooseDirectory(wchar_t* name, size_t capacity);
      void ForEachFileInDirectory(const wchar_t* directory, IEventCallback<const wchar_t*>& onFile);
   }

   namespace OS
   {
      void ShutdownApp();
   }

   struct IDebuggerWindow;
   
   ROCOCOAPI IDebuggerPopulator
   {
      virtual void Populate(IDebuggerWindow& debugger) = 0;
   };

	ROCOCOAPI IDebuggerWindow: public ILogger
	{
     
		virtual void AddDisassembly(RGBAb colour, const wchar_t* text, RGBAb bkColor = RGBAb(255,255,255), bool bringToView = false) = 0;
      virtual void BeginStackUpdate() = 0;
      virtual void EndStackUpdate() = 0;
      virtual void InitDisassembly(size_t codeId) = 0;
      virtual void AddSourceCode(const wchar_t* name, const wchar_t* sourceCode) = 0;
		virtual void Free() = 0;
		virtual Windows::IWindow& GetDebuggerWindowControl() = 0;
      virtual void PopulateStackView(Visitors::ITreePopulator& populator) = 0;
      virtual void PopulateRegisterView(Visitors::IListPopulator& populator) = 0;
      virtual void Run(IDebuggerPopulator& populator, IDebugControl& control) = 0;
      virtual void SetCodeHilight(const wchar_t* source, const Vec2i& start, const Vec2i& end, const wchar_t* message) = 0;
		virtual void ShowWindow(bool show, IDebugControl* debugControl) = 0;   
	};
}

namespace Sexy
{
	namespace Script
	{
		struct IPublicScriptSystem;
	}

	typedef size_t ID_BYTECODE;

	namespace Sex
	{
		struct ISParserTree;
		struct ISExpression;
		typedef const ISExpression& cr_sex;
		class ParseException;
		
	}

	namespace VM
	{
		struct IVirtualMachine;
	}

	enum EXECUTERESULT;
}

namespace Rococo
{
	struct ArchetypeCallback
	{
		Sexy::ID_BYTECODE byteCodeId;
		const uint8* callerSF;
	};

	ROCOCOAPI ISourceCache
	{
		virtual Sexy::Sex::ISParserTree* GetSource(const wchar_t* resourceName) = 0;
		virtual void Free() = 0;
		virtual void Release(const wchar_t* resourceName) = 0;
	};

	struct IArgStack
	{
		virtual void PushInt32(int32 value) = 0;
		virtual void PushInt64(int64 value) = 0;
		virtual void PushPointer(void * value) = 0;
	};

	struct IOutputStack
	{
		virtual int32 PopInt32() = 0;
	};

	struct IArgEnumerator
	{
		virtual void PushArgs(IArgStack& args) = 0;
		virtual void PopOutputs(IOutputStack& args) = 0;
	};

	void DebuggerLoop(Sexy::Script::IPublicScriptSystem &ss, IDebuggerWindow& debugger);

	struct ScriptCompileArgs
	{
		Sexy::Script::IPublicScriptSystem& ss;
	};

	void InitSexyScript(Sexy::Sex::ISParserTree& mainModule, IDebuggerWindow& debugger, Sexy::Script::IPublicScriptSystem& ss, ISourceCache& sources, IEventCallback<ScriptCompileArgs>& onCompile);
	void ExecuteFunction(Sexy::ID_BYTECODE bytecodeId, IArgEnumerator& args, Sexy::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger);
	void ExecuteFunction(const wchar_t* name, IArgEnumerator& args, Sexy::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger);
	int32 ExecuteSexyScript(Sexy::Sex::ISParserTree& mainModule, IDebuggerWindow& debugger, Sexy::Script::IPublicScriptSystem& ss, ISourceCache& sources, int32 param, IEventCallback<ScriptCompileArgs>& onCompile);
	ISourceCache* CreateSourceCache(IInstallation& installation);

	void ThrowSex(Sexy::Sex::cr_sex s, const wchar_t* format, ...);
	void ScanExpression(Sexy::Sex::cr_sex s, const wchar_t* hint, const char* format, ...);
	void ValidateArgument(Sexy::Sex::cr_sex s, const wchar_t* arg);

	Vec3 GetVec3Value(Sexy::Sex::cr_sex sx, Sexy::Sex::cr_sex sy, Sexy::Sex::cr_sex sz);
	RGBAb GetColourValue(Sexy::Sex::cr_sex s);
	Quat GetQuat(Sexy::Sex::cr_sex s);

	void LogParseException(Sexy::Sex::ParseException& ex, IDebuggerWindow& logger);

	fstring GetAtomicArg(Sexy::Sex::cr_sex s);
}

namespace Rococo
{
	ROCOCOAPI IRandom
	{
		virtual uint32 operator()() = 0;
		virtual void Seed(uint32 value) = 0;
	};

   namespace Events
   {
      typedef int32 EventHash;

      class EventId
      {
      private:
         const wchar_t* name;
         EventHash hash;
         mutable EventHash id{ 0 };

      public:
         EventId(const wchar_t* const _name, EventHash _hash) : name(_name), hash(_hash) { }
         EventId(const EventId& src) : name(src.name), hash(src.hash), id(src.id) {}
         EventId operator = (const EventId& src) = delete;

         const wchar_t* Name() const  { return name; }
         operator const wchar_t*() const { return name; }

         operator EventHash() const;
      };

      EventId operator "" _event(const wchar_t* name, size_t len);

      struct Event
      {
         EventId id;
         int64 sizeInBytes;
         operator EventId() const { return id; }

         Event(EventId _id) : id(_id) {}
      };

      inline bool operator == (const EventId& a, const EventId& b)
      {
         return (EventHash)a == (EventHash)b;
      }

      inline bool operator != (const EventId& a, const EventId& b)
      {
         return !(a == b);
      }

      ROCOCOAPI IObserver
      {
         virtual void OnEvent(Event& ev) = 0;
      };

      ROCOCOAPI IPublisher
      {
         virtual void Attach(IObserver* observer) = 0;
         virtual void Detach(IObserver* observer) = 0;
         virtual void Publish(Event& ev) = 0;
      };

      ROCOCOAPI IPublisherSupervisor : public IPublisher
      {
         virtual void Free() = 0;
      };

      IPublisherSupervisor* CreatePublisher();

      template<class T> inline void Publish(IPublisher& publisher, T& ev)
      {
         ev.sizeInBytes = sizeof(T);
         publisher.Publish(ev);
      }

      void ThrowBadEvent(const Event& ev);

      template<class T> inline T& As(Event& ev)
      {
         T& t = static_cast<T&>(ev);
         if (t.sizeInBytes != sizeof(T)) ThrowBadEvent(ev);
         return t;
      }

      namespace Input
      {
         enum MouseFlags : int32
         {
            MouseFlags_LDown = 0x0001,
            MouseFlags_LUp = 0x0002,
            MouseFlags_RDown = 0x0004,
            MouseFlags_RUp = 0x0008,
            MouseFlags_MDown = 0x0010,
            MouseFlags_MUp = 0x0020,
            MouseFlags_Wheel = 0x0400,
            MouseFlags_LRMW = 0x043F
         };

         extern EventId OnMouseMoveRelative;

         struct OnMouseMoveRelativeEvent : public Event
         {
            OnMouseMoveRelativeEvent() : Event(OnMouseMoveRelative) {}
            int32 dx;
            int32 dy;
            int32 dz;
         };

         extern EventId OnMouseChanged;

         struct OnMouseChangedEvent : public Event
         {
            OnMouseChangedEvent() : Event(OnMouseChanged) {}
            int32 flags;
         };
      }
   } // Events

	namespace Random
	{
		uint32 Next(IRandom& rng);
		uint32 Next(IRandom& rng, uint32 modulus);
		void Seed(IRandom& rng, uint32 value = 0);
		float NextFloat(IRandom& rng, float minValue, float maxValue);
		Vec3 NextNormalVector(IRandom& rng);

		class RandomMT : public IRandom
		{
		public:
			RandomMT(uint32 seed = 0);
			~RandomMT();
			virtual uint32 operator()();
			virtual void Seed(uint32 index);
		private:
			struct alignas(16) OpaqueBlock
			{
				uint8 opaque[6*1024];
			} block;
		};
	}
}

#endif