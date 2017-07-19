#ifndef ROCOCO_IO_H
#define ROCOCO_IO_H

namespace Rococo
{
	namespace IO
	{
      void UseBufferlessStdout();

		ROCOCOAPI IStreamer
		{
			virtual void Close() = 0; // Closes the IO object responsible for the stream
			virtual cstr Name() const = 0; // Gives a resource identifier for the stream, such as a filename
			virtual void Free() = 0; // Delete the object that implements the streamer. Free will call Close before deleting the object to which the ROCOCOAPI refers
		};

		ROCOCOAPI IReader : public IStreamer
		{
			virtual size_t Read(char* buffer, size_t capacity) = 0;
		};

		ROCOCOAPI IFixedLengthReader : public IReader
		{
			virtual int64 Length() const = 0;
		};

		ROCOCOAPI IBinaryWriter : public IStreamer
		{
			virtual void Write(const uint8* buffer, uint32 nBytes) = 0;
		};

		ROCOCOAPI IUnicode16Writer
		{
			virtual void Append(const wchar_t* format, ...) = 0;
		};

		ROCOCOAPI IUnicode16WriterSupervisor : public IUnicode16Writer
		{
			virtual void Free() = 0;
		};

		ROCOCOAPI IScriptGenerator
		{
			virtual void AppendStringLiteral(IUnicode16Writer& writer, cstr text) = 0;
		};

		class FileImage
		{
		private:
			char* data;
			size_t len;

			FileImage(FileImage& src) = delete;
			FileImage& operator = (FileImage& src) = delete;

		public:
			FileImage(IFixedLengthReader& reader);
			~FileImage();

			char* Data() { return data; }
			const char* Data() const { return data; }
			size_t Length() const { return len; }
		};

		void Print(IBinaryWriter& writer, const char* format, ...);
      void SaveUserFile(cstr filename, cstr s);
	}

	ROCOCOAPI IBuffer
	{
		virtual uint8* GetData() = 0;
		virtual const uint8* GetData() const = 0;
		virtual size_t Length() const = 0;
	};

	ROCOCOAPI IExpandingBuffer: public IBuffer
	{
		virtual void Resize(size_t length) = 0;
		virtual void Free() = 0;
	};

	IExpandingBuffer* CreateExpandingBuffer(size_t initialCapacity);

	struct SysUnstableArgs {};
   struct FileModifiedArgs
   {
      cstr resourceName;
      bool Matches(cstr resource);
      void GetPingPath(rchar* path, size_t capacity);
   };

   struct MemoryUsage
   {
      uint64 current;
      uint64 peak;
   };

   MemoryUsage ProcessMemory();

	ROCOCOAPI IOS
	{
		virtual void ConvertUnixPathToSysPath(cstr unixPath, rchar* sysPath, size_t bufferCapacity) const = 0;
		virtual void EnumerateModifiedFiles(IEventCallback<FileModifiedArgs>& cb) = 0;

		// Call if the system has become unstable due to bad assets et al
		virtual void FireUnstable() = 0;
		virtual void SetUnstableHandler(IEventCallback<SysUnstableArgs>* cb) = 0;
		virtual void GetBinDirectoryAbsolute(rchar* binDirectory, size_t capacityChars) const = 0;
		virtual bool IsFileExistant(cstr absPath) const = 0;
		virtual void LoadAbsolute(cstr absPath, IExpandingBuffer& buffer, int64 maxFileLength) const = 0;
		virtual size_t MaxPath() const = 0;
		virtual void Monitor(cstr absPath) = 0;
		virtual void UTF8ToUnicode(const char* s, wchar_t* unicode, size_t cbUtf8count, size_t unicodeCapacity) = 0;
	};

	ROCOCOAPI IInstallation
	{
		virtual cstr Content() const = 0;
		virtual void LoadResource(cstr resourcePath, IExpandingBuffer& buffer, int64 maxFileLength) = 0;
		virtual IOS& OS() = 0;
	};

	ROCOCOAPI IInstallationSupervisor: public IInstallation
	{
		virtual void Free() = 0;
	};

	ROCOCOAPI IOSSupervisor: public IOS
	{
		virtual void Free() = 0;
	};

   IOSSupervisor* GetOS();

	IInstallationSupervisor* CreateInstallation(cstr contentIndicatorName, IOS& os);

	bool DoesModifiedFilenameMatchResourceName(cstr modifiedFilename, cstr resourceName);

   namespace IO
   {
      enum { MAX_PATHLEN = 260 };
      rchar GetFileSeparator();
      void EndDirectoryWithSlash(rchar* pathname, size_t capacity);
      void GetUserPath(rchar* fullpath, size_t capacity, cstr shortname);
      void DeleteUserFile(cstr filename);
   }
}

#endif