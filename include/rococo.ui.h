#ifndef BLOKE_UI_H
#define BLOKE_UI_H

namespace Sexy
{
	namespace Script
	{
		struct IPublicScriptSystem;
	}
}

namespace Rococo
{
	// Generic OS independent window handle. For operating systems without windows, should normally be null
	struct WindowHandle
	{
		WindowHandle() : ptr(nullptr) {}
		void* ptr;
	};

	struct KeyboardEvent
	{
		uint16 scanCode;
		uint16 Flags;
		uint16 Reserved;
		uint16 VKey;
		uint32 Message;

		bool IsUp() const { return (Flags & 0x0001) != 0; }
	};

	struct MouseEvent
	{
		uint16 flags;

		union {
			uint32 buttons;
			struct {
				uint16  buttonFlags;
				uint16  buttonData;
			};
		};

		uint32 ulRawButtons;
		int32 dx;
		int32 dy;

		enum Flags { MouseWheel = 0x0400, RDown = 0x0004, RUp = 0x0008, LDown = 0x0001, LUp = 0x0002 };

		bool HasFlag(Flags flag) const { return (buttonFlags & flag) != 0; }
		bool IsRelative() const { return (flags & 0x0001) == 0; }
	};

	struct SourceFileSet;

	struct NO_VTABLE IScriptExecutionTarget
	{
		virtual void ExecuteAppScript(const wchar_t* sourceName, ICallback<Sexy::Script::IPublicScriptSystem>* onPostCompile) = 0;
	};
}

#endif // BLOKE_UI_H