// rococo.gluey.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include <rococo.types.h>

namespace Rococo
{
   namespace Gluey 
   {
      struct IActivate
      {

      };

      struct IDocumentElementId
      {

      };

      struct IPopulateRequest
      {

      };

      struct IController
      {
         virtual void SendActivateToDocument(IActivate& activate) = 0;
         virtual void RequestPopulateFromDocument(IPopulateRequest& request) = 0;
         virtual void PopulateGui(IDocumentElementId& element, bool value) = 0;
         virtual void PopulateGui(IDocumentElementId& element, int value) = 0;
         virtual void Subscribe(IDocumentElementId& element, bool addElseRemove) = 0;
      };

      struct IControllerSupervisor : public IController
      {
         virtual void Free() = 0;
      };
   } // Rococo::Guey
} // Rococo

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
   
}

