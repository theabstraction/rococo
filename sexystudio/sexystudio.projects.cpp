#include "sexystudio.impl.h"
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <sexy.types.h>

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>

#include <rococo.io.h>

namespace Rococo::SexyStudio
{
	using namespace Rococo::Sex;

	void PopulateBranchWithSTree(IGuiTree& tree, ISParserTree& srcTree)
	{
		auto branchId = tree.AppendItem(0);
		tree.SetItemText(srcTree.Source().Name(), branchId);
	}

	void PopulateTreeWithSXYFiles(IGuiTree& tree, cstr contentFolder)
	{
		tree.Clear();

		auto hRoot = tree.AppendItem(0);
		tree.SetItemText(contentFolder, hRoot);

		struct ANON : IEventCallback<IO::FileItemData>
		{
			IGuiTree* tree;

			void OnEvent(IO::FileItemData& item) override
			{
				auto idItem = tree->AppendItem((ID_TREE_ITEM) item.containerContext);

				U8FilePath itemText;
				Format(itemText, "%ws", item.itemRelContainer);
				tree->SetItemText(itemText, idItem);

				item.outContext = (void*) idItem;
			}
		} cb;

		cb.tree = &tree;

		WideFilePath path;
		Format(path, L"%hs", contentFolder);

		try
		{
			Rococo::IO::ForEachFileInDirectory(path, cb, true, (void*) hRoot);
		}
		catch (IException& ex)
		{
			auto hError = tree.AppendItem(hRoot);
			tree.SetItemText("Error recursing content folder", hError);
			auto hErrorMsg = tree.AppendItem(hError);
			tree.SetItemText(ex.Message(), hErrorMsg);
			return;
		}

		Auto<ISParser> sparser = Sexy_CreateSexParser_2_0(Rococo::Memory::CheckedAllocator());
		Auto<ISourceCode> src = sparser->LoadSource(L"C:\\work\\rococo\\content\\scripts\\mhost\\harrier.sxy", { 1,1 });
		Auto<ISParserTree> s_tree = sparser->CreateTree(*src);

	//	PopulateBranchWithSTree(tree, *s_tree);
	}

	void Run()
	{
		
	}
}