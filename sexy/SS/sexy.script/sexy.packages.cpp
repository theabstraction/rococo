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

	struct PackageNamespaceToken
	{
		char token[PACKAGE_TOKEN_CAPACITY];
		std::vector<PackageNamespaceToken> subspaces;
		std::vector<PackageFileData> files;

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

	struct SexyPackager
	{
		IPackage* dataPackage;
		PackageNamespaceToken root;

		SexyPackager(IPackage* p):
			dataPackage(p), 
			root("", 10)
		{

		}

		~SexyPackager()
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
			ns.files.push_back(data);
		}

		void MapFilesAtLevelToNamespaces(PackageNamespaceToken& ns, const char* resourcePath)
		{
			dataPackage->BuildFileCache(resourcePath);

			struct A : IEventCallback<cstr>
			{
				SexyPackager* This;
				PackageNamespaceToken* ns;
				void OnEvent(cstr path) override
				{
					if (EndsWith(path, ".sxy"))
					{
						PackageFileData f;
						This->dataPackage->GetFileInfo(path, f);
						This->AddFileToSubspaces(*ns, f);
					}
				}
			} addFilenames;
			addFilenames.This = this;
			addFilenames.ns = &ns;

			dataPackage->ForEachFileInCache(addFilenames);
		}

		void ComputeNamespace(PackageNamespaceToken& ns, const char* resourcePath)
		{
			MapDirectoriesAtLevelToNamespaces(ns, resourcePath);
			MapFilesAtLevelToNamespaces(ns, resourcePath);

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

				ComputeNamespace(subspace, subpath);
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

		bool IsPublisherForNamespace(const INamespace& ns) const
		{
			auto* fullname = ns.FullName();

			auto* start = fullname->Buffer;
			auto* end = start + fullname->Length;

			auto* p = start;
			auto* branch = &root;

			while (p < end)
			{
				char c = *p;
				if (c == '.')
				{
					for (auto& subspace : branch->subspaces)
					{
						if (subspace.DoesTokenMatch(start, p))
						{
							branch = &subspace;
							start = p + 1;
							goto advanceToNextToken;
						}
					}

					// Mismatch - no child matches our namespace
					return false;
				}

			advanceToNextToken:
				p++;
			}

			for (auto& subspace : branch->subspaces)
			{
				if (subspace.DoesTokenMatch(start, end))
				{
					return true;
				}
			}

			return false;
		}
	};

	struct Packager: ISexyPackagerSupervisor
	{
		std::unordered_map<StringKey, SexyPackager, StringKey::Hash> packages;

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
			SexyPackager pkg(package);
			auto k = packages.insert(std::make_pair(key, pkg)).first;
			k->second.ComputeNamespace(k->second.root, "");
			k->second.RemoveEmptySubspaces(k->second.root);

			return true;
		}

		void LoadNamespaceAndRecurse(const PackageNamespaceToken& ns)
		{

		}

		void LoadSubpackages(cstr namespaceFilter, cstr packageName)
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
				LoadNamespaceAndRecurse(*ns);
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

			LoadNamespaceAndRecurse(*ns);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Script
{
	ISexyPackagerSupervisor* CreatePackager()
	{
		return new Packager();
	}
}
