#include "RococoGRHostWidget.h"
#include <rococo.great.sex.h>
#include <rococo.gui.retained.h>

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

TSharedRef<SWidget> URococoGRHostWidget::RebuildWidget()
{
	slateHostWidget = SNew(SRococoGRHostWidget);
	slateHostWidget->SyncCustodian(mapPathToTexture, _FontDirectory);
	return slateHostWidget.ToSharedRef();
}

static void ConvertFStringToUTF8Buffer(TArray<uint8>& buffer, const FString& src)
{
	int32 nElements = FTCHARToUTF8_Convert::ConvertedLength(*src, src.Len());
	buffer.SetNumUninitialized(nElements + 1);
	FTCHARToUTF8_Convert::Convert(reinterpret_cast<UTF8CHAR*>(buffer.GetData()), buffer.Num(), *src, nElements);
	buffer[buffer.Num() - 1] = 0;
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
		UE_LOG(RococoGUI, Error, TEXT("Sexml ping path has to be an ascii sequence: <%s>"), *sexmlPingPath);
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