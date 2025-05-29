#pragma once
#include <Widgets/SLeafWidget.h>

#include <RococoGuiAPI.h>

namespace Rococo
{
	template<class T> struct IEventCallback;
}

namespace Rococo::GreatSex
{
	struct IGreatSexGenerator;
}


class FPaintArgs;
class FSlateWindowElementList;
class UTexture;

using TMapPathToTexture = TMap<FString, UTexture2D*>;

class SRococoGRHostWidget : public SLeafWidget
{
private:
	// OnPaint is a const function, and we need to access the none const API, so make our Gui retained system mutable
	mutable Rococo::AutoFree<Rococo::Gui::IGRSystemSupervisor> grSystem;
	mutable Rococo::AutoFree<Rococo::Gui::IUE5_GRCustodianSupervisor> custodian;

public:
	SLATE_BEGIN_ARGS(SRococoGRHostWidget)
	{}
	SLATE_END_ARGS()

	SRococoGRHostWidget();

	// Slate widgets are volatile, so store the mapPathToTexture elsewhere and sync our Custodian to it just after construction
	void SyncCustodian(TMapPathToTexture& mapPathToTexture);

	void Construct(const FArguments& args);
	FVector2D ComputeDesiredSize(float) const override;
	int32 OnPaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& cullingRect, OUT FSlateWindowElementList& drawElements, int32 LayerId, const FWidgetStyle& widgetStyle, bool bParentEnabled) const override;

	void LoadFrame(const char* pingPath, Rococo::IEventCallback<Rococo::GreatSex::IGreatSexGenerator>& onPrepForLoading);
};
