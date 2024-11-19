#include "sexy.script.stdafx.h"
#include <rococo.io.h>
#include <Sexy.S-Parser.h>
#include <rococo.ide.h>
#include <rococo.maths.h>
#include <rococo.sexy.ide.h>
#include <rococo.visitors.h>
#include <rococo.time.h>
#include <rococo.package.h>
#include <algorithm>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Sex;
using namespace Rococo::IO;
using namespace Rococo::Script;

namespace Rococo
{
	SCRIPTEXPORT_API void ThrowSex(cr_sex s, cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		char msg[512];
		SafeVFormat(msg, sizeof(msg), format, args);

		auto start = s.Start();
		auto end = s.End();

		char specimen[64];
		Rococo::Sex::GetSpecimen(specimen, s);

		ParseException ex(start, end, "ParseException", msg, specimen, &s);

		OS::TripDebugger();

		throw ex;
	}
}

namespace Rococo
{
	ISourceCode* DuplicateSourceCode(IOS& os, IExpandingBuffer& rbuffer, ISParser& parser, const IBuffer& rawData, const char* resourcePath)
	{
		const char* utf8data = (const char*)rawData.GetData();
		size_t rawLength = rawData.Length();

		if (rawLength < 2)
		{
			Throw(0, "Script file '%s' was too small", resourcePath);
		}

		if (rawLength % 2 == 0)
		{
			char bom = *utf8data;
			if (bom == 0 || (bom & 0x80))
			{
				Throw(0, "Script file '%s' was not UTF-8 or began with non-ASCII character", resourcePath);
			}
		}

		return parser.DuplicateSourceBuffer((cstr)rawData.GetData(), (int)rawData.Length(), Vec2i{ 1,1 }, resourcePath);
	}

	class SourceCache : public ISourceCache, private IMathsVenue
	{
	private:
		struct Binding
		{
			ISParserTree* tree;
			ISourceCode* code;
			Time::ticks loadTime;
		};
		TSexyStringMap<Binding> sources;
		// TODO -> allocator using the SourceCache allocator
		AutoFree<IExpandingBuffer> fileBuffer;
		// TODO -> allocator using the SourceCache allocator
		AutoFree<IExpandingBuffer> dataBuffer;
		IInstallation& installation;
		IAllocator& allocator;
		Auto<ISParser> parser;

		struct VisitorInfo
		{
			U8FilePath pingPath;
			size_t fileLength;
			char time[64];
		};
		TSexyVector<VisitorInfo> visitorData;
		TSexyVector<IPackage*> packages;

		bool allowSysPaths;
	public:
		SourceCache(IInstallation& _installation, IAllocator& _allocator, bool _allowSysPaths) :
			fileBuffer(CreateExpandingBuffer(64_kilobytes)),
			dataBuffer(CreateExpandingBuffer(64_kilobytes)),
			installation(_installation),
			allocator(_allocator),
			parser(CreateSexParser_2_0(_allocator)),
			allowSysPaths(_allowSysPaths)
		{
		}

		IAllocator& Allocator() const override
		{
			return allocator;
		}

		ISourceCache* GetInterface()
		{
			return static_cast<ISourceCache*>(this);
		}

		~SourceCache()
		{
			for (auto& i : sources)
			{
				i.second.code->Release();
				if (i.second.tree) i.second.tree->Release();
			}
		}

		IMathsVenue* Venue() override
		{
			return static_cast<IMathsVenue*>(this);
		}

		void ShowVenue(IMathsVisitor& visitor) override
		{
			if (visitorData.empty())
			{
				visitorData.reserve(sources.size());

				for (auto& i : sources)
				{
					VisitorInfo info;
					Time::FormatTime(i.second.loadTime, info.time, sizeof info.time);
					info.fileLength = i.second.code->SourceLength();
					Format(info.pingPath, "%s", (cstr)i.first);
					visitorData.push_back(info);
				}

				std::sort(visitorData.begin(), visitorData.end(),
					[](const VisitorInfo& a, const VisitorInfo& b)
					{
						return b.fileLength < a.fileLength;
					}
				);
			}

			visitor.ShowString("", "   File Length     Timestamp");

			for (auto& i : visitorData)
			{
				visitor.ShowString(i.pingPath, "%8d bytes      %8.8s", i.fileLength, i.time);
			}
		}

		void Free() override
		{
			this->~SourceCache();
			allocator.FreeData(this);
		}

		int LoadSourceAsTextFileElseReturnErrorCode(cstr resourceName, IStringPopulator& populator) override
		{
			auto i = sources.find(resourceName);
			if (i != sources.end())
			{
				populator.Populate(i->second.code->SourceStart());
				return 0;
			}

			try
			{
				installation.LoadResource(resourceName, *dataBuffer, 64_megabytes);
				populator.Populate((cstr)dataBuffer->GetData());
				return 0;
			}
			catch (IException& ex)
			{
				if (ex.ErrorCode() != 0)
				{
					return ex.ErrorCode();
				}
				else
				{
					return -1;
				}
			}
		}

		ISParserTree* GetSource(cstr pingName) override
		{
			auto i = sources.find(pingName);
			if (i != sources.end())
			{
				if (i->second.tree == nullptr)
				{
					// a null tree indicates the src was cached, but the tree generation threw an exception
					i->second.code->Release();
					sources.erase(i);
				}
				else
				{
					return i->second.tree;
				}
			}

			visitorData.clear();

			ISourceCode* src = nullptr;

			bool success = installation.TryLoadResource(pingName, *fileBuffer, 64_megabytes);
			if (success)
			{
				src = DuplicateSourceCode(installation.OS(), *dataBuffer, *parser, *fileBuffer, pingName);
			}
			else
			{
				WideFilePath sysPath;
				installation.ConvertPingPathToSysPath(pingName, sysPath);
				U8FilePath expandedPath;
				installation.ConvertSysPathToPingPath(sysPath, expandedPath);

				auto scripts = "!scripts/"_fstring;
				if (StartsWith(expandedPath, scripts))
				{
					const char* packagePath = expandedPath.buf + scripts.length;
					for (auto p : packages)
					{
						PackageFileData pfd;
						if (p->TryGetFileInfo(packagePath, pfd))
						{
							char fullname[256];
							SafeFormat(fullname, "Package[%s]@%s", p->FriendlyName(), packagePath);

							if (pfd.filesize > 0x7FFFFFFFLL)
							{
								Throw(0, "%s file length too long", fullname);
							}

							src = parser->ProxySourceBuffer(pfd.data, (int32)pfd.filesize, { 1,1 }, fullname);
							break;
						}
					}
				}

				if (src == nullptr)
				{
					Throw(0, "\nCould not find file [%s] in content directory or packages", pingName);
				}
			}

			sources[pingName] = Binding{ nullptr, src, 0 };

			// We have cached the source, so that if tree generation creates an exception, the source codes is still existant

			ISParserTree* tree = parser->CreateTree(*src);
			sources[pingName] = Binding{ tree, src, Time::UTCTime() };

			return tree;
		}

		void Release(cstr resourceName) override
		{
			visitorData.clear();

			for (auto i = sources.begin(); i != sources.end(); ++i)
			{
				if (installation.DoPingsMatch(i->first, resourceName))
				{
					i->second.tree->Release();
					i->second.code->Release();
					sources.erase(i);
					break;
				}
			}
		}

		void ReleaseAll() override
		{
			visitorData.clear();

			for (auto& i : sources)
			{
				if (i.second.tree) i.second.tree->Release();
				i.second.code->Release();
			}

			sources.clear();
		}

		void AddPackage(IPackage* package) override
		{
			// Only add a package if a package with the same hash code is not found in the source
			auto i = std::find_if(packages.begin(), packages.end(),
				[package](const IPackage* p)
				{
					return package->HashCode() == p->HashCode();
				});

			if (i == packages.end())
			{
				packages.push_back(package);
			}
		}

		void RegisterPackages(Rococo::Script::IPublicScriptSystem& ss) override
		{
			for (auto p : packages)
			{
				ss.RegisterPackage(p);
			}
		}
	};

	SCRIPTEXPORT_API ISourceCache* CreateSourceCache(IInstallation& installation, IAllocator& allocator, bool allowSysPaths)
	{
		void* buffer = allocator.Allocate(sizeof SourceCache);
		auto* cache = new (buffer) SourceCache(installation, allocator, allowSysPaths);
		return cache->GetInterface();
	}

	void AddFluffle(Rococo::Script::IPublicScriptSystem& ss, ISourceCache& sources, cstr pingPath, cr_sex s)
	{
		Throw(s, __FUNCTION__ ": not implemented");
	}

	void Preprocess(cr_sex sourceRoot, ISourceCache& sources, IScriptEnumerator& implicitIncludes, Rococo::Script::IPublicScriptSystem& ss)
	{
		bool hasIncludedFiles = false;
		bool hasIncludedNatives = false;
		bool hasIncludedImports = false;
		bool hasIncludedFluffles = false;

		size_t nImplicitIncludes = implicitIncludes.Count();
		for (size_t i = 0; i < nImplicitIncludes; ++i)
		{
			cstr implicitFile = implicitIncludes.ResourceName(i);

			if (implicitFile && *implicitFile)
			{
				try
				{
					auto includedModule = sources.GetSource(implicitFile);

					ss.ValidateSecureFile(implicitFile, includedModule->Source().SourceStart(), includedModule->Source().SourceLength());

					ss.AddTree(*includedModule);
				}
				catch (IException& ex)
				{
					Throw(0, "Error adding implicit include file %s. %s", implicitFile, ex.Message());
				}
			}
		}

		for (int i = 0; i < sourceRoot.NumberOfElements(); ++i)
		{
			cr_sex sincludeExpr = sourceRoot[i];
			if (IsCompound(sincludeExpr) && sincludeExpr.NumberOfElements() >= 3)
			{
				cr_sex squot = sincludeExpr[0];
				cr_sex stype = sincludeExpr[1];

				if (squot == "'" && stype == "#include")
				{
					if (hasIncludedFiles)
					{
						Throw(sincludeExpr, "An include directive has already been stated. Merge directives.");
					}

					hasIncludedFiles = true;

					for (int j = 2; j < sincludeExpr.NumberOfElements(); j++)
					{
						cr_sex sname = sincludeExpr[j];
						if (!IsStringLiteral(sname))
						{
							Throw(sname, "expecting string literal in include directive (' #include \"<name1>\" \"<name2 etc>\" ...) ");
						}

						auto name = sname.String();

						try
						{
							auto includedModule = sources.GetSource(name->Buffer);
							ss.AddTree(*includedModule);
						}
						catch (ParseException&)
						{
							throw;
						}
						catch (IException& ex)
						{
							Throw(sname, "Error with include file. %s", ex.Message());
						}
					}
				}
				if (squot == "'" && stype == "#fluffle")
				{
					if (hasIncludedFluffles)
					{
						Throw(sincludeExpr, "A fluffle directive has already been stated. Merge directives.");
					}

					hasIncludedFluffles = true;

					for (int j = 2; j < sincludeExpr.NumberOfElements(); j++)
					{
						cr_sex sname = sincludeExpr[j];

						if (!IsStringLiteral(sname))
						{
							Throw(sname, "expecting string literal in fluffle directive (' #fluffle \"<name1>\" \"<name2 etc>\" ...) ");
						}

						auto name = sname.String();

						try
						{
							AddFluffle(ss, sources, name->Buffer, sname);
						}
						catch (ParseException&)
						{
							throw;
						}
						catch (IException& ex)
						{
							Throw(sname, "Error with fluffle. %s", ex.Message());
						}
					}
				}
				else if (squot == "'" && stype == "#natives")
				{
					if (hasIncludedNatives)
					{
						Throw(sincludeExpr, "A natives directive has already been stated. Merge directives.");
					}

					hasIncludedNatives = true;

					for (int j = 2; j < sincludeExpr.NumberOfElements(); j++)
					{
						cr_sex sname = sincludeExpr[j];
						if (!IsStringLiteral(sname))
						{
							Throw(sname, "expecting string literal in natives directive (' #natives \"<name1>\" \"<name2 etc>\" ...) ");
						}

						auto name = sname.String();

						ss.AddNativeLibrary(name->Buffer);
					}

					break;
				}
				else if (squot == "'" && stype == "#import")
				{
					if (hasIncludedImports)
					{
						Throw(sincludeExpr, "An import directive has already been stated. Merge directives.");
					}

					hasIncludedImports = true;

					for (int j = 2; j < sincludeExpr.NumberOfElements(); j++)
					{
						cr_sex simport = sincludeExpr[j];
						if (simport.NumberOfElements() != 2)
						{
							Throw(simport, "expecting (<package> <namespace_filter>) literal in import directive");
						}

						auto packageName = GetAtomicArg(simport[0]);
						AssertStringLiteral(simport[1]);
						cstr namespaceFilter = simport[1].c_str();

						try
						{
							ss.LoadSubpackages(namespaceFilter, packageName);
						}
						catch (ParseException&)
						{
							throw;
						}
						catch (IException& ex)
						{
							Throw(simport, "%s", ex.Message());
						}
					}
				}
			}
		}
	}

	SCRIPTEXPORT_API void InitSexyScript(ISParserTree& mainModule, IDebuggerWindow& debugger, IPublicScriptSystem& ss, ISourceCache& sources, IScriptEnumerator& implicitIncludes, IScriptCompilationEventHandler& onCompile, StringBuilder* declarationBuilder)
	{
		ScriptCompileArgs args{ ss };
		onCompile.OnCompile(args);

		sources.RegisterPackages(ss);

		Preprocess(mainModule.Root(), sources, implicitIncludes, ss);

		ss.AddTree(mainModule);
		ss.Compile(declarationBuilder);
	}
} // Rococo::Script
