#include <rococo.api.h>
#include <rococo.io.h>
#include <rococo.file.browser.h>
#include <rococo.strings.h>

#include <vector>

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::Events;
using namespace Rococo::Browser;

template<typename T> void ForEachSubPath(const FilePath<T>& dir, IEventCallback<FilePath<T>>& cb, size_t skipStartChars, T sep)
{
	FilePath<T> tempCharBuffer = { 0 };

	T* p = tempCharBuffer.buf;

	for (const T* s = dir.buf + skipStartChars; *s != 0; ++s)
	{
		T c = *s;
		if (c != sep)
		{
			*p++ = c;
		}
		else
		{
			*p++ = sep;
			*p = 0;
			cb.OnEvent(tempCharBuffer);
			if (s[1] == 0) return;
		}
	}

	if (p != tempCharBuffer.buf)
	{
		*p++ = sep;
		*p++ = 0;
		cb.OnEvent(tempCharBuffer);
	}
}

template<class T> size_t Length(const T* s)
{
	const T* p = s;
	for (; *p != 0; p++)
	{

	}

	return p - s;
}

void AddPathSeparator(U32FilePath& target, char32_t c)
{
	size_t len = Length(target.buf);
	if (len + 1 >= target.CAPACITY)
	{
		Throw(0, "AddPathSeparator(target): length > CAPACITY");
	}

	target.buf[len] = c;
	target.buf[len + 1] = 0;
}

void Merge(U32FilePath& target, const char32_t* prefix, const U32FilePath& suffix)
{
	size_t lenPrefix = Length(prefix);
	size_t lenSuffix = Length(suffix.buf);

	if (lenPrefix + lenSuffix + 1 >= target.CAPACITY)
	{
		Throw(0, "Merge(target, prefix, suffix): Cannot merge paths. Combined length > CAPACITY");
	}

	memcpy(target.buf, prefix, sizeof(char32_t) * lenPrefix);

	for (size_t i = 0; i < lenSuffix; ++i)
	{
		target.buf[i + lenPrefix] = suffix[i];
	}

	target.buf[lenPrefix + lenSuffix] = 0;
}

struct PingPopulator : public IDirectoryPopulator
{
	U32FilePath initialDirectory;
	IInstallation& installation;

	U8FilePath currentDirectory;

	std::vector<U32FilePath> subDirectories;

	struct FileInfo
	{
		U32FilePath path;
		char timestamp[32];
		uint64 osFileTime;
		size_t length;
	};
	std::vector<FileInfo> subFiles;

	U8FilePath requiredPrefix = { "!" };

	PingPopulator(IInstallation& _installation) :
		installation(_installation)
	{
		Format(currentDirectory, "!");
		Populate();
	}

	~PingPopulator()
	{
		ClearSubdirs();
	}

	size_t FileCount() const override
	{
		return subFiles.size();
	}

	size_t DirectoryCount() const override
	{
		return subDirectories.size();
	}

	const U32FilePath& GetFile(size_t index, uint64& fileLength, FileTimestamp& timestamp) const
	{
		if (index >= subFiles.size()) Throw(0, "PingPopulator::GetFile: index out of range");

		auto& f = subFiles[index];
		fileLength = f.length;
		timestamp.osFileTime = f.osFileTime;
		SafeFormat(timestamp.text, sizeof(timestamp.text), "%s", f.timestamp);
		return f.path;
	}

	const U32FilePath& GetDirectory(size_t index) const
	{
		if (index >= subDirectories.size()) Throw(0, "PingPopulator::GetDirectory: index out of range");

		auto& d = subDirectories[index];
		return d;
	}

	void SetCurrentDirectory(const U32FilePath& path) override
	{
		auto& dir = currentDirectory;
		int depth = 0;

		U8FilePath asciiPath;
		ToU8(path, asciiPath);

		if (!StartsWith(asciiPath, requiredPrefix))
		{
			asciiPath = requiredPrefix;
		}

		if (Eq(asciiPath, "!"))
		{
			Format(currentDirectory, "!");
			Populate();
			return;
		}

		if (asciiPath[0] == '!')
		{
			currentDirectory = asciiPath;
		}
		else
		{
			Format(currentDirectory, "!%s", asciiPath);
		}

		Populate();
	}

	void ForEachSubPathFromCurrent(IEventCallback<U32FilePath>& cb) override
	{
		U32FilePath root;
		PathFromAscii("!", '/', root);
		cb.OnEvent(root);

		U32FilePath u32current;
		PathFromAscii(currentDirectory, '/', u32current);

		ForEachSubPath<char32_t>(u32current, cb, 1, '/');
	}

	void EnumerateSubfolders(const U32FilePath& root, IFileCallback& cb, bool recurse)
	{
		U8FilePath pingPath;
		ToU8(root, pingPath);

		WideFilePath sysPath;
		installation.ConvertPingPathToSysPath(pingPath, sysPath);

		struct : public IEventCallback<IO::FileItemData>
		{
			IFileCallback* cb;
			const wchar_t* sysPath;
			U32FilePath root;

			void OnEvent(IO::FileItemData& item) override
			{
				if (item.isDirectory)
				{
					if (*item.containerRelRoot == 0) // Only interested in the first layer of folders
					{
						U8FilePath buf; // ascii should be fine, since we are in a subdir of a ping path
						Format(buf, "%ls%ls", item.containerRelRoot, item.itemRelContainer);
						U32FilePath itemPath;
						PathFromAscii(buf, '/', itemPath);
						cb->OnFile(root, itemPath, nullptr, 0);
					}
				}
			}
		} invokeCallback;

		invokeCallback.cb = &cb;
		invokeCallback.sysPath = sysPath;
		invokeCallback.root = root;

		Rococo::IO::ForEachFileInDirectory(sysPath, invokeCallback, recurse);
	}

	void EnumerateFiles(const U32FilePath& root, IFileCallback& cb, bool recurse)
	{
		U8FilePath pingPath;
		ToU8(root, pingPath);

		WideFilePath sysPath;
		installation.ConvertPingPathToSysPath(pingPath, sysPath);

		struct : public IEventCallback<IO::FileItemData>
		{
			IFileCallback* cb;
			const wchar_t* sysPath;
			U32FilePath root;
			void OnEvent(IO::FileItemData& item) override
			{
				if (!item.isDirectory)
				{
					if (*item.containerRelRoot == 0) // Only interested in the first layer of folders
					{
						char buf[260];
						SafeFormat(buf, 260, "%ls%ls", item.containerRelRoot, item.itemRelContainer);
						U32FilePath itemPath;
						PathFromAscii(buf, '/', itemPath);

						IO::FileAttributes attr;
						if (IO::TryGetFileAttributes(item.fullPath, attr))
						{
							cb->OnFile(root, itemPath, attr.timestamp, attr.fileLength);
						}
						else
						{
							cb->OnFile(root, itemPath, "", 0);
						}
					}
				}
			}
		} invokeCallback;

		invokeCallback.cb = &cb;
		invokeCallback.sysPath = sysPath;
		invokeCallback.root = root;

		Rococo::IO::ForEachFileInDirectory(sysPath, invokeCallback, recurse);
	}

	void ClearSubdirs()
	{
		subDirectories.clear();
		subFiles.clear();
	}

	void Populate()
	{
		ClearSubdirs();

		struct : IFileCallback
		{
			std::vector<U32FilePath>* subDirectories;

			void OnFile(const U32FilePath& root, const U32FilePath& subpath, cstr timestamp, uint64 length) override
			{
				subDirectories->push_back(subpath);
				AddPathSeparator(subDirectories->back(), '/');
			}
		} addToSubList;

		addToSubList.subDirectories = &subDirectories;

		U32FilePath u32current;
		PathFromAscii(currentDirectory, '/', u32current);

		EnumerateSubfolders(u32current, addToSubList, false);

		struct : IFileCallback
		{
			std::vector<FileInfo>* subFiles;

			void OnFile(const U32FilePath& root, const U32FilePath& subpath, cstr timestamp, uint64 length) override
			{
				FileInfo info;
				info.path = subpath;
				SafeFormat(info.timestamp, sizeof(info.timestamp), "%s", timestamp);
				info.length = length;
				subFiles->push_back(info);
			}
		} addToFileList;
		addToFileList.subFiles = &subFiles;

		EnumerateFiles(u32current, addToFileList, false);
	}

	void Free() override
	{
		delete this;
	}

	void GetFullPath(U32FilePath& fullPath, const U32FilePath& subdir) const
	{
		U32FilePath u32current;
		PathFromAscii(currentDirectory, '/', u32current);

		Merge(fullPath, u32current, subdir);
	}

	void GetFullPathToFile(const U32FilePath& shortFileName, U32FilePath& fullPath) const
	{
		U32FilePath u32current;
		PathFromAscii(currentDirectory, '/', u32current);

		Merge(fullPath, u32current, shortFileName);
	}

	void LimitRoot(const U32FilePath& prefix) override
	{
		ToU8(prefix, requiredPrefix);
		if (!EndsWith(requiredPrefix, "/"))
		{
			Throw(0, "PingPopulator::LimitRoot(%s). Prefix did not end with a '/'", (cstr)requiredPrefix);
		}

		if (!StartsWith(currentDirectory, requiredPrefix))
		{
			SetCurrentDirectory(prefix);
		}
	}
};

namespace Rococo
{
	namespace Browser
	{
		IDirectoryPopulator* CreatePingPopulator(IInstallation& installation)
		{
			return new PingPopulator(installation);
		}
	}
}