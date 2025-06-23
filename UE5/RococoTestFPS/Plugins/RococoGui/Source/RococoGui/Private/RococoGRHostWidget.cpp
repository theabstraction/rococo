#include "RococoGRHostWidget.h"
#include "rococo.GR.UE5.h"
#include <rococo.great.sex.h>
#include <rococo.gui.retained.h>
#include <rococo.ui.h>

DECLARE_LOG_CATEGORY_EXTERN(RococoGUI, Error, All);
DEFINE_LOG_CATEGORY(RococoGUI);

static FN_GlobalPrepGenerator s_fnGlobalPrepGenerator = nullptr;

namespace Rococo::Gui
{
	ROCOCOGUI_API void SetGlobalPrepGenerator(FN_GlobalPrepGenerator fnGlobalPrepGenerator)
	{
		s_fnGlobalPrepGenerator = fnGlobalPrepGenerator;
	}
}

void URococoGRHostWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	slateHostWidget.Reset();
}

struct NullHandler: ISRococoGRHostWidgetEventHandler
{
	// Overridden in a derived class to assign such things as a focus renderer to the GR system
	void OnGRSystemConstructed(Rococo::Gui::IUE5_GRCustodianSupervisor& custodian, Rococo::Gui::IGRSystem& gr) override 
	{
	}
};

TSharedRef<SWidget> URococoGRHostWidget::RebuildWidget()
{
	slateHostWidget = SNew(SRococoGRHostWidget);

	NullHandler doNothing;
	slateHostWidget->SyncCustodian(mapPathToTexture, _FontAsset, _UseDefaultFocusRenderer, _SlateEventHandler ? *_SlateEventHandler : doNothing);

	return slateHostWidget.ToSharedRef();
}

Rococo::Gui::IUE5_GRCustodianSupervisor* URococoGRHostWidget::GetCurrentCustodian()
{
	if (!slateHostWidget)
	{
		return nullptr;
	}

	return slateHostWidget->GetCustodian();
}

static void ConvertFStringToUTF8Buffer(TArray<uint8>& buffer, const FString& src)
{
	int32 nElements = FTCHARToUTF8_Convert::ConvertedLength(*src, src.Len());
	buffer.SetNumUninitialized(nElements + 1);
	FTCHARToUTF8_Convert::Convert(reinterpret_cast<UTF8CHAR*>(buffer.GetData()), buffer.Num(), *src, nElements);
	buffer[nElements] = 0;
}

static bool IsAsciiPath(const FString& path)
{
	for (TCHAR c : path)
	{
		if (c <= 32 || c >= 128)
		{
			return false;
		}
	}

	return true;
}

void URococoGRHostWidget::LoadFrame(const FString& sexmlPingPath, Rococo::IEventCallback<Rococo::GreatSex::IGreatSexGenerator>& onPrepForLoading)
{
	if (!IsAsciiPath(sexmlPingPath))
	{
		UE_LOG(RococoGUI, Error, TEXT("URococoGRHostWidget::Sexml ping path has to be a sequence of printable ascii characters. Legal Ascii values are (33-127): <%s>"), *sexmlPingPath);
		return;
	}

	if (**sexmlPingPath != TEXT('!'))
	{
		UE_LOG(RococoGUI, Error, TEXT("URococoGRHostWidget::Sexml ping path has to start with a ping (!): <%s>"), *sexmlPingPath);
		return;
	}

	TArray<uint8> asciiPingPathBuffer;
	ConvertFStringToUTF8Buffer(OUT asciiPingPathBuffer, sexmlPingPath);
	const char* asciiPingPath = (const char*) asciiPingPathBuffer.GetData();
	LoadFrame(asciiPingPath, onPrepForLoading);
}

void URococoGRHostWidget::LoadFrame(const char* sexmlPingPath, Rococo::IEventCallback<Rococo::GreatSex::IGreatSexGenerator>& onPrepForLoading)
{
	if (slateHostWidget.IsValid())
	{
		slateHostWidget->LoadFrame(sexmlPingPath, onPrepForLoading);
	}
	else
	{
		UE_LOG(RococoGUI, Error, TEXT("SlateHostWidget is not valid. LoadFrame(<%hs>) failed"), sexmlPingPath);
	}
}

void URococoGRHostWidgetBuilder::ReloadFrame()
{
	if (_SexmlPingPath.IsEmpty())
	{
		return;
	}

	struct Proxy : Rococo::IEventCallback<Rococo::GreatSex::IGreatSexGenerator>
	{
		URococoGRHostWidgetBuilder* This = nullptr;

		void OnEvent(Rococo::GreatSex::IGreatSexGenerator& generator) override
		{
			This->OnPrepForLoading(generator);
		}
	} onPrepForLoad;

	onPrepForLoad.This = this;

	LoadFrame(_SexmlPingPath, onPrepForLoad);
}

#include <Modules/ModuleManager.h>

void LoadGlobalOptions(const FString& key, Rococo::GreatSex::IGreatSexGenerator& generator)
{
	if (!s_fnGlobalPrepGenerator)
	{
		UE_LOG(RococoGUI, Error, TEXT("ROCOCOGUI_API void SetGlobalPrepGenerator(...) not invoked. Falling back on demo options"));
		Rococo::GreatSex::AddTestOptions(generator);
	}
	else
	{	
		s_fnGlobalPrepGenerator(key, generator);
	}
}

void URococoGRHostWidgetBuilder::OnPrepForLoading(Rococo::GreatSex::IGreatSexGenerator& generator)
{
	if (_UseGlobalOptions)
	{
		LoadGlobalOptions(_GlobalOptionsKey, generator);
	}
}

#include <rococo.os.h>

void CopySpatialInfo(Rococo::MouseEvent& dest, const FPointerEvent& src, const FGeometry& geometry)
{
	FVector2f delta = src.GetCursorDelta();
	dest.dx = (int) delta.X;
	dest.dy = (int) delta.Y;

	FVector2f cursorPosScreenSpace = src.GetScreenSpacePosition();
	FVector2f localPos = geometry.AbsoluteToLocal(cursorPosScreenSpace);

	dest.cursorPos.x = (int) localPos.X;
	dest.cursorPos.y = (int) localPos.Y;
}

Rococo::Gui::GRKeyContextFlags ToContext(const FPointerEvent& ev)
{
	Rococo::Gui::GRKeyContextFlags context;
	context.isAltHeld = ev.IsAltDown();
	context.isCtrlHeld = ev.IsControlDown();
	context.isShiftHeld = ev.IsShiftDown();
	return context;
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
		}
		else if (mouseKey == EKeys::MiddleMouseButton)
		{
			me.buttonFlags = MouseEvent::Flags::MDown;
		}
		else if (mouseKey == EKeys::LeftMouseButton)
		{
			me.buttonFlags = MouseEvent::Flags::LDown;
		}
		else
		{
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

FEventReply URococoGRHostWidgetBuilder::RouteMouseButtonDown(const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	return ::RouteMouseButtonDown(GetCurrentCustodian(), geometry, ue5MouseEvent);
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
		}
		else if (mouseKey == EKeys::MiddleMouseButton)
		{
			me.buttonFlags = MouseEvent::Flags::MUp;
		}
		else if (mouseKey == EKeys::LeftMouseButton)
		{
			me.buttonFlags = MouseEvent::Flags::LUp;
		}
		else
		{
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

FEventReply URococoGRHostWidgetBuilder::RouteMouseButtonUp(const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	return ::RouteMouseButtonUp(GetCurrentCustodian(), geometry, ue5MouseEvent);
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

FEventReply URococoGRHostWidgetBuilder::RouteMouseMove(const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	return ::RouteMouseMove(GetCurrentCustodian(), geometry, ue5MouseEvent);
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


FEventReply URococoGRHostWidgetBuilder::RouteMouseWheel(const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	return ::RouteMouseWheel(GetCurrentCustodian(), geometry, ue5MouseEvent);
}

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
		kex.VKey = ue5KeyEvent.GetKeyCode();
		FName name = ue5KeyEvent.GetKey().GetFName();
		kex.scanCode = 0;
		kex.Flags = 0;
		kex.unicode = ue5KeyEvent.GetCharacter();
		custodian->RouteKeyboardEvent(kex);
	}
	catch (IException& ex)
	{
		LogExceptionAndContinue(ex, __FUNCTION__, nullptr);;
	}

	return FEventReply(true);
}

FEventReply URococoGRHostWidgetBuilder::RouteKeyDown(const FGeometry& geometry, FKeyEvent ue5KeyEvent)
{
	return ::RouteKeyDown(GetCurrentCustodian(), geometry, ue5KeyEvent);
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
		kex.VKey = ue5KeyEvent.GetKeyCode();
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

FEventReply URococoGRHostWidgetBuilder::RouteKeyUp(const FGeometry& geometry, FKeyEvent ue5KeyEvent)
{
	return ::RouteKeyUp(GetCurrentCustodian(), geometry, ue5KeyEvent);
}