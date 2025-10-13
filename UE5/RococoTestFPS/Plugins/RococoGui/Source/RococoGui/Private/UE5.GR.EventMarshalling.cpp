#include "UE5.GR.EventMarshalling.h"
#include <Widgets/SWidget.h>
#include "RococoGuiAPI.h"
#include <RococoGui_BPEvents.h>
#include <rococo.ui.h>
#include <Components/SlateWrapperTypes.h>
#include <Framework/Application/SlateApplication.h>

void CopySpatialInfo(Rococo::MouseEvent& dest, const FPointerEvent& src, const FGeometry& geometry);
void CopySpatialInfo_NoFullscreenCorrection(Rococo::MouseEvent& dest, const FPointerEvent& src, const FGeometry& geometry);
Rococo::Gui::GRKeyContextFlags ToContext(const FPointerEvent& ev);

namespace Rococo
{
	ROCOCO_API void LogExceptionAndContinue(Rococo::IException& ex, const char* prelude, const char* postlude);
}

DECLARE_LOG_CATEGORY_EXTERN(RococoEvents, Error, All);
DEFINE_LOG_CATEGORY(RococoEvents);

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

static FName name_OnButtonClick(TEXT("OnButtonClick"));
static FName name_OnUserDefined(TEXT("OnUserDefined"));

FName GetName(Rococo::Gui::GRWidgetEvent& ev)
{
	using namespace Rococo::Gui;

	switch (ev.eventType)
	{
	case EGRWidgetEventType::BUTTON_CLICK:
		return name_OnButtonClick;
	case EGRWidgetEventType::USER_DEFINED:
		return name_OnUserDefined;
	default:
		return name_OnUserDefined; // User defined value that is valued (EGRWidgetEventType::USER_DEFINED + offset) where offset > 0
	}
}

void RaiseError(UObject* object, const FString& methodMsg, const FString& prop, const FString& err)
{
	if (object->Implements<URococoReflectionEventHandler>())
	{
		auto errorHandler = TScriptInterface<IRococoReflectionEventHandler>(object);
		errorHandler->Execute_OnBadMethod(object, err, methodMsg, prop);
	}	
	else if (object)
	{
		UE_LOG(RococoEvents, Error, TEXT("%s: event object %s does not implement IRococoReflectionEventHandler"), *err, *object->GetClass()->GetName());
	}
	else
	{
		UE_LOG(RococoEvents, Error, TEXT("%s: no event object"), *err);
	}
}

void RaiseError(UObject* object, UFunction* method, const FProperty* prop, const FString& err)
{
	RaiseError(object, method->GetName(), prop ? prop->GetName() : TEXT(""), err);
}

void Invoke(UFunction* method, UObject* handler, Rococo::Gui::GRWidgetEvent& ev, Rococo::Gui::IGRSystem& gr)
{
	bool hasInput = false;

	for (TFieldIterator<FProperty> i(method, EFieldIteratorFlags::ExcludeSuper); i; ++i)
	{
		FProperty* property = *i;

		if (!property->HasAnyPropertyFlags(CPF_Parm))
		{
			// Not a parameter, probably a local variable
			continue;
		}

		if (!property->HasAnyPropertyFlags(CPF_ConstParm))
		{
			RaiseError(handler, method, property, TEXT("Expecting const input ref"));
		}

		if (!property->HasAnyPropertyFlags(CPF_ReferenceParm))
		{
			RaiseError(handler, method, property, TEXT("Expecting const input ref"));
		}

		if (hasInput)
		{
			RaiseError(handler, method, nullptr, TEXT("Expecting only one method argument of type FRococoGREvent"));
		}

		auto* sp = CastField<FStructProperty>(property);
		if (!sp)
		{
			RaiseError(handler, method, property, TEXT("Expecting first method argument to be a struct of type FRococoGREvent"));
			return;
		}

		if (sp->Struct != FRococoGREvent::StaticStruct())
		{
			// Signature mismatch
			RaiseError(handler, method, property, TEXT("Expecting first method argument to be of type FRococoGREvent"));
			return;
		}

		hasInput = true;
	}

	if (!hasInput)
	{
		RaiseError(handler, method, nullptr, TEXT("Expecting only one method argument of type FRococoGREvent"));
		return;
	}

	FRococoGREvent args;
	args.EventCode = ev.eventType == Rococo::Gui::EGRWidgetEventType::USER_DEFINED ? RococoGREventCode::USER_DEFINED : (RococoGREventCode)(ev.eventType);
	args.extendedEventCode = (int)ev.eventType;
	args.MetaDataInt = ev.iMetaData;
	args.PanelId = ev.panelId;

	auto* widget = gr.FindWidget(ev.panelId);
	if (widget)
	{
		args.MetaDataString = FString::Printf(TEXT("%hs"), ev.sMetaData);
		args.SenderDesc = FString::Printf(TEXT("%hs"), widget->Panel().Desc());
	}

	handler->ProcessEvent(method, &args);
}

bool IsAsciiFunctionName(const char* s)
{
	if (s == nullptr || *s == 0) return false;

	if (!isalpha(*s))
	{
		return false;
	}

	s++;

	for (; *s != 0; s++)
	{
		if (!isalnum(*s))
		{
			return false;
		}
	}

	return true;
}

ROCOCO_GUI_API void RouteGREventViaReflection(UObject* handler, Rococo::Gui::GRWidgetEvent& ev, Rococo::Gui::IGRSystem& gr)
{
	if (!handler)
	{
		return;
	}

	UClass* objectClass = handler->GetClass();

	FName methodName = GetName(ev);

	auto* widget = gr.FindWidget(ev.panelId);
	if (widget)
	{
		auto* desc = widget->Panel().Desc();
		if (IsAsciiFunctionName(desc))
		{
			FString extendedName = FString::Printf(TEXT("%s_%hs"), *methodName.ToString(), desc);
			UFunction* methodEx = objectClass->FindFunctionByName(FName(extendedName));
			if (methodEx)
			{
				Invoke(methodEx, handler, ev, gr);
				return;
			}
		}
	}

	UFunction* method = objectClass->FindFunctionByName(methodName);
	if (!method)
	{
		RaiseError(handler, methodName.ToString(), TEXT(""), FString::Printf(TEXT("Could not find function in event handler %s"), *handler->GetClass()->GetName()));
		return;
	}

	Invoke(method, handler, ev, gr);
}


FEventReply RouteMouseButtonDown(TScriptInterface<IRococoEmittedUIEventHandler>& eventHandler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
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

		auto result = custodian->RouteMouseEvent(me, ToContext(ue5MouseEvent));
		if (result == Rococo::Gui::EGREventRouting::NextHandler)
		{
			return eventHandler.GetObject() ? eventHandler->Execute_OnEmittedMouseButtonDown(eventHandler.GetObject(), geometry, ue5MouseEvent) : FEventReply(false);
		}
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);;
	}

	return FEventReply(true);
}

FEventReply RouteMouseButtonUp(TScriptInterface<IRococoEmittedUIEventHandler>& eventHandler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
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
		auto result = custodian->RouteMouseEvent(me, ToContext(ue5MouseEvent));
		if (result == Rococo::Gui::EGREventRouting::NextHandler)
		{
			return eventHandler.GetObject() ? eventHandler->Execute_OnEmittedMouseButtonUp(eventHandler.GetObject(), geometry, ue5MouseEvent) : FEventReply(false);
		}
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);;
	}

	return FEventReply(true);
}

FEventReply RouteMouseMove(TScriptInterface<IRococoEmittedUIEventHandler>& eventHandler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
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
		auto result = custodian->RouteMouseEvent(me, ToContext(ue5MouseEvent));
		if (result == Rococo::Gui::EGREventRouting::NextHandler)
		{
			return eventHandler.GetObject() ? eventHandler->Execute_OnEmittedMouseMove(eventHandler.GetObject(), geometry, ue5MouseEvent) : FEventReply(false);
		}
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);;
	}

	return FEventReply(true);
}

FEventReply RouteMouseWheel(TScriptInterface<IRococoEmittedUIEventHandler>& eventHandler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
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
		// Mouse wheel events do not seem to need to erroneously scale the mouse cursor
		CopySpatialInfo_NoFullscreenCorrection(me, ue5MouseEvent, geometry);
		auto result = custodian->RouteMouseEvent(me, ToContext(ue5MouseEvent));
		if (result == Rococo::Gui::EGREventRouting::NextHandler)
		{
			return eventHandler.GetObject() ? eventHandler->Execute_OnEmittedMouseWheel(eventHandler.GetObject(), geometry, ue5MouseEvent) : FEventReply(false);
		}
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

FEventReply RouteKeyDown(TScriptInterface<IRococoEmittedUIEventHandler>& eventHandler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, FKeyEvent ue5KeyEvent)
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

		auto result = custodian->RouteKeyboardEvent(kex);
		if (result == Rococo::Gui::EGREventRouting::NextHandler)
		{
			return eventHandler.GetObject() ? eventHandler->Execute_OnEmittedKeyDown(eventHandler.GetObject(), geometry, ue5KeyEvent) : FEventReply(false);
		}
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);
	}

	return FEventReply(true);
}

FEventReply RouteKeyUp(TScriptInterface<IRococoEmittedUIEventHandler>& eventHandler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, FKeyEvent ue5KeyEvent)
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
		auto result = custodian->RouteKeyboardEvent(kex);
		if (result == Rococo::Gui::EGREventRouting::NextHandler)
		{
			return eventHandler.GetObject() ? eventHandler->Execute_OnEmittedKeyUp(eventHandler.GetObject(), geometry, ue5KeyEvent) : FEventReply(false);
		}
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);;
	}

	return FEventReply(true);
}