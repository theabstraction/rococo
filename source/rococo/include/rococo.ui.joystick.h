// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.types.h>

namespace Rococo::Joysticks
{
	union XBox360ControllerButtons
	{
		struct Button
		{
			// buttons take 1 bit each, but the type of ButtonBool determines the size of 
			// word in which the bit is stored. This works for Windows at least, which
			// is all that matters here, because this code is for Windows & XBOX360 controllers only.
			typedef uint16 ButtonBool;
			ButtonBool up : 1;
			ButtonBool down : 1;
			ButtonBool left : 1;
			ButtonBool right : 1;
			ButtonBool start : 1;
			ButtonBool back : 1;
			ButtonBool left_thumb : 1;
			ButtonBool right_thumb : 1;
			ButtonBool left_shoulder : 1;
			ButtonBool right_shoulder : 1;
			ButtonBool unused_1 : 1;
			ButtonBool unused_2 : 1;
			ButtonBool A : 1;
			ButtonBool B : 1;
			ButtonBool X : 1;
			ButtonBool Y : 1;

		} button;
		uint16 allButtons;
	};

	/* field for field copy of XINPUT_STATE so we can avoid including the Xinput header
	* which amounts to thousands of lines of code to define the one struct!
	*/
	struct Joystick_XBOX360
	{
		uint32 packetNumber;
		XBox360ControllerButtons buttons;
		uint8  leftTrigger;
		uint8  rightTrigger;
		int16 thumbLX;
		int16 thumbLY;
		int16 thumbRX;
		int16 thumbRY;
	};

#pragma pack(push, 1)
	struct JoystickButtonEvent
	{
		uint16 vkCode;
		uint16 unicodeValue;
		uint16 flags;

		enum class Flag: uint16
		{
			DOWN = 0x0001,
			UP = 0x0002,
			REPEAT = 0x004
		};

		bool IsKeyDown() const
		{
			return (flags & (uint16)Flag::DOWN) != 0;
		}

		bool IsKeyUp() const
		{
			return (flags & (uint16)Flag::UP) != 0;
		}

		bool IsRepeat() const
		{
			return (flags & (uint16)Flag::REPEAT) != 0;
		}

		uint8 userIndex;
		uint8 hid;
	};
#pragma pack(pop)

	ROCOCO_INTERFACE IJoysticks
	{
		virtual uint16 GetVKeyCode(cstr keyName) = 0;
		virtual void AppendKeynames(Strings::IStringPopulator& keyNameCallback) = 0;
	};

	ROCOCO_INTERFACE IJoystick_XBOX360: IJoysticks
	{
		/* describe the gamepad state as a number of lines as text - used for debugging gamepad issues */
		virtual void EnumerateStateAsText(const Joystick_XBOX360& x, Strings::IStringPopulator & cb) = 0;

		virtual void Poll(IEventCallback<const JoystickButtonEvent>& onButtonStateChanged) = 0;

		[[nodiscard]] virtual boolean32 TryGet(uint32 index, Joystick_XBOX360 & state) = 0;

		/* Vibrate the controller, strength ranges from 0 to 1 */
		virtual void Vibrate(uint32 index, float leftStrength, float rightStrength) = 0;
	};

	ROCOCO_INTERFACE IJoystick_XBOX360_Supervisor : IJoystick_XBOX360
	{
		virtual void Free() = 0;
	};

	ROCOCO_API IJoystick_XBOX360_Supervisor* CreateJoystick_XBox360Proxy();
}
