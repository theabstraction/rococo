#include <rococo.types.h>
#include "dystopia.h"

#include <Windows.h>

namespace Dystopia
{
	using namespace Rococo;

	struct Key
	{
		wchar_t keyName[24];
		uint32 virtualKeycode;
		uint32 charCode;
	};

	class KeyboardMap : public IKeyboardSupervisor
	{
	public:
		virtual void Free() { delete this; }

		Key keys[256];

		KeyboardMap()
		{
			auto hKL = GetKeyboardLayout(0);

			for (uint32 scancode = 0; scancode < 256; ++scancode)
			{
				uint32 vcode = MapVirtualKeyExW(scancode, MAPVK_VSC_TO_VK_EX, hKL);
				keys[scancode].virtualKeycode = vcode;
				keys[scancode].keyName[0] = 0;
				keys[scancode].charCode = 0;

				if (vcode)
				{
					keys[scancode].charCode = MapVirtualKeyExW(vcode, MAPVK_VK_TO_CHAR, hKL);

					LONG code = (scancode << 16) & 0x00FF0000;
					if (0 == GetKeyNameText(code, keys[scancode].keyName, 24))
					{
						keys[scancode].keyName[0] = 0;
					}
				}
			}
		}

		virtual uint32 GetScanCode(const wchar_t* keyName) const
		{
			for (uint32 scancode = 0; scancode < 256; ++scancode)
			{
				if (_wcsicmp(keys[scancode].keyName, keyName) == 0)
				{
					return scancode;
				}
			}

			return 0;
		}
	};

	IKeyboardSupervisor* CreateKeyboardMap()
	{
		return new KeyboardMap();
	}
}