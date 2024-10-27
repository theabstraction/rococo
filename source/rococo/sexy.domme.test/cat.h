#pragma once

#include <rococo.domme.h>

namespace Rococo::Animals
{
	ROCOCO_INTERFACE ICat
	{
		virtual void MakeBiscuits() = 0;
	};

	ROCOCO_INTERFACE ICatSupervisor : ICat
	{
		virtual void Free() = 0;
	};

	ICatSupervisor* CreateCat(Rococo::Domme::ScriptingResources& scripting, cstr catSourceFile);
}