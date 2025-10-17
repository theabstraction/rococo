// Copyright (c) 2025 Mark Anthony Taylor. All rights reserved. Email: mark.anthony.taylor@gmail.com.
#pragma once
#include <CoreMinimal.h>
#include <Components\SlateWrapperTypes.h>

class IRococoEmittedUIEventHandler;

namespace Rococo
{
	struct MouseEvent;
	struct IException;
}

namespace Rococo::Gui
{
	struct IUE5_GRCustodianSupervisor;
	struct GRKeyContextFlags;
}

FEventReply RouteMouseButtonDown(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteMouseButtonUp(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteMouseMove(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteMouseWheel(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteKeyDown(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, FKeyEvent ue5KeyEvent);
FEventReply RouteKeyUp(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, FKeyEvent ue5KeyEvent);
