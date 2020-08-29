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

namespace Rococo
{
	struct Quat;

	ROCOCO_ID(ID_BITMAP, uint64, -1);
	ROCOCO_ID(ID_FONT, int32, -1);
	ROCOCO_ID(ID_TEXTURE, size_t, -1)

	struct Sphere
	{
		Vec3 centre;
		float radius;
	};

	struct Platform;
	struct Gravity;
	struct Metres;
	struct Quat;

	struct StringKeyValuePairArg
	{
		cstr key;
		cstr value;
	};

#ifdef _WIN32
	namespace Windows
	{
		struct IWindow; // defined in rococo.windows.h which provides HWND
	}
#else
	ROCOCO_ID(ID_OSWINDOW, uint64, 0);

	namespace Windows
	{
		struct IWindow
		{
			virtual operator ID_OSWINDOW() const = 0;
		};
	}
#endif

	int32 Format(U8FilePath& path, cstr format, ...);
	int32 Format(WideFilePath& path, const wchar_t* format, ...);

	void Assign(U8FilePath& dest, const wchar_t* wideSrc);
	void Assign(WideFilePath& dest, const char* src);

	enum { MAX_FQ_NAME_LEN = 127 };
	void ValidateFQNameIdentifier(cstr fqName);

	template<typename COLOUR_STRUCT>
	ROCOCOAPI IImagePopulator
	{
		virtual void OnImage(const COLOUR_STRUCT* pixelBuffer, int width, int height) = 0;
	};

	namespace Fonts
	{
		ROCOCOAPI IFontGlyphBuilder
		{
			virtual void AddGlyph(wchar_t code) = 0;
			virtual void AddGlyph(unsigned char asciiValue) = 0;
		};

		struct ArrayGlyph
		{
			int32 A; // dx cursor rollback before rendering
			int32 B; // Width in pixels
			int32 C; // dx cursor advance after rendering
			int32 Index; // enumeration index
		};

		struct GlyphDesc
		{
			const ArrayGlyph& glyph;
			const char32_t charCode;

			GlyphDesc(const ArrayGlyph& _glyph, const char32_t _charCode) : glyph(_glyph), charCode(_charCode) {}
		};

		struct ArrayFontMetrics
		{
			int32 height;
			int32 ascent;
			int32 descent;
			int32 italic;
			int32 weight;
			int32 internalLeading;
			int32 imgWidth;
			int32 imgHeight;
		};

		typedef uint8 GRAYSCALE;

		ROCOCOAPI IArrayFont
		{
			virtual int32 NumberOfGlyphs() const = 0;

			virtual const ArrayGlyph& operator[](unsigned char) = 0;
			virtual const ArrayGlyph& operator[](wchar_t) = 0;
			virtual const ArrayGlyph& operator[](char32_t) = 0;

			virtual const ArrayFontMetrics & Metrics() const = 0;

			/* Enumerate the glyphs in the registered glyphs for this font
			 The callback gives the ABC widths of each glyph as well as the UNICODE value
			*/
			virtual void ForEachGlyph(IEventCallback<const GlyphDesc>& cb) = 0;

			/*
			 GenerateImage will work for characters outside the glyph set, but this is abuse of the API
			 In general the consumer uses ForEachGlyph to iterate over all registered glyphs
			 then calls GenerateImage in the callback to get an image, and saves it to memory or disk
			*/
			virtual void GenerateImage(const char32_t charCode, IImagePopulator<GRAYSCALE>& populator) = 0;
		};

		ROCOCOAPI IArrayFontSupervisor : IArrayFont
		{
			virtual void Free() = 0;
		};

		ROCOCOAPI IArrayFontSet
		{
			virtual void Populate(IFontGlyphBuilder & builder);
		};

		struct FontSpec
		{
			cstr fontName;
			int32 height;
			int32 weight;
			boolean32 italic;
		};

		/*
		  Implement your own IArrayFontSent. The populate function should call IFontGlyphBuilder::AddGlyph
		  for every UNICODE value you want in the font image set. Since this can be rather expensive in
		  memory for the full UNICODE set, I suggest you restrict glyphs to ASCII 32 to 126 and add whatever
		  extra characters your particular application needs.
		*/
		IArrayFontSupervisor* CreateOSFont(IArrayFontSet& glyphSet, const FontSpec& spec);
	}

	namespace Windows
	{
		IWindow& NoParent();
		int ShowMessageBox(IWindow& window, cstr text, cstr caption, uint32 uType);
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

	struct IRenderer;
	struct IInstallation;
	struct IOS;
	struct IRenderContext;
	struct IBuffer;

	ROCOCOAPI IUltraClock
	{
	   virtual OS::ticks FrameStart() const = 0;	// The time of the current render frame
	   virtual OS::ticks Start() const = 0;		// The time at which the mainloop started
	   virtual OS::ticks FrameDelta() const = 0;	// The time between the previous frame and the current frame.
	   virtual Seconds DT() const = 0; // Get a sanitized timestep in seconds
	};

	struct IRandom;
	struct KeyboardEvent;
	struct MouseEvent;

	void SplitString(cstr text, size_t length, cstr seperators, IEventCallback<cstr>& onSubString);

	void GetTimestamp(char str[26]);

	struct IGuiRenderContext;

	ROCOCOAPI IBitmapCache
	{
		virtual ID_BITMAP Cache(cstr resourceName) = 0;
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
	ROCOCO_ID(ID_SYS_MESH, size_t, -1)

	ROCOCO_ID(ID_WIDGET, int32, 0);
	ROCOCO_ID(ID_UI_EVENT_TYPE, int64, 0);

	bool operator == (const fstring& a, const fstring& b);

	uint32 FastHash(cstr text);

	fstring to_fstring(cstr const msg);

#ifdef _WIN32
	typedef size_t lsize_t;
#else
	typedef unsigned long long lsize_t;
#endif

	inline lsize_t operator "" _megabytes(lsize_t mb)
	{
		return mb * 1024 * 1024;
	}

	inline lsize_t operator "" _kilobytes(lsize_t kb)
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
	    virtual void RefreshAtDepth(int stackDepth) = 0; // Refresh source and disassembly, but do not refresh the CallStack view
	};

	ROCOCOAPI ILogger
	{
		virtual void AddLogSection(RGBAb colour, cstr format, ...) = 0;
		virtual void ClearLog() = 0;
		virtual int Log(cstr format, ...) = 0;
	};

	namespace IO
	{
		struct IUnicode16Writer;
		bool ChooseDirectory(char* name, size_t capacity);

		struct FileItemData
		{
			const wchar_t* fullPath;
			const wchar_t* containerRelRoot;
			const wchar_t* itemRelContainer;
			bool isDirectory;
		};
		void ForEachFileInDirectory(const wchar_t* directory, IEventCallback<FileItemData>& onFile, bool recurse);

		struct FileAttributes
		{
			uint64 fileLength;
			char timestamp[24];
		};
		
		bool TryGetFileAttributes(const wchar_t* sysPath, FileAttributes& attr);
	}

	namespace OS
	{
		ROCOCOAPI IAppControl
		{
			virtual bool IsRunning() const = 0;
			virtual void ShutdownApp() = 0;
		};

		ROCOCOAPI IAppControlSupervisor : public IAppControl
		{
			virtual void Free() = 0;
		};

		IAppControlSupervisor* CreateAppControl();

		void BeepWarning();
		void BuildExceptionString(char* buffer, size_t capacity, IException& ex, bool appendStack);
		void CopyExceptionToClipboard(IException& ex);
		void EditImageFile(Rococo::Windows::IWindow& window, const wchar_t* sysPath);

		struct IThreadControl;

		ROCOCOAPI IThreadJob
		{
			virtual uint32 RunThread(IThreadControl& control) = 0;
		};

		ROCOCOAPI IThreadControl : public ILock
		{
			virtual bool IsRunning() const = 0;
			virtual void Resume() = 0;
			virtual void SetRealTimePriority() = 0;
			virtual void SleepUntilAysncEvent(uint32 milliseconds) = 0;
			virtual cstr GetErrorMessage() const = 0;
		};

		ROCOCOAPI IThreadSupervisor : public IThreadControl
		{
			virtual void Free() = 0;
		};

		IThreadSupervisor* CreateRococoThread(IThreadJob* thread, uint32 stacksize);

		void* AllocBoundedMemory(size_t nBytes);
		void FreeBoundedMemory(void* pMemory);

		enum TargetDirectory
		{
			TargetDirectory_UserDocuments = 0,
			TargetDirectory_Root
		};

		void SaveAsciiTextFile(TargetDirectory target, const wchar_t* filename, const fstring& text);
	}

	struct IDebuggerWindow;

	ROCOCOAPI IDebuggerPopulator
	{
	   virtual void Populate(IDebuggerWindow& debugger) = 0;
	};

	struct MenuCommand
	{
		cstr name;
		const uint8* buffer;
		size_t len;
	};

	ROCOCOAPI IDebuggerWindow : public ILogger
	{
		virtual void AddDisassembly(RGBAb colour, cstr text, RGBAb bkColor = RGBAb(255,255,255), bool bringToView = false) = 0;
		virtual void InitDisassembly(size_t codeId) = 0;
		virtual void AddSourceCode(cstr name, cstr sourceCode) = 0;
		virtual void Free() = 0;
		virtual Windows::IWindow& GetDebuggerWindowControl() = 0;
		virtual void PopulateMemberView(Visitors::ITreePopulator& populator) = 0;
		virtual void PopulateRegisterView(Visitors::IListPopulator& populator) = 0;
		virtual void PopulateVariableView(Visitors::IListPopulator& populator) = 0;
		virtual void PopulateCallStackView(Visitors::IListPopulator& populator) = 0;
		virtual void Run(IDebuggerPopulator& populator, IDebugControl& control) = 0;
		virtual void SetCodeHilight(cstr source, const Vec2i& start, const Vec2i& end, cstr message) = 0;
		virtual void ShowWindow(bool show, IDebugControl* debugControl) = 0;
	};
}

namespace Rococo
{
	namespace Script
	{
		struct IPublicScriptSystem;
	}

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

	namespace Cute
	{
		struct IMasterWindowFactory;
		struct IWindowSupervisor;
	}

	enum EXECUTERESULT : int32;
}

namespace Rococo
{
	namespace Script
	{
		struct ArchetypeCallback
		{
			ID_BYTECODE byteCodeId;
			const uint8* callerSF;
		};
	}

	struct IMathsVenue;

	ROCOCOAPI ISourceCache
	{
		virtual IAllocator& Allocator() = 0;
		virtual Rococo::Sex::ISParserTree* GetSource(cstr resourceName) = 0;
		virtual void Free() = 0;
		virtual void Release(cstr resourceName) = 0;
		virtual void ReleaseAll() = 0;
		virtual IMathsVenue* Venue() = 0;
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

	void DebuggerLoop(Rococo::Script::IPublicScriptSystem &ss, IDebuggerWindow& debugger);

	struct ScriptCompileArgs
	{
		Rococo::Script::IPublicScriptSystem& ss;
	};

	struct ScriptPerformanceStats
	{
		OS::ticks loadTime;
		OS::ticks compileTime;
		OS::ticks executeTime;
	};

	void InitSexyScript(Rococo::Sex::ISParserTree& mainModule, IDebuggerWindow& debugger, Rococo::Script::IPublicScriptSystem& ss, ISourceCache& sources, IEventCallback<ScriptCompileArgs>& onCompile);
	void ExecuteFunction(Rococo::ID_BYTECODE bytecodeId, IArgEnumerator& args, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace);
	void ExecuteFunction(cstr name, IArgEnumerator& args, Rococo::Script::IPublicScriptSystem& ss, IDebuggerWindow& debugger, bool trace);
	int32 ExecuteSexyScript(ScriptPerformanceStats& stats, Rococo::Sex::ISParserTree& mainModule, IDebuggerWindow& debugger, Rococo::Script::IPublicScriptSystem& ss, ISourceCache& sources, int32 param, IEventCallback<ScriptCompileArgs>& onCompile, bool trace);
	ISourceCache* CreateSourceCache(IInstallation& installation);

	void ThrowSex(Rococo::Sex::cr_sex s, cstr format, ...);
	void ScanExpression(Rococo::Sex::cr_sex s, cstr hint, const char* format, ...);
	void ValidateArgument(Rococo::Sex::cr_sex s, cstr arg);

	Vec3 GetVec3Value(Rococo::Sex::cr_sex sx, Rococo::Sex::cr_sex sy, Rococo::Sex::cr_sex sz);
	RGBAb GetColourValue(Rococo::Sex::cr_sex s);
	Quat GetQuat(Rococo::Sex::cr_sex s);

	void LogParseException(Rococo::Sex::ParseException& ex, IDebuggerWindow& logger);

	fstring GetAtomicArg(Rococo::Sex::cr_sex s);
}

namespace Rococo
{
	ROCOCOAPI IRandom
	{
		virtual uint32 operator()() = 0;
		virtual void Seed(uint32 value) = 0;
	};


	struct KeyboardEvent;
	struct MouseEvent;

	namespace Events
	{
		typedef uint32 EventHash;

		struct EventIdRef
		{
			const char* name;
			mutable EventHash hashCode;
		};

		struct EventArgs
		{
			int64 sizeInBytes;
		};

		template<class T> struct TEventArgs : public EventArgs
		{
			T value;
			operator T () { return value; }
		};

		template<class T,class U> struct T2EventArgs : public EventArgs
		{
			T value1;
			U value2;
		};

		class IPublisher;

		struct Event
		{
			IPublisher& publisher;
			EventArgs& args;
			EventIdRef id;
		};
	}

	inline Events::EventIdRef operator "" _event(cstr name, size_t len)
	{
		return Events::EventIdRef{ name, 0 };
	}

	namespace Events
	{
		// Used by GUI panels to request an upate to a label or field just before rendering.
		// A request is sent out by the gui panel, and the formatter object responds with the same id, but setting isRequested to false
		struct TextOutputEvent : EventArgs
		{
			bool isGetting;
			char text[128];
		};

		struct RouteKeyboardEvent : EventArgs
		{
			const KeyboardEvent* ke;
			bool consume;
		};

		struct RouteMouseEvent : EventArgs
		{
			const MouseEvent* me;
			Vec2i absTopleft;
		};

		struct ScreenResizeEvent : EventArgs
		{
			GuiRect bounds;
		};

		struct VisitorItemClickedEvent : EventArgs
		{
			cstr key;
			cstr value;
		};

		ROCOCOAPI IObserver
		{
		   virtual void OnEvent(Event& ev) = 0;
		};

		class ROCOCO_NO_VTABLE IPublisher
		{
		private:
			virtual void RawPost(const EventArgs& ev, const EventIdRef& id, bool isLossy) = 0;
			virtual void RawPublish(EventArgs& ev, const EventIdRef& id) = 0;
		public:
			virtual bool Match(const Event& ev, const EventIdRef& ref) = 0;
			virtual EventIdRef CreateEventIdFromVolatileString(const char* volatileString) = 0;
			virtual void Deliver() = 0;
			virtual void Subscribe(IObserver* observer, const EventIdRef& eventName) = 0;
			virtual void Unsubscribe(IObserver* observer) = 0;
			virtual void ThrowBadEvent(const Event& ev) = 0;

			template<class T> inline void Post(T& ev, const EventIdRef& id, bool isLossy)
			{
				ev.sizeInBytes = sizeof(T);
				RawPost(ev, id, isLossy);
			}
			template<class T> inline void Publish(T& ev, const EventIdRef& id)
			{
				ev.sizeInBytes = sizeof(T);
				RawPublish(ev, id);
			}
		};

		inline bool operator == (const Event& ev, const EventIdRef& ref)
		{
			return ev.publisher.Match(ev, ref);
		}

		inline bool operator == (const EventIdRef& ref, const Event& ev)
		{
			return ev == ref;
		}

		ROCOCOAPI IPublisherSupervisor : public IPublisher
		{
		   virtual void Free() = 0;
		};

		IPublisherSupervisor* CreatePublisher();

		template<class T> inline T& As(Event& ev)
		{
			T& t = static_cast<T&>(ev.args);
			if (t.sizeInBytes != sizeof(T)) ev.publisher.ThrowBadEvent(ev);
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

			struct OnMouseMoveRelativeEvent : public EventArgs
			{
				int32 dx;
				int32 dy;
				int32 dz;
			};

			struct OnMouseChangedEvent : public EventArgs
			{
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
				uint8 opaque[6 * 1024];
			} block;
		};
	}
}

namespace Rococo
{
	void ExpandZoneToContain(GuiRect& rect, const Vec2i& p);
	void ExpandZoneToContain(GuiRectf& rect, const Vec2& p);

	namespace OS
	{
		void PrintDebug(const char* format, ...);
		void TripDebugger();
		void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr caption);
		bool IsDebugging();
		void BreakOnThrow(BreakFlag flag);
		void SetBreakPoints(int flags);
		ticks CpuTicks();
		ticks CpuHz();
		ticks UTCTime();
		void FormatTime(ticks utcTime, char* buffer, size_t nBytes);
		bool StripLastSubpath(wchar_t* fullpath);
		bool IsFileExistant(const wchar_t* path);
		void Format_C_Error(int errorCode, char* buffer, size_t capacity);
		int OpenForAppend(void** fp, cstr name);
		int OpenForRead(void** fp, cstr name);
		void UILoop(uint32 milliseconds);
		void ToSysPath(wchar_t* path);
		void ToUnixPath(wchar_t* path);
		void ToSysPath(char* path);
		void ToUnixPath(char* path);
		void SanitizePath(char* path);
		void SaveClipBoardText(cstr text, Windows::IWindow& window);
		bool TryGetColourFromDialog(RGBAb& colour, Windows::IWindow& window);
		cstr GetAsciiCommandLine();
		void LoadAsciiTextFile(char* data, size_t capacity, const wchar_t* filename);
		void LoadAsciiTextFile(IEventCallback<cstr>& onLoad, const wchar_t* filename);
		void GetEnvVariable(wchar_t* data, size_t capacity, const wchar_t* envVariable);
		void PollKeys(uint8 scanArray[256]);
		void MakeContainerDirectory(char* filename);
		void MakeContainerDirectory(wchar_t* filename);
		void FormatErrorMessage(char* message, size_t sizeofBuffer, int errorCode);
	}

	namespace Memory
	{
		IAllocator& CheckedAllocator();
		IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes);
	}

	template<typename T, typename U> inline bool HasFlag(T flag, U flags)
	{
		return (flags & flag) != 0;
	}

	struct GlyphData
	{
		int32 index;
		int32 a;
		uint32 b;
		int32 c;
	};
}


#endif