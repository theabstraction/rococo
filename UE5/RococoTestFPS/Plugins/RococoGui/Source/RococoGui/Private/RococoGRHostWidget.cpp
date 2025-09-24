#include "RococoGRHostWidget.h"
#include "rococo.GR.UE5.h"
#include <rococo.great.sex.h>
#include <rococo.gui.retained.h>
#include <rococo.ui.h>
#include "ReflectedGameOptionsBuilder.h"
#include <GameFramework/GameUserSettings.h>

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
	_SlateHostWidget.Reset();
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
	lastCategory = RococoControlCategory::NONE;

	_SlateHostWidget = SNew(SRococoGRHostWidget);

	NullHandler doNothing;
	_SlateHostWidget->SyncCustodian(this, _MapPathToTexture, _FontAsset, _UseDefaultFocusRenderer, _SlateEventHandler ? *_SlateEventHandler : doNothing, _GlobalFontMetrics ? *_GlobalFontMetrics : *this);

	auto* custodian = _SlateHostWidget->GetCustodian();
	if (custodian)
	{
		custodian->SetLogging(_LogToScreen, _LogToFile);
	}

	return _SlateHostWidget.ToSharedRef();
}

Rococo::Gui::IUE5_GRCustodianSupervisor* URococoGRHostWidget::GetCurrentCustodian()
{
	if (!_SlateHostWidget)
	{
		return nullptr;
	}

	return _SlateHostWidget->GetCustodian();
}

void URococoGRHostWidget::SetControlCategory(RococoControlCategory category)
{
	if (category == lastCategory)
	{
		return;
	}

	lastCategory = category;
	
	Rococo::Gui::IUE5_GRCustodianSupervisor* custodian = URococoGRHostWidget::GetCurrentCustodian();
	if (custodian)
	{
		const char* sCategory = nullptr;
		switch (category)
		{
		case RococoControlCategory::KEYBOARD:
			sCategory = "Keyboard";
			break;
		case RococoControlCategory::XBOX:
			sCategory = "XBOX";
			break;
		case RococoControlCategory::PLAYSTATION:
			sCategory = "Playstation";
			break;
		}

		if (sCategory != nullptr)
		{
			custodian->SetControlType(sCategory);
		}
	}
}

void URococoGRHostWidget::GRPumpMessages()
{
	if (!_SlateHostWidget)
	{
		return;
	}

	auto* gr = static_cast<Rococo::Gui::IGRSystemSupervisor*>(_SlateHostWidget->GR());
	if (!gr)
	{
		return;
	}

	if (!_GREventHandler)
	{
		return;
	}

	gr->SetEventHandler(this);

	// This will trigger URococoGRHostWidget::OnGREvent for each pending event. _GREventHandler must be assigned first
	gr->DispatchMessages();
}

ROCOCOGUI_API void RouteGREventViaReflection(UObject* handler, Rococo::Gui::GRWidgetEvent& ev, Rococo::Gui::IGRSystem& gr);

Rococo::Gui::EGREventRouting URococoGRHostWidget::OnGREvent(Rococo::Gui::GRWidgetEvent& ev)
{
	if (_GREventHandler == nullptr)
	{
		UE_LOG(RococoGUI, Error, TEXT("RouteGREventViaReflection unhandled, as handler was NULL"));
	}
	else
	{
		if (!_SlateHostWidget)
		{
			UE_LOG(RococoGUI, Error, TEXT("RouteGREventViaReflection unhandled, as _SlateHostWidget was NULL"));
		}
		else if (!_SlateHostWidget->GR())
		{
			UE_LOG(RococoGUI, Error, TEXT("RouteGREventViaReflection unhandled, as _SlateHostWidget->GR() was NULL"));
		}
		else
		{
			RouteGREventViaReflection(_GREventHandler, ev, *_SlateHostWidget->GR());
		}
	}
	return Rococo::Gui::EGREventRouting::Terminate;
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
	if (_SlateHostWidget.IsValid())
	{
		_SlateHostWidget->LoadFrame(sexmlPingPath, onPrepForLoading);
		auto* gr = _SlateHostWidget->GR();
		if (gr)
		{
			gr->Root().SetSelectionChangeHandler(this);
		}

	}
	else
	{
		UE_LOG(RococoGUI, Error, TEXT("SlateHostWidget is not valid. LoadFrame(<%hs>) failed"), sexmlPingPath);
	}
}

int URococoGRHostWidget::GetUE5PointSize(int rococoPointSize)
{
	float f = FMath::Clamp(_FontPointSizeRatio, 0.2f, 8.0f);
	return (int) (f * rococoPointSize);
}

void URococoGRHostWidget::OnSelectionChanged(Rococo::Gui::IGRPanel& panel, Rococo::Gui::EGRSelectionChangeOrigin origin)
{
	UNUSED(panel);

	RococoSelectionChangeOrigin ue5origin = RococoSelectionChangeOrigin::None;

	switch (origin)
	{
	case Rococo::Gui::EGRSelectionChangeOrigin::VMenuKeyNav:
		ue5origin = RococoSelectionChangeOrigin::VMenuKeyNav;
		break;
	case Rococo::Gui::EGRSelectionChangeOrigin::CarouselLRArrows:
		ue5origin = RococoSelectionChangeOrigin::CarouselLRArrows;
		break;
	case Rococo::Gui::EGRSelectionChangeOrigin::ButtonClick:
		ue5origin = RococoSelectionChangeOrigin::ButtonClick;
		break;
	case Rococo::Gui::EGRSelectionChangeOrigin::ScalarChangeKey:
		ue5origin = RococoSelectionChangeOrigin::ScalarChangeKey;
		break;
	}

	EvSelectionChanged(ue5origin);
}

void URococoGRHostWidgetBuilder::ReloadFrame()
{
	if (_SexmlPingPath.IsEmpty())
	{
		UE_LOG(RococoGUI, Warning, TEXT("URococoGRHostWidgetBuilder::ReloadFrame: empty sexml ping path"));
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

void LoadGlobalOptions(Rococo::GreatSex::IReflectedGameOptionsBuilder& builder, const TArray<UObject*>& context, Rococo::GreatSex::IGreatSexGenerator& generator)
{
	if (!s_fnGlobalPrepGenerator)
	{
		UE_LOG(RococoGUI, Error, TEXT("ROCOCOGUI_API void SetGlobalPrepGenerator(...) not invoked. Falling back on demo options"));
		Rococo::GreatSex::AddTestOptions(generator);
	}
	else
	{	
		s_fnGlobalPrepGenerator(builder, context, generator);
	}
}

URococoGRHostWidgetBuilder::URococoGRHostWidgetBuilder()
{
	optionsBuilder = Rococo::GreatSex::CreateReflectedGameOptionsBuilder();
}

URococoGRHostWidgetBuilder::~URococoGRHostWidgetBuilder()
{
	if (optionsBuilder)
	{
		optionsBuilder->Free();
	}
}

void URococoGRHostWidgetBuilder::OnPrepForLoading(Rococo::GreatSex::IGreatSexGenerator& generator)
{
	if (_UseGlobalOptions)
	{
		LoadGlobalOptions(*optionsBuilder, _GeneratorContext, generator);
	}
}

#include <rococo.os.h>

#include <Widgets\SViewport.h>
#include <Rendering\RenderingCommon.h>
#include <Engine\UserInterfaceSettings.h>

FIntPoint GetViewportSize()
{
	auto* viewportWidget = GEngine->GetGameViewportWidget().Get();
	if (!viewportWidget)
	{
		return 1.0;
	}

	auto* viewport = viewportWidget->GetViewportInterface().Pin().Get();
	if (!viewport)
	{
		return 1.0;
	}

	return viewport->GetSize();
}

// Mouse co-ordiantes are scaled: PointerEvent.GetLastScreenSpacePosition() * WindowSize / DisplaySize
// The display size is the primary display, NOT the actual monitor display, so we need to undo the scaling and apply the monitor scaling, this gives the correction factor
FVector2f GetFullscreenCorrection(const FGeometry& geometry)
{
	UGameUserSettings* settings = GEngine->GetGameUserSettings();
	if (settings->GetFullscreenMode() == EWindowMode::Fullscreen)
	{
		FDisplayMetrics metrics;
		FSlateApplication::Get().GetCachedDisplayMetrics(OUT metrics);

		for (auto& monitor : metrics.MonitorInfo)
		{
			FVector2f monitorCentre((monitor.WorkArea.Left + monitor.WorkArea.Right) >> 1, (monitor.WorkArea.Top + monitor.WorkArea.Bottom) >> 1);

			if (geometry.IsUnderLocation(monitorCentre))
			{
				FVector2f primaryDisplaySize{ (float)metrics.PrimaryDisplayWidth,  (float)metrics.PrimaryDisplayHeight };
				FVector2f resolution{ (float)monitor.DisplayRect.Right - monitor.DisplayRect.Left, (float)monitor.DisplayRect.Bottom - monitor.DisplayRect.Top };
				FVector2f correction = primaryDisplaySize / resolution;
				return correction;
			}
		}
	}

	return FVector2f(1.0f, 1.0f);
}

void CopySpatialInfo(Rococo::MouseEvent& dest, const FPointerEvent& src, const FGeometry& geometry)
{
	FVector2f delta = src.GetCursorDelta();
	dest.dx = (int) delta.X;
	dest.dy = (int) delta.Y;

	FVector2f cursorPosScreenSpace = src.GetScreenSpacePosition();

	FVector2f correctedScreenSpacePos = cursorPosScreenSpace * GetFullscreenCorrection(geometry);

	FVector2f localPos = geometry.AbsoluteToLocal(correctedScreenSpacePos);

	dest.cursorPos.x = (int) localPos.X;
	dest.cursorPos.y = (int) localPos.Y;
}

void CopySpatialInfo_NoFullscreenCorrection(Rococo::MouseEvent& dest, const FPointerEvent& src, const FGeometry& geometry)
{
	FVector2f delta = src.GetCursorDelta();
	dest.dx = (int)delta.X;
	dest.dy = (int)delta.Y;

	FVector2f cursorPosScreenSpace = src.GetScreenSpacePosition();
	FVector2f localPos = geometry.AbsoluteToLocal(cursorPosScreenSpace);

	dest.cursorPos.x = (int)localPos.X;
	dest.cursorPos.y = (int)localPos.Y;
}

Rococo::Gui::GRKeyContextFlags ToContext(const FPointerEvent& ev)
{
	Rococo::Gui::GRKeyContextFlags context;
	context.isAltHeld = ev.IsAltDown();
	context.isCtrlHeld = ev.IsControlDown();
	context.isShiftHeld = ev.IsShiftDown();
	return context;
}

#include "UE5.GR.EventMarshalling.h"

FEventReply URococoGRHostWidgetBuilder::RouteMouseButtonDown(const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	return ::RouteMouseButtonDown(GetCurrentCustodian(), geometry, ue5MouseEvent);
}

FEventReply URococoGRHostWidgetBuilder::RouteMouseButtonUp(const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	return ::RouteMouseButtonUp(GetCurrentCustodian(), geometry, ue5MouseEvent);
}

FEventReply URococoGRHostWidgetBuilder::RouteMouseMove(const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	return ::RouteMouseMove(GetCurrentCustodian(), geometry, ue5MouseEvent);
}

FEventReply URococoGRHostWidgetBuilder::RouteMouseWheel(const FGeometry& geometry, const FPointerEvent& ue5MouseEvent)
{
	return ::RouteMouseWheel(GetCurrentCustodian(), geometry, ue5MouseEvent);
}

FEventReply URococoGRHostWidgetBuilder::RouteKeyDown(const FGeometry& geometry, FKeyEvent ue5KeyEvent)
{
	return ::RouteKeyDown(GetCurrentCustodian(), geometry, ue5KeyEvent);
}

FEventReply URococoGRHostWidgetBuilder::RouteKeyUp(const FGeometry& geometry, FKeyEvent ue5KeyEvent)
{
	return ::RouteKeyUp(GetCurrentCustodian(), geometry, ue5KeyEvent);
}