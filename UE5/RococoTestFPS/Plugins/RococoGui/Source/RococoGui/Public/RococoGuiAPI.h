#pragma once

#include <Runtime\Core\Public\HAL\Platform.h>

#include <rococo.gui.retained.ex.h>

namespace Rococo
{
	struct SlateRenderContext;
}

namespace Rococo::GreatSex
{
	struct LoadFrameException;
	DECLARE_ROCOCO_INTERFACE IGreatSexGenerator;
}

namespace Rococo::Gui
{
	// The custodian allows the Rococo Gui to render to UE5 slate widgets. It also routes UI events into the Rococo widget system.
	ROCOCO_INTERFACE IUE5_GRCustodianSupervisor : IGRCustodianSupervisor
	{
		// If invoked this will make the custodian render the error than the widget system
		virtual void AddLoadError(Rococo::GreatSex::LoadFrameException& err) = 0;
		virtual void Bind(IGRSystemSupervisor& grSystem) = 0;
		virtual IO::IInstallation& Installation() = 0;
		virtual void Render(SlateRenderContext& rc) = 0;
		virtual void RouteKeyboardEvent(const KeyboardEvent& key) = 0;
		virtual void RouteMouseEvent(const MouseEvent& me, const GRKeyContextFlags& context) = 0;
	};
}

namespace Rococo::Gui
{
	ROCOCOGUI_API IUE5_GRCustodianSupervisor* Create_UE5_GRCustodian(TMap<FString, UTexture2D*>& mapPathToImageTexture, const FSoftObjectPath& font);

	typedef void (*FN_GlobalPrepGenerator)(const FString& key, Rococo::GreatSex::IGreatSexGenerator& generator);

	ROCOCOGUI_API void SetGlobalPrepGenerator(FN_GlobalPrepGenerator fnGlobalPrepGenerator);
}
