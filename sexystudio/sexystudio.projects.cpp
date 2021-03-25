#include "sexystudio.impl.h"
#include <Sexy.S-Parser.h>
#include <sexy.types.h>

namespace Rococo::SexyStudio
{
	using namespace Rococo::Sex;

	void PopulateTreeWithSXYFiles(IGuiTree& tree, cstr contentFolder)
	{
		tree.Clear();

		auto hRoot = tree.AppendItem(0);
		tree.SetItemText("<root>", hRoot);

		auto hCat = tree.AppendItem(hRoot);
		tree.SetItemText("cat", hCat);

		auto hDog = tree.AppendItem(hRoot);
		tree.SetItemText("dog", hDog);

		Auto<ISParser> sparser = Sexy_CreateSexParser_2_0(Rococo::Memory::CheckedAllocator());

		Auto<ISourceCode> src = sparser->LoadSource(L"C:\\work\\rococo\\content\\scripts\\mhost\\harrier.sxy", { 1,1 });
		Auto<ISParserTree> tree = sparser->CreateTree(*src);
	}

	void Run()
	{
		using namespace Rococo::Sex;
		auto& s = *sparser;
		
		
	}
}