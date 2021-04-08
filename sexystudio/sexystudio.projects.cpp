#include "sexystudio.impl.h"
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <sexy.types.h>

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>

#include <rococo.io.h>

#include <rococo.hashtable.h>
#include <vector>

namespace Rococo::SexyStudio
{
	using namespace Rococo::Sex;

	void PopulateBranchWithSTree(IGuiTree& tree, ISParserTree& srcTree)
	{
		auto branchId = tree.AppendItem(0);
		tree.SetItemText(srcTree.Source().Name(), branchId);
	}

	struct SXYMeta
	{
		int errorCode = 0;
		HString errorMessage;
		size_t id = 0;
	};

	struct SXYMetaTable : ISXYMetaTable
	{
		Auto<ISParser> sparser;
		stringmap<SXYMeta*> metadata;
		std::vector<SXYMeta*> idToMeta;

		enum { HR_FILE_TOO_LARGE = 0x20000001 };

		SXYMetaTable() :
			sparser(Sexy_CreateSexParser_2_0(Rococo::Memory::CheckedAllocator()))
		{

		}

		~SXYMetaTable()
		{
			for (auto* meta : idToMeta)
			{
				delete meta;
			}
		}

		void Free() override
		{
			delete this;
		}

		bool TryGetError(ID_SXY_META metaId, int& errorCode, char* buffer, size_t nBytesInBuffer)
		{
			if (metaId > idToMeta.size())
			{
				if (buffer) SafeFormat(buffer, nBytesInBuffer, "Bad ID_SXY_META");
				errorCode = E_INVALIDARG;
				return true;
			}

			auto& meta = *idToMeta[metaId];

			if (meta.errorCode != 0 || meta.errorMessage.length() > 0)
			{
				if (buffer) SafeFormat(buffer, nBytesInBuffer, "%s", meta.errorMessage.c_str());
				errorCode = meta.errorCode;
				return true;
			}

			errorCode = 0;
			if (buffer) *buffer = 0;
			return false;
		}

		ID_SXY_META RefreshSXYStatus(cstr path) override
		{
			SXYMeta meta;

			try
			{
				WIN32_FILE_ATTRIBUTE_DATA data;
				if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data))
				{
					Throw(GetLastError(), "Could not retrieve file attributes");
				}

				if (data.nFileSizeHigh > 0 || data.nFileSizeLow > 1_megabytes)
				{
					meta.errorCode = HR_FILE_TOO_LARGE;
					meta.errorMessage = "file too large";
				}

				WideFilePath wPath;
				Format(wPath, L"%hs", path);
				Auto<ISourceCode> src = sparser->LoadSource(wPath, { 1,1 });

				try
				{
					Auto<ISParserTree> s_tree = sparser->CreateTree(*src);
				}
				catch (IException&)
				{
					throw;
				}
			}
			catch (IException& ex)
			{
				meta.errorCode = ex.ErrorCode();
				meta.errorMessage = ex.Message();
			}

			auto i = metadata.find(path);
			if (i == metadata.end())
			{
				meta.id = idToMeta.size() + 1;
				auto* pCopy = new SXYMeta(meta);
				metadata.insert(path, pCopy);
				idToMeta.push_back(pCopy);
			}
			else
			{
				meta.id = i->second->id;
				*i->second = meta;
			}

			return meta.id;
		}
	};

	ISXYMetaTable* CreateSXYMetaTable()
	{
		return new SXYMetaTable();
	}

	void UpdateItem(IGuiTree& tree, ID_TREE_ITEM idTree, ID_SXY_META metaId)
	{
		
	}

	void PopulateTreeWithSXYFiles(IGuiTree& tree, cstr contentFolder, ISXYMetaTable& metaTable, IIDEFrame& frame)
	{
		tree.Clear();

		auto hRoot = tree.AppendItem(0);
		tree.SetItemText(contentFolder, hRoot);

		struct ANON : IEventCallback<IO::FileItemData>
		{
			float count = 0;
			IIDEFrame* frame;

			void OnEvent(IO::FileItemData& item) override
			{
				if (item.isDirectory)
				{
					char progress[1024];
					SafeFormat(progress, "Evaluating directory...\r\n   %ws", item.fullPath);
					frame->SetProgress(0.0f, progress);
				}
				count += 1.0f;
			}
		} fileCounter;

		fileCounter.frame = &frame;

		struct ANONCOUNTER : IEventCallback<IO::FileItemData>
		{
			IGuiTree* tree;
			ISXYMetaTable* metaTable;
			IIDEFrame* frame;
			float totalCount = 0;
			float count = 0;

			void OnEvent(IO::FileItemData& item) override
			{
				count += 1.0f;

				auto idItem = tree->AppendItem((ID_TREE_ITEM) item.containerContext);

				U8FilePath itemText;
				Format(itemText, "%ws", item.itemRelContainer);
				tree->SetItemText(itemText, idItem);

				if (item.isDirectory)
				{
					tree->SetItemImage(idItem, 0);
					tree->SetItemExpandedImage(idItem, 1);

					char progressText[1024];
					SafeFormat(progressText, "Scanning directory...\r\n  %ws", item.fullPath);

					float percent = clamp(totalCount == 0 ? 50.0f : 100.0f * count / totalCount, 0.0f, 99.9f);
					frame->SetProgress(percent, progressText);
				}
				else
				{
					if (Rococo::EndsWith(item.fullPath, L".sxy"))
					{
						U8FilePath u8Path;
						Format(u8Path, "%ws", item.fullPath);

						char progressText[1024];
						SafeFormat(progressText, "Parsing SXY file...\r\n  %ws", item.fullPath);

						float percent = clamp(totalCount == 0 ? 50.0f : 100.0f * count / totalCount, 0.0f, 99.9f);
						frame->SetProgress(percent, progressText);

						ID_SXY_META id = metaTable->RefreshSXYStatus(u8Path);
						tree->SetContext(idItem, id);

						UpdateItem(*tree, idItem, id);

						tree->SetItemImage(idItem, 2);
					}
					else
					{
						tree->SetItemImage(idItem, 3);
					}
				}

				item.outContext = (void*) idItem;
			}
		} cb;

		cb.tree = &tree;
		cb.metaTable = &metaTable;
		cb.frame = &frame;

		WideFilePath path;
		Format(path, L"%hs", contentFolder);

		try
		{
			Rococo::IO::ForEachFileInDirectory(path, fileCounter, true, nullptr);
			cb.totalCount = fileCounter.count;
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
	}

	void Run()
	{
		
	}
}