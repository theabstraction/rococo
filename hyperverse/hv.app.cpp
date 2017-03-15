#include "hv.h"

namespace
{
   using namespace Rococo;
   using namespace HV;

   class HVApp : public IApp, public IEventCallback<FileModifiedArgs>
   {
      Cosmos& e;

   public:
      HVApp(Cosmos& _e) : e(_e)
      {
         
      }

      virtual void Free()
      {
         delete this;
      }

      virtual void OnCreated()
      {

      }

      virtual void OnEvent(FileModifiedArgs& args)
      {
         HV::Events::OnFileChangedEvent ev;
         ev.args = &args;
         Publish(e.publisher, ev);
      }

      virtual uint32 OnFrameUpdated(const IUltraClock& clock)
      {
         e.installation.OS().EnumerateModifiedFiles(*this);
         e.renderer.Render(e.scene);
         return 5;
      }

      virtual void OnKeyboardEvent(const KeyboardEvent& k)
      {

      }

      virtual void OnMouseEvent(const MouseEvent& me)
      {

      }

      virtual const wchar_t* Title() const
      {
         return L"Hyperverse, by Mark Anthony Taylor";
      }
   };
}

namespace HV
{
   IApp* CreateHVApp(Cosmos& e)
   {
      return new HVApp(e);
   }
}
