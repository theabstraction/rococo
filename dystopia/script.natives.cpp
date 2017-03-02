#include "dystopia.h"
#include <rococo.strings.h>
#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>

#include <rococo.maths.h>

using namespace Dystopia;

namespace Dystopia
{
	IMeshes* FactoryConstructDystopiaMeshesGetMeshes(IMeshes* _context)
	{
		return _context;
	}

	IJournal* FactoryConstructDystopiaJournalGetJournal(IJournal* _context)
	{
		return _context;
	}

	ILevelBuilder* FactoryConstructDystopiaLevelsGetLevel(ILevel* _level)
	{
		return &_level->Builder();
	}

	IGui* FactoryConstructDystopiaGuiGetGui(IGui* _gui)
	{
		return _gui;
	}

   Dystopia::UI::IUIBuilder* FactoryConstructDystopiaUIGetUIBuilder(Dystopia::UI::IUIBuilder* _context)
   {
      return _context;
   }
}

#include "dystopia.sxh.inl"