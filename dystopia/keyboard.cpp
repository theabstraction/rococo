#include "dystopia.h"
#include "rococo.strings.h"
#include <Windows.h>

#include <sexy.types.h>
#include <Sexy.S-Parser.h>

namespace Dystopia
{
	using namespace Rococo;
	using namespace Sexy;
	using namespace Sexy::Sex;

	struct Key
	{
		wchar_t keyName[24];
	};

	class KeyboardMap : public IKeyboardSupervisor
	{
	public:
		virtual void Free() { delete this; }

		Key keys[512];

		void SetName(const wchar_t* name, uint32 index)
		{
			SafeCopy(keys[index].keyName, name, _TRUNCATE);
		}

		void MapKeys(ISParserTree& tree)
		{
			cr_sex root = tree.Root();

			if (root.NumberOfElements() < 1)
			{
				ThrowSex(root, L"No elements in the script file");
			}

			cr_sex version = root[0];

			if (version.NumberOfElements() != 5)
			{
				ThrowSex(version, L"Expecting (' scancodes for windows <region>)");
			}

			const ISExpression* quote, *cat1, *cat2, *cat3, *region;
			ScanExpression(version, L"(' scancodes for windows <region>)", "a a a a a", &quote, &cat1, &cat2, &cat3, &region);

			ValidateArgument(*quote, L"'");
			ValidateArgument(*cat1, L"scancodes");
			ValidateArgument(*cat2, L"for");
			ValidateArgument(*cat3, L"windows");

			for (int i = 1; i < root.NumberOfElements(); ++i)
			{
				cr_sex sbinding = root[i];

				if (sbinding.NumberOfElements() != 2)
				{
					ThrowSex(sbinding, L"Expecting two elements in key binding");
				}

				cr_sex sscancode = sbinding[0];
				cr_sex sname = sbinding[1];

				if (!IsAtomic(sscancode)) ThrowSex(sscancode, L"Expecting number");
				if (!IsAtomic(sname)) ThrowSex(sname, L"Expecting name");

				int scancode = _wtoi(sscancode.String()->Buffer);
				if (scancode > 0 && scancode < 512)
				{
					SetName(sname.String()->Buffer, scancode);
				}
			}
		}

		void LoadKeys(ISourceCache& sourceCache)
		{
			try
			{
				auto tree = sourceCache.GetSource(L"!keymap.sxy");
				MapKeys(*tree);
			}
			catch (Sexy::Sex::ParseException& pex)
			{
				Vec2i p = pex.Start();
            Vec2i q = pex.End();
				Throw(pex.ErrorCode(), L"Error parsering %s\n%s: %s\n(%d,%d) to (%d,%d)\n%s\n", L"keymap.sxy", pex.Name(), pex.Message(), p.x, p.y, q.x, q.y, pex.Specimen());
			}
		}

		KeyboardMap(IInstallation& installation, ISourceCache& sourceCache)
		{
			auto hKL = GetKeyboardLayout(0);

			memset(keys, 0, sizeof(keys));

			LoadKeys(sourceCache);

			for (uint32 scancode = 0; scancode < 512; ++scancode)
			{
				if (!keys[scancode].keyName[0])
				{
					LONG code = (scancode << 16) & 0x01FF0000;
					if (0 == GetKeyNameText(code, keys[scancode].keyName, 24))
					{
						keys[scancode].keyName[0] = 0;
					}
					else
					{
						for (wchar_t* s = keys[scancode].keyName; *s != 0; ++s)
						{
							if (*s == ' ') *s = '_';
						}
					}
				}
			}

		//	WriteKeys();
		}

		void WriteKeys()
		{
			AutoFree<IStringBuilder> sb(CreateSafeStringBuilder(4096));
			sb->AppendFormat(L"\uFEFF"); // unicode stub
			sb->AppendFormat(L"(' scancodes for windows UK-GB)\n");
			for (uint32 scancode = 0; scancode < 512; ++scancode)
			{
				if (keys[scancode].keyName[0]) sb->AppendFormat(L"(%u %s)\n", scancode, keys[scancode].keyName);
			}

			HANDLE hFile = CreateFile(L"C:\\work\\rococo\\content\\keymap.sxy", GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				DWORD len;
				WriteFile(hFile, (const wchar_t*)*sb, DWORD(wcslen(*sb) * sizeof(wchar_t)), &len, nullptr);
				CloseHandle(hFile);
			}
		}

		virtual uint32 GetScanCode(const wchar_t* keyName) const
		{
			for (uint32 scancode = 0; scancode < 512; ++scancode)
			{
				if (_wcsicmp(keys[scancode].keyName, keyName) == 0)
				{
					return scancode;
				}
			}

			return 0;
		}
	};

	IKeyboardSupervisor* CreateKeyboardMap(IInstallation& installation, ISourceCache& sourceCache)
	{
		return new KeyboardMap(installation, sourceCache);
	}
}