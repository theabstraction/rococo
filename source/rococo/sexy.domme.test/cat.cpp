#include "cat.h"

#include <rococo.domme.h>

using namespace Rococo;
using namespace Rococo::Domme;

namespace Rococo::Animals::Implementation
{
	struct Cat : ICatSupervisor
	{
		DommeObject D;

		int makeBiscuitsIndex = -1;

		Cat(ScriptingResources& _scripting, cstr sourceName) : D(_scripting, sourceName, "Rococo.ICat")
		{
			makeBiscuitsIndex = D.GetMethodIndex("MakeBiscuits", 1, 0);
		}

		~Cat()
		{
		}

		void MakeBiscuits() override
		{
			// Todo - required fix - push stack frame and pop frame afterwards
			//auto& vm = D.VM();
			D.CallVirtualMethod(makeBiscuitsIndex);
		}

		void Free() override
		{
			delete this;
		}
	};

	ICatSupervisor* CreateCat(ScriptingResources& scripting, cstr sourceFile)
	{
		return new Cat(scripting, sourceFile);
	}
}