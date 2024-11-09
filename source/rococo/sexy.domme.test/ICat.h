#pragma once

#include <rococo.domme.h>

namespace Rococo::Animals
{
	ROCOCO_INTERFACE ICat
	{
		virtual void MakeBiscuits(int nBiscuits, float dt) = 0;
	};

	ROCOCO_INTERFACE ICatSupervisor : ICat
	{
		virtual void _Free() = 0;

		// Completes the script, terminating the virtual machine
		virtual void _Terminate() = 0;
	};

	ICatSupervisor* CreateCat(Rococo::Domme::ScriptingResources& scripting, cstr catSourceFile);
}

DECLARE_DOMME_INTERFACE(Rococo::Animals::ICatSupervisor)
