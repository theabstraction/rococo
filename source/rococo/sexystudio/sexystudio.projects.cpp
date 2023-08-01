#include "sexystudio.impl.h"
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <sexy.types.h>

#include <rococo.auto-release.h>
#include <rococo.io.h>
#include <rococo.os.h>

#include <rococo.hashtable.h>
#include <vector>

#include <rococo.package.h>

namespace
{
	using namespace Rococo::SexyStudio;

	struct SourceTree : ISourceTree
	{
		struct Item
		{
			HString path;
			int lineNumber;
		};
		std::unordered_map<ID_TREE_ITEM, Item> items;

		void Add(ID_TREE_ITEM item, cstr text, int lineNumber) override
		{
			items[item] = { text, lineNumber };
		}

		void Clear() override
		{
			items.clear();
		}

		SourceAndLine Find(ID_TREE_ITEM item) const override
		{
			auto i = items.find(item);
			return i != items.end() ? SourceAndLine { i->second.path.c_str(), i->second.lineNumber } : SourceAndLine { nullptr, 0 };
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::SexyStudio
{
	using namespace Rococo;
	using namespace Rococo::Sex;

	ISourceTree* CreateSourceTree()
	{
		return new SourceTree();
	}

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
		tree.SetItemText(branchId, srcTree.Source().Name());
	}

	void PopulateTreeWithSXYFiles(IGuiTree& tree, ISexyDatabase& database, IIDEFrame& frame, ISourceTree& sourceTree)
	{
		tree.Clear();
		database.Clear();

		auto hRoot = tree.AppendItem(0);
		tree.SetItemText(hRoot, database.Solution().GetScriptFolder());

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
			ISourceTree* sourceTree;
			float totalCount = 0;
			float count = 0;

			void OnEvent(IO::FileItemData& item) override
			{
				count += 1.0f;

				auto idItem = tree->AppendItem((ID_TREE_ITEM) item.containerContext);

				U8FilePath itemText;
				Format(itemText, "%ws", item.itemRelContainer);
				tree->SetItemText(idItem, itemText);

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

						sourceTree->Add(idItem, u8Path, 1);
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
		cb.sourceTree = &sourceTree;

		WideFilePath scriptPath;
		Format(scriptPath, L"%hs", database.Solution().GetScriptFolder());

		if (!Rococo::IO::IsDirectory(scriptPath))
		{
			auto hError = tree.AppendItem(hRoot);
			tree.SetItemText(hError, "...directory not found");
			return;
		}

		try
		{
			Rococo::IO::ForEachFileInDirectory(scriptPath, incFileCounter, true, nullptr);
			cb.totalCount = incFileCounter.count;
			Rococo::IO::ForEachFileInDirectory(scriptPath, cb, true, (void*) hRoot);
		}
		catch (IException& ex)
		{
			auto hError = tree.AppendItem(hRoot);
			tree.SetItemText(hError, "Error recursing content folder");
			auto hErrorMsg = tree.AppendItem(hError);
			tree.SetItemText(hErrorMsg, ex.Message());
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

	void PopulateDatabaseWithPackagesRecursive(IPackage& package, ISexyDatabase& database, cstr searchPath)
	{
		package.BuildDirectoryCache(searchPath);

		struct ANON : IEventCallback<cstr>
		{
			IPackage* package;
			ISexyDatabase* database;
			std::vector<HString> subdirectories;

			void OnEvent(cstr dirname)
			{
				PopulateDatabaseFromPackageDirectory(*package, *database, dirname);
				subdirectories.push_back(dirname);
			}
		} onDir;
		onDir.package = &package;
		onDir.database = &database;

		package.ForEachDirInCache(onDir);

		for (auto& subdir : onDir.subdirectories)
		{
			PopulateDatabaseWithPackagesRecursive(package, database, subdir);
		}
	}

	void PopulateTreeWithPackages(cstr packageFolder, ISexyDatabase& database)
	{
		WideFilePath widePath;
		Assign(widePath, packageFolder);

		U8FilePath asciiName;
		Format(asciiName, "%s", packageFolder);

		U8FilePath packageShortName;
		if (!TryGetShortPackageName(packageShortName, packageFolder))
		{
			Format(packageShortName, "-unknown-%llu", Rococo::Time::TickCount());
		}

		AutoFree<IPackageSupervisor> package = OpenZipPackage(widePath, packageShortName);

		PopulateDatabaseWithPackagesRecursive(*package, database, "");
	}

	void BuildDatabaseFromProject(ISexyDatabase& database, cr_sex sProjectRoot, cstr projectPath, bool addNativeDeclarations)
	{
		database.UpdateFile_SXY(projectPath);

		cstr natives[] =
		{
			"!scripts/native/Sys.Maths.sxy",
			"!scripts/native/Sys.Reflection.sxy",
			"!scripts/native/Sys.Type.Strings.sxy",
			"!scripts/native/Sys.Type.sxy",
		};

		for (auto nativePath : natives)
		{
			U8FilePath sysPath;
			database.PingPathToSysPath(nativePath, sysPath);
			database.UpdateFile_SXY(sysPath);
		}

		int bestPriority = INT_MAX;
		cstr bestDeclarationsFile = nullptr;

		for (int i = 0; i < sProjectRoot.NumberOfElements(); ++i)
		{
			auto& sTopLevelItem = sProjectRoot[i];
			if (sTopLevelItem.NumberOfElements() >= 3)
			{
				cr_sex sQuote = GetAtomicArg(sTopLevelItem, 0);
				cr_sex sDirective = GetAtomicArg(sTopLevelItem, 1);

				if (Eq(sQuote.c_str(), "'"))
				{
					// Raw s-expression

					if (Eq(sDirective.c_str(), "#include"))
					{
						// Include statement

						for (int j = 2; j < sTopLevelItem.NumberOfElements(); ++j)
						{
							cr_sex sPingPath = sTopLevelItem[j];

							if (IsStringLiteral(sPingPath))
							{
								cstr pingPath = sPingPath.c_str();

								U8FilePath sysPath;
								database.PingPathToSysPath(pingPath, sysPath);
								database.UpdateFile_SXY(sysPath);

								int priority = 0;
								cstr declarations = addNativeDeclarations ? database.Solution().GetDeclarationPathForInclude(pingPath, priority) : nullptr;
								if (declarations)
								{
									if (priority < bestPriority)
									{
										bestPriority = priority;
										bestDeclarationsFile = declarations;
									}
								}
							}
						}
					}
					else if (Eq(sDirective.c_str(), "#import"))
					{
						// Import statement

						for (int j = 2; j < sTopLevelItem.NumberOfElements(); ++j)
						{
							cr_sex sPackage = sTopLevelItem[j];
							if (sPackage.NumberOfElements() == 2)
							{
								cr_sex sPackageName = sPackage[0];

								if (IsAtomic(sPackageName))
								{
									cstr packageName = sPackageName.c_str();

									cstr packagePath = database.Solution().GetPackagePingPath(packageName);

									U8FilePath sysPackagePath;
									database.PingPathToSysPath(packagePath, sysPackagePath);

									PopulateTreeWithPackages(sysPackagePath, database);

									int priority = 0;
									cstr declarations = addNativeDeclarations ? database.Solution().GetDeclarationPathForImport(packageName, priority) : nullptr;
									if (declarations)
									{
										if (priority < bestPriority)
										{
											bestPriority = priority;
											bestDeclarationsFile = declarations;
										}
									}
								}
							}
						}
					}
				}
			}
		}

		if (addNativeDeclarations && bestDeclarationsFile)
		{
			U8FilePath sysPath;
			database.PingPathToSysPath(bestDeclarationsFile, sysPath);
			database.UpdateFile_SXY(sysPath);
		}
	}

	void Run()
	{
		
	}
}