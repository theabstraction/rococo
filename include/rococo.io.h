#ifndef ROCOCO_IO_H
#define ROCOCO_IO_H

namespace Rococo
{
	namespace IO
	{
		struct NO_VTABLE IStreamer
		{
			virtual void Close() = 0; // Closes the IO object responsible for the stream
			virtual const wchar_t* Name() const = 0; // Gives a resource identifier for the stream, such as a filename
			virtual void Free() = 0; // Delete the object that implements the streamer. Free will call Close before deleting the object to which the interface refers
		};

		struct NO_VTABLE IReader : public IStreamer
		{
			virtual size_t Read(char* buffer, size_t capacity) = 0;
		};

		struct NO_VTABLE IFixedLengthReader : public IReader
		{
			virtual int64 Length() const = 0;
		};

		struct NO_VTABLE IBinaryWriter : public IStreamer
		{
			virtual void Write(const uint8* buffer, uint32 nBytes) = 0;
		};

		struct NO_VTABLE IUnicode16Writer
		{
			virtual void Append(const wchar_t* format, ...) = 0;
		};

		struct NO_VTABLE IUnicode16WriterSupervisor : public IUnicode16Writer
		{
			virtual void Free() = 0;
		};

		struct NO_VTABLE IScriptGenerator
		{
			virtual void AppendStringLiteral(IUnicode16Writer& writer, const wchar_t* text) = 0;
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
	}

	struct NO_VTABLE IBuffer
	{
		virtual uint8* GetData() = 0;
		virtual size_t Length() const = 0;
	};

	struct NO_VTABLE IExpandingBuffer: public IBuffer
	{
		virtual void Resize(size_t length) = 0;
		virtual void Free() = 0;
	};

	IExpandingBuffer* CreateExpandingBuffer(size_t initialCapacity);

	struct NO_VTABLE IOS
	{
		virtual void LoadResource(const wchar_t* resourcePath, IExpandingBuffer& buffer, int64 maxFileLength) = 0;
	};
}

#endif