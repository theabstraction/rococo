#include <rococo.mplat.h>
#include <rococo.os.win32.h>
#include <Xinput.h>
#include <rococo.strings.h>

namespace
{
	using namespace Rococo;
	using namespace Rococo::Joysticks;

	class XBOX360_Stick : public IJoystick_XBOX360_Supervisor
	{
	public:
		boolean32 TryGet(uint32 index, Joystick_XBOX360& state) override
		{
			static_assert(sizeof XINPUT_STATE == sizeof Joystick_XBOX360);
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

		void EnumerateStateAsText(const Joystick_XBOX360& x, IEventCallback<cstr>& cb) override
		{
			char text[128];
			SafeFormat(text, "Thumbs(Left): %5d %5d", x.thumbLX, x.thumbLY);
			cb.OnEvent(text);

			SafeFormat(text, "Thumbs(Right): %5d %5d", x.thumbRX, x.thumbRY);
			cb.OnEvent(text);

			SafeFormat(text, "Triggers %3.3u %3.3u", x.leftTrigger, x.rightTrigger);
			cb.OnEvent(text);

			const auto& b = x.buttons.button;

			SafeFormat(text, "A: %d, B: %d, X: %d, Y: %d", b.A, b.B, b.X, b.Y);
			cb.OnEvent(text);

			SafeFormat(text, "Up: %d, Down: %d, Left: %d, Right: %d", b.up, b.down, b.left, b.right);
			cb.OnEvent(text);

			SafeFormat(text, "Start: %d, Back: %d", b.start, b.back);
			cb.OnEvent(text);

			SafeFormat(text, "Thumb Buttons: %d %d", b.left_thumb, b.right_thumb);
			cb.OnEvent(text);

			SafeFormat(text, "Shoulders: %d %d", b.left_shoulder, b.right_shoulder);
			cb.OnEvent(text);
		}

		void Free() override
		{
			delete this;
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
