#include "sexy.script.stdafx.h"
#include <rococo.package.h>
#include <rococo.strings.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Script;

namespace
{
	enum { PACKAGE_TOKEN_CAPACITY = 16 };

	struct PackageNamespaceToken
	{
		char token[PACKAGE_TOKEN_CAPACITY];
		std::vector<PackageNamespaceToken> subspaces;
		int filecount = 0;
		bool hasDirectory = false;

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
			root("", p->CountDirectories(nullptr) + p->CountFiles(nullptr))
		{

		}

		~SexyPackager()
		{

		}

		void MapDirectoriesAtLevelToNamespaces(PackageNamespaceToken& ns, const char* resourcePath)
		{
			int32 nDirectories = dataPackage->CountDirectories(resourcePath);
			for (int i = 0; i < nDirectories; ++i)
			{
				SubPackageData dir;
				dataPackage->GetDirectoryInfo(resourcePath, i, dir);

				if (IsSexyNamespaceToken(dir.name))
				{
					ns.subspaces.push_back(PackageNamespaceToken(dir.name, 0));
					ns.subspaces.back().hasDirectory = true;
				}
			}
		}

		void AddFilenameToSubspaces(const char* token, PackageNamespaceToken& ns, const char* resourcePath)
		{
			auto i = std::find_if(ns.subspaces.begin(), ns.subspaces.end(),
				[token](const PackageNamespaceToken& subspace)
				{
					return Eq(token, subspace.token);
				});

			if (i != ns.subspaces.end())
			{
				i->filecount++;
			}
			else
			{
				ns.subspaces.push_back(PackageNamespaceToken(token, 0));
				ns.subspaces.back().filecount++;
			}
		}

		void MapFilesAtLevelToNamespaces(PackageNamespaceToken& ns, const char* resourcePath)
		{
			int32 nFiles = dataPackage->CountFiles(resourcePath);
			for (int32 i = 0; i < nFiles; ++i)
			{
				SubPackageData f;
				dataPackage->GetFileInfo(resourcePath, i, f);
				if (EndsWith(f.name, ".sxy"))
				{
					auto* end = ValidateSexyFileNameAndRetEnd(f.name);

					char token[PACKAGE_TOKEN_CAPACITY];
					char* t = token;
					for (auto* p = (cstr)f.name; p != end; ++p)
					{
						*t++ = *p++;
					}

					*t = 0;

					AddFilenameToSubspaces(token, ns, resourcePath);
				}
			}
		}

		void ComputeNamespace(PackageNamespaceToken& ns, const char* resourcePath)
		{
			MapDirectoriesAtLevelToNamespaces(ns, resourcePath);
			MapFilesAtLevelToNamespaces(ns, resourcePath);

			for (auto& subspace: ns.subspaces)
			{
				if (!subspace.hasDirectory)
					continue;

				U8FilePath subpath;
				if (resourcePath == nullptr)
				{
					SafeFormat(subpath.buf, "%s", subspace.token);
				}
				else
				{
					SafeFormat(subpath.buf, "%s/%s", resourcePath, subspace.token);
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
					return subspace.filecount == 0 && subspace.subspaces.empty();
				});

			ns.subspaces.erase(j, ns.subspaces.end());
		}

		bool ImplementsNamespace(const INamespace& ns) const
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
		std::vector<SexyPackager> uniquePackages;

		void RegisterNamespacesInPackage(IPackage* package) override
		{
			if (package == nullptr)
			{
				Throw(0, "%hs: package argument was null", __FUNCTION__);
			}

			auto i = std::find_if(uniquePackages.begin(), uniquePackages.end(), 
				[package](SexyPackager& other)
				{
					return package == other.dataPackage ||
						Eq(package->UniqueName(), other.dataPackage->UniqueName());
				});

			if (i == uniquePackages.end())
			{
				SexyPackager sxyPackage(package);
				uniquePackages.push_back(sxyPackage);
				auto& back = uniquePackages.back();
				back.ComputeNamespace(back.root, nullptr);
				back.RemoveEmptySubspaces(back.root);
			}
		}

		bool ImplementsNamespace(const INamespace& ns) const
		{
			for (auto& p : uniquePackages)
			{
				if (p.ImplementsNamespace(ns))
				{
					return true;
				}
			}
			return false;
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
