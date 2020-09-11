#include <rococo.api.h>
#include <rococo.package.h>
#include <rococo.io.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zlib.h>
#include <vector>

using namespace Rococo;

namespace
{
	struct ZipPackage : IPackageSupervisor
	{
		ZipPackage(const wchar_t* filename, const char* key)
		{

		}

		cstr UniqueName() const override
		{
			return nullptr;
		}

		void LoadFileImageForCopying(const char* resourcePath, IFileHandler& handler) override
		{

		}

		void LoadFileImageIntoBuffer(const char* resourcePath, void* buffer, int64 capacity) override
		{

		}

		void GetFileInfo(const char* resourcePath, int index, SubPackageData& pkg) const override
		{

		}

		void GetDirectoryInfo(const char* resourcePath, int index, SubPackageData& pkg) const override
		{

		}

		int CountDirectories(const char* resourcePath) const override
		{
			return 0;
		}

		int CountFiles(const char* resourcePath) const override
		{
			return 0;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo
{
	IPackageSupervisor* OpenZipPackage(const wchar_t* sysPath, const char* friendlyName)
	{
		return new ZipPackage(sysPath, friendlyName);
	}
}