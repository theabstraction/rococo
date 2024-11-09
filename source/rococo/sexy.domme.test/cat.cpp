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

		Cat(ScriptingResources& _scripting, cstr sourceName) : D(_scripting, sourceName, "Rococo.Animals", "ICat")
		{
			makeBiscuitsIndex = D.GetMethodIndex("MakeBiscuits", 2, 0);
		}

		~Cat()
		{
		}

		void MakeBiscuits(int nBiscuits, float dt) override
		{
			REGISTER_DOMME_CALL(D);

			D.Push(nBiscuits);
			D.Push(dt);
			D.CallVirtualMethod(makeBiscuitsIndex);
			D.PopBytes(sizeof nBiscuits + sizeof dt);

			VALIDATE_REGISTERS;
		}

		void _Terminate() override
		{
			D.Terminate();
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Animals
{
	ICatSupervisor* CreateCat(ScriptingResources& scripting, cstr sourceFile)
	{
		return new Implementation::Cat(scripting, sourceFile);
	}
}