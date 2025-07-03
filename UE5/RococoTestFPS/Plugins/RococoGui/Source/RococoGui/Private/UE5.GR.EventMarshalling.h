#pragma once
#include <CoreMinimal.h>

FEventReply RouteMouseButtonDown(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteMouseButtonUp(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteMouseMove(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteMouseWheel(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, const FPointerEvent& ue5MouseEvent);
FEventReply RouteKeyDown(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, FKeyEvent ue5KeyEvent);
FEventReply RouteKeyUp(Rococo::Gui::IUE5_GRCustodianSupervisor* custodian, const FGeometry& geometry, FKeyEvent ue5KeyEvent);
