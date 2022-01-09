#include "sexystudio.impl.h"
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <sexy.types.h>

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>

#include <rococo.io.h>

#include <rococo.hashtable.h>
#include <vector>

#include <rococo.package.h>

namespace Rococo::SexyStudio
{
	using namespace Rococo::Sex;

	bool TryGetShortPackageName(U8FilePath& path, cstr packagePath)
	{
		auto* end = GetFinalNull(packagePath);
		auto* start = packagePath;

		char* p = const_cast<char*>(end);

		p -= 5;

		if (p < start || !Eq(p, ".sxyz"))
		{
			return false;
		}

		end = p;

		p -= 1;

		while (p >= start)
		{
			if (*p == '\\' || *p == '/')
			{
				p++;
				memcpy(path.buf, p, end - p);
				path.buf[end - p] = 0;
				return true;
			}

			p--;
		}

		return false;
	}

	void PopulateBranchWithSTree(IGuiTree& tree, ISParserTree& srcTree)
	{
		auto branchId = tree.AppendItem(0);
		tree.SetItemText(srcTree.Source().Name(), branchId);
	}

	void PopulateTreeWithSXYFiles(IGuiTree& tree, cstr contentFolder, ISexyDatabase& database, IIDEFrame& frame)
	{
		tree.Clear();
		database.Clear();

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
		} incFileCounter;

		incFileCounter.frame = &frame;

		struct ANONCOUNTER : IEventCallback<IO::FileItemData>
		{
			IGuiTree* tree;
			ISexyDatabase* database;
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

						database->UpdateFile_SXY(u8Path);

						tree->SetContext(idItem, 0);

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
		cb.database = &database;
		cb.frame = &frame;

		WideFilePath path;
		Format(path, L"%hs", contentFolder);

		try
		{
			Rococo::IO::ForEachFileInDirectory(path, incFileCounter, true, nullptr);
			cb.totalCount = incFileCounter.count;
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

		database.Sort();
	}

	void PopulateDatabaseFromPackageDirectory(IPackage& package, ISexyDatabase& database, cstr dirname)
	{
		struct ANON : IEventCallback<cstr>
		{
			IPackage* package;
			ISexyDatabase* database;

			void OnEvent(cstr filename)
			{
				PackageFileData pfd;
				package->GetFileInfo(filename, pfd);
				if (pfd.filesize < 1_megabytes)
				{
					database->UpdateFile_SXY_PackedItem(pfd.data, (int32)pfd.filesize, pfd.name);
				}
			}
		} onFile;
		onFile.package = &package;
		onFile.database = &database;

		package.BuildFileCache(dirname);
		package.ForEachFileInCache(onFile);
	}

	void PopulateTreeWithPackages(cstr searchPath, cstr packageFolder, ISexyDatabase& database)
	{
		WideFilePath widePath;
		Assign(widePath, packageFolder);

		U8FilePath asciiName;
		Format(asciiName, "%s", packageFolder);

		U8FilePath packageShortName;
		if (!TryGetShortPackageName(packageShortName, packageFolder))
		{
			Format(packageShortName, "-unknown-%llu", Rococo::OS::CpuTicks());
		}

		AutoFree<IPackageSupervisor> package = OpenZipPackage(widePath, packageShortName);

		package->BuildDirectoryCache(searchPath);

		struct ANON : IEventCallback<cstr>
		{
			IPackage* package;
			ISexyDatabase* database;

			void OnEvent(cstr dirname)
			{
				PopulateDatabaseFromPackageDirectory(*package, *database, dirname);
			}
		} onDir;
		onDir.package = package;
		onDir.database = &database;

		package->ForEachDirInCache(onDir);
	}

	void Run()
	{
		
	}
}