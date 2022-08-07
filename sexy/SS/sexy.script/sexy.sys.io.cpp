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
		FileWriterInstanceWithInternals stderrWriter;
		CStringConstant* commandLineConstant;

		char envBuffer[32768];

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
			

			stderrWriter = stdoutWriter;
			stderrWriter.fp = stderr;

			cstr commandLine = Rococo::OS::GetCommandLineText();
			commandLineConstant = ss.GetStringReflection(commandLine);
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

	void GetStdErr(NativeCallEnvironment& e)
	{
		InterfacePointer pIWriter = &From(e).stderrWriter.stub.pVTables[0];
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

	void PrintBraceSpacing(FileWriterInstance& instance, int depth)
	{
		int spacing = clamp(instance.width, 0, 16);
		for (int i = 0; i < depth * spacing; ++i)
		{
			fputc(' ', instance.fp);
		}
	}

	void PrintAtomicSpacing(FileWriterInstance& instance, int depth)
	{
		int spacing = clamp(instance.precision, 0, 16);
		for (int i = 0; i < depth * spacing; ++i)
		{
			fputc(' ', instance.fp);
		}
	}

	void PrintExpression(FileWriterInstance& instance, cr_sex s, int depth);

	void PrintCompundExpression(FileWriterInstance& instance, cr_sex s, int depth)
	{
		PrintBraceSpacing(instance, depth);

		fputc('(', instance.fp);

		bool hasCompoundChildren = false;
		for (int i = 0; i < s.NumberOfElements(); ++i)
		{
			if (s[i].NumberOfElements() > 0)
			{
				hasCompoundChildren = true;
				break;
			}
		}

		if (hasCompoundChildren) fputc('\n', instance.fp);

		for (int i = 0; i < s.NumberOfElements(); ++i)
		{
			if (i > 0)
			{
				PrintAtomicSpacing(instance, depth);
			}

			PrintExpression(instance, s[i], depth + 1);
		}

		if (hasCompoundChildren)
		{
			fputc('\n', instance.fp);
			PrintBraceSpacing(instance, depth);
		}
		fputc(')', instance.fp);
		PrintAtomicSpacing(instance, depth);
	}

	void PrintExpression(FileWriterInstance& instance, cstr s)
	{
		fputc('"', instance.fp);

		for (cstr p = s; *p != 0; p++)
		{
			switch (*p)
			{
			case '\n':
				fputs("&n", instance.fp);
				break;
			case '\r':
				fputs("&r", instance.fp);
				break;
			case '\t':
				fputs("&t", instance.fp);
				break;
			case '\b':
				fputs("&b", instance.fp);
				break;
			default:
				fputc(*p, instance.fp);
				break;
			}
		}

		fputc('"', instance.fp);
	}

	void PrintExpression(FileWriterInstance& instance, cr_sex s, int depth)
	{
		switch (s.Type())
		{
		case EXPRESSION_TYPE_ATOMIC:
			fputs(s.String()->Buffer, instance.fp);
			break;
		case EXPRESSION_TYPE_NULL:
			fputs("()", instance.fp);
			break;
		case EXPRESSION_TYPE_STRING_LITERAL:
			PrintExpression(instance, s.String()->Buffer);
			break;
		case EXPRESSION_TYPE_COMPOUND:
			PrintCompundExpression(instance, s, depth);
			break;
		}
	}

	void WriteIExpression(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		InterfacePointer ipS;
		ReadInput(1, ipS, e);

		IScriptSystem& ss = (IScriptSystem&)e.ss;

		auto& instance = From(e).ToFileWriter(writer);

		CClassExpression* object = (CClassExpression*) InterfaceToInstance(ipS);

		auto* type = object->Header.Desc->TypeInfo;

		if (IsNullType(*type))
		{
			return;
		}

		if (ss.GetExpressionType() == type || ss.GetExpressionBuilderType() == type)
		{
			if (!object->ExpressionPtr)
			{
				return;
			}

			cr_sex s = *object->ExpressionPtr;
			PrintExpression(instance, s, 0);
		}
		else
		{
			e.ss.ThrowNative(0, __FUNCTION__, "Unsupported expression class");
		}
	}

	void WriteSubstring(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		InterfacePointer string;
		ReadInput(1, string, e);

		int32 pos;
		ReadInput(2, pos, e);

		int32 length;
		ReadInput(3, length, e);

		if (length == 0) return;

		auto& instance = From(e).ToFileWriter(writer);

		auto* sc = (CStringConstant*)InterfaceToInstance(string);

		if (sc->length > 0 && sc->pointer)
		{
			if (pos < 0 || pos >= sc->length)
			{
				char msg[256];
				SafeFormat(msg, "Bad start position. Position was %d. String length was %d", pos, sc->length);
				e.ss.ThrowNative(0, __FUNCTION__, msg);
			}

			if (length < 0 || length + pos >= sc->length)
			{
				// Append everything from the start position
				if (fputs(sc->pointer + pos, instance.fp) < 0)
				{
					char msg[256];
					SafeFormat(msg, "Error writing substring");
					e.ss.ThrowNative(0, __FUNCTION__, msg);
				}
			}

			if (length != fwrite(sc->pointer + pos, 1, length, instance.fp))
			{
				char msg[256];
				SafeFormat(msg, "Error writing substring. Segment length should have been %d bytes", length);
				e.ss.ThrowNative(0, __FUNCTION__, msg);
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

	void WriteToFile(NativeCallEnvironment& e)
	{
		InterfacePointer string;
		ReadInput(0, string, e);

		auto* sc = (CStringConstant*)InterfaceToInstance(string);

		if (sc->length <= 0 || sc->pointer == nullptr)
		{
			e.ss.ThrowNative(0, __FUNCTION__, "Filename was blank");
			return;
		}

		FILE* fp = nullptr;
		errno_t err = fopen_s(&fp, sc->pointer, "wb");
		if (!fp || err != 0)
		{
			char errMsg[256];
			strerror_s(errMsg, err);
			e.ss.ThrowNative(0, sc->pointer, errMsg);
			return;
		}

		FileWriterInstanceWithInternals* file = nullptr;

		try
		{
			file = (FileWriterInstanceWithInternals*)e.ss.CreateScriptObjectDirect(sizeof FileWriterInstanceWithInternals, *From(e).fileWriterType);
		}
		catch (...)
		{
			fclose(fp);
			throw;
		}

		if (!file)
		{
			fclose(fp);
			e.ss.ThrowFromNativeCode(0, "Could not allocate FileWriterInstanceWithInternals");
			return;
		}

		file->flags = 0;
		file->formatSpec[0] = 0;
		file->formatPos = 0;
		file->precision = -1;
		file->width = -1;
		file->fp = fp;

		InterfacePointer ipWriter = &file->stub.pVTables[0];
		WriteOutput(0, ipWriter, e);
	}

	void CloseWriter(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& ioSystem = From(e);
		auto& instance = ioSystem.ToFileWriter(writer);

		if (&ioSystem.stdoutWriter != &instance && &ioSystem.stderrWriter != &instance)
		{
			if (instance.fp)
			{
				fclose(instance.fp);
				instance.fp = nullptr;
			}
		}	
	}

	int64 GetFilePosition(FILE* fp)
	{
		int64 pos = (int64)_ftelli64(fp);
		if (pos < 0)
		{
			char msg[256];
			int err = errno;
			if (err != 0)
			{
				if (0 != strerror_s(msg, err))
				{
					Throw(0, "_ftelli64: unknown error code 0x%8.8X", err);
				}
				else
				{
					Throw(0, "_ftelli64: error '%s'", msg);
				}
			}
			else
			{
				Throw(0, "_ftelli64: unknown error");
			}
		}

		return pos;
	}

	void SetFilePosition(FILE* fp, bool fromEnd, int64 position)
	{
		errno_t err;
		if (0 != (err = _fseeki64(fp, position, fromEnd ? SEEK_END : SEEK_SET)))
		{
			cstr dir = fromEnd ? "SEEK_END" : "SEEK_SET";

			char msg[256];
			if (0 != strerror_s(msg, err))
			{
				Throw(0, "_fseeki64(fp, %lld, %s): unknown error code 0x%8.8X", position, dir, err);
			}
			else
			{
				Throw(0, "_fseeki64(fp, %lld, %s): %s", position, dir, msg);
			}
	}
	}

	void GetFileLength(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& ioSystem = From(e);
		auto& instance = ioSystem.ToFileWriter(writer);

		try
		{
			int64 pos = GetFilePosition(instance.fp);
			SetFilePosition(instance.fp, true, 0);
			int64 length = GetFilePosition(instance.fp);
			SetFilePosition(instance.fp, false, pos);
			WriteOutput(0, length, e);
		}
		catch (IException& ex)
		{
			e.ss.ThrowNative(ex.ErrorCode(), __FUNCTION__, ex.Message());
		}
	}

	void GetFilePosition(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& ioSystem = From(e);
		auto& instance = ioSystem.ToFileWriter(writer);

		try
		{
			int64 pos = GetFilePosition(instance.fp);
			WriteOutput(0, pos, e);
		}
		catch (IException& ex)
		{
			e.ss.ThrowNative(ex.ErrorCode(), __FUNCTION__, ex.Message());
		}
	}

	void SetFilePosition(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& ioSystem = From(e);
		auto& instance = ioSystem.ToFileWriter(writer);

		int64 pos;
		ReadInput(1, pos, e);

		try
		{
			SetFilePosition(instance.fp, false, pos);
		}
		catch (IException& ex)
		{
			e.ss.ThrowNative(ex.ErrorCode(), __FUNCTION__, ex.Message());
		}
	}

	void FlushWriter(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& ioSystem = From(e);
		auto& instance = ioSystem.ToFileWriter(writer);

		fflush(instance.fp);
	}

	void SetWriterBuffering(NativeCallEnvironment& e)
	{
		InterfacePointer writer;
		ReadInput(0, writer, e);

		auto& ioSystem = From(e);
		auto& instance = ioSystem.ToFileWriter(writer);

		int32 length, mode;
		ReadInput(1, length, e);
		ReadInput(2, mode, e);

		size_t bufferlength = clamp(length, 0, 1024 * 1024);

		switch (mode)
		{
		case 1:
			mode = _IOFBF;
			break;
		case 2:
			mode = _IOLBF;
			break;
		case 3:
			mode = _IONBF;
			break;
		default:
			e.ss.PublicProgramObject().Log().Write("setvbuf - bad mode");
			return;
		}

		int err = setvbuf(instance.fp, nullptr, mode, bufferlength);
		if (!err)
		{
			// The function is deemed a request and failure not recorded. The Win32 docs show the only valid errno is EINVAL, which does not tell us much. Our API clamps bufferlength and mode to 1,2 or 3, so not much scope for error
		}
	}

	void GetCommandLine(NativeCallEnvironment& e)
	{
		auto& ioSystem = From(e);
		InterfacePointer pCL = &ioSystem.commandLineConstant->header.pVTables[0];
		WriteOutput(0, pCL, e);
	}

	void AppendEnvironmentVariable(NativeCallEnvironment& e)
	{
		auto& ioSystem = From(e);

		IScriptSystem& ss = (IScriptSystem&) e.ss;
		
		InterfacePointer ipKey;
		ReadInput(0, ipKey, e);

		InterfacePointer ipValueBuilder;
		ReadInput(1, ipValueBuilder, e);

		CStringConstant* sc = (CStringConstant*) InterfaceToInstance(ipKey);
		FastStringBuilder* sb = (FastStringBuilder*) InterfaceToInstance(ipValueBuilder);

		if (!sc->length || !sc->pointer)
		{
			ss.ThrowNative(0, __FUNCTION__, "No key was supplied");
			return;
		}

		size_t requiredLen;
		errno_t err = getenv_s(&requiredLen, nullptr, 0, sc->pointer);
		if (err != 0)
		{
			int32 length = 0;
			WriteOutput(0, 0, e);
			return;
		}

		if (requiredLen > sizeof IOSystem::envBuffer)
		{
			ss.ThrowNative(0, __FUNCTION__, "Insufficient buffer");
			return;
		}

		int32 length = (int32) requiredLen;
		WriteOutput(0, length, e);

		if (sb->stub.Desc->TypeInfo == e.ss.GetStringBuilderType())
		{
			getenv_s(&requiredLen, ioSystem.envBuffer, sizeof IOSystem::envBuffer, sc->pointer);

			if (requiredLen > 0)
			{
				int32 capacity = sb->capacity - sb->length;
				if (capacity <= 0)
				{
					return;
				}

				memcpy_s(sb->buffer + sb->length, capacity, ioSystem.envBuffer, requiredLen);
				sb->length += (int32)requiredLen - 1;
			}
		}
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
			ss.AddNativeCall(sysIO, ANON_NS::GetStdErr, &ioSystem, "GetStdErr -> (Sys.IO.IWriter stdout)", __FILE__, __LINE__, true);
			ss.AddNativeCall(sysIO, ANON_NS::WriteToFile, &ioSystem, "WriteToFile (IString path) -> (Sys.IO.IFileWriter writer)", __FILE__, __LINE__, true);
			ss.AddNativeCall(sysIO, ANON_NS::GetCommandLine, &ioSystem, "CommandLine -> (IString path)", __FILE__, __LINE__, true);
			ss.AddNativeCall(sysIO, ANON_NS::AppendEnvironmentVariable, &ioSystem, "AppendEnvironmentVariable (IString variableName)(IStringBuilder sb)->(Int32 length)", __FILE__, __LINE__, true);
		}

		const INamespace& sysIONative = ss.AddNativeNamespace("Sys.IO.Native");
		ss.AddNativeCall(sysIONative, ANON_NS::WriteChar, &ioSystem, "WriteChar (Sys.IO.IWriter writer)(Int32 asciiValue)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsDecimal, &ioSystem, "WriteAsDecimal (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsFormat, &ioSystem, "WriteAsFormat (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsHex, &ioSystem, "WriteAsHex (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsSpecE, &ioSystem, "WriteAsSpecE (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsSpecF, &ioSystem, "WriteAsSpecF (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsSpecG, &ioSystem, "WriteAsSpecG (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteAsUnsigned, &ioSystem, "WriteAsUnsigned (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteBool, &ioSystem, "WriteBool (Sys.IO.IWriter writer)(Bool x)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteIExpression, &ioSystem, "WriteIExpression (Sys.IO.IWriter writer) (Sys.Reflection.IExpression s)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteIString, &ioSystem, "WriteIString (Sys.IO.IWriter writer) (IString s)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteInt32, &ioSystem, "WriteInt32 (Sys.IO.IWriter writer) (Int32 x)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteInt64, &ioSystem, "WriteInt64 (Sys.IO.IWriter writer) (Int64 x)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteFloat32, &ioSystem, "WriteFloat32 (Sys.IO.IWriter writer) (Float32 x)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteFloat64, &ioSystem, "WriteFloat64 (Sys.IO.IWriter writer) (Float64 x)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WritePointer, &ioSystem, "WritePointer (Sys.IO.IWriter writer) (Pointer x)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::WriteSubstring, &ioSystem, "WriteSubstring (Sys.IO.IWriter writer) (IString s) (Int32 startPos) (Int32 numberOfChars) ->", __FILE__, __LINE__, true);

		ss.AddNativeCall(sysIONative, ANON_NS::FlushWriter, &ioSystem, "FlushWriter (Sys.IO.IWriter writer)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::SetWriterBuffering, &ioSystem, "SetWriterBuffering (Sys.IO.IWriter writer)(Int32 length)(Int32 mode)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::GetFileLength, &ioSystem, "GetFileLength (Sys.IO.IFileWriter writer) -> (Int64 length)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::GetFilePosition, &ioSystem, "GetFilePosition (Sys.IO.IFileWriter writer) -> (Int64 position)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::SetFilePosition, &ioSystem, "SetFilePosition (Sys.IO.IFileWriter writer) (Int64 position) ->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::SetPrecision, &ioSystem, "SetPrecision (Sys.IO.IWriter writer) (Int32 precision)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::SetRightAlign, &ioSystem, "SetRightAlign (Sys.IO.IWriter writer) (Bool rightAlign)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::SetWidth, &ioSystem, "SetWidth (Sys.IO.IWriter writer) (Int32 width)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::SetZeroPrefix, &ioSystem, "SetZeroPrefix (Sys.IO.IWriter writer) (Bool useZeroPrefix)->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::UseDefaultFormatting, &ioSystem, "UseDefaultFormatting (Sys.IO.IWriter writer) ->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysIONative, ANON_NS::CloseWriter, &ioSystem, "CloseWriter (Sys.IO.IWriter writer) ->", __FILE__, __LINE__, true);
	}
}