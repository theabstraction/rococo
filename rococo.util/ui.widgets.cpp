#include <rococo.widgets.h>

#include <unordered_map>

namespace Rococo
{
   namespace UI
   {
      typedef int32 ID_WIDGET;
      typedef int64 ID_UICOMMAND;

      enum EWidgetState: int32
      {
         EWidgetState_NotFocus = 0,
         EWidgetState_HasFocus = 1
      };

      struct IUIBuilder
      {
         virtual void AddButton(ID_WIDGET id, Vec2i span, cstr text) = 0;
         virtual void AddFrame(ID_WIDGET frameId, Vec2i span) = 0;
         virtual void AddWidgetToFrame(ID_WIDGET frameId, ID_WIDGET id) = 0;
         virtual void CentreChildrenHorizontally(ID_WIDGET frameId) = 0;
         virtual void CentreChildrenVertically(ID_WIDGET frameId) = 0;
         virtual void ShrinkWrapWidget(ID_WIDGET frameId) = 0;
         virtual void ExpandToFit(ID_WIDGET frameId) = 0;
         virtual void SetBorder(ID_WIDGET id, EWidgetState state, Vec2i dxdy, RGBAb c1, RGBAb c2) = 0;
         virtual void SetBackcolours(ID_WIDGET id, EWidgetState state, RGBAb b1, RGBAb b2) = 0;
         virtual void SetButtonPulse(ID_WIDGET id, ID_UICOMMAND commandId, boolean32 fireWhenDown, boolean32 fireWhenUp) = 0;
         virtual void SetFont(ID_WIDGET id, EWidgetState state, int32 fontId, RGBAb fontColour) = 0;
         virtual void Move(ID_WIDGET id, Vec2i positionInContainer) = 0;
      };
   }
}

using namespace Rococo;
using namespace UI;

namespace
{

}

namespace Rococo
{
   namespace UI
   {

   }
}