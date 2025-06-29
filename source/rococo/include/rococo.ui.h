#ifndef BLOKE_UI_H
#define BLOKE_UI_H

#ifdef _WIN32
# include <rococo.vkeys.h>
#endif

namespace Rococo
{
	namespace Script
	{
		struct IPublicScriptSystem;
	}
}

namespace Rococo
{
	// Generic OS independent window handle. For operating systems without windows, should normally be null

	struct KeyboardEvent
	{
		uint16 scanCode;
		uint16 Flags;
		uint16 Reserved;
		uint16 VKey;
		uint32 Message;
		int32 extraInfo;
		int32 unicode;

		bool IsUp() const { return (Flags & 0x0001) != 0; }
	};

	struct KeyboardEventEx: KeyboardEvent
	{
		int32 isCtrlHeld : 1;
		int32 isShiftHeld : 1;
		int32 isAltHeld : 1;
	};

	ROCOCO_INTERFACE IKeyboardSink
	{
		virtual bool OnKeyboardEvent(const KeyboardEventEx& key) = 0;
	};

#pragma pack(push,1)

	struct MouseContext
	{
		uint32 isCtrlHeld : 1;
		uint32 isShiftHeld : 1;
		uint32 isAltHeld : 1;
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

		uint32 ulRawButtons_Unused;
		int32 dx;
		int32 dy;

		Vec2i cursorPos;

		enum Flags { MouseWheel = 0x0400, RDown = 0x0004, RUp = 0x0008, LDown = 0x0001, LUp = 0x0002, MDown = 0x0010, MUp = 0x0020 };

		bool HasFlag(Flags flag) const { return (buttonFlags & flag) != 0; }
		bool IsRelative() const { return (flags & 0x0001) == 0; }

		MouseContext contextFlags;
	};

#pragma pack(pop)

	struct SourceFileSet;

	namespace IO
	{
		//	ROCOCO_API bool IsKeyPressed(int vkeyCode);
		// Safe format of clipboard contents, Truncates to buffer capacity and null terminates the string.
		ROCOCO_API void PasteFromClipboard(char* asciiBuffer, size_t capacity);
	}
}

#endif // BLOKE_UI_H