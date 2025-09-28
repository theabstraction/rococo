#pragma once
#include <CoreMinimal.h>

FEventReply RouteMouseButtonDown(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteMouseButtonUp(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteMouseMove(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteMouseWheel(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteKeyDown(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, FKeyEvent ue5KeyEvent);
FEventReply RouteKeyUp(TScriptInterface<IRococoEmittedUIEventHandler>& handler, Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, FKeyEvent ue5KeyEvent);
