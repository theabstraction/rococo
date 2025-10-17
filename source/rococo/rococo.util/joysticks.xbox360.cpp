// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#include <rococo.ui.joystick.h>
#include <rococo.os.win32.h>
#include <Xinput.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>

#pragma comment(lib, "xinput.lib")

namespace
{
	using namespace Rococo;
	using namespace Rococo::Joysticks;
	using namespace Rococo::Strings;

	class XBOX360_Stick : public IJoystick_XBOX360_Supervisor
	{
	public:
		enum { MAX_CONTROLLERS = 4};

		XINPUT_STATE lastKnownState[MAX_CONTROLLERS] = { 0 };

		XBOX360_Stick()
		{
		}

		virtual ~XBOX360_Stick()
		{
		}

		boolean32 TryGet(uint32 index, Joystick_XBOX360& state) override
		{
			static_assert(sizeof(XINPUT_STATE) == sizeof(Joystick_XBOX360));

			auto result = XInputGetState(index, reinterpret_cast<XINPUT_STATE*>(&state));
			if (result == 0)
			{
				return true;
			}
			else
			{
				state = { 0 };
				return false;
			}
		}

		void Poll(IEventCallback<const JoystickButtonEvent>& onButtonStateChanged) override
		{
			for (DWORD i = 0; i < MAX_CONTROLLERS; i++)
			{
				XINPUT_KEYSTROKE keystroke;
				if (ERROR_SUCCESS == XInputGetKeystroke(i, 0, &keystroke))
				{
					static_assert(sizeof(keystroke) == sizeof(JoystickButtonEvent));
					static_assert((uint16)JoystickButtonEvent::Flag::DOWN == XINPUT_KEYSTROKE_KEYDOWN);
					static_assert((uint16)JoystickButtonEvent::Flag::UP == XINPUT_KEYSTROKE_KEYUP);
					static_assert((uint16)JoystickButtonEvent::Flag::REPEAT == XINPUT_KEYSTROKE_REPEAT);

					onButtonStateChanged.OnEvent(reinterpret_cast<const JoystickButtonEvent&>(keystroke));
				}
			}
		}

		void Vibrate(uint32 index, float leftStrength, float rightStrength) override
		{
			constexpr float scale = 65535.0f;
			float l = clamp(leftStrength * scale, 0.0f, scale);
			float r = clamp(rightStrength * scale, 0.0f, scale);

			uint16 u16L = (uint16)l;
			uint16 u16R = (uint16)r;

			XINPUT_VIBRATION v{ u16L, u16R };
			XInputSetState(index, &v);
		}

		void EnumerateStateAsText(const Joystick_XBOX360& x, IStringPopulator& cb) override
		{
			char text[128];
			SafeFormat(text, "Thumbs(Left): %5d %5d", x.thumbLX, x.thumbLY);
			cb.Populate(text);

			SafeFormat(text, "Thumbs(Right): %5d %5d", x.thumbRX, x.thumbRY);
			cb.Populate(text);

			SafeFormat(text, "Triggers %3.3u %3.3u", x.leftTrigger, x.rightTrigger);
			cb.Populate(text);

			const auto& b = x.buttons.button;

			SafeFormat(text, "A: %d, B: %d, X: %d, Y: %d", b.A, b.B, b.X, b.Y);
			cb.Populate(text);

			SafeFormat(text, "Up: %d, Down: %d, Left: %d, Right: %d", b.up, b.down, b.left, b.right);
			cb.Populate(text);

			SafeFormat(text, "Start: %d, Back: %d", b.start, b.back);
			cb.Populate(text);

			SafeFormat(text, "Thumb Buttons: %d %d", b.left_thumb, b.right_thumb);
			cb.Populate(text);

			SafeFormat(text, "Shoulders: %d %d", b.left_shoulder, b.right_shoulder);
			cb.Populate(text);
		}

		void Free() override
		{
			delete this;
		}

		stringmap<uint16> mapKeyNameToVkCode;

		void CacheKeycodes()
		{
			if (mapKeyNameToVkCode.empty())
			{
				mapKeyNameToVkCode["A"] = VK_PAD_A;
				mapKeyNameToVkCode["B"] = VK_PAD_B;
				mapKeyNameToVkCode["X"] = VK_PAD_X;
				mapKeyNameToVkCode["Y"] = VK_PAD_Y;
				mapKeyNameToVkCode["Shoulder.R"] = VK_PAD_RSHOULDER;
				mapKeyNameToVkCode["Shoulder.L"] = VK_PAD_LSHOULDER;
				mapKeyNameToVkCode["Trigger.R"] = VK_PAD_RTRIGGER;
				mapKeyNameToVkCode["Trigger.L"] = VK_PAD_LTRIGGER;
				mapKeyNameToVkCode["DPAD.U"] = VK_PAD_DPAD_UP;
				mapKeyNameToVkCode["DPAD.D"] = VK_PAD_DPAD_DOWN;
				mapKeyNameToVkCode["DPAD.L"] = VK_PAD_DPAD_LEFT;
				mapKeyNameToVkCode["DPAD.R"] = VK_PAD_DPAD_RIGHT;
				mapKeyNameToVkCode["Start"] = VK_PAD_START;
				mapKeyNameToVkCode["Back"] = VK_PAD_BACK;
				mapKeyNameToVkCode["Thumb.L.Press"] = VK_PAD_LTHUMB_PRESS;
				mapKeyNameToVkCode["Thumb.R.Press"] = VK_PAD_RTHUMB_PRESS;
				mapKeyNameToVkCode["Thumb.L.U"] = VK_PAD_LTHUMB_UP;
				mapKeyNameToVkCode["Thumb.L.D"] = VK_PAD_LTHUMB_DOWN;
				mapKeyNameToVkCode["Thumb.L.L"] = VK_PAD_LTHUMB_LEFT;
				mapKeyNameToVkCode["Thumb.L.R"] = VK_PAD_LTHUMB_RIGHT;
				mapKeyNameToVkCode["Thumb.L.UL"] = VK_PAD_LTHUMB_UPLEFT;
				mapKeyNameToVkCode["Thumb.L.UR"] = VK_PAD_LTHUMB_UPRIGHT;
				mapKeyNameToVkCode["Thumb.L.DR"] = VK_PAD_LTHUMB_DOWNRIGHT;
				mapKeyNameToVkCode["Thumb.L.DL"] = VK_PAD_LTHUMB_DOWNLEFT;
				mapKeyNameToVkCode["Thumb.R.U"] = VK_PAD_RTHUMB_UP;
				mapKeyNameToVkCode["Thumb.R.D"] = VK_PAD_RTHUMB_DOWN;
				mapKeyNameToVkCode["Thumb.R.L"] = VK_PAD_RTHUMB_LEFT;
				mapKeyNameToVkCode["Thumb.R.R"] = VK_PAD_RTHUMB_RIGHT;
				mapKeyNameToVkCode["Thumb.R.UL"] = VK_PAD_RTHUMB_UPLEFT;
				mapKeyNameToVkCode["Thumb.R.UR"] = VK_PAD_RTHUMB_UPRIGHT;
				mapKeyNameToVkCode["Thumb.R.DR"] = VK_PAD_RTHUMB_DOWNRIGHT;
				mapKeyNameToVkCode["Thumb.R.DL"] = VK_PAD_RTHUMB_DOWNLEFT;
			}
		}

		uint16 GetVKeyCode(cstr keyName) override
		{
			auto i = mapKeyNameToVkCode.find(keyName);
			if (i == mapKeyNameToVkCode.end())
			{
				if (mapKeyNameToVkCode.empty())
				{
					CacheKeycodes();
					return GetVKeyCode(keyName);
				}

				SetLastError((DWORD) E_INVALIDARG);
				return 0;
			}

			SetLastError(NO_ERROR);
			return i->second;
		}

		void AppendKeynames(IStringPopulator& keyNameCallback) override
		{
			CacheKeycodes();
			for (auto& i : mapKeyNameToVkCode)
			{
				keyNameCallback.Populate(i.first);
			}
		}
	};
}

namespace Rococo::Joysticks
{
	IJoystick_XBOX360_Supervisor* CreateJoystick_XBox360Proxy()
	{
		return new XBOX360_Stick();
	}
}
