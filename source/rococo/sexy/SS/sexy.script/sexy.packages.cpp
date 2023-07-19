#include "sexy.script.stdafx.h"
#include <rococo.package.h>
#include <rococo.hashtable.h>
#include <sexy.vector.h>
#include <sexy.s-parser.h>

using namespace Rococo;
using namespace Rococo::Script;

#ifdef __APPLE__
# define _alloca alloca
#endif

namespace
{
	enum { PACKAGE_TOKEN_CAPACITY = 16 };

	struct SexyPackage;

	struct PackedFile: ISourceCode
	{
		SexyPackage& parent;
		PackageFileData data;
		mutable ISParserTree* tree = nullptr;
		HString id;

		PackedFile(const PackageFileData& _data, SexyPackage& _parent);

		~PackedFile()
		{
			if (tree)
			{
				tree->Release();
			}
		}

		const Vec2i& Origin() const override
		{
			static Vec2i origin = { 1,1 };
			return origin;
		}

		cstr SourceStart() const override
		{
			return data.data;
		}

		const int SourceLength() const override
		{
			return (int32) (int64) data.filesize;
		}

		cstr Name() const override
		{
			return id;
		}

		refcount_t AddRef() override
		{
			return 1;
		}

		refcount_t Release() override
		{
			return 0;
		}

		ISourceCode& ToBase() const
		{
			const ISourceCode& This = *this;
			return const_cast<ISourceCode&>(This);
		}
	};

	struct PackageNamespaceToken
	{
		char token[PACKAGE_TOKEN_CAPACITY];
		TSexyVector<PackageNamespaceToken> subspaces;
		TSexyVector<PackedFile> files;

		PackageNamespaceToken(const char* text, int32 nSubspaces)
		{
			CopyString(token, PACKAGE_TOKEN_CAPACITY, text);
			subspaces.reserve(nSubspaces);
		}

		bool DoesTokenMatch(const char* __restrict start, const char* __restrict end) const
		{
			auto* t = token;
			for (auto* c = start; c < end; ++c)
			{
				if (*t++ != *c)
				{
					return false;
				}
			}

			return *t == 0;
		}

		const PackageNamespaceToken* Find(const char* __restrict start, const char* __restrict end) const
		{
			for (auto& i : subspaces)
			{
				if (i.DoesTokenMatch(start, end))
				{
					return &i;
				}
			}

			return nullptr;
		}
	};

	// Assumes terminated by dot
	const char* ValidateSexyFileNameAndRetEnd(const char* filename)
	{
		if (filename[0] < 'A' || filename[0] > 'Z')
		{
			Throw(0, "Expecting first character in '%s' to be capital A-Z", filename);
		}

		const char* p;
		for (p = filename + 1; *p != '.'; p++)
		{
			if (!IsAlphaNumeric(*p))
			{
				auto dif = (int64) (p - filename);
				Throw(0, "Expecting character at position %lld in '%s'", dif, filename);
			}
		}

		if (!Eq(p, ".sxy"))
		{
			Throw(0, "Expecting only one dot '%s'", filename);
		}

		auto dif = (int64)(p - filename);
		if (dif >= PACKAGE_TOKEN_CAPACITY)
		{
			Throw(0, "Expecting fewer than %d characters '%s'", PACKAGE_TOKEN_CAPACITY, filename);
		}

		return p;
	}

	bool IsSexyNamespaceToken(const char* subdir)
	{
		if (subdir[0] < 'A' || subdir[0] > 'Z')
		{
			return false;
		}

		const char* p;
		for (p = subdir + 1; *p != 0; p++)
		{
			if (!IsAlphaNumeric(*p))
			{
				return false;
			}
		}

		auto dif = (int64)(p - subdir);
		if (dif >= PACKAGE_TOKEN_CAPACITY)
		{
			return false;
		}

		return true;
	}

	struct SexyPackage
	{
		IPackage* dataPackage;
		PackageNamespaceToken root;

		SexyPackage(IPackage* p):
			dataPackage(p), 
			root("", 10)
		{

		}

		~SexyPackage()
		{

		}

		void MapDirectoriesAtLevelToNamespaces(PackageNamespaceToken& ns, const char* resourcePath)
		{
			dataPackage->BuildDirectoryCache(resourcePath);

			struct A: IEventCallback<cstr>
			{
				PackageNamespaceToken& ns;
				A(PackageNamespaceToken& _ns) : ns(_ns) {}

				void OnEvent(const char* path) override
				{
					// Paths end with slash, which we need to strip
					auto len = strlen(path);
					auto* path2 = reinterpret_cast<char*>(_alloca(len));
					memcpy_s(path2, len, path, len - 1);

					const char* subspaceToken = path2;
					for (auto i = 0; i < len - 1; ++i)
					{
						if (path[i] == '/') subspaceToken = path2 + i + 1;
					}
					path2[len - 1] = 0;

					if (IsSexyNamespaceToken(subspaceToken))
					{
						ns.subspaces.push_back(PackageNamespaceToken(subspaceToken, 0));
					}
				}
			} buildSubspace(ns);

			dataPackage->ForEachDirInCache(buildSubspace);
		}

		void AddFileToSubspaces(PackageNamespaceToken& ns, const PackageFileData& data)
		{
			ns.files.push_back(PackedFile(data, *this));
		}

		void MapFilesAtLevelToNamespaces(IPublicScriptSystem& ss, PackageNamespaceToken& ns, const char* resourcePath)
		{
			dataPackage->BuildFileCache(resourcePath);

			struct A : IEventCallback<cstr>
			{
				SexyPackage* This;
				PackageNamespaceToken* ns;
				IPublicScriptSystem* ss;

				void OnEvent(cstr path) override
				{
					if (EndsWith(path, ".sxy"))
					{
						PackageFileData f;
						This->dataPackage->GetFileInfo(path, f);

						if (f.filesize < 0x7FFFFFFFLL)
						{
							// ISourceCode source length is int32 so we can't handle more than that
							// in any case do we really want to treat files larger than 2GB as anything
							// other than an error?
							This->AddFileToSubspaces(*ns, f);
						}
						else
						{
							// Most package implementations will probably prohibit 2GB files, so this
							// code may well never be called in human existence
							char msg[1024];
							SafeFormat(msg, "This may well be a unique event in human history:\n%s was not less than 2GB in length, and will not be referenced in the package", f.name.buf);
							ss->PublicProgramObject().Log().Write(msg);
						}
					}
				}
			} addFilenames;
			addFilenames.This = this;
			addFilenames.ns = &ns;
			addFilenames.ss = &ss;

			dataPackage->ForEachFileInCache(addFilenames);
		}

		void ComputeNamespace(IPublicScriptSystem& ss, PackageNamespaceToken& ns, const char* resourcePath)
		{
			MapDirectoriesAtLevelToNamespaces(ns, resourcePath);
			MapFilesAtLevelToNamespaces(ss, ns, resourcePath);

			for (auto& subspace: ns.subspaces)
			{
				U8FilePath subpath;
				if (*resourcePath == 0)
				{
					SafeFormat(subpath.buf, "%s/", subspace.token);
				}
				else
				{
					SafeFormat(subpath.buf, "%s%s/", resourcePath, subspace.token);
				}

				ComputeNamespace(ss, subspace, subpath);
			}
		}

		void RemoveEmptySubspaces(PackageNamespaceToken& ns)
		{
			for (auto& subspace : ns.subspaces)
			{
				RemoveEmptySubspaces(subspace);
			}

			auto j = std::remove_if(ns.subspaces.begin(), ns.subspaces.end(),
				[](PackageNamespaceToken& subspace)
				{
					return subspace.files.empty() && subspace.subspaces.empty();
				});

			ns.subspaces.erase(j, ns.subspaces.end());
		}
	};

	struct Packager: ISexyPackagerSupervisor
	{
		IScriptSystem& ss;
		stringmap<SexyPackage> packages;

		Packager(IScriptSystem& _ss) : ss(_ss) {}

		bool RegisterNamespacesInPackage(IPackage* package) override
		{
			if (package == nullptr)
			{
				Throw(0, "%hs: package argument was null", __FUNCTION__);
			}

			cstr name = package->FriendlyName();
			if (name == nullptr)
			{
				Throw(0, "%hs: package name was null", __FUNCTION__);
			}

			if (strstr(name, "@") != nullptr)
			{
				Throw(0, "%hs: package name contained illegal character '@'", __FUNCTION__);
			}

			for (auto& i : packages)
			{
				if (i.second.dataPackage->HashCode() == package->HashCode())
				{
					return false;
				}
			}

			auto key = package->FriendlyName();
			auto j = packages.find(key);
			if (j != packages.end())
			{
				Throw(0, "%s: A package of the same name '%s' is already registered",
					__FUNCTION__, (cstr)key);
			}

			SexyPackage pkg(package);
			auto k = packages.insert(key, pkg).first;
			k->second.ComputeNamespace(ss, k->second.root, "");
			k->second.RemoveEmptySubspaces(k->second.root);

			return true;
		}

		static bool DoesFileMatchFilter(cstr subspace, cstr filename, cstr filter)
		{
			auto* f = filename + strlen(subspace) + 1;
			auto* g = filter;
			for (;;)
			{
				switch (*g)
				{
				case '*':
					return true;
				case 0:
					return *f == 0;
				default:
					if (*f != *g) return false; else break;
				}

				g++;
				f++;
			}
		}

		static bool MapFileNameToNamespace(char defaultNamespace[NAMESPACE_MAX_LENGTH], cstr filename)
		{
			auto* f = filename; // Sys/Maths/I32/Double.sxy
			
			auto lenPathname = strlen(f);

			if (lenPathname >= NAMESPACE_MAX_LENGTH)
			{
				Throw(0, "Pathname too long %s", f);
			}

			memcpy(defaultNamespace, f, lenPathname); //  Sys/Maths/I32/Double.sxy
			defaultNamespace[lenPathname] = 0;

			auto* d = defaultNamespace + lenPathname;
			while (d > defaultNamespace)
			{
				if (*d == '/')
				{
					*d = 0; // Sys/Maths/I32
					break;
				}

				d--;
			}

			if (d == defaultNamespace)
			{
				// This means we had a filename not under a directory, in which case
				// there can be no default namespace, ergo, it cannot be listed in a package
				return false;
			}

			for (d = defaultNamespace; *d != 0; d++)
			{
				switch (*d)
				{
				case '/':
					*d = '.'; break;
				case '.':
					return false; // Dots not allowed
				default:
					break;
				}
			}

			return true;
		}

		template<class T> size_t LoadFilesWhere(const PackageNamespaceToken& ns, SexyPackage& sp, T condition)
		{
			size_t count = 0;

			for (auto& file : ns.files)
			{
				auto* f = file.Name(); // Package[double]@Sys/Maths/I32/Double.sxy 
				while (*f++ != '@') {} // Sys/Maths/I32/Double.sxy 

				char defaultNamespace[NAMESPACE_MAX_LENGTH];
				if (condition(f) && MapFileNameToNamespace(defaultNamespace, f))
				{
					if (file.tree == nullptr)
					{
						try
						{
							file.tree = ss.SParser().CreateTree((ISourceCode&)file);
						}
						catch (ParseException& pex)
						{
							char err[2048];
							SafeFormat(err, "%s: %s", file.Name(), pex.Message());
							ParseException pex2(pex.Start(), pex.End(), pex.Name(), err, pex.Specimen(), pex.Source());
							throw pex2;
						}
						catch (IException& ex)
						{
							Throw(ex.ErrorCode(), "Error parsing %s\n%s", file.Name(), ex.Message());
						}
					}
					auto* module = static_cast<IModuleBuilder*>(ss.AddTree(*file.tree));
					module->SetPackage(sp.dataPackage, defaultNamespace);
					count++;
				}
			}

			return count;
		}

		void LoadNamespaceAndRecurse(OUT size_t& count, cstr namespaceText, const PackageNamespaceToken& ns, SexyPackage& sp)
		{
			size_t fileCount = LoadFilesWhere(ns, sp,
				[](const char* /* filename */)
				{
					return true;
				}
			);

			count += fileCount;

			// recurse
			for (auto& subspace : ns.subspaces)
			{
				if (*namespaceText == 0)
				{
					LoadNamespaceAndRecurse(count, subspace.token, subspace, sp);
				}
				else
				{
					char subPrefix[NAMESPACE_MAX_LENGTH];
					SecureFormat(subPrefix, "%s.%s", namespaceText, subspace.token);
					LoadNamespaceAndRecurse(count, subPrefix, subspace, sp);
				}
			}
		}

		[[noreturn]] static void ThrowSubspaceMismatch(const PackageNamespaceToken* ns, cstr start)
		{
			if (ns->subspaces.empty())
			{
				Throw(0, "Could not match subspace %s. The parent [%s] has no subspaces", start, ns->token);
			}
			else
			{
				char subspaces[512];
				StackStringBuilder ssb(subspaces, 512);
				for (auto& sub : ns->subspaces)
				{
					ssb.AppendFormat("[%s] ", sub.token);
				}
				Throw(0, "Could not match subspace %s to anything in the parent namespace %s\nKnown subspaces: %s\n", start, ns->token, subspaces);
			}
		}

		static auto GetSubspace(cstr namespaceText, SexyPackage& package)
		{
			const auto* ns = &package.root;

			const char* start = namespaceText;
			const char* end;
			const char* s;

			for (s = namespaceText; *s != 0; ++s)
			{
				if (*s == '.')
				{
					end = s;

					auto ns1 = ns->Find(start, end);
					if (ns1 == nullptr)
					{
						ThrowSubspaceMismatch(ns, start);
					}
					ns = ns1;

					start = s + 1;
					end = start;
				}
			}

			end = s;

			auto ns1 = ns->Find(start, end);
			if (ns1 == nullptr)
			{
				ThrowSubspaceMismatch(ns, start);
			}

			return ns1;
		}

		void LoadSubpackagesWithSearchFilter(cstr namespaceFilter, SexyPackage& package)
		{
			// Sys.Maths.I32.Doub* 

			const char* n;
			for (n = namespaceFilter + strlen(namespaceFilter); n > namespaceFilter; n--)
			{
				if (*n == '.')
				{
					n++;
					break;
				}
			}

			if (n == namespaceFilter)
			{
				Throw(0, "The search filter did not refer to a subspace. Filters have the format A.B.C.xyz*");
			}

			// n = Doub*

			auto len = n - namespaceFilter;
			if (len >= NAMESPACE_MAX_LENGTH)
			{
				Throw(0, "The search filter subspace was too long");
			}

			auto* filenameFilter = n;

			char namespaceText[NAMESPACE_MAX_LENGTH];
			memcpy(namespaceText, namespaceFilter, len-1);
			namespaceText[len-1] = 0; // namespaceText = Sys.Maths.I32

			auto subspace = GetSubspace(namespaceText, package);

			size_t fileCount = LoadFilesWhere(*subspace, package,
				[namespaceText, filenameFilter](const char* filename)
				{
					return DoesFileMatchFilter(namespaceText, filename, filenameFilter);
				}
			);

			if (fileCount == 0) Throw(0, "No files were enumerated under [%s] with filter '%s'", namespaceText, filenameFilter);
		}

		void LoadSubpackagesRecursively(cstr namespaceText, SexyPackage& package)
		{
			const PackageNamespaceToken* subspace;

			if (namespaceText == nullptr || *namespaceText == 0)
			{
				namespaceText = "";
				subspace = &package.root;
			}
			else
			{
				subspace = GetSubspace(namespaceText, package);
			}

			size_t count = 0;
			LoadNamespaceAndRecurse(OUT count, namespaceText, *subspace, package);
			if (count == 0) Throw(0, "No files were enumerated under the root");
		}

		void LoadSubpackages(cstr namespaceFilter, cstr packageName) override
		{
			auto key = packageName;
			auto i = packages.find(key);
			if (i == packages.end())
			{
				Throw(0, "Packager::LoadSubpackages(..., \"%s\"): could not find package", packageName);
			}

			if (namespaceFilter && EndsWith(namespaceFilter, "*"))
			{
				try
				{
					LoadSubpackagesWithSearchFilter(namespaceFilter, i->second);
				}
				catch (IException& ex)
				{
					Throw(ex.ErrorCode(), "Packager::LoadSubpackages(\"%s\", \"%s\")\n%s", namespaceFilter, packageName, ex.Message());
				}
			}
			else
			{
				try
				{
					LoadSubpackagesRecursively(namespaceFilter, i->second);
				}
				catch (IException& ex)
				{
					cstr ns = namespaceFilter == nullptr ? "<null>" : namespaceFilter;
					Throw(ex.ErrorCode(), "Packager::LoadSubpackages(%s, \"%s\")\n%s", ns, packageName, ex.Message());
				}
			}
		}

		void Free() override
		{
			delete this;
		}
	};

	PackedFile::PackedFile(const PackageFileData& _data, SexyPackage& _parent) :
		parent(_parent), data(_data)
	{
		char name[1024];
		SafeFormat(name, "Package[%s]@%s", parent.dataPackage->FriendlyName(), data.name.buf);
		id = name;
	}
}

namespace Rococo::Script
{
	ISexyPackagerSupervisor* CreatePackager(IScriptSystem& ss)
	{
		return new Packager(ss);
	}
}
