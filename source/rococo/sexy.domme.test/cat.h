#pragma once

#include <rococo.domme.h>

namespace Rococo::Animals
{
	ROCOCO_INTERFACE ICat
	{
		virtual void MakeBiscuits(int nBiscuits) = 0;
	};

	ROCOCO_INTERFACE ICatSupervisor : ICat
	{
		virtual void Free() = 0;

		// Completes the script, terminating the virtual machine
		virtual void _Terminate() = 0;
	};

	ICatSupervisor* CreateCat(Rococo::Domme::ScriptingResources& scripting, cstr catSourceFile);
}