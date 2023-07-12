#include <rococo.cute.h>
#include <rococo.io.h>
#include "rococo.cute.post.h"
#include <rococo.strings.h>
#include <sexy.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>

using namespace Rococo;
using namespace Rococo::Cute;
using namespace Rococo::Post;

int RunMessageLoop(IMasterWindowFactory& factory);

int Main(IInstallation& installation, IMasterWindowFactory& factory)
{
	using namespace Rococo::Cute;

	struct : Post::IRecipient
	{
		void OnPost(const Mail& mail) override
		{
			auto* p = InterpretAs<PopulateTree>(mail);
			if (p != nullptr && Eq(p->id, "populator.tree.solution"))
			{
				auto* r1 = p->Root->AddItem("Solution");
				auto a = r1->AddItem("Amadeus");
				auto b = r1->AddItem("Beethoven");
				auto c = r1->AddItem("Chopin");

				auto bs = b->AddItem("Symphonies");
				bs->AddItem("Symphony 1-3");
				bs->AddItem("Symphony 4-6");
				bs->AddItem("Symphony 7-9");

				auto bc = b->AddItem("Piano Concertos");
				bc->AddItem("Piano Concerto No 1-2");
				bc->AddItem("Piano Concerto No 3-5");

				auto ao = a->AddItem("Operas");
				ao->AddItem("The Marriage of Figaro");
				ao->AddItem("The Magic Flute");

				auto ce = c->AddItem("Etudes");
				ce->AddItem("No 9");
			}
		}
	} q;

	factory.Postbox().Subscribe(PostType_PopulateTree, &q);

	using namespace Rococo::Script;

	AutoFree<IScriptSystemFactory> ssFactory = CreateScriptSystemFactory_1_5_0_0(Rococo::Memory::CheckedAllocator());

	ExecuteScriptSpec spec;
	ExecuteWindowScript("!scripts/create.main.window.sxy", installation, *ssFactory, spec, factory);

	factory.Postbox().Unsubscribe<PopulateTree>(&q);

	return RunMessageLoop(factory);
}
