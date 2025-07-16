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
	_SlateHostWidget = SNew(SRococoGRHostWidget);

	NullHandler doNothing;
	_SlateHostWidget->SyncCustodian(this, _MapPathToTexture, _FontAsset, _UseDefaultFocusRenderer, _SlateEventHandler ? *_SlateEventHandler : doNothing, _CustodianManager ? *_CustodianManager : *this);

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