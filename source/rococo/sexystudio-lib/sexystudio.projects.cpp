#include "sexystudio.api.h"
#include "sexystudio.database.h"
#include <rococo.strings.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <sexy.types.h>

#include <rococo.auto-release.h>
#include <rococo.io.h>
#include <rococo.os.h>

#include <rococo.hashtable.h>
#include <vector>

#include <rococo.package.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::SexyStudio;

namespace
{
	struct SourceTree : ISourceTree
	{
		struct Item
		{
			HString path;
			int lineNumber;
		};
		std::unordered_map<SOURCE_TREE_ITEM_ID, Item> items;

		stringmap<int> mapSrcToCount;

		void Add(SOURCE_TREE_ITEM_ID item, cstr text, int lineNumber) override
		{
			U8FilePath nameAndLineNumber;
			Format(nameAndLineNumber, "%s#%d", text, lineNumber);
			auto i = mapSrcToCount.insert(nameAndLineNumber, 1);
			if (!i.second)
			{
				// Already existed
				i.first->second++;
			}
			else
			{
				items[item] = { text, lineNumber };
			}
		}

		void Clear() override
		{
			items.clear();
			mapSrcToCount.clear();
		}

		SourceAndLine Find(SOURCE_TREE_ITEM_ID item) const override
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

	SEXYSTUDIO_API ISourceTree* CreateSourceTree()
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

	void PopulateDatabaseFromPackageDirectory(IPackage& package, ISexyDatabase& database, cstr dirname)
	{
		struct ANON : Strings::IStringPopulator
		{
			IPackage* package;
			ISexyDatabase* database;

			void Populate(cstr filename) override
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

		struct ANON : Strings::IStringPopulator
		{
			IPackage* package;
			ISexyDatabase* database;
			std::vector<HString> subdirectories;

			void Populate(cstr dirname) override
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

	SEXYSTUDIO_API void PopulateTreeWithPackage(cstr sysPackagePath, ISexyDatabase& database)
	{
		WideFilePath widePath;
		Assign(widePath, sysPackagePath);

		U8FilePath asciiName;
		Format(asciiName, "%s", sysPackagePath);

		U8FilePath packageShortName;
		if (!TryGetShortPackageName(packageShortName, sysPackagePath))
		{
			Format(packageShortName, "-unknown-%llu", Rococo::Time::TickCount());
		}

		if (!database.HasResource(sysPackagePath))
		{
			database.MarkResource(sysPackagePath);

			AutoFree<IPackageSupervisor> package = OpenZipPackage(widePath, packageShortName);

			PopulateDatabaseWithPackagesRecursive(*package, database, "");
		}
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
			database.PingPathResolver().PingPathToSysPath(nativePath, sysPath);
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
								database.PingPathResolver().PingPathToSysPath(pingPath, sysPath);
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
									database.PingPathResolver().PingPathToSysPath(packagePath, sysPackagePath);

									PopulateTreeWithPackage(sysPackagePath, database);

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
			database.PingPathResolver().PingPathToSysPath(bestDeclarationsFile, sysPath);
			database.UpdateFile_SXY(sysPath);
		}
	}

	void Run()
	{
		
	}
}