#ifndef ROCOCO_IO_H
#define ROCOCO_IO_H

namespace Rococo
{
	ROCOCO_INTERFACE IBuffer
	{
		virtual uint8 * GetData() = 0;
		virtual const uint8* GetData() const = 0;
		virtual size_t Length() const = 0;
	};

	ROCOCO_INTERFACE IExpandingBuffer : public IBuffer
	{
		virtual void Resize(size_t length) = 0;
		virtual void Free() = 0;
	};


	// TODO, add a rerence to an allocator
	ROCOCO_API IExpandingBuffer* CreateExpandingBuffer(size_t initialCapacity);

	struct SysUnstableArgs {};
	struct FileModifiedArgs
	{
		const wchar_t* sysPath;
		ROCOCO_API bool Matches(cstr pingPath) const;
	};

	struct MemoryUsage
	{
		uint64 current;
		uint64 peak;
	};

	ROCOCO_API MemoryUsage ProcessMemory();
}

namespace Rococo::IO
{
	enum TargetDirectory
	{
		TargetDirectory_UserDocuments = 0,
		TargetDirectory_Root
	};

	ROCOCO_INTERFACE ISysMonitor
	{
		// call this periodically
		virtual void DoHousekeeping() = 0;
	};

	ROCOCO_INTERFACE IShaderMonitor: ISysMonitor
	{
		// Add an include path
		virtual void AddIncludePath(cstr path) = 0;

		// A define to everything compiled
		virtual void AddMacro(cstr name, cstr value) = 0;

		// compiles everything in the directory and its descendants in alphabetical order. If null, enumerates all hlsl files in the monitor directory
		virtual void CompileDirectory(cstr path) = 0;

		// when hlsl files are changed in this directory or its descendants they will be recompiled .
		virtual void SetMonitorDirectory(cstr path) = 0;

		virtual uint64 QueueLength() const = 0;

		// Delete the shader monitor
		virtual void Free() = 0;
	};

	enum class EShaderLogPriority
	{
		Compiled,
		Cosmetic,
		Info,
		Warning,
		ErrorCode,
		Error
	};

	ROCOCO_INTERFACE IShaderMonitorEventHook
	{
		virtual void OnLog(IShaderMonitor& monitor, EShaderLogPriority priority, cstr file, cstr message) = 0;
	};

	ROCOCO_INTERFACE IShaderMonitorEvents: IShaderMonitorEventHook
	{
		virtual void OnModifiedFileSkipped(IShaderMonitor& monitor, cstr hlslFile) = 0;
	};

	ROCOCO_INTERFACE IShaderMonitorEventsProxy
	{
		virtual void AddHook(IShaderMonitorEventHook * hook) = 0;
		virtual void RemoveHook(IShaderMonitorEventHook* hook) = 0;
	};

	struct IUnicode16Writer;
	ROCOCO_API bool ChooseDirectory(char* name, size_t capacity);
	ROCOCO_API bool ChooseDirectory(char* name, size_t capacity, cstr title);
	ROCOCO_API bool IsDirectory(const wchar_t* filename);
	ROCOCO_API bool IsDirectory(cstr filename);
	ROCOCO_API void EnsureUserDocumentFolderExists(const wchar_t* subdirectory);
	ROCOCO_API void SaveAsciiTextFile(TargetDirectory target, const wchar_t* filename, const fstring& text);
	ROCOCO_API void SaveAsciiTextFile(TargetDirectory target, cstr filename, const fstring& text);
	ROCOCO_API bool StripLastSubpath(wchar_t* fullpath);
	ROCOCO_API bool IsFileExistant(const char* path);
	ROCOCO_API bool IsFileExistant(const wchar_t* path);
	ROCOCO_API void ToSysPath(wchar_t* path);
	ROCOCO_API void ToUnixPath(wchar_t* path);
	ROCOCO_API void ToSysPath(char* path);
	ROCOCO_API void ToUnixPath(char* path);
	ROCOCO_API void SanitizePath(char* path);
	ROCOCO_API void SanitizePath(wchar_t* path);
	ROCOCO_API void SaveBinaryFile(cstr targetPath, const uint8* buffer, size_t nBytes);
	ROCOCO_API void SaveBinaryFile(const wchar_t* targetPath, const uint8* buffer, size_t nBytes);
	ROCOCO_API void GetExeName(U8FilePath & path);
	ROCOCO_API void GetExePath(U8FilePath& path);

	// Open a file and fit into buffer. In the case of a truncation an IException is thrown. The function returns the number of bytes copied to the buffer.
	ROCOCO_API size_t LoadAsciiTextFile(char* data, size_t capacity, const wchar_t* filename);
	ROCOCO_API size_t LoadAsciiTextFile(char* data, size_t capacity, cstr filename);
	ROCOCO_API void LoadAsciiTextFile(Strings::IStringPopulator& callback, const wchar_t* filename);

	ROCOCO_INTERFACE IBinaryFileLoader
	{
		virtual uint8* LockWriter(size_t length) = 0;
		virtual void Unlock() = 0;
	};

	ROCOCO_API void LoadBinaryFile(IBinaryFileLoader& loader, const wchar_t* filename, uint64 maxLength);
	ROCOCO_API void LoadBinaryFile(IBinaryFileLoader& loader, const char* filename, uint64 maxLength);

	ROCOCO_API bool MakeContainerDirectory(char* filename);
	ROCOCO_API bool MakeContainerDirectory(wchar_t* filename);

	struct FileItemData
	{
		const wchar_t* fullPath;
		const wchar_t* containerRelRoot;
		const wchar_t* itemRelContainer;
		void* containerContext;
		void* outContext;
		bool isDirectory;
	};

	struct U8FileItemData
	{
		cstr fullPath;
		cstr containerRelRoot;
		cstr itemRelContainer;
		void* containerContext;
		void* outContext;
		bool isDirectory;
	};

	// Gets the current director path sans end slashes
	ROCOCO_API void GetCurrentDirectoryPath(U8FilePath& path);

	ROCOCO_API void ForEachFileInDirectory(const wchar_t* directory, IEventCallback<FileItemData>& onFile, bool recurse, void* containerContext = nullptr);

	ROCOCO_API void ForEachFileInDirectory(const char* directory, IEventCallback<U8FileItemData>& onFile, bool recurse, void* containerContext = nullptr);

	ROCOCO_INTERFACE IPathCache
	{
		virtual void AddPathsFromDirectory(const char* directory, bool recurse) = 0;
		virtual void Sort() = 0;
		virtual size_t NumberOfFiles() const = 0;
		virtual cstr GetFileName(size_t index) const = 0;
	};

	ROCOCO_INTERFACE IPathCacheSupervisor: IPathCache
	{
		virtual void Free() = 0;
	};

	ROCOCO_API IPathCacheSupervisor* CreatePathCache();

	struct FileAttributes
	{
		uint64 fileLength;
		char timestamp[24];
	};

	ROCOCO_API bool TryGetFileAttributes(const wchar_t* sysPath, FileAttributes& attr);

	void NormalizePath(WideFilePath& path);

	ROCOCO_API void ToU8(const U32FilePath& src, U8FilePath& dest);
	ROCOCO_API void ToWide(const U32FilePath& src, WideFilePath& dest);
	ROCOCO_API void PathFromAscii(cstr ascii_string, U32FilePath& path);
	ROCOCO_API void PathFromWide(const wchar_t* wide_string, U32FilePath& path);

	// Creates a directory at the specified path. If it does not exist and the operation fails an exception is thrown
	ROCOCO_API void CreateDirectoryFolder(const WideFilePath& path);
	ROCOCO_API void CreateDirectoryFolder(const U8FilePath& path);
	ROCOCO_API char DirectorySeparatorChar();
	ROCOCO_API void UseBufferlessStdout();

	ROCOCO_INTERFACE IStreamer
	{
		virtual void Close() = 0; // Closes the IO object responsible for the stream
		virtual cstr Name() const = 0; // Gives a resource identifier for the stream, such as a filename
		virtual void Free() = 0; // Delete the object that implements the streamer. Free will call Close before deleting the object to which the ROCOCO_INTERFACE refers
	};

	ROCOCO_INTERFACE IReader : public IStreamer
	{
		virtual size_t Read(char* buffer, size_t capacity) = 0;
	};

	ROCOCO_INTERFACE IFixedLengthReader : public IReader
	{
		virtual int64 Length() const = 0;
	};

	ROCOCO_INTERFACE IBinaryWriter : public IStreamer
	{
		virtual void Write(const uint8* buffer, uint32 nBytes) = 0;
	};

	ROCOCO_INTERFACE IUnicode16Writer
	{
		virtual void Append(const wchar_t* format, ...) = 0;
	};

	ROCOCO_INTERFACE IUnicode16WriterSupervisor : public IUnicode16Writer
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IScriptGenerator
	{
		virtual void AppendStringLiteral(IUnicode16Writer& writer, cstr text) = 0;
	};

	ROCOCO_INTERFACE IBinaryArchive
	{
		virtual void Reserve(uint64 nBytes) = 0;
		virtual void SeekAbsolute(uint64 position) = 0;
		virtual uint64 Position() const = 0;
		virtual void Write(size_t sizeOfElement, size_t nElements, const void* pElements) = 0;
		virtual void Free() = 0;
		virtual void Truncate() = 0;

		template<typename T> inline auto& Write(const T& t)
		{
			Write(sizeof(T), 1, &t);
			return *this;
		}
	};

	ROCOCO_API IBinaryArchive* CreateNewBinaryFile(const wchar_t* sysPath);

	ROCOCO_INTERFACE IBinarySource
	{
		virtual uint32 Read(uint32 capacity, void* pElements) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_API IBinarySource* ReadBinarySource(const wchar_t* sysPath);

	ROCOCO_INTERFACE IReadOnlyBinaryMapping
	{
		virtual const char* Data() const = 0;
		virtual const uint64 Length() const = 0;
		virtual void Free() = 0;
	};

	ROCOCO_API IReadOnlyBinaryMapping* CreateReadOnlyBinaryMapping(const wchar_t* sysPath);

	class FileImage
	{
	private:
		char* data;
		size_t len;

		FileImage(FileImage& src) = delete;
		FileImage& operator = (FileImage& src) = delete;

	public:
		ROCOCO_API FileImage(IFixedLengthReader& reader);
		ROCOCO_API ~FileImage();

		char* Data() { return data; }
		const char* Data() const { return data; }
		size_t Length() const { return len; }
	};

	ROCOCO_API void Print(IBinaryWriter& writer, const char* format, ...);
	ROCOCO_API void SaveUserFile(cstr filename, cstr s);

	ROCOCO_INTERFACE ITableRowData
	{
		virtual int32 NextInt32() = 0;
		virtual int64 NextInt64() = 0;
		virtual float NextFloat32() = 0;
		virtual double NextFloat64() = 0;
		virtual bool NextBool() = 0;

		// Retrieve a string. Valid until the next method call or the calling function returns.
		virtual fstring NextTempString() = 0;
	};

	struct TableRowHeaders
	{
		int NumberOfRows;
		cstr ExcelFile;
		cstr TableName;
	};

	enum class ColumnType: int32
	{
		UnderlyingTypeInt32,
		UnderlyingTypeInt64,
		UnderlyingTypeFloat32,
		UnderlyingTypeFloat64,
		UnderlyingTypeBool,
		UnderlyingTypeUTF8
	};

	struct ColumnHeader
	{
		cstr name;
		ColumnType type;
	};

	ROCOCO_INTERFACE ITableRowBuilder
	{
		virtual void OnColumns(int numberOfColumns, const ColumnHeader* headers) = 0;
		virtual void OnHeaders(const TableRowHeaders& headers) = 0;
		virtual void OnRow(ITableRowData & row) = 0;
	};

	ROCOCO_API void ValidateHeader(const ColumnHeader& archiveHeader, ColumnType cppType, cstr archiveFile);
	ROCOCO_API void ParseTableRows(IBinarySource& source, ITableRowBuilder& builder);
	ROCOCO_API void ParseTableRows(cstr sourcePath, ITableRowBuilder& builder);
	ROCOCO_API void ParseTableRows(const IInstallation& installation, cstr pingPath, ITableRowBuilder& builder);

	ROCOCO_INTERFACE ILoadEventReader
	{
		virtual void ReadData(void* buffer, uint32 capacity, uint32 & bytesRead) = 0;
	};

	ROCOCO_INTERFACE ILoadEventsCallback
	{
		virtual void OnFileOpen(int64 fileLength) = 0;
		virtual void OnDataAvailable(ILoadEventReader& reader) = 0;
	};

	// The (I)nput/(O)utput (S)ystem
	ROCOCO_INTERFACE IOS
	{
		virtual void ConvertUnixPathToSysPath(const wchar_t* unixPath, WideFilePath & sysPath) const = 0;
		virtual void EnumerateModifiedFiles(IEventCallback<FileModifiedArgs>& cb) = 0;

		// Call if the system has become unstable due to bad assets et al
		virtual void FireUnstable() = 0;
		virtual void SetUnstableHandler(IEventCallback<SysUnstableArgs>* cb) = 0;

		// Not terminated with a slash
		virtual void GetBinDirectoryAbsolute(WideFilePath& binDirectory) const = 0;

		virtual bool IsFileExistant(cstr absPath) const = 0;
		virtual bool IsFileExistant(const wchar_t* absPath) const = 0;
		virtual void LoadAbsolute(const wchar_t* absPath, IExpandingBuffer& buffer, int64 maxFileLength) const = 0;
		virtual void LoadAbsolute(const wchar_t* absPath, ILoadEventsCallback& cb) const = 0;
		virtual size_t MaxPath() const = 0;
		virtual void Monitor(const wchar_t* absPath) = 0;
		virtual bool TryLoadAbsolute(const wchar_t* absPath, ILoadEventsCallback& cb, ErrorCode& sysErrorCode) const = 0;
	};

	ROCOCO_INTERFACE IInstallation
	{
		virtual bool TryExpandMacro(cstr macroPrefixPlusPath, U8FilePath & expandedPath) = 0;
		virtual const wchar_t* Content() const = 0;
		virtual void LoadResource(cstr pingPath, IExpandingBuffer& buffer, int64 maxFileLength) = 0;
		virtual void LoadResource(cstr resourcePath, ILoadEventsCallback& cb) = 0;
		virtual bool TryLoadResource(cstr pingPath, IExpandingBuffer& buffer, int64 maxFileLength) = 0;
		virtual bool TryLoadResource(cstr pingPath, ILoadEventsCallback& cb, OUT ErrorCode& errorCode) = 0;
		virtual void ConvertPingPathToSysPath(cstr pingPath, WideFilePath& path) const = 0;
		virtual void ConvertPingPathToSysPath(cstr pingPath, U8FilePath& path) const = 0;
		virtual void ConvertSysPathToMacroPath(const wchar_t* sysPath, U8FilePath& pingPath, cstr macro) const = 0;
		virtual void ConvertSysPathToPingPath(const wchar_t* sysPath, U8FilePath& pingPath) const = 0;
		virtual void ConvertSysPathToPingPath(const char* sysPath, U8FilePath& pingPath) const = 0;
		virtual bool DoPingsMatch(cstr a, cstr b) const = 0;
		virtual void Macro(cstr name, cstr pingFolder) = 0;
		virtual void CompressPingPath(cstr pingPath, U8FilePath& resultPath) const = 0;
		virtual IOS& OS() = 0;
	};

	ROCOCO_INTERFACE IInstallationSupervisor : public IInstallation
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IOSSupervisor : public IOS
	{
		virtual void Free() = 0;
	};

	ROCOCO_API IOSSupervisor* GetIOS();

	ROCOCO_API IInstallationSupervisor* CreateInstallation(const wchar_t* contentIndicatorName, IOS& ios);
	ROCOCO_API IInstallationSupervisor* CreateInstallationDirect(const wchar_t* contentDirectory, IOS& ios);

	ROCOCO_API bool DoesModifiedFilenameMatchResourceName(cstr modifiedFilename, cstr resourceName);

	enum { MAX_PATHLEN = 260 };
	[[nodiscard]] constexpr char GetFileSeparator() noexcept
	{
#ifdef _WIN32
		return '\\';
#else
		return '/';
#endif
	}

	[[nodiscard]] constexpr const char* GetFileSeparatorString() noexcept
	{
#ifdef _WIN32
		return "\\";
#else
		return "/";
#endif
	}
	ROCOCO_API void EndDirectoryWithSlash(char* pathname, size_t capacity);
	ROCOCO_API void EndDirectoryWithSlash(wchar_t* pathname, size_t capacity);
#ifdef _WIN32 // Windows may have UNICODE16 characters in the username, so ascii insufficient to hold the user path
	ROCOCO_API void GetUserPath(wchar_t* fullpath, size_t capacity, cstr shortname);
	inline void GetUserPath(WideFilePath& fullpath, cstr shortname)
	{
		GetUserPath(fullpath.buf, WideFilePath::CAPACITY, shortname);
	}
#else
	ROCOCO_API void GetUserPath(char* fullpath, size_t capacity, cstr shortname);
#endif
	ROCOCO_API void DeleteUserFile(cstr filename);

	ROCOCO_API bool TrySwapExtension(U8FilePath& path, cstr expectedExtension, cstr newExtenson);
}

#endif