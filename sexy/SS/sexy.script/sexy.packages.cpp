#include "sexy.script.stdafx.h"
#include <rococo.package.h>
#include <rococo.strings.h>
#include <vector>
#include <unordered_map>

using namespace Rococo;
using namespace Rococo::Script;

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

		refcount_t AddRef()
		{
			return 1;
		}

		refcount_t Release()
		{
			return 0;
		}

		operator ISourceCode&() const
		{
			const ISourceCode& This = *this;
			return const_cast<ISourceCode&>(This);
		}
	};

	struct PackageNamespaceToken
	{
		char token[PACKAGE_TOKEN_CAPACITY];
		std::vector<PackageNamespaceToken> subspaces;
		std::vector<PackedFile> files;

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
		std::unordered_map<StringKey, SexyPackage, StringKey::Hash> packages;

		Packager(IScriptSystem& _ss) : ss(_ss) {}

		bool RegisterNamespacesInPackage(IPackage* package) override
		{
			if (package == nullptr)
			{
				Throw(0, "%hs: package argument was null", __FUNCTION__);
			}

			for (auto& i : packages)
			{
				if (i.second.dataPackage->HashCode() == package->HashCode())
				{
					return false;
				}
			}

			StringKey key(package->FriendlyName());
			auto j = packages.find(key);
			if (j != packages.end())
			{
				Throw(0, "%s: A package of the same name '%s' is already registered",
					__FUNCTION__, (cstr)key);
			}

			key.Persist();
			SexyPackage pkg(package);
			auto k = packages.insert(std::make_pair(key, pkg)).first;
			k->second.ComputeNamespace(ss, k->second.root, "");
			k->second.RemoveEmptySubspaces(k->second.root);

			return true;
		}

		void LoadNamespaceAndRecurse(cstr prefix, const PackageNamespaceToken& ns, SexyPackage& sp)
		{
			for (auto& file : ns.files)
			{
				if (!file.tree)
				{
					file.tree = ss.SParser().CreateTree((ISourceCode&)file);
				}

				auto* module = static_cast<IModuleBuilder*>(ss.AddTree(*file.tree));
				module->SetPackage(sp.dataPackage, prefix);
			}

			for (auto& subspace : ns.subspaces)
			{
				if (*prefix == 0)
				{
					LoadNamespaceAndRecurse(subspace.token, subspace, sp);
				}
				else
				{
					char subPrefix[NAMESPACE_MAX_LENGTH];
					SecureFormat(subPrefix, "%s.%s", prefix, subspace.token);
					LoadNamespaceAndRecurse(subPrefix, subspace, sp);
				}
			}
		}

		void LoadSubpackages(cstr namespaceFilter, cstr packageName) override
		{
			StringKey key(packageName);
			auto i = packages.find(key);
			if (i == packages.end())
			{
				Throw(0, "%s: could not find package '%s'", __FUNCTION__, packageName);
			}

			const auto* ns = &i->second.root;
			
			if (namespaceFilter == nullptr || *namespaceFilter == 0)
			{
				LoadNamespaceAndRecurse("", *ns, i->second);
				return;
			}

			auto* start = namespaceFilter;
			auto* end = start;

			const char* s;
			for (s = namespaceFilter; *s != 0; ++s)
			{
				if (*s == '.')
				{
					end = s;

					ns = ns->Find(start, end);
					if (ns == nullptr)
					{
						Throw(0, "%s: Could not match '%s' to anything in the package %s",
							__FUNCTION__, namespaceFilter, i->second.dataPackage->FriendlyName());
					}

					start = s + 1;
					end = start;
				}
			}

			end = s;

			ns = ns->Find(start, end);
			if (ns == nullptr)
			{
				Throw(0, "%s: Could not match '%s' to anything in the package %s",
					__FUNCTION__, namespaceFilter, i->second.dataPackage->FriendlyName());
			}

			LoadNamespaceAndRecurse(namespaceFilter, *ns, i->second);
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
