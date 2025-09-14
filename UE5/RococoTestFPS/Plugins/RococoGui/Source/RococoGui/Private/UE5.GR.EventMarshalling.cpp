#include "UE5.GR.EventMarshalling.h"
#include <Widgets/SWidget.h>
#include "RococoGuiAPI.h"

namespace Rococo
{
	uint16 GetJoystickVKey(const FName& name);
}

bool useRococoMouseAndKeyboardLogging = false;

void UseRococoMouseAndKeyboardLogging(bool value)
{
	useRococoMouseAndKeyboardLogging = value;
}

void LogRococoControlEvent(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const char* text)
{
	if (useRococoMouseAndKeyboardLogging)
	{
		custodian->Log("%s", text);
	}
}

FEventReply RouteMouseButtonDown(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	using namespace Rococo;

	try
	{
		if (!custodian)
		{
			return FEventReply(false);
		}

		MouseEvent me = { 0 };

		FKey mouseKey = ue5MouseEvent.GetEffectingButton();
		if (mouseKey == EKeys::RightMouseButton)
		{
			me.buttonFlags = MouseEvent::Flags::RDown;
			LogRococoControlEvent(custodian, "MouseButtonDown-R");
		}
		else if (mouseKey == EKeys::MiddleMouseButton)
		{
			me.buttonFlags = MouseEvent::Flags::MDown;
			LogRococoControlEvent(custodian, "MouseButtonUp-M");
		}
		else if (mouseKey == EKeys::LeftMouseButton)
		{
			me.buttonFlags = MouseEvent::Flags::LDown;
			LogRococoControlEvent(custodian, "MouseButtonUp-L");
		}
		else
		{
			LogRococoControlEvent(custodian, "MouseButtonUp-Unknown");
			return FEventReply(false);
		}

		CopySpatialInfo(me, ue5MouseEvent, geometry);

		custodian->RouteMouseEvent(me, ToContext(ue5MouseEvent));
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);;
	}

	return FEventReply(true);
}

FEventReply RouteMouseButtonUp(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	using namespace Rococo;

	try
	{
		if (!custodian)
		{
			return FEventReply(false);
		}

		MouseEvent me = { 0 };

		FKey mouseKey = ue5MouseEvent.GetEffectingButton();
		if (mouseKey == EKeys::RightMouseButton)
		{
			me.buttonFlags = MouseEvent::Flags::RUp;
			LogRococoControlEvent(custodian, "MouseButtonUp-R");
		}
		else if (mouseKey == EKeys::MiddleMouseButton)
		{
			me.buttonFlags = MouseEvent::Flags::MUp;
			LogRococoControlEvent(custodian, "MouseButtonUp-M");
		}
		else if (mouseKey == EKeys::LeftMouseButton)
		{
			me.buttonFlags = MouseEvent::Flags::LUp;
			LogRococoControlEvent(custodian, "MouseButtonUp-L");
		}
		else
		{
			LogRococoControlEvent(custodian, "MouseButtonUp-Unknown");
			return FEventReply(false);
		}

		/*
			The call stack invokes RoutePointerUpEvent with an empty widgetpath. This causes the TransformPointerEvent function to be skipped, so we don't need to correct the fullscreen mapping
			FReply FSlateApplication::RoutePointerUpEvent(const FWidgetPath & WidgetsUnderPointer, const FPointerEvent & PointerEvent)
		*/
		CopySpatialInfo_NoFullscreenCorrection(me, ue5MouseEvent, geometry);
		custodian->RouteMouseEvent(me, ToContext(ue5MouseEvent));
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);;
	}

	return FEventReply(true);
}

FEventReply RouteMouseMove(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	using namespace Rococo;

	try
	{
		if (!custodian)
		{
			return FEventReply(false);
		}

		MouseEvent me = { 0 };
		CopySpatialInfo(me, ue5MouseEvent, geometry);
		custodian->RouteMouseEvent(me, ToContext(ue5MouseEvent));
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);;
	}

	return FEventReply(true);
}

FEventReply RouteMouseWheel(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	using namespace Rococo;

	try
	{
		if (!custodian)
		{
			return FEventReply(false);
		}

		MouseEvent me = { 0 };
		me.buttonFlags = MouseEvent::Flags::MouseWheel;
		me.buttonData = (int)(ue5MouseEvent.GetWheelDelta() * 120.0f);
		CopySpatialInfo(me, ue5MouseEvent, geometry);
		custodian->RouteMouseEvent(me, ToContext(ue5MouseEvent));
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);;
	}

	return FEventReply(true);
}

#ifdef _WIN32_BABY

Rococo::uint16 GetVirtualKey(const FKeyEvent& ev)
{
	return ev.GetKeyCode();
}

#else

Rococo::uint16 GetVirtualKey(const FKeyEvent& ev)
{
	using namespace Rococo::IO::VirtualKeys;

	FName name = ev.GetKey().GetFName();

	uint16 vkCode = Rococo::GetJoystickVKey(name);

	if (vkCode != 0)
	{
		return vkCode;
	}

	static TMap<FName, uint16> keyNameToVCode;
	if (keyNameToVCode.Num() == 0)
	{
		keyNameToVCode.Add(FName("Enter"), VKCode_ENTER);
		keyNameToVCode.Add(FName("Escape"), VKCode_ESCAPE);
		keyNameToVCode.Add(FName("SpaceBar"), VKCode_SPACEBAR);
		keyNameToVCode.Add(FName("Tab"), VKCode_TAB);
		keyNameToVCode.Add(FName("BackSpace"), VKCode_BACKSPACE);
		keyNameToVCode.Add(FName("Delete"), VKCode_DELETE);
		keyNameToVCode.Add(FName("Left"), VKCode_LEFT);
		keyNameToVCode.Add(FName("Right"), VKCode_RIGHT);
		keyNameToVCode.Add(FName("Up"), VKCode_UP);
		keyNameToVCode.Add(FName("Down"), VKCode_DOWN);
		keyNameToVCode.Add(FName("C"), VKCode_C);
		keyNameToVCode.Add(FName("V"), VKCode_V);
		keyNameToVCode.Add(FName("PageUp"), VKCode_PGUP);
		keyNameToVCode.Add(FName("PageDown"), VKCode_PGDOWN);
		keyNameToVCode.Add(FName("Home"), VKCode_HOME); // -> note that it appears that Android does not support Home keys
		keyNameToVCode.Add(FName("End"), VKCode_END); // -> note that it appears that Android does not support End keys
		keyNameToVCode.Add(FName("AntiTab"), VKCode_ANTITAB); // -> note that antitab is not defined by UE5 source, you have to synthesize your own anti tab event
	}

	auto* pVkCode = keyNameToVCode.Find(name);
	return pVkCode != nullptr ? *pVkCode : 0;
}

#endif

FEventReply RouteKeyDown(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, FKeyEvent ue5KeyEvent)
{
	using namespace Rococo;

	try
	{
		if (!custodian)
		{
			return FEventReply(false);
		}

		KeyboardEventEx kex;
		memset(&kex, 0, sizeof(kex));
		kex.isAltHeld = ue5KeyEvent.IsAltDown();
		kex.isCtrlHeld = ue5KeyEvent.IsControlDown();
		kex.isShiftHeld = ue5KeyEvent.IsShiftDown();
		kex.VKey = GetVirtualKey(ue5KeyEvent);
		FName name = ue5KeyEvent.GetKey().GetFName();
		kex.scanCode = 0;
		kex.Flags = 0;
		auto charCode = ue5KeyEvent.GetCharacter();

		if (charCode < 'A' || charCode > 'Z')
		{
			kex.unicode = charCode;
		}
		else
		{
			bool shiftCaps = FSlateApplication::Get().GetModifierKeys().AreCapsLocked();
			if (kex.isShiftHeld)
			{
				shiftCaps = !shiftCaps;
			}

			kex.unicode = shiftCaps ? charCode : tolower(charCode);
		}

		custodian->RouteKeyboardEvent(kex);
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);;
	}

	return FEventReply(true);
}

FEventReply RouteKeyUp(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, FKeyEvent ue5KeyEvent)
{
	using namespace Rococo;

	try
	{
		if (!custodian)
		{
			return FEventReply(false);
		}

		KeyboardEventEx kex;
		memset(&kex, 0, sizeof(kex));
		kex.isAltHeld = ue5KeyEvent.IsAltDown();
		kex.isCtrlHeld = ue5KeyEvent.IsControlDown();
		kex.isShiftHeld = ue5KeyEvent.IsShiftDown();
		kex.VKey = GetVirtualKey(ue5KeyEvent);
		FName name = ue5KeyEvent.GetKey().GetFName();
		kex.scanCode = 0;
		kex.Flags = 1;
		kex.unicode = ue5KeyEvent.GetCharacter();
		custodian->RouteKeyboardEvent(kex);
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);;
	}

	return FEventReply(true);
}