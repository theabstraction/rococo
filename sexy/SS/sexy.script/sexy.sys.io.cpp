#include "sexy.script.stdafx.h"

#include <cstdio>

#define ANON_NS Anon::Sys::IO

namespace ANON_NS
{
	struct IOSystem;
}

namespace Rococo::Script
{
	void AddSysIO(IScriptSystem& ss, ANON_NS::IOSystem& ioSystem);
}

namespace ANON_NS
{
	using namespace Rococo;
	using namespace Rococo::Script;

	enum FileWriterInstanceFlags
	{
		FileWriterInstanceFlags_ApplyFormat = 0x00000001,
		FileWriterInstanceFlags_LeftAlign = 0x00000002,
		FileWriterInstanceFlags_IntsAsHexadecimal = 0x00000004,
		FileWriterInstanceFlags_IntsAsUnsigned = 0x00000008,
		FileWriterInstanceFlags_FloatsAsE = 0x00000010,
		FileWriterInstanceFlags_FloatsAsG = 0x00000020,
		FileWriterInstanceFlags_ZeroPrefix = 0x00000040,
		FileWriterInstanceFlags_AddSign = 0x00000080,
		FileWriterInstanceFlags_LeadingZeroes = 0x00000100,
		FileWriterInstanceFlags_FormatString = 0x00000200
	};

#pragma pack(push,1)
	struct FileWriterInstance
	{
		ObjectStub stub;
		FILE* fp;
		int32 precision = -1;
		int32 width = -1;
		int32 flags = 0;
	};

	struct FileWriterInstanceWithInternals: FileWriterInstance
	{
		int32 formatPos = 0;
		InterfacePointer cursor;
		char formatSpec[32] = { 0 };

		void AddFlag(int flag)
		{
			flags |= flag;
		}

		void RemoveFlag(int flag)
		{
			flags &= ~flag;
		}

		bool HasFlag(int flag) const
		{
			return (flags & flag) == flag;
		}

		void UpdateFormat()
		{
			StackStringBuilder sb(formatSpec, sizeof formatSpec);
			
			sb.AppendChar('%');
			
			if (HasFlag(FileWriterInstanceFlags_LeftAlign))
			{
				sb.AppendChar('-');
			}

			if (HasFlag(FileWriterInstanceFlags_AddSign))
			{
				sb.AppendChar('+');
			}

			if (HasFlag(FileWriterInstanceFlags_LeadingZeroes))
			{
				sb.AppendChar('0');
			}

			if (width >= 0) sb << width;
			if (precision > 0)
			{
				sb.AppendChar('.');
				sb << precision;
			}

			formatPos = sb.Length();
		}
	};
#pragma pack(pop)

	struct IOSystem : IIOSystem
	{
		IScriptSystem& ss;
		IStructure* fileWriterType = nullptr;
		FileWriterInstanceWithInternals stdoutWriter;

		IOSystem(IScriptSystem& l_ss) : ss(l_ss)
		{
			const IStructure& fileWriterClass = GetFileWriterType();
			if (fileWriterClass.SizeOfStruct() != sizeof ANON_NS::FileWriterInstance)
			{
				Throw(0, "Expecting FileWriter in Sys.IO.sxy to be of size %llu bytes", sizeof ANON_NS::FileWriterInstance);
			}

			Rococo::Script::AddSysIO(ss, *this);

			stdoutWriter.fp = stdout;
			stdoutWriter.stub.Desc = (ObjectDesc*) fileWriterType->GetVirtualTable(0);
			stdoutWriter.stub.pVTables[0] = (VirtualTable*) fileWriterType->GetVirtualTable(1);
			stdoutWriter.stub.refCount = ObjectStub::NO_REF_COUNT;

			auto* sysIO = ss.PublicProgramObject().GetRootNamespace().FindSubspace("Sys.IO");
			if (!sysIO)
			{
				Throw(0, "Could not find Sys.IO");
			}

			auto* pFileCursorInterfaceDef = sysIO->FindInterface("IFileCursor");
			if (!pFileCursorInterfaceDef)
			{
				Throw(0, "Could not find Sys.IO.IFileCursor");
			}

			stdoutWriter.cursor = &pFileCursorInterfaceDef->UniversalNullInstance()->pVTables[0];
		}

		FileWriterInstanceWithInternals& ToFileWriter(InterfacePointer ip)
		{
			auto* stub = InterfaceToInstance(ip);
			if (stub->Desc->TypeInfo != fileWriterType)
			{
				ss.ThrowNative(0, __FUNCTION__, "class was not FileWriter@Sys.IO");
			}

			return *(FileWriterInstanceWithInternals*)stub;
		}

		const IStructure& GetFileWriterType()
		{
			if (fileWriterType)
			{
				return *fileWriterType;
			}

			enum { SYS_IO_MODULE = 4 };
			auto& sysIOModule = ss.ProgramObject().GetModule(SYS_IO_MODULE);
			fileWriterType = sysIOModule.FindStructure("FileWriter");
			if (!fileWriterType)
			{
				Throw(0, "%s: Cannot find 'class FileWriter' in %s", __FUNCTION__, sysIOModule.Name());
			}

			if (fileWriterType->InterfaceCount() < 1)
			{
				Throw(0, "%s: Expecting at least one interface in 'class FileWriter' in %s", __FUNCTION__, sysIOModule.Name());
			}

			return *fileWriterType;
		}

		virtual void Free() override
		{
			delete this;
		}
	};

	IOSystem& From(NativeCallEnvironment& e)
	{
		IOSystem* sysIO = (IOSystem*)e.context;
		return *sysIO;
	}

	void GetStdOut(NativeCallEnvironment& e)
	{
		InterfacePointer pIWriter = &From(e).stdoutWriter.stub.pVTables[0];
		WriteOutput(0, pIWriter, e);
	}

	void WriteInt32(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		int32 x;
		ReadInput(1, x, e);

		auto& instance = From(e).ToFileWriter(writer);

		cstr type;

		if (instance.HasFlag(FileWriterInstanceFlags_IntsAsHexadecimal))
		{
			type = "%X";
		}
		else if (instance.HasFlag(FileWriterInstanceFlags_IntsAsUnsigned))
		{
			type = "%u";
		}
		else
		{
			type = "%d";
		}

		if (instance.HasFlag(FileWriterInstanceFlags_ApplyFormat))
		{
			CopyString(instance.formatSpec + instance.formatPos, 4, type + 1);
			fprintf(instance.fp, instance.formatSpec, x);
		}
		else
		{
			fprintf(instance.fp, type, x);
		}
	}

	void WriteInt64(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		int64 x;
		ReadInput(1, x, e);

		auto& instance = From(e).ToFileWriter(writer);

		cstr type;

		if (instance.HasFlag(FileWriterInstanceFlags_IntsAsHexadecimal))
		{
			type = "%llX";
		}
		else if (instance.HasFlag(FileWriterInstanceFlags_IntsAsUnsigned))
		{
			type = "%llu";
		}
		else
		{
			type = "%lld";
		}

		if (HasFlag(FileWriterInstanceFlags_ApplyFormat, instance.flags))
		{
			CopyString(instance.formatSpec + instance.formatPos, 4, type + 1);
			fprintf(instance.fp, instance.formatSpec, x);
		}
		else
		{
			fprintf(instance.fp, type, x);
		}
	}

	void WriteFloat64(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		float64 x;
		ReadInput(1, x, e);

		auto& instance = From(e).ToFileWriter(writer);

		cstr type;

		if (instance.HasFlag(FileWriterInstanceFlags_FloatsAsE))
		{
			type = "%e";
		}
		else if (instance.HasFlag(FileWriterInstanceFlags_FloatsAsG))
		{
			type = "%g";
		}
		else
		{
			type = "%f";
		}

		if (HasFlag(FileWriterInstanceFlags_ApplyFormat, instance.flags))
		{
			instance.formatSpec[instance.formatPos] = type[1];
			instance.formatSpec[instance.formatPos + 1] = 0;
			fprintf(instance.fp, instance.formatSpec, x);
		}
		else
		{
			fprintf(instance.fp, type, x);
		}
	}

	void WriteFloat32(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		float32 x;
		ReadInput(1, x, e);

		auto& instance = From(e).ToFileWriter(writer);

		cstr type;

		if (instance.HasFlag(FileWriterInstanceFlags_FloatsAsE))
		{
			type = "%e";
		}
		else if (instance.HasFlag(FileWriterInstanceFlags_FloatsAsG))
		{
			type = "%g";
		}
		else
		{
			type = "%f";
		}

		if (HasFlag(FileWriterInstanceFlags_ApplyFormat, instance.flags))
		{
			instance.formatSpec[instance.formatPos] = type[1];
			instance.formatSpec[instance.formatPos + 1] = 0;
			fprintf(instance.fp, instance.formatSpec, x);
		}
		else
		{
			fprintf(instance.fp, type, x);
		}
	}

	void WriteBool(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		boolean32 x;
		ReadInput(1, x, e);

		auto& instance = From(e).ToFileWriter(writer);

		fputs(x ? "true" : "false", instance.fp);
	}

	void WriteIString(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		InterfacePointer string;
		ReadInput(1, string, e);
		
		auto& instance = From(e).ToFileWriter(writer);

		auto* sc = (CStringConstant*)InterfaceToInstance(string);

		if (sc->length > 0 && sc->pointer)
		{
			if (instance.HasFlag(FileWriterInstanceFlags_ApplyFormat | FileWriterInstanceFlags_FormatString))
			{
				instance.formatSpec[instance.formatPos] = 's';
				instance.formatSpec[instance.formatPos + 1] = 0;
				fprintf(instance.fp, instance.formatSpec, sc->pointer);
				instance.RemoveFlag(FileWriterInstanceFlags_FormatString);
			}
			else
			{
				fputs(sc->pointer, instance.fp);
			}
		}
	}

	void WritePointer(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		void* pointer;
		ReadInput(1, pointer, e);

		auto& instance = From(e).ToFileWriter(writer);

		fprintf(instance.fp, "%p", pointer);
	}

	void WriteChar(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		int rawValue;
		ReadInput(1, rawValue, e);

		auto& instance = From(e).ToFileWriter(writer);

		int eightBitValue = rawValue & 0xFF;
		fputc(eightBitValue & 0xFF, instance.fp);
	}

	void WriteAsDecimal(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		instance.RemoveFlag(FileWriterInstanceFlags_IntsAsHexadecimal);
		instance.RemoveFlag(FileWriterInstanceFlags_IntsAsUnsigned);
	}

	void WriteAsFormat(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);
		instance.AddFlag(FileWriterInstanceFlags_FormatString);
	}

	void WriteAsHex(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		instance.AddFlag(FileWriterInstanceFlags_IntsAsHexadecimal);
		instance.RemoveFlag(~FileWriterInstanceFlags_IntsAsUnsigned);
	}

	void WriteAsUnsigned(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		instance.AddFlag(FileWriterInstanceFlags_IntsAsUnsigned);
		instance.RemoveFlag(FileWriterInstanceFlags_IntsAsHexadecimal);
	}

	void WriteAsSpecE(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		instance.AddFlag(FileWriterInstanceFlags_FloatsAsE);
		instance.RemoveFlag(FileWriterInstanceFlags_FloatsAsG);
	}

	void WriteAsSpecF(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		instance.RemoveFlag(FileWriterInstanceFlags_FloatsAsE);
		instance.RemoveFlag(FileWriterInstanceFlags_FloatsAsG);
	}

	void WriteAsSpecG(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		instance.RemoveFlag(FileWriterInstanceFlags_FloatsAsE);
		instance.AddFlag(FileWriterInstanceFlags_FloatsAsG);
	}

	void SetPrecision(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		int precision;
		ReadInput(1, precision, e);

		instance.precision = clamp(precision, 0, 16);

		instance.AddFlag(FileWriterInstanceFlags_ApplyFormat);

		instance.UpdateFormat();
	}

	void SetRightAlign(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		boolean32 rightAlign;
		ReadInput(1, rightAlign, e);

		if (rightAlign)
		{
			instance.RemoveFlag(FileWriterInstanceFlags_LeftAlign);
		}
		else
		{
			instance.AddFlag(FileWriterInstanceFlags_LeftAlign);
		}

		instance.AddFlag(FileWriterInstanceFlags_ApplyFormat);

		instance.UpdateFormat();
	}

	void SetWidth(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		int width;
		ReadInput(1, width, e);

		instance.width = clamp(width, 0, 16);
		instance.AddFlag(FileWriterInstanceFlags_ApplyFormat);

		instance.UpdateFormat();
	}

	void SetZeroPrefix(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		boolean32 useZeroPrefix;
		ReadInput(1, useZeroPrefix, e);

		if (useZeroPrefix)
		{
			instance.AddFlag(FileWriterInstanceFlags_ZeroPrefix);
			instance.AddFlag(FileWriterInstanceFlags_ApplyFormat);
		}
		else
		{
			instance.RemoveFlag(FileWriterInstanceFlags_ZeroPrefix);
			instance.AddFlag(FileWriterInstanceFlags_ApplyFormat);
		}

		instance.UpdateFormat();
	}

	void UseDefaultFormatting(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		instance.precision = -1;
		instance.width = -1;
		instance.flags = 0;

		instance.formatSpec[0] = {0};
		instance.formatPos = 0;
	}

	void GetCursor(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& instance = From(e).ToFileWriter(writer);

		WriteOutput(0, instance.cursor, e);
	}
}

namespace Rococo::Script
{
	IIOSystem* CreateIOSystem(IScriptSystem& ss)
	{
		return new ANON_NS::IOSystem(ss);
	}

	void AddSysIO(IScriptSystem& ss, ANON_NS::IOSystem& ioSystem)
	{
		{
			const INamespace& sysIO = ss.AddNativeNamespace("Sys.IO");
			ss.AddNativeCall(sysIO, ANON_NS::GetStdOut, &ioSystem, "GetStdOut -> (Sys.IO.IWriter stdout)", __FILE__, __LINE__, true);
		}

		const INamespace& sysIONative = ss.AddNativeNamespace("Sys.IO.Native");
		ss.AddNativeCall(sysIONative, ANON_NS::GetCursor, &ioSystem, "GetCursor (Sys.IO.IWriter writer) -> (Sys.IO.IFileCursor cursor)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteChar, &ioSystem, "WriteChar (Sys.IO.IWriter writer)(Int32 asciiValue)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsDecimal, &ioSystem, "WriteAsDecimal (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsFormat, &ioSystem, "WriteAsFormat (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsHex, &ioSystem, "WriteAsHex (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsSpecE, &ioSystem, "WriteAsSpecE (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsSpecF, &ioSystem, "WriteAsSpecF (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsSpecG, &ioSystem, "WriteAsSpecG (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsUnsigned, &ioSystem, "WriteAsUnsigned (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteBool, &ioSystem, "WriteBool (Sys.IO.IWriter writer)(Bool x)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteIString, &ioSystem, "WriteIString (Sys.IO.IWriter writer) (IString s)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteInt32, &ioSystem, "WriteInt32 (Sys.IO.IWriter writer) (Int32 x)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteInt64, &ioSystem, "WriteInt64 (Sys.IO.IWriter writer) (Int64 x)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteFloat32, &ioSystem, "WriteFloat32 (Sys.IO.IWriter writer) (Float32 x)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteFloat64, &ioSystem, "WriteFloat64 (Sys.IO.IWriter writer) (Float64 x)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WritePointer, &ioSystem, "WritePointer (Sys.IO.IWriter writer) (Pointer x)->", __FILE__, __LINE__, true);

		ss.AddNativeCall(sysIONative, ANON_NS::SetPrecision, &ioSystem, "SetPrecision (Sys.IO.IWriter writer) (Int32 precision)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::SetRightAlign, &ioSystem, "SetRightAlign (Sys.IO.IWriter writer) (Bool rightAlign)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::SetWidth, &ioSystem, "SetWidth (Sys.IO.IWriter writer) (Int32 width)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::SetZeroPrefix, &ioSystem, "SetZeroPrefix (Sys.IO.IWriter writer) (Bool useZeroPrefix)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::UseDefaultFormatting, &ioSystem, "UseDefaultFormatting (Sys.IO.IWriter writer) ->", __FILE__, __LINE__, true);
	}
}