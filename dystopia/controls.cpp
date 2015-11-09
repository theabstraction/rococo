#include "dystopia.h"
#include "human.types.h"
#include "rococo.maths.h"
#include "rococo.ui.h"

#include <sexy.types.h>
#include <Sexy.S-Parser.h>

#include <unordered_map>

#include <rococo.io.h>

namespace
{
	using namespace Dystopia;
	using namespace Rococo;
	using namespace Sexy::Sex;
	using namespace Sexy;

	class ControlMapper: public IControlsSupervisor
	{
	private:
		AutoFree<IKeyboardSupervisor> keyboard;

		IInstallation& installation;
		ISourceCache& sourceCache;

		struct ActionBinding
		{
			ActionMapType type;
			bool isVector;
		};

		struct MapKeyToAction
		{
			ActionBinding action[512];
		} mapKeyToAction;

		enum MouseControl
		{
			MouseControl_LButton = 0,
			MouseControl_RButton,
			MouseControl_MButton,
			MouseControl_Wheel,
			MouseControl_Cursor,
			MouseControl_NumberOfControls
		};

		struct MapMouseControlToAction
		{
			ActionBinding action[MouseControl_NumberOfControls];
		} mapMouseControlToAction;

		std::unordered_map<std::wstring, MouseControl> mapNameToMouseControl;

		std::unordered_map<std::wstring, ActionBinding> mapNameToAction;
	public:
		ControlMapper(IInstallation& _installation, ISourceCache& _sourceCache) :
			installation(_installation),
			keyboard(CreateKeyboardMap(_installation, _sourceCache)),
			sourceCache(_sourceCache)
		{
			mapNameToMouseControl[L"LButton"] = MouseControl_LButton;
			mapNameToMouseControl[L"RButton"] = MouseControl_RButton;
			mapNameToMouseControl[L"MButton"] = MouseControl_MButton;
			mapNameToMouseControl[L"Wheel"] = MouseControl_Wheel;
			mapNameToMouseControl[L"Cursor"] = MouseControl_Cursor;

			memset(&mapKeyToAction, 0, sizeof(mapKeyToAction));
		}

		virtual void Free()
		{
			delete this;
		}

		virtual void MapMouseEvent(const MouseEvent& me, IEventCallback<ActionMap>& onAction)
		{
			if (me.HasFlag(MouseEvent::LDown))
			{
				ActionMap arg;
				arg.isActive = true;
				arg.type = mapMouseControlToAction.action[MouseControl_LButton].type;
				if (arg.type != ActionMapTypeWait) onAction.OnEvent(arg);
			}
			if (me.HasFlag(MouseEvent::LUp))
			{
				ActionMap arg;
				arg.isActive = false;
				arg.type = mapMouseControlToAction.action[MouseControl_LButton].type;
				if (arg.type != ActionMapTypeWait) onAction.OnEvent(arg);
			}
			if (me.HasFlag(MouseEvent::RDown))
			{
				ActionMap arg;
				arg.isActive = true;
				arg.type = mapMouseControlToAction.action[MouseControl_RButton].type;
				if (arg.type != ActionMapTypeWait) onAction.OnEvent(arg);
			}
			if (me.HasFlag(MouseEvent::RUp))
			{
				ActionMap arg;
				arg.isActive = false;
				arg.type = mapMouseControlToAction.action[MouseControl_RButton].type;
				if (arg.type != ActionMapTypeWait) onAction.OnEvent(arg);
			}
			if (me.HasFlag(MouseEvent::MDown))
			{
				ActionMap arg;
				arg.isActive = true;
				arg.type = mapMouseControlToAction.action[MouseControl_MButton].type;
				if (arg.type != ActionMapTypeWait) onAction.OnEvent(arg);
			}
			if (me.HasFlag(MouseEvent::MUp))
			{
				ActionMap arg;
				arg.isActive = false;
				arg.type = mapMouseControlToAction.action[MouseControl_MButton].type;
				if (arg.type != ActionMapTypeWait) onAction.OnEvent(arg);
			}
			if (me.IsRelative())
			{
				ActionMap arg;
				arg.vector = { me.dx, me.dy };
				arg.type = mapMouseControlToAction.action[MouseControl_Cursor].type;
				if (arg.type != ActionMapTypeWait) onAction.OnEvent(arg);
			}
			if (me.HasFlag(MouseEvent::MouseWheel))
			{
				ActionMap arg;
				arg.vector = { 0, (int32)(int16)me.buttonData };
				arg.type = mapMouseControlToAction.action[MouseControl_Wheel].type;
				if (arg.type != ActionMapTypeWait) onAction.OnEvent(arg);
			}
		}

		virtual void MapKeyboardEvent(const KeyboardEvent& k, IEventCallback<ActionMap>& onAction)
		{
			if (k.scanCode < 512 && k.VKey != 0xFF)
			{
				ActionMap arg;
				arg.isActive = !k.IsUp();
				arg.type = mapKeyToAction.action[k.scanCode].type;

				if (arg.type != ActionMapTypeWait)
				{
					onAction.OnEvent(arg);
				}
			}
		}

		virtual void AddAction(const wchar_t* name, ActionMapType type, bool isVector)
		{
			if (!mapNameToAction.insert(std::make_pair(std::wstring(name), ActionBinding{ type, isVector })).second)
			{
				Throw(0, L"Cannot add action %s again. The action is already defined", name);
			}
		}

		void MapControl(cr_sex scontrol)
		{
			if (scontrol.NumberOfElements() > 2)
			{
				ValidateArgument(scontrol[0], L"map");

				cr_sex saction = scontrol[1];
				if (!IsAtomic(saction))
				{
					ThrowSex(scontrol, L"Expecting action name");
				}

				auto* actionName = saction.String()->Buffer;
				auto i = mapNameToAction.find(actionName);
				if (i == mapNameToAction.end())
				{
					ThrowSex(scontrol, L"Unrecognized action name");
				}

				auto& ab = i->second;

				for (int i = 2; i < scontrol.NumberOfElements(); ++i)
				{
					cr_sex skey = scontrol[i];
					if (IsAtomic(skey))
					{
						auto* key = skey.String()->Buffer;
						uint32 scancode = keyboard->GetScanCode(key);
						if (scancode != 0 && scancode < 512)
						{
							if (ab.isVector)
							{
								ThrowSex(scontrol, L"Cannot map action to a key. The action yields a vector.");
							}
							mapKeyToAction.action[scancode] = ab;
						}
						else
						{
							auto i = mapNameToMouseControl.find(key);
							if (i != mapNameToMouseControl.end())
							{
								mapMouseControlToAction.action[i->second] = ab;
							}
							else
							{
								ThrowSex(skey, L"Unregcognized control name");
							}
						}
					}
				}
			}
		}

		void MapControls(Sexy::Sex::ISParserTree& tree)
		{
			cr_sex root = tree.Root();

			if (root.NumberOfElements() < 1)
			{
				ThrowSex(root, L"No elements in the script file");
			}

			cr_sex version = root[0];

			const ISExpression* quote, *category, *filetype;
			ScanExpression(version, L"(' file.type dystopia.controls)", "a a a", &quote, &category, &filetype);

			ValidateArgument(*quote, L"'");
			ValidateArgument(*category, L"file.type");
			ValidateArgument(*filetype, L"dystopia.controls");

			for (int i = 1; i < root.NumberOfElements(); ++i)
			{
				cr_sex scontrol = root[i];
				MapControl(scontrol);
			}
		}

		virtual void LoadMapping(const wchar_t* resourcePath)
		{
			try
			{
				auto tree = sourceCache.GetSource(resourcePath);
				MapControls(*tree);
			}
			catch (Sexy::Sex::ParseException& pex)
			{
				SourcePos p = pex.Start();
				SourcePos q = pex.End();
				Throw(pex.ErrorCode(), L"Error parsering %s\n%s: %s\n(%d,%d) to (%d,%d)\nSpecimen: %s", resourcePath, pex.Name(), pex.Message(), p.X, p.Y, q.X, q.Y, pex.Specimen());
			}
		}
	};
}

namespace Dystopia
{
	IControlsSupervisor* CreateControlMapper(IInstallation& installation, ISourceCache& sourceCache)
	{
		return new ControlMapper(installation, sourceCache);
	}
}