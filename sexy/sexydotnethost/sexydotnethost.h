// sexydotnethost.h

#pragma once

using namespace System;
using namespace System::Text;
using namespace System::IO;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;

using namespace Sexy;
using namespace Sexy::Script;
using namespace Sexy::Sex;

using namespace Sexy::VM;
using namespace Sexy::Compiler;

#include "sexchar.to.unicode.conversions.h"

namespace
{  
	inline void WriteLineToStandardOutput(const char* text) { puts(text); }
	inline void WriteLineToStandardOutput(const wchar_t* text) { _putws(text); }

	typedef void (__stdcall *FN_Log)(const wchar_t* text);
	typedef bool (__stdcall *FN_RouteMessages)();

	bool IsException(const ParseException& ex)
	{
		return ex.Name()[0] != 0 && ex.Message()[0] != 0;
	}

	class CLogger: public ILog
	{
		FN_Log m_log;
	public:
		CLogger(FN_Log fnLog)
		{
			m_log = fnLog;
		}

		void PopLastException(ParseException& ex)
		{
			ex = lastException;
			lastException = ParseException();
		}

		virtual void Write(csexstr message)
		{
#ifdef SEXCHAR_IS_WIDE
			m_log((Char*) message);
#else
			size_t len = strlen(message);
			Char* unicodeMessage = (Char*) _malloca(2*len+2);

			for(size_t i = 0; i < len; ++i)
			{
				unicodeMessage[i] = (Char) message[i];				
			}

			unicodeMessage[len] = 0;
			m_log(unicodeMessage);

			_freea(unicodeMessage);
#endif
		}

		void OnUnhandledException(int errorCode, csexstr exceptionType, csexstr message, void* exceptionInstance) 
		{
			Write(exceptionType);
			Write(SEXTEXT(": code "));

			TokenBuffer txt;
			StringPrint(txt, SEXTEXT("%s: code %d\n"), exceptionType, errorCode);
			Write(message);
		}

		void OnJITCompileException(Sex::ParseException& ex)
		{
			lastException = ex;
		}

		ParseException lastException;
	};

	IScriptSystem* ToSS(IntPtr s) { return (IScriptSystem*) s.ToPointer(); }
	VM::IDisassembler* ToDisassembler(IntPtr s) { return (IDisassembler*) s.ToPointer(); }
	CLogger* ToLog(IntPtr s) { return (CLogger*) s.ToPointer(); }
	ISourceCode* ToSC(IntPtr handle) { return (ISourceCode*) handle.ToPointer(); }
	ISParserTree* ToTree(IntPtr handle) { return (ISParserTree*) handle.ToPointer(); }
	const IFunction& ToFunction(IntPtr handle) { return *(const IFunction*) handle.ToPointer(); }
}

namespace SexyDotNet { namespace Host
{
	class CStepCallback: public Sexy::VM::IStepCallback
	{
	private:
		int nextStepIndex;
		FN_RouteMessages routeMessages;

	public:
		CStepCallback(FN_RouteMessages _routeMessages):
		  routeMessages(_routeMessages) 
		{ 
			nextStepIndex = 0;
		}
		virtual void OnStep(Sexy::VM::IDebugger& debugger);

		virtual void StepNext()
		{
			nextStepIndex++;
		}
	};

	public enum class HostLimits { MAX_SOURCE_FILE_KILOBYTES = 4096};

	public enum class VariableKind { Input, Output, Local, Pseudo, Global };

	ref class SexyScriptLanguage;

	public value struct SourceLocation
	{
		SourceLocation(int pos, int line) { Pos = pos; Line = line; }
		property int Pos;
		property int Line;

		virtual String^ ToString() override
		{
			return gcnew String("(" + Pos + ", " + Line + ")");
		}
	};

	public value class FunctionRef
	{
	private:
		IntPtr handle;
		String^ name;

	public:
		FunctionRef(IntPtr _handle, String^ _name): handle(_handle), name(_name)	{	}
		property String^ Name  { String^ get()  { return name;   } }
		property IntPtr Handle { IntPtr get()  { return handle; } }
	};

	public ref class CompileError: public System::Exception
	{
	public:
		property String^ SourceFile;
		property SourceLocation Start;
		property SourceLocation End;

		CompileError(String^ srcFile, String^ message, SourceLocation start, SourceLocation end): Exception(message)
		{
			SourceFile = srcFile;
			Start = start;
			End = end;
		}

		virtual String^ ToString() override
		{
			return gcnew String(SourceFile + ": " + Message);
		}
	};

	public ref class AssemblySection
	{
	public:
		property String^ FunctionName { String^ get() { return functionName; } }
		property IList<String^>^ Segments { IList<String^>^ get() { return segments; } }

		AssemblySection(String^ functionName, List<String^>^ segments)
		{
			this->functionName = functionName;
			this->segments = segments;
		}
	private:
		List<String^>^ segments;
		String^ functionName;
	};

	public ref class SourceModule
	{
	private:
		String^ name;
		IntPtr moduleHandle;
		IntPtr scHandle;
		IntPtr expressionTree;
		SexyScriptLanguage^ parent;
		List<AssemblySection^>^ sections;
		
		void ReleaseAll();		

		!SourceModule() {	ReleaseAll();	}
	public:
		SourceModule(SexyScriptLanguage^ parent, IntPtr moduleHandle, IntPtr scHandle, IntPtr expressionTree, String^ name);
		~SourceModule()	{	ReleaseAll();	}

		property IntPtr NativeModuleHandle { IntPtr get() { return moduleHandle;	}	}
		property String^ Name { String^ get() { return name; } }
		property IList<AssemblySection^>^ Sections { IList<AssemblySection^>^ get() { return sections; } }
		int bytecodeVersion;
	};

	public value struct SourceFileSegment
	{
	public:
		SourceFileSegment(int lineNumber, IntPtr codeBase, SourceModule^ src, FunctionRef function)
		{
			this->LineNumber = lineNumber;
			this->CodeBase = codeBase;
			this->Source = src;
			this->Function = function;
		}

		virtual String^ ToString() override
		{
			return gcnew String(Source->Name +  " (Line #" + LineNumber + "): " +  Function.Name);
		}

		int LineNumber;
		IntPtr CodeBase;
		SourceModule^ Source;
		FunctionRef Function;
	};

	public value struct VariableDesc
	{
	public:
		VariableDesc(int sfOffset, String^ name, String^ type, String^ value, VariableKind direction, IntPtr address):
		  SFOffset(sfOffset), Name(name), Type(type), Value(value), Direction(direction), Address(address) {}

		int SFOffset;
		String^ Name;
		String^ Type;
		String^ Value;
		VariableKind Direction;
		IntPtr Address;

		virtual String^ ToString() override
		{
			return gcnew String(Type + " " + Name + ": " + Value);
		}
	};

	public delegate void TerminationHandler(int code);
	public delegate void LogHandler(Char* logText);
	public delegate bool RouteSysMessagesHandler();

	public ref class SexyScriptLanguage
	{
	private:
		Dictionary<String^, SourceModule^>^ sourceModules;
		Dictionary<IntPtr,SourceFileSegment>^ sourceFileSegments;

		IntPtr routeSysMessagesHandle;
		IntPtr logHandle;
		IntPtr nativeHandle;
		IntPtr disassemblerHandle;
		IntPtr currentlyViewedModule;
		IntPtr currentlyViewedExpression;
		String^ currentViewedSourceCode;
		String^ currentViewedFilename;
		IntPtr stepCallbackHandle;

		bool goodstate;
		bool initializedVM;

		void BeginExecution();
		void Disassemble();
		void ReleaseAll();

		SourceLocation start;
		SourceLocation end;
				
		!SexyScriptLanguage()	{	ReleaseAll();	}

		TerminationHandler^ terminationHandler;
		LogHandler^ logHandler;
		RouteSysMessagesHandler^ routeSysMessagesHandler;

		GCHandle twiddler;
		GCHandle twiddler2;
	public:
		SexyScriptLanguage(LogHandler^ _logHandler, RouteSysMessagesHandler^ _msgHandler)
		{
			logHandler = _logHandler;
			routeSysMessagesHandler = _msgHandler;

			goodstate = false;
			initializedVM = false;

			twiddler = GCHandle::Alloc(logHandler);
			twiddler2 = GCHandle::Alloc(routeSysMessagesHandler);

			IntPtr ptrLog = Marshal::GetFunctionPointerForDelegate(logHandler);
			logHandle = IntPtr(new CLogger((FN_Log) ptrLog.ToPointer()));

			ProgramInitParameters pip;
			pip.MaxProgramBytes = 32768;
			nativeHandle = IntPtr(CreateScriptV_1_1_0_0(pip, *ToLog(logHandle)));
			sourceModules = gcnew Dictionary<String^,SourceModule^>();
			disassemblerHandle = IntPtr(ToSS(nativeHandle)->PublicProgramObject().VirtualMachine().Core().CreateDisassembler());
			currentlyViewedModule = IntPtr::Zero;

			IntPtr ptrRouter =  Marshal::GetFunctionPointerForDelegate(routeSysMessagesHandler);
			stepCallbackHandle = IntPtr(new CStepCallback((FN_RouteMessages) ptrRouter.ToPointer()));
			routeSysMessagesHandle = IntPtr(new CLogger((FN_Log) ptrLog.ToPointer()));
		}
		
		~SexyScriptLanguage() 
		{
			ReleaseAll();
		}
		
		void RefreshDisassembler()
		{
			Disassemble();
			UpdateDisassembly();
		}
		
		SourceModule^ AddModule(String^ moduleFullPath);
		bool UpdateDisassembly();

		SourceModule^ GetModule(String^ name)
		{
			SourceModule^ sm = nullptr;
			return sourceModules->TryGetValue(name, sm) ? sm : nullptr;
		}	

		property Dictionary<IntPtr,SourceFileSegment>^ SourceFileSegments 
		{
			Dictionary<IntPtr,SourceFileSegment>^ get()
			{
				return sourceFileSegments;
			}
		}

		List<VariableDesc>^ GetVariables(Int32 callDepth);
		List<VariableDesc>^ GetElements(String^ variableName, Int32 callDepth);

		property SourceLocation Start { SourceLocation get() { return start; } }
		property SourceLocation End { SourceLocation get() { return end; } }

		property String ^ Filename { String ^ get() {return currentViewedFilename;	}	}	
		property String ^ SourceCode { String ^ get() {return currentViewedSourceCode;	}	}	
		property IntPtr NativeHandle { IntPtr get() {return nativeHandle;	}	}	
		property IntPtr DisassemblerHandle { IntPtr get() {return disassemblerHandle;	}	}	
		Int64 GetRegisterValue(int index) {	return ToSS(nativeHandle)->PublicProgramObject().VirtualMachine().Cpu().D[index].int64Value; }
		bool StepInto();
		bool StepOver();
		bool StepOut();

		void ThrowOnCompileError();

		void Compile();
		int Execute();		

		IntPtr GetCallerSF(IntPtr sf);
		IntPtr GetPCAddress(Int32 callDepth);
		IntPtr GetReturnAddress(IntPtr sf);
		IntPtr GetStackFrame(Int32 callDepth);

		event TerminationHandler^ EvTerminated
		{
			void add(TerminationHandler ^ d) { terminationHandler += d;	}
			void remove(TerminationHandler ^ d) {	terminationHandler -= d; }
			void raise(int i) { terminationHandler->Invoke(i); }
		}	
	};
}} // SexyDotNet::Host
