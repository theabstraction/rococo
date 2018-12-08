#include <rococo.cute.h>
#include <rococo.io.h>
#include <rococo.cute.h>
#include "rococo.cute.post.h"

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
			if (p != nullptr)
			{
				auto* r1 = p->Root->AddItem("R1");
				auto a = r1->AddItem("A");
				r1->AddItem("B");
				r1->AddItem("C");

				a->AddItem("Camoflage");
			}
		}
	} q;

	factory.Postbox().Subscribe(PostType_PopulateTree, &q);

	ExecuteScriptSpec spec;
	ExecuteWindowScript("!scripts/create.main.window.sxy", installation, spec, factory);

	factory.Postbox().Unsubscribe<PopulateTree>(&q);

	return RunMessageLoop(factory);
}
