/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	https://github.com/theabstraction/rococo

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	

	1. This software is open-source. It can be freely copied, distributed, compiled and the compilation executed.
	
	1.a Modification of the software is not allowed where such modifcations fail to include this exact comment header in each source file, or alter, reduce or extend the Sexy language.
	The purpose of this limitation is to prevent different incompatible versions of the Sexy language emerging. 

	1.b You are allowed to fix bugs and implement performance improvements providing you inform Sexy's copyright owner via email. Such changes may then be incorporated in
	later versions of Sexy without compromising Sexy's copyright license.
	
	2. You are not permitted to copyright derivative versions of the source code. You are free to compile the code into binary libraries and include the binaries in a commercial application. 

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM 'AS IS' WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

#include "sexy.types.h"
#include "sexy.strings.h"
#include <rococo.sexy.api.h>

#include <stdio.h>

#ifdef _WIN32
# include <intrin.h>
#endif

#ifdef _MSC_VER
# include <crtdbg.h> // _CrtSetReportMode, used by TestFsbAuditAppendAndTruncateTruncates
#endif

#include <memory.h>
#include <vector>
#include <string.h>
#include <stdlib.h>

#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"

#ifdef char_IS_WIDE
# define _UNICODE
# define UNICODE
#endif

#include <rococo.os.win32.h>

#include <limits>
#include <new>

#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexy.script.h"
#include "sexy.script.exports.h"
#include "Sexy.S-Parser.h"
#include "Sexy.Compiler.h"
#include <rococo.api.h>
#include <sexy.dispatch.inl>
#include <rococo.package.h>
#include <rococo.os.h>
#include <rococo.strings.h>
#include <rococo.time.h>

#include <rococo.sexy.allocators.h>

#define validate(_Expression) if (!(_Expression)) { ShowFailure(#_Expression, __FILE__, __LINE__); Throw(0, "Validation failure"); }

#define TEST(test) Test(#test, test, false, false)
#define TEST2(test) Test(#test, test, true, false)
#define TEST3(test) Test(#test, test, true, true)

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
using namespace Rococo::Strings;

namespace
{
   void FormatSysMessage(char* text, size_t capacity, int msgNumber)
   {
      OS::Format_C_Error(msgNumber, text, capacity);
   }

	typedef void (*FN_TEST)(IPublicScriptSystem& ss);

	bool AreAproxEqual(double x, double y)
	{
		if (y != 0)
		{
			double delta = 1 - (x/y);
			return delta > -1.0e-13 && delta < 1.0e-13;
		}

		return x == 0;
	}

	struct CLogger: public ILog
	{
		CLogger()
		{
		}

		~CLogger()
		{
		}

		bool TryGetNextException(OUT ParseException& ex)
		{
			if (exceptions.empty()) return false;

			ex = exceptions.back();
			exceptions.pop_back();

			return true;
		}

		int ExceptionCount() const
		{
			return (int32) exceptions.size();
		}

		void Write(cstr text)
		{
			lastWritten = text ? text : "";
			WriteToStandardOutput("%s", text);
		}

		void OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance) 
		{
			UNUSED(errorCode);
			UNUSED(exceptionInstance);
			ParseException ex(Vec2i{ 0,0 },Vec2i{ 0,0 }, exceptionType, message,"", NULL);
			exceptions.push_back(ex);
		}

		void OnJITCompileException(Sex::ParseException& ex)
		{
			exceptions.push_back(ex);
		}

		void Clear()
		{
			exceptions.clear();
		}

		typedef std::vector<ParseException> TExceptions;
		TExceptions exceptions;
		HString lastWritten; // The last message the VM logged, e.g. an exception that ended Execute with EXECUTERESULT_THROWN
	} s_logger;

	void Abort()
	{
		if (Rococo::OS::IsDebugging())
			Rococo::OS::TripDebugger();
		else
			exit(-1); 
	}

	void PrintStructures(IPublicScriptSystem& ss, ILog& log)
	{
		const IPublicProgramObject& obj = ss.PublicProgramObject();
		for(int i = 0; i < obj.ModuleCount(); ++i)
		{
			const IModule& m = obj.GetModule(i);

			char msg[256];
			SafeFormat(msg, 256,"\r\nModule %s", m.Name()); 
			log.Write(msg);

			for(int j = 0; j < m.StructCount(); ++j)
			{
				const IStructure& s = m.GetStructure(j);

            SafeFormat(msg, 256,"\r\nstruct %s - %d bytes", s.Name(), s.SizeOfStruct());
				log.Write(msg);

				for(int k = 0; k < s.MemberCount(); ++k)
				{
					const IMember& member = s.GetMember(k);
               SafeFormat(msg, 256,"  %s %s", member.UnderlyingType()->Name(), member.Name());
					log.Write(msg);
				}
			}
		}
	}

	void ShowFailure(const char* expression, const char* filename, int lineNumber)
	{
		printf("\nValidation failed in %s[%d]: %s\n", filename, lineNumber, expression);
	}

	void PrintExpression(cr_sex s, int &totalOutput, int maxOutput)
	{
		switch (s.Type())
		{
		case EXPRESSION_TYPE_ATOMIC:
			totalOutput += WriteToStandardOutput(" %s", (cstr) s.c_str());
			break;
      case EXPRESSION_TYPE_NULL:
         totalOutput += WriteToStandardOutput("()");
         break;
		case EXPRESSION_TYPE_STRING_LITERAL:
			totalOutput += WriteToStandardOutput(" \"%s\"", (cstr) s.c_str());
			break;
		case EXPRESSION_TYPE_COMPOUND:
			
			totalOutput += WriteToStandardOutput("( ");

			for(int i = 0; i < s.NumberOfElements(); ++i)
			{
				if (totalOutput > maxOutput) 
				{
					return;
				}
				
				cr_sex child = s.GetElement(i);
				PrintExpression(child, totalOutput, maxOutput);								
			}
			
			totalOutput += WriteToStandardOutput(" )");
		}				
	}

	void PrintParseException(const ParseException& e)
	{
		WriteToStandardOutput("\r\nParse error:\r\nSource: %s\r\nExpression: line %d pos %d to line %d pos %d", e.Name(), e.Start().y, e.Start().x, e.End().y, e.End().x);

		if (e.Source())
		{
			WriteToStandardOutput(" depth %d\r\n", e.Source()->TransformationDepth());
		}

		WriteToStandardOutput("\r\nReason: % s\r\n", e.Message());
		
		if (e.Source())
		{
			WriteToStandardOutput("\n\nTransformationDepth: %d\r\n", e.Source()->TransformationDepth());
			int depth = 0;
			for (const ISExpression* s = e.Source(); s != NULL; s = s->GetOriginal())
			{
				if (depth++ > 0)  WriteToStandardOutput("Macro expansion %d:\r\n", depth);

				int totalOutput = 0;
				PrintExpression(*s, totalOutput, 1024);

				if (totalOutput > 1024) WriteToStandardOutput("...");

				WriteToStandardOutput("\r\n");
			}
		}
	}

	void ValidateLogs(bool isSourceValid = true)
	{
		if (s_logger.ExceptionCount() > 0)
		{
			ParseException ex;
			while(s_logger.TryGetNextException(ex))
			{
				if (isSourceValid) PrintParseException(ex);
				else
				{
					WriteToStandardOutput("\r\nParse error:\r\nSource: %s\r\nExpression: (%d,%d) to (%d,%d)\r\nReason: %s\r\n", ex.Name(), ex.Start().x, ex.Start().y, ex.End().x, ex.End().y, ex.Message());
				}
			}
			validate(false);
		}
	}

	void ValidateExecution(EXECUTERESULT result)
	{		
		ValidateLogs();
		validate(EXECUTERESULT_TERMINATED == result);
	}

	void Disassemble(VM::IDisassembler& disassembler, const IFunction& f, IPublicScriptSystem& ss)
	{
		CodeSection section;
		f.Code().GetCodeSection(OUT section);

		WriteToStandardOutput("\n------------%s #%lld ---------------\n", f.Name(), section.Id);

		size_t start = ss.PublicProgramObject().ProgramMemory().GetFunctionAddress(section.Id);
		size_t programLength = ss.PublicProgramObject().ProgramMemory().GetFunctionLength(section.Id);
		const uint8* code = ss.PublicProgramObject().ProgramMemory().StartOfMemory() + start;
		size_t i = 0;
		while(i < programLength)
		{
			VM::IDisassembler::Rep rep;
			disassembler.Disassemble(code + i, OUT rep);

			WriteToStandardOutput("%8llu %s %s\n", i, rep.OpcodeText, rep.ArgText);

			validate (rep.ByteCount != 0);
			i += rep.ByteCount;
		}

		WriteToStandardOutput("\n\n");
	}

	void Disassemble(cstr fname, const IModule& module, IPublicScriptSystem& ss)
	{
		const IFunction* f = module.FindFunction(fname);
		validate(f != NULL);

		VM::IDisassembler* disassembler = ss.PublicProgramObject().VirtualMachine().Core().CreateDisassembler();	
			Disassemble(*disassembler, *f, ss);
		disassembler->Free();
	}

	void Secure(IPublicScriptSystem& ss)
	{
		AddNativeCallSecurity_ToSysNatives(ss);
	}

	int testCount = 0;

	void Test(const char* name, FN_TEST fnTest, bool addinCoroutines, bool addInIO)
	{
		testCount++;
		{
			printf("<<<<<< %s\r\n", name);

			ProgramInitParameters pip;
			pip.addCoroutineLib = addinCoroutines;

#ifdef _DEBUG
			pip.useDebugLibs = true;
#else
			pip.useDebugLibs = false;
#endif
			pip.addIO = addInIO;
			pip.MaxProgramBytes = 32768;

			CScriptSystemProxy ssp(pip, s_logger);

			auto& ss = ssp();
			if (!IsPointerValid(&ss)) exit(-1);
			Secure(ss);
			try
			{
				fnTest(ss);
				ValidateLogs();
				size_t leakCount = ss.PublicProgramObject().FreeLeakedObjects();
				if (leakCount > 0)
				{
					Throw(0, "Warning %llu leaked objects", leakCount);
				}
			}
			catch (STCException& e)
			{
				WriteToStandardOutput("Error: %s\r\nSource: %s\r\n.Code %d", e.Message(), e.Source(), e.Code());
				throw;
			}
			catch (ParseException& e)
			{
				PrintParseException(e);
				throw;
			}
			catch (IException& ose)
			{
				if (ose.ErrorCode() != 0)
				{
					char osMessage[256];
					FormatSysMessage(osMessage, 256, ose.ErrorCode());
					WriteToStandardOutput("Error code%d~%x,%s\r\n%s\r\n", ose.ErrorCode(), ose.ErrorCode(), ose.Message(), osMessage);
				}
				else
				{
					WriteToStandardOutput("%s", ose.Message());
				}
				throw;
			}
			catch (std::exception& stdex)
			{
				printf("std::exception: %s\r\n", stdex.what());
				throw;
			}

			printf("%s >>>>>>\r\n\r\n", name);
		}
	}

	void TestConstruction(IPublicScriptSystem& ss)
	{
		UNUSED(ss);

		try
		{
			ProgramInitParameters pip;
			pip.MaxProgramBytes = 32768;
#ifdef _DEBUG
			pip.useDebugLibs = true;
#endif

			CScriptSystemProxy ssp(pip, s_logger);
			validate(&ssp() != NULL);
			validate(&ssp().PublicProgramObject() != NULL);
		}
		catch(ParseException& ex)
		{
			PrintParseException(ex);
			exit(-1);
		}
		catch(IException& ose)
		{
			char osMessage[256];
			FormatSysMessage(osMessage, 256, ose.ErrorCode());
			WriteToStandardOutput("OS Error. %s\r\n%s\r\n", ose.Message(), osMessage);
			exit(-1);
		}
	}

	void TestCreateNamespace(IPublicScriptSystem& ss)
	{
		Auto<ISourceCode> sc(ss.SParser().ProxySourceBuffer("(namespace Sys.Data)", -1, Vec2i{ 0,0 }, "test2", nullptr));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();
	}

	void TestCreateDeclarations(IPublicScriptSystem& ss)
	{
		Auto<ISourceCode> sc(ss.SParser().ProxySourceBuffer("(namespace Sys.Data)", -1, Vec2i{ 0,0 }, "test2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());

		AutoFree<IDynamicStringBuilder> sb = CreateDynamicStringBuilder(16384);
		ss.Compile(&sb->Builder());

		fstring result = *sb->Builder();
		printf_s("%s", result.buffer);
	}

	bool SetProgramAndEntryPoint(IPublicProgramObject& object, const INamespace& ns, cstr fname)
	{
		const IFunction* f = ns.FindFunction(fname);
		if (f == NULL)
		{
			return false;
		}
		else
		{
			object.SetProgramAndEntryPoint(*f);
			return true;
		}
	}

	VM::IVirtualMachine& StandardTestInit(IPublicScriptSystem& ss, ISParserTree& tree, IModule** pModule = NULL)
	{
		IModule* srcModule = ss.AddTree(tree);
		if (pModule != NULL) *pModule = srcModule;
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		return ss.PublicProgramObject().VirtualMachine();
	}

	// We use this when we have a test that specifies macros in some source tree and invokes it the successor
	VM::IVirtualMachine& StandardTestInit(IPublicScriptSystem& ss, ISParserTree& macroTree, ISParserTree& mainTree)
	{
		IModule* srcModule = ss.AddTree(macroTree);
		ss.Compile();

		IModule* srcModule2 = ss.AddTree(mainTree);
		ss.PartialCompile();

		UNUSED(srcModule);
		UNUSED(srcModule2);

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		return ss.PublicProgramObject().VirtualMachine();
	}

	void TestAssignInt32Literal(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 exitCode)(Int32 result): (exitCode = 1)(result = 0xABCDEF01))"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignInt32Literal");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 exitCode
		vm.Push(101); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 result = vm.PopInt32();
		int32 exitCode = vm.PopInt32();
		validate(exitCode == 1);
		validate(result == 0xABCDEF01);
	}

	void TestAssignDerivativeFromRef(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using Sys.Maths)"
			"(function F (Vec3i w) -> (Int32 sum): "
			"	(Vec3i v = w)"
			"	(sum = (v.x / v.y))"
			")"
			"(function Main -> (Int32 result):"
			"	(Vec3i v = 12 4 7)"
			"	(F v -> result)"
			")"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignDerivativeFromRef");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 result = vm.PopInt32();
		validate(result == 3);
	}

	void TestStringReplace(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(using Sys.Type)\n"
			"(using Sys.IO)\n"
			"(function Main -> (Int32 result):\n"
			"	(IStringBuilder sb = (NewStringBuilder 16))\n"
			"	(#build sb \"Hello Guide\")"
			"   (sb.Replace 0 \"Guide\" \"Jed\")\n"
			"   (#printf sb \"&n\")"
			"   (sb.Replace 0 \"Jed\" \"Guide\")\n"
			"   (#printf sb \"&n\")"
			"   (sb.Replace 0 \"Hello\" \"Guide\")\n"
			"   (#printf sb \"&n\")"
			"   (sb.Replace 0 \"Guide\" \"Jed\")\n"
			"   (#printf sb \"&n\")"
			"   (sb.Replace 0 \"Jed\" \"Guide\")\n"
			"   (#printf sb \"&n\")"
			"   (    try ("
			"          (sb.Replace 0 \"Guide\" \"Very Big Guide\")\n"
			"        )"
			"        catch e ("
			"          (#printf e.Message \"&n\")\n"
			"        )"
			"   )"
			"   (#printf sb \"&n\")"
			")"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 result = vm.PopInt32();
		UNUSED(result);
	}

	void TestAssignFloat32Literal(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 exitCode)(Float32 result)(Float32 result2): (exitCode = -125)(result = 1.7)(result2 = 1.0e15))"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignFloat32Literal");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));


		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 exitCode
		vm.Push(0.25f); // Allocate stack space for the float32 result
		vm.Push(0.25f); // Allocate stack space for the float32 result2
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float32 result2 = vm.PopFloat32();
		float32 result = vm.PopFloat32();
		int32 exitCode = vm.PopInt32();		
		validate(exitCode == -125);
		validate(result == 1.7f);
		validate(result2/1.0e15f == 1.0f);
	}

	void TestCallPrivateMethod(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(interface Sys.ICat (Mew -> (Int32 len)))"
			"(class Cat (implements Sys.ICat))"
			"(method Cat.Construct : )"
			"(factory Sys.NewCat Sys.ICat : (construct Cat))"
			"(method Cat.Mew ->  (Int32 len): (this.PrintMew -> len))"
			"(method Cat.PrintMew ->  (Int32 len): (Sys.Print \"Mew\" -> len))"
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 exitCode): "
			"   (Sys.ICat cat (Sys.NewCat))"
			"   (cat.Mew -> exitCode)"
			")"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 exitCode
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 result2 = vm.PopInt32();
		validate(result2 == 3);
	}

	void TestCallPrivateMethod2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(interface Sys.ICat (Mew -> (Int32 len)))"
			"(interface Sys.IPet (Pet -> (Int32 len)))"
			"(class Cat (implements Sys.ICat)(implements Sys.IPet) (Int32 id))"
			"(method Cat.Construct (Int32 id): (this.id = id))"
			"(factory Sys.NewCat Sys.IPet (Int32 id) : (construct Cat id))"
			"(method Cat.Mew -> (Int32 id): (this.PrintMew -> id))"
			"(method Cat.Pet ->  (Int32 id): (this.Mew -> id))"
			"(method Cat.PrintMew ->  (Int32 result): (result = this.id))"
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 exitCode): "
			"   (Sys.IPet cat (Sys.NewCat 774))"
			"   (cat.Pet -> exitCode)"
			")"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 exitCode
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 result2 = vm.PopInt32();
		validate(result2 == 774);
	}

	void TestCallPrivateMethodviaDispatch(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(interface Sys.ICat (attribute dispatch))"

			"(class Cat (implements Sys.ICat) (Int32 id))"
			"(method Cat.Construct (Int32 id): (this.id = id))"
			"(factory Sys.NewCat Sys.ICat (Int32 id) : (construct Cat id))"

			"(struct MewArgs (Int32 id))"
			"(method Cat.Mew (MewArgs args) -> : (args.id = this.id))"

			"(namespace EntryPoint)"
			"(function Main -> (Int32 exitCode): "
			"   (Sys.ICat cat (Sys.NewCat 773))"
			"   (MewArgs args)"
			"   (cat.Mew args)"
			"   (exitCode = args.id)"
			")"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 exitCode
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 result2 = vm.PopInt32();
		validate(result2 == 773);
	}

	void TestAssignFloat64Literal(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 exitCode)(Float64 result)(Float64 result2):(exitCode = 0xAB)\n(result = 1.73)(result2 = -1.0e-15))"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignFloat64Literal");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));


		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 exitCode
		vm.Push(0.25); // Allocate stack space for the float64 result
		vm.Push(0.25); // Allocate stack space for the float64 result2
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float64 result2 = vm.PopFloat64();
		float64 result = vm.PopFloat64();
		int32 exitCode = vm.PopInt32();		
		validate(exitCode == 0xAB);
		validate(AreAproxEqual(result2,-1.0e-15) == 1.0);
		validate(AreAproxEqual(result, 1.73));
	}

   void TestAssignMatrixVariable(IPublicScriptSystem& ss)
   {
      cstr srcCode =
        "(namespace EntryPoint)"
        "(using Sys.Maths)"
        "(function Main (Int32 id) -> (Int32 exitCode): "
        "  (Matrix4x4 m)"
        "  (Matrix4x4 n)"
        "  (m = n)"
        ")"
        "(alias Main EntryPoint.Main)";
      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignMatrixVariable");
      Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

      VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
      vm.Push(100); // Allocate stack space for the int32 exitCode
      vm.Push(101); // Allocate stack space for the int32 result

      auto result = vm.Execute(VM::ExecutionFlags(false, true));
      ValidateExecution(result);
   }

   void TestMacroExpandCatch(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   R"sexy(
(namespace EntryPoint)
	(alias Main EntryPoint.Main)

(using Sys.Type)

(function Main -> (Int32 exitCode):
	(
		try
		(
			(#string msg2 256 "Bonjour!")
			(Int32 len = msg2.Length)
			(if (len > 0)
				(exitCode = 1)
			)
		)
		catch e
		(
		)
	)

	(#string msg 256 "Hello World")
	(Sys.Print msg)
)
)sexy";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
	   vm.Push(0); // Allocate stack space for the int32 exitcode

	   auto result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);

	   int32 exitCode = vm.PopInt32();
	   validate(exitCode == 1);
   }

   void TestPackage(IPublicScriptSystem& ss)
   {
	   cstr srcCode = 
		 R"sexy(
			(namespace EntryPoint)
			(using Sys.Maths)

			(function Main (Int32 id) -> (Int32 exitCode):
			  (exitCode = (Sys.Maths.I32.Double 7))
			)
			(alias Main EntryPoint.Main)
		)sexy";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   AutoFree<IPackageSupervisor> packageContent =
		   OpenZipPackage(LR"(/work/rococo/sexy/SS/sexy.script.test/double.sxyz)", "double");

	   ss.RegisterPackage(packageContent);
	   ss.LoadSubpackages("Sys.Maths.I32.*", "double");

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
	   vm.Push(99); // Allocate stack space for the int32 exitcode
	   vm.Push(101); // Allocate stack space for the int32 id

	   auto result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);

	   int32 id = vm.PopInt32();
	   int32 exitCode = vm.PopInt32();
	   validate(exitCode == 14);
	   validate(id == 101);
   }

   void TestUseDefaultNamespace(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint)"
		   "(using Sys.Maths)"

		   "($ Sys.Maths.I32)"

		   "(function Double (Int32 x)->(Int32 y): (y = (2 * x)))"

		   "(alias Double $.Double)"

		   "(function Main (Int32 id) -> (Int32 exitCode): "
		   "  (exitCode = (Sys.Maths.I32.Double 7))"
		   ")"
		   "(alias Main EntryPoint.Main)";
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
	   vm.Push(99); // Allocate stack space for the int32 exitcode
	   vm.Push(101); // Allocate stack space for the int32 id

	   auto result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);

	   int32 id = vm.PopInt32();
	   int32 exitCode = vm.PopInt32();
	   validate(exitCode == 14);
	   validate(id == 101);
   }

   void TestUseDefaultNamespace2(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint)\n"
		   "(using Sys.Maths)\n"

		   "($ Sys.Dogs)\n"

		   "(interface $.IDog\n"
		   "	(Bark ->)\n"
		   ")\n"

		   "(using $)"

		   "(class Dog (implements $.IDog))"

		   "(method Dog.Bark -> : )"

		   "(method Dog.Construct : )"

		   "(factory $.NewDog $.IDog : (construct Dog))"

		   "(function Main (Int32 id) -> (Int32 exitCode): \n"
		   "     (IDog dog (NewDog))\n"
		   ")\n"
		   "(alias Main EntryPoint.Main)\n";
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
	   vm.Push(99); // Allocate stack space for the int32 exitcode
	   vm.Push(101); // Allocate stack space for the int32 id

	   auto result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);

	   int32 id = vm.PopInt32();
	   int32 exitCode = vm.PopInt32();
	   UNUSED(exitCode);
	   UNUSED(id);
   }

   void TestAssignDerivatives(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "  (struct BitmapLocation"
		   "(Int32 left top right bottom index)"
		   ")"

		   "(struct Vec2i"
		   "(Int32 x)(Int32 y)"
		   "   )"

		   "   (struct Bitmap"
		   "   (IString filename)"
		   "	   (BitmapLocation location)"
		   "	   )"

		   "	   (struct Entity"
		   "	   (Vec2i screenPosition)"
		   "		   (Bitmap image1)"
		   "		   (Bitmap image2)"
		   "		   )"

		   "		   (function Main -> (Int32 result) :"
		   "(BitmapLocation defaultBmpLoc = 0 0 0 0 -1)"

		   "			   (Entity a0 = (10  10) (\"pawn.1.tiff\" defaultBmpLoc) (\"pawn.2.tiff\" defaultBmpLoc))"
		   "			   (Entity a1 = (40  10) (\"pawn.1.tiff\" defaultBmpLoc) (\"pawn.2.tiff\" defaultBmpLoc))"
		   "			   )"

		   "			   (namespace EntryPoint)"
		   " (alias Main EntryPoint.Main)";
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestAssignDerivatives");
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
	   vm.Push(0); // Allocate stack space for the int32 result

	   auto result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
   }

   void TestAssignVectorVariableByRef(IPublicScriptSystem& ss)
   {
      cstr srcCode =
         "(namespace EntryPoint)"
         "(using Sys.Maths)"
         "(function InitVec (Vec3 v) -> :  "
         "  (Vec3 n = 1 2 3)"
         "  (v = n)"
         ")"
         "(function Main (Int32 id) -> (Float32 a)(Float32 b)(Float32 c): "
         "  (Vec3 v)"
         "  (InitVec v)"
         "  (a = v.x)"
         "  (b = v.y)"
         "  (c = v.z)"
         ")"
         "(alias Main EntryPoint.Main)";
      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignVectorVariableByRef");
      Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

      VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
      vm.Push(100); // Allocate stack space for int32 exitCode
      vm.Push(100); // Allocate stack space for int32 a
      vm.Push(100); // Allocate stack space for int32 b
      vm.Push(100); // Allocate stack space for int32 c

      auto result = vm.Execute(VM::ExecutionFlags(false, true));

      auto exitCode = vm.PopInt32();
      auto c = vm.PopFloat32();
      auto b = vm.PopFloat32();
      auto a = vm.PopFloat32();

      ValidateExecution(result);

      validate(exitCode == 100)
      validate(a == 1.0f);
      validate(b == 2.0f);
      validate(c == 3.0f);
   }

	void TestAssignInt64Literal(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 exitCode)(Int64 result):"
			"   (exitCode = -75)(result = 0x800FCDCDEFEFABCD)   )\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignInt64Literal");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));


		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 exitCode
		vm.Push(0x0123456712345678LL); // Allocate stack space for the int64 result
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int64 result = vm.PopInt64();
		int32 exitCode = vm.PopInt32();		
		validate(exitCode == -75);
		validate(result == 0x800FCDCDEFEFABCD);
	}

	void TestAssignInt64Variable(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int64 exitCode)(Int64 result): (result = 0xABCDEF01CDEF0123)(exitCode = result))"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignInt64Variable");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		int64 dummy = 1;
		vm.Push(dummy); // Allocate stack space for the int64 exitCode
		vm.Push(dummy); // Allocate stack space for the int64 result
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int64 result = vm.PopInt64();
		int64 exitCode = vm.PopInt64();
		validate(exitCode == 0xABCDEF01CDEF0123);
		validate(result == 0xABCDEF01CDEF0123);
	}

	void TestAssignInt32Variable(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 exitCode)(Int32 result): (result = 0xABCDEF01)(exitCode = result))"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignInt32Variable");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 exitCode
		vm.Push(101); // Allocate stack space for the int32 result
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 result = vm.PopInt32();
		int32 exitCode = vm.PopInt32();
		validate(exitCode == 0xABCDEF01);
		validate(result == 0xABCDEF01);
	}

	void TestAssignFloat32Variable(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Float32 exitCode)(Float32 result): (result = 0.62)(exitCode = result))"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignFloat32Variable");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(1.0f); // Allocate stack space for the float32 exitCode
		vm.Push(1.0f); // Allocate stack space for the float32 result
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float32 result = vm.PopFloat32();
		float32 exitCode = vm.PopFloat32();
		validate(exitCode == 0.62f);
		validate(result == 0.62f);
	}

	void TestAssignFloat64Variable(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Float64 exitCode)(Float64 result): (result = 0.62)(exitCode = result))"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignFloat64Variable");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(1.0); // Allocate stack space for the float32 exitCode
		vm.Push(1.0); // Allocate stack space for the float32 result
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float64 result = vm.PopFloat64();
		float64 exitCode = vm.PopFloat64();
		validate(AreAproxEqual(exitCode, 0.62));
		validate(AreAproxEqual(result, 0.62));
	}

	void TestTopLevelMacro2(IPublicScriptSystem& ss)
	{
		cstr srcCodeModuleMain =
		//	"(#Sys.optional Sys.Type.Vec2)\n"

			"(namespace EntryPoint)\n"
			"(alias Main EntryPoint.Main)\n"

			"(using Sys)"
			"(using Sys.Type)\n"
			"(using Sys.IO)\n"
			"(using Sys.Reflection)\n"

			"(#Sys.optional Sys.Maths.Vec2i)"

			"(function Main -> (Int32 result):\n"
			"	 (IExpression in = '(#optional Sys.Maths.Vec2i))\n"
			"	 (IExpression origin = ' !)\n"
			"	 (IExpressionBuilder out = (NewExpressionBuilder origin))\n"
			"    (Sys.Type.Macros.BuildOptionalType in out)"

			"    (IExpression sOrigin = origin.Parent)"

			"    (IWriter stdout = GetStdOut)"
			"    (stdout.SetPrecision 1)"
			"    (stdout.SetWidth 4)"

			"    (#Sys.IO.printf out)"

			"    (OptionalSysMathsVec2i v)"
			"    (v.value.x = 777)"
			"    (result = v.value.x)"
			")\n"
			;

		Auto<ISourceCode> scMain = ss.SParser().ProxySourceBuffer(srcCodeModuleMain, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> mainTree(ss.SParser().CreateTree(scMain()));

		ss.BeginPartialCompilation(nullptr);
		ss.PartialCompile();

		IModule* mainModule = ss.AddTree(*mainTree);
		UNUSED(mainModule);
		ss.PartialCompile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		auto& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 777);
	}


	void TestPartialCompiles(IPublicScriptSystem& ss)
	{
		cstr srcCodeModuleMain =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"
			"	  (EntryPoint.IRobot robby (EntryPoint.NewRobot))"
			"	  (result = (sizeof robby))"
			")"
			"(alias Main EntryPoint.Main)"
			;

		cstr srcCodeModuleRobot = 
			"(namespace EntryPoint)"
			"(class Robot (defines EntryPoint.IRobot)(Int32 id))"
			"(method Robot.Construct : )"
			"(method Robot.Id -> (Int32 id) : (id = 1984))"
			"(method Robot.Destruct -> : )"
			"(factory EntryPoint.NewRobot EntryPoint.IRobot : (construct Robot))"
			;

		Auto<ISourceCode> scMain = ss.SParser().ProxySourceBuffer(srcCodeModuleMain, -1, Vec2i{ 0,0 }, "srcCodeModuleMain");
		Auto<ISParserTree> mainTree(ss.SParser().CreateTree(scMain()));

		Auto<ISourceCode> scRobot = ss.SParser().ProxySourceBuffer(srcCodeModuleRobot, -1, Vec2i{ 0,0 }, "srcCodeModuleRobot");
		Auto<ISParserTree> robotTree(ss.SParser().CreateTree(scRobot()));

		ss.BeginPartialCompilation(nullptr);

		IModule* robotModule = ss.AddTree(*robotTree);
		UNUSED(robotModule);
		ss.PartialCompile();

		IModule* mainModule = ss.AddTree(*mainTree);
		UNUSED(mainModule);
		ss.PartialCompile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		auto& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();

		int robbysize = sizeof(ObjectStub) + sizeof(int32); // Object Stub + this.id
		validate(x == robbysize);
	}

	void TestLocalVariable(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 exitCode):\n"
			"     (Int32 result) (result = 0xABCDEF01) (exitCode = result)   )\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLocalVariable");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 exitCode
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 exitCode = vm.PopInt32();
		validate(exitCode == 0xABCDEF01);
	}

	void TestBooleanLiteralVsLiteralMatches(IPublicScriptSystem& ss)
	{
		cstr srcCode =			
			"(function Main -> (Bool x)(Bool y)(Bool z)(Bool a)(Bool b)(Bool c)(Bool d)(Bool e):\n"
			"     (x = (0 > 72))\n"
			"     (y = (16 > 0))\n"
			"     (z = (10 < 12))\n"
			"     (a = (5 < 4))\n"
			"     (b = (0xABCD <= 0xABCC))\n"
			"     (c = (0xABCD <= 0xABCD))\n"
			"     (d = (0x000000000000ABCD >= 0x000000000000ABCE))\n"
			"     (e = (1.0e100 != 1.0e-100))   )\n"
			"(namespace EntryPoint)\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanLiteralVsLiteralMatches");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 x
		vm.Push(100); // Allocate stack space for the int32 y
		vm.Push(100); // Allocate stack space for the int32 z
		vm.Push(100); // Allocate stack space for the int32 a
		vm.Push(100); // Allocate stack space for the int32 b
		vm.Push(100); // Allocate stack space for the int32 c
		vm.Push(100); // Allocate stack space for the int64 d
		vm.Push(100);// Allocate stack space for the int32 e
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int64 e = vm.PopInt32();
		int64 d = vm.PopInt32();
		int32 c = vm.PopInt32();
		int32 b = vm.PopInt32();
		int32 a = vm.PopInt32();		
		int32 z = vm.PopInt32();
		int32 y = vm.PopInt32();
		int32 x = vm.PopInt32();
		validate(x == 0);
		validate(y == 1);
		validate(z == 1);
		validate(a == 0);
		validate(b == 0);
		validate(c == 1);
		validate(d == 0);
		validate(e == 1);
	}

	void TestBooleanMismatch(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"     (x = (72 > 72.1))   )\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanMismatch");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		
		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanVariableVsLiteralMatches(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x)(Bool y)(Bool z):\n"
			"   (Int32 temp)\n"
			"   (temp = 25)\n"
			"   (x = (temp > 25))\n"
			"   (y = (24 != temp))\n"
			"   (Float32 temp2)\n"
			"   (temp2 = 24.1)\n"
			"   (z = (24.2 >= temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanVariableVsLiteralMatches");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
				
		vm.Push(100); // Allocate stack space for the int32 x
		vm.Push(100); // Allocate stack space for the int32 y
		vm.Push(100); // Allocate stack space for the int32 z
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 z = vm.PopInt32();
		int32 y = vm.PopInt32();
		int32 x = vm.PopInt32();
		validate(x == 0);
		validate(y == 1);
		validate(z == 1);
	}

	void TestBooleanVariableVsVariable(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x)(Bool y)(Bool z):"
			"   (Int32 temp1 = 25)\n"
			"   (Int32 temp2 = 24)\n"
			"   (x = (temp1 >= temp2))\n"
			"   (y = (temp1 == temp2))\n"
			"   (z = (temp1 < temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		vm.Push(100); // Allocate stack space for the int32 y
		vm.Push(100); // Allocate stack space for the int32 z
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 z = vm.PopInt32();
		int32 y = vm.PopInt32();
		int32 x = vm.PopInt32();
		validate(x == 1);
		validate(y == 0);
		validate(z == 0);
	}

	void TestBooleanAssignByTwoVariables(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool result):"
			"   (Bool a = true)\n"
			"   (Bool b = true)\n"
			"   (result = (a or b))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the boolean32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 result = vm.PopInt32();
		validate(result == 1);
	}

	void TestBooleanExpressions(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x)(Bool y)(Bool z)(Bool a)(Bool b)(Bool c):"
			"   (Bool temp1 = 0)\n"
			"   (Bool temp2 = 1)\n"
			"   (Bool temp3 = 1)\n"
			"   (a = (temp3 and 1))\n"
			"   (b = (temp3 or  temp2))\n"
			"   (c = (temp3 xor temp2))\n"
			"   (x = (temp1 and temp2))\n"
			"   (y = (temp1 or  temp2))\n"
			"   (z = (temp1 xor temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		vm.Push(100); // Allocate stack space for the int32 y
		vm.Push(100); // Allocate stack space for the int32 z
		vm.Push(100); // Allocate stack space for the int32 a
		vm.Push(100); // Allocate stack space for the int32 b
		vm.Push(100); // Allocate stack space for the int32 c
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 c = vm.PopInt32();
		int32 b = vm.PopInt32();
		int32 a = vm.PopInt32();
		int32 z = vm.PopInt32();
		int32 y = vm.PopInt32();
		int32 x = vm.PopInt32();
		validate(a == 1);
		validate(b == 1);
		validate(c == 0);
		validate(x == 0);
		validate(y == 1);
		validate(z == 1);
	}

	void TestBooleanVarCompare(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 x):\n"
			"   (Bool a = true)\n"
			"   (Bool b = true)\n"
			"   (if (a == b) (x = 6))\n"
			"   (if (a == true) (x += 6))\n"
			"   (if (false != b) (x += 6))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 18);
	}

	void TestBooleanCompareVarToCompound(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Int32 a = 1)\n"
			"   (Int32 b = 3)\n"
			"   (x = (a < (b - 1)))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompareVarToCompound");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Bool temp1 = 0)\n"
			"   (Bool temp2 = 1)\n"
			"   (x = ((temp2 and temp1) xor temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Bool temp1 = 0)\n"
			"   (Bool temp2 = 1)\n"
			"   (x = ((temp2 xor temp1) xor temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanCompoundExpressions3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Bool temp1 = 0)\n"
			"   (Bool temp2 = 1)\n"
			"   (x = ((temp2 and temp1) or temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Bool temp1 = 0)\n"
			"   (Bool temp2 = 1)\n"
			"   (x = ((temp2 or temp1) and temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions4");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions5(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Bool temp1 = 0)\n"
			"   (Bool temp2 = 1)\n"
			"   (x = ((temp2 and temp1) and temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions5");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanCompoundExpressions6(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Bool temp1 = 0)\n"
			"   (Bool temp2 = 1)\n"
			"   (x = (temp2 and (temp2 and temp1)))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions6");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanCompoundExpressions7(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Bool temp1 = 0)\n"
			"   (Bool temp2 = 1)\n"
			"   (x = (temp2 or (temp2 and temp1)))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions7");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions8(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Bool temp1 = 0)\n"
			"   (Bool temp2 = 1)\n"
			"   (x = (true and (true and temp1)))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions8");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanCompoundExpressions9(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Bool temp1 = 0)\n"
			"   (Bool temp2 = 1)\n"
			"   (x = ((true or false) and (true and temp1)))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions9");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanCompoundExpressions10(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Bool temp1 = 0)\n"
			"   (Bool temp2 = 1)\n"
			"   (x = ((((temp2 and (temp2 or (temp1))) and (true and (temp1 or temp2))))))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions10");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions11(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (x = ((((true and (7 >= 6)) xor (true and (4.2 < 3.3))))))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions11");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions12(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Bool x):\n"
			"   (Float32 temp1 = 0.5)\n"
			"   (Float32 temp2 = 0.25)\n"
			"   (x = (((true and (temp1 >= temp2)) xor (true and (temp2 != temp1)))))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanCompoundExpressions12");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestIfThen1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 x):\n"
			"   (Float32 temp1 = 0.5)\n"
			"   (Float32 temp2 = 0.25)\n"
			"   (if (temp2 < temp1)\n"
			"				(x = 74)\n"
			"			 else\n"
			"				(x = 77)"
			"   )\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestIfThen1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 74);
	}

	void TestIfThen2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 x):\n"
			"   (Float32 temp1 = 0.5)\n"
			"   (Float32 temp2 = 0.25)\n"
			"   (if ((temp1 > 0.3) and (temp2 < temp1))\n"
			"				(if (temp2 > 0) (x = 74) else (x = 73))\n"
			"			 else\n"
			"				(x = 77)"
			"   )\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestIfThen2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 74);
	}

   void TestFloatArithmeticByMember(IPublicScriptSystem& ss)
   {
      cstr srcCode =
         "(namespace EntryPoint)\n"
         "(function Main -> (Float32 x)(Float32 y)(Float32 z)(Float32 w):\n"
        "     (Sys.Maths.Vec2 screenSpan = 10 20)\n"

        "     (Float32 halfX = (0.5 * screenSpan.x))\n"
        "     (Float32 halfY = (0.5 * screenSpan.y))\n"
        "     (Float32 x0)\n"
        "     (Float32 x1)\n"
        "     (x0 = (halfX - 20))\n"
        "     (x1 = (halfY + 20))\n"

        "     (x = x0)\n"
        "     (y = x1)\n"
        "	  (z = (x0 + x1))\n"
        "     (w = 0)\n"
        ")\n"
         "(alias Main EntryPoint.Main)";
      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFloatArithmeticByMember");
      Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

      VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

      vm.Push(100.0f); // Allocate stack space for the float32 x
      vm.Push(100.0f); // Allocate stack space for the float32 y
      vm.Push(100.0f); // Allocate stack space for the float32 z
      vm.Push(100.0f); // Allocate stack space for the float32 w
      ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
      float32 w = vm.PopFloat32();
      float32 z = vm.PopFloat32();
      float32 y = vm.PopFloat32();
      float32 x = vm.PopFloat32();
      validate(x == -15.0f);
      validate(y == 30.0f);
      validate(z == 15.0f);
      validate(w == 0.0f);
   }

	void TestFloatArithmetic(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float32 x)(Float32 y)(Float32 z)(Float32 w):\n"
			"   (Float32 temp1 = 0.5)\n"
			"   (Float32 temp2 = 0.25)\n"
			"   (x = (temp1 - temp2))\n"
			"			(y = (temp1 + temp2))\n"
			"   (z = (temp1 * temp2))\n"
			"			(w = (temp1 / temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFloatArithmetic1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0f); // Allocate stack space for the float32 x
		vm.Push(100.0f); // Allocate stack space for the float32 y
		vm.Push(100.0f); // Allocate stack space for the float32 z
		vm.Push(100.0f); // Allocate stack space for the float32 w
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float32 w = vm.PopFloat32();
		float32 z = vm.PopFloat32();
		float32 y = vm.PopFloat32();
		float32 x = vm.PopFloat32();
		validate(x == 0.25f);
		validate(y == 0.75f);
		validate(z == 0.125f);
		validate(w == 2.0f);
	}

	void TestDoubleArithmetic0(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float64 x):\n"
			"   (Float64 temp1 = 0.5)\n"
			"   (x = (temp1 - 0.1))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDoubleArithmetic0");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		IModule* srcModule = NULL;
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree(), &srcModule);

		vm.Push(100.0); // Allocate stack space for the float64 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float64 x = vm.PopFloat64();
		validate(x == 0.4);
	}

	void TestDoubleArithmetic1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float64 x):\n"
			"   (Float64 temp1 = 0.5)\n"
			"   (Float64 temp2 = 0.25)\n"
			"   (x = (temp1 - temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDoubleArithmetic1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		IModule* srcModule = NULL;
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree(), &srcModule);

		vm.Push(100.0); // Allocate stack space for the float64 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float64 x = vm.PopFloat64();
		validate(x == 0.25);
	}

	void TestDoubleArithmetic2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float64 x):\n"
			"   (Float64 temp1 = 0.5)\n"
			"   (Float64 temp2 = 0.25)\n"
			"   (x = (temp1 + temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDoubleArithmetic2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0); // Allocate stack space for the float64 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float64 x = vm.PopFloat64();
		validate(x == 0.75);
	}

	void TestDoubleArithmetic3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float64 x):\n"
			"   (Float64 temp1 = 0.5)\n"
			"   (Float64 temp2 = 0.25)\n"
			"   (x = (temp1 * temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDoubleArithmetic3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0); // Allocate stack space for the float64 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float64 x = vm.PopFloat64();
		validate(x == 0.125);
	}

	void TestDoubleArithmetic4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float64 x):\n"
			"   (Float64 temp1 = 0.5)\n"
			"   (Float64 temp2 = 0.25)\n"
			"	  (x = (temp1 / temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDoubleArithmetic4");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0); // Allocate stack space for the float64 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float64 x = vm.PopFloat64();
		validate(x == 2.0);
	}

	void TestInt32Arithmetic(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 x)(Int32 y)(Int32 z)(Int32 w):\n"
			"   (Int32 temp1 = 8)\n"
			"   (Int32 temp2 = 4)\n"
			"   (x = (temp1 - temp2))\n"
			"			(y = (temp1 + temp2))\n"
			"   (z = (temp1 * temp2))\n"
			"			(w = (temp1 / temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestInt32Arithmetic");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		vm.Push(100); // Allocate stack space for the int32 y
		vm.Push(100); // Allocate stack space for the int32 z
		vm.Push(100); // Allocate stack space for the int32 w
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 w = vm.PopInt32();
		int32 z = vm.PopInt32();
		int32 y = vm.PopInt32();
		int32 x = vm.PopInt32();
		validate(x == 4);
		validate(y == 12);
		validate(z == 32);
		validate(w == 2);
	}

	void TestInt64Arithmetic(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int64 x)(Int64 y)(Int64 z)(Int64 w):\n"
			"   (Int64 temp1 = 8)\n"
			"   (Int64 temp2 = 4)\n"
			"   (x = (temp1 - temp2))\n"
			"			(y = (temp1 + temp2))\n"
			"   (z = (temp1 * temp2))\n"
			"			(w = (temp1 / temp2))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestInt64Arithmetic");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		int64 dummy = 100;
		vm.Push(dummy); // Allocate stack space for the int64 x
		vm.Push(dummy); // Allocate stack space for the int64 y
		vm.Push(dummy); // Allocate stack space for the int64 z
		vm.Push(dummy); // Allocate stack space for the int64 w
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int64 w = vm.PopInt64();
		int64 z = vm.PopInt64();
		int64 y = vm.PopInt64();
		int64 x = vm.PopInt64();
		validate(x == 4);
		validate(y == 12);
		validate(z == 32);
		validate(w == 2);
	}

	void TestInt32CompoundArithmetic(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"
			"   (Int32 temp1 = 8)"
			"   (Int32 temp2 = 4)"
			"			(result = ((temp1 / temp2) + (temp1 * temp2)))"
			")"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestInt32CompoundArithmetic");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 i = vm.PopInt32();
		validate(i == 34);
	}

	void TestFloatCompoundArithmetic(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float32 x)(Float32 y)(Float32 z)(Float32 w):\n"
			"   (Float32 temp1 = 0.5)\n"
			"   (Float32 temp2 = 0.25)\n"
			"   (x = ((temp1)))\n"
			"			(y = (temp1 - (temp1 + temp2)))\n"
			"   (z = ((temp1 * temp2) + temp2))\n"
			"			(w = ((temp1 / temp2) + (temp1 * temp2)))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFloatCompoundArithmetic");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0f); // Allocate stack space for the float32 x
		vm.Push(100.0f); // Allocate stack space for the float32 y
		vm.Push(100.0f); // Allocate stack space for the float32 z
		vm.Push(100.0f); // Allocate stack space for the float32 w
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float32 w = vm.PopFloat32();
		float32 z = vm.PopFloat32();
		float32 y = vm.PopFloat32();
		float32 x = vm.PopFloat32();
		validate(x == 0.5f);
		validate(y == -0.25f);
		validate(z == 0.375f);
		validate(w == 2.125f);
	}

	void TestFloatLiteralArithmetic(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float32 x)(Float32 y)(Float32 z)(Float32 w):\n"
			"   (Float32 temp1 = 0.5)\n"
			"   (Float32 temp2 = 0.25)\n"
			"   (x = (((7))))\n"
			"			(y = (x - (2 + 3)))\n"
			"   (z = ((4 * 8) + 4))\n"
			"			(w = ((9 / 3) + (2 * 7)))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFloatCompoundArithmetic");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0f); // Allocate stack space for the float32 x
		vm.Push(100.0f); // Allocate stack space for the float32 y
		vm.Push(100.0f); // Allocate stack space for the float32 z
		vm.Push(100.0f); // Allocate stack space for the float32 w
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float32 w = vm.PopFloat32();
		float32 z = vm.PopFloat32();
		float32 y = vm.PopFloat32();
		float32 x = vm.PopFloat32();
		validate(x == 7.0f);
		validate(y == 2.0f);
		validate(z == 36.0f);
		validate(w == 17.0f);
	}

	void TestIfThen3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 x):\n"
			"   (Float32 temp1 = 1)\n"
			"   (Float32 temp2 = 2)\n"
			"   (if ((temp1 - temp2) >= 0.25) (x = 5) else (x = 7)) \n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestIfThen3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the float32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 7);
	}

	void TestFunctionCallWithOutParameter(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Maths)\n"
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 exitValue):\n"
			"    (Vec2i result)"
			"    (GenerateResult result)"
			"    (exitValue = result.x / result.y)"
			")\n"
			"(function GenerateResult (out Vec2i v)-> :\n"
			"   (v.x = 27)\n"
			"   (v.y = 3)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 exitValue
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 9);
	}

	void TestFunctionCall1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float32 exitValue1):\n"
			"    (exitValue1 = (GenerateResult))"
			")\n"
			"(function GenerateResult -> (Float32 x):\n"
			"   (x = 5)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFunctionCall1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0f); // Allocate stack space for the float32 exitValue1
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float32 x = vm.PopFloat32();
		validate(x == 5.0f);
	}

	void TestFunctionCall2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 exitValue):\n"
			"    (exitValue = (1 + (GenerateResult)))"
			"    (exitValue = (3 + (GenerateResult)))"
			"    (exitValue = (5 + (GenerateResult)))"
			")\n"
			"(function GenerateResult -> (Int32 x):\n"
			"   (x = 5)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFunctionCall2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the Int32 exitValue
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 z = vm.PopInt32();
		validate(z == 10);
	}

	void TestFunctionCall2_(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float32 exitValue1)(Float32 exitValue2)(Float32 exitValue3):\n"
			"    (exitValue1 = (1 + (GenerateResult)))"
			"    (exitValue2 = (3 + (GenerateResult)))"
			"    (exitValue3 = (5 + (GenerateResult)))"
			")\n"
			"(function GenerateResult -> (Float32 x):\n"
			"   (x = 5)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFunctionCall2_");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0f); // Allocate stack space for the float32 exitValue1
		vm.Push(100.0f); // Allocate stack space for the float32 exitValue2
		vm.Push(100.0f); // Allocate stack space for the float32 exitValue3
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float32 z = vm.PopFloat32();
		float32 y = vm.PopFloat32();
		float32 x = vm.PopFloat32();
		validate(x == 6.0f);
		validate(y == 8.0f);
		validate(z == 10.0f);
	}

	void TestFunctionCall3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 exitValue1):\n"
			"    (exitValue1 = (Square 7))"
			")\n"
			"(function Square (Int32 x) -> (Int32 y):\n"
			"   (y = (x * x))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFunctionCall3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the float32 exitValue1
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 49);
	}

	void TestFunctionCall4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float32 x) (Int32 y) (Float64 z) (Int64 w) (Bool isOK):\n"
			"    (Float32 a = 5)"
			"    (Float64 b = 6)"
			"    (Int32 c = 7)"
			"    (Int64 d = 8)"
			"    (Bool e = true)"
			"    (F a b c d e -> x y z w isOK)"
			")\n"
			"(function F (Float32 a) (Float64 b) (Int32 c) (Int64 d) (Bool e) -> (Float32 x) (Int32 y) (Float64 z) (Int64 w) (Bool isOK):\n"
			"   (x = (a * a))"
			"   (y = (c * c))"
			"   (z = (b * b))"
			"   (w = (d * d))"
			"   (isOK = (false xor e))"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFunctionCall4");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0f); // Allocate stack space for the float32 x
		vm.Push(110); // Allocate stack space for the int32 y
		vm.Push(70.0); // Allocate stack space for the float64 z
		vm.Push((int64)115); // Allocate stack space for the int64 w
		vm.Push((int32) false); // Allocate stack space for the boolean
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		bool isOK = vm.PopInt32() == 0 ? false : true;
		int64 w = vm.PopInt64();
		float64 z = vm.PopFloat64();
		int32 y = vm.PopInt32();
		float32 x = vm.PopFloat32();

		validate(x == 25.0f);
		validate(y == 49);
		validate(z == 36.0);
		validate(w == 64L);
		validate(isOK == true);
	}

	void TestFunctionCallRecursion1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 exitValue1):\n"
			"    (exitValue1 = (Factorial 5))"
			")\n"
			"(function Factorial (Int32 x) -> (Int32 result):\n"
			"   (if (x <= 1) (result = 1) else (result = (x * (Factorial(x - 1)))))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFunctionCallRecursion1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the float32 exitValue1
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 120);
	}

	void TestFunctionCallRecursion2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 exitValue1):\n"
			"    (exitValue1 = (MyFunction1 5000))"
			")\n"
			"(function MyFunction1 (Int32 x) -> (Int32 result):\n"
			"   (if (x > 100) (result = (MyFunction2 x)) else (result = x))\n"
			")\n"
			"(function MyFunction2 (Int32 x) -> (Int32 result):\n"
			"   (if (x > 100) (result = (MyFunction1 (x / 2))) else (result = x))\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFunctionCallRecursion2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the float32 exitValue1
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 78);
	}

	void TestFunctionCallMultiOutput1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 x)(Float64 y):\n"
			"    (MyFunction 5000 -> x y)"
			")\n"
			"(function MyFunction (Int32 x) -> (Int32 result1)(Float64 result2):\n"
			"   (result1 = (x * 2))\n"
			"   (result2 = 25)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestFunctionCallMultiOutput1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		vm.Push(100.0); // Allocate stack space for the float64 y
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		double y = vm.PopFloat64();
		int32 x = vm.PopInt32();
		validate(x == 10000);
		validate(y == 25.0);
	}

	void TestPushStructToArray(IPublicScriptSystem& ss)
	{
		cstr srcCode =
R"(

(using Sys.Maths)
(namespace EntryPoint)

(struct Slot
	(Rectf rect)
	(Int32 id)
)

(function Main -> (Float32 result):
	(array Slot slots 1)
	(Rectf rect = 1 2 10 11)
	(slots.Push Slot (rect  7))
	(Slot s0)
	(s0 = (slots 0))
	(result = s0.rect.bottom)
)
(alias Main EntryPoint.Main)

)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the float32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		float x = vm.PopFloat32();
		validate(x == 11.0f);
	}

	void TestStructWithVec4f(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(struct Def\n"
			"	(Sys.Maths.Vec4 v)\n"
			")\n"
			"(function Main -> (Float32 result):\n"
			"	 (Def d)"
			"    (d.v = 1 2 3 4)"
			"    (result = (d.v.x + d.v.w))"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStructWithVec4f");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0f); // Allocate stack space for the float32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		float32 x = vm.PopFloat32();
		validate(x == 5.0f);
	}

	void TestStructure(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Float32 result):\n"
			"	 (Vector4 v)"
			"    (v.x = 12)"
			"    (v.y = 10)"
			"    (v.z = 2)"
			"    (if (v.z == 2) (result = (v.x * v.y)))"
			")\n"
			"(struct Vector4\n"
			"	(Float32 x y z)\n"
			"	(Float32 w)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStructure");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0f); // Allocate stack space for the float32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		float32 x = vm.PopFloat32();
		validate(x == 120.0f);
	}

	void TestStructureAssignToSingleArgumentCompoundCallReturn(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function SetTo123x (Int32 multiplier) (out Vec2i result) -> :\n"
			"	(result.x = (multiplier * 12))\n"
			"	(result.y = (multiplier * 3))\n"
			")\n"
			"(function Main -> (Int32 result):\n"
			"	(Vec2i v = SetTo123x 7)\n"
			"	(result = (v.x / v.y))\n"
			")\n"
			"(struct Vec2i\n"
			"	(Int32 x y)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestStructureAssignToSingleArgumentCompoundCallReturn2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function SetTo123x (Int32 multiplier) (out Vec2i result) -> :\n"
			"	(result.x = (multiplier * 12))\n"
			"	(result.y = (multiplier * 3))\n"
			")\n"
			"(function Main -> (Int32 result):\n"
			"	(Vec2i v)(v = SetTo123x 7)\n"
			"	(result = (v.x / v.y))\n"
			")\n"
			"(struct Vec2i\n"
			"	(Int32 x y)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestStructureAssignToAtomicCallReturn(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function SetTo123 (out Vec2i result) -> : (result = 12 3))"
			"(function Main -> (Int32 result):\n"
			"	(Vec2i v = SetTo123)\n"
			"	(result = (v.x / v.y))\n"
			")\n"
			"(struct Vec2i\n"
			"	(Int32 x y)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestStructureAssignToAtomicCallReturn2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function SetTo123 (out Vec2i result) -> : (result = 12 3))"
			"(function Main -> (Int32 result):\n"
			"	(Vec2i v)(v = SetTo123)\n"
			"	(result = (v.x / v.y))\n"
			")\n"
			"(struct Vec2i\n"
			"	(Int32 x y)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestStructureAssignToAtomicMethodCallReturn(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(class Robot (defines Sys.Type.IRobot))"
			"(method Robot.Construct : )"
			"(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))"
			"(method Robot.SetTo123 (out Vec2i result) -> : (result = 12 3))"
			"(function Main -> (Int32 result):\n"
			"	(Sys.Type.IRobot robot (Sys.Type.NewRobot))"
			"	(Vec2i v = robot.SetTo123)\n"
			"	(result = (v.x / v.y))\n"
			")\n"
			"(struct Vec2i\n"
			"	(Int32 x y)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestStructureAssignToAtomicMethodCallReturn2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(class Robot (defines Sys.Type.IRobot))"
			"(method Robot.Construct : )"
			"(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))"
			"(method Robot.SetTo123 (out Vec2i result) -> : (result = 12 3))"
			"(function Main -> (Int32 result):\n"
			"	(Sys.Type.IRobot robot (Sys.Type.NewRobot))"
			"	(Vec2i v)(v = robot.SetTo123)\n"
			"	(result = (v.x / v.y))\n"
			")\n"
			"(struct Vec2i\n"
			"	(Int32 x y)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestStructureAssignToCompoundMethodCallReturn(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(class Robot (defines Sys.Type.IRobot))\n"
			"(method Robot.Construct : )\n"
			"(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))\n"
			"(method Robot.SetTo123x (Int32 z) (out Vec3i result) -> : (result = 12 3 z))\n"
			"(function Main -> (Int32 result):\n"
			"	(Sys.Type.IRobot robot (Sys.Type.NewRobot))\n"
			"	(Vec3i v = robot.SetTo123x 7)\n"
			"	(result = v.z + (v.x / v.y))\n"
			")\n"
			"(struct Vec3i\n"
			"	(Int32 x y z)\n"
			")\n"
			"(alias Main EntryPoint.Main)\n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 11);
	}

	void TestStructureAssignToCompoundMethodCallReturn2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(class Robot (defines Sys.Type.IRobot))\n"
			"(method Robot.Construct : )\n"
			"(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))\n"
			"(method Robot.SetTo123x (Int32 z) (out Vec3i result) -> : (result = 12 3 z))\n"
			"(function Main -> (Int32 result):\n"
			"	(Sys.Type.IRobot robot (Sys.Type.NewRobot))\n"
			"	(Vec3i v)(v = robot.SetTo123x 7)\n"
			"	(result = v.z + (v.x / v.y))\n"
			")\n"
			"(struct Vec3i\n"
			"	(Int32 x y z)\n"
			")\n"
			"(alias Main EntryPoint.Main)\n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 11);
	}

	void TestStructure2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 result):\n"
			"		 (Vector3i v)\n"
			"    (v.x = 3)\n"
			"    (v.y = 5)\n"
			"    (v.z = 7)\n"
			"    (result = (SumAndProduct3i v))\n"
			")\n"
			"(function SumAndProduct3i (Vector3i a) -> (Int32 sum):\n"
			"		 (sum = ((a.x + a.y) * a.z))\n"
			")\n"
			"(struct Vector3i\n"
			"   (Int32 x y z)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStructure2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		const IModule& m = ss.PublicProgramObject().GetModule(0);
		UNUSED(m);

		vm.Push(100); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 56);
	}

	void TestStructure3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 result):\n"
			"		 (Vector3i v)\n"
			"		 (Vector3i w)\n"
			"    (v.x = 2)\n"
			"    (v.y = 3)\n"
			"    (v.z = 4)\n"
			"    (w.x = 5)\n"
			"    (w.y = 7)\n"
			"    (w.z = 9)\n"
			"    (result = (Dot3i v w))\n"
			")\n"
			"(function Dot3i (Vector3i a)(Vector3i b) -> (Int32 product):\n"
			"		 (product = ((a.z * b.z) + ((a.x * b.x) + (a.y * b.y))))\n"
			")\n"
			"(struct Vector3i\n"
			"   (Int32 x y z)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStructure3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();

		// 2*5 + 3*7 + 4*9 = 67

		validate(x == 67);
	}

	void TestStructure4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"    (alias Main EntryPoint.Main)"
			"(function Main -> (Int32 result):"
			"	 (Quat4i a)"
			"	 (Quat4i b)"
			"    (a.v.x = 2)"
			"    (a.v.y = 3)"
			"    (a.v.z = 4)"
			"    (a.s = 5)"
			"    (b.v.x = 5)"
			"    (b.v.y = 7)"
			"    (b.v.z = 9)"
			"    (b.s   = 11)"
			"    (Quat4i sum)"
			"    (AddQuat4i a b sum)"
			"    (result = 1)"
			")"
			"(function AddQuat4i (Quat4i a)(Quat4i b)(Quat4i sum) -> :"
			"	 (sum.v.x = (a.v.x + b.v.x))"
			"	 (sum.v.y = (a.v.y + b.v.y))"
			"	 (sum.v.z = (a.v.z + b.v.z))"
			"	 (sum.s   = (a.s   + b.s))"
			")"
			"(struct Vector3i"
			"   (Int32 x y z)"
			")"
			"(struct Quat4i"
			"   (Vector3i v)(Int32 s)"
			")"
			"(struct Quat4ia"
			"   (Vec3i v)(Int32 s)"
			")"
			"(struct Quat4ib"
			"   (Test.Maths.Vec3i v)(Int32 s)"
			")"
			"(using Test.Maths)"
			"(namespace Test)"
			"(namespace Test.Maths)"
			"    (alias Vector3i Test.Maths.Vec3i)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStructure4");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestWhileLoop1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 result):\n"
			"		 (Int32 count = 0)\n"
			"		 (result = 10)\n"
			"		 (while (count < 2)\n"
			"     (count = (count + 1)) (result = (result + 1)) \n"
			"    ) \n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestWhileLoop1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 12);
	}

	void TestForStatement(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
(namespace EntryPoint)
(function Main -> (Int32 result):
	(for (Int32 i = 0)(i < 4)(i += 1)
		(Sys.Print "Hello mum!&n")
	) 
)
(alias Main EntryPoint.Main))";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestDynamicDispatch(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(interface Sys.IDog (Bark (BarkEventArgs args)-> ))"
			"(interface Sys.IDispatch (attribute dispatch) (LastHz -> (Int32 hz)))\n"
			"(class Dog (implements Sys.IDog) (implements Sys.IDispatch)(Int32 lastHz))\n"
			"(struct BarkEventArgs (Int32 hz))\n"
			"(method Dog.Bark (BarkEventArgs args)-> : (this.lastHz = args.hz))\n"
			"(method Dog.Construct -> :)\n"
			"(method Dog.LastHz -> (Int32 hz) : (hz = this.lastHz))\n"
			"(factory Sys.NewDog Sys.IDispatch : (construct Dog)) \n"

			"(function Main -> (Int32 result):\n"
			"       (Sys.IDispatch dog (Sys.NewDog))\n"
			"       (BarkEventArgs args)\n"
			"		(args.hz = 42)\n"
			"		(dog.Bark args) \n"
			"       (result = dog.LastHz)\n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 42);
		s_logger.Clear();
	}

	void TestArrayInClass(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
(namespace EntryPoint)
	(alias Main EntryPoint.Main)

(using Sys.Type)

(class Robot (defines Sys.IRobot)
	(array IString commands)
)

(method Robot.Construct :
	(array IString commands)
	(this.commands = commands)
)

(factory Sys.NewRobot Sys.IRobot : (construct Robot))

(function Main -> (Int32 result):
	(Sys.IRobot robot (Sys.NewRobot))
)
)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_TERMINATED);
	}

	void TestListInClass(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
(namespace EntryPoint)
	(alias Main EntryPoint.Main)

(using Sys.Type)

(class Robot (defines Sys.IRobot)
	(list IString commands)
)

(method Robot.Construct :
	(list IString commands)
	(this.commands = commands)
)

(factory Sys.NewRobot Sys.IRobot : (construct Robot))

(function Main -> (Int32 result):
	(Sys.IRobot robot (Sys.NewRobot))
)
)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_TERMINATED);
	}

	void TestMapInClass(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
(namespace EntryPoint)
	(alias Main EntryPoint.Main)

(using Sys.Type)

(class Robot (defines Sys.IRobot)
	(map IString IString macros)
)

(method Robot.Construct :
	(map IString IString macros)
	(this.macros = macros)
)

(factory Sys.NewRobot Sys.IRobot : (construct Robot))

(function Main -> (Int32 result):
	(Sys.IRobot robot (Sys.NewRobot))
)
)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_TERMINATED);
	}

	void TestListDeleteHeadAndThrow(IPublicScriptSystem& ss)
	{
		cstr srcCode =
R"(
(namespace EntryPoint)
	(alias Main EntryPoint.Main)

(function Pop (list Int32 a)-> :
	(node n = a.Head)
	(n.Pop)
)

(function Main -> (Int32 result):
	(list Int32 a)
	(a.Append 6)
	(node n = a.Head)
	(Pop a)
	(node q = a.Head) // throws
)
)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);
		s_logger.Clear();
	}

	void TestListReverseEnumeration(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(alias Main EntryPoint.Main)\n"
			"(function Main -> (Int32 result):\n"
			"       (list Int32 a)\n"
			"       (a.Append 6)\n"
			"       (a.Append 7)\n"
			"       (a.Append 8)\n"
			"		(node n = a.Tail)\n"
			"		(do\n"
			"			(Int32 value = n.Value)\n"
			"			(result += value)\n"
			"		while n.GoPrevious) \n"
			")\n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 21);
	}

	void TestCompareInterfaces(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(alias Main EntryPoint.Main)\n"

			"(interface Sys.IRobot)\n"
			"(interface Sys.ICat)\n"
			"(class RobotCat (implements Sys.IRobot)(implements Sys.ICat))\n"
			"(method RobotCat.Construct : )\n"
			"(factory Sys.NewRobotCat Sys.IRobot : (construct RobotCat))\n"

			"(function Main -> (Int32 result):\n"
			"		(Sys.IRobot bot (Sys.NewRobotCat))\n"
			"		(cast bot -> Sys.ICat cat)"
			"		(if (bot != cat) (Sys.Throw 0 \"Bad cat\"))\n"
			"		(if (bot == cat) (return)\n"
			"		(Sys.Throw 0 \"Badder cat\"))\n"
			")\n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestWhileLoopBreak(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 result):\n"
			"		 (Int32 count = 0)\n"
			"		 (result = 10)\n"
			"		 (while (count < 6)\n"
			"     (count = (count + 1)) \n"
			"     (if (count == 4) (break)) \n"
			"     (result = (result + 1)) \n"
			"    ) \n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 13);
	}

	void TestWhileLoopContinue(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 result):\n"
			"		 (Int32 count = 0)\n"
			"		 (result = 10)\n"
			"		 (while (count < 6)\n"
			"     (count = (count + 1)) \n"
			"     (if (count == 4) (continue)) \n"
			"     (result = (result + 1)) \n"
			"    ) \n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 15);
	}

	void TestWhileLoopContinueWithStruct(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		R"(
			(namespace EntryPoint)	

			(struct StackItem4
				(Float32 x)
			)

			(function Main -> (Int32 result):
				 (Int32 count = 0)
				 (result = 10)
				 (while (count < 6)
					(count += 1)
					(if (count == 4) 
						(StackItem4 item)
						(continue)
					)
					(result += 1)
				)
			)
			(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 15);
	}

	void TestDoWhile(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 result):\n"
			"		 (Int32 count = 15)\n"
			"		 (result = 10)\n"
			"		 (do (result = (result + 1))\n"
			"    (count = (count - 1))"
			"     while (count > 0) \n"
			"    ) \n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 25);
	}

	void TestDoWhileBreak(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 result):\n"
			"		 (Int32 count = 15)\n"
			"		 (result = 10)\n"
			"		 (do (result = (result + 1))\n"
			"    (count = (count - 1))"
			"    (if (count == 2) (break))"
			"     while (count > 0) \n"
			"    ) \n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 23);
	}

	void TestDoWhileContinue(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 result):\n"
			"		 (Int32 count = 15)\n"
			"		 (result = 10)\n"
			"		 (do (count = (count - 1))\n"
			"      (if (count == 2) (continue))"
			"      (result = (result + 1))\n"			
			"     while (count > 0) \n"
			"    ) \n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDoWhileContinue");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 24);
	}

	void TestNestedWhileLoops(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(function Main -> (Int32 result):\n"
			"		 (Int32 count = 0)\n"
			"		 (Int32 i = 0)\n"
			"		 (result = 10)\n"
			"		 (while (count < 2)\n"
			"     (count = (count + 1)) \n"
			"     (i = 0) \n"
			"     (while (i < 2) (i = (i + 1)) (result = (result + 1)) ) \n"
			"    ) \n"
			")\n"
			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestNestedWhileLoops");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 14);
	}

	void TestArchetypeDeclaration(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint) \n"
			"(function Main -> (Int32 result): \n"
			"		 (result = 1) \n"
			") \n"
			"(archetype EntryPoint.FloatToFloat (Float32 x) -> (Float32 y)) \n"
			"(alias Main EntryPoint.Main) \n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArchetypeDeclaration");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1);

		const INamespace* testSpace = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(testSpace != NULL);

		const IArchetype* decl = testSpace->FindArchetype("FloatToFloat");
		validate(decl != NULL);
		validate(decl->NumberOfInputs() == 1);
		validate(decl->NumberOfOutputs() == 1);
		validate(&decl->GetArgumentType(0) == &decl->GetArgumentType(1) && decl->GetArgumentType(0).VarType() == SexyVarType_Float32);
	}

	void TestNullArchetype(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint) \n"
			"(function Main -> (Int32 result): \n"
			"		(EntryPoint.FloatToInt f)\n"
			"		(f 1 ->	result) \n"
			") \n"
			"(archetype EntryPoint.FloatToInt (Float32 x) -> (Int32 y)) \n"
			"(alias Main EntryPoint.Main) \n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestNullArchetype");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestDefaultNullMethod(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint) \n"
			"(function Main -> (Int32 result): \n"
			"		(EntryPoint.INode node)\n"
			"		(EntryPoint.INode next = node.Next) \n"
			"     (result = next.Value) \n"
			") \n"
			"(interface EntryPoint.INode \n"
		   "   (Next -> (EntryPoint.INode next)) \n"
			"   (Value -> (Int32 id)) \n"
		   ")\n"
			"(alias Main EntryPoint.Main) \n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestDefaultNullMethod");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestArchetypeCall(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint) \n"
			"(using EntryPoint) \n"
			"(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y) ) \n"
			"(function Main -> (Int32 result): \n"
			"		 (IntToInt f = Square) \n"
			"    (result = (f 5)) \n"
			") \n"
			"(function Square (Int32 x) -> (Int32 result): \n"
			"		 (result = (x * x)) \n"
			") \n"			
			"(alias Main EntryPoint.Main) \n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArchetypeCall");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

// 		IModule& m = ss.PublicProgramObject().GetModule(0);
// 		Disassemble("Main"), m, ss);

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 25);
	}

	void TestArchetypePropagation(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint) \n"
			"(using EntryPoint) \n"
			"(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y)) \n"
			"(function Main -> (Int32 result): \n"
			"    (result = (Call Square 6)) \n"
			") \n"
			"(function Square (Int32 x) -> (Int32 result): \n"
			"		 (result = (x * x)) \n"
			") \n"			
			"(function Call (EntryPoint.IntToInt f)(Int32 x) -> (Int32 result): \n"
			"		 (result = (f x)) \n"
			") \n"			
			"(alias Main EntryPoint.Main) \n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArchetypePropagation");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 36);
	}

	void TestPushSecondInterface(IPublicScriptSystem& ss)
	{
		cstr srcCode =

			"(namespace EntryPoint) \n"
			"(alias Main EntryPoint.Main)"
			"(using EntryPoint) \n"
			"(using Sys.Maths)\n"

			"(interface Sys.ICat (Go ->))"
			"(interface Sys.IRobot (Rub ->))"
			"(class RobotCat (implements Sys.ICat)(implements Sys.IRobot))"
			"(method RobotCat.Construct : )"
			"(method RobotCat.Rub -> : (Sys.Print \"Rub\"))"
			"(method RobotCat.Go -> : (TriggerRub this))"
			"(factory Sys.NewRobotCat Sys.ICat : (construct RobotCat))"

			"(function TriggerRub (Sys.IRobot robot) -> : (robot.Rub))"

			"(function Main -> (Int32 result): \n"
			"    (Sys.ICat cat (Sys.NewRobotCat))"
			"	 (cat.Go) \n"
			") \n"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}


	void TestStructArgFromStructArg(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(function OnMouseUp(MouseClicksArgs args)(Rectf rect)-> :"
			"    (Bool isInRect = (IsPointInRect args.pos rect))"
			"    (if (not isInRect) (Sys.Throw 0 \"not in rect\"))"
			")"

			"(function IsPointInRect(Vec2 point)(Rectf rect)->(Bool result) :"
			"	(if ((point.x > rect.left) and (point.x < rect.right))"
			"		(if ((point.y > rect.top) and (point.y < rect.bottom))"
			"			(result = true)"
			"		)"
			"	)"
			")"
			
			"(namespace EntryPoint) \n"
			"(alias Main EntryPoint.Main)"
			"(using EntryPoint) \n"
			"(using Sys.Maths)\n"

			"(struct MouseClicksArgs"
			"   (Vec2 pos)"
			")"
			
			"(function Main -> (Int32 result): \n"
			"    (MouseClicksArgs args = (10 10))"
			"    (Rectf rect = 0 0 100 100)"
			"	 (OnMouseUp args rect) \n"
			") \n"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}


	void TestArchetypeReturn(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint) \n"
			"(using EntryPoint) \n"
			"(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y)) \n"
			"(function Main -> (Int32 result): \n"
			"	 (IntToInt f = (GetMathsFunction)) \n"
			"    (result = (f 7)) \n"
			") \n"
			"(function Square (Int32 x) -> (Int32 result): \n"
			"		 (result = (x * x)) \n"
			") \n"			
			"(function GetMathsFunction -> (EntryPoint.IntToInt f): \n"
			"		 (f = Square) \n"
			") \n"			
			"(alias Main EntryPoint.Main) \n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArchetypeReturn");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 49);
	}

	void TestArchetypeReturnFromMultipleOutput(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint) \n"
			"(using EntryPoint) \n"
			"(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y)) \n"
			"(function Main -> (Int32 result): \n"
			"		 (IntToInt f) \n"
			"    (GetMathsFunction -> f)"
			"    (result = (f 7)) \n"
			") \n"
			"(function Square (Int32 x) -> (Int32 result): \n"
			"		 (result = (x * x)) \n"
			") \n"			
			"(function GetMathsFunction -> (EntryPoint.IntToInt f): \n"
			"		 (f = Square) \n"
			") \n"			
			"(alias Main EntryPoint.Main) \n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArchetypeReturnFromMultipleOutput");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 49);
	}

	void TestEmbeddedArchetype(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using EntryPoint)"
			"(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y) )"
			"(function Main -> (Int32 result):"
			"		 (Job job)"
			"    (job.f = Square)"
			"    (job.arg = 9)"
			"    (result = (Call job))"
			")"
			"(function Square (Int32 x) -> (Int32 result):"
			"		 (result = (x * x))"
			")"			
			"(function Call (Job job) -> (Int32 result):"
			"		 (result = (job.f job.arg))"
			")"		
			"(struct Job"
			"		 (IntToInt f)"
			"		 (Int32 arg)"
			")"			
			"(alias Main EntryPoint.Main) \n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestEmbeddedArchetype");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 81);
	}

	void TestClosure(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using EntryPoint)"
			"(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y))"
			"(function Main -> (Int32 result):"
			"		(IntToInt f = "
			"			(closure (Int32 x) -> (Int32 y):"
			"				(y = (x * x))"
			"			)"
			"		)"
			"   (result = (f 7))"
			")"	
			"(alias Main EntryPoint.Main)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestClosure");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 49);
	}

	void TestClosureInputToStruct(IPublicScriptSystem& ss)
	{
		cstr srcCode =
R"(

(namespace EntryPoint)

(using EntryPoint)

(struct Magic (Int32 a)(Int32 b))

(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y))

(function Do (Magic magic)-> :
	(IntToInt f =
			(closure (Int32 x) -> (Int32 y):
				(magic.a = x)
			)
	)
	(f 7)
)

(function Main -> (Int32 result):
	(Magic magic = 1 2)
	(Do magic)
	(result = magic.a)
)
(alias Main EntryPoint.Main)

)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 7);
	}

	void TestClosureAndThis(IPublicScriptSystem& ss)
	{
		cstr srcCode =
R"((namespace EntryPoint)
(using EntryPoint)
(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y))

(class Dog (defines Sys.IDog))

(archetype Sys.Void -> )

(method Dog.Bark -> (Int32 result):		
	(Sys.IDog dog = this)
	//(result = dog.Id)

	(Sys.Void f =
		 (closure -> :
			(result = this.Id)
		 )
	)	

	(f)
)

(method Dog.Id -> (Int32 id):		
	(id = 4)
)

(method Dog.Construct : )

(factory Sys.NewDog Sys.IDog : (construct Dog))

(function Main -> (Int32 result):
	(Sys.IDog rover4 (Sys.NewDog))
	(rover4.Bark -> result)
)
(alias Main EntryPoint.Main))";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestClosureAndThisMember(IPublicScriptSystem& ss)
	{
		cstr srcCode =
R"((namespace EntryPoint)
(using EntryPoint)
(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y))

(class Dog (defines Sys.IDog)
	(Int32 id)
)

(archetype Sys.Void -> )

(method Dog.Bark -> (Int32 result):		
	// (Sys.IDog dog = this)

	(Sys.Void f =
		 (closure -> :
			(result = this.id)
		 )
	)	

	(f)
)

(method Dog.Construct : (this.id = 4))

(factory Sys.NewDog Sys.IDog : (construct Dog))

(function Main -> (Int32 result):
	(Sys.IDog rover4 (Sys.NewDog))
	(rover4.Bark -> result)
)
(alias Main EntryPoint.Main))";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestClosureWithVariable(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using EntryPoint)"
			"(archetype EntryPoint.VoidToInt -> (Int32 y))"
			"(function Main -> (Int32 result):"
			"		(Int32 a = 2)"
			"		(VoidToInt f = "
			"			(closure -> (Int32 y):"
			"				(y = (a * 7))"
			"			)"
			"		)"
			"   (result = (f))"
			")"	
			"(alias Main EntryPoint.Main)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestClosureWithVariable");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 14);
	}

	void TestReturnClosureWithVariableSucceed(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using EntryPoint)"
			"(archetype EntryPoint.VoidToInt -> (Int32 y))"
			"(function Main -> (Int32 result):"
			"		(VoidToInt f = (CallInternalClosure))"
			"   (result = (f))"
			")"	
			"(function CallInternalClosure -> (VoidToInt result):"
			"    (result = (closure -> (Int32 y): (y = (3 * 7))))"
			")"
			"(alias Main EntryPoint.Main)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestReturnClosureWithVariableSucceed");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 21);
	}

	void TestReturnClosureWithVariableFail(IPublicScriptSystem& ss)
	{
		// Demonstrate that a returned archetype cannot access the parent variables.

		cstr srcCode =
			"(namespace EntryPoint)"
			"(using EntryPoint)"
			"(archetype EntryPoint.VoidToInt -> (Int32 y))"
			"(function Main -> (Int32 result):"
			"		(VoidToInt f = (CallInternalClosure))"
			"   (result = (f))"
			")"	
			"(function CallInternalClosure -> (VoidToInt result):"
			"    (Int32 a = 4)"
			"    (result = (closure -> (Int32 y): (y = (a * 7))))"
			")"
			"(alias Main EntryPoint.Main)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestReturnClosureWithVariableFails");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true)); // Should throw here
		validate(EXECUTERESULT_THROWN == result);
		
		validate(s_logger.ExceptionCount() > 0);
		s_logger.Clear();
	}

	void TestNativeCall(IPublicScriptSystem& ss)
	{
		struct ANON
		{
			static void CpuHz(NativeCallEnvironment& e)
			{				
				int64 hz = 170;
				WriteOutput(0, hz, e);
			}

			static void CpuTime(NativeCallEnvironment& e)
			{
            static int64 count = 0;
            count += 1000;
				WriteOutput(0, count, e);
			}
		};

		const INamespace& ns = ss.AddNativeNamespace("Sys.Time");

		try
		{
			ss.AddNativeCall(ns, ANON::CpuHz, NULL,"CpuHz -> (Int64 hz)", __FILE__, __LINE__, false, 0);
			ss.AddNativeCall(ns, ANON::CpuTime, NULL,"CpuTime -> (Int64 count)", __FILE__, __LINE__, false, 0);
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}	

		cstr srcCode =
			"(function Main -> (Int64 result):"
			"		(result = (Sys.Time.CpuHz))"
			")"				
			"(namespace EntryPoint)(alias Main EntryPoint.Main)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x1000000000000000LL); // Allocate stack space for the int64 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int64 x = vm.PopInt64();
		validate(170 == x);
	}

	static int marbleCode = 0;

	void TestEnumByCPP(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
			(function Main -> (Int32 result):
				(Sys.Time.Day day = 6)
				(result = day.Value)
			)
			(namespace EntryPoint)
				(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		const INamespace& nsTime = ss.AddNativeNamespace("Sys.Time");

		try
		{
			ss.CreateEnumType(nsTime, __FILE__, __LINE__, "Day", Rococo::SexyVarType_Int32);
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}

		ss.PartialCompile(nullptr);

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 6);
	}

	void TestRock(IPublicScriptSystem& ss)
	{
		struct Marble
		{
			int64 value1;
			int64 value2;

			Marble(): value1(marbleCode + 8), value2(marbleCode + 16)
			{
				printf("ConstructMarble: %p %llu %llu\n", this, value1, value2);
			}
		};

		struct ANON
		{
			static void AssignMarble(NativeCallEnvironment& e)
			{
				marbleCode += 3;

				Marble* marbleDest;
				ReadInput(1, marbleDest, e);

				Marble* marbleSrc;
				ReadInput(0, marbleSrc, e);

				printf("AssignMarble: dest  %p %llu %llu\n", marbleDest, marbleDest->value1, marbleDest->value2);
				printf("AssignMarble: src  %p %llu %llu\n", marbleSrc, marbleSrc->value1, marbleSrc->value2);

				marbleDest->value1 = marbleSrc->value1 + 1;
				marbleDest->value2 = marbleSrc->value2 + 2;

				printf("AssignMarble: dest  %p %llu %llu\n", marbleDest, marbleDest->value1, marbleDest->value2);
				printf("AssignMarble: src  %p %llu %llu\n", marbleSrc, marbleSrc->value1, marbleSrc->value2);
			}

			static void ConstructMarble(NativeCallEnvironment& e)
			{
				marbleCode += 7;
				Marble* marble;
				ReadInput(0, marble, e);
				new (marble) Marble();
			}

			static void DestructMarble(NativeCallEnvironment& e)
			{
				marbleCode += 9;
				Marble* m;
				ReadInput(0, m, e);

				printf("Destruct: %p %llu %llu\n", m, m->value1, m->value2);
			}
		};

		const INamespace& ns = ss.AddNativeNamespace("Sys.Rock");

		try
		{
			ss.CreateRockType(ns, __FILE__, __LINE__, "Marble", 16_bytes);

			ss.AddNativeCall(ns, ANON::ConstructMarble, NULL, "++Marble (out Sys.Rock.Marble marble)->", __FILE__, __LINE__, false, 0);
			ss.AddNativeCall(ns, ANON::DestructMarble, NULL,  "--Marble (const Sys.Rock.Marble marble)->", __FILE__, __LINE__, false, 0);
			ss.AddNativeCall(ns, ANON::AssignMarble, NULL, "->Marble (const Sys.Rock.Marble src)(out Sys.Rock.Marble dest)->", __FILE__, __LINE__, false, 0);
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}

		cstr srcCode = 
		R"(
			(function Main -> (Int32 result):
				(Sys.Rock.Marble marble)
				(if ((sizeof Sys.Rock.Marble) != 16) 
					(Sys.Throw 0 "Expecting sizeof marble to be 16 bytes")
				)

				(Sys.Rock.Marble stone = marble)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		marbleCode = 0;

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);

		validate(marbleCode == 35);
	}

	void TestConstants(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Sys.Type.Day day = .Wednesday)
				(result = day.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 3);
	}

	void TestConstants2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function ToInt (Day day) -> (Int32 value):
				(value = day.Value)
			)

			(function Main -> (Int32 result):
				(result = ToInt .Wednesday)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 3);
	}

	void TestConstants3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Day today = .Wednesday)
				(Day tomorrow = .Thursday)
				(if (today == .Friday)
					 (result = 4)
					 (return)
				)
				
				(if (today != .Friday)
					 (result += 3)
				)

				(if (today == .Wednesday)
					 (result += 5)
				)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 8);
	}

	void TestConstants4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(struct Time
				(Sys.Type.Day day)
				(Int32 seconds)
			)

			(function Main -> (Int32 result):
				(Time t = .Friday 1)
				(result = t.day.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 5);
	}

	void TestConstants5(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(array Day days 5)
				(days.Push 1)
				(days.Push .Friday)
				(result = (days 1))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 5);
	}

	void TestConstants6(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(array Day days 5)
				(days.Push 1)
				(days.Set 0 .Friday)
				(result = (days 0))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 5);
	}

	void TestConstants7(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(list Day a)
				(a.Append .Friday)
				(node na = a.Head)

				(list Day b)
				(b.Prepend .Monday)
				(node nb = b.Head)

				(list Day c)
				(c.Append .Monday)
				(c.Append .Friday)
				(node hc = c.Head)
				(hc.Append .Wednesday)
				(node mid = hc.Next)

				(Int32 va = na.Value)
				(Int32 vb = nb.Value)
				(Int32 vm = mid.Value)
				(result = ((va + (vb * 10)) + (vm * 100)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 315);
	}

	void TestConstants8(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(map Int32 Day mv)
				(mv.Insert 45 .Friday)
				(node nv = (mv 45))

				(map Day Int32 mk)
				(mk.Insert .Friday 7)
				(node nk = (mk .Friday))

				(Int32 vv = nv.Value)
				(Int32 vk = nk.Value)
				(result = (vv + (vk * 10)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 75);
	}

	void TestConstants9(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
			(strong BigDay (Int64))

			(alias BigDay Sys.Type.BigDay)

			(enum Sys.Type.BigDay
				(Monday 1)
				(Huge 4294967297)
			)

			(function Main -> (Int32 result):
				(array BigDay days 5)
				(days.Push .Monday)
				(days.Set 0 .Huge)
				(BigDay d = (days 0))

				(list BigDay lst)
				(lst.Append .Huge)
				(node n = lst.Head)

				(map Int32 BigDay mv)
				(mv.Insert 1 .Huge)
				(node nv = (mv 1))

				(map BigDay Int32 mk)
				(mk.Insert .Huge 2)
				(node nk = (mk .Huge))

				(Int64 huge = 4294967297)
				(Int64 vd = d.Value)
				(Int64 vn = n.Value)
				(Int64 vnv = nv.Value)
				(Int32 vk = nk.Value)
				(Int32 ok = 0)
				(if (vd == huge) (ok += 1))
				(if (vn == huge) (ok += 1))
				(if (vnv == huge) (ok += 1))
				(if (vk == 2) (ok += 1))
				(result = ok)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestConstantsCompoundEnum(IPublicScriptSystem& ss)
	{
		// Hand-written one-element compound holding an enum constant: (Day day = (.Friday))
		cstr srcCode =
			R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Day day = (.Friday))
				(Int32 v = day.Value)
				(result = v)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 5);
	}

	void TestConstantsCompoundEnumMember(IPublicScriptSystem& ss)
	{
		// Memberwise init from a one-element compound holding an enum constant: (Time t = (.Friday) 1)
		cstr srcCode =
			R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(struct Time
				(Sys.Type.Day day)
				(Int32 seconds)
			)

			(function Main -> (Int32 result):
				(Time t = (.Friday) 1)
				(result = t.day.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 5);
	}

	void TestConstantsCompoundEnumArg(IPublicScriptSystem& ss)
	{
		// Argument passing from a one-element compound holding an enum constant: (ToInt (.Friday))
		cstr srcCode =
			R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function ToInt (Day day) -> (Int32 value):
				(value = day.Value)
			)

			(function Main -> (Int32 result):
				(result = ToInt (.Friday))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 5);
	}

	// Qualified enum constants: <EnumType>@<Value>, e.g. Day@Friday or Sys.Type.Day@Friday

	static int32 RunEnumQualifiedTest(IPublicScriptSystem& ss, cstr srcCode, cstr name)
	{
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, name);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		return vm.PopInt32();
	}

	// Expects compilation to fail with a message containing expectedFragment
	static void RunEnumQualifiedFailTest(IPublicScriptSystem& ss, cstr srcCode, cstr name, cstr expectedFragment)
	{
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, name);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);
		ParseException ex;
		validate(s_logger.TryGetNextException(ex));
		if (strstr(ex.Message(), expectedFragment) == nullptr)
		{
			printf("Expected '%s' in: %s\n", expectedFragment, ex.Message());
			validate(false);
		}
		s_logger.Clear();
	}

	void TestEnumQualified(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Day day = Day@Wednesday)
				(result = day.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 3);
	}

	void TestEnumQualifiedNamespace(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Sys.Type.Day day = Sys.Type.Day@Friday)
				(Day other = Sys.Type.Day@Monday)
				(Int32 a = day.Value)
				(Int32 b = other.Value)
				(result = ((a * 10) + b))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 51);
	}

	void TestEnumQualifiedCompound(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Day day = (Day@Friday))
				(Day other)
				(other = (Sys.Type.Day@Tuesday))
				(Int32 a = day.Value)
				(Int32 b = other.Value)
				(result = ((a * 10) + b))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 52);
	}

	void TestEnumQualifiedMember(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(struct Time
				(Sys.Type.Day day)
				(Int32 seconds)
			)

			(function Main -> (Int32 result):
				(Time t = Day@Friday 1)
				(Time u = (Sys.Type.Day@Thursday) 2)
				(Int32 a = t.day.Value)
				(Int32 b = u.day.Value)
				(result = ((a * 10) + b))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 54);
	}

	void TestEnumQualifiedArg(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function ToInt (Day day) -> (Int32 value):
				(value = day.Value)
			)

			(function Main -> (Int32 result):
				(Int32 a = (ToInt Day@Friday))
				(Int32 b = (ToInt (Sys.Type.Day@Monday)))
				(result = ((a * 10) + b))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 51);
	}

	void TestEnumQualifiedCompare(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Day today = Day@Wednesday)
				(if (today == Day@Friday)
					 (result = 4)
					 (return)
				)
				
				(if (today != Sys.Type.Day@Friday)
					 (result += 3)
				)

				(if (today == Sys.Type.Day@Wednesday)
					 (result += 5)
				)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 8);
	}

	void TestEnumQualifiedArray(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(array Day days 5)
				(days.Push Day@Monday)
				(days.Push Sys.Type.Day@Tuesday)
				(days.Set 0 Day@Friday)
				(Day d0 = (days 0))
				(Day d1 = (days 1))
				(Int32 a = d0.Value)
				(Int32 b = d1.Value)
				(result = ((a * 10) + b))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 52);
	}

	void TestEnumQualifiedList(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(list Day a)
				(a.Append Day@Friday)
				(node na = a.Head)

				(list Day b)
				(b.Prepend Sys.Type.Day@Monday)
				(node nb = b.Head)

				(list Day c)
				(c.Append Day@Monday)
				(c.Append Day@Friday)
				(node hc = c.Head)
				(hc.Append Day@Wednesday)
				(node mid = hc.Next)

				(Int32 va = na.Value)
				(Int32 vb = nb.Value)
				(Int32 vm = mid.Value)
				(result = ((va + (vb * 10)) + (vm * 100)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 315);
	}

	void TestEnumQualifiedMap(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(map Int32 Day mv)
				(mv.Insert 45 Day@Friday)
				(node nv = (mv 45))

				(map Day Int32 mk)
				(mk.Insert Sys.Type.Day@Friday 7)
				(node nk = (mk Day@Friday))

				(Int32 vv = nv.Value)
				(Int32 vk = nk.Value)
				(result = (vv + (vk * 10)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 75);
	}

	void TestEnumQualifiedUnknownValue(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Day day = Day@Fryday)
				(result = day.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Unrecognized enum Fryday for");
	}

	void TestEnumQualifiedWrongType(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(strong Colour (Int32))
			(alias Colour Sys.Type.Colour)
			(enum Sys.Type.Colour
				(Red 1)
				Green
			)

			(function Main -> (Int32 result):
				(Day day = Colour@Red)
				(result = day.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Type mismatch");
	}

	void TestEnumQualifiedToInt32(IPublicScriptSystem& ss)
	{
		// A qualified enum constant initializes its underlying integer type with its value
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Int32 x = Day@Friday)
				(result = x)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 5);
	}

	void TestEnumQualifiedUnknownType(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Day day = Dya@Friday)
				(result = day.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Unknown enum type Dya");
	}

	void TestEnumQualifiedBadSyntax(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Day day = Day@)
				(result = day.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Expected <EnumType>@<Value>");
	}

	void TestEnumQualifiedArithmetic(IPublicScriptSystem& ss)
	{
		// A qualified enum constant is a strong type, so it does not take part in Int32 arithmetic
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Int32 x = (1 + Day@Friday))
				(result = x)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "is an enum constant of strong type");
	}

	void TestEnumQualifiedAsFunction(IPublicScriptSystem& ss)
	{
		// The head of a compound statement is a function name, so (Day@Friday 1) reports an enum constant where a type or function name is expected
		cstr srcCode = R"(
			(strong Day (Int32))

			(alias Day Sys.Type.Day)

			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(function Main -> (Int32 result):
				(Day@Friday 1)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "is an enum constant, not a");
	}

	// Bitwise OR on strong enums. Both operands are of the enum type, and so is the result, which need not be a named member

	void TestEnumOrInit(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = (Access@Read | Access@Write))
				(Access b = (.Read | .Exec))
				(Access c = (Sys.Type.Access@Read | (.Write | (.Exec))))
				(Int32 va = a.Value)
				(Int32 vb = b.Value)
				(Int32 vc = c.Value)
				(result = ((va * 100) + ((vb * 10) + vc)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 357);
	}

	void TestEnumOrAssign(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(a = (a | .Exec))
				(Access b)
				(b = (Access@Write | a))
				(Access c = .Write)
				(Access d = (a | c))
				(Int32 va = a.Value)
				(Int32 vb = b.Value)
				(Int32 vd = d.Value)
				(result = ((va * 100) + ((vb * 10) + vd)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 577);
	}

	void TestEnumOrMember(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(struct Perm
				(Access access)
				(Int32 n)
			)

			(function Main -> (Int32 result):
				(Perm p = (Access@Read | Access@Write) 9)
				(Int32 before = p.access.Value)
				(p.access = (p.access | .Exec))
				(Access a = (p.access | Access@Read))
				(Int32 va = a.Value)
				(result = ((before * 100) + ((va * 10) + p.n)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 379);
	}

	void TestEnumOrArg(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function ToInt (Access a) -> (Int32 value):
				(value = a.Value)
			)

			(function Main -> (Int32 result):
				(Access w = .Write)
				(Int32 x = (ToInt (Access@Read | Access@Write)))
				(Int32 y = (ToInt (.Read | .Exec)))
				(Int32 z = (ToInt (w | .Exec)))
				(result = ((x * 100) + ((y * 10) + z)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 356);
	}

	void TestEnumOrReturn(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function Combine (Access a)(Access b) -> (Access c):
				(c = (a | b))
			)

			(function AllAccess -> (Access c):
				(c = (.Read | (.Write | .Exec)))
			)

			(function Main -> (Int32 result):
				(Access r = (Combine .Read .Exec))
				(Access s = (AllAccess))
				(Int32 vr = r.Value)
				(Int32 vs = s.Value)
				(result = ((vr * 10) + vs))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 57);
	}

	void TestEnumOrCompare(IPublicScriptSystem& ss)
	{
		// Read | Write is 3, which is not a named member, but is still a valid Access value
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = (Access@Read | Access@Write))
				(if (a == (Access@Read | Access@Write))
					(result += 1)
				)
				(if (a != (.Read | .Exec))
					(result += 10)
				)
				(if (a == (.Read | .Exec))
					(result += 1000)
				)
				(if (a == Access@Exec)
					(result += 1000)
				)
				(Int32 va = a.Value)
				(if (va == 3)
					(result += 100)
				)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 111);
	}

	void TestEnumOrInt64(IPublicScriptSystem& ss)
	{
		// An Int64 enum keeps its high bits through |
		cstr srcCode = R"(
			(strong BigAccess (Int64))

			(alias BigAccess Sys.Type.BigAccess)

			(enum Sys.Type.BigAccess
				(Low 1)
				(High 4294967296)
			)

			(function Main -> (Int32 result):
				(BigAccess a = (BigAccess@Low | .High))
				(BigAccess b = .Low)
				(b = (b | a))
				(Int64 expected = 4294967297)
				(Int64 va = a.Value)
				(Int64 vb = b.Value)
				(if (va == expected) (result += 1))
				(if (vb == expected) (result += 10))
				(if (b == (.High | .Low)) (result += 100))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 111);
	}

	void TestEnumOrMixedConstants(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(strong Colour (Int32))

			(alias Colour Sys.Type.Colour)

			(enum Sys.Type.Colour
				(Red 1)
				(Green 2)
			)

			(function Main -> (Int32 result):
				(Access a = (Access@Read | Colour@Red))
				(result = a.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Type mismatch");
	}

	void TestEnumOrMixedVariables(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(strong Colour (Int32))

			(alias Colour Sys.Type.Colour)

			(enum Sys.Type.Colour
				(Red 1)
				(Green 2)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(Colour c = .Red)
				(Access b = (a | c))
				(result = b.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "c is of type Colour, but the enum expression is of type Access");
	}

	void TestEnumOrIntLiteral(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = (Access@Read | 2))
				(result = a.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "2 is a literal, which does not convert to enum type Access");
	}

	void TestEnumOrIntVariable(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Int32 i = 2)
				(Access a = (.Read | i))
				(result = a.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "i is of type Int32, but the enum expression is of type Access");
	}

	void TestEnumOrIntoInt32(IPublicScriptSystem& ss)
	{
		// An enum | assigned to a plain integer yields the underlying value
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function ToInt32 (Int32 x) -> (Int32 value):
				(value = x)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(Int32 i = (a | .Write))
				(Int32 j = (Access@Read | Access@Exec))
				(Int32 k)
				(k = (Access@Write | (a | .Exec)))
				(Int32 m = (ToInt32 (Access@Exec | a)))
				(result = ((i * 1000) + ((j * 100) + ((k * 10) + m))))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 3575);
	}

	void TestEnumOrIntoInt64(IPublicScriptSystem& ss)
	{
		// An Int32 enum | widens to an Int64 target, keeping its sign
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function ToInt64 (Int64 x) -> (Int64 value):
				(value = x)
			)

			(function Main -> (Int32 result):
				(Access a = .Write)
				(Int64 value = (Access@Read | Access@Write))
				(Int64 v2)
				(v2 = (a | .Exec))
				(Int64 v3 = (ToInt64 (Access@Exec | .Read)))
				(Int64 v4 = (Access@Sign | Access@Read))
				(Int64 expected4 = -2147483647)
				(if (value == 3) (result += 1))
				(if (v2 == 6) (result += 10))
				(if (v3 == 5) (result += 100))
				(if (v4 == expected4) (result += 1000))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 1111);
	}

	void TestEnumOrInt64IntoInt64(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong BigAccess (Int64))

			(alias BigAccess Sys.Type.BigAccess)

			(enum Sys.Type.BigAccess
				(Low 1)
				(High 4294967296)
			)

			(function Main -> (Int32 result):
				(Int64 value = (BigAccess@Low | BigAccess@High))
				(Int64 expected = 4294967297)
				(if (value == expected) (result = 1))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 1);
	}

	void TestEnumOrInt64IntoInt32(IPublicScriptSystem& ss)
	{
		// An Int64 enum does not narrow to Int32
		cstr srcCode = R"(
			(strong BigAccess (Int64))

			(alias BigAccess Sys.Type.BigAccess)

			(enum Sys.Type.BigAccess
				(Low 1)
				(High 4294967296)
			)

			(function Main -> (Int32 result):
				(Int32 value = (BigAccess@Low | BigAccess@High))
				(result = value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "which does not convert to Int32");
	}

	void TestEnumOrUnqualifiedIntoInt32(IPublicScriptSystem& ss)
	{
		// With a plain integer target, and no qualified constant or enum variable, the type of .Read cannot be inferred
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Int32 value = (.Read | .Write))
				(result = value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Cannot infer the enum type of .Read");
	}

	void TestEnumOrFloat(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Float32 f = 1)
				(Access a = (.Read | f))
				(result = a.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "f is of type Float32, but the enum expression is of type Access");
	}

	void TestEnumOrFloatEnum(IPublicScriptSystem& ss)
	{
		// | needs an integer underlying type
		cstr srcCode = R"(
			(strong Shade (Float32))

			(alias Shade Sys.Type.Shade)

			(enum Sys.Type.Shade
				(Light 1)
				(Dark 2)
			)

			(function Main -> (Int32 result):
				(Shade s = (Shade@Light | Shade@Dark))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "| requires an integer enum type");
	}

	void TestEnumOrOtherOperators(IPublicScriptSystem& ss)
	{
		// Only | is defined for enums
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = (Access@Read + Access@Write))
				(result = a.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Only | is defined for enum type Access");
	}

	void TestEnumOrOtherOperatorsVariable(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(Access b = .Write)
				(Access c = (a * b))
				(result = c.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Only | is defined for enum type Access");
	}

	// Delta flag operators: (x |= y) is x = x | y, and (x &~= y) is x = x & ~y

	void TestDeltaOrInt32(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(function Main -> (Int32 result):
				(Int32 a = 1)
				(a |= 6)
				(Int32 b = 0x10)
				(Int32 c = 3)
				(b |= c)
				(Int32 d = 8)
				(d |= (c | 0x20))
				(result = ((a * 10000) + ((b * 100) + d)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 71943);
	}

	void TestDeltaClearInt32(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(function Main -> (Int32 result):
				(Int32 a = 7)
				(a &~= 2)
				(Int32 b = 0x1F)
				(Int32 c = 0x11)
				(b &~= c)
				(Int32 d = 0x3F)
				(d &~= (c | 0x20))
				(Int32 e = 4)
				(e &~= 1)
				(result = ((a * 1000000) + ((b * 10000) + ((d * 100) + e))))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		// a = 5, b = 0x0E = 14, d = 0x0E = 14, e = 4 (clearing an unset bit is a no-op)
		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 5141404);
	}

	void TestDeltaInt32SignBit(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(function Main -> (Int32 result):
				(Int32 a = 1)
				(a |= -2)
				(Int32 b = -1)
				(b &~= 1)
				(Int32 c = 0)
				(c |= 0x80000000)
				(Int32 d = -1)
				(d &~= 0x80000000)
				(if (a == -1) (result += 1))
				(if (b == -2) (result += 10))
				(if (c == -2147483648) (result += 100))
				(if (d == 2147483647) (result += 1000))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 1111);
	}

	void TestDeltaInt64(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(function Main -> (Int32 result):
				(Int64 a = 1)
				(a |= 0x100000000)
				(Int64 hi = 0x200000000)
				(Int64 b = 5)
				(b |= hi)
				(Int64 c = 0x300000003)
				(c &~= 0x100000001)
				(Int64 d = 0x300000003)
				(d &~= hi)
				(Int64 e = 0)
				(e |= (hi | 1))
				(Int64 f = -1)
				(f &~= 0x8000000000000000)
				(Int64 ea = 0x100000001)
				(Int64 eb = 0x200000005)
				(Int64 ec = 0x200000002)
				(Int64 ed = 0x100000003)
				(Int64 ee = 0x200000001)
				(Int64 ef = 0x7FFFFFFFFFFFFFFF)
				(if (a == ea) (result += 1))
				(if (b == eb) (result += 10))
				(if (c == ec) (result += 100))
				(if (d == ed) (result += 1000))
				(if (e == ee) (result += 10000))
				(if (f == ef) (result += 100000))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 111111);
	}

	void TestDeltaOrEnum(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Share 8)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(a |= Access@Write)
				(Access b = .Read)
				(b |= .Exec)
				(Access c = .Share)
				(c |= (.Read | .Write))
				(Access d = .Share)
				(d |= (Sys.Type.Access@Exec | Access@Read))
				(Access e = .Read)
				(e |= (.Exec))
				(Access f = .Share)
				(f |= b)
				(Int32 va = a.Value)
				(Int32 vb = b.Value)
				(Int32 vc = c.Value)
				(Int32 vd = d.Value)
				(Int32 ve = e.Value)
				(Int32 vf = f.Value)
				(result = ((va * 100000) + ((vb * 10000) + ((vc * 1000) + ((vd * 100) + ((ve * 10) + vf))))))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		// a = 3, b = 5, c = 11, d = 13, e = 5, f = 13
		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == (3 * 100000 + 5 * 10000 + 11 * 1000 + 13 * 100 + 5 * 10 + 13));
	}

	void TestDeltaClearEnum(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = (.Read | (.Write | .Exec)))
				(a &~= .Write)
				(Access w = .Exec)
				(Access b = (.Read | .Exec))
				(b &~= w)
				(Access c = .Read)
				(c &~= Access@Exec)
				(Access d = (.Read | (.Write | .Exec)))
				(d &~= (.Read | .Exec))
				(Int32 va = a.Value)
				(Int32 vb = b.Value)
				(Int32 vc = c.Value)
				(Int32 vd = d.Value)
				(result = ((va * 1000) + ((vb * 100) + ((vc * 10) + vd))))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		// a = 5, b = 1, c = 1 (Exec was not set, so no change), d = 2
		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 5112);
	}

	void TestDeltaEnumRoundTrip(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
			)

			(struct Perm
				(Access access)
				(Int32 n)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(a |= .Exec)
				(if (a == (.Read | .Exec)) (result += 1))
				(a &~= .Exec)
				(if (a == .Read) (result += 10))
				(Perm p = .Read 7)
				(p.access |= .Write)
				(p.access &~= .Read)
				(Access q = p.access)
				(if (q == .Write) (result += 100))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 111);
	}

	void TestEnumConstantExponentLikeName(IPublicScriptSystem& ss)
	{
		// .Exec begins like the exponent of a float literal, but is an enum constant
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = .Exec)
				(result = a.Value)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 4);
	}

	void TestDeltaOrFloatRejected(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(function Main -> (Int32 result):
				(Float32 f = 1)
				(f |= 2)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Delta operator '|=' requires an Int32, Int64 or strong enum lhs, but f is Float32");
	}

	void TestDeltaClearFloatRejected(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(function Main -> (Int32 result):
				(Float64 f = 1)
				(f &~= 2)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Delta operator '&~=' requires an Int32, Int64 or strong enum lhs, but f is Float64");
	}

	void TestDeltaOrEnumIntRejected(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(a |= 2)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "2 is a literal, which does not convert to enum type Access");
	}

	void TestDeltaOrEnumOtherTypeRejected(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
			)

			(strong Colour (Int32))

			(alias Colour Sys.Type.Colour)

			(enum Sys.Type.Colour
				(Red 1)
				(Green 2)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(a |= Colour@Red)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Type mismatch: Colour@Red is of type Colour, but the target type is Access");
	}

	void TestDeltaClearEnumOtherTypeRejected(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
			)

			(strong Colour (Int32))

			(alias Colour Sys.Type.Colour)

			(enum Sys.Type.Colour
				(Red 1)
				(Green 2)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(Colour c = .Red)
				(a &~= c)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "c is of type Colour, but the enum expression is of type Access");
	}

	void TestDeltaAddEnumRejected(IPublicScriptSystem& ss)
	{
		// Only the flag operators apply to enums
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(a += 1)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Only |= and &~= apply to enum type Access");
	}

	// Strong types that are not enums, e.g. (strong Joules (Float64)), follow the arithmetic rules of their underlying type

	void TestStrongMathFloat(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Joules (Float64))

			(function Main -> (Int32 result):
				(Joules j = 1000)
				(j *= 2)
				(j += 500)
				(j -= 100)
				(j /= 2)
				(Joules k = 50)
				(j += k)
				(j -= k)
				(Joules s = (j + j))
				(Joules t = (j * 2))
				(Float64 vj = j.Value)
				(Float64 vs = s.Value)
				(Float64 ej = 1200)
				(Float64 es = 2400)
				(if (vj == ej) (result += 1))
				(if (vs == es) (result += 10))
				(if (s == t) (result += 100))
				(if (s == (j * 2)) (result += 1000))
				(if (k != (j + j)) (result += 10000))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 11111);
	}

	void TestStrongMathInt(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Count (Int32))
			(strong Ticks (Int64))

			(function Main -> (Int32 result):
				(Count c = 10)
				(c += 5)
				(c -= 3)
				(c *= 4)
				(c /= 6)
				(Count two = 2)
				(c *= two)
				(c -= two)
				(Count d = (c + c))
				(Count e = (c * 2))
				(Count f = (c + 1))
				(Ticks t = 0x100000000)
				(t *= 2)
				(Int64 vt = t.Value)
				(Int64 et = 0x200000000)
				(Int32 vc = c.Value)
				(Int32 vd = d.Value)
				(Int32 vf = f.Value)
				(if (vc == 14) (result += 1))
				(if (vd == 28) (result += 10))
				(if (d == e) (result += 100))
				(if (d == (c * 2)) (result += 1000))
				(if (vf == 15) (result += 10000))
				(if (vt == et) (result += 100000))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 111111);
	}

	void TestStrongMathExpressions(IPublicScriptSystem& ss)
	{
		// Arithmetic into a non-enum strong target, and a comparison with an arithmetic expression, without delta operators
		cstr srcCode = R"(
			(strong Joules (Float64))
			(strong Count (Int32))

			(function Main -> (Int32 result):
				(Joules j = 3)
				(Joules k = (j + j))
				(Joules m = (j * 2))
				(Count c = 4)
				(Count d = (c - 1))
				(Count e = (c / 2))
				(Int32 vd = d.Value)
				(Int32 ve = e.Value)
				(if (k == m) (result += 1))
				(if (k == (j * 2)) (result += 10))
				(if (d == (c - 1)) (result += 100))
				(if (vd == 3) (result += 1000))
				(if (ve == 2) (result += 10000))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 11111);
	}

	void TestStrongFlagsInt(IPublicScriptSystem& ss)
	{
		// |= and &~= apply to an integer-based strong type that is not an enum, as they do to its underlying type
		cstr srcCode = R"(
			(strong Mask (Int32))

			(function Main -> (Int32 result):
				(Mask m = 1)
				(m |= 6)
				(m &~= 2)
				(Int32 a = m.Value)
				(Mask x = 8)
				(m |= x)
				(Int32 b = m.Value)
				(m &~= (x | 1))
				(Int32 c = m.Value)
				(result = ((a * 10000) + ((b * 100) + c)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 51304);
	}

	void TestStrongFlagsFloatRejected(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Joules (Float64))

			(function Main -> (Int32 result):
				(Joules j = 1000)
				(j |= 1)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Delta operator '|=' requires an Int32, Int64 or strong enum lhs, but j is Float64");
	}

	// Flag tests: (v & .Key) and (v & E@Key) are Bool, true if (v & key) != 0

	void TestEnumFlagTestDot(IPublicScriptSystem& ss)
	{
		// (v & .Key) is true when any bit of Key is set in v
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = (.Read | .Exec))
				(if (a & .Read) (result += 1))
				(if (a & .Write) (result += 10))
				(if (a & .Exec) (result += 100))
				(Bool hasWrite = (a & .Write))
				(Bool hasExec = (a & .Exec))
				(if (not hasWrite) (result += 1000))
				(if hasExec (result += 10000))
				(if (not (a & .Write)) (result += 100000))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 111101);
	}

	void TestEnumFlagTestQualified(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = .Write)
				(if (a & Access@Write) (result += 1))
				(if (a & Sys.Type.Access@Write) (result += 10))
				(if (a & Access@Read) (result += 100))
				(Bool b = false)
				(b = (a & Access@Write))
				(if b (result += 1000))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 1011);
	}

	void TestEnumFlagTestInt32(IPublicScriptSystem& ss)
	{
		// A plain integer of the underlying type takes a qualified key
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Int32 x = 5)
				(if (x & Access@Read) (result += 1))
				(if (x & Access@Write) (result += 10))
				(if (x & Sys.Type.Access@Exec) (result += 100))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 101);
	}

	void TestEnumFlagTestInt64(IPublicScriptSystem& ss)
	{
		// High bits survive the test for an Int64 enum
		cstr srcCode = R"(
			(strong BigAccess (Int64))

			(alias BigAccess Sys.Type.BigAccess)

			(enum Sys.Type.BigAccess
				(Low 1)
				(High 4294967296)
			)

			(function Main -> (Int32 result):
				(Int64 x = 0x100000000)
				(BigAccess b = .High)
				(if (x & BigAccess@High) (result += 1))
				(if (x & BigAccess@Low) (result += 10))
				(if (b & .High) (result += 100))
				(if (b & .Low) (result += 1000))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 101);
	}

	void TestEnumFlagTestMember(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(struct Perm
				(Access access)
				(Int32 n)
			)

			(function Main -> (Int32 result):
				(Perm p = (.Read | .Write) 7)
				(if (p.access & .Write) (result += 1))
				(if (p.access & Access@Exec) (result += 10))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 1);
	}

	void TestEnumFlagTestLogic(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = (.Read | .Write))
				(if ((a & .Read) and (a & .Write)) (result += 1))
				(if ((a & .Read) and (a & .Exec)) (result += 10))
				(if ((a & .Exec) or (a & Access@Write)) (result += 100))
				(if ((a & .Exec) or (a & .Exec)) (result += 1000))
				(Bool both = ((a & .Read) and (a & .Write)))
				(if both (result += 10000))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 10101);
	}

	void TestEnumFlagTestMultiBit(IPublicScriptSystem& ss)
	{
		// A key with several bits, such as ReadWrite = 3, tests true if ANY of its bits is set, not all of them
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = .Write)
				(if (a & .ReadWrite) (result += 1))
				(Access e = .Exec)
				(if (e & .ReadWrite) (result += 10))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 1);
	}

	void TestEnumFlagTestDelta(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(a |= .Exec)
				(if (a & .Exec) (result += 1))
				(a &~= .Exec)
				(if (a & .Exec) (result += 10))
				(if (a & .Read) (result += 100))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 101);
	}

	void TestEnumFlagTestWhile(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = (.Read | (.Write | .Exec)))
				(while (a & .Write)
					(a &~= .Write)
					(result += 1)
				)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 1);
	}

	void TestEnumFlagTestUnknownKey(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(Bool b = (a & .Delete))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Unrecognized enum Delete for Access");
	}

	void TestEnumFlagTestOtherEnum(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(strong Colour (Int32))

			(alias Colour Sys.Type.Colour)

			(enum Sys.Type.Colour
				(Red 1)
				(Green 2)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(Bool b = (a & Colour@Red))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Type mismatch: Colour@Red is of type Colour, but a is of type Access");
	}

	void TestEnumFlagTestInt64VarInt32Enum(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Int64 x = 1)
				(Bool b = (x & Access@Read))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "x is Int64, but Access@Read is of type Access, which is based on Int32");
	}

	void TestEnumFlagTestInt32VarInt64Enum(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong BigAccess (Int64))

			(alias BigAccess Sys.Type.BigAccess)

			(enum Sys.Type.BigAccess
				(Low 1)
				(High 4294967296)
			)

			(function Main -> (Int32 result):
				(Int32 x = 1)
				(Bool b = (x & BigAccess@Low))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "x is Int32, but BigAccess@Low is of type BigAccess, which is based on Int64");
	}

	void TestEnumFlagTestFloatVar(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Float32 x = 1)
				(Bool b = (x & Access@Read))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "x is Float32, but Access@Read is of type Access, which is based on Int32");
	}

	void TestEnumFlagTestNonEnumStrong(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Mask (Int32))

			(function Main -> (Int32 result):
				(Mask m = 1)
				(Bool b = (m & .Read))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "m is of type Mask, which has no enum values");
	}

	void TestEnumFlagTestIntDot(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Int32 x = 1)
				(Bool b = (x & .Read))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Cannot infer the enum type of .Read from Int32 x. Qualify it, e.g. <EnumType>@Read");
	}

	void TestEnumFlagTestLiteral(IPublicScriptSystem& ss)
	{
		// Only enum keys are accepted on the right of &
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(ReadWrite 3)
				(Exec 4)
			)

			(function Main -> (Int32 result):
				(Access a = .Read)
				(Bool b = (a & 1))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Expecting an enum key on the right of &");
	}

	// Qualified enum constants into plain integers, and enums in the (?? cond a b) ternary

	void TestEnumToIntInit(IPublicScriptSystem& ss)
	{
		// A qualified enum constant initializes a plain integer with its value
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Int32 x = Access@Write)
				(Int32 y = Sys.Type.Access@Exec)
				(Int32 z = (Access@Read))
				(result = ((x * 100) + ((y * 10) + z)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 241);
	}

	void TestEnumToIntAssign(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function GetWrite -> (Int32 value):
				(value = Access@Write)
			)

			(function Main -> (Int32 result):
				(Int32 x = 0)
				(x = Access@Exec)
				(Int32 y = (GetWrite))
				(result = ((x * 10) + y))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 42);
	}

	void TestEnumToIntMember(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(struct Pair
				(Int32 a)
				(Int64 b)
			)

			(function Main -> (Int32 result):
				(Pair p = Access@Write Access@Exec)
				(Int32 a = p.a)
				(Int64 b = p.b)
				(Int64 four = 4)
				(if (b == four) (result = (a * 10)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 20);
	}

	void TestEnumToIntArg(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Twice32 (Int32 x) -> (Int32 value):
				(value = (x * 2))
			)

			(function Twice64 (Int64 x) -> (Int64 value):
				(value = (x * 2))
			)

			(function Main -> (Int32 result):
				(Int32 a = (Twice32 Access@Exec))
				(Int64 b = (Twice64 Sys.Type.Access@Write))
				(Int64 four = 4)
				(if (b == four) (result = a))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 8);
	}

	void TestEnumToIntWiden(IPublicScriptSystem& ss)
	{
		// An Int32 enum widens to Int64 with its sign kept
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Int64 x = Access@Sign)
				(Int64 expected = -2147483648)
				(if (x == expected) (result = 1))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 1);
	}

	void TestEnumToInt64(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong BigAccess (Int64))

			(alias BigAccess Sys.Type.BigAccess)

			(enum Sys.Type.BigAccess
				(Low 1)
				(High 4294967296)
			)

			(function Main -> (Int32 result):
				(Int64 x = BigAccess@High)
				(Int64 expected = 4294967296)
				(if (x == expected) (result = 1))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 1);
	}

	void TestEnumToIntNarrow(IPublicScriptSystem& ss)
	{
		// An Int64 enum does not narrow to Int32
		cstr srcCode = R"(
			(strong BigAccess (Int64))

			(alias BigAccess Sys.Type.BigAccess)

			(enum Sys.Type.BigAccess
				(Low 1)
				(High 4294967296)
			)

			(function Main -> (Int32 result):
				(Int32 x = BigAccess@High)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "BigAccess@High is an enum constant of type BigAccess, based on Int64, which does not convert to Int32");
	}

	void TestEnumToIntFloat(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Float32 x = Access@Read)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Access@Read is an enum constant of type Access, based on Int32, which does not convert to Float32");
	}

	void TestEnumToIntUnqualified(IPublicScriptSystem& ss)
	{
		// With a plain integer target the enum type of .Read cannot be inferred
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Int32 x = .Read)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Cannot infer the enum type of .Read. Qualify it, e.g. <EnumType>@Read");
	}

	void TestEnumToIntArgUnqualified(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Twice32 (Int32 x) -> (Int32 value):
				(value = (x * 2))
			)

			(function Main -> (Int32 result):
				(Int32 a = (Twice32 .Read))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Cannot infer the enum type of .Read. Qualify it, e.g. <EnumType>@Read");
	}

	void TestEnumToOtherEnum(IPublicScriptSystem& ss)
	{
		// An enum constant does not convert to another enum type
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(strong Colour (Int32))

			(alias Colour Sys.Type.Colour)

			(enum Sys.Type.Colour
				(Red 1)
				(Green 2)
			)

			(function Main -> (Int32 result):
				(Colour c = Access@Read)
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Type mismatch: Access@Read is of type Access, but the target type is Colour");
	}

	void TestEnumTernaryDot(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Bool yes = true)
				(Bool no = false)
				(Access a = (?? yes .Read .Write))
				(Access b = (?? no .Read .Write))
				(Int32 va = a.Value)
				(Int32 vb = b.Value)
				(result = ((va * 10) + vb))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 12);
	}

	void TestEnumTernaryQualified(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Bool yes = true)
				(Access a = (?? yes Access@Exec Sys.Type.Access@Read))
				(Access b = (?? (not yes) Access@Exec Sys.Type.Access@Read))
				(Int32 va = a.Value)
				(Int32 vb = b.Value)
				(result = ((va * 10) + vb))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 41);
	}

	void TestEnumTernaryVariables(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Bool yes = true)
				(Access x = .Exec)
				(Access y = .Write)
				(Access a = (?? yes x y))
				(Access b = (?? yes .Read y))
				(Access c = (?? (not yes) x Access@Read))
				(Int32 va = a.Value)
				(Int32 vb = b.Value)
				(Int32 vc = c.Value)
				(result = ((va * 100) + ((vb * 10) + vc)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 411);
	}

	void TestEnumTernaryContexts(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(struct Perm
				(Access access)
				(Int32 n)
			)

			(function ToInt (Access a) -> (Int32 value):
				(value = a.Value)
			)

			(function Main -> (Int32 result):
				(Bool yes = true)
				(Access a = .Read)
				(a = (?? yes .Exec a))
				(Int32 va = a.Value)
				(Int32 vb = (ToInt (?? yes .Write .Read)))
				(Perm p = (?? yes Access@Exec .Read) 3)
				(Int32 vp = p.access.Value)
				(if (a == (?? yes .Exec .Read)) (result += 1000))
				(result += ((va * 100) + ((vb * 10) + vp)))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 1424);
	}

	void TestEnumTernaryOr(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Bool yes = true)
				(Access a = (?? yes (.Read | .Write) .Exec))
				(Access b = (.Exec | (?? yes .Read .Write)))
				(Int32 va = a.Value)
				(Int32 vb = b.Value)
				(result = ((va * 10) + vb))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 35);
	}

	void TestEnumTernaryToInt(IPublicScriptSystem& ss)
	{
		// Qualified constants in a ternary with a plain integer target convert as in an assignment
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Bool yes = true)
				(Int32 x = (?? yes Access@Exec Access@Read))
				(Int64 y = (?? (not yes) 7 Access@Write))
				(Int64 two = 2)
				(if (y == two) (result = x))
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		validate(RunEnumQualifiedTest(ss, srcCode, __FUNCTION__) == 4);
	}

	void TestEnumTernaryOtherEnum(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(strong Colour (Int32))

			(alias Colour Sys.Type.Colour)

			(enum Sys.Type.Colour
				(Red 1)
				(Green 2)
			)

			(function Main -> (Int32 result):
				(Bool yes = true)
				(Access a = (?? yes .Read Colour@Red))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Type mismatch: Colour@Red is of type Colour, but the target type is Access");
	}

	void TestEnumTernaryLiteral(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Bool yes = true)
				(Access a = (?? yes .Read 2))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "2 is a literal, which does not convert to enum type Access");
	}

	void TestEnumTernaryIntVariable(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Bool yes = true)
				(Int32 i = 2)
				(Access a = (?? yes .Read i))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "i is of type Int32, but the enum expression is of type Access");
	}

	void TestEnumTernaryUnqualifiedToInt(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(strong Access (Int32))

			(alias Access Sys.Type.Access)

			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)

			(function Main -> (Int32 result):
				(Bool yes = true)
				(Int32 x = (?? yes .Read .Write))
				(result = 0)
			)
			(namespace EntryPoint)(alias Main EntryPoint.Main)
		)";

		RunEnumQualifiedFailTest(ss, srcCode, __FUNCTION__, "Cannot infer the enum type of .Read");
	}

	// Preamble shared by the array shorthand, append and fromstring tests: Day (Monday 1 .. Friday 5), and an unrelated Other
	static cstr ENUM_DAY_OTHER_PREAMBLE = R"(
			(strong Day (Int32))
			(alias Day Sys.Type.Day)
			(enum Sys.Type.Day
				(Monday 1)
				Tuesday
				Wednesday
				Thursday
				Friday
			)

			(strong Other (Int32))
			(alias Other Sys.Type.Other)
			(enum Sys.Type.Other
				(X 1)
			)

			(strong Access (Int32))
			(alias Access Sys.Type.Access)
			(enum Sys.Type.Access
				(Read 1)
				(Write 2)
				(Exec 4)
				(Sign -2147483648)
			)
		)";

	static int32 RunEnumPreambleTest(IPublicScriptSystem& ss, cstr body, cstr name)
	{
		char full[8192];
		SafeFormat(full, sizeof(full), "%s%s\n(namespace EntryPoint)(alias Main EntryPoint.Main)", ENUM_DAY_OTHER_PREAMBLE, body);
		return RunEnumQualifiedTest(ss, full, name);
	}

	static void RunEnumPreambleFailTest(IPublicScriptSystem& ss, cstr body, cstr name, cstr expectedFragment)
	{
		char full[8192];
		SafeFormat(full, sizeof(full), "%s%s\n(namespace EntryPoint)(alias Main EntryPoint.Main)", ENUM_DAY_OTHER_PREAMBLE, body);
		RunEnumQualifiedFailTest(ss, full, name, expectedFragment);
	}

	void TestEnumArrayShorthandDot(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(array Day days 4)
				(days.Push Day@Monday)
				(days.Push Day@Monday)
				(days 0 .Friday)
				(days 1 .Wednesday)
				(Day d0 = (days 0))
				(Day d1 = (days 1))
				(Int32 a = d0.Value)
				(Int32 b = d1.Value)
				(result = ((a * 10) + b))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 53);
	}

	void TestEnumArrayShorthandQualified(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(array Day days 4)
				(days.Push Day@Monday)
				(days.Push Day@Monday)
				(days 0 Day@Friday)
				(days 1 Sys.Type.Day@Tuesday)
				(Day d0 = (days 0))
				(Day d1 = (days 1))
				(Int32 a = d0.Value)
				(Int32 b = d1.Value)
				(result = ((a * 10) + b))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 52);
	}

	void TestEnumArrayShorthandWrongEnum(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(array Day days 4)
				(days.Push Day@Monday)
				(days 0 Other@X)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "Type mismatch: Other@X is of type");
	}

	void TestArrayShorthandSetInt32(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(array Int32 a 4)
				(a.Push 1)
				(a.Push 2)
				(a 1 (3 * 7))
				(a 0 4)
				(result = (((a 0) * 100) + (a 1)))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 421);
	}

	// (append <string-builder> <enum-variable>) appends the name of the enum value, or for a flagger type the space-delimited names of its flags. No match appends nothing
	void TestEnumAppendNamed(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Friday)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb day)
				(Int32 diff = (Sys.Type.Strings.Compare sb "Friday"))
				(if (diff == 0) (result = sb.Length) else (result = -1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 6);
	}

	void TestEnumAppendAllValues(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Matches (Day day) (IString expected) -> (Int32 n):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb day)
				(Int32 diff = (Sys.Type.Strings.Compare sb expected))
				(if (diff == 0) (n = 1) else (n = 0))
			)

			(function Main -> (Int32 result):
				(result = 0)
				(result += (Matches Day@Monday "Monday"))
				(result += (Matches Day@Tuesday "Tuesday"))
				(result += (Matches Day@Wednesday "Wednesday"))
				(result += (Matches Day@Thursday "Thursday"))
				(result += (Matches Day@Friday "Friday"))
				(result += (Matches Day@Friday "Monday"))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 5);
	}

	void TestEnumAppendNoName(IPublicScriptSystem& ss)
	{
		// Day is not a flagger, so a value with no name appends nothing
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day a = (Day@Tuesday | Day@Friday))
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(sb "abc")
				(Int32 before = sb.Length)
				(append sb a)
				(Int32 after = sb.Length)
				(Int32 diff = (Sys.Type.Strings.Compare sb "abc"))
				(result = ((10 * before) + after))
				(if (diff != 0) (result = -1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 33);
	}

	void TestEnumAppendSignBit(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Access a = .Sign)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb a)
				(Int32 diff = (Sys.Type.Strings.Compare sb "Sign"))
				(if (diff == 0) (result = sb.Length) else (result = -1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 4);
	}

	void TestEnumAppendInt64(IPublicScriptSystem& ss)
	{
		// Huge shares its low 32 bits with Zero, so a 32-bit compare would wrongly give "Zero", and One|Huge, which has no name, shares its low 32 bits with One
		// Three overlaps One, so Big is not a flagger and One|Huge appends nothing (a flagger would append "One Huge")
		cstr body = R"(
			(strong Big (Int64))
			(alias Big Sys.Type.Big)
			(enum Sys.Type.Big
				(Zero 0)
				(One 1)
				(Huge 4294967296)
				(Three 3)
			)

			(function Main -> (Int32 result):
				(Big b = .Huge)
				(IStringBuilder s = (Sys.Type.NewStringBuilder 64))
				(append s b)
				(Int32 diff = (Sys.Type.Strings.Compare s "Huge"))
				(if (diff == 0) (result = s.Length) else (result = -1))

				(Big z = .Zero)
				(IStringBuilder t = (Sys.Type.NewStringBuilder 64))
				(append t z)
				(Int32 diff2 = (Sys.Type.Strings.Compare t "Zero"))
				(if (diff2 == 0) (result = (result + (10 * t.Length))) else (result = -1))

				(Big none = (Big@One | Big@Huge))
				(IStringBuilder u = (Sys.Type.NewStringBuilder 64))
				(append u none)
				(result = (result + (100 * u.Length)))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 44);
	}

	void TestEnumAppendField(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(struct Meeting
				(Int32 id)
				(Day day)
			)

			(function Main -> (Int32 result):
				(Meeting m = 7 Day@Wednesday)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb m.day)
				(Int32 diff = (Sys.Type.Strings.Compare sb "Wednesday"))
				(if (diff == 0) (result = sb.Length) else (result = -1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 9);
	}

	void TestEnumAppendToExisting(IPublicScriptSystem& ss)
	{
		// append adds to what the builder holds, and mixes with the builder's own (sb <value>) appends
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(sb "Days: ")
				(append sb day)
				(sb " and ")
				(day = .Thursday)
				(append sb day)
				(Int32 diff = (Sys.Type.Strings.Compare sb "Days: Monday and Thursday"))
				(if (diff == 0) (result = sb.Length) else (result = -1))

				(Day a = (Day@Tuesday | Day@Friday))
				(append sb a)
				(result = (result + (100 * sb.Length)))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 2525);
	}

	void TestEnumAppendBuilderParameter(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function AppendDay (IStringBuilder sb) (Day d) -> :
				(append sb d)
			)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(AppendDay sb Day@Tuesday)
				(Int32 diff = (Sys.Type.Strings.Compare sb "Tuesday"))
				(if (diff == 0) (result = sb.Length) else (result = -1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 7);
	}

	void TestEnumAppendNullBuilderThrows(IPublicScriptSystem& ss)
	{
		// The null IStringBuilder is not a FastStringBuilder, so append throws a script exception rather than writing through it
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(IStringBuilder sb)
				(result = 1)
				(try
					(append sb day)
				catch e
					(result = 7)
				)
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 7);
	}

	void TestEnumAppendPlainStrongRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(strong Id (Int32))
			(function Main -> (Int32 result):
				(Id id = 3)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb id)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "append requires an enum type, but Id has no enum values");
	}

	void TestEnumAppendJoulesRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(strong Joules (Float64))
			(function Main -> (Int32 result):
				(Joules j = 1000)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb j)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "append requires an enum type, but Joules has no enum values");
	}

	void TestEnumAppendIntRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int32 i = 3)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb i)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "append requires a variable or field of an enum type, but i is of type Int32");
	}

	void TestEnumAppendFloatRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Float32 f = 3)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb f)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "append requires a variable or field of an enum type, but f is of type Float32");
	}

	void TestEnumAppendUnknownRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb nosuchday)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "append: unknown variable nosuchday");
	}

	void TestEnumAppendConstantRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb Day@Friday)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "append: unknown variable Day@Friday");
	}

	void TestEnumAppendIStringTargetRejected(IPublicScriptSystem& ss)
	{
		// An IString is not a builder
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Friday)
				(IString s = "abc")
				(append s day)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "append requires an IStringBuilder, but s is of type IString");
	}

	void TestEnumAppendIntTargetRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Friday)
				(Int32 x = 0)
				(append x day)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "append requires an IStringBuilder, but x is of type Int32");
	}

	void TestEnumAppendUnknownTargetRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Friday)
				(append nosb day)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "append: unknown string builder nosb");
	}

	void TestEnumAppendMissingOperandRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "Expecting (append <string-builder> <enum-variable>)");
	}

	void TestEnumAppendTooManyOperandsRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Friday)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb day day)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "Expecting (append <string-builder> <enum-variable>)");
	}

	void TestEnumAppendCompoundOperandRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Friday)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb (day))
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "Expecting (append <string-builder> <enum-variable>)");
	}

	void TestEnumToStringRemoved(IPublicScriptSystem& ss)
	{
		// tostring was replaced by append
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Friday)
				(IString s = tostring day)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "tostring may be the problem");
	}

	void TestEnumFromStringRoundTrip(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function RoundTrip (Day d) -> (Int32 n):
				(IStringBuilder s = (Sys.Type.NewStringBuilder 64))
				(append s d)
				(Bool ok = false)
				(Day e = fromstring s -> ok)
				(Int32 a = d.Value)
				(Int32 b = e.Value)
				(n = 0)
				(if ok (if (a == b) (n = 1)))
			)

			(function Main -> (Int32 result):
				(result = 0)
				(result += (RoundTrip Day@Monday))
				(result += (RoundTrip Day@Tuesday))
				(result += (RoundTrip Day@Wednesday))
				(result += (RoundTrip Day@Thursday))
				(result += (RoundTrip Day@Friday))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 5);
	}

	void TestEnumFromStringLiteral(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(Bool ok = false)
				(day = fromstring "Tuesday" -> ok)
				(Int32 v = day.Value)
				(result = (v * 10))
				(if ok (result += 1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 21);
	}

	void TestEnumFromStringVariable(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(IString s = "Thursday")
				(Bool ok = false)
				(Day day = fromstring s -> ok)
				(Int32 v = day.Value)
				(result = (v * 10))
				(if ok (result += 1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 41);
	}

	void TestEnumFromStringBuilder(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 32))
				(sb "Wednesday")
				(Bool ok = false)
				(Day day = .Monday)
				(day = fromstring sb -> ok)
				(Int32 v = day.Value)
				(result = (v * 10))
				(if ok (result += 1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 31);
	}

	void TestEnumFromStringField(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(struct Meeting
				(Int32 id)
				(Day day)
			)

			(function Main -> (Int32 result):
				(Meeting m = 7 Day@Monday)
				(IString s = "Friday")
				(Bool ok = false)
				(m.day = fromstring s -> ok)
				(Day d = m.day)
				(Int32 v = d.Value)
				(result = ((v * 10) + (m.id * 100)))
				(if ok (result += 1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 751);
	}

	void TestEnumFromStringNoMatchUnchanged(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Wednesday)
				(Bool ok = true)
				(day = fromstring "Funday" -> ok)
				(Int32 v = day.Value)
				(result = (v * 10))
				(if ok (result += 1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 30);
	}

	void TestEnumFromStringDeclarationNoMatch(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(IString s = "Funday")
				(Bool ok = true)
				(Day day = fromstring s -> ok)
				(Int32 v = day.Value)
				(result = (v * 10))
				(if ok (result += 1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 0);
	}

	void TestEnumFromStringCaseSensitive(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(Bool ok = true)
				(day = fromstring "tuesday" -> ok)
				(Int32 v = day.Value)
				(result = (v * 10))
				(if ok (result += 1))
				(IString s = "TUESDAY")
				(Bool ok2 = true)
				(day = fromstring s -> ok2)
				(if ok2 (result += 100))
				(IString t = "Tuesday ")
				(day = fromstring t -> ok2)
				(if ok2 (result += 1000))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 10);
	}

	void TestEnumFromStringEmptyAndNull(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Friday)
				(IString e = "")
				(IString n)
				(Bool ok1 = true)
				(Bool ok2 = true)
				(Bool ok3 = true)
				(day = fromstring e -> ok1)
				(day = fromstring n -> ok2)
				(day = fromstring "" -> ok3)
				(Int32 v = day.Value)
				(result = (v * 1000))
				(if ok1 (result += 100))
				(if ok2 (result += 10))
				(if ok3 (result += 1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 5000);
	}

	void TestEnumFromStringInt64(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(strong Big (Int64))
			(alias Big Sys.Type.Big)
			(enum Sys.Type.Big
				(Zero 0)
				(One 1)
				(Huge 4294967296)
				(Neg -4294967295)
			)

			(function Main -> (Int32 result):
				(result = 0)
				(Bool ok = false)
				(Big b = fromstring "Huge" -> ok)
				(Int64 v = b.Value)
				(if (ok and (v == 4294967296)) (result += 1))

				(IString s = "Neg")
				(b = fromstring s -> ok)
				(v = b.Value)
				(if (ok and (v == -4294967295)) (result += 10))

				(b = fromstring "Nope" -> ok)
				(v = b.Value)
				(if ((not ok) and (v == -4294967295)) (result += 100))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 111);
	}

	void TestEnumFromStringIntTargetRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int32 x = 0)
				(Bool ok = false)
				(x = fromstring "Monday" -> ok)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "fromstring requires an enum target, but x is of type Int32");
	}

	void TestEnumFromStringIntDeclarationRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Bool ok = false)
				(Int32 x = fromstring "Monday" -> ok)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "fromstring requires an enum target, but x is of type Int32");
	}

	void TestEnumFromStringJoulesRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(strong Joules (Float64))
			(function Main -> (Int32 result):
				(Bool ok = false)
				(Joules j = fromstring "Monday" -> ok)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "fromstring requires an enum type, but Joules has no enum values");
	}

	void TestEnumFromStringNonBoolRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(Int32 ok = 0)
				(day = fromstring "Monday" -> ok)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "fromstring: ok must be a Bool, but is of type Int32");
	}

	void TestEnumFromStringUnknownBoolRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(day = fromstring "Monday" -> nosuchflag)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "fromstring: unknown Bool variable nosuchflag");
	}

	void TestEnumFromStringNonStringRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(Bool ok = false)
				(Int32 i = 2)
				(day = fromstring i -> ok)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "fromstring requires an IString or a string literal, but i is of type Int32");
	}

	void TestEnumFromStringUnknownStringRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(Bool ok = false)
				(day = fromstring nosuchstring -> ok)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "fromstring: unknown string variable nosuchstring");
	}

	void TestEnumFromStringSubexpressionRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(Bool ok = false)
				(day = (fromstring "Monday" -> ok))
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "fromstring is only valid as a directive");
	}

	void TestEnumFromStringArgumentRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Show (Day d) -> (Int32 n):
				(n = d.Value)
			)

			(function Main -> (Int32 result):
				(Bool ok = false)
				(result = (Show (fromstring "Monday" -> ok)))
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "fromstring is only valid as a directive");
	}

	void TestEnumFromStringStandaloneRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Bool ok = false)
				(fromstring "Monday" -> ok)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "fromstring is only valid as a directive of the form (<enum-variable> = fromstring <string> -> <Bool-variable>)");
	}

	void TestEnumFromStringMissingArrowRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(Bool ok = false)
				(day = fromstring "Monday" ok)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "Expecting (<enum-variable> = fromstring <string> -> <Bool-variable>)");
	}

	void TestEnumFromStringMissingBoolRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(day = fromstring "Monday" ->)
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "Expecting (<enum-variable> = fromstring <string> -> <Bool-variable>)");
	}

	void TestEnumFromStringNoArrowNoBoolRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(day = fromstring "Monday")
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "Expecting (<enum-variable> = fromstring <string> -> <Bool-variable>)");
	}

	void TestEnumFromStringDeclarationNoBoolRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = fromstring "Monday")
				(result = 0)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "Expecting (<enum-type> <name> = fromstring <string> -> <Bool-variable>)");
	}

	// Flags: (flags <type> ...) is (enum <type> ...) with every value bitwise distinct from every other.
	// A type is a flagger (IStructure::IsFlagger) if its enum values are pairwise bitwise distinct, whether declared with enum or flags.
	// Alignment: None 0, Top 1, Left 2, Bottom 4, Right 8. Edge: North 1, South 2 (no zero name). Wide: Lo 3, Hi 12 (multi-bit flags). BigFlags (Int64): Low 1, High 2^32
	static cstr ENUM_FLAGS_PREAMBLE = R"(
			(strong Alignment (Int32))
			(alias Alignment Sys.Type.Alignment)
			(flags Sys.Type.Alignment
				(None 0)
				(Top 1)
				(Left 2)
				(Bottom 4)
				(Right 8)
			)

			(strong Edge (Int32))
			(alias Edge Sys.Type.Edge)
			(flags Sys.Type.Edge
				(North 1)
				(South 2)
			)

			(strong Wide (Int32))
			(alias Wide Sys.Type.Wide)
			(flags Sys.Type.Wide
				(Lo 3)
				(Hi 12)
			)

			(strong BigFlags (Int64))
			(alias BigFlags Sys.Type.BigFlags)
			(flags Sys.Type.BigFlags
				(Low 1)
				(High 4294967296)
			)
		)";

	static int32 RunEnumFlagsTest(IPublicScriptSystem& ss, cstr body, cstr name)
	{
		char full[16384];
		SafeFormat(full, sizeof(full), "%s%s%s\n(namespace EntryPoint)(alias Main EntryPoint.Main)", ENUM_DAY_OTHER_PREAMBLE, ENUM_FLAGS_PREAMBLE, body);
		return RunEnumQualifiedTest(ss, full, name);
	}

	static void RunEnumFlagsFailTest(IPublicScriptSystem& ss, cstr body, cstr name, cstr expectedFragment)
	{
		char full[16384];
		SafeFormat(full, sizeof(full), "%s%s%s\n(namespace EntryPoint)(alias Main EntryPoint.Main)", ENUM_DAY_OTHER_PREAMBLE, ENUM_FLAGS_PREAMBLE, body);
		RunEnumQualifiedFailTest(ss, full, name, expectedFragment);
	}

	static void ValidateMessageHas(cstr message, cstr expectedFragment)
	{
		if (strstr(message, expectedFragment) == nullptr)
		{
			printf("Expected '%s' in: %s\n", expectedFragment, message);
			validate(false);
		}
	}

	// Expects a bad enum or flags declaration to fail. The declaration is processed when the module compiles, so the error may surface from Compile or from Execute
	static void RunEnumDeclarationFailTest(IPublicScriptSystem& ss, cstr declarations, cstr name, cstr expectedFragment)
	{
		char full[8192];
		SafeFormat(full, sizeof(full), "%s\n(function Main -> (Int32 result): (result = 0))\n(namespace EntryPoint)(alias Main EntryPoint.Main)", declarations);

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(full, -1, Vec2i{ 0,0 }, name);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		bool compiled = false;
		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
			compiled = true;
			vm.Push(0);
			EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
			validate(result == EXECUTERESULT_THROWN);
			ParseException ex;
			validate(s_logger.TryGetNextException(ex));
			ValidateMessageHas(ex.Message(), expectedFragment);
			s_logger.Clear();
		}
		catch (ParseException& ex)
		{
			validate(!compiled);
			ValidateMessageHas(ex.Message(), expectedFragment);
			s_logger.Clear();
		}
	}

	void TestEnumFlagsDeclaration(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Alignment a = (Alignment@Top | Alignment@Right))
				(Alignment n = .None)
				(Int32 v = a.Value)
				(Int32 z = n.Value)
				(result = ((v * 10) + z))
				(if (a & .Right) (result += 1000))
				(if (a & .Left) (result += 100000))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 1090);
	}

	void TestEnumFlagsSequential(IPublicScriptSystem& ss)
	{
		// Same syntax as enum: unvalued names count up from the previous value. 0, 1, 2 are pairwise distinct
		cstr body = R"(
			(strong Tri (Int32))
			(alias Tri Sys.Type.Tri)
			(flags Sys.Type.Tri Zero One Two)

			(function Main -> (Int32 result):
				(Tri t = Tri@Two)
				(result = t.Value)
				(Bool f = isflagged Tri)
				(if f (result += 10))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 12);
	}

	void TestEnumFlagsMultiBit(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Wide w = (Wide@Lo | Wide@Hi))
				(result = w.Value)
				(Bool f = isflagged Wide)
				(if f (result += 100))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 115);
	}

	void TestEnumFlagsPlainEnumDuplicatesStillAllowed(IPublicScriptSystem& ss)
	{
		// enum still permits duplicate and overlapping values; such a type is not a flagger
		cstr body = R"(
			(strong Dup (Int32))
			(alias Dup Sys.Type.Dup)
			(enum Sys.Type.Dup (A 1) (B 1) (C 3))

			(function Main -> (Int32 result):
				(Dup d = Dup@B)
				(result = d.Value)
				(Bool f = isflagged Dup)
				(if f (result += 10))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 1);
	}

	void TestEnumIsFlaggedFlags(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Bool a = isflagged Alignment)
				(Bool e = isflagged Edge)
				(Bool b = isflagged BigFlags)
				(Bool q = isflagged Sys.Type.Alignment)
				(result = 0)
				(if a (result += 1))
				(if e (result += 10))
				(if b (result += 100))
				(if q (result += 1000))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 1111);
	}

	void TestEnumIsFlaggedEnum(IPublicScriptSystem& ss)
	{
		// Day is 1..5, so Wednesday (3) overlaps Monday and Tuesday: not a flagger. Access (1, 2, 4, sign bit) is declared with enum but is pairwise distinct: a flagger
		cstr body = R"(
			(function Main -> (Int32 result):
				(Bool d = true)
				(d = isflagged Day)
				(Bool a = false)
				(a = isflagged Access)
				(result = 0)
				(if d (result += 1))
				(if a (result += 10))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 10);
	}

	void TestEnumIsFlaggedField(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(struct Info
				(Int32 id)
				(Bool flagged)
			)

			(function Main -> (Int32 result):
				(Info info = 7 false)
				(info.flagged = isflagged Edge)
				(result = info.id)
				(Bool g = info.flagged)
				(if g (result += 10))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 17);
	}

	void TestEnumFlagsAppendList(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Matches (Alignment a) (IString expected) -> (Int32 n):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb a)
				(Int32 diff = (Sys.Type.Strings.Compare sb expected))
				(if (diff == 0) (n = 1) else (n = 0))
			)

			(function Main -> (Int32 result):
				(result = 0)
				(result += (Matches (Alignment@Top | Alignment@Left) "Top Left"))
				(result += (Matches (Alignment@Left | Alignment@Top) "Top Left"))
				(result += (Matches (Alignment@Right | Alignment@Top | Alignment@Bottom) "Top Bottom Right"))
				(result += (Matches ((Alignment@Top | Alignment@Left) | (Alignment@Bottom | Alignment@Right)) "Top Left Bottom Right"))
				(result += (Matches Alignment@Bottom "Bottom"))
				(result += (Matches Alignment@None "None"))
				(result += (Matches (Alignment@Top | Alignment@None) "Top"))
				(result += (Matches (Alignment@Top | Alignment@Left) "Left Top"))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 7);
	}

	void TestEnumFlagsAppendToExisting(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Alignment a = (Alignment@Bottom | Alignment@Left))
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(sb "[")
				(append sb a)
				(sb "] [")
				(append sb a)
				(sb "]")
				(Int32 diff = (Sys.Type.Strings.Compare sb "[Left Bottom] [Left Bottom]"))
				(if (diff == 0) (result = sb.Length) else (result = -1))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 27);
	}

	void TestEnumFlagsAppendZeroNoName(IPublicScriptSystem& ss)
	{
		// Edge has no zero name, so 0 appends nothing
		cstr body = R"(
			(function Main -> (Int32 result):
				(Edge e)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(sb "x")
				(append sb e)
				(result = sb.Length)
				(Edge both = (Edge@North | Edge@South))
				(IStringBuilder t = (Sys.Type.NewStringBuilder 64))
				(append t both)
				(Int32 diff = (Sys.Type.Strings.Compare t "North South"))
				(if (diff == 0) (result += 1000))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 1001);
	}

	void TestEnumFlagsAppendUncoveredBits(IPublicScriptSystem& ss)
	{
		// Bits that no flag covers append nothing, rather than a partial list
		cstr body = R"(
			(function Main -> (Int32 result):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(sb "ab")
				(Alignment a = 18)
				(append sb a)
				(Alignment b = 16)
				(append sb b)
				(Int32 diff = (Sys.Type.Strings.Compare sb "ab"))
				(result = sb.Length)
				(if (diff != 0) (result = -1))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 2);
	}

	void TestEnumFlagsAppendMultiBitPartial(IPublicScriptSystem& ss)
	{
		// Lo is 3: the value 13 sets Hi and only part of Lo, so it is not covered
		cstr body = R"(
			(function Main -> (Int32 result):
				(Wide w = (Wide@Hi | Wide@Lo))
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(append sb w)
				(Int32 diff = (Sys.Type.Strings.Compare sb "Lo Hi"))
				(if (diff == 0) (result = sb.Length) else (result = -1))
				(Wide p = 13)
				(append sb p)
				(result += (100 * sb.Length))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 505);
	}

	void TestEnumFlagsAppendInt64(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(BigFlags b = (BigFlags@High | BigFlags@Low))
				(IStringBuilder s = (Sys.Type.NewStringBuilder 64))
				(append s b)
				(Int32 diff = (Sys.Type.Strings.Compare s "Low High"))
				(if (diff == 0) (result = s.Length) else (result = -1))
				(BigFlags h = .High)
				(IStringBuilder t = (Sys.Type.NewStringBuilder 64))
				(append t h)
				(Int32 diff2 = (Sys.Type.Strings.Compare t "High"))
				(if (diff2 == 0) (result += 100) else (result = -1))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 108);
	}

	void TestEnumFlaggerEnumAppend(IPublicScriptSystem& ss)
	{
		// Access is declared with enum but is a flagger, so combinations append a list; Day is not a flagger, so a nameless Day appends nothing
		cstr body = R"(
			(function Main -> (Int32 result):
				(Access a = (Access@Write | Access@Read))
				(IStringBuilder s = (Sys.Type.NewStringBuilder 64))
				(append s a)
				(Int32 diff = (Sys.Type.Strings.Compare s "Read Write"))
				(if (diff == 0) (result = s.Length) else (result = -1))
				(Day d = (Day@Tuesday | Day@Friday))
				(IStringBuilder t = (Sys.Type.NewStringBuilder 64))
				(append t d)
				(result += (100 * t.Length))
				(Day w = (Day@Monday | Day@Tuesday))
				(append t w)
				(Int32 diff3 = (Sys.Type.Strings.Compare t "Wednesday"))
				(if (diff3 == 0) (result += 1000))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 1010);
	}

	void TestEnumFlagsAppendTruncates(IPublicScriptSystem& ss)
	{
		// Like the builder's other appends, text beyond the capacity of a fixed builder is truncated. 56 chars of padding leave 7 free in a token builder
		cstr body = R"(
			(function Main -> (Int32 result):
				(IStringBuilder sb = (Sys.Type.NewTokenBuilder))
				(sb "01234567890123456789012345678901234567890123456789012345")
				(Alignment a = ((Alignment@Top | Alignment@Left) | Alignment@Bottom))
				(append sb a)
				(Int32 diff = (Sys.Type.Strings.Compare sb "01234567890123456789012345678901234567890123456789012345Top Lef"))
				(if (diff == 0) (result = sb.Length) else (result = -1))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 63);
	}

	void TestEnumFlagsAppendExpands(IPublicScriptSystem& ss)
	{
		// Builders made by NewStringBuilder are expandable, so append grows them as their other appends do
		cstr body = R"(
			(function Main -> (Int32 result):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 8))
				(Alignment a = ((Alignment@Top | Alignment@Left) | Alignment@Bottom))
				(append sb a)
				(Int32 diff = (Sys.Type.Strings.Compare sb "Top Left Bottom"))
				(if (diff == 0) (result = sb.Length) else (result = -1))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 15);
	}

	void TestEnumFlagsFromStringLiteral(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Alignment a = .None)
				(Bool ok = false)
				(a = fromstring "Top Left" -> ok)
				(Int32 v = a.Value)
				(result = (v * 10))
				(if ok (result += 1))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 31);
	}

	void TestEnumFlagsFromStringDelimiters(IPublicScriptSystem& ss)
	{
		// Commas and/or whitespace (space, tab, newline) delimit tokens, leading and trailing delimiters included
		cstr body = R"(
			(function Parse (IString s) -> (Int32 n):
				(Bool ok = false)
				(Alignment a = .None)
				(a = fromstring s -> ok)
				(n = a.Value)
				(if (not ok) (n = -1))
			)

			(function Main -> (Int32 result):
				(Bool ok1 = false)
				(Bool ok2 = false)
				(Bool ok3 = false)
				(Alignment a = fromstring "Top,Left" -> ok1)
				(Alignment b = fromstring " , Bottom ,&tRight,, " -> ok2)
				(Alignment c = fromstring "Top,&nRight" -> ok3)
				(Int32 va = a.Value)
				(Int32 vb = b.Value)
				(Int32 vc = c.Value)
				(Int32 p = (Parse "Left, Bottom"))
				(result = (va * 10000))
				(result += (vb * 100))
				(result += vc)
				(result += (p * 1000000))
				(if (not ok1) (result = -1))
				(if (not ok2) (result = -1))
				(if (not ok3) (result = -1))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 6031209);
	}

	void TestEnumFlagsFromStringVariable(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(IString s = "Bottom, Right")
				(Bool ok = false)
				(Alignment a = fromstring s -> ok)
				(Int32 v = a.Value)
				(result = (v * 10))
				(if ok (result += 1))
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(sb "&tTop  Left,")
				(Bool ok2 = false)
				(Alignment b = fromstring sb -> ok2)
				(Int32 w = b.Value)
				(result += (w * 1000))
				(if ok2 (result += 100))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 3221);
	}

	void TestEnumFlagsFromStringBadTokenUnchanged(IPublicScriptSystem& ss)
	{
		// One bad token fails the lot: ok is false and the target keeps its value, for literals and variables
		cstr body = R"(
			(function Main -> (Int32 result):
				(Alignment a = .Bottom)
				(Bool ok1 = true)
				(a = fromstring "Top Lfet" -> ok1)
				(IString s = "Left Top Rihgt")
				(Bool ok2 = true)
				(a = fromstring s -> ok2)
				(Bool ok3 = true)
				(a = fromstring "Top left" -> ok3)
				(Bool ok4 = true)
				(IString t = "Top|Left")
				(a = fromstring t -> ok4)
				(Int32 v = a.Value)
				(result = (v * 10000))
				(if ok1 (result += 1))
				(if ok2 (result += 10))
				(if ok3 (result += 100))
				(if ok4 (result += 1000))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 40000);
	}

	void TestEnumFlagsFromStringEmpty(IPublicScriptSystem& ss)
	{
		// Empty, null, or only delimiters: no match
		cstr body = R"(
			(function Main -> (Int32 result):
				(Alignment a = .Right)
				(Bool ok1 = true)
				(Bool ok2 = true)
				(Bool ok3 = true)
				(Bool ok4 = true)
				(Bool ok5 = true)
				(IString e = "")
				(IString n)
				(IString d = " ,&t, ")
				(a = fromstring e -> ok1)
				(a = fromstring n -> ok2)
				(a = fromstring d -> ok3)
				(a = fromstring "" -> ok4)
				(a = fromstring " , , " -> ok5)
				(Int32 v = a.Value)
				(result = (v * 100000))
				(if ok1 (result += 1))
				(if ok2 (result += 10))
				(if ok3 (result += 100))
				(if ok4 (result += 1000))
				(if ok5 (result += 10000))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 800000);
	}

	void TestEnumFlagsFromStringRepeated(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Bool ok1 = false)
				(Bool ok2 = false)
				(Alignment a = fromstring "Top Top,Top" -> ok1)
				(IString s = "None Left None Left")
				(Alignment b = fromstring s -> ok2)
				(Int32 va = a.Value)
				(Int32 vb = b.Value)
				(result = ((va * 10) + vb))
				(if ok1 (result += 100))
				(if ok2 (result += 1000))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 1112);
	}

	void TestEnumFlagsFromStringZeroName(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Alignment a = .Right)
				(Bool ok = false)
				(a = fromstring "None" -> ok)
				(result = a.Value)
				(if ok (result += 10))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 10);
	}

	void TestEnumFlagsFromStringInt64(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(IString s = "High Low")
				(Bool ok = false)
				(BigFlags b = fromstring s -> ok)
				(BigFlags expected = (BigFlags@High | BigFlags@Low))
				(result = 0)
				(if (b == expected) (result = 1))
				(if ok (result += 10))
				(Bool ok2 = false)
				(BigFlags c = fromstring "High" -> ok2)
				(if (c == BigFlags@High) (result += 100))
				(if ok2 (result += 1000))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 1111);
	}

	void TestEnumFlagsFromStringLongList(IPublicScriptSystem& ss)
	{
		// The 256-char limit is per token: a 600-char list of short names still matches
		cstr body = R"(
			(function Main -> (Int32 result):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 1024))
				(Int32 i = 0)
				(while (i < 100)
					(sb "Top, ")
					(i += 1)
				)
				(sb "Right")
				(Int32 len = sb.Length)
				(Bool ok = false)
				(Alignment a = fromstring sb -> ok)
				(Int32 v = a.Value)
				(result = (v * 10000))
				(if ok (result += len))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 90505);
	}

	void TestEnumFlagsFromStringLongToken(IPublicScriptSystem& ss)
	{
		// A token of 256 or more chars never matches, and fails the whole string
		cstr body = R"(
			(function Main -> (Int32 result):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 1024))
				(sb "Top ")
				(Int32 i = 0)
				(while (i < 300)
					(sb "A")
					(i += 1)
				)
				(Alignment a = .Left)
				(Bool ok = true)
				(a = fromstring sb -> ok)
				(result = a.Value)
				(if ok (result += 10))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 2);
	}

	void TestEnumFlagsFromStringRoundTrip(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function RoundTrip (Alignment a) -> (Int32 n):
				(IStringBuilder s = (Sys.Type.NewStringBuilder 64))
				(append s a)
				(Bool ok = false)
				(Alignment b = fromstring s -> ok)
				(n = 0)
				(if ok (if (a == b) (n = 1)))
			)

			(function Main -> (Int32 result):
				(result = 0)
				(result += (RoundTrip Alignment@None))
				(result += (RoundTrip Alignment@Top))
				(result += (RoundTrip (Alignment@Top | Alignment@Left)))
				(result += (RoundTrip (Alignment@Bottom | Alignment@Right | Alignment@Left)))
				(result += (RoundTrip ((Alignment@Top | Alignment@Left) | (Alignment@Bottom | Alignment@Right))))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 5);
	}

	void TestEnumFromStringNonFlaggerListRejected(IPublicScriptSystem& ss)
	{
		// Day is not a flagger, so a list is not split: "Monday Tuesday" is one name that does not exist
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Friday)
				(Bool ok1 = true)
				(day = fromstring "Monday Tuesday" -> ok1)
				(IString s = "Monday,Tuesday")
				(Bool ok2 = true)
				(day = fromstring s -> ok2)
				(Bool ok3 = true)
				(IString t = " Monday")
				(day = fromstring t -> ok3)
				(Int32 v = day.Value)
				(result = (v * 1000))
				(if ok1 (result += 1))
				(if ok2 (result += 10))
				(if ok3 (result += 100))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 5000);
	}

	void TestEnumCaseCollisionEnumRejected(IPublicScriptSystem& ss)
	{
		cstr decls = R"(
			(strong Pos (Int32))
			(alias Pos Sys.Type.Pos)
			(enum Sys.Type.Pos (Top 1) (Left 2) (TOP 4))
		)";

		RunEnumDeclarationFailTest(ss, decls, __FUNCTION__, "TOP and Top differ only by case");
	}

	void TestEnumCaseCollisionFlagsRejected(IPublicScriptSystem& ss)
	{
		cstr decls = R"(
			(strong Pos (Int32))
			(alias Pos Sys.Type.Pos)
			(flags Sys.Type.Pos (Top 1) (LEFT 2) (Left 4))
		)";

		RunEnumDeclarationFailTest(ss, decls, __FUNCTION__, "Left and LEFT differ only by case");
	}

	void TestEnumCaseCollisionAcrossDeclarationsRejected(IPublicScriptSystem& ss)
	{
		cstr decls = R"(
			(strong Pos (Int32))
			(alias Pos Sys.Type.Pos)
			(enum Sys.Type.Pos (Top 1) (Left 2))
			(enum Sys.Type.Pos (TOp 1))
		)";

		RunEnumDeclarationFailTest(ss, decls, __FUNCTION__, "TOp and Top differ only by case");
	}

	void TestEnumCaseDistinctNamesAllowed(IPublicScriptSystem& ss)
	{
		// Names that differ by more than case are fine, and re-declaring the same name with the same value is still harmless
		cstr body = R"(
			(strong Pos (Int32))
			(alias Pos Sys.Type.Pos)
			(enum Sys.Type.Pos (Top 1) (Tops 2) (OTop 4))
			(enum Sys.Type.Pos (Top 1))

			(function Main -> (Int32 result):
				(Pos p = Pos@OTop)
				(result = p.Value)
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 4);
	}

	void TestEnumFlagsCombinationRejected(IPublicScriptSystem& ss)
	{
		cstr decls = R"(
			(strong Pos (Int32))
			(alias Pos Sys.Type.Pos)
			(flags Sys.Type.Pos (Top 1) (Left 2) (All 3))
		)";

		RunEnumDeclarationFailTest(ss, decls, __FUNCTION__, "flags Pos: All (3 / 0x3) shares bits with Top (1 / 0x1)");
	}

	void TestEnumFlagsDuplicateRejected(IPublicScriptSystem& ss)
	{
		cstr decls = R"(
			(strong Pos (Int32))
			(alias Pos Sys.Type.Pos)
			(flags Sys.Type.Pos (Top 1) (Left 2) (Up 1))
		)";

		RunEnumDeclarationFailTest(ss, decls, __FUNCTION__, "flags Pos: Up (1 / 0x1) has the same value as Top");
	}

	void TestEnumFlagsDuplicateZeroRejected(IPublicScriptSystem& ss)
	{
		cstr decls = R"(
			(strong Pos (Int32))
			(alias Pos Sys.Type.Pos)
			(flags Sys.Type.Pos (None 0) (Top 1) (Zero 0))
		)";

		RunEnumDeclarationFailTest(ss, decls, __FUNCTION__, "flags Pos: Zero (0 / 0x0) has the same value as None");
	}

	void TestEnumFlagsSequentialOverlapRejected(IPublicScriptSystem& ss)
	{
		// Unvalued names count up, so the third name gets 3, which overlaps 1 and 2
		cstr decls = R"(
			(strong Pos (Int32))
			(alias Pos Sys.Type.Pos)
			(flags Sys.Type.Pos (Top 1) Left Bottom)
		)";

		RunEnumDeclarationFailTest(ss, decls, __FUNCTION__, "flags Pos: Bottom (3 / 0x3) shares bits with Top (1 / 0x1)");
	}

	void TestEnumFlagsNegativeRejected(IPublicScriptSystem& ss)
	{
		cstr decls = R"(
			(strong Pos (Int32))
			(alias Pos Sys.Type.Pos)
			(flags Sys.Type.Pos (Top 1) (Everything -1))
		)";

		RunEnumDeclarationFailTest(ss, decls, __FUNCTION__, "flags Pos: Everything (-1 / 0xFFFFFFFFFFFFFFFF) shares bits with Top");
	}

	void TestEnumFlagsAfterEnumRejected(IPublicScriptSystem& ss)
	{
		// flags checks its values against values the type already has
		cstr decls = R"(
			(strong Pos (Int32))
			(alias Pos Sys.Type.Pos)
			(enum Sys.Type.Pos (Top 1) (Left 2))
			(flags Sys.Type.Pos (Corner 6))
		)";

		RunEnumDeclarationFailTest(ss, decls, __FUNCTION__, "flags Pos: Corner (6 / 0x6) shares bits with Left (2 / 0x2)");
	}

	void TestEnumFlagsTooFewElementsRejected(IPublicScriptSystem& ss)
	{
		cstr decls = R"(
			(strong Pos (Int32))
			(alias Pos Sys.Type.Pos)
			(flags Sys.Type.Pos)
		)";

		RunEnumDeclarationFailTest(ss, decls, __FUNCTION__, "Expecting at least three elements in a flags expression");
	}

	void TestEnumFlagsUnknownTypeRejected(IPublicScriptSystem& ss)
	{
		cstr decls = R"(
			(flags Sys.Type.Nowhere (Top 1))
		)";

		RunEnumDeclarationFailTest(ss, decls, __FUNCTION__, "(flags Sys.Type.Nowhere ...)");
	}

	void TestEnumIsFlaggedPlainStrongRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(strong Joules (Float64))
			(alias Joules Sys.Type.Joules)

			(function Main -> (Int32 result):
				(Bool f = isflagged Joules)
				(result = 0)
			)
		)";

		RunEnumFlagsFailTest(ss, body, __FUNCTION__, "isflagged requires an enum type, but Joules has no enum values");
	}

	void TestEnumIsFlaggedIntRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Bool f = false)
				(f = isflagged Int32)
				(result = 0)
			)
		)";

		RunEnumFlagsFailTest(ss, body, __FUNCTION__, "isflagged requires an enum type, but Int32 is not a strong type");
	}

	void TestEnumIsFlaggedUnknownTypeRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Bool f = isflagged Alignmnet)
				(result = 0)
			)
		)";

		RunEnumFlagsFailTest(ss, body, __FUNCTION__, "isflagged: unknown type Alignmnet");
	}

	void TestEnumIsFlaggedVariableOperandRejected(IPublicScriptSystem& ss)
	{
		// The operand is a type name, not a variable
		cstr body = R"(
			(function Main -> (Int32 result):
				(Alignment a = .Top)
				(Bool f = isflagged a)
				(result = 0)
			)
		)";

		RunEnumFlagsFailTest(ss, body, __FUNCTION__, "isflagged requires a type name");
	}

	void TestEnumIsFlaggedTargetRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int32 f = isflagged Alignment)
				(result = 0)
			)
		)";

		RunEnumFlagsFailTest(ss, body, __FUNCTION__, "isflagged produces a Bool, but f is of type Int32");
	}

	void TestEnumIsFlaggedAssignTargetRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Alignment f = .Top)
				(f = isflagged Alignment)
				(result = 0)
			)
		)";

		RunEnumFlagsFailTest(ss, body, __FUNCTION__, "isflagged produces a Bool, but f is of type Alignment");
	}

	void TestEnumIsFlaggedMissingOperandRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Bool f = isflagged)
				(result = 0)
			)
		)";

		RunEnumFlagsFailTest(ss, body, __FUNCTION__, "Expecting (Bool <name> = isflagged <enum-type>)");
	}

	void TestEnumIsFlaggedStandaloneRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(isflagged Alignment)
				(result = 0)
			)
		)";

		RunEnumFlagsFailTest(ss, body, __FUNCTION__, "isflagged is only valid as a directive");
	}

	static int64 TestNativeHandle_globalHandle = 0;

	struct HandleImage
	{
		int64 nativeValue;
	};

	void TestNativeHandle(IPublicScriptSystem& ss)
	{
		struct ANON
		{
			static void SetGlobalHandle(NativeCallEnvironment& e)
			{
				Rococo::uint8* _sf = e.cpu.SF();
				ptrdiff_t _offset = 2 * sizeof(size_t);

				HandleImage* handle;
				_offset += sizeof(handle);
				ReadInput(handle, _sf, -_offset);

				TestNativeHandle_globalHandle = handle->nativeValue;
			}

			static void GetHandle9001(NativeCallEnvironment& e)
			{
				Rococo::uint8* _sf = e.cpu.SF();
				ptrdiff_t _offset = 2 * sizeof(size_t);

				HandleImage* handle;
				_offset += sizeof(handle);
				ReadInput(handle, _sf, -_offset);

				handle->nativeValue = 9001LL;
			}
		};

		try
		{
			auto& nsAnimals = ss.AddNativeNamespace("Sys.Animals");
			ss.CreateHandleType(nsAnimals, __FILE__, __LINE__, "HMONKEY");

			ss.AddNativeCall(nsAnimals, ANON::GetHandle9001, NULL, "GetHandle9001 (Sys.Animals.HMONKEY hMonkey) ->", __FILE__, __LINE__, false, 0);
			ss.AddNativeCall(nsAnimals, ANON::SetGlobalHandle, NULL, "SetGlobalHandle (Sys.Animals.HMONKEY hMonkey) ->", __FILE__, __LINE__, false, 0);
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}

		cstr srcCode =
			"(function Main -> (Int32 result):"
			"		(Sys.Animals.HMONKEY hMonkey)"
			"		(Sys.Animals.GetHandle9001 hMonkey)"
			"		(Sys.Animals.SetGlobalHandle hMonkey)"
			")"
			"(namespace EntryPoint)(alias Main EntryPoint.Main)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestNativeHandle");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int64 x = vm.PopInt32();
		validate(0 == x);
		validate(TestNativeHandle_globalHandle == 9001LL);
	}

	void TestNativeHandleSecurity(IPublicScriptSystem& ss)
	{
		try
		{
			auto& nsAnimals = ss.AddNativeNamespace("Sys.Animals");
			ss.CreateHandleType(nsAnimals, __FILE__, __LINE__, "HMONKEY");
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}

		// This will fail because HMONKEY is a handle with no public members, so memberwise assignment fails
		cstr srcCode =
			"(function Main -> (Int32 result):\n"
			"		(MyMonkey monkey = 0 (614))\n"
			")\n"
			"(struct MyMonkey (Int64 a)(Sys.Animals.HMONKEY hMonkey))\n"
			"(namespace EntryPoint)(alias Main EntryPoint.Main)\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestNativeHandleSecurity");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);
		s_logger.Clear();
	}

	void TestNativeCall2(IPublicScriptSystem& ss)
	{
		struct ANON
		{
			static void Square(NativeCallEnvironment& e)
			{				
				int x;
				ReadInput(0, OUT x, e);
				int32 result = x * x;				
				WriteOutput(0, result,e);
			}
		};

		const INamespace& ns = ss.AddNativeNamespace("Sys.Maths");

		try
		{
			ss.AddNativeCall(ns, ANON::Square, NULL,"Square (Int32 x)-> (Int32 y)", __FILE__, __LINE__, false, 0);
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}	

		cstr srcCode =
			"(function Main -> (Int32 result):"
			"		(result = (Sys.Maths.Square 11))"
			")"				
			"(namespace EntryPoint)(alias Main EntryPoint.Main)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestNativeCall2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(115); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 121);
	}

	void TestNativeCall3(IPublicScriptSystem& ss)
	{
		struct ANON
		{
			static void George(NativeCallEnvironment& e)
			{	
				int x, y;
				ReadInput(0, OUT x, e);
				ReadInput(1, OUT y, e);

				int32 result1 = x * y;				
				int32 result2 = x - y;

				WriteOutput(0, result1, e);
				WriteOutput(1, result2, e);
			}
		};

		const INamespace& ns = ss.AddNativeNamespace("Sys.Maths");

		try
		{
			ss.AddNativeCall(ns, ANON::George, NULL,"George (Int32 x)(Int32 y)-> (Int32 pxy)(Int32 dxy)", __FILE__, __LINE__, false, 0);
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}	

		cstr srcCode =
			"(function Main -> (Int32 result1)(Int32 result2):"
			"		(Sys.Maths.George 11 9 -> result1 result2 )"
			")"				
			"(namespace EntryPoint)(alias Main EntryPoint.Main)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestNativeCall3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(115); // Allocate stack space for the int32 result1
		vm.Push(115); // Allocate stack space for the int32 result2
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 result1 = vm.PopInt32();
		int32 result2 = vm.PopInt32();
		validate(result1 == 2);
		validate(result2 == 99);
	}

	void TestInterfaceDefinition(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result1):"
			")"				
			"(alias Main EntryPoint.Main)"

			"(interface EntryPoint.IPlayer"				
			"    (GetId -> (Int32 value))"
			"    (SetId (Int32 value) ->)"
			")";
			
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestInterfaceDefinition");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		UNUSED(vm);
		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		const IObjectInterface* i = ns->FindInterface("IPlayer");
		validate(i != NULL);		
		validate(i->MethodCount() == 2);
		const IArchetype& a1 = i->GetMethod(0);
		const IArchetype& a2 = i->GetMethod(1);
		validate(AreEqual(a1.Name(),"GetId"));
		validate(AreEqual(a2.Name(),"SetId"));
		validate(a1.NumberOfInputs() == 1);
		validate(a1.NumberOfOutputs() == 1);
		validate(a2.NumberOfInputs() == 2);
		validate(a2.NumberOfOutputs() == 0);
		validate(AreEqual(a1.GetArgName(0),"value"));
		validate(AreEqual(a2.GetArgName(0),"value"));
		validate(a1.GetArgumentType(0).VarType() == SexyVarType_Int32);
		validate(a2.GetArgumentType(0).VarType() == SexyVarType_Int32);
	} 

	void TestClassDefinition(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result1):"
			")"				
			"(alias Main EntryPoint.Main)"

			"(interface EntryPoint.IPlayer"				
			"    (GetId -> (Int32 value))"
			"    (SetId (Int32 value) ->)"
			")"

			"(class Player (implements EntryPoint.IPlayer)"				
			"    (Int32 id)"
			")"
			
			"(method Player.GetId -> (Int32 value):"				
			"    (value = this.id)"
			")"
			
			"(method Player.SetId (Int32 value) -> :"				
			"    (this.id = value)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestClassDefinition");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());	
		UNUSED(vm);

		const IModule& m = ss.PublicProgramObject().GetModule(ss.GetIntrinsicModuleCount());
		const IStructure* s = m.FindStructure("Player");

		validate(s != NULL);
		validate(s->InterfaceCount() == 1);
		const IObjectInterface& i1 = s->GetInterface(0);
		validate(&i1 != NULL);

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		const IObjectInterface* i2 = ns->FindInterface("IPlayer");

		validate(i2 != NULL);
		validate(&i1 == i2);
	} 

	void TestDefaultParameter(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(function Add (Int32 a)(Int32 b (attribute default 7)) -> (Int32 sum):"
			"	(sum = a + b)"
			")"
			"(function Main -> (Int32 result):"
			"		(result = (Add 4 .))"
			")"
			"(namespace EntryPoint)(alias Main EntryPoint.Main)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(115); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 11);
	}

	void TestMissingMethod(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result1):"
			")"				
			"(alias Main EntryPoint.Main)"

			"(interface EntryPoint.IPlayer"				
			"    (GetId -> (Int32 value))"
			"    (SetId (Int32 value) ->)"
			")"

			"(class Player (implements EntryPoint.IPlayer)"				
			"    (Int32 id)"
			")"

			"(method Player.GetId -> (Int32 value):"				
			"    (value = this.id)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMissingMethod");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());	
			UNUSED(vm);
			validate(false);
		}
		catch (ParseException& e)
		{
			cstr msg = e.Message();
			validate(GetSubString(msg,"Expecting method") != NULL);
			validate(GetSubString(msg,"Player.SetId") != NULL);
		}
	} 

   void TestDuplicateVariable(IPublicScriptSystem& ss)
   {
      cstr srcCode =
         "(namespace EntryPoint)\n"
         "(function Main -> (Int32 exitCode):\n"
        "  (Int32 a = 1)"
        "  (Int32 a = 1)"
        ")\n"
         "(alias Main EntryPoint.Main)\n";

      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDuplicateVariable");
      Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

      try
      {
         VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
         vm.Push(0); // Allocate stack space for the int32 exitCode
         auto result = vm.Execute(VM::ExecutionFlags(false, true));
         validate(result == EXECUTERESULT_THROWN);
         s_logger.Clear();
         return;
      }
      catch (ParseException&)
      {
         validate(false);
      }
   }

	void TestClassInstance(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using EntryPoint)"
			"(function Main -> (Int32 result):"
			"    (IPlayer player (NewPlayer))"
			"    (player.SetId 1812)"
			"    (player.GetId -> result)"
			")"				
			"(alias Main EntryPoint.Main)"

			"(interface EntryPoint.IPlayer"			
			"    (GetId -> (Int32 value))"
			"    (SetId (Int32 value) ->)"
			")"

			"(class Player (implements IPlayer)"				
			"    (Int32 id)"
			")"

			"(method Player.GetId -> (Int32 value):"				
			"    (value = this.id)"
			")"

			"(method Player.SetId (Int32 value) -> :"				
			"    (this.id = value)"
			")"

			"(method Player.Construct -> :"
			"	(this.id = 0)"
			")"
			
			"(factory EntryPoint.NewPlayer IPlayer :"
			"   (construct Player)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestClassInstance");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(1815); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1812);
	} 

	void TestConstructor(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"
			"    (EntryPoint.IPlayer player (EntryPoint.NewPlayer 1812))"
			"    (player.GetId -> result)"
			")"				
			"(alias Main EntryPoint.Main)"

			"(interface EntryPoint.IPlayer"			
			"    (GetId -> (Int32 value))"
			"    (SetId (Int32 value) ->)"
			")"

			"(class Player (implements EntryPoint.IPlayer)"				
			"    (Int32 id)"
			")"

			"(method Player.GetId -> (Int32 value):"				
			"    (value = this.id)"
			")"

			"(method Player.SetId (Int32 value) -> :"				
			"    (this.id = value)"
			")"

			"(method Player.Construct (Int32 value): "			
			"    (this.id = value)"
			")"

			"(factory EntryPoint.NewPlayer EntryPoint.IPlayer (Int32 id) : (construct Player id))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestConstructor");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(1815); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1812);
	} 

	void TestNullObject(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"
			""
			"    (EntryPoint.IPlayer player)"
			"    (player.GetId -> result)"
			")"
			"(alias Main EntryPoint.Main)"

			"(interface EntryPoint.IPlayer"
			"    (GetId -> (Int32 value))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestNullObject");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		AutoFree<VM::IDisassembler> dis(vm.Core().CreateDisassembler());

		struct ANON : public VM::ITraceOutput
		{
			IPublicScriptSystem* ss;
			VM::IDisassembler* dis;
			VM::IVirtualMachine& vm;
			ANON(VM::IVirtualMachine& _vm) : vm(_vm) {}

			virtual void Report()
			{
				VM::IDisassembler::Rep rep;
				dis->Disassemble(vm.Cpu().PC(), rep);

				auto id = ss->PublicProgramObject().ProgramMemory().GetFunctionContaingAddress(vm.Cpu().PC() - vm.Cpu().ProgramStart);
				auto* f = GetFunctionFromBytecode(ss->PublicProgramObject(), id);

				if (f)
				{
					printf("[ %s ] %s: %s\n", f->Name(), rep.OpcodeText, rep.ArgText);
				}
				else
				{
					printf("[ ] %s: %s\n", rep.OpcodeText, rep.ArgText);
				}

				auto* D = vm.Cpu().D;
				printf("PC:%16.16llx SP:%16.16llx SF:%16.16llx D4:%16.16llx D5:%16.16llx D6:%16.16llx D7:%16.16llx D8:%16.16llx\n", D[0].uint64Value, D[1].uint64Value, D[2].uint64Value, D[4].uint64Value, D[5].uint64Value, D[6].uint64Value, D[7].uint64Value, D[8].uint64Value);
			}
		} tracer(vm);

		tracer.dis = dis;
		tracer.ss = &ss;

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestNullIString(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(function Main -> (Int32 result):"
			"    (Sys.Type.IString s)"
			"    (s.Length -> result)"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__, nullptr);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestDuplicateFunctionError(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):)"	
			"(function Main -> (Int32 result):)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDuplicateFunctionError");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());	
			UNUSED(vm);
			validate(false);
		}
		catch (ParseException& e)
		{
			validate(AreEqual(e.Message(),"Function Main already defined in TestDuplicateFunctionError"));
		}
	}

	void TestDuplicateStructureError(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):)"	
			"(struct Vector3 (Float32 x y z))"	
			"(struct Vector3 (Float32 x y z))"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDuplicateStructureError");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());	
			UNUSED(vm);
			validate(false);
		}
		catch (ParseException& e)
		{
			validate(StartsWith(e.Message(),"Duplicate structure definition for Vector3"));
		}
	}

	void TestBigNamespaceError(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(namespace T134567890123456789012345678901234567890123456789012345678901234TX3456789012345678901234567890123456789012345678901234567890123)" // 127 chars is ok
			"(namespace T234567890123456789012345678901234567890123456789012345678901234T234567890123456789012345678901234567890123456789012345678901234)" // 128 chars is not
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 1,1 },"TestBigNamespaceError");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());	
			UNUSED(vm);
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(),"T234", 4));
		}
	}

	void TestBigFunctionError(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function E2345678901234567890123456789012TX34567890123456789012345678901 -> (Int32 x):)" // 63 chars is ok
			"(function T2345678901234567890123456789012T2345678901234567890123456789012 -> (Int32 x):)" // 64 chars is not
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBigFunctionError");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			UNUSED(vm);
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(),"T234", 4));
		}
	}

	void TestBigStructureError(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(struct TX34567890123456789012345678901 (Int32 x))" // 31 chars is ok
			"(struct T2345678901234567890123456789012 (Int32 x))" // 32 chars is not
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBigStructureError");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			UNUSED(vm);
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(),"T234", 4));
		}
	}

	void TestBigInterfaceError(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(interface EntryPoint.TX34567890123456789012345678901 (GetId -> (Int32 id)))" // 31 chars is ok
			"(interface EntryPoint.T2345678901234567890123456789012 (GetId -> (Int32 id)))" // 32 chars is not
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBigInterfaceError");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			UNUSED(vm);
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(),"EntryPoint.T234", 9));
		}
	}

	void TestBigArchetypeError(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(archetype EntryPoint.TX34567890123456789012345678901 (Int32 x) -> (Int32 y))" // 31 chars is ok
			"(archetype EntryPoint.T2345678901234567890123456789012 (Int32 x) -> (Int32 y))" // 32 chars is not
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBigArchetypeError");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			UNUSED(vm);
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(),"EntryPoint.T234", 9));
		}
	}

	void TestDuplicateInterfaceError(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(interface EntryPoint.A (GetId -> (Int32 id)))" 
			"(interface EntryPoint.A (GetId -> (Int32 id)))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDuplicateInterfaceError");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			UNUSED(vm);
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(),"EntryPoint.A"));
		}
	}

	void TestDuplicateArchetypeError(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(archetype EntryPoint.A -> (Int32 id))" 
			"(archetype EntryPoint.A -> (Int32 id))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDuplicateArchetypeError");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());	
			UNUSED(vm);
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(),"EntryPoint.A"));
		}
	}

	void TestTryWithoutThrow(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"		
			"		("
			"			try ( result = 15)"   
			"   catch e ( result = (result + 7) )"
			"   )"
			")"			
			"(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))"
			"(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))"
			"(class TestException (implements Sys.Type.IException))"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestTryWithoutThrow");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 15);
	}

	void TestTryFinallyWithoutThrow(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"		
			"	(try (result = 15)"   
			"		catch e ( result = (result + 7) )"
			"		finally ( result = (result - 5) )"
			"   )"
			")"			
			"(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))"
			"(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))"
			"(class TestException (implements Sys.Type.IException))"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestTryFinallyWithoutThrow");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 10);
	}

	void TestCatch(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"		
			"   (result = 12)"
			"		(try"
			"			("
			"				(Sys.Type.IException ex (EntryPoint.NewException))"
			"				(throw ex) "
			"				(result = 11)"
			"			)"   
			"		catch e"
			"			("
			"				(result = (result + 15))"
			"			)"
			"		)"
			")"			
			"(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))"
			"(method TestException.Construct : )"
			"(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))"
			"(class TestException (implements Sys.Type.IException))"
			"(factory EntryPoint.NewException Sys.Type.IException : (construct TestException))"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestCatch");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 27);
	}

	void TestStringAndBuild(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
		(namespace TestApp)

		(using Sys.Type)

		(namespace EntryPoint)
		(function Main -> (Int32 result):
			(#string sb 256 "Hello ")
			(#build sb "World") 
		)

		(alias Main EntryPoint.Main)
)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestThrowInCatch(IPublicScriptSystem& ss)
	{
		cstr srcCode =
R"(
		(namespace TestApp)

		(using Sys.Type)

		(archetype TestApp.VoidFunction -> )

		(function RunTest(TestApp.VoidFunction f) -> :
			(IString functionName = f.Name)

			(
				try
				(
					(f)
				)
				catch e
				(
					(#string msg 256 functionName " threw an exception:" e.Message)
					(Sys.Throw e.ErrorCode msg)
				)
			)

			(#string msg 256 functionName " succeeded&n")
			(Sys.Print msg)
		)

		(function AssertMatchInt32(IString msg)(Int32 a)(Int32 b)-> :
			(if (a == b) (return))
			//(IStringBuilder errMsg = (Sys.Type.NewParagraphBuilder))
			//(#build errMsg a b) 
			(#string errMsg 256 msg)
			(errMsg.SetFormat 4 4 false false)
			(#build errMsg a b) // ": (" a " != " b ") ")
		//	(Sys.Throw 0 errMsg)
		)

		(function Test -> :
			(AssertMatchInt32 "Return result" 1 2)
		)

		(namespace EntryPoint)
		(function Main -> (Int32 result):
			(RunTest Test)
			(IStringBuilder sb = (Sys.Type.NewParagraphBuilder))
			(#build sb "World") 
		)

		(alias Main EntryPoint.Main)
)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestDeepCatch(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"		
			"   (result = 12)"
			"	("		
			"			try ( (Double 11 -> result) )"   
			"			catch e ( result = (result + 15) )"
			"   )"
			")"			
			"(function Double (Int32 x) -> (Int32 y):"		
			"   (Int32 z)"
			"   (Sys.Type.IException ex (EntryPoint.NewException))(throw ex)"			
			"   (y = (2 * x))"
			")"			
			"(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))"
			"(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))"
			"(method TestException.Construct : )"
			"(class TestException (implements Sys.Type.IException))"
			"(factory EntryPoint.NewException Sys.Type.IException : (construct TestException))"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDeepCatch");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 27);
	}

	int g_destructorTestInt;

	void TestDestructor(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main2 -> (Int32 result):"
			"   (Sys.IRobot robby (Sys.NewRobot))"
			"   (result = 7 + robby.Arms)"
			")"

			"(function Main -> (Int32 result):"
			"   (Main2 -> result)"
			")"

			"(alias Main EntryPoint.Main)"

			"(class Robot (defines Sys.IRobot) (Int32 nArms))"
			"(method Robot.Destruct -> : (Sys.InvokeTest))"
			"(method Robot.Null -> : )"
			"(method Robot.Arms -> (Int32 armCount): (armCount = this.nArms))"
			"(method Robot.Construct -> : )"
			"(factory Sys.NewRobot Sys.IRobot : (construct Robot))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDestructor");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		g_destructorTestInt = 0;

		struct ANON {	static void InvokeTest(NativeCallEnvironment& e)
		{				
			UNUSED(e);
			g_destructorTestInt = 1;
		}};

		const INamespace& nsSys = ss.AddNativeNamespace("Sys");

		try
		{
			ss.AddNativeCall(nsSys, ANON::InvokeTest, NULL,"InvokeTest ->", __FILE__, __LINE__, false, 0);
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}	

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(g_destructorTestInt == 1);
		int32 x = vm.PopInt32();
		validate(x == 7);
	}

	void TestExceptionDestruct(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"	
			"(Sys.Type.IException ex (EntryPoint.NewException))"
			"(try"
			"   ((EntryPoint.IRobot robby (EntryPoint.NewRobot))(throw ex))"
			"catch e ((result = 7)))"
			")"			
			"(alias Main EntryPoint.Main)"

			"(class Robot (defines EntryPoint.IRobot))"
			"(method Robot.Construct : )"
			"(method Robot.Id -> (Int32 id) : (id = 1984))"
			"(method Robot.Destruct -> : (Sys.InvokeTest))"
			"(factory EntryPoint.NewRobot EntryPoint.IRobot : (construct Robot))"
			"(class TestException (implements Sys.Type.IException))"
			"(method TestException.Construct : )"
			"(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))"			
			"(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))"
			"(factory EntryPoint.NewException Sys.Type.IException : (construct TestException))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestExceptionDestruct");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		g_destructorTestInt = 0;

		struct ANON {	static void InvokeTest(NativeCallEnvironment& e)
		{				
			UNUSED(e);
			g_destructorTestInt = 1;
		}};

		const INamespace& nsSys = ss.AddNativeNamespace("Sys");

		try
		{
			ss.AddNativeCall(nsSys, ANON::InvokeTest, NULL,"InvokeTest ->", __FILE__, __LINE__, false, 0);
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}	

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(g_destructorTestInt == 1);
		int32 x = vm.PopInt32();
		validate(x == 7);
	}

	void TestDestructorThrows(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"	
			"(Sys.Type.IException ex (EntryPoint.NewException))"
			"(try"
			"   ("
			"		(EntryPoint.IRobot robby (EntryPoint.NewRobot))"
			"		(throw ex)"
			"	)"
			"catch e ((result = 7)))"
			")"			
			"(alias Main EntryPoint.Main)"

			"(class Robot (defines EntryPoint.IRobot))"
			"(method Robot.Wait -> : )"
			"(method Robot.Destruct -> :"
			"  (Sys.Type.IException inner(EntryPoint.NewException))"
			"  (throw inner)"
			")"
			"(method Robot.Construct -> : )"
			"(class TestException (implements Sys.Type.IException))"
			"(method TestException.Construct : )"
			"(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))"			
			"(method TestException.Message -> (Sys.Type.IString s): (s = \"To be expected\"))"		
			"(factory EntryPoint.NewRobot EntryPoint.IRobot : (construct Robot))"
			"(factory EntryPoint.NewException Sys.Type.IException : (construct TestException))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDestructorThrows");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		s_logger.Write("Expecting a complaint about an exception thrown by a destructor:");
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(EXECUTERESULT_THROWN == result);
		ss.PublicProgramObject().FreeLeakedObjects(nullptr);
	}

	void TestThrowFromCatch(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"	
			"(Sys.Type.IException ex (EntryPoint.NewException))"
			"	(try"
			"		(try"
			"			("
			"				(result = 1)"
			"				(EntryPoint.IRobot robby (EntryPoint.NewRobot))"
			"				(throw ex)"
			"			)"
			"		 catch e1"
			"			("
			"				(result = (result + 3))"
			"				(Sys.Type.IException ex1 (EntryPoint.NewException))"
			"				(throw ex1)"
			"			)"			
			"		)"		
			"	catch e2"	
			"		((result = (result + 7)))"
			"	)"
			")"			
			"(alias Main EntryPoint.Main)"

			"(class Robot (defines EntryPoint.IRobot))"
			"(method Robot.Construct : )"
			"(method Robot.Id -> (Int32 id) : (id = 1984))"
			"(method Robot.Destruct -> : )"
			"(factory EntryPoint.NewRobot EntryPoint.IRobot : (construct Robot))"
			"(class TestException (implements Sys.Type.IException))"
			"(method TestException.Construct : )"
			"(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))"
			"(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))"
			"(factory EntryPoint.NewException Sys.Type.IException : (construct TestException))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestThrowFromCatch");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 11);
	}

   void TestCompareGetAccessorWithOne(IPublicScriptSystem& ss)
   {
      cstr srcCode =
         "(namespace EntryPoint)"
         "(class Job (defines Sys.IJob))"
		 "(method Job.Construct : )"
         "(method Job.Type -> (Int32 value): (value = 117))"
		 "(factory EntryPoint.NewJob Sys.IJob : (construct Job))"
         "(function Main -> (Int32 result):"
         "(Int32 vst2 = 117)"
        "     (Sys.IJob job (EntryPoint.NewJob))"
        "	  (if (job.Type == vst2)"
        "	      (result = 55)"
        "	   else"
        "	      (result = 89)"
        "	   )"
        ")"
         "(alias Main EntryPoint.Main)"
         ;

      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestCompareGetAccessorWithOne");
      Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

      VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

      vm.Push(0); // Allocate stack space for the int32 result

      EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
      ValidateExecution(result);

      int x = vm.PopInt32();
      validate(x == 55);
   }

   void TestCompareGetAccessorVsCompound(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint)"
		   "(class Job (defines Sys.IJob))"
		   "(method Job.Construct : )"
		   "(method Job.Type -> (Int32 value): (value = 117))"
		   "(factory EntryPoint.NewJob Sys.IJob : (construct Job))"
		   "(function Main -> (Int32 result):"
		   "(Int32 vst2 = 117)"
		   "     (Sys.IJob job (EntryPoint.NewJob))"
		   "	  (if (job.Type == (vst2))"
		   "	      (result = 55)"
		   "	   else"
		   "	      (result = 89)"
		   "	   )"
		   ")"
		   "(alias Main EntryPoint.Main)"
		   ;

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(0); // Allocate stack space for the int32 result

	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);

	   int x = vm.PopInt32();
	   validate(x == 55);
   }

   void TestCompareGetAccessorWithOne2(IPublicScriptSystem& ss)
   {
      cstr srcCode =
        "(namespace EntryPoint)"
		"(class Job (defines Sys.IJob))"
		"(method Job.Construct : )"
		"(method Job.Type -> (Int32 value): (value = 117))"
		"(factory EntryPoint.NewJob Sys.IJob : (construct Job))"
        "(function Main -> (Int32 result):"
			"(Int32 vst2 = 118)"
        "     (Sys.IJob job (EntryPoint.NewJob))"
        "	  (if (job.Type < vst2)"
        "	      (result = 55)"
        "	   else"
        "	      (result = 89)"
        "	   )"
        ")"
         "(alias Main EntryPoint.Main)"
         ;

      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestCompareGetAccessorWithOne2");
      Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

      VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

      vm.Push(0); // Allocate stack space for the int32 result

      EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
      ValidateExecution(result);

      int x = vm.PopInt32();
      validate(x == 55);
   }

	void TestSizeOf(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"	
			"	  (EntryPoint.IRobot robby (EntryPoint.NewRobot))"
			"	  (result = (sizeof robby))"
			")"			
			"(alias Main EntryPoint.Main)"
			"(class Robot (defines EntryPoint.IRobot)(Int32 id))"
			"(method Robot.Construct : )"
			"(method Robot.Id -> (Int32 id) : (id = 1984))"
			"(method Robot.Destruct -> : )"
			"(factory EntryPoint.NewRobot EntryPoint.IRobot : (construct Robot))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestSizeOf");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();

		int robbysize = sizeof(ObjectStub) + sizeof(int32); // Object Stub + this.id
		validate(x == robbysize); 
	}

	void TestCatchArg(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"		
			"   (result = 12)"
			"		("		
			"			try"
			"			("
			"				(Sys.Type.IException ex (EntryPoint.NewException 1943))"
			"				(throw ex)"
			"				(result = 11)"
			"			)"   
			"		    catch e ( (e.ErrorCode -> result) )"
			"   )"
			")"			
			"(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = this.errorCode))"
			"(method TestException.Construct (Int32 errorCode) : (this.errorCode = errorCode) )"
			"(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))"
			"(class TestException (implements Sys.Type.IException) (Int32 errorCode))"
			"(factory EntryPoint.NewException Sys.Type.IException (Int32 errorCode) : (construct TestException errorCode))"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestCatchArg");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1943);
	}

	void TestCatchInstanceArg(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"		
			" (Sys.Type.IException ex (EntryPoint.NewException 1943))"  
			" (ex.ErrorCode -> result)"
			")"			
			"(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = this.errorCode))"
			"(method TestException.Construct (Int32 errorCode): (this.errorCode = errorCode) )"
			"(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))"
			"(class TestException (implements Sys.Type.IException) (Int32 errorCode))"
			"(factory EntryPoint.NewException Sys.Type.IException (Int32 errorCode) : (construct TestException errorCode))"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestCatchInstanceArg");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1943);
	}

	void TestInstancePropagation(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"		
			" (Sys.IRobot robby (Sys.NewRobot 7))" 
			" (RobotThink robby)"
			" (result = robby.BrainState)"
			")"
			"(function RobotThink (Sys.IRobot x) -> :"		
			" (IncBrainState x)"   
			")"
			"(function IncBrainState (Sys.IRobot x) -> :"		
			" (x.SetBrainState ((x.BrainState) + 1))"   
			")"
			"(alias Main EntryPoint.Main)"
			"(class Robot (defines Sys.IRobot) (Int32 brainState))"
			"(method Robot.Construct (Int32 brainState): (this.brainState = brainState))"
			"(method Robot.SetBrainState (Int32 x) -> : (this.brainState = x))"
			"(method Robot.BrainState -> (Int32 x) : (x = this.brainState))"
			"(factory Sys.NewRobot Sys.IRobot (Int32 brainState): (construct Robot brainState))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestInstancePropagation");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 8);
	}

	void TestInstanceMemberPropagation(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"		
			" (Sys.IRobot robby (Sys.NewRobot 7))"
			" (RobotThink robby)"
			" (result = robby.BrainState)"
			")"
			"(function RobotThink (Sys.IRobot x) -> :"
			" (IncBrainState x)"
			")"
			"(function IncBrainState (Sys.IRobot x) -> :"		
			" (x.SetBrainState (Inc x.BrainState))"    
			")"
			"(function Inc (Int32 x) -> (Int32 y):"		
			" (y = (x + 1))"   
			")"
			"(alias Main EntryPoint.Main)"
			"(class Robot (defines Sys.IRobot) (Int32 brainState))"
			"(method Robot.Construct (Int32 brainState): (this.brainState = brainState))"
			"(method Robot.SetBrainState (Int32 x) -> : (this.brainState = x))"
			"(method Robot.BrainState -> (Int32 x) : (x = this.brainState))"
			"(factory Sys.NewRobot Sys.IRobot (Int32 brainState): (construct Robot brainState))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestInstanceMemberPropagation");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 8);
	}

	void TestInterfacePropagation(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"		
			"	(EntryPoint.IRobot robby(EntryPoint.MakeRobot 7))"
			" (RobotThink robby)"
			" (robby.GetBrainState -> result)"
			")"
			"(function RobotThink (EntryPoint.IRobot x) -> :"		
			" (IncRobotBrainState x)"   
			")"
			"(function IncRobotBrainState (EntryPoint.IRobot x) -> :"		
			" (x.SetBrainState ((x.GetBrainState) + 1))"   
			")"
			"(alias Main EntryPoint.Main)"
			"(interface EntryPoint.IRobot"
			"  (GetBrainState -> (Int32 value))"
			"  (SetBrainState (Int32 value) ->)"
			")"
			"(class Robot (implements EntryPoint.IRobot) (Int32 brainState))"
			"(method Robot.GetBrainState -> (Int32 value):"				
			"    (value = this.brainState)"
			")"
			"(method Robot.SetBrainState (Int32 value)-> :"				
			"    (this.brainState = value)"
			")"
			"(method Robot.Construct (Int32 value): "			
			"    (this.brainState = value)"
			")"
			"(factory EntryPoint.MakeRobot EntryPoint.IRobot (Int32 brainState): (construct Robot brainState))"	
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestInterfacePropagation");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 8);
	}

	void TestMultipleInterfaces(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)(namespace Test)"
			""
			"(function Main -> (Int32 result):"		
			" (Test.IRobot robby(Test.MakeRobot 7))"	
			" (robby.GetBrainState -> result)"
			")"
			"(alias Main EntryPoint.Main)"
			""
			"(interface Test.IRobot"
			"  (GetBrainState -> (Int32 value))"
			")"
			"(interface Test.IExistence"
			"  (Exists -> (Bool value))"
			")"
			"(class Robot (implements Test.IExistence) (implements Test.IRobot) (Int32 brainState))"
			"(method Robot.GetBrainState -> (Int32 value):"				
			"    (value = this.brainState)"
			")"
			"(method Robot.Exists -> (Bool value):"				
			"    (value = true)"
			")"
			"(method Robot.Construct (Int32 value): "		
			"    (this.brainState = value)"
			")"
			"(factory Test.MakeRobot Test.IRobot (Int32 brainState) : (construct Robot brainState))"	
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMultipleInterfaces");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 7);
	}

	void TestNullString(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			""
			"(using Sys.Type)"

			"(function Main -> (Int32 result):"			
			" (IString s)"	
			" (s.Length -> result)"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestStringConstant(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			""
			"(using Sys.Type)"
			"(function Main -> (Int32 result):"			
			" (IString s = \"hello world\")"	
			" (s.Length -> result)"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 11);
	}

	void TestPrint(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			""
			"(function Main -> (Int32 result):"			
			" (Sys.Print \"Hello World\" -> result)"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 11);
	}

	void TestAssignNotItself(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			""
			"(function Main -> (Int32 result):"
			"   (Bool isValid = true)"
			"   (isValid = (not isValid))"
			"   (if isValid (result = 7))"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		if (x != 0)
		{
			printf("\n%d\n", x);
		}
		validate(x == 0);
	}

	void TestPrintViaInstance(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"  (alias Main EntryPoint.Main)"
			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"  (IString message = \"Bilbo Baggins\")"
			"  (Sys.Print message -> result)"
			")"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 13);
	}

	void TestMemoString(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			""
			"(function Main -> (Int32 result):"			
			" (Sys.Type.IString s(Sys.Type.Memo \"Hello World - !\"))"
			" (Sys.Print s -> result)"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMemoString");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));
		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 15);
	}

	void TestMemoString2(IPublicScriptSystem& ss)
	{
		// Just like TestMemoString, but with a (using ...) directive
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			""
			"(using Sys.Type)"
			""
			"(function Main -> (Int32 result):"			
			" (IString s(Memo \"Hello World -- ? \"))"
			" (Sys.Print s -> result)"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMemoString2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 17);
	}

	void TestDynamicCast(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			""
			"(namespace Stuff) (using Stuff)"
			""
			"(function Main -> (Int32 result):"			
			"  (IExistence robby (NewRobot 9000))"
			"  (Foobar robby -> result)"
			")"
			"(interface Stuff.IRobot"
			"  (Id -> (Int32 id))"
			")"
			"(interface Stuff.IExistence"
			"  (ExistenceNumber -> (Int32 value))"
			")"
			"(class Robot"
			"  (implements Stuff.IExistence)"
			"  (implements Stuff.IRobot)"
			"  (Int32 id)"
			")"
			"(function Foobar (Stuff.IExistence entity) -> (Int32 result):"
			"	 (cast entity -> IRobot robot)"
			"  (result = ((robot.Id) + (entity.ExistenceNumber)))"
			")"
			"(method Robot.Construct (Int32 id): (this.id = id))"
			"(method Robot.Id -> (Int32 id): (id = this.id))"
			"(method Robot.ExistenceNumber -> (Int32 value): (value = 1234))"
			"(factory Stuff.NewRobot IExistence (Int32 id): (construct Robot id))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDynamicCast");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 10234);
	}

	void TestDynamicCast2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			""
			"(namespace Stuff) (using Stuff)"
			""
			"(function Main -> (Int32 result):"
			"  (IExistence robby (NewRobot 9000))"
			"  (Moomoo moomoo = 1 robby)"
			"  (Foobar moomoo -> result)"
			")"
			"(interface Stuff.IRobot"
			"  (Id -> (Int32 id))"
			")"
			"(interface Stuff.IExistence"
			"  (ExistenceNumber -> (Int32 value))"
			")"
			"(class Robot"
			"  (implements Stuff.IExistence)"
			"  (implements Stuff.IRobot)"
			"  (Int32 id)"
			")"

			"(struct Moomoo (Int32 id) (Stuff.IExistence entity))"
			"(function Foobar (Moomoo m) -> (Int32 result):"
			"	 (cast m.entity -> IRobot robot)"
			"  (result = ((robot.Id) + (m.entity.ExistenceNumber)))"
			")"
			"(method Robot.Construct (Int32 id): (this.id = id))"
			"(method Robot.Id -> (Int32 id): (id = this.id))"
			"(method Robot.ExistenceNumber -> (Int32 value): (value = 1234))"
			"(factory Stuff.NewRobot IExistence (Int32 id): (construct Robot id))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestDynamicCast2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 10234);
	}

	void TestStaticCast1(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"sexy(
			(namespace EntryPoint)
			   (alias Main EntryPoint.Main)
			
			(using Sys.Type)
			
			(function Main -> (Int32 result):
			    (IStringBuilder sb = NewPathBuilder)
			    (#build sb "Hello World")
			    (IString text = sb)
			    (Sys.Print text -> result)
			)

			)sexy";
			

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 11);
	}

	void TestInlinedFactory(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			""
			"(namespace Stuff) (using Stuff)"
			""
			"(function Main -> (Int32 result):"			
			"  (IRobot robby (Dreambot 9000))"
			"  (robby.Id -> result)"
			")"
			"(class Robot"
			"  (defines Stuff.IRobot)"
			"  (Int32 id)"
			")"
			"(method Robot.Construct (Int32 id): (this.id = id))"
			"(method Robot.Id -> (Int32 id): (id = this.id))"
			"(factory Stuff.Dreambot Stuff.IRobot (Int32 id): (construct Robot id))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestInlinedFactory");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 9000);
	}

	void TestForwardGotoSiblingLabel(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"sexy(
			(using Sys.Type)
			(using Sys.Maths)

			(namespace EntryPoint)
			 (alias Main EntryPoint.Main)

			(function Main -> (Int32 result):
			  (Int32 id = 72)
			  (goto skip42)
			  (id = 42)
              (label skip42)
			  (result = id)
			)
			)sexy";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_TERMINATED);

		int32 x = vm.PopInt32();
		validate(x == 72);

		s_logger.Clear();
	}

	void TestForwardGotoSiblingLabelWithFailure(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"sexy(
			(using Sys.Type)
			(using Sys.Maths)

			(namespace EntryPoint)
			 (alias Main EntryPoint.Main)

			(function Main -> (Int32 result):
			  (Int32 id = 72)
			  (goto skip42)
			  (id = 42)
              (
                    (label skip42)
              )
			  (result = id)
			)
			)sexy";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);

		s_logger.Clear();
	}

	void TestBackwardGotoSiblingLabel(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"sexy(
			(using Sys.Type)
			(using Sys.Maths)

			(namespace EntryPoint)
			 (alias Main EntryPoint.Main)

			(function Main -> (Int32 result):
              (label testResult)
			  (if (result > 0)
                (return)
			  )
			  (result = 72)
			  (goto testResult)
			  (result = 0)
			)
			)sexy";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_TERMINATED);

		ValidateLogs();

		int32 x = vm.PopInt32();
		validate(x == 72);

		s_logger.Clear();
	}

	void TestBackwardGotoSiblingLabelWithCleanup(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"sexy(
			(using Sys.Type)
			(using Sys.Maths)

			(namespace EntryPoint)
			 (alias Main EntryPoint.Main)

			(function Main -> (Int32 result):
              (label testResult)
			  (if (result > 0)
                (return)
			  )
			  (result = 72)
			  (Int32 a)
              (Int32 b)
              (Int32 c)
			  (goto testResult)
			  (result = 0)
			)
			)sexy";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_TERMINATED);

		ValidateLogs();

		int32 x = vm.PopInt32();
		validate(x == 72);

		s_logger.Clear();
	}

	void TestBackwardGotoParentLabel(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"sexy(
			(using Sys.Type)
			(using Sys.Maths)

			(namespace EntryPoint)
			 (alias Main EntryPoint.Main)

			(function Main -> (Int32 result):
              (label testResult)
              (if (result > 0)
                (return)
              )
			  (result = 5)
			  (if (result > 0)
                (goto testResult)
              )
			)
			)sexy";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_TERMINATED);

		ValidateLogs();

		int32 x = vm.PopInt32();
		validate(x == 5);

		s_logger.Clear();
	}

	void TestBackwardGotoParentLabelWithCleanup(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"sexy(
			(using Sys.Type)
			(using Sys.Maths)

			(namespace EntryPoint)
			 (alias Main EntryPoint.Main)

			(function Main -> (Int32 result):
              (Int32 x)
              (label testResult)
              (Int32 y)
              (if (result > 0)
                (Int32 z)
                (return)
              )
			  (Int32 w)
			  (result = 5)
			  (if (result > 0)
                (Int32 p)
                (goto testResult)
              )

			  (Int32 q)
			)
			)sexy";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_TERMINATED);

		ValidateLogs();

		int32 x = vm.PopInt32();
		validate(x == 5);

		s_logger.Clear();
	}

	void TestBackwardGotoParentLabelWithFailure(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"sexy(
			(using Sys.Type)
			(using Sys.Maths)

			(namespace EntryPoint)
			 (alias Main EntryPoint.Main)

			(function Main -> (Int32 result):
              (if (result > 0)
                (label testResult) // the label is neither a sibling nor an ancestor of the goto statement below, so is prohibited
                (return)
              )
			  (result = 5)
			  (if (result > 0)
                (goto testResult)
              )
			)
			)sexy";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);

		s_logger.Clear();
	}

	void TestForwardGotoSiblingLabelFailure(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"sexy(
			(using Sys.Type)
			(using Sys.Maths)

			(namespace EntryPoint)
			 (alias Main EntryPoint.Main)

			(function Main -> (Int32 result):
			  (Int32 id = 72)
			  (goto skip42)
			  (id = 42)
			  (Int32 dummy) // This should crash the compilation as it allocates between the goto and the succeeding label
              (label skip42)
			  (result = id)
			)
			)sexy";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);

		s_logger.Clear();
	}

	void TestStrongNumber(IPublicScriptSystem& ss)
	{
		// This fails by design because an id will not automatically cast to a number
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(strong CustomerId (Int32))"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"
			"  (CustomerId id = 72)"
			"  (result = id)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);
		s_logger.Clear();
	}

	void TestStrongNumber2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(strong CustomerId (Int32))"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"
			"  (CustomerId id = 72)"
			"  (result = id.Value)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result);

		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 72);
	}

	void TestStrongNumber3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(strong CustomerId (Int32))"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function GetValue (CustomerId id)-> (Int32 value):"
			"	(value = id.Value)"
			")"

			"(function Main -> (Int32 result):"
			"  (CustomerId id = 72)"
			"  (result = (GetValue id))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result);

		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 72);
	}

	void TestStrongNumber4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(strong CustomerId (Int32))"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function GetValue (CustomerId id)-> (Int32 value):"
			"	(value = id.Value)"
			")"

			"(function Main -> (Int32 result):"
			"  (Int32 id = 72)"
			"  (result = (GetValue id))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result);

		validate(result == EXECUTERESULT_THROWN);
		s_logger.Clear();
	}

	void TestStrongNumber5(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(strong CustomerId (Int32))"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function GetValue -> (CustomerId id):"
			"	(id.Value = 72)"
			")"

			"(function Main -> (Int32 result):"
			"  (CustomerId id)"
			"  (id = GetValue)"
			"  (result = id.Value)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 72);	
	}

	void TestStrongNumber6(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(strong CustomerId (Int32))"
			"(strong BankId (Int32))"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function GetValue -> (CustomerId id):"
			"	(id.Value = 72)"
			")"

			"(function Main -> (Int32 result):"
			"  (BankId id)"
			"  (id = GetValue)"
			"  (result = id.Value)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		validate(result == EXECUTERESULT_THROWN);
		s_logger.Clear();
	}

	void TestStrongNumber7(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(strong CustomerId (Int32))"
			"(alias CustomerId EntryPoint.CustomerId)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"
			"  (EntryPoint.CustomerId id = 72)"
			"  (result = id.Value)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result);

		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 72);
	}

	void TestRecti1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (Recti rect = 1 2 3 4)"
			"  (result = rect.left)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestRecti1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestRecti2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (Recti rect = 1 2 3 4)"
			"  (result = rect.top)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestRecti2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 2);
	}

	void TestRecti3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (Recti rect = 1 2 3 4)"
			"  (result = rect.right)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestRecti3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 3);
	}

	void TestRecti4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (Recti rect = 1 2 3 4)"
			"  (result = rect.bottom)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestRecti4");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestWindow(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.OS.Win32)"
			"(using Sys.OS.Win32.Windows)"
			"(using Sys.OS.Thread)"
			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (Recti rect = 40 520 680 40))"
			"  (IWin32Window window (CreateWindow \"Sexy Test Window\" rect))"
			"  (window.Show)"

			"  (IThread mainThread (GetCurrentThread))"
			"  (Int32 limit = 1)"
			"  (while ((not mainThread.IsQuitting) and (limit > 0))"
			"    (mainThread.Wait 1000)"
			"    (limit = (limit - 1))"
			"  )"

			"  (result = 320)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestWindow");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 320);
	}

	void TestReflectionGetCurrentExpression(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Reflection)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (IExpression s = GetCurrentExpression)"
			"  (s.ChildCount -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestReflectionGetCurrentExpression");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestReflectionGetParent(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Reflection)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (IExpression s = GetCurrentExpression)"
			"  (IExpression parent = s.Parent)"
			"  (parent.ChildCount -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestReflectionGetParent");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 8);
	}

	void TestReflectionGetChild_BadIndex(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Reflection)"

			"(namespace EntryPoint)"
			" (alias ProtectedMain EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (IExpression s = GetCurrentExpression)"
			"  (IExpression child = (s.Child -1))"
			"  (child.ChildCount -> result)"
			")"

			"(function ProtectedMain -> (Int32 result):"
			"	(try"
			"		(Main -> result)"
			"	catch ex"
			"		("
			"			(Int32 len)"
			"			(Sys.Print ex.Message -> len)"
			"			(result = 1001)"
			"		)"
			"	)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestReflectionGetChild_BadIndex");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1001);
	}

	void TestReflectionGetChild(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Reflection)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (IExpression s = GetCurrentExpression)" // s = (IExpression s = GetCurrentExpression)
			"  (IExpression child = (s.Child 0))"  // child 0 is 'IExpression' which is atomic and therefore has no children
			"  (child.ChildCount -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestReflectionGetChild");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestReflectionGetAtomic(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"
			"(using Sys.Reflection)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (IExpression s = GetCurrentExpression)" // s = (IExpression s = GetCurrentExpression)
			"  (IExpression child = (s.Child 0))"  // child 0 is 'IExpression' which is atomic and therefore has no children
			"  (IString name = child.Text)"
			"  (Sys.Print name -> result)" // returns the strlen of 'IExpression'
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestReflectionGetAtomic");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 11);
	}

	void TestNullMember(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (result = (sizeof Lapdancer))"
			")"

			"(interface EntryPoint.IPole"
			"  (Index -> (Int32 value))"
			")"

			"(struct Lapdancer"
			"  (EntryPoint.IPole pole)" // This creates two members, the concrete IPole, initially a null-object and a reference to the interface
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestNullMember");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == sizeof(size_t)); // lapdancer's only non-pseudo member is a pointer
	}

	void TestNullMemberInit(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)"

			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"			
			"  (Lapdancer lapdancer)"
			"  (lapdancer.pole.Index -> result)"
			")"

			"(interface EntryPoint.IPole"
			"  (Index -> (Int32 value))"
			")"

			"(class Pole"
			"  (implements EntryPoint.IPole)"
			"  (Int32 index)"
			")"

			"(method Pole.Index -> (Int32 index): (index = this.index))"

			"(struct Lapdancer"
			"  (EntryPoint.IPole pole)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestNullMemberInit");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns,"Main"));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestSysThrow(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"  (alias Main EntryPoint.Main)"
			"(function Main -> (Int32 result):"
			"(try"
			"		("
			"			(Sys.Throw -1 \"Wobbly\")"
			"		)"		
			" catch e"		
			"		("
			"			(e.ErrorCode -> result)"
			"         (Int32 nChars)"
			"			(Sys.Print e.Message -> nChars)"
			"		)"		
			"  )"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestSysThrow");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == -1);
	}

	void TestGetArchetypeName(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"(
(archetype Sys.VoidFunction -> )
(function BitBang -> : )

(namespace EntryPoint)
	(alias Main EntryPoint.Main)

(function Main -> (Int32 result):
	(Sys.VoidFunction f = BitBang)
	(Sys.Print f.Name)
)
)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestSysThrow2(IPublicScriptSystem& ss) // like sys.throw, but adds a stack spanner in the works
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"  (alias Main EntryPoint.Main)"
			"(function Main -> (Int32 result):"
			"(try"
			"		("
			"			(Int32 stackSpanner)"
			"			(Sys.Throw -1 \"Wobbly\")"
			"		)"		
			" catch e"	
			"		("
			"			(e.ErrorCode -> result)"
			"         (Int32 nChars)"
			"			(Sys.Print e.Message -> nChars)"
			"		)"		
			"  )"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestSysThrow");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == -1);
	}

	void TestVirtualFromVirtual(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"  (alias Main EntryPoint.Main)"
			"(function Main -> (Int32 result):"
			"(EntryPoint.ITest tester (Sys.NewTest))"
			"   (tester.Think)"
			"   (tester.Id -> result)"
			")"
			"(interface EntryPoint.ITest"
			"     (Id -> (Int32 id))"
			"     (Think ->)"
			"     (IncId ->)"
			")"
			"(class Tester"
			"     (implements EntryPoint.ITest)"
			"     (Int32 id)"
			")"
			"(method Tester.Id -> (Int32 id): (id = this.id))"
			"(method Tester.IncId -> : (this.id = (this.id + 1)))"
			"(method Tester.Think -> : (this.IncId))"
			"(method Tester.Construct : (this.id = 6))"
			"(factory Sys.NewTest EntryPoint.ITest : (construct Tester))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestVirtualFromVirtual");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 7);
	}

	void TestNullRefInit(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"  (alias Main EntryPoint.Main)"
			"(function Main -> (Int32 result):"
			"	(EntryPoint.ITest tester = GetNullRef)"
			"   (tester.Id -> result)"
			")"
			"(interface EntryPoint.ITest"
			"     (Id -> (Int32 id))"
			")"
			"(class Tester"
			"     (implements EntryPoint.ITest)"
			"     (Int32 id)"
			")"
			"(method Tester.Id -> (Int32 id): (id = this.id))"
			"(method Tester.Construct : (this.id = 6))"
			"(function GetNullRef -> (EntryPoint.ITest testRef): )"			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestNullRefInit");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestModuleCount(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"  (alias Main EntryPoint.Main)"
			"(using Sys.Reflection)"

			"(function Main -> (Int32 result):"
			"	(IScriptSystem ss = Sys.Reflection.GetScriptSystem)"
			"   (ss.ModuleCount -> result)"
			")"			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestModuleCount");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == ss.PublicProgramObject().ModuleCount());
	}

	void TestPrintModules(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"  (alias Main EntryPoint.Main)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(IScriptSystem ss = Sys.Reflection.GetScriptSystem)"
			"	(Int32 moduleCount)"
			"   (ss.ModuleCount -> moduleCount)"
			"   (for (Int32 i = 0) (i < moduleCount) (i = (i + 1))"
			"		(IModule module = (ss.Module i))"
			"		(Int32 len)"
			"		(Sys.Print module.Name -> len)"
			"	)"
			")"			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestPrintModules");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestTypeInference1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"  (alias Main EntryPoint.Main)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"

			"(using Sys.Maths)"

			"(struct Mat2x2 (Vec2i row1 row2))"

			"(function Main -> (Int32 result):"
			"	(Mat2x2 m = (4 3)(2 1))"
			"   (Float32 x = (5 - (m.row1.x - m.row2.x)))"
			"   (if (x == 3) (result = 12))"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT r = vm.Execute(VM::ExecutionFlags(false, true));
		
		validate(r == EXECUTERESULT_THROWN);

		s_logger.Clear();
	}

	void TestPrintStructs(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"  (alias Main EntryPoint.Main)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(IScriptSystem ss = Sys.Reflection.GetScriptSystem)"
			"	(Int32 moduleCount = ss.ModuleCount)"

			"   (for (Int32 i = 0)  (i < moduleCount)  (i += 1)"
			"			(IModule module = (ss.Module i))"

			"			(for (Int32 j = 0) (j < module.StructCount) (j += 1)"
			"				(IStructure s = (module.Structure j))"
			"				(Int32 len)"
			"				(Sys.Print s.Name -> len)"
			"			)"
			"	)"
			")"			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestPrintStructs");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestMacro(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Maths)"
			"(using Sys.Reflection)"

			// Define a macro called 'square' instanced as #square that takes an input IExpression of 'in' and builds an expression 'out', which substitutes for the instance #square

			// square usage: (#square f) evaluates to ((f) * (f))
			"(macro Sys.Maths.square in out"
			"	(Sys.ValidateSubscriptRange in.ChildCount 2 3 \"macro square supports one argument only\")"
			"	(IExpressionBuilder lhs = out.AddCompound)"
			"	(lhs.Copy (in.Child 1))"
			"	(out.AddAtomic \"*\")"
			"	(IExpressionBuilder rhs = out.AddCompound)"
			"	(rhs.Copy (in.Child 1))"
			")";

		cstr srcCode2 =
		"(using Sys.Maths)"
		"(namespace EntryPoint)"
		"   (alias Main EntryPoint.Main)"
		"(function Main -> (Int32 result):"
		"	(Int32 i = 7)"
		"	(result = (#square i))"
		")";

		Auto<ISourceCode> sc1 = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestMacro_Macro");
		Auto<ISParserTree> tree1(ss.SParser().CreateTree(sc1()));

		Auto<ISourceCode> sc2 = ss.SParser().ProxySourceBuffer(srcCode2, -1, Vec2i{ 0,0 }, "TestMacro_Main");
		Auto<ISParserTree> tree2(ss.SParser().CreateTree(sc2()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree1(), tree2());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 49);
	}

	void TestMacroSiblings(IPublicScriptSystem& ss)
	{
		cstr macroCode =
			"(using Sys.Reflection)"

			// do_twice usage: (#do_twice (s)) evaluates to (s) (s))
			"(macro Sys.Maths.doTwice in out"
			"	(Sys.ValidateSubscriptRange in.ChildCount 2 2 \"macro 'doTwice' supports one argument only\")"
			"	(out.Copy (in.Child 1))"
			"   (IExpressionBuilder parent = out.TransformParent)"
			"   (Int32 outIndex = (parent.IndexOf in))"
			"	(IExpressionBuilder secondInstance = (parent.InsertCompoundAfter outIndex))"
			"	(secondInstance.Copy (in.Child 1))"
			")";
		cstr mainCode =
			"(using Sys.Maths)"
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			"(function Main -> (Int32 result):"
			"	(result = 7)"
			"	(#doTwice (result += 1))"
			")"
			;

		Auto<ISourceCode> sc1 = ss.SParser().ProxySourceBuffer(macroCode, -1, Vec2i{ 0,0 }, "TestMacroSiblings_Macro");
		Auto<ISParserTree> tree1(ss.SParser().CreateTree(sc1()));

		Auto<ISourceCode> sc2 = ss.SParser().ProxySourceBuffer(mainCode, -1, Vec2i{ 0,0 }, "TestMacroSiblings_Main");
		Auto<ISParserTree> tree2(ss.SParser().CreateTree(sc2()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree1(), tree2());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 9);
	}

	void TestIndexOf(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			" (alias Main EntryPoint.Main)\n"

			"(using Sys.Type)\n"
			"(using Sys.Reflection)\n"

			"(function Main -> (Int32 result):\n"
			"	(IExpression s = '(cheese oranges (#paragraph 1024 apples pears) onion bikes))\n"
			"	(IExpression sOranges = (s 1))\n"
			"   (Int32 index = (s.IndexOf sOranges))\n"
			"   (result = index)\n"
			")\n"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 1);
	}

	void TestTransformParent(IPublicScriptSystem& ss)
	{
		// TransformParent is used by macros to create a mutable parent and thence add siblings

		cstr srcCode =
			"(namespace EntryPoint)\n"
			" (alias Main EntryPoint.Main)\n"

			"(using Sys.Type)\n"
			"(using Sys.Reflection)\n"

			"(function Main -> (Int32 result):\n"
			"	(IExpression s = '(cheese oranges (#paragraph 1024 apples pears) onion bikes))\n"
			"	(IExpression sOranges = (s 1))\n"
			"	(IExpressionBuilder sBuilder = sOranges.TransformParent)\n"

			"   (Int32 index = (sBuilder.IndexOf sOranges))\n"

			"   (IExpressionBuilder builderSequence = (sBuilder.InsertCompoundAfter index))"

			"   (result = index)\n"
			")\n"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 1);
	}

	void TestMacroSiblings2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(#string welcome 64 \"Hello World!&n\")"
			"	(Sys.Print welcome -> result)"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int welcomeLength = vm.PopInt32();
		validate(welcomeLength > 0);
	}

	void TestMacroSiblings3Throws(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"   (#token smallToken \"123456789012345678901234567890123456789012345678901234567890&n\")"
			"   (Sys.Print smallToken)"
			"   (try "
			"	 (((#token tooBigToken \"12345678901234567890123456789012345678901234567890123456789012345\")))"
			"    catch ex"
			"		("
	        "			(Sys.Print ex.Message -> result)"
			"			(Sys.Print \"&n\")"
			"		)"
			"   )"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int welcomeLength = vm.PopInt32();
		validate(welcomeLength > 10);
	}

	void AddTestSecurity(IPublicScriptSystem& ss)
	{
		struct SecuritySystem : ISecuritySystem
		{
			void ValidateSafeToWrite(IPublicScriptSystem& ss, cstr pathname)
			{
				UNUSED(ss);
				UNUSED(pathname);
			}

			void ValidateSafeToRead(IPublicScriptSystem& ss, cstr pathname)
			{
				UNUSED(ss);
				UNUSED(pathname);
			}
		};
		static SecuritySystem s_security;

		ss.SetSecurityHandler(s_security);
	}

	void TestConsoleOutput(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			" (alias Main EntryPoint.Main)\n"

			"(using Sys.Type)\n"
			"(using Sys.IO)\n"
			"(using Sys.Type.Formatters)\n"
			"(using Sys.Maths)\n"

			"(function Main -> (Int32 result):\n"
			"   (IWriter stdout = Sys.IO.GetStdOut)\n"
			"   (#build stdout \"Hello World\" \"!!!&n&tDecimal(256): \" 256 \"&n&tHex(256): 0x\" Hex 256 \"&n&tUnsigned(-1): \" Unsigned -1 \"&n\")\n"
			"   (Bool falseValue = false)\n"
			"   (Bool trueValue = true)\n"
			"   (#build stdout \"&n&tfalse: \" falseValue)\n"
			"   (#build stdout \"&n&ttrue: \" trueValue)\n"
			"   (Int64 bigNum = 0x1245678ABCDEF00)"
			"   (#build stdout \"&n&tbigNum: 0x\" Hex bigNum)\n"
			"   (#build stdout \"&n&tbigNum: \" Decimal bigNum)\n"
			"   (Float32 shortPI = (#pi))\n"
			"   (Float64 longPI = (#pi))\n"
			"   (#build stdout \"&n&tPI-F32: \" shortPI)\n"
			"   (#build stdout \"&n&tPI-F64: \" longPI)\n"
			"   (#build stdout \"&n\")\n"
			"   (IString happyDays = \"Happy Days\")\n"
			"	(#build stdout \"&n&tPointer: 0x\" happyDays.Buffer)\n"
			"	(#build stdout \"&n&tAscii Tilde(126): \")\n"
			"	(stdout.WriteChar 126)\n"
			"   (Float64 smallNum = 0.0001)\n"
			"   (stdout.SetPrecision 4)\n"
			"   (stdout.SetWidth 16)\n"
			"   (stdout.SetZeroPrefix false)\n"
			"   (stdout.SetRightAlign false)\n"
			"	(#build stdout \"&n&tE:\" SpecE smallNum)\n"
			"	(#build stdout \"&n&tF:\" SpecF smallNum)\n"
			"   (stdout.SetRightAlign true)\n"
			"   (#build stdout \"&n&tFormatted:\" Format \"Hi\")\n"
			"	(#build stdout \"&n&tG:\" SpecG smallNum)\n"
			"	(stdout.SetFormattingToDefault)\n"
			"	(#build stdout \"&n&tG:\" SpecG smallNum)\n"
			"   (#build stdout \"&n\")\n"
			"   (IFileWriter testFile = (Sys.IO.WriteToFile \"..\\..\\gen\\temp\\console.test.txt\"))\n"
			"   (#build testFile \"Hello World\")\n"
			"   (Int64 pos = testFile.Position)\n"
			"   (testFile.FlushWhenWritten)"
			"   (testFile.SetPosition 1)\n"
			"   (testFile \"u\")\n"
			"	(testFile.SetPosition pos)\n"
			"	(testFile \"!\")\n"
			"   (testFile.WriteSubstring \"Enjoy\" 2 2)\n"
			"   (Int64 endPos = testFile.Length)\n"
			"   (result = (Sys.Maths.I64.ToInt32 endPos))"
			"   (Sys.IO.CloseWriter testFile)\n"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		AddTestSecurity(ss);

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == strlen("Hullo World!jo"));
	}

	void TestConsoleOutput2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			" (alias Main EntryPoint.Main)\n"

			"(using Sys.Type)\n"
			"(using Sys.IO)\n"
			"(using Sys.Type.Formatters)\n"
			"(using Sys.Maths)\n"

			"(function Main -> (Int32 result):\n"
			"   (#printf \"Hello World!\")\n"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestConsoleOutput3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			" (alias Main EntryPoint.Main)\n"

			"(using Sys.Type)\n"
			"(using Sys.IO)\n"
			"(using Sys.Type.Formatters)\n"
			"(using Sys.Maths)\n"

			"(function Main -> (Int32 result):\n"
			"   (IString newLine = \"&n\")\n"
			"   (#printf \"Command Line:\" CommandLine newLine)\n"
			"   (#string env 32768)\n"
			"   (Sys.IO.AppendEnvironmentVariable \"Rhodes\" env)\n"
			"   (#printf \"env-ZelaznyChops: \" env newLine)\n"
			"   (env.Clear)"
			"   (Sys.IO.AppendEnvironmentVariable \"COMPUTERNAME\" env)\n"
			"   (#printf \"env-COMPUTERNAME: \" env newLine)\n"
			"   (env.Clear)"
			"   (Sys.IO.AppendEnvironmentVariable \"USERPROFILE\" env)\n"
			"   (#printf \"env-USERPROFILE: \" env newLine)\n"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestConsoleOutput4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			" (alias Main EntryPoint.Main)\n"

			"(using Sys.Type)\n"
			"(using Sys.IO)\n"
			"(using Sys.Reflection)\n"

			"(function Main -> (Int32 result):\n"
			"   (IExpression s = ' ((Hello world!) \"How are you today?&n\"))\n"
			"   (IWriter stdout = GetStdOut)"
			"   (stdout.SetWidth 4)"
			"   (stdout.SetPrecision 1)"
			"   (#build stdout s \"&n\")\n"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestCoroutine1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys)"
			"(using Sys.Maths)"
			"(using Sys.Reflection)"

			"(class PrintDogs (implements ICoroutine))"
			"(method PrintDogs.Construct :)"
			"(factory Sys.NewPrintDogs ICoroutine : (construct PrintDogs))"
			"(class PrintCats (implements ICoroutine))"
			"(method PrintCats.Construct :)"
			"(factory Sys.NewPrintCats ICoroutine : (construct PrintCats))"

			"(method PrintDogs.Run -> :"
			" (Print \"RobotDog\")"
			" (yield)"
			" (Print \"ToyDog\")"
			" (yield)"
			" (Print \"BigDog\")"
			")"

			"(method PrintCats.Run -> :"
			" (Print \"FluffyCat\")"
			" (yield 50)"
			" (Print \"MagicCat\")"
			" (yield)"
			" (Print \"Mooncat\")"
			")"

			"(function Main -> (Int32 result):"
			"	(ICoroutine dogs (NewPrintDogs))"
			"	(ICoroutine cats (NewPrintCats))"
			"	(ICoroutineControl coroutines (Coroutines))"
			"   (coroutines.Add dogs)"
			"   (coroutines.Add cats)"
			"   (while (coroutines.Continue != 0))"
			"   (Sys.Print \"yo!\")"
			"   (Int64 dogId = (coroutines.Add dogs))"
			"   (coroutines.Add cats)"
			"   (coroutines.Release dogId)"
			"   (while (coroutines.Continue != 0))"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 0);
	}


	int32 RunScriptMain(IPublicScriptSystem& ss, cstr srcCode)
	{
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0);
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		return vm.PopInt32();
	}

	// Single coroutine whose Run returns; Main must still be able to assign a result.
	void TestCoroutineCompleteOne(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(using Sys)"
			"(class Quick (implements ICoroutine))"
			"(method Quick.Construct :)"
			"(factory Sys.NewQuick ICoroutine : (construct Quick))"
			"(method Quick.Run -> : )"
			"(function Main -> (Int32 result):"
			"	(ICoroutine q (NewQuick))"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(coroutines.Add q)"
			"	(Int64 id = (coroutines.Continue))"
			"	(Int64 empty = (coroutines.Continue))"
			"	(if ((id != 0) and (empty == 0)) (result = 42) else (result = 0))"
			")";
		validate(RunScriptMain(ss, srcCode) == 42);
	}

	// Last remaining runnable coroutine terminates while another is timed-dormant.
	// while (Continue > 0) must return -1 (or 0) without crashing, then we ReleaseAll.
	void TestCoroutineCompleteLastWhileDormant(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(using Sys)"
			"(global Int32 done = 0)"
			"(class Sleeper (implements ICoroutine))"
			"(method Sleeper.Construct :)"
			"(factory Sys.NewSleeper ICoroutine : (construct Sleeper))"
			"(method Sleeper.Run -> : (yield 2000000))"
			"(class Worker (implements ICoroutine))"
			"(method Worker.Construct :)"
			"(factory Sys.NewWorker ICoroutine : (construct Worker))"
			"(method Worker.Run -> :"
			"	(Int32 x)"
			"	(global done -> x)"
			"	(x += 1)"
			"	(global done <- x)"
			")"
			"(function Main -> (Int32 result):"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(ICoroutine sleeper (NewSleeper))"
			"	(ICoroutine worker (NewWorker))"
			"	(coroutines.Add sleeper)"
			"	(coroutines.Add worker)"
			"	(while (coroutines.Continue > 0))"
			"	(coroutines.ReleaseAll)"
			"	(global done -> result)"
			")";
		validate(RunScriptMain(ss, srcCode) == 1);
	}

	void TestCoroutineReleaseAllThenContinue(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(using Sys)"
			"(class Q (implements ICoroutine))"
			"(method Q.Construct :)"
			"(factory Sys.NewQ ICoroutine : (construct Q))"
			"(method Q.Run -> : (yield))"
			"(function Main -> (Int32 result):"
			"	(ICoroutine q (NewQ))"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(coroutines.Add q)"
			"	(coroutines.ReleaseAll)"
			"	(Int64 last = (coroutines.Continue))"
			"	(if (last == 0) (result = 1) else (result = 0))"
			")";
		validate(RunScriptMain(ss, srcCode) == 1);
	}

	// Uncaught throw inside Run: the test harness catches the scheduler error,
	// the host stack is restored, and a subsequent VM push/pop works.
	void TestCoroutineThrowInRun(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(using Sys)"
			"(class Boom (implements ICoroutine))"
			"(method Boom.Construct :)"
			"(factory Sys.NewBoom ICoroutine : (construct Boom))"
			"(method Boom.Run -> : (Sys.Throw 1 \"boom\"))"
			"(function Main -> (Int32 result):"
			"	(ICoroutine b (NewBoom))"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(coroutines.Add b)"
			"	(coroutines.Continue)"
			"	(result = 99)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0);

		auto stackBytes = [&]() -> int64 { return (int64)(vm.Cpu().StackEnd - vm.Cpu().StackStart); };

		bool caught = false;
		try
		{
			EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
			if (result == EXECUTERESULT_TERMINATED)
			{
				Throw(0, "Expected the coroutine throw to abort Main");
			}
			caught = true;
			printf("TestCoroutineThrowInRun: Execute returned %d\n", (int)result);
		}
		catch (IException& e)
		{
			caught = true;
			printf("TestCoroutineThrowInRun: %s\n", e.Message());
			cstr msg = e.Message();
			bool hasCrash = false;
			if (msg)
			{
				for (cstr p = msg; *p; p++)
				{
					if (p[0] == 'B' && p[1] == 'o' && p[2] == 'o' && p[3] == 'm' && p[4] == ' ' && p[5] == 'c')
					{
						hasCrash = true;
						break;
					}
				}
			}
			validate(hasCrash);
		}

		validate(caught);
		validate(stackBytes() == 65536);
		vm.SetStatus(EXECUTERESULT_RUNNING);
		vm.Push((int32)123);
		validate(vm.PopInt32() == 123);
		s_logger.Clear();
	}

	// Timed-yield, then Release the id without keeping a local. Destruct should run; no leak.
	void TestCoroutineReleaseDormantNoLeak(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(using Sys)"
			"(global Int32 destroyed = 0)"
			"(class Sleeper (implements ICoroutine))"
			"(method Sleeper.Construct :)"
			"(factory Sys.NewSleeper ICoroutine : (construct Sleeper))"
			"(method Sleeper.Run -> : (yield 2000000))"
			"(method Sleeper.Destruct -> :"
			"	(Int32 x)"
			"	(global destroyed -> x)"
			"	(x += 1)"
			"	(global destroyed <- x)"
			")"
			"(function Main -> (Int32 result):"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(ICoroutine s (NewSleeper))"
			"	(Int64 id = (coroutines.Add s))"
			"	(ICoroutine none)"
			"	(s = none)"
			"	(Int64 last = (coroutines.Continue))"
			"	(coroutines.Release id)"
			"	(Int32 d)"
			"	(global destroyed -> d)"
			"	(if ((last != 0) and (d == 1)) (result = 1) else (result = 0))"
			")";
		validate(RunScriptMain(ss, srcCode) == 1);
	}

	// Add from inside Run is deferred until the slice ends; both coroutines then complete.
	void TestCoroutineAddDuringRun(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(using Sys)"
			"(global Int32 hits = 0)"
			"(class Other (implements ICoroutine))"
			"(method Other.Construct :)"
			"(factory Sys.NewOther ICoroutine : (construct Other))"
			"(method Other.Run -> :"
			"	(Int32 x)"
			"	(global hits -> x)"
			"	(x += 1)"
			"	(global hits <- x)"
			")"
			"(class Worker (implements ICoroutine))"
			"(method Worker.Construct :)"
			"(factory Sys.NewWorker ICoroutine : (construct Worker))"
			"(method Worker.Run -> :"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(ICoroutine o (NewOther))"
			"	(coroutines.Add o)"
			"	(yield)"
			"	(Int32 x)"
			"	(global hits -> x)"
			"	(x += 1)"
			"	(global hits <- x)"
			")"
			"(function Main -> (Int32 result):"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(ICoroutine w (NewWorker))"
			"	(coroutines.Add w)"
			"	(while (coroutines.Continue > 0))"
			"	(global hits -> result)"
			")";
		validate(RunScriptMain(ss, srcCode) == 2);
	}

	// Release(self) from inside Run is deferred until the slice ends.
	void TestCoroutineReleaseSelfDuringRun(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(using Sys)"
			"(global Int64 myId = 0)"
			"(global Int32 ran = 0)"
			"(class SelfRel (implements ICoroutine))"
			"(method SelfRel.Construct :)"
			"(factory Sys.NewSelfRel ICoroutine : (construct SelfRel))"
			"(method SelfRel.Run -> :"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(Int64 id)"
			"	(global myId -> id)"
			"	(coroutines.Release id)"
			"	(Int32 x)"
			"	(global ran -> x)"
			"	(x += 1)"
			"	(global ran <- x)"
			"	(yield)"
			")"
			"(function Main -> (Int32 result):"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(ICoroutine slf (NewSelfRel))"
			"	(Int64 id = (coroutines.Add slf))"
			"	(global myId <- id)"
			"	(coroutines.Continue)"
			"	(Int64 after = (coroutines.Continue))"
			"	(Int32 x)"
			"	(global ran -> x)"
			"	(if ((x == 1) and (after == 0)) (result = 1) else (result = 0))"
			")";
		validate(RunScriptMain(ss, srcCode) == 1);
	}

	// ReleaseAll from inside Run is deferred until the slice ends.
	void TestCoroutineReleaseAllDuringRun(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(using Sys)"
			"(class Worker (implements ICoroutine))"
			"(method Worker.Construct :)"
			"(factory Sys.NewWorker ICoroutine : (construct Worker))"
			"(method Worker.Run -> :"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(coroutines.ReleaseAll)"
			"	(yield)"
			")"
			"(function Main -> (Int32 result):"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(ICoroutine w (NewWorker))"
			"	(coroutines.Add w)"
			"	(coroutines.Continue)"
			"	(Int64 after = (coroutines.Continue))"
			"	(if (after == 0) (result = 1) else (result = 0))"
			")";
		validate(RunScriptMain(ss, srcCode) == 1);
	}

	// Nested Continue from inside Run throws; the harness catches it, host stack is restored.
	void TestCoroutineNestedContinue(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(using Sys)"
			"(class Nested (implements ICoroutine))"
			"(method Nested.Construct :)"
			"(factory Sys.NewNested ICoroutine : (construct Nested))"
			"(method Nested.Run -> :"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(coroutines.Continue)"
			")"
			"(function Main -> (Int32 result):"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(ICoroutine n (NewNested))"
			"	(coroutines.Add n)"
			"	(coroutines.Continue)"
			"	(result = 99)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0);

		auto stackBytes = [&]() -> int64 { return (int64)(vm.Cpu().StackEnd - vm.Cpu().StackStart); };

		bool caught = false;
		try
		{
			EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
			if (result == EXECUTERESULT_TERMINATED)
			{
				Throw(0, "Expected nested Continue to abort Main");
			}
			caught = true;
			printf("TestCoroutineNestedContinue: Execute returned %d\n", (int)result);
		}
		catch (IException& e)
		{
			caught = true;
			printf("TestCoroutineNestedContinue: %s\n", e.Message());
			cstr msg = e.Message();
			bool hasMsg = false;
			cstr needle = "Cannot call ICoroutineControl.Continue";
			if (msg)
			{
				for (cstr p = msg; *p; p++)
				{
					cstr n = needle;
					cstr q = p;
					while (*n && *q && *n == *q) { n++; q++; }
					if (*n == 0) { hasMsg = true; break; }
				}
			}
			validate(hasMsg);
		}

		validate(caught);
		validate(stackBytes() == 65536);
		vm.SetStatus(EXECUTERESULT_RUNNING);
		vm.Push((int32)123);
		validate(vm.PopInt32() == 123);
		s_logger.Clear();
	}

	// (yield 0) must be an immediate yield, not a timed wait or a leftover D4 wait.
	void TestCoroutineYieldZero(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(using Sys)"
			"(global Int32 hits = 0)"
			"(class A (implements ICoroutine))"
			"(method A.Construct :)"
			"(factory Sys.NewA ICoroutine : (construct A))"
			"(method A.Run -> :"
			"	(yield 0)"
			"	(Int32 x)"
			"	(global hits -> x)"
			"	(x += 1)"
			"	(global hits <- x)"
			")"
			"(class B (implements ICoroutine))"
			"(method B.Construct :)"
			"(factory Sys.NewB ICoroutine : (construct B))"
			"(method B.Run -> :"
			"	(yield 0)"
			"	(Int32 x)"
			"	(global hits -> x)"
			"	(x += 1)"
			"	(global hits <- x)"
			")"
			"(function Main -> (Int32 result):"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(ICoroutine a (NewA))"
			"	(ICoroutine b (NewB))"
			"	(coroutines.Add a)"
			"	(coroutines.Add b)"
			"	(while (coroutines.Continue > 0))"
			"	(global hits -> result)"
			")";
		validate(RunScriptMain(ss, srcCode) == 2);
	}

	// Coroutine stacks are StackPool::StackSize (8K). Deep recurses far past that while Victim
	// is parked mid-Run with a sentinel in its own frame. Without guard pages the overflow
	// writes into whatever memory follows Deep's stack (another coroutine stack or heap metadata).
	// With guard pages it must fail cleanly: Continue throws "... stack overflow", the host
	// stack is restored, and Victim can still be resumed and finds its sentinel intact.
	void TestCoroutineStackOverflow(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(alias Resume EntryPoint.Resume)"
			"(using Sys)"
			"(global Int32 victimOk = 0)"
			"(function Recurse (Int32 n) -> (Int32 depth):"
			"	(Int32 pad1 = n)"
			"	(Int32 pad2 = n)"
			"	(if (n > 0) (depth = (Recurse (n - 1))) else (depth = 0))"
			")"
			"(class Deep (implements ICoroutine))"
			"(method Deep.Construct :)"
			"(factory Sys.NewDeep ICoroutine : (construct Deep))"
			"(method Deep.Run -> :"
			"	(yield)"
			"	(Recurse 2000)"
			")"
			"(class Victim (implements ICoroutine))"
			"(method Victim.Construct :)"
			"(factory Sys.NewVictim ICoroutine : (construct Victim))"
			"(method Victim.Run -> :"
			"	(Int64 sentinel = 1234567890123)"
			"	(yield)"
			"	(yield)"
			"	(yield)"
			"	(if (sentinel == 1234567890123)"
			"		(Int32 one = 1)"
			"		(global victimOk <- one)"
			"	)"
			")"
			"(function Main -> (Int32 result):"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(ICoroutine deep (NewDeep))"
			"	(ICoroutine victim (NewVictim))"
			"	(coroutines.Add deep)"
			"	(coroutines.Add victim)"
			"	(while (coroutines.Continue != 0))"
			"	(global victimOk -> result)"
			")"
			"(function Resume -> (Int32 result):"
			"	(ICoroutineControl coroutines (Coroutines))"
			"	(while (coroutines.Continue != 0))"
			"	(global victimOk -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0);

		auto stackBytes = [&]() -> int64 { return (int64)(vm.Cpu().StackEnd - vm.Cpu().StackStart); };

		auto contains = [](cstr msg, cstr needle) -> bool
		{
			if (msg)
			{
				for (cstr p = msg; *p; p++)
				{
					cstr n = needle;
					cstr q = p;
					while (*n && *q && *n == *q) { n++; q++; }
					if (*n == 0) return true;
				}
			}
			return false;
		};

		// The scheduler error either escapes Execute as an IException or is logged and ends Main with EXECUTERESULT_THROWN
		HString report;
		s_logger.lastWritten = HString();
		try
		{
			EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
			printf("TestCoroutineStackOverflow: Execute returned %d", (int)result);
			if (result == EXECUTERESULT_TERMINATED)
			{
				printf(", victimOk = %d", vm.PopInt32());
			}
			printf("\n");
			if (result == EXECUTERESULT_THROWN) report = s_logger.lastWritten;
		}
		catch (IException& e)
		{
			printf("TestCoroutineStackOverflow: %s\n", e.Message());
			report = e.Message() ? e.Message() : "";
		}

		validate(contains(report.c_str(), "stack overflow"));

		// The report names the concrete class behind the ICoroutine interface, its id and the function that was running
		validate(contains(report.c_str(), "Coroutine Deep crashed"));
		validate(contains(report.c_str(), "class Deep of "));
		validate(contains(report.c_str(), "coroutine id "));
		validate(contains(report.c_str(), "executing Recurse"));
		validate(stackBytes() == 65536);
		vm.SetStatus(EXECUTERESULT_RUNNING);
		vm.Push((int32)123);
		validate(vm.PopInt32() == 123);
		s_logger.Clear();

		// The scheduler still holds Victim. Resume it from a fresh entry point: its stack must be intact.
		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Resume"));
		vm.Push(0);
		EXECUTERESULT resumeResult = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(resumeResult);
		validate(vm.PopInt32() == 1);
	}

	// Non-coroutine script code that recurses without limit. Main's 64K VM stack must end in a clean EXECUTERESULT_SEH with SP at or
	// past StackEnd (a guard page was hit), and the VM must stay usable: another entry point runs, and a second overflow behaves the same.
	void TestVMStackOverflow(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"
			"(alias Second EntryPoint.Second)"
			"(function Recurse (Int32 n) -> (Int32 depth):"
			"	(Int32 pad1 = n)"
			"	(Int32 pad2 = n)"
			"	(if (n > 0) (depth = (Recurse (n - 1))) else (depth = 0))"
			")"
			"(function Main -> (Int32 result):"
			"	(result = (Recurse 100000))"
			")"
			"(function Second -> (Int32 result):"
			"	(result = 42)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		validate(ns != NULL);

		for (int pass = 0; pass < 2; pass++)
		{
			validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Main"));
			vm.Push(0);

			EXECUTERESULT result = EXECUTERESULT_RUNNING;
			try
			{
				result = vm.Execute(VM::ExecutionFlags(false, true));
				printf("TestVMStackOverflow: pass %d Execute returned %d, exception code %d, SP - StackEnd = %lld\n", pass, (int)result, (int)vm.Cpu().ExceptionCode, (long long)(vm.Cpu().SP() - vm.Cpu().StackEnd));
			}
			catch (IException& e)
			{
				printf("TestVMStackOverflow: pass %d %s\n", pass, e.Message());
			}

			validate(result == EXECUTERESULT_SEH);
			validate(vm.Cpu().ExceptionCode == VM::EXCEPTIONCODE_BAD_ADDRESS);
			validate(vm.Cpu().SP() >= vm.Cpu().StackEnd);
			validate(vm.Cpu().SP() - vm.Cpu().StackEnd <= 4096); // stopped at the end of the stack, not somewhere in the memory beyond it
			validate(vm.Cpu().StackEnd - vm.Cpu().StackStart == 65536);

			vm.SetStatus(EXECUTERESULT_RUNNING);
			s_logger.Clear();

			validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, "Second"));
			vm.Push(0);
			EXECUTERESULT secondResult = vm.Execute(VM::ExecutionFlags(false, true));
			ValidateExecution(secondResult);
			validate(vm.PopInt32() == 42);
		}
	}

	void TestTypenames(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(result = (Sys.Print type.Name))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 32);
	}

	void TestMutableArgFunction(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"

			"(function DoubleVec (const Vec2i p)(out Vec2i result) -> :"
			"	(result.x = p.x * 2)"
			"	(result.y = p.y * 2)"
			")"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(result = (Sys.Print type.Name))"
			"   (Vec2i p = 7 9)"
			"   (Vec2i q)"
			"   (DoubleVec p q)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		printf("\n");

		int x = vm.PopInt32();
		validate(x == 32);
	}

	void TestNullOutInterface(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"


			"(function Main -> (Int32 result):"
			"	(Sys.Type.IString s = \"Hello World\")"
			"   (s = 0)"
			"	(result = s.Length)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		printf("\n");

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestConstArgFunction(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"

			"(function DoubleVec (const Vec2i p)(Vec2i result) -> :"
			"	(result.x = p.x * 2)"
			"	(result.y = p.y * 2)"
			")"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(result = (Sys.Print type.Name))"
			"   (Vec2i p = 7 9)"
			"   (Vec2i q)"
			"   (DoubleVec p q)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		printf("\n");

		int x = vm.PopInt32();
		validate(x == 32);
	}

	void TestTypeOf(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(result = (Sys.Print type.Name))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		printf("\n");

		int x = vm.PopInt32();
		validate(x == 32);
	}

	void TestTypeOfClass(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"sexy(
			(namespace EntryPoint)			
			 (alias Main EntryPoint.Main)

			(using Sys.IO)
			(using Sys.Maths)
			(using Sys.Reflection)
			(using Sys.Type)

			(interface Sys.Type.IDynamic (attribute dispatch))

			(struct MouseArgs (Int32 x y))

			(class MouseMoveHandler (implements Sys.Type.IDynamic))

			(method MouseMoveHandler.OnMouseMove (MouseArgs args)-> :
				(#printf "Mouse: " args.x " " args.y "&n" )
			)
			
			(method MouseMoveHandler.Construct : )

			(factory Sys.Type.NewMouseMoveHandler Sys.Type.IDynamic : (construct MouseMoveHandler))			

			(function Main -> (Int32 result):
				(Sys.Type.IDynamic handler (Sys.Type.NewMouseMoveHandler))
				(IStructure type = typeof handler)
				(#printf (type.Name) "&n")
			))sexy";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		printf("\n");

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestEnumerateInterfaces(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(result = type.InterfaceCount)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		printf("\n");

		int x = vm.PopInt32();
		validate(x == 1);
	}

	void TestGetInterfaceName(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(IString i0 = type.GetInterfaceName)"
			"   (result = (Sys.Print i0))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		printf("\n");

		int x = vm.PopInt32();
		validate(x == 11);
	}

	void TestGetMethodCount(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(Int32 count = (type.GetMethodCount))"
			"   (result = count)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		printf("\n");

		int x = vm.PopInt32();
		validate(x == 9);
	}

	void TestAppendMethodName(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"
			"(using Sys.Type.Strings)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(IStringBuilder nb = NewTokenBuilder)"
			"	(type.AppendMethodName nb 0)"
			"   (result = (Sys.Print nb))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		printf("\n");
		validate(x == 5);
	}

	void TestGetMethodArgCounts(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"
			"(using Sys.Type.Strings)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(Int32 inputCount)(Int32 outputCount)"
			"	(type.GetMethodArgCounts 0 -> inputCount outputCount)"
			"   (result = (inputCount + (7 * outputCount)))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 9); // methods have an intrinsic input of the instance ref, with Child we also have the index, and there is one output. (2 + 7 x 1) = 9
	}

	void TestGetMethodGetInputTypeAndName(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"
			"(using Sys.Type.Strings)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(IStringBuilder sbName = NewTokenBuilder)"
			"	(IStringBuilder sbType = NewTokenBuilder)"
			"	(type.AppendInputTypeAndName 0 0 sbType sbName)"
			"	(IStringBuilder sbTypeAndName = NewTokenBuilder)"
			"   (#build sbTypeAndName sbType \" \" sbName)"
			"   (result = (Sys.Print sbTypeAndName))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		printf("\n");
		validate(x == 11);
	}

	void TestGetMethodGetOutputTypeAndName(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"
			"(using Sys.Type.Strings)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(IStringBuilder sbName = NewTokenBuilder)"
			"	(IStringBuilder sbType = NewTokenBuilder)"
			"	(type.AppendOutputTypeAndName 0 0 sbType sbName)"
			"	(IStringBuilder sbTypeAndName = NewTokenBuilder)"
			"   (#build sbTypeAndName sbType \" \" sbName)"
			"   (result = (Sys.Print sbTypeAndName))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		printf("\n");
		validate(x == 11);
	}

	void TestGetMethodGetInputType(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"
			"(using Sys.Type.Strings)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"   (IStructure argType = (type.GetInputType 0 0))"
			"   (result = (Sys.Print argType.Name))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		printf("\n");
		validate(x == 5);
	}

	void TestGetMethodGetOutputType(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"
			"(using Sys.Type.Strings)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"   (IStructure argType = (type.GetOutputType 0 0))"
			"   (result = (Sys.Print argType.Name))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		printf("\n");
		validate(x == 32);
	}

	void TestIsMethodInputOfType(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"
			"(using Sys.Type.Strings)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(IStructure int32type = typeof Sys.Type.Int32)"
			"   (if (type.IsMethodInputOfType 0 0 int32type)"
			"       (result = 7)"
			"   )"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		printf("\n");
		validate(x == 7);
	}

	void TestIsMethodOutputOfType(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"
			"(using Sys.Type.Strings)"

			"(function Main -> (Int32 result):"
			"	(IStructure type = typeof Sys.Reflection.IExpression)"
			"	(IStructure childOutputType = typeof Sys.Reflection.IExpression)"
			"   (if (type.IsMethodOutputOfType 0 0 childOutputType)"
			"       (result = 7)"
			"   )"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		printf("\n");
		validate(x == 7);
	}

	void TestInvokeMethodViaReflectionFails(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"
			"(using Sys.Type.Strings)"

			"(function Main -> (Int32 result):"
			"   (IStringBuilder sb = NewTokenBuilder)"
			"   (Sys.Maths.Vec2i point = 7258 8369)"
			"   (IString method = \"AppendInt64\")"
			"	(invoke sb method point)"
			"   (Sys.Print sb)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);
		s_logger.Clear();
	}

	void TestInvokeMethodViaReflection(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"
			"(using Sys.Type.Strings)"

			"(interface Sys.Type.IRobot (attribute dispatch)"
			")"

			"(class Robot (implements Sys.Type.IRobot))"

			"(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))"

			"(method Robot.Construct : )"

			"(method Robot.MoveTo (Vec2i pos)-> : "
			"   (IStringBuilder sb = NewTokenBuilder)"
			"   (#build sb pos.x \" \" pos.y)"
			"   (Sys.Print sb)"
			")"

			"(function Main -> (Int32 result):"
			"   (IRobot robot (NewRobot))"
			"   (Sys.Maths.Vec2i point = 7258 8369)"
			"   (robot.MoveTo point)"
			"   (IString moveToMethod = \"MoveTo\")"
			"	(invoke robot moveToMethod point)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		printf("\n");
	}

	void TestExpressionArg(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Maths)"
		"(using Sys.Reflection)"
  
		"(function Main -> (Int32 result):"
		"	(IExpression s = '($1 = ($1 * 2)) )"
		"	(s.ChildCount -> result)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestExpressionArg");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 3);
	}

	void TestExpressionAppendTo(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(IExpression s = ' (Hello \" \" world))"
			"   (IStringBuilder sb = NewTokenBuilder)"
			"	(for  (Int32 i = 0)   (i < s.ChildCount)  (i += 1)" 
			"       (IExpression child = (s i))"
			"		(child.AppendTextTo sb)"
			"   )"
			"   (Sys.Print sb -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 11);
	}

	void TestStringBuilderLength1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type)"

			"(class Apple (defines Sys.IApple)(IStringBuilder sb))"
			"(method Apple.Construct : "
			"	(this.sb = NewTokenBuilder)"
			"	(#build this.sb \"12345678!\")"
			")"

			"(method Apple.GetLength -> (Int32 length):"
			"	(length = this.sb.Length)"
			")"

			"(factory Sys.NewApple Sys.IApple : (construct Apple))"

			"(function Main -> (Int32 result):"
			"	(Sys.IApple apple (Sys.NewApple))"
			"	(apple.GetLength -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 9);
	}

	void TestSubstitution(IPublicScriptSystem& ss)
	{
		cstr macroCode =
			"(namespace EntryPoint)"
			"(using Sys.Reflection)"
			"(macro EntryPoint.double in out"
			"	(IExpression s = '($1 = ($1 * 2)) )"
			"	(out.Substitute in s)"
			")";

		cstr mainCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			
			"(function Main -> (Int32 result):"
			"	(result = 7)"
			"	(#EntryPoint.double result)"
			")";

		Auto<ISourceCode> sc1 = ss.SParser().ProxySourceBuffer(macroCode, -1, Vec2i{ 0,0 }, "TestSubstitution_Macro");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc1()));

		Auto<ISourceCode> sc2 = ss.SParser().ProxySourceBuffer(mainCode, -1, Vec2i{ 0,0 }, "TestSubstitution_Main");
		Auto<ISParserTree> tree2(ss.SParser().CreateTree(sc2()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree(), tree2());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 14);
	}

	void TestStringMember(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct Mouse"
			"	(Int32 x)"
			"	(Int32 y)"
			"   (IString name)"
			")"

			"(function Main -> (Int32 result):"
			"	(Mouse m)"
			"	(m.name = \"Geoff\")"
			"	(Sys.Print m.name -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestStringMember");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 5);
	}

	void TestStringMember2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct Mouse"
			"	(Int32 x)"
			"	(Int32 y)"
			"   (IString name)"
			")"

			"(function SetMouseName (Mouse m) -> :"
			"	(m.name = \"Geoff\")"
			")"

			"(function Main -> (Int32 result):"
			"	(Mouse m)"
			"	(SetMouseName m)" 
			"	(Sys.Print m.name -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestStringMember2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 5);
	}

	void TestStringMember3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct Button"
			"	(Int32 state)"
			"   (IString name)"
			")"

			"(struct Mouse"
			"	(Button b1)"
			"	(Button b2)"
			"   (IString name)"
			")"

			"(function Main -> (Int32 result):"
			"	(Mouse m = (0 \"left\") (1 \"right\") \"Helen\")"
			"	(Sys.Print m.name -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestStringMember3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 5);
	}

	void TestStringBuilder(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
		"(using Sys.Maths)"
		"(using Sys.Reflection)"
		"(using Sys.Type.Formatters)"
  
		"(function Main -> (Int32 result):"
		"	(Sys.Type.IStringBuilder s = NewPathBuilder)"
		"	(s.AppendIString \"Hello World!*\")"
		"	(Sys.Print s -> result)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStringBuilder");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 13);
	}

	void TestStringBuilderBig(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
		"(using Sys.Maths)"
		"(using Sys.Reflection)"
		"(using Sys.Type.Formatters)"
  
		"(function Main -> (Int32 result):"
		"	(Sys.Type.IStringBuilder s = NewPathBuilder)"
		"	(#Sys.Type.build  s \"Hello World![] \" NewLine)"
//		"	(#Sys.Type.build  s \"Decimal: \" Decimal 1234 \". Hex: 0x\" Hex 1234 NewLine)"
//		"	(#Sys.Type.build  s \"E form: \" SpecE 3.e-2 \". F form: \" SpecF 3.e-2 \". G form: \" SpecG 3.e-2 NewLine)"
//		"	(cast s -> IString str)"
//		"	(Sys.Print str -> result)"
//		"	(s.Clear) (s.SetFormat 4 4 false false)"
//		"	(#Sys.Type.build  s \"Hello World! \" NewLine)"
//		"	(#Sys.Type.build  s \"Decimal: \" Decimal 1234 \". Hex: 0x\" Hex 1234 NewLine)"
//		"	(#Sys.Type.build  s \"E form: \" SpecE 3.e-2 \". F form: \" SpecF 3.e-2 \". G form: \" SpecG 3.e-2 NewLine)"

		"	(Sys.Print s -> result)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStringBuilderBig");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	//	ValidateExecution(result);
		UNUSED(result);
		int x = vm.PopInt32();
		UNUSED(x);
//		validate(x > 11); 
	}

	void TestRefTypesInsideClosure(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Maths)"
		"(using Sys.Reflection)"
		"(using Sys.Type.Formatters)"

		"(archetype EntryPoint.VoidToVoid ->)"

		"(function GoPrint (Sys.Type.IStringBuilder s) -> :"
		"	(VoidToVoid g = "
		"			(closure -> :"
		"				(Sys.Print s)" // tests that the compiler passes an upvalue reference correctly
		"			)"
		"	)"
		"(g)"
		")"
  
		"(function Main -> (Int32 result):"
		"	(Sys.Type.IStringBuilder s = Sys.Type.NewPathBuilder)"
		"	(#Sys.Type.build  s \"Hello World!(@) \" NewLine)"
		"	(GoPrint s)(result = 12)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStringBuilderInsideClosure");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 12);
	}

	void TestCaptureInFunctionAllInClosure(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type.Formatters)"
			"(using Sys.Type)"

			"(archetype EntryPoint.VoidToVoid ->)"

			"(function Double(Int32 x)->(Int32 y) :"
			"	(y = (2 * x))"
			")"

			"(function Main -> (Int32 result): "
			"	(Int32 x = 17)"
			"	(VoidToVoid g ="
			"		(closure -> :"
			"			(Int32 z = (Double x))"
			"			(result = z)"
			"		)"
			"	)"
			"	(g)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestCaptureInFunctionAllInClosure");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 34);
	}

	void TestEssentialInterface(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type.Formatters)"
			"(using Sys.Type)"

			"(interface Sys.IDog (attribute essential)"
			"    (Id -> (Int32 id))"
			")"

			"(function Main -> (Int32 result): "
			"	(Sys.IDog dog)"
			"   (result = 0)"
			"	(try (dog.Id -> result)"
			"	 catch e ( Sys.Print e.Message -> result)"
			"	)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x > 0);
	}

	void TestInterfaceForNull(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type.Formatters)"
			"(using Sys.Type)"

			"(interface Sys.IDog (attribute essential)"
			"    (Id -> (Int32 id))"
			")"

			"(function Main -> (Int32 result): "
			"	(Sys.IDog dog)"
			"   (if (dog ?)"
			"		(result = 7)"
			"	else"
			"		(result = 9)"
			"	)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 9);
	}

	void TestFactoryReturnEmpty(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(interface Sys.IDog (attribute essential)"
			"    (Id -> (Int32 id))"
			")"

			"(factory Sys.NullDog Sys.IDog :"
			"    (return)"
			")"

			"(function Main -> (Int32 result): "
			"	(Sys.IDog dog (Sys.NullDog))"
			"   (if (dog ?)"
			"		(result = 7)"
			"	else"
			"		(result = 9)"
			"	)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 9);
	}

	void TestCaptureStruct(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"

			"(using Sys.Maths)"
			"(using Sys.Reflection)"
			"(using Sys.Type.Formatters)"
			"(using Sys.Type)"

			"(archetype Sys.VoidToInt -> (Int32 result))"

			"(function Main -> (Int32 result): "
			"	(Vec3i p = 17 19 23)"
			"	(VoidToInt g ="
			"		(closure -> (Int32 result):"
			"			(Vec3i q = p)"
			"			(result = p.x)"
			"		)"
			"	)"
			"	(g -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 17);
	}

	void TestDerivedInterfaces(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		"	(alias Main EntryPoint.Main)"

		"(using EntryPoint)"

		"(interface EntryPoint.IDog"
		"	(Bark -> (Int32 value))"
		")"

		"(interface EntryPoint.IRobotDog"
		"	(extends EntryPoint.IDog)"
		"	(Powerup -> (Int32 value))"
		")"
		""
		"(class Doggy (implements IRobotDog))"

		"(method Doggy.Powerup -> (Int32 value): (value = 5))"
		"(method Doggy.Bark -> (Int32 value): (value = 12))"
		"(method Doggy.Construct : )"

		"(factory EntryPoint.NewDoggy IRobotDog : (construct Doggy))"
   
		"(function Main -> (Int32 result):"
		"	(IRobotDog dog (NewDoggy))"
		"	(result = ((dog.Bark) + (dog.Powerup)))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDerivedInterfaces");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 17);
	}

	void TestDerivedInterfaces2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		"	(alias Main EntryPoint.Main)"

		"(using EntryPoint)"

		"(interface EntryPoint.IDog"
		"	(Bark -> (Int32 value))"
		")"

		"(interface EntryPoint.IRobotDog"
		"	(extends EntryPoint.IDog)"
		"	(Powerup -> (Int32 value))"
		")"
		""
		"(class Doggy (implements IRobotDog))"

		"(method Doggy.Powerup -> (Int32 value): (value = 5))"
		"(method Doggy.Bark -> (Int32 value): (value = 12))"

		"(method Doggy.Construct : )"

		"(factory EntryPoint.NewDoggy IRobotDog : (construct Doggy))"
   
		"(function Main -> (Int32 result):"
		"	(IRobotDog dog (NewDoggy))"
		"	(cast dog -> EntryPoint.IDog k9)"
		"	(result = ((k9.Bark) + (dog.Powerup)))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDerivedInterfaces2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 17);
	}

	void TestSearchSubstring(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Maths)"
		"(using Sys.Reflection)"
		"(using Sys.Type.Formatters)"
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(IString opera = \"The Magic Flute\")"
		"	(IString instrument = \"Flute\")"
		"	(result = (Strings.FindLeftNoCase opera 4 instrument))" // the substring occurs at position 10 in the containing string
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestSearchSubstring");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 10);
	}

	void TestRightSearchSubstring(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Maths)"
		"(using Sys.Reflection)"
		"(using Sys.Type.Formatters)"
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(IString musical = \"The tapper\")"
		"	(IString instrument = \"tap\")"
		"	(result = (Strings.FindRightWithCase musical 0 ((musical.Length) - 1) instrument))" // the substring occurs at position 17 in the containing string
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestSearchSubstring");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		printf("%d", x);
		validate(x == 4);
	}

	void TestAppendSubstring(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Maths)"
		"(using Sys.Reflection)"
		"(using Sys.Type.Formatters)"
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(Sys.Type.IStringBuilder s = NewPathBuilder)"
		"	(s.AppendSubstring \"The Magic Flute\" 4 5)"
		"	(s.AppendSubstring \"The Magic Flute\" 3 -1)"
		"	(s.AppendSubstring \"The Magic Flute\" 11 10)"
		"	(Sys.Print s -> result)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAppendSubstring");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 21);
	}

	void TestStringbuilderTruncate(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Maths)"
		"(using Sys.Reflection)"
		"(using Sys.Type.Formatters)"
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(Sys.Type.IStringBuilder s = NewPathBuilder)"
		"	(s \"The Magic Flute\")"
		"	(s.SetLength 8)"
		"	(Sys.Print s -> result)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStringbuilderTruncate");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 8);
	}

	void TestSetCase(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Maths)"
		"(using Sys.Reflection)"
		"(using Sys.Type.Formatters)"
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(Sys.Type.IStringBuilder s = NewPathBuilder)"
		"	(s \"The Magic Flute\")"
		"	(s.ToUpper 0 9)"
		"	(s.ToLower 10 15)"
		"	(Sys.Print s -> result)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestSetCase");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 15);
	}

	void TestStringSplit(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Maths)"
		"(using Sys.Reflection)"
		"(using Sys.Type.Formatters)"
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(Int32 len = 0)"
		"	(IStringBuilder sb = NewPathBuilder)"
		"	(Strings.ForEachSubstring f = "
		"		(closure (IStringBuilder sb)(Int32 index) -> :"
		"			(len = (len + (Sys.Print sb)))"
		"			(sb.SetLength 0)"
		"		)"
		"	)"
		"	(Strings.Split sb \"192.168.1.2\" \".\" f)"
		"	(result = len)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStringSplit");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 8);
	}

	void TestArrayPushViaClosure(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(archetype Sys.VoidToVoid -> )"

			"(function Main -> (Int32 result):"
			"	(array IString items 1)"
			"   (Sys.VoidToVoid f = (closure -> : (items.Push \"Geoff\")))"
			"   (f)"
			"   (IString item = (items 0))"
			"   (result = item.Length)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestArrayPushViaClosure");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 5);
	}

	void TestArrayPushViaClosure3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(archetype Sys.VoidToVoid -> )"

			"(struct People (array IString items))"

			"(method People.Construct :"
			"	(array IString items 1)"
			"   (this.items = items)"
			")"

			"(function AppendToArray (IString name)(array IString items)-> :"
			"	(items.Push name)"
			")"

			"(function Main -> (Int32 result):"
			"	(People people)"
			"   (Sys.VoidToVoid f = (closure -> : (AppendToArray \"Geoff\"  people.items)))"
			"   (f)"
			"   (IString item = (people.items 0))"
			"   (result = item.Length)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 5);
	}

	void TestArrayPushViaClosure2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(archetype Sys.VoidToVoid -> )"

			"(struct People (array IString items))"
			"(method People.Construct :"
			"	(array IString items 1)"
			"   (this.items = items)"
			")"

			"(function Main -> (Int32 result):"
			"	(People people)"
			"   (Sys.VoidToVoid f = (closure -> : (people.items.Push \"Geoff\")))"
			"   (f)"
			"   (IString item = (people.items 0))"
			"   (result = item.Length)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestArrayPushViaClosure2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 5);
	}

	void TestArrayProxy(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(array Int32 a 4)"
			"	(a.Push 36)"
			"   (array Int32 proxy)"
			"   (proxy = a)"
			"	(result = (proxy 0))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestArrayProxy");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 36);
	}

	void TestArrayInt32(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(array Int32 a)"
		"	(result = a.Capacity)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInt32");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestArrayNull(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct Int32Array"
			"	(array Int32 elements)"
			")"

			"(function NullArray (Int32Array a)-> :"
			"	(a.elements.Null)"
			")"

			"(function Main -> (Int32 result):"
			//"	(array Int32 a 2)"
			//"	(a.Null)"
			//"	(a.Null)"
			"	(Int32Array b)"
			"   (NullArray b)"
			"	(result = b.elements.Capacity)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestArrayAssignEmpty(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct Int32Array"
			"	(array Int32 elements)"
			")"

			"(function Main -> (Int32 result):"
			"	(array Int32 a 1)"
			")";
			/*		
			"//	(Int32Array b)"
			"// (b.elements = a)"
			"//	(result = b.elements.Capacity)"
			")";
			*/

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestArrayInt32Expand(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(array Int32 a 1)"
			"	(a.Push 7)"
			"	(a.Push 8)"
			"	(result = a.Capacity)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestArrayInt32Expand");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 2);
	}

	void TestArrayOfArchetypes(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(archetype Sys.OneToOne (Int32 x) -> (Int32 y))"

			"(function DoubleInt (Int32 x) -> (Int32 y): (y = (2 * x)))"

			"(function Main -> (Int32 result):"			
			"	(array Sys.OneToOne a 4)"
			"   (a.Push DoubleInt)"
			"   (Sys.OneToOne f = (a 0))"
			"	(result = (f 17))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestArrayArchetype");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 34);
	}

	void TestArrayInt32_2(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(array Int32 a 4)"
		"	(result = a.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInt32_2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestArrayInt32_3(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(array Int32 a 4)"
		"	(a.Push 1812)"
		"	(result = (a 0))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInt32_3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 1812);
	}

	void TestArrayInt32_4(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(array Int32 a 4)"
		"	(a.Push 1)"
		"	(a.Push 2)"
		"	(a.Push 3)"
		"	(a.Push 4)"
		"	(result = (a 3))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInt32_4");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 4);
	}

	void TestArrayInt32Reassign(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(array Int32 a 1)"
			"	(a.Push 2492)"
			"	(a.Set 0 1470)"
			"	(result = (a 0))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestArrayInt32Reassign");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 1470);
	}

	void TestArrayInt32_5(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(array Int32 a 4)"
		"	(a.Push 2492)"
		"	(a.Push 1232)"
		"	(a.Set 1 1470)"
		"	(a.Push 1132)"
		"	(result = (a 1))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInt32_5");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 1470);
	}

	void TestArrayInt32_6(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(array Int32 a 4)"
		"	(a.Push 2492)"
		"	(Int32 x = a.Length)"
		"	(a.Pop)"
		"	(result = (x + a.Length))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInt32_6");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 1);
	}

	void TestArrayInt32_7(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(array Int32 a 4)"
		"	(a.Push 2492)"
		"	(result = a.PopOut)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInt32_7");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 2492);
	}

	void TestArrayInt32_8(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(function PassThrough (Int32 x) -> (Int32 y):"
		"	(y = x)"
		")"
  
		"(function Main -> (Int32 result):"
		"	(array Int32 a 4)"
		"	(a.Push 2492)"
		"	(result = (PassThrough (a 0)))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInt32_8");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 2492);
	}

	void TestArrayFloat64(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(function PassThrough (Float64 x) -> (Float64 y):"
		"	(y = x)"
		")"
  
		"(function Main -> (Int32 result):"
		"	(array Float64 a 4)"
		"	(a.Push 2492.7)"
		"	(Sys.Type.IStringBuilder sb = NewTokenBuilder)"
		"   (sb.SetFormat 1 4 false false)"
		"	(sb Formatters.SpecF)"
		"	(sb (a 0))"
		"	(Sys.Print sb)"
		"	(result = sb.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayFloat64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 6);
	}

	void TestThrowInConstructor(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			"(using Sys)"

			"(using Sys.Type)"

			"(class Cat (defines Sys.ICat))"
			"(method Cat.Construct : (Throw 0 \"Boo\"))"
			"(factory Sys.NewCat Sys.ICat : (construct Cat))"

			"(function Main -> (Int32 result):"
			"	(Sys.ICat cat (Sys.NewCat))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);
		s_logger.Clear();
	}

	void TestArrayInt32_9(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
 
		"(function Main -> (Int32 result):"
		"	(array Int32 a 4)"
		"	(a.Set 3 2492)"
		"	(result = a.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInt32_9");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 4);
	}

	void TestArrayStruct(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Vec3 (Float32 x y z))"
 
		"(function Main -> (Int32 result):"
		"	(array Vec3 a 4)"
		"	(Vec3 v = 1 2 3)"
		"	(a.Push v)"
		"	(result = a.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayStruct");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 1);
	}

	void TestArrayStruct_2(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Vec3i (Int32 x y z))"
 
		"(function Main -> (Int32 result):"
		"	(array Vec3i a 4)"
		"	(Vec3i v = 1 2 3)"
		"	(a.Push v)"
		"	(Vec3i w = (a 0))"
		"	(Vec3i w0)"
		"	(w0 = (a 0))"	
		"	(result = w.z)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 3);
	}

	void TestArrayStruct_3(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Vec3i (Int32 x y z))"
 
		"(function Main -> (Int32 result):"
		"	(array Vec3i a 4)"
		"	(Vec3i v = 1 2 3)"
		"	(a.Set 3 v)"
		"	(Vec3i w)"
		"	(w = (a 3))"
		"	(result = w.y)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayStruct_2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 2);
	}

	void TestGetSysMessage(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(IString boo = \"boo boo\")"
		"	(IString message = (Sys.Type.GetSysMessage(boo.Buffer)))"
		"	(result = (message.Length))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestGetSysMessage");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 7);
	}

	void TestInternalDestructorsCalled(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(interface Sys.Type.IDog (Bark -> ))"
		"(class Dog (implements Sys.Type.IDog))"
		"(method Dog.Bark -> : )"
		"(factory Sys.Type.BitchSpawn Sys.Type.IDog : (construct Dog))"

		"(method Dog.Construct -> : (Sys.Print \"Dog created\"))"
		"(method Dog.Destruct -> : (Sys.Print \"Dog destroyed\"))"

		"(struct Zoo (Int32 zookeeperId) (IDog dog))"
  
		"(function Main -> (Int32 result):"
		"	(Zoo zoo)"
		"	(zoo.dog = (Sys.Type.BitchSpawn))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestInternalDestructorsCalled");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestInternalDestructorsCalled2(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(interface Sys.Type.IDog (Bark -> ))"
		"(class Dog (implements Sys.Type.IDog))"
		"(method Dog.Bark -> : )"
		"(factory Sys.Type.BitchSpawn Sys.Type.IDog : (construct Dog))"

		"(method Dog.Construct -> : (Sys.Print \"Dog created\"))"
		"(method Dog.Destruct -> : (Sys.Print \"Dog destroyed\"))"

		"(struct Zoo (Int32 zookeeperId) (IDog dog))"
  
		"(function Main -> (Int32 result):"
		"	(try"
		"		("
		"			(Zoo zoo)"
		"			(zoo.dog = (Sys.Type.BitchSpawn))"
		"			(Sys.Throw -1 \"Test\")"
		"		)"
		"	catch ex"
		"		("
		"		)"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestInternalDestructorsCalled2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestArrayRef(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(function FillArray (array Int32 a) -> : "
		"	(a.Set 3 7)"
		")"
  
		"(function Main -> (Int32 result):"
		"	(array Int32 a 4)"
		"	(FillArray a)"
		"	(result = (a 3))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayRef");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 7);
	}

	void TestReturnArrayRef(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function GetArray -> (array Int32 result):"
			"	(array Int32 temp 4)"
			"	(temp.Push 5)"
			"	(result = temp)"
			")"

			"(function Main -> (Int32 result):"
			"	(array Int32 a = (GetArray))"
			"	(result = (a 0))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestReturnArrayRef");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 5);
	}

	void TestReturnArrayRefAndIgnore(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function GetArray -> (array Int32 result):"
			"	(array Int32 temp 4)"
			"	(temp.Push 5)"
			"	(result = temp)"
			")"

			"(function Main -> (Int32 result):"
			"	(GetArray)"
			"	(result = 7)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestReturnArrayRefAndIgnore");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		UNUSED(result);
		ParseException ex;
		validate(s_logger.TryGetNextException(ex));
		validate(strstr(ex.Message(), "Expecting assignment") != nullptr);
	}

	void TestArrayStrongTyping(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(function FillArray (array Int32 a) -> : "
		"	(a 3 7)"
		")"
  
		"(function Main -> (Int32 result):"
		"	(array Float32 a 4)"
		"	(FillArray a)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayStrongTyping");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		UNUSED(result);
		ParseException ex;
		if (!s_logger.TryGetNextException(ex))
			validate(false);
	}

	void TestArrayStrongTyping2(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(interface Sys.Type.IDog (Bark (array Int32 barkAngles) ->))"
		"(class Rover (implements Sys.Type.IDog))"
		"(method Rover.Bark (array Float32 barkAngles) -> :)"
  
		"(function Main -> (Int32 result):"
		"	(Rover rover)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayStrongTyping2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());	
			UNUSED(vm);
			validate(false);	
		}
		catch(ParseException&)
		{
		}		
	}

	void TestArrayStrongTyping3(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(interface Sys.Type.IDog (Bark (array Int32 barkAngles) ->))"
		"(class Rover (implements Sys.Type.IDog))"
		"(method Rover.Bark (array Int32 barkAngles) -> :)"
  
		"(function Main -> (Int32 result):"
		"	(Rover rover)"
		"	(array Float32 a 4)"
		"	(rover.Bark a)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayStrongTyping3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		UNUSED(result);
		ParseException ex;
		if (!s_logger.TryGetNextException(ex))
			validate(false);
	}

	void TestArrayInStruct(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct DogKennel (array Int32 dogIds))"

		"(method DogKennel.Construct (Int32 capacity) ->"
		"	(construct dogIds capacity): "			
		")"
  
		"(function Main -> (Int32 result):"
		"	(DogKennel kennel (4))"
		"	(kennel.dogIds.Push 1812)"
		"	(result = (kennel.dogIds 0))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInStruct");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 1812);
	}

	void TestArrayClear(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(array Int32 ids 4)"
			"	(ids.Push 1812)"
			"	(ids.Clear)"
			"   (result = ids.Length)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestArrayInStruct2(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct DogKennel (array Int32 dogIds))"
		"(struct Sanctuary (DogKennel kennel))"

		"(method DogKennel.Construct (Int32 capacity)"
		"	-> (construct dogIds capacity) "	
		"	:"
		")"

		"(method Sanctuary.Construct (Int32 kennelCapacity)"
		"	-> (construct kennel kennelCapacity)"		
		"	:"
		")"
  
		"(function Main -> (Int32 result):"
		"	(Sanctuary sanctuary (4) )"
		"	(sanctuary.kennel.dogIds.Push 1812)"
		"	(result = (sanctuary.kennel.dogIds 0))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInStruct2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 1812);
	}

	void TestConstructInArray(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct DogKennel (Int32 dogId))"
		"(struct Sanctuary (array DogKennel kennels))"

		"(method DogKennel.Construct (Int32 id): "
		"	(this.dogId = id)"
		")"

		"(method Sanctuary.Construct (Int32 maxKennels):"
		"	(array DogKennel kennels maxKennels)"
		"   (this.kennels = kennels)"
		")"

		"(function Main -> (Int32 result):"
		"	(Sanctuary sanctuary (4) )"
		"	(sanctuary.kennels.Push 1812)" // This should invoke the DogKennel constructor into the memory slot of the first array element
		"	(result = (sanctuary.kennels 0 dogId))" // Grab dogId from sanctuary.kennels(0)
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestConstructInArray");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1812);
	}

	void TestClamp1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(result = (Sys.Maths.I32.Clamp -1 2 5))" 
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 2);
	}

	void TestClamp2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(result = (Sys.Maths.I32.Clamp 6 2 5))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 5);
	}

	void TestClamp3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(result = (Sys.Maths.I32.Clamp 4 2 5))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestArrayInStructWithThis(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct DogKennel (array Int32 dogIds))"

			"(method DogKennel.Construct (Int32 capacity):"
			"	(array Int32 dogIds capacity) "
			"	(this.dogIds = dogIds)"
			")"

			"(function Main -> (Int32 result):"
			"	(DogKennel kennel ( 5 ))" //
			"   (kennel.dogIds.Push 1812)"
			"   (result = (kennel.dogIds 0))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestArrayInStructWithThis");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 1812);
	}

	void TestArrayForeachWithinForEach(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct DogKennel (array Int32 dogIds))"
		"(struct Sanctuary (array DogKennel kennels))"

		"(method DogKennel.Construct (Int32 capacity):"
		"	(array Int32 dogIds capacity) "	
		"	(this.dogIds = dogIds)"
		")"

		"(method Sanctuary.Construct (Int32 kennelCapacity):"
		"	(array DogKennel kennels kennelCapacity) "
		"	(this.kennels = kennels)"		
		")"
  
		"(function Main -> (Int32 result):"
		"	(Sanctuary sanctuary (4) )" // Create a sanctuary with a capacity of 4 kennels
		"	(sanctuary.kennels.Push 4)" // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		"	(sanctuary.kennels.Push 4)" // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		"	(sanctuary.kennels.Push 4)" // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		"	(sanctuary.kennels.Push 4)" // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		"	(foreach k # sanctuary.kennels" // Enumerate over all 4 kennesl
		"		(k.dogIds.Push 1)" // Put a new dog id in the kennel
		"		(k.dogIds.Push 2)" // Put a new dog id in the kennel
		"		(k.dogIds.Push 3)" // Put a new dog id in the kennel
		"		(k.dogIds.Push 4)" // Put a new dog id in the kennel
		"		(foreach d # k.dogIds" // Enumerate through kennel
		"			(result += d)" // Sum the dogid to result
		"		)" // Sum the dogid to result
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayForeachWithinForEach");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 40);
	}

	void TestArrayForeachAndThrow(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Sanctuary (array Int32 kennelIds))"

		"(method Sanctuary.Construct (Int32 capacity):"
		"	(array Int32 kennelIds capacity)"		
		"	(this.kennelIds = kennelIds)"
		")"
  
		"(function Main -> (Int32 result):"
		"	(Sanctuary sanctuary (10) )" // Create a sanctuary with a capacity of 10 kennels
		"	(sanctuary.kennelIds.Set 9 0)" // Define and null the ten entries by setting entry 9 to 0

		"	(try"
		"		("
		"			(foreach k # sanctuary.kennelIds" // Enumerate over all 10 kennels
		"				(Sys.Throw -1 \"Test: foreach throw\")"
		"			)"
		"		)"
		"	catch ex"
		"		("
		"			(Sys.Print ex.Message)"
		"			(result = 1999)"
		"		)"
		"	)"
		"	(sanctuary.kennelIds.Pop)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayForeachAndThrow");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 1999);
	}

	void TestArrayForeachAndThrow2(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Sanctuary (array Int32 kennelIds))"

		"(method Sanctuary.Construct (Int32 capacity)"
		"	-> (construct kennelIds capacity)"		
		"	:"
		")"
  
		"(function Main -> (Int32 result):"
		"	(Sanctuary sanctuary (10) )" // Create a sanctuary with a capacity of 10 kennels
		"	(sanctuary.kennelIds 9 0)" // Define and null the ten entries by setting entry 9 to 0

		"	(foreach i k # (sanctuary.kennelIds 0 9)" // Enumerate over all 10 kennels
		"		(try"
		"			("
		"				(Sys.Throw -1 \"Test: foreach2 throw\")"
		"			)"
		"		catch ex"
		"			("
		"				(result = (result + i))" // Sum the index to result")
		"			)"
		"		)"		
		"	)"		
		"	(sanctuary.kennelIds.Pop)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayForeachAndThrow2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 45);
	}

	void TestArrayLast(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(array Int32 a (10) )"
			"	(a.Push 10)"
			"	(a.Push 19)"
			"	(foreach k # (a @last)"
			"		(result += k)"
			"	)"
			"	(foreach k # (a 0)"
			"		(result += k)"
			"	)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 29);
	}

	void TestArrayForeachElementInArray(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
 
		"(function Main -> (Int32 result):"
		"	(array Int32 a (10) )"
		"	(for (Int32 i = 0) (i < 10) (i += 1)"
		"		(a.Push (i + 10))"
		"	)"

		"	(foreach k # a"
		"		(result += k)"
		"	)"		
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayForeachElementInArray");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 145);
	}

	void TestArrayForeachEachElementInArrayWithoutIndex(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
 
		"(function Main -> (Int32 result):"
		"	(array Int32 a (10) )"
		"	(for (Int32 i = 0) (i < 10) (i += 1)"
		"		(a.Push (i + 10))"
		"	)"

		"	(foreach k # a"
		"		(result += k)"
		"	)"		
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayForeachEachElementInArrayWithoutIndex");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 145);
	}

	void TestArrayElementIsClass(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(class Test (defines Sys.ITest) (Int32 id))"
			"(method Test.Construct (Int32 id): (this.id = id))"
			"(method Test.Id -> (Int32 id): (id = this.id))"
			"(method Test.Destruct -> : (Sys.Print \"Test finished\"))"
			"(factory Sys.NewTest Sys.ITest (Int32 id) : (construct Test id))"

			"(function Main -> (Int32 result):"
			"	(array Test a (2) )"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);
		s_logger.Clear();
	}

	void TestArrayElementDeconstruct(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"

		"(using Sys.Type)"

		"(class Test (defines Sys.ITest) (Int32 id))"
		"(method Test.Construct (Int32 id): (this.id = id))"
		"(method Test.Id -> (Int32 id): (id = this.id))"
		"(method Test.Destruct -> : (Sys.Print \"Test finished\"))"
		"(factory Sys.NewTest Sys.ITest (Int32 id) : (construct Test id))"

		"(function Main -> (Int32 result):"
		"	(array Sys.ITest a 2)"
		"	(Sys.ITest e0 (Sys.NewTest 7))"
		"	(Sys.ITest e1 (Sys.NewTest 12))"
		"   (a.Push e0)"
		"   (a.Push e1)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayElementDeconstruct");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestArrayWithinArrayDeconstruct(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)\n"
		" (alias Main EntryPoint.Main)\n"
  
		"(using Sys.Type)\n"

		"(class Test (defines Sys.ITest) (Int32 id))\n"
		"(method Test.Construct (Int32 id): (this.id = id))\n"
		"(method Test.Id -> (Int32 id): (id = this.id))\n"
		"(method Test.Destruct -> : (Sys.Print \"Test destructed\"))\n"
		"(factory Sys.NewTest Sys.ITest (Int32 id) : (construct Test id))\n"

		"(struct Axis (array Sys.ITest tests))\n"
		"(method Axis.Construct (Int32 testsPerAxis):"
		"	(array Sys.ITest tests testsPerAxis)"
		"   (this.tests = tests)"
		")"
		"(function Main -> (Int32 result):\n"
		"	(array Axis axes 3 )\n"
		"	(axes.Push 3)\n" // N.B 3 here is the argument passed to Axis.Construct, which sets the testPerAxis value to 3
		"	(axes.Push 3)\n"
		"	(axes.Push 3)\n"
		"	(foreach axis # axes\n "
		"		(Sys.ITest e0 (Sys.NewTest 1))\n"
		"		(Sys.ITest e1 (Sys.NewTest 1))\n"
		"		(Sys.ITest e2 (Sys.NewTest 1))\n"
		"		(axis.tests.Push e0)\n"
		"		(axis.tests.Push e1)\n"
		"		(axis.tests.Push e2)\n"
		"	)\n"
		")\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayWithinArrayDeconstruct");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestArrayElementDeconstructWhenThrown(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(class Test (defines Sys.ITest) (Int32 id))"
		"(method Test.Construct (Int32 id): (this.id = id))"
		"(method Test.Id -> (Int32 id): (id = this.id))"
		"(method Test.Destruct -> : (Sys.Print \"&nDestructor for class Test called&n\"))"
		"(factory Sys.NewTest Sys.ITest (Int32 id) : (construct Test id))"

		"(function Main2 -> (Int32 result):"
		"	(array Sys.ITest a 2 )"
		"   (Sys.ITest e0 (Sys.NewTest 7))"
		"   (a.Push e0)"
		"	(Sys.Throw 747 \"This should trigger the autodestruct sequence\")"
		")"
 
		"(function Main -> (Int32 result):"
		"	(try"
		"		("
		"			(Main2 -> result)"
		"		)"
		"	catch ex"
		"		("
		"			(result = ex.ErrorCode)"
		"		)"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayElementDeconstructWhenThrown");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 747);
	}

	void TestArrayEmptyForEach(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(function Main -> (Int32 result):"
			"	(array Vec4 vectors 5)"
			"	(foreach v # vectors"
			"		(result = 7)"
			"	)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestLambda(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"
			"(using Sys.Maths)"

			"(archetype Sys.Type.OnNameStringValueInt32Enum (IString name)(Int32 value) -> (Bool continueEnumeration))"

			"(function ForEachChessPiece (Int32 delta) (Sys.Type.OnNameStringValueInt32Enum callback) -> (Int32 numberOfPieces): "
			"    (callback \"queen\" (10 + delta))"
			"    (callback \"rook\" (5 + delta))"
			"    (callback \"bishop\" (4 + delta))"
			"    (callback \"knight\" (3 + delta))"
			"    (callback \"pawn\" (1 + delta))"
			"    (numberOfPieces = 5)"
			")"

			"(function Main -> (Int32 result):"
			"	(ForEachChessPiece 7 "
			"		(name value -> continueEnumeration : "
			"			(result += value)"
			"			(continueEnumeration = true)"
			"		)"
			"	)"
			")";

			// This is semantically equivalent to 
			/*
			* 
			* (Sys.Type.OnNameStringValueInt32Enum callback = 
				(closure (String key)(Int32 value) -> (Bool continueEnumeration):
					(result += value)
					(continueEnumeration = true)
			*	)
			* )
			* 
			* (ForEachChessPiece callback)
			* 
			* */

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 58);
	}

	void TestArrayElementLockRef(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Vec4 (Int32 x y z w))"
 
		"(function Main -> (Int32 result):"
		"	(array Vec4 vectors 5)"
		"	(vectors.Push Vec4 (1 2 3 0))"
		"	(foreach v # (vectors 0)"
		"		(result = v.z)"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayElementLockRef");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 3);
	}

	void TestLinkedListContained(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(struct ListInt32"
			"	(list Int32 elements)"
			")"

			"(namespace EntryPoint)"
			"	(alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"
			"	(ListInt32 a)"
			"	(if (a.elements.Length == 0) (result = 7) )"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 7);
	}

	void TestLinkedListNodeInline(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(struct ListInt32"
			"	(list Int32 elements)"
			")"

			"(namespace EntryPoint)"
			"	(alias Main EntryPoint.Main)"

			"(function Main -> (Int32 result):"
			"	(list Int32 elements)"
			"	(elements.Append 7)"
			"	(node n = elements.Head)"
			"	(if (n.Value == 7)"
			"		(result = 3)"
			"	)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 3);
	}

	void TestLinkedList(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(list Int32 a)"
		"	(result = a.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedList");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestLinkedList2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(list Int32 a)"
		"	(a.Append 25)"
		"	(result = a.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedList2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 1);
	}

	void TestLinkedList3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(list Int32 a)"
		"	(a.Append 25)"
		"	(node n = a.Tail)"
		"	(result = (n.Value + a.Length))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedList3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 26);
	}

	void TestLinkedList4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(list Int32 a)"
		"	(a.Prepend 25)"
		"	(node n = a.Head)"
		"	(result = ((3 + n.Value) + (a.Length + 2)))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedList4");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 31);
	}

	void TestLinkedList6(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(list Int32 a)"
		"	(try"
		"		("
		"			(node n = a.Head)"
		"			(result = n.Value)"
		"		)"
		"	catch ex"
		"		("
		"			(result = 39)"
		"			(Sys.Print ex.Message)"
		"		)"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedList6");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 39);
	}

	void TestLinkedList7(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(list Int32 a)"
		"	(a.Prepend 17)"
		"	(a.Prepend 34)"
		"	(node head = a.Head)"
		"	(node tail = head.Next)"
		"	(result = tail.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedList7");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 17);
	}

	void TestLinkedList8(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(list Int32 a)"
		"	(a.Append 17)"
		"	(a.Append 34)"
		"	(node tail = a.Tail)"
		"	(node head = tail.Previous)"
		"	(result = head.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedList8");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 17);
	}

	void TestLinkedList9(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(list Int32 a)"
		"	(a.Append 17)"
		"	(a.Append 34)"
		"	(node head = a.Head)"
		"	(head.Append 32)" // This inserts an element 32 between 17 and 34
		"	(node tail = a.Tail)"
		"	(node newNode = tail.Previous)"
		"	(result = newNode.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedList9");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 32);
	}

	void TestLinkedList10(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(list Int32 a)"
		"	(a.Append 17)"
		"	(a.Append 34)"
		"	(node n = a.Tail)"
		"	(n.Pop)"
		"	(node tail = a.Tail)"
		"	(result = tail.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedList10");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 17);
	}

	void TestLinkedList11(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct Int32List (list Int32 elements))"

			"(function Main -> (Int32 result):"
			"	(Int32List stuff)"
			"	(list Int32 a)"
			"   (stuff.elements = a)"
			"	(stuff.elements.Append 17)"
			"	(stuff.elements.Prepend 34)"
			"	(node tail = stuff.elements.Tail)"
			"	(result = (result + tail.Value))"
			"	(foreach n # stuff.elements (result = (result + n.Value)))"
			"	(result = (result + stuff.elements.Length))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestLinkedList11");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 70);
	}

	void TestLeakyLinkedList(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(function Main -> (Int32 result):"
		"	(list Int32 a)"
		"	(a.Append 17)"
		"	(foreach n # a"
		"		(result = 255)"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __func__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 255);
	}

	void TestLinkedListOfArchetypes(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(archetype Sys.OneToOne (Int32 x) -> (Int32 y))"

			"(function DoubleInt (Int32 x) -> (Int32 y): (y = (2 * x)))"

			"(function Main -> (Int32 result):"
			"	(list Sys.OneToOne a)"
			"	(a.Append DoubleInt)"
			"	(node head = a.Head)"
			"	(Sys.OneToOne f = head.Value)"
			"   (result = (f 17))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestLinkedListOfArchetypes");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 34);
	}

	void TestLinkedList12(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(function AppendThree (list Int32 a) -> : "
		"	(a.Prepend 17)"
		"	(a.Prepend 34)"
		"	(a.Prepend 9)"
		")"
  
		"(function Main -> (Int32 result):"
		"	(list Int32 a)"
		"	(AppendThree a)"
		"	(node head = a.Head)"
		"	(result = head.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedList12");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 9);
	}

	void TestLinkedListOfInterfaces(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		R"(
			(namespace EntryPoint)
			 (alias Main EntryPoint.Main)

			(using Sys.Type)

			(class Cat (defines Sys.ICat))

			(method Cat.Construct : )

			(method Cat.Id -> (Int32 id): (id = 5))

			(factory Sys.NewCat Sys.ICat : 
				(construct Cat)
			)

			(function Main -> (Int32 result):
				(Sys.ICat cat (Sys.NewCat))
				(list Sys.ICat cats)
				(cats.Append cat)
				(node tail = cats.Tail)
				(Sys.ICat tiddles = tail.Value)
				(tiddles.Id -> result)
			)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 5);
	}

	void TestLinkedListOfInterfaces2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(class Cat (defines Sys.ICat) (Int32 id))"
			"(method Cat.Construct (Int32 id): (this.id = id))"
			"(method Cat.Id -> (Int32 id): (id = this.id))"
			"(factory Sys.NewCat Sys.ICat (Int32 id) : (construct Cat id))"

			"(function Main -> (Int32 result):"
			"	(Sys.ICat albert (Sys.NewCat 8))"
			"	(Sys.ICat bob (Sys.NewCat 12))"
			"	(list Sys.ICat cats)"
			"	(cats.Append albert)"
			"   (node tail = cats.Tail)"
			"   (tail.Append bob)"
			"   (node newTail = cats.Tail)"
			"   (Sys.ICat favouriteCat = newTail.Value)"
			"   (favouriteCat.Id -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 12);
	}

	void TestLinkedListOfLists(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Dog (list Int32 boneIds))"

		"(method Dog.Construct -> : "
			"(list Int32 boneIds)"
			"(this.boneIds = boneIds)"
		")"
  
		"(function Main -> (Int32 result):"
		"	(list Dog dogs)"
		"	(dogs.Append ())"
		"	(node head = dogs.Head)"
		"	(Dog firstDog = & head)"
		"	(firstDog.boneIds.Append 27)"
		"	(node bone = firstDog.boneIds.Head)"
		"	(result = bone.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedListOfLists");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 27);
	}

	void TestLinkedListForeach1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"

		"	(list Int32 a)"

		"	(a.Append 17)"
		"	(a.Append 34)"
		"	(a.Append 333)"

		"	(foreach i n # a "
		"		(result = (i + (result + n.Value)))"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedListForeach1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 387);
	}

	void TestLinkedListForeach2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"

		"	(list Int32 a)"

		"	(a.Append 17)"
		"	(a.Append 34)"
		"	(a.Append 333)"

		"	(foreach n # a "
		"		(result = (result + n.Value))"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedListForeach2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 384);
	}

	void TestLinkedListForeach3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"

		"	(list Int32 a)"

		"	(a.Append 17)"
		"	(a.Append 34)"
		"	(a.Append 333)"

		"	(a.Clear)"

		"	(foreach n # a "
		"	)"
		"	(result = a.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedListForeach3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestLinkedListForeach4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"

		"	(list Int32 a)"

		"	(a.Append 17)"
		"	(a.Append 34)"
		"	(a.Append 333)"

		"	(foreach n # a "
		"		(n.Pop)"
		"	)"
		"	(result = a.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedListForeach4");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 2);
	}

	void TestLinkedListForeach5(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"

		"	(list Int32 a)"

		"	(a.Append 17)"
		"	(a.Append 34)"
		"	(a.Append 333)"

		"	(foreach n # a "
		"		(result = (result + 1))"
		"		(a.Clear)"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedListForeach5");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 1);
	}

	void TestLinkedListForeach6(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		R"(
(namespace EntryPoint)
	(alias Main EntryPoint.Main)
  
(using Sys.Type)
  
(function Main -> (Int32 result):

	(list Int32 a)

	(a.Append 17)
	(a.Append 34)
	(a.Append 333)

	(foreach n # a
		(try
			(
				(Sys.Throw n.Value "thrown in foreach ")
			)
		catch ex
			(
				(result += 1)
				(Sys.Print ex.Message)
			)
		)
		(result += 10)
	)
))";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 33);
	}

	void TestLinkedListForeach7(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"

		"	(list Int32 a)"

		"	(a.Append 17)"
		"	(a.Append 34)"
		"	(a.Append 333)"

		"	(try"
		"		("
		"			(foreach n # a (Sys.Throw n.Value \"thrown in foreach\"))"
		"		)"
		"	catch ex"
		"		("
		"			(result = (result + 1))"
		"			(Sys.Print ex.Message)"
		"		)"
		"	)"
		"	(result = (result + 10))"

		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedListForeach7");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 11);
	}

	void TestLinkedListForeach8(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"

			"	(list Int32 a)"

			"	(a.Append 17)"

			"	(foreach n # a"
			"		(result = n.Value)"
			"		(break)"
			"	)"	
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 17);
	}

	void TestStaticCast(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			R"sexy(
			(namespace EntryPoint)
			   (alias Main EntryPoint.Main)
			   (using Sys.Type)

			(interface Sys.I1 (GetI2 -> (Sys.I2 i2)))
			(interface Sys.I2 (Get6 -> (Int32 value)))

			(class Apple (implements Sys.I1)(implements Sys.I2))
			(method Apple.Construct : )
			(factory Sys.NewApple Sys.I1 : (construct Apple))
			(method Apple.GetI2 -> (Sys.I2 i2): (i2 = this))
			(method Apple.Get6 -> (Int32 value): (value = 6))

			(function Main -> (Int32 result):
				(Sys.I1 apple (Sys.NewApple))
				(Sys.I2 i2 = apple.GetI2)
				(result = i2.Get6)
			)
			)sexy";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 6);
	}

	void TestListStruct(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Vec3 (Float32 x y z))"
 
		"(function Main -> (Int32 result):"
		"	(list Vec3 a)"
		"	(Vec3 v = 1 2 3)"
		"	(a.Append v)"
		"	(result = a.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestListStruct");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 1);
	}

	void TestListStruct2(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Vec3 (Int32 x y z))"
 
		"(function Main -> (Int32 result):"
		"	(list Vec3 a)"
		"	(Vec3 v = 1 2 3)"
		"	(a.Append v)"
		"	(node n = a.Head)"
		"	(Vec3 val = & n)"
		"	(result = val.z)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestListStruct2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 3);
	}

	void TestListStruct3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(class Cat (defines Sys.ICat))"
			"(method Cat.Construct : )"
			"(method Cat.Id -> (Int32 id): (id = 5))"
			"(factory Sys.NewCat Sys.ICat : (construct Cat))"

			"(struct CatAndPos (Int32 x y z) (Sys.ICat cat))"

			"(function Main -> (Int32 result):"
			"   (Sys.ICat teddy (Sys.NewCat))"
			"	(list CatAndPos a)"
			"	(CatAndPos v = 1 2 3 teddy)"
			"	(a.Append v)"
			"	(node n = a.Head)"
			"	(CatAndPos val = & n)"
			"	(result = val.cat.Id)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 5);
	}

	void TestListStrongTyping(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(function FillArray (list Int32 a) -> : "
		"	(a.Append 37)"
		")"
  
		"(function Main -> (Int32 result):"
		"	(list Float32 a)"
		"	(FillArray a)"
		"	(node n = a.Head)"
		"	(result = n.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestListStrongTyping");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		UNUSED(result);
		ParseException ex;
		if (!s_logger.TryGetNextException(ex))
			validate(false);
	}

	void TestStructWithInterface(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(class Cat (defines Sys.ICat))"
			"(method Cat.Construct : )"
			"(method Cat.Id -> (Int32 id): (id = 5))"
			"(factory Sys.NewCat Sys.ICat : (construct Cat))"

			"(struct CatAndPos (Int32 x y z) (Sys.ICat cat))"

			"(function Main -> (Int32 result):"
			"   (Sys.ICat teddy (Sys.NewCat))"
			"	(CatAndPos v = 1 2 3 teddy)"
			"   (v.cat = teddy)"
			"   (v.cat.Id -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 5);
	}

	void TestStructWithCircularReferences(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(class Dog(defines Sys.IDog) (Sys.ICat cat))"
			"(method Dog.Construct :)"
			"(method Dog.Id -> (Int32 id) : (id = 5))"
			"(method Dog.SetFriend(Sys.ICat cat) -> : (this.cat = cat))"
			"(factory Sys.NewDog Sys.IDog : (construct Dog))"

			"(class Cat(defines Sys.ICat) (Sys.IDog dog))"
			"(method Cat.Construct :)"
			"(method Cat.Id -> (Int32 id) : (id = 5))"
			"(method Cat.SetFriend(Sys.IDog dog) -> : (this.dog = dog))"
			"(factory Sys.NewCat Sys.ICat : (construct Cat))"

			"(function Main -> (Int32 result) :"
			"(Sys.ICat teddy(Sys.NewCat))"
			"	(Sys.IDog rover(Sys.NewDog))"
			"	(teddy.SetFriend rover)"
			"	(rover.SetFriend teddy)"
			"	(teddy.Id -> result)"
			"	)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 5);
	}

	void TestEmptyMap(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct Bunions"
			"	(IString name)"
			"   (array Sys.Maths.Vec3 positions)"
			")"

			"(function Main -> (Int32 result):"
			"	(map Int32 Bunions a)"
			"	(result = a.Length)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestMap(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map IString Int32 a)"
		"	(result = a.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMap");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestMapStringToString(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(map IString IString a)"
			"	(a.Insert \"Joe\" \"90\")"
			"	(node n = (a \"Joe\"))"
			"   (IString value = n.Value)"
			"   (value.Length -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 2);
	}

	void TestMapKey(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(map IString Int32 a)"
			"	(a.Insert \"Joe\" 90)"
			"	(node n = (a \"Joe\"))"
			"   (if n"
			"     (IString key = n.Key)"
			"     (result = key.Length)"
			"   )"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 3);
	}

	void TestMapGetKey(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(map Int32 IString a)"
			"	(a.Insert 90 \"Joe\")"
			"	(node n = (a 90))"
			"   (result = n.Key)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 90);
	}

	void TestMapGetKey64(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Float64 result):"
			"	(map Float64 IString a)"
			"	(a.Insert 90 \"Joe\")"
			"	(node n = (a 90))"
			"   (result = n.Key)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0.0); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		double x = vm.PopFloat64();
		validate(x == 90.0);
	}

	void TestMapOfArchetypes(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(archetype Sys.OneToOne (Int32 x) -> (Int32 y))"

			"(function DoubleInt (Int32 x) -> (Int32 y): (y = (2 * x)))"

			"(function Main -> (Int32 result):"
			"	(map IString Sys.OneToOne a)"
			"	(a.Insert \"Joe\" DoubleInt)"
			"	(node n = (a \"Joe\"))"
			"   (Sys.OneToOne f = n.Value)"
			"	(result = (f 17))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestMapOfArchetypes");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 34);
	}

	void TestStartsWith(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"   (Bool isSo = (Sys.Type.Strings.StartsWith \"Dogmatic\" \"Dog\"))"
			"	(if isSo (result = 57))"
			"	(isSo = (Sys.Type.Strings.StartsWith \"Catastrophe\" \"Mouse\"))"
			"	(if isSo (result += 57))"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 57);
	}

	void TestMap2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map IString Int32 a)"
		"	(a.Insert \"Joe\" 90 )"
		"	(result = a.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMap2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 1);
	}

	void TestMapStringInsertByVariable(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(map IString Int32 a)"
			"	(IString joe = \"Joe\")"
			"	(a.Insert joe 90 )"
			"	(node n = (a joe))"
			"   (result = n.Value)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 90);
	}

		void TestMapStringInsertByVariable2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"	(map IString IString a)"
			"	(IString joe = \"Joe\")"
			"	(a.Insert joe \"cool\" )"
			"	(node n = (a joe))"
			"   (IString value = n.Value)"
			"   (value.Length -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 4);
	}

	void TestMap3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map IString Int32 a)"
		"	(node n = (a \"Joe\"))"
		"	(if (n.Exists)"
		"		(result = 1)"
		"	else"
		"		(result = 2)"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMap3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 2);
	}

	void TestMap4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map IString Int32 a)"
		"	(a.Insert \"Joe\" 90 )"
		"	(node n = (a \"Joe\"))"
		"	(if (n.Exists)"
		"		(result = 3)"
		"	else"
		"		(result = 4)"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMap4");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 3);
	}

	void TestMap5(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map IString Int32 a)"
		"	(node n = (a \"Joe\"))"
		"	(result = n.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMap5");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		UNUSED(result);
		ParseException ex;
		if (!s_logger.TryGetNextException(ex))
			validate(false);
	}

	void TestMap6(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map IString Int32 a)"
		"	(a.Insert \"Joe\" 90)"
		"	(node n = (a \"Joe\"))"
		"	(result = n.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMap6");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 90);
	}

	void TestMap7(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map IString Int32 a)"
		"	(a.Insert \"Joe\" 90)"
		"	(a.Insert \"Alice\" 37)"
		"	(a.Insert \"Fred\" 65)"
		"	(node n = (a \"Joe\"))"
		"	(result = n.Value)"
		"	(node m = (a \"Fred\"))"
		"	(result = (result + m.Value))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMap7");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 155);
	}

	void TestMap8(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct MapStringToInt32"
				"(map IString Int32 elements)"
			")"

			"(function Main -> (Int32 result):"
			""
			"	(MapStringToInt32 a)"
			"	(map IString Int32 elements)"
			"	(a.elements = elements)"
			"	(a.elements.Insert \"Joe\" 90)"
			"	(node n = (a.elements \"Joe\"))"
			"	(result = n.Value)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestMap8");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 90);
	}

	void TestMapOverwriteValue(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map IString Int32 a)"
		"	(a.Insert \"Joe\" 90)"
		"	(a.Insert \"Joe\" 239)"
		"	(node m = (a \"Joe\"))"
		"	(result = m.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapOverwriteValue");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 239);
	}

	void TestMapOverwriteValue64(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map IString Float64 a)"
		"	(a.Insert \"Joe\" 90)"
		"	(a.Insert \"Joe\" 239)"
		"	(node m = (a \"Joe\"))"
		"	(if (m.Value == 239) (result = 1) else (result = 0))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapOverwriteValue64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 1);
	}

	void TestMapIndexedByInt32(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map Int32 Int32 a)"
		"	(a.Insert 45 48)"
		"	(node n = (a 45))"
		"	(result = n.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapIndexedByInt32");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 48);
	}

	void TestMapIndexedByFloat64(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map Float64 Int32 a)"
		"	(a.Insert 45 48)"
		"	(node n = (a 45))"
		"	(result = n.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapIndexedByFloat64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 48);
	}

	void TestDeleteKey(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map Float32 Int32 a)"
		"	(a.Insert 45 48)"
		"	(a.Insert 37 -248)"
		"	(node n = (a 37))"
		"	(n.Pop)"
		"	(node m = (a 45))"
		"	(result = (m.Value + a.Length))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDeleteKey");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 49);
	}

	void TestMapValueInterface(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(class Test (defines Sys.ITest))"
			"(method Test.Construct : )"
			"(method Test.Id -> (Int32 id) : (id = 6) )"
			"(factory Sys.NewTest Sys.ITest : (construct Test))"

			"(function Main -> (Int32 result):"
			"	(map Int32 Sys.ITest a)"
			"	(Sys.ITest t (Sys.NewTest))"
			"	(a.Insert 45 t)"
			"	(node n = (a 45))"
			"	(Sys.ITest value = n.Value)"
			"	(result = value.Id)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 6);
	}

	void TestMapValueStruct0(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct Vec4 (Int32 x y z w))"

			"(function Main -> (Int32 result):"
			"	(map Int32 Vec4 a)"
			"	(a.Insert 45 Vec4 (1 2 3 4))"
			"	(node n = (a 45))"
			"	(Vec4 v = & n)"
			"	(result = v.x)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestMapValueStruct0");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 1);
	}


	void TestMapValueStruct(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(class Test (defines Sys.ITest))"
		"(method Test.Construct : )"
		"(method Test.Id -> (Int32 id) : (id = 6) )"
		"(factory Sys.NewTest Sys.ITest : (construct Test))"

		"(struct Vec4 (Int32 x y z w) (Sys.ITest test))"
  
		"(function Main -> (Int32 result):"
		"	(map Int32 Vec4 a)"
		"	(Sys.ITest t (Sys.NewTest))"
		"	(a.Insert 45 Vec4 (1 2 3 4 t))"
		"	(node n = (a 45))"
		"	(Vec4 v = & n)"
		"	(result = v.test.Id)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapValueStruct");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 6);
	}

	void TestMapValueConstruct(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Vec4 (Int32 x y z w))"
		"(method Vec4.Construct (Int32 x) (Int32 y) (Int32 z) -> : (this.x = x) (this.y = y) (this.z = z) (this.w = 1))" 
  
		"(function Main -> (Int32 result):"
		"	(map Int32 Vec4 a)"
		"	(a.Insert 45 Vec4 (1 2 3 4))"
		"	(node n = (a 45))"
		"	(Vec4 v = & n)"
		"	(result = v.z)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapValueConstruct");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 3);
	}

	void TestMapForeach1(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Vec4 (Int32 x y z w))"
		"(method Vec4.Construct (Int32 x) (Int32 y) (Int32 z) -> : (this.x = x) (this.y = y) (this.z = z) (this.w = 1))" 
  
		"(function Main -> (Int32 result):"
		"	(map Int32 Vec4 a)"
		"	(a.Insert 45 Vec4 (1 2 3))"
		"	(a.Insert 23 Vec4 (5 6 7))"
		"	(a.Insert -6 Vec4 (8 9 10))"
		"	(foreach n # a "
		"		(Vec4 v = & n)"
		"		(result = (result + v.x))"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapForeach1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 14);
	}

	void TestMapInStruct(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Vec4Map (map Int32 Vec4 a))"

		"(struct Vec4 (Int32 x y z w))"
		"(method Vec4.Construct (Int32 x) (Int32 y) (Int32 z) -> : (this.x = x) (this.y = y) (this.z = z) (this.w = 1))" 
  
		"(function Main -> (Int32 result):"
		"	(Vec4Map m)"
		"   (map Int32 Vec4 a)"
		"	(m.a = a)"
		"	(m.a.Insert 45 Vec4 (1 2 3))"
		"	(m.a.Insert 23 Vec4 (5 6 7))"
		"	(m.a.Insert -6 Vec4 (8 9 10))"
		"	(foreach n # m.a "
		"		(Vec4 v = & n)"
		"		(result = (result + v.x))"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapInStruct");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 14);
	}

	void TestMapInMap(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
		
		"(struct Vec4 (Int32 x y z w))"
		"(method Vec4.Construct (Int32 x) (Int32 y) (Int32 z) -> : (this.x = x) (this.y = y) (this.z = z) (this.w = 1))" 

		"(struct Vec4Map (map Int32 Vec4 a))"
		"(method Vec4Map.Construct : "
			"(map Int32 Vec4 a)"
			"(this.a = a)"
		")"

		"(function Main -> (Int32 result):"
		"	(map Int32 Vec4Map m)"
		"	(m.Insert 45 Vec4Map ())"
		"	(node n = (m 45))"
		"	(Vec4Map subMap = & n)"
		"	(subMap.a.Insert 22 Vec4 (199 233 455))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapInMap");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());
	}

	void TestMapInsertCorrectRefs(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct Value (IStringBuilder sb))"

			"(function AddJoe90 (map Int32 Value m) -> :"
			"    (Value v)"
			"    (v.sb = NewTokenBuilder)"
			"    (v.sb \"Joe\")"
			"    (m.Insert 90 v)"
			")"

			"(function Main -> (Int32 result):"
			"	(map Int32 Value m)"
			"   (AddJoe90 m)"
			"	(node n = (m 90))"
			"	(Value value = & n)"
			"	(value.sb.Length -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 3);
	}

	void TestMapCall(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(function Insert55v52 (map Int32 Int32 x) -> : (x.Insert 55 52))"
		
		"(function Main -> (Int32 result):"
		"	(map Int32 Int32 m)"
		"	(Insert55v52 m)"
		"	(node n = (m 55))"
		"	(result = n.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapCall");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 52);
	}

	void TestMapStrongTyping(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(function Insert55v52 (map Int64 Int64 a) -> : "
		"	(a.Insert 55 52)"
		")"
  
		"(function Main -> (Int32 result):"
		"	(map Int32 Float32 a)"
		"	(Insert55v52 a)"
		"	(node n = (a 55))"
		"	(result = n.Value)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapStrongTyping");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		UNUSED(result);
		ParseException ex;
		if (!s_logger.TryGetNextException(ex))
			validate(false);
	}

	void TestMapThrowAndCleanup(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(try"
		"		("
		"			(map IString Int32 a)"
		"			(a.Insert \"Joe\" 90 )"
		"			(Sys.Throw 7001 \"Ayup\")"
		"			(result = a.Length)"
		"		)"
		"	catch ex"
		"		("
		"			(result = ex.ErrorCode)"
		"		)"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapThrowAndCleanup");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 7001);
	}

	void TestMapThrowAndCleanup2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(map IString Int32 a)"
		"	(a.Insert \"Joe\" 90 )"
		"	(try"
		"		("
		"			(foreach v # a (Sys.Throw 7051 \"Ayup\"))"	
		"		)"
		"	catch ex"
		"		("
		"			(result = ex.ErrorCode)"
		"		)"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapThrowAndCleanup2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 7051);
	}

	void TestMapForeach2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct Vec4 (Int32 x y z w))"
		"(method Vec4.Construct (Int32 x) (Int32 y) (Int32 z) -> : (this.x = x) (this.y = y) (this.z = z) (this.w = 1))" 
  
		"(function Main -> (Int32 result):"
		"	(map Int32 Vec4 a)"
		"	(a.Insert 45 Vec4 (1 2 3))"
		"	(a.Insert 23 Vec4 (5 6 7))"
		"	(a.Insert -6 Vec4 (8 9 10))"
		"	(foreach n # a "
		"		(n.Pop)"
		"	)"
		"	(result = a.Length)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMapForeach2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestTypedef(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(alias Sys.Type.Int32 Sys.Type.Degrees)"
  
		"(function Main -> (Degrees result):"
		"	(result = 360)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestTypedef");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 360);
	}

	void ValidateNearZero(float f, float epsilon)
	{
		if (f < -epsilon || f > epsilon)
		{
			validate(false);
		}
	}

	void ValidateNearZero(double f, double epsilon)
	{
		if (f < -epsilon || f > epsilon)
		{
			validate(false);
		}
	}

	void TestMathSinCosF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x) (Float32 y):"
		"	(Sys.Maths.F32.Sin 0.5 -> x)"
		"	(Sys.Maths.F32.Cos 0.5 -> y)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathSinCosF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float32 result
		vm.Push(0.7f); // Allocate stack space for the float32 result		

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float cosx = vm.PopFloat32();
		float sinx = vm.PopFloat32();
		
		ValidateNearZero(sinx - 0.47942553860420300027328793521557f, 0.00001f);
		ValidateNearZero(cosx - 0.87758256189037271611628158260383f, 0.00001f);
	}

	void TestMathSinCos(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x) (Float64 y):"
		"	(Sys.Maths.F64.Sin 0.5 -> x)"
		"	(Sys.Maths.F64.Cos 0.5 -> y)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathSinCos");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result
		vm.Push(0.7); // Allocate stack space for the float64 result		

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double cosx = vm.PopFloat64();
		double sinx = vm.PopFloat64();

		ValidateNearZero(sinx - 0.47942553860420300027328793521557, 0.00001);
		ValidateNearZero(cosx - 0.87758256189037271611628158260383, 0.00001);
	}

	void TestMathTan(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.Tan 0.5 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathTan");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double tanx = vm.PopFloat64();

		ValidateNearZero(tanx - 0.54630248984379051325517946578029, 0.00001);
	}

	void TestMathTanF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.Tan 0.5 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathTanF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float tanx = vm.PopFloat32();

		ValidateNearZero(tanx - 0.54630248984379051325517946578029f, 0.00001f);
	}

	void TestMathArcTanF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.ArcTan 0.5 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathArcTanF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float arctanx = vm.PopFloat32();

		ValidateNearZero(arctanx - 0.46364760900080611621425623146121f, 0.00001f);
	}

	void TestMathArcTan(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.ArcTan 0.5 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathArcTan");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double arctanx = vm.PopFloat64();

		ValidateNearZero(arctanx - 0.46364760900080611621425623146121, 0.00001);
	}

	void TestMathArcTanGradF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.ArcTanYX 1 2 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathArcTanGradF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float arctanx = vm.PopFloat32();

		ValidateNearZero(arctanx - 0.46364760900080611621425623146121f, 0.00001f);
	}

	void TestMathArcTanGrad(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.ArcTanYX 1 2 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathArcTanGrad");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double arctanx = vm.PopFloat64();

		ValidateNearZero(arctanx - 0.46364760900080611621425623146121, 0.00001);
	}

	void TestMathArcSin(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.ArcSin 0.5 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathArcSin");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double arcsinx = vm.PopFloat64();

		ValidateNearZero(arcsinx - 0.52359877559829887307710723054658, 0.00001);
	}

	void TestMathArcSinF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.ArcSin 0.5 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathArcSinF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float arcsinx = vm.PopFloat32();

		ValidateNearZero(arcsinx - 0.52359877559829887307710723054658f, 0.00001f);
	}

	void TestMathArcCos(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.ArcCos 0.5 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathArcCos");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double arccosx = vm.PopFloat64();

		ValidateNearZero(arccosx - 1.0471975511965977461542144610932, 0.00001);
	}

	void TestMathArcCosF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.ArcCos 0.5 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathArcCosF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float arccosx = vm.PopFloat32();

		ValidateNearZero(arccosx - 1.0471975511965977461542144610932f, 0.00001f);
	}

	void TestMathSinhCoshF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x) (Float32 y):"
		"	(Sys.Maths.F32.Sinh 0.5 -> x)"
		"	(Sys.Maths.F32.Cosh 0.5 -> y)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathSinhCoshF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float32 result
		vm.Push(0.7f); // Allocate stack space for the float32 result		

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float coshx = vm.PopFloat32();
		float sinhx = vm.PopFloat32();

		ValidateNearZero(coshx - 1.1276259652063807852262251614027f, 0.00001f);
		ValidateNearZero(sinhx - 0.52109530549374736162242562641149f, 0.00001f);
	}

	void TestMathSinhCosh(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x) (Float64 y):"
		"	(Sys.Maths.F64.Sinh 0.5 -> x)"
		"	(Sys.Maths.F64.Cosh 0.5 -> y)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathSinhCosh");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result
		vm.Push(0.7); // Allocate stack space for the float64 result		

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double coshx = vm.PopFloat64();
		double sinhx = vm.PopFloat64();
		

		ValidateNearZero(coshx - 1.1276259652063807852262251614027, 0.00001);
		ValidateNearZero(sinhx - 0.52109530549374736162242562641149, 0.00001);
	}

	void TestMathTanh(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.Tanh 0.5 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathTanh");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double tanx = vm.PopFloat64();

		ValidateNearZero(tanx - 0.46211715726000975850231848364367, 0.00001);
	}

	void TestMathTanhF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.Tanh 0.5 -> x)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathTanhF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float tanx = vm.PopFloat32();

		ValidateNearZero(tanx - 0.46211715726000975850231848364367f, 0.00001f);
	}

	void TestMathExp(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.Exp 1.0 -> x)"
		"	(x = (x - 2.7182818284590452353602874713527) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathExp");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double x = vm.PopFloat64();

		ValidateNearZero(x, 0.00001);
	}

	void TestMathExpF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.Exp 1.0 -> x)"
		"	(x = (x - 2.7182818284590452353602874713527) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathExpF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float x = vm.PopFloat32();

		ValidateNearZero(x, 0.00001f);
	}

	void TestMathLogN(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.LogN 2.0 -> x)"
		"	(x = (x - 0.69314718055994530941723212145818) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathLogN");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double x = vm.PopFloat64();

		ValidateNearZero(x, 0.00001);
	}

	void TestMathLogNF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.LogN 2 -> x)"
		"	(x = (x - 0.69314718055994530941723212145818) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathLogNF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float x = vm.PopFloat32();

		ValidateNearZero(x, 0.00001f);
	}

	void TestMathLog10(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.Log10 1000.0 -> x)"
		"	(x = (x - 3) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathLog10");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double x = vm.PopFloat64();

		ValidateNearZero(x, 0.00001);
	}

	void TestMathLog10F(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.Log10 100000 -> x)"
		"	(x = (x - 5) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathLog10F");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float x = vm.PopFloat32();

		ValidateNearZero(x, 0.00001f);
	}

	void TestMathPow(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.Power 3.0 4.0 -> x)"
		"	(x = (x - 81) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathPow");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double x = vm.PopFloat64();

		ValidateNearZero(x, 0.00001);
	}

	void TestMathPowF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.Power 7.0 2.0 -> x)"
		"	(x = (x - 49.0) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathPowF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float x = vm.PopFloat32();

		ValidateNearZero(x, 0.00001f);
	}

	void TestMathAbsF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.Abs -5 -> x)"
		"	(x = (x - 5) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathAbsF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float x = vm.PopFloat32();

		ValidateNearZero(x, 0.00001f);
	}

	void TestMathAbs(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.Abs 12 -> x)"
		"	(x = (x - (Sys.Maths.F64.Abs -12) ))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathAbs");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double x = vm.PopFloat64();

		ValidateNearZero(x, 0.00001);
	}

	void TestMathSqrtF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.SquareRoot 25 -> x)"
		"	(x = (x - 5) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathSqrtF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float x = vm.PopFloat32();

		ValidateNearZero(x, 0.00001f);
	}

	void TestMathSqrt(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.SquareRoot 144 -> x)"
		"	(x = (x - 12))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathSqrt");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double x = vm.PopFloat64();

		ValidateNearZero(x, 0.00001);
	}

	void TestMathAbsInt32(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 x):"
		"	(Sys.Maths.I32.Abs 12 -> x)"
		"	(x = (x - (Sys.Maths.I32.Abs -12) ))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathAbsInt32");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(3); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();

		validate(x == 0)
	}

	void TestMathAbsInt64(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int64 x):"
		"	(Sys.Maths.I64.Abs 12 -> x)"
		"	(x = (x - (Sys.Maths.I64.Abs -12) ))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathAbsInt64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push((int64)0); // Allocate stack space for the int64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int64 x = vm.PopInt64();

		validate(x == 0)
	}

	void TestMathCeilF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.Ceiling 25.00123 -> x)"
		"	(x = (x - 26) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathCeilF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float x = vm.PopFloat32();

		ValidateNearZero(x, 0.00001f);
	}

	void TestMathCeil(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.Ceiling 17.98 -> x)"
		"	(x = (x - 18))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathCeil");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double x = vm.PopFloat64();

		ValidateNearZero(x, 0.00001);
	}

	void TestMathFloorF(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float32 x):"
		"	(Sys.Maths.F32.Floor 25.00123 -> x)"
		"	(x = (x - 25) )"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathFloorF");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3f); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float x = vm.PopFloat32();

		ValidateNearZero(x, 0.00001f);
	}

	void TestMathFloor(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Float64 x):"
		"	(Sys.Maths.F64.Floor 17.98 -> x)"
		"	(x = (x - 17))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathFloor");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0.3); // Allocate stack space for the float64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double x = vm.PopFloat64();

		ValidateNearZero(x, 0.00001);
	}

	void TestMathModInt32(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 rem):"
			"	(Sys.Maths.I32.Mod 16 9 -> rem)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathModInt32");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(3); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int rem = vm.PopInt32();

		validate(rem == 7);
	}

	void TestMathModInt64(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int64 rem):"
			"	(Sys.Maths.I64.Mod 12 5 -> rem)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathModInt64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push((int64)0); // Allocate stack space for the int64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int64 rem = vm.PopInt64();
		int64 quot = vm.PopInt64();
		UNUSED(quot);

		validate(rem == 2);
	}

	void TestMathShiftInt64(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int64 x):"
		"	(x = 0)"
		"	(x = (x + (Sys.Maths.I64.LeftShift 3 2)))"
		"	(x = (x + (Sys.Maths.I64.RightShift 32 5)))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathShiftInt64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push((int64)0); // Allocate stack space for the int64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int64 x = vm.PopInt64();

		validate(x == 13);
	}

	void TestMathShiftInt32(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 x):"
		"	(x = 0)"
		"	(x = (x + (Sys.Maths.I32.LeftShift 3 2)))"
		"	(x = (x + (Sys.Maths.I32.RightShift 32 5)))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMathShiftInt32");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0x3); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();

		validate(x == 13);
	}

	void TestMinMaxInt32(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 x):"
			"	(Int32 y = (Sys.Maths.I32.MinOf 5 7))"
			"	(Int32 z = (Sys.Maths.I32.MinOf -1 14))"
			"	(Sys.Maths.I32.MaxOf y z -> x)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestMinMaxInt32");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();

		validate(x == 5);
	}

	void TestMinMaxFloat32(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Float32 x):"
			"	(Float32 y = (Sys.Maths.F32.MinOf 5 7))"
			"	(Float32 z = (Sys.Maths.F32.MinOf -1 14))"
			"	(Sys.Maths.F32.MaxOf y z -> x)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestMinMaxFloat32");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float x = vm.PopFloat32();

		validate(x == 5.0f);
	}

	void TestMinMaxInt64(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int64 x):"
			"	(Int64 y = (Sys.Maths.I64.MinOf 5 7))"
			"	(Int64 z = (Sys.Maths.I64.MinOf -1 14))"
			"	(Sys.Maths.I64.MaxOf y z -> x)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestMinMaxInt64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push((int64)0x3); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int64 x = vm.PopInt64();

		validate(x == 5);
	}

	void TestMinMaxFloat64(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Float64 x):"
			"	(Float64 y = (Sys.Maths.F64.MinOf 5 7))"
			"	(Float64 z = (Sys.Maths.F64.MinOf -1 14))"
			"	(Sys.Maths.F64.MaxOf y z -> x)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestMinMaxFloat64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push((int64)0x3); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float64 x = vm.PopFloat64();

		validate(x == 5.0);
	}

	void TestLimitsInt32(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int32 x)(Int32 y):"
			"	(x = Sys.Maths.I32.MinValue)"
			"	(y = Sys.Maths.I32.MaxValue)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestLimitsInt32");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the y int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 y = vm.PopInt32();
		int32 x = vm.PopInt32();

		validate(x == 0x80000000);
		validate(y == 0x7FFFFFFF);
	}

	void TestIsInfinity(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):"
			"	(Sys.Maths.F32.IsInfinity (1 / 0) -> x)"
			"	(Sys.Maths.F32.IsInfinity (-1 / 0) -> y)"
			"	(Sys.Maths.F32.IsInfinity (0 / 0) -> z)"
			"	(Sys.Maths.F32.IsInfinity 3 -> w)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestIsInfinity");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		boolean32 w = vm.PopInt32();
		boolean32 z = vm.PopInt32();
		boolean32 y = vm.PopInt32();
		boolean32 x = vm.PopInt32();

		validate(x == 1);
		validate(y == 1);
		validate(z == 0);
		validate(w == 0);
	}

	void TestIsQuietNan(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Bool x)(Bool y)(Bool z):"
			"	(Sys.Maths.F32.IsQuietNan (0 / 0) -> x)"
			"	(Sys.Maths.F32.IsQuietNan (1 / 0) -> y)"
			"	(Sys.Maths.F32.IsQuietNan 2 -> z)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestIsQuietNan");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		boolean32 z = vm.PopInt32();
		boolean32 y = vm.PopInt32();
		boolean32 x = vm.PopInt32();

		validate(x == 1);
		validate(y == 0);
		validate(z == 0);
	}

	void TestIsFinite(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):"
			"	(Sys.Maths.F32.IsFinite (0 / 0) -> x)"
			"	(Sys.Maths.F32.IsFinite (1 / 0) -> y)"
			"	(Sys.Maths.F32.IsFinite 0 -> z)"
			"	(Sys.Maths.F32.IsFinite 1 -> w)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestIsFinite");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		boolean32 w = vm.PopInt32();
		boolean32 z = vm.PopInt32();
		boolean32 y = vm.PopInt32();
		boolean32 x = vm.PopInt32();

		validate(x == 0);
		validate(y == 0);
		validate(z == 1);
		validate(w == 1);
	}

	void TestIsNormal(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):"
			"	(Sys.Maths.F32.IsNormal (0 / 0) -> x)"
			"	(Sys.Maths.F32.IsNormal (1 / 0) -> y)"
			"	(Sys.Maths.F32.IsNormal 0 -> z)"
			"	(Sys.Maths.F32.IsNormal 1 -> w)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestIsNormal");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		boolean32 w = vm.PopInt32();
		boolean32 z = vm.PopInt32();
		boolean32 y = vm.PopInt32();
		boolean32 x = vm.PopInt32();

		validate(x == 0);
		validate(y == 0);
		validate(z == 0);
		validate(w == 1);
	}

	void TestIsInfinity64(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):"
			"	(Sys.Maths.F64.IsInfinity (1 / 0) -> x)"
			"	(Sys.Maths.F64.IsInfinity (-1 / 0) -> y)"
			"	(Sys.Maths.F64.IsInfinity (0 / 0) -> z)"
			"	(Sys.Maths.F64.IsInfinity 3 -> w)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestIsInfinity64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		boolean32 w = vm.PopInt32();
		boolean32 z = vm.PopInt32();
		boolean32 y = vm.PopInt32();
		boolean32 x = vm.PopInt32();

		validate(x == 1);
		validate(y == 1);
		validate(z == 0);
		validate(w == 0);
	}

	void TestIsQuietNan64(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Bool x)(Bool y)(Bool z):"
			"	(Sys.Maths.F64.IsQuietNan (0 / 0) -> x)"
			"	(Sys.Maths.F64.IsQuietNan (1 / 0) -> y)"
			"	(Sys.Maths.F64.IsQuietNan 2 -> z)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestIsQuietNan64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		boolean32 z = vm.PopInt32();
		boolean32 y = vm.PopInt32();
		boolean32 x = vm.PopInt32();

		validate(x == 1);
		validate(y == 0);
		validate(z == 0);
	}

	void TestIsFinite64(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):"
			"	(Sys.Maths.F64.IsFinite (0 / 0) -> x)"
			"	(Sys.Maths.F64.IsFinite (1 / 0) -> y)"
			"	(Sys.Maths.F64.IsFinite 0 -> z)"
			"	(Sys.Maths.F64.IsFinite 1 -> w)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestIsFinite64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		boolean32 w = vm.PopInt32();
		boolean32 z = vm.PopInt32();
		boolean32 y = vm.PopInt32();
		boolean32 x = vm.PopInt32();

		validate(x == 0);
		validate(y == 0);
		validate(z == 1);
		validate(w == 1);
	}

	void TestIsNormal64(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):"
			"	(Sys.Maths.F64.IsNormal (0 / 0) -> x)"
			"	(Sys.Maths.F64.IsNormal (1 / 0) -> y)"
			"	(Sys.Maths.F64.IsNormal 0 -> z)"
			"	(Sys.Maths.F64.IsNormal 1 -> w)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestIsNormal64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the x int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		boolean32 w = vm.PopInt32();
		boolean32 z = vm.PopInt32();
		boolean32 y = vm.PopInt32();
		boolean32 x = vm.PopInt32();

		validate(x == 0);
		validate(y == 0);
		validate(z == 0);
		validate(w == 1);
	}

	void TestLimitsFloat32(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Float32 x)(Float32 y):"
			"	(x = Sys.Maths.F32.MinValue)"
			"	(y = Sys.Maths.F32.MaxValue)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestLimitsFloat32");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // Allocate stack space for the x int32 result
		vm.Push(0x3); // Allocate stack space for the y int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		float y = vm.PopFloat32();
		float x = vm.PopFloat32();

		validate(x == std::numeric_limits<float>::min());
		validate(y == std::numeric_limits<float>::max());
	}

	void TestLimitsFloat64(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Float64 x)(Float64 y):"
			"	(x = Sys.Maths.F64.MinValue)"
			"	(y = Sys.Maths.F64.MaxValue)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestLimitsFloat64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push((int64)0x3); // Allocate stack space for the x int32 result
		vm.Push((int64)0x3); // Allocate stack space for the y int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		double y = vm.PopFloat64();
		double x = vm.PopFloat64();

		validate(x == std::numeric_limits<double>::min());
		validate(y == std::numeric_limits<double>::max());
	}

	void TestLimitsInt64(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(function Main -> (Int64 x)(Int64 y):"
			"	(x = Sys.Maths.I64.MinValue)"
			"	(y = Sys.Maths.I64.MaxValue)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestLimitsInt64");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push((int64)0x3); // Allocate stack space for the x int32 result
		vm.Push((int64)0x3); // Allocate stack space for the y int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int64 y = vm.PopInt64();
		int64 x = vm.PopInt64();

		validate(x == 0x8000000000000000);
		validate(y == 0x7FFFFFFFFFFFFFFF);
	}

	void TestReturnInterface(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)\n"
		" (alias Main EntryPoint.Main)\n"
  
		"(using Sys.Type)\n"

		"(namespace Bot)\n"

		"(interface Bot.IRobotBrain (Id -> (Int32 id)))\n"
		"(interface Bot.IRobot (Brain -> (Bot.IRobotBrain brain)))\n"

		"(class Robot (implements Bot.IRobot) (Bot.IRobotBrain brain))\n"
		"(class RobotBrain (implements Bot.IRobotBrain))\n"

		"(method Robot.Construct : (this.brain = (Bot.NewBrain)))\n"
		"(method RobotBrain.Construct : )\n"
		"(method RobotBrain.Id -> (Int32 id): (id = 9000))\n"
		"(method Robot.Brain -> (Bot.IRobotBrain brain): (brain = this.brain))\n"
		"(factory Bot.NewRobot Bot.IRobot : (construct Robot))\n"
		"(factory Bot.NewBrain Bot.IRobotBrain : (construct RobotBrain))\n"
  
		"(function Main -> (Int32 exitCode):\n"
		"	(Bot.IRobot robby (Bot.NewRobot))\n"
		"	(Bot.IRobotBrain brain = robby.Brain)\n"
		"	(brain.Id -> exitCode)\n"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestReturnInterface");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0x7FFF7FFF); // Allocate stack space for the int23 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 exitCode = vm.PopInt32();

		validate(exitCode == 9000);
	}

	void TestDoubleArrowsInFunction(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 result):"	
			"	(result = 7)"
			")"			
			"(alias Main EntryPoint.Main)"

			"(function F (Int32 x) -> (Int32 y) -> : (y = x))"			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDoubleArrowsInFunction");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());	
			vm.Push(0); // Allocate stack space for the int32 result
			EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
			UNUSED(result);
			validate(false);
		}
		catch (ParseException&)
		{
			
		}
	}

	void TestMemberwiseInit(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(struct GameObject (IString name) )"	
			"(function Main -> (Int32 result):"	
			"	(GameObject obj = \"dog\")"
			"	(Sys.Print obj.name)"
			"	(result = obj.name.Length)"
			")"			
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMemberwiseInit");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 3);		
	}

	void TestMemberwise2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"(using Sys.Maths)\n"
			"(struct MatObject (Vec3i a b))\n"
			"(function Main -> (Int32 result):\n"
			"	(Vec3i i = 12 0 0)\n"
			"	(Vec3i j = 0  3 0)\n"
			"	(MatObject mo = i j)\n"
			"   (result = (mo.a.x / mo.b.y))\n"
			")\n"
			"(alias Main EntryPoint.Main)\n"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMemberwise2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 4);
	}

	void TestStructStringPassByRef(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(struct GameObject (Int32 value) (IString name) (Int32 id))"	
			"(function PrintGameObject (GameObject obj)-> (Int32 length) : "	
			"	(Sys.Print obj.name -> length)"	
			")"	
			"(function Main -> (Int32 result):"	
			"	(GameObject obj = 7 \"dog\" 12)"
			"	(PrintGameObject obj -> result)"
			")"			
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStructStringPassByRef");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 3);		
	}

	void TestEmptyString(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
	
			"(function Main -> (Int32 result):"	
			"	(IString s = \"\")"
			"	(Sys.Print s -> result)"
			")"			
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestEmptyString");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 0);		
	}

	void TestMeshStruct(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"

			"(struct MeshInstance"
			"	(IString name)"
			"	(IString modelName)"
			"	(Sys.Maths.Matrix4x4 worldMatrix)"
			")"

			"(function Main -> (Int32 result):"	
			"	(MeshInstance instance = "
			"		\"geoff\" \"steve\""
			"		("
			"			(-1.000000 0.000000 -0.000000 -1.239238)"
			"			(-0.000000 -1.000000 0.000796 3.846236)"
			"			(0.000000 0.000796 1.000000 0.506750)"
			"			(0.000000 0.000000 0.000000 1.000000)"
			"		)"
			"	)"
			"	(Sys.Print instance.name)"
			"	(Sys.Print instance.modelName)"
			")"			
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMeshStruct");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 0);		
	}

	void TestMeshStruct2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"

			"(struct MeshInstance"
			"	(IString name)"
			"	(IString modelName)"
			"	(Sys.Maths.Matrix4x4 worldMatrix)"
			")"

			"(function Main -> (Int32 result):"	
			"	(MeshInstance instance = "
			"		\"geoff\" \"steve\""
			"		("
			"			(-1.000000 0.000000 -0.000000 -1.239238)"
			"			(-0.000000 -1.000000 0.000796 3.846236)"
			"			(0.000000 0.000796 1.000000 0.506750)"
			"			(0.000000 0.000000 0.000000 1.000000)"
			"		)"
			"	)"
			"	(Cat instance -> result)"
			")"	
			"(function Cat(MeshInstance instance) -> (Int32 result):"	
			"	(Dog instance.worldMatrix -> result)"
			")"	
			"(function Dog(Sys.Maths.Matrix4x4 m) -> (Int32 result):"	
			"	(if (m.r2.y == 0.000796) (result = 7) else (result = 9))"
			")"	
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMeshStruct2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 7);		
	}

	void TestMeshStruct3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"

			"(struct MeshInstance"
			"	(IString name)"
			"	(IString modelName)"
			"	(Sys.Maths.Matrix4x4 worldMatrix)"
			")"

			"(function Main -> (Int32 result):"	
			"	(MeshInstance instance = "
			"		\"geoff\" \"steve\""
			"		("
			"			(-1.000000 0.000000 -0.000000 -1.239238)"
			"			(-0.000000 -1.000000 0.000796 3.846236)"
			"			(0.000000 0.000796 1.000000 0.506750)"
			"			(0.000000 0.000000 0.000000 1.000000)"
			"		)"
			"	)"
			"	(PrintModelName instance -> result)"
			")"	
			"(function PrintModelName(MeshInstance instance) -> (Int32 result):"	
			"	(PrintString instance.modelName -> result)"
			")"	
			"(function PrintString(IString s) -> (Int32 result):"	
			"	(Sys.Print s -> result)"
			")"	
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMeshStruct3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 5);		
	}

	void TestMeshStruct4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type.Formatters)"
			"(namespace EntryPoint)"

			"(struct MeshInstance"
			"	(IString name)"
			"	(IString modelName)"
			"	(Sys.Maths.Matrix4x4 worldMatrix)"
			")"

			"(function Main -> (Int32 result):"	
			"	(MeshInstance instance = "
			"		\"geoff\" \"steve\""
			"		("
			"			(-1.000000 0.000000 -0.000000 -1.239238)"
			"			(-0.000000 -1.000000 0.000796 3.846236)"
			"			(0.000000 0.000796 1.000000 0.506750)"
			"			(0.000000 0.000000 0.000000 1.000000)"
			"		)"
			"	)"
			"	(Sys.Type.IStringBuilder s = Sys.Type.NewPathBuilder)"
			"	(Pointer pName = instance.name.Buffer)"
			"	(#Sys.Type.build s \"Hello World!\" pName NewLine)"
			"	(Sys.Print s)"
			"	(PrintModelName instance)"
			"	(result = 7)"
			")"	
			"(function PrintModelName(MeshInstance instance) -> (Int32 result):"	
			"	(PrintString instance -> result)"
			")"	
			"(function PrintString (MeshInstance instance) -> (Int32 result):"	
			"	(Sys.Type.IStringBuilder s = Sys.Type.NewPathBuilder)"
			"	(Pointer pName = instance.name.Buffer)"
			"	(#Sys.Type.build s \"Hello World!\" pName NewLine)"
			"	(Sys.Print s)"
			")"	
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMeshStruct4");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 7);		
	}

	void TestYield(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"

			"(function Main -> (Int32 result):"	
			"	(yield)"	
			"	(result = 7)"
			")"	
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestYield");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		validate(EXECUTERESULT_YIELDED == vm.Execute(VM::ExecutionFlags(false, true)));
		validate(EXECUTERESULT_TERMINATED == vm.ContinueExecution(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 7);		
	}

	void TestStructCopyRef(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"	(alias Main EntryPoint.Main)"
			"(struct Planet"
			"	(Int32 a b c d)"
			")"
			"(struct Cosmos"
			"	(Planet earth)"
			")"
			"(function Init (Cosmos c)-> :"
			"	(Planet e = 1 2 3 4)"
			"	(c.earth = e)"
			")"
			"(function Main -> (Int32 result):"
			"	(Cosmos c)"
			"	(Init c)"
			"	(result = c.earth.a)"
			"	(result = (result + c.earth.b))"
			"	(result = (result + c.earth.c))"
			"	(result = (result + c.earth.d))"
			")"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestStructCopyRef");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		validate(EXECUTERESULT_TERMINATED == vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 10);
	}

	void TestStructByRefAssign(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(struct Cube (Int32 x y z))"
			"(function Main -> (Int32 result):"	
			"	(Cube a = 7 12 23)"	
			"	(Cube b = 13 15 17)"	
			"	(AssignCubeBtoA a b)"	
			"	(result = a.x)"
			")"	
			"(function AssignCubeBtoA (Cube a)(Cube b)-> :"
			"	(a.x = b.x)"
			"	(a.y = b.y)"
			"	(a.z = b.z)"
			")"
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStructByRefAssign");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		validate(EXECUTERESULT_TERMINATED == vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 13);		
	}

	void TestAssignVectorComponentsFromParameters(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using Sys.Maths)"
			"(function Main -> (Float32 result):"	
			"	(SetVec 7.0 12.5 23.5 -> result)"	
			")"	
			"(function SetVec (Float32 x)(Float32 y)(Float32 z)->(Float32 sum) :"
			"	(Vec3 pos = x y z)"
			"	(sum = (pos.y + pos.z))"
			")"
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignVectorComponentsFromParameters");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		validate(EXECUTERESULT_TERMINATED == vm.Execute(VM::ExecutionFlags(false, true)));

		float value = vm.PopFloat32();
		validate(value == 36.0f);		
	}

	void TestAddRefWithLocalVariable(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using Sys.Maths)"
			"(using EntryPoint)"
			"(interface Sys.IFidelio"
			"	(Id -> (Int32 id))"
			")"
			"(class Fidelio (implements Sys.IFidelio))"
			"(method Fidelio.Id -> (Int32 id): (id = 12))"
			"(method Fidelio.Construct -> : )"
			"(factory Sys.Fidelio Sys.IFidelio : (construct Fidelio))"
			
			"(function Main -> (Int32 result):"
			"	(Sys.IFidelio f (Sys.Fidelio))"
			"	(Sys.IFidelio g = f)"
			"	(g.Id -> result)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestAddRefWithLocalVariable");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		int32 id = vm.PopInt32();
		validate(id == 12);
	}

	void TestConstructFromInterface(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using Sys.Maths)"
			"(using EntryPoint)"
			"(interface Sys.IFidelio"
			"	(Id -> (Int32 id))"
			")"
			"(class Fidelio (implements Sys.IFidelio))"
			"(method Fidelio.Id -> (Int32 id): (id = 12))"
			"(method Fidelio.Construct -> : )"
			"(factory Sys.Fidelio Sys.IFidelio : (construct Fidelio))"
			"(class Operas (defines Sys.IOperas) (Sys.IFidelio f))"
			"(method Operas.Construct (Sys.IFidelio f)-> : (this.f = f))"
			"(method Operas.ByBeethoven -> (Sys.IFidelio f): (f = this.f))"
			"(factory Sys.Operas Sys.IOperas (Sys.IFidelio f): (construct Operas f))"
			"(function Main -> (Int32 result):"	
			"	(Sys.IFidelio f (Sys.Fidelio))"	
			"	(Sys.IOperas operas (Sys.Operas f))"	
			"	(Sys.IFidelio bf = operas.ByBeethoven)"
			"	(bf.Id -> result)"
			")"	
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestConstructFromInterface");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == Rococo::EXECUTERESULT_TERMINATED);	
	}

	void TestIgnoreOutputInterface(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using Sys.Maths)"
			"(using EntryPoint)"
			"(interface Sys.IFidelio"
			"	(Id -> (Int32 id))"
			")"
			"(class Fidelio (implements Sys.IFidelio))"
			"(method Fidelio.Id -> (Int32 id): (id = 12))"
			"(method Fidelio.Construct -> : )"
			"(factory Sys.Fidelio Sys.IFidelio : (construct Fidelio))"
			"(class Operas (defines Sys.IOperas) (Sys.IFidelio f))"
			"(method Operas.Construct (Sys.IFidelio f)-> : (this.f = f))"
			"(method Operas.ByBeethoven -> (Sys.IFidelio f): (f = this.f))"
			"(factory Sys.Operas Sys.IOperas (Sys.IFidelio f): (construct Operas f))"
			"(function Main -> (Int32 result):"
			"	(Sys.IFidelio f (Sys.Fidelio))"
			"	(Sys.IOperas operas (Sys.Operas f))"
			"	(operas.ByBeethoven)"
			"	(f.Id -> result)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == Rococo::EXECUTERESULT_TERMINATED);
	}

	void TestAssignPointerFromFunction(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"

			"(function Main -> (Pointer result):"	
			"	(Pointer p = (Test.GetPointer 9))"	
			"	(result = p)"	
			")"	
			"(alias Main EntryPoint.Main)"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestAssignPointerFromFunction");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		struct ANON
		{
			static void GetPointer(NativeCallEnvironment& e)
			{
				void* ptr = (void*) 0x12345678;
				WriteOutput(0, ptr, e);
			}
		};

		const INamespace& ns = ss.AddNativeNamespace("Test");
		ss.AddNativeCall(ns, ANON::GetPointer, NULL,"GetPointer (Int32 id) -> (Pointer pointer)", __FILE__, __LINE__, false, 0);
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push((void*) 0); // Allocate stack space for the int32 result

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		void* value = vm.PopPointer();
		validate(value == (void*) 0x12345678);		
	}

	void TestMinimumConstruct(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(class Eros)"
			" (method Eros.Construct : )"
			"(function Main -> :"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestMinimumConstruct");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);
	}

	void TestRaw(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(' man of bronze \"talos\")"
			"(namespace EntryPoint)"
			"(function Main -> :"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);
	}

	void TestDeltaOperators(IPublicScriptSystem& ss)
	{
		cstr srcCode = "(namespace EntryPoint)"
			"(using Sys.Maths)"
			"(using EntryPoint)"
			"(function Main -> (Int32 result) :"
			"(Int32 x = 5)"
				"(Int32 y = 5)"
				"(x -= 3)"
				"(if (x != 2) (Sys.Throw 0 \"Bad delta subtract immediate\"))"
				"(x += 4)"
				"(if (x != 6) (Sys.Throw 0 \"Bad delta add immediate\"))"
				"(x *= 2)"
				"(if (x != 12) (Sys.Throw 0 \"Bad delta multiply immediate\"))"
				"(x /= 3)"
				"(if (x != 4) (Sys.Throw 0 \"Bad delta divide immediate\"))"
				"(x += y)"
				"(if (x != 9) (Sys.Throw 0 \"Bad delta add variable\"))"
				"(x -= y)"
				"(if (x != 4) (Sys.Throw 0 \"Bad delta subtract variable\"))"
				"(x *= y)"
				"(if (x != 20) (Sys.Throw 0 \"Bad delta multiply variable\"))"
				"(x /= y)"
				"(if (x != 4) (Sys.Throw 0 \"Bad delta divide variable\"))"
				"(result = x)"
				")"
				"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);
		
		int x = vm.PopInt32();
		validate(x == 4);
	}

	void TestMethodFromClosure(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"

			"(class Robot (defines Sys.IRobot))"

			"(method Robot.Id -> (Int32 id): (id = 5))"

			"(method Robot.Construct : )"

			"(factory Sys.NewRobot Sys.IRobot : (construct Robot))"

			"(archetype Sys.VoidToVoid -> )"

			"(function Main -> (Int32 result):\n"
			"	(Sys.IRobot robot (Sys.NewRobot))\n"
			"   (Sys.VoidToVoid c = (closure -> : (robot.Id -> result)))\n"
			"   (c)\n"
			")\n"

			"(alias Main EntryPoint.Main)";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		const IModule& m = ss.PublicProgramObject().GetModule(0);
		UNUSED(m);

		vm.Push(100); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 5);
	}

	void TestDeltaOperators3(IPublicScriptSystem& ss)
	{
		cstr srcCode = "(namespace EntryPoint)"
			"(using Sys.Maths)"
			"(using EntryPoint)"
			"(function Main -> (Int32 result) :"
			"(Int64 x = 5)"
			"(Int64 y = 5)"
			"(x -= 3)"
			"(if (x != 2) (Sys.Throw 0 \"Bad delta subtract immediate\"))"
			"(x += 4)"
			"(if (x != 6) (Sys.Throw 0 \"Bad delta add immediate\"))"
			"(x *= 2)"
			"(if (x != 12) (Sys.Throw 0 \"Bad delta multiply immediate\"))"
			"(x /= 3)"
			"(if (x != 4) (Sys.Throw 0 \"Bad delta divide immediate\"))"
			"(x += y)"
			"(if (x != 9) (Sys.Throw 0 \"Bad delta add variable\"))"
			"(x -= y)"
			"(if (x != 4) (Sys.Throw 0 \"Bad delta subtract variable\"))"
			"(x *= y)"
			"(if (x != 20) (Sys.Throw 0 \"Bad delta multiply variable\"))"
			"(x /= y)"
			"(if (x != 4) (Sys.Throw 0 \"Bad delta divide variable\"))"
			"(Sys.Maths.I64.ToInt32 x -> result)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		int x = vm.PopInt32();
		validate(x == 4);
	}

	void TestDeltaOperators5(IPublicScriptSystem& ss)
	{
		cstr srcCode = "(namespace EntryPoint)"
			"(using Sys.Maths)"
			"(using EntryPoint)"
			"(function Main -> (Float32 result) :"
			"	(Vec2 v = 9 3)"
			"	(Float32 y = 5)"
			"	(y += v.x)"
			"   (result = y)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0.0f); // add our output to the stack

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		auto x = vm.PopFloat32();
		validate(x == 14);
	}

	void TestDeltaOperators4(IPublicScriptSystem& ss)
	{
		cstr srcCode = "(namespace EntryPoint)"
			"(using Sys.Maths)"
			"(using EntryPoint)"
			"(function Main -> (Int32 result) :"
			"(Float64 x = 5)"
			"(Float64 y = 5)"
			"(x -= 3)"
			"(if (x != 2) (Sys.Throw 0 \"Bad delta subtract immediate\"))"
			"(x += 4)"
			"(if (x != 6) (Sys.Throw 0 \"Bad delta add immediate\"))"
			"(x *= 2)"
			"(if (x != 12) (Sys.Throw 0 \"Bad delta multiply immediate\"))"
			"(x /= 3)"
			"(if (x != 4) (Sys.Throw 0 \"Bad delta divide immediate\"))"
			"(x += y)"
			"(if (x != 9) (Sys.Throw 0 \"Bad delta add variable\"))"
			"(x -= y)"
			"(if (x != 4) (Sys.Throw 0 \"Bad delta subtract variable\"))"
			"(x *= y)"
			"(if (x != 20) (Sys.Throw 0 \"Bad delta multiply variable\"))"
			"(x /= y)"
			"(if (x != 4) (Sys.Throw 0 \"Bad delta divide variable\"))"
			"(Sys.Maths.F64.ToInt32 x -> result)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		int x = vm.PopInt32();
		validate(x == 4);
	}

	void TestDeltaOperators2(IPublicScriptSystem& ss)
	{
		cstr srcCode = "(namespace EntryPoint)"
			"(using Sys.Maths)"
			"(using EntryPoint)"
			"(function Main -> (Int32 result) :"
			"(Float32 x = 5)"
			"(Float32 y = 5)"
			"(x -= 3)"
			"(if (x != 2) (Sys.Throw 0 \"Bad delta subtract immediate\"))"
			"(x += 4)"
			"(if (x != 6) (Sys.Throw 0 \"Bad delta add immediate\"))"
			"(x *= 2)"
			"(if (x != 12) (Sys.Throw 0 \"Bad delta multiply immediate\"))"
			"(x /= 3)"
			"(if (x != 4) (Sys.Throw 0 \"Bad delta divide immediate\"))"
			"(x += y)"
			"(if (x != 9) (Sys.Throw 0 \"Bad delta add variable\"))"
			"(x -= y)"
			"(if (x != 4) (Sys.Throw 0 \"Bad delta subtract variable\"))"
			"(x *= y)"
			"(if (x != 20) (Sys.Throw 0 \"Bad delta multiply variable\"))"
			"(x /= y)"
			"(if (x != 4) (Sys.Throw 0 \"Bad delta divide variable\"))"
			"(Sys.Maths.F32.ToInt32 x -> result)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		int x = vm.PopInt32();
		validate(x == 4);
	}

	void TestGlobalInt32(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(global Int32 i = 6)"
			"(namespace EntryPoint)"	
			"(function Main -> (Int32 x):"
			"	(global i -> x)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestGlobalInt32");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // add our output to the stack

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int x = vm.PopInt32();
		
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);
		
		validate(x == 6);
	}

	void TestArrayRefMember(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(struct Entity (Int32 x y)(IString s))"
			"(struct World (array Entity entities))"
			"(method World.Construct : (array Entity entities 2)(this.entities = entities))"
			"(function Main -> (Int32 result):"
			"	(Entity e0 = 3 5 \"Hello World\")"
			"	(Entity e1 = 7 9 \"Goodbye Universe\")"
			"   (World world ())"
			"   (world.entities.Push e0)"
			"   (world.entities.Push e1)"
			"   (foreach e # (world.entities 0)"
			"		(result = e.x)"
			"   )"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // add our output to the stack

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int x = vm.PopInt32();

		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		validate(x == 3);
	}

	void TestArrayPushBool(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(struct Entity (Bool isCat))"

			"(function Main -> (Int32 result):"
			"	(array Entity entities 1)"
			"   (entities.Push Entity (true))"
			"   (Entity e)"
			"	(e = (entities 0))"
			"   (if (e.isCat)"
			"		(result = 72)"
			"   )"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // add our output to the stack

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int output = vm.PopInt32();

		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		validate(output == 72);
	}

	void TestArrayRefMember2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(struct Entity (Sys.Maths.Vec2i position))"
			"(struct World (array Entity entities))"
			"(method World.Construct : (array Entity entities 2)(this.entities = entities))"
			"(function Main -> (Int32 result):"
			"	(Entity e0 = (3 5))"
			"	(Entity e1 = (7 9))"
			"   (World world ())"
			"   (world.entities.Push e0)"
			"   (world.entities.Push e1)"
			"   (Entity e)"
			"	(e = (world.entities 0))"
			"   (result = e.position.y)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // add our output to the stack

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int output = vm.PopInt32();

		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		validate(output == 5);
	}

	void TestStringArray(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using Sys.Type)"
			"(function Main -> (Int32 x):"
			"	(array IString s 12)"
			"	(s.Push \"Hello world\")"
			"   (IString txt = (s 0))"
			"   (Sys.Print txt)"
			"   (x = txt.Length)"
			"   (foreach string # s (Sys.Print string))"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // add our output to the stack

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int x = vm.PopInt32();

		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		validate(x == 11);
	}

	void TestPushStringLen(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(using Sys.Type)"

			"(struct Context (IString text))"

			"(function Mop (Int32 length) -> (Int32 result): (result = length + 7))"
			"(function Main -> (Int32 x):"
			"	(IString s = \"Hello World\")"
			"   (Context c)"
			"   (c.text = s)"
			"	(Mop c.text.Length -> x)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // add our output to the stack

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int x = vm.PopInt32();

		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		validate(x == 18);
	}

	void TestGlobalInt32_2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(global Int32 i = 6)"
			"(namespace EntryPoint)"
			"(function Main -> (Int32 x):"
			"	(Int32 value = 4)"
			"	(global i <- value)"
			"	(global i -> x)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestGlobalInt32_2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // add our output to the stack

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int x = vm.PopInt32();

		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		validate(x == 4);
	}

	void TestGlobalInt32_3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(global Int64 i = 6)"
			"(namespace EntryPoint)"
			"(function Main -> (Int64 x):"
			"	(global i <- 4)"
			"	(global i -> x)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestGlobalInt32_3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push((int64)0x3); // add our output to the stack

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int64 x = vm.PopInt64();

		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		validate(x == 4);
	}

	void TestBitwiseOr(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"(function Main -> (Int32 exitCode):"
			"	(Int32 i = 6)"
			"	(Int32 j = (i | 3))"
			"   (exitCode = j)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestBitwiseOr");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x00000001);
		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();

		int32 exitCode = vm.PopInt32();
		validate(exitCode == 7);

		validate(result == Rococo::EXECUTERESULT_TERMINATED);
	}

	static int32 RunBitwiseOrInt32(IPublicScriptSystem& ss, cstr srcCode, cstr name)
	{
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, name);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0);
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		return vm.PopInt32();
	}

	static int64 RunBitwiseOrInt64(IPublicScriptSystem& ss, cstr srcCode, cstr name)
	{
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, name);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push((int64)0);
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		return vm.PopInt64();
	}

	static void RunBitwiseOrFail(IPublicScriptSystem& ss, cstr srcCode, cstr name, cstr expectedFragment)
	{
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, name);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
			vm.Push(0);
			EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
			validate(result == EXECUTERESULT_THROWN);
			ParseException ex;
			validate(s_logger.TryGetNextException(ex));
			if (strstr(ex.Message(), expectedFragment) == nullptr)
			{
				printf("Expected '%s' in: %s\n", expectedFragment, ex.Message());
				validate(false);
			}
			s_logger.Clear();
		}
		catch (ParseException& e)
		{
			if (strstr(e.Message(), expectedFragment) == nullptr)
			{
				printf("Expected '%s' in: %s\n", expectedFragment, e.Message());
				validate(false);
			}
		}
	}

	void TestBitwiseOrInt64VarLiteral(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(function Main -> (Int64 exitCode):
				(Int64 i = 0x100000000)
				(Int64 j = (i | 5))
				(exitCode = j)
			)
			(alias Main EntryPoint.Main)
		)";

		validate(RunBitwiseOrInt64(ss, srcCode, "TestBitwiseOrInt64VarLiteral") == 0x100000005LL);
	}

	void TestBitwiseOrLiteralFold(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(function Main -> (Int32 j32)(Int64 j64):
				(j32 = (6 | 3))
				(j64 = (0x100000000 | 5))
			)
			(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestBitwiseOrLiteralFold");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0);
		vm.Push((int64)0);
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int64 j64 = vm.PopInt64();
		int32 j32 = vm.PopInt32();
		validate(j32 == 7);
		validate(j64 == 0x100000005LL);
	}

	void TestBitwiseOrVariables(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(function Main -> (Int32 j32)(Int64 j64):
				(Int32 a32 = 6)
				(Int32 b32 = 3)
				(j32 = (a32 | b32))
				(Int64 a64 = 0x100000000)
				(Int64 b64 = 5)
				(j64 = (a64 | b64))
			)
			(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestBitwiseOrVariables");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0);
		vm.Push((int64)0);
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int64 j64 = vm.PopInt64();
		int32 j32 = vm.PopInt32();
		validate(j32 == 7);
		validate(j64 == 0x100000005LL);
	}

	void TestBitwiseOrNegative(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(function Main -> (Int32 lit32)(Int32 var32)(Int64 lit64)(Int64 sign64):
				(lit32 = (-8 | 1))
				(Int32 i = -8)
				(var32 = (i | 1))
				(lit64 = (-8 | 1))
				(Int64 s = 0x8000000000000000)
				(sign64 = (s | 1))
			)
			(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestBitwiseOrNegative");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0);
		vm.Push(0);
		vm.Push((int64)0);
		vm.Push((int64)0);
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int64 sign64 = vm.PopInt64();
		int64 lit64 = vm.PopInt64();
		int32 var32 = vm.PopInt32();
		int32 lit32 = vm.PopInt32();
		validate(lit32 == -7);
		validate(var32 == -7);
		validate(lit64 == -7);
		validate(sign64 == (int64)0x8000000000000001ULL);
	}

	void TestBitwiseOrNested(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(function Main -> (Int32 nested)(Int32 orThenAdd)(Int32 orOfAdd):
				(Int32 a = 6)
				(Int32 b = 3)
				(Int32 c = 8)
				(nested = ((a | b) | c))
				(orThenAdd = ((a | b) + 1))
				(orOfAdd = (a | (b + 1)))
			)
			(alias Main EntryPoint.Main)
		)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestBitwiseOrNested");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0);
		vm.Push(0);
		vm.Push(0);
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 orOfAdd = vm.PopInt32();
		int32 orThenAdd = vm.PopInt32();
		int32 nested = vm.PopInt32();
		validate(nested == 15);     // (6|3)|8 = 7|8 = 15, not 17
		validate(orThenAdd == 8);   // (6|3)+1 = 8
		validate(orOfAdd == 6);     // 6|(3+1) = 6|4 = 6, not 10
	}

	void TestBitwiseOrFloat32LiteralRejected(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(function Main -> (Int32 exitCode):
				(Float32 j = (1.0 | 2.0))
				(exitCode = 0)
			)
			(alias Main EntryPoint.Main)
		)";

		RunBitwiseOrFail(ss, srcCode, "TestBitwiseOrFloat32LiteralRejected", "Unsupported operator in literal arithmetic");
	}

	void TestBitwiseOrFloat32VarRejected(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(function Main -> (Int32 exitCode):
				(Float32 a = 1.0)
				(Float32 b = 2.0)
				(Float32 j = (a | b))
				(exitCode = 0)
			)
			(alias Main EntryPoint.Main)
		)";

		RunBitwiseOrFail(ss, srcCode, "TestBitwiseOrFloat32VarRejected", "Unhandled operator in arithmetic expression");
	}

	void TestBitwiseOrFloat64LiteralRejected(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(function Main -> (Int32 exitCode):
				(Float64 j = (1.0e50 | 2.0e50))
				(exitCode = 0)
			)
			(alias Main EntryPoint.Main)
		)";

		RunBitwiseOrFail(ss, srcCode, "TestBitwiseOrFloat64LiteralRejected", "Unsupported operator in literal arithmetic");
	}

	void TestBitwiseOrFloat64VarRejected(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(function Main -> (Int32 exitCode):
				(Float64 a = 1.0)
				(Float64 b = 2.0)
				(Float64 j = (a | b))
				(exitCode = 0)
			)
			(alias Main EntryPoint.Main)
		)";

		RunBitwiseOrFail(ss, srcCode, "TestBitwiseOrFloat64VarRejected", "Unhandled operator in arithmetic expression");
	}

	void TestBitwiseOrAssignAndArg(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(function PassOr (Int32 x)->(Int32 y) : (y = x))
			(namespace EntryPoint)
			(function Main -> (Int32 exitCode):
				(Int32 i = 6)
				(Int32 j = 0)
				(j = (i | 3))
				(Int32 k = (PassOr (i | 3)))
				(exitCode = (j + k))
			)
			(alias Main EntryPoint.Main)
		)";

		validate(RunBitwiseOrInt32(ss, srcCode, "TestBitwiseOrAssignAndArg") == 14);
	}

	void TestCompareStruct(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(function IsEqVec2fVec2f (Vec2 a)(Vec2 b)->(Bool isEqual) :"
			"	(isEqual = ((a.x == b.x) and (a.y == b.y)))"
			")"
			"(using Sys.Maths)"
			"(namespace EntryPoint)"
			"(function Main -> (Int32 exitCode):"
			"	(Vec2 a = 0 1)"
			"	(Vec2 b = 0 1)"
			"  (if (a == b) (exitCode = 7) else (exitCode = 9))"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x00000001);
		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();

		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		int32 exitCode = vm.PopInt32();
		validate(exitCode == 7);
	}

	void TestMacroAsArgument1(IPublicScriptSystem& ss)
	{
		cstr macroCode =
			"(macro Sys.Seventeen in out (out.AddAtomic \"0x11\"))";

		cstr mainCode = 
			"(function PassThrough (Int32 x)->(Int32 y) : (y = x))"
			"(namespace EntryPoint)"
			"(function Main -> (Int32 exitCode):"
			"	(exitCode = (PassThrough (#Sys.Seventeen)))"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc1 = ss.SParser().ProxySourceBuffer(macroCode, -1, Vec2i {0,0}, "TestMacroAsArgument1_Macro");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc1()));

		Auto<ISourceCode> sc2 = ss.SParser().ProxySourceBuffer(mainCode, -1, Vec2i{ 0,0 }, "TestMacroAsArgument1_Main");
		Auto<ISParserTree> tree2(ss.SParser().CreateTree(sc2()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree(), tree2());

		vm.Push(0x00000001);
		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();

		int32 exitCode = vm.PopInt32();
		validate(exitCode == 17);

		validate(result == Rococo::EXECUTERESULT_TERMINATED);
	}

	void TestFactoryReturnsBaseInterface(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(interface Sys.IRobot)"
			"(interface Sys.ISpiderRobot (extends Sys.IRobot))"
			"(class Spider (implements Sys.ISpiderRobot))"
			"(namespace EntryPoint)"
			"(method Spider.Construct : )"
			"(factory Sys.NewSpider Sys.IRobot : (construct Spider))"
			"(function Main -> (Int32 exitCode):"
			"	(Sys.IRobot spider (Sys.NewSpider))"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x00000001);
		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();

		int32 exitCode = vm.PopInt32();
		validate(exitCode == 0);

		validate(result == Rococo::EXECUTERESULT_TERMINATED);
	}

	void TestMultipleDerivation(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(interface Sys.IBase"
			"	(A(Int64 i) -> )"
			")"

			"(interface Sys.IDerived"
			"	(extends Sys.IBase)"
			")"

			"(class Impl"
			"	(implements Sys.IDerived)"
			")"

			"(method Impl.A(Int64 i) -> : )"

			"(namespace EntryPoint)"
			"(function Main -> :"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestMultipleDerivation");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);
	}

	void TestMultipleDerivation2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(interface Sys.IBase"
			"	(A(Int64 i) -> (Bool result))"
			")"

			"(interface Sys.IDerived"
			"	(extends Sys.IBase)"
			")"

			"(class Impl"
			"	(implements Sys.IDerived)"
			")"

			"(method Impl.A(Int64 i) -> (Bool result): (result = 0))"

			"(namespace EntryPoint)"
			"(function Main -> :"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestMultipleDerivation2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);
	}

	void TestClassAttribute(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)\n"
			"(class Dog (defines Sys.IDog)\n"
			"	(attribute not-serialized)"
			")\n"
			"(method Dog.Woof -> (IString bark): \n"
			"	(bark = \"&nWoof&n&n\")\n"
			")\n"
			"(method Dog.Construct : )\n"
			"(factory Sys.Dog Sys.IDog  : (construct Dog))\n"
			"(namespace EntryPoint)\n"
			"(function Main -> :\n"
			"	(Sys.IDog dog(Sys.Dog))\n"
			"	(Sys.Print dog.Woof)\n"
			")\n"
			"(alias Main EntryPoint.Main)\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 1,1 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);
	}

	void TestClassDefinesInterface(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)\n"
			"(class Dog (defines Sys.IDog)\n"
			")\n"
			"(method Dog.Woof -> (IString bark): \n"
			"	(bark = \"&nWoof&n&n\")\n"
			")\n"
			"(method Dog.Construct : )\n"
			"(factory Sys.Dog Sys.IDog  : (construct Dog))\n"
			"(namespace EntryPoint)\n"
			"(function Main -> :\n"
			"	(Sys.IDog dog(Sys.Dog))\n"
			"	(Sys.Print dog.Woof)\n"
			")\n"
			"(alias Main EntryPoint.Main)\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestClassDefinesInterface");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);
	}

	void TestClassExtendsInterface(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(using Sys.Type)\n"
			"(interface Sys.IAnimal\n"
			"	(Name -> (IString name))\n"
			")\n"
			"(class Dog (defines Sys.IDog extends Sys.IAnimal)\n"
			")\n"
			"(method Dog.Woof -> (IString bark): \n"
			"	(bark = \"&nWoof&n&n\")\n"
			")\n"
			"(method Dog.Name -> (IString name): \n"
			"	(name = \"&nDigby&n&n\")\n"
			")\n"
			"(method Dog.Construct : )\n"
			"(factory Sys.Dog Sys.IDog  : (construct Dog))\n"
			"(namespace EntryPoint)\n"
			"(function Main -> :\n"
			"	(Sys.IDog dog(Sys.Dog))\n"
			"	(Sys.Print dog.Woof)\n"
			"	(Sys.Print dog.Name)\n"
			")\n"
			"(alias Main EntryPoint.Main)\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestClassExtendsInterface");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		auto& sys = *ss.PublicProgramObject().GetRootNamespace().FindSubspace("Sys");
		auto idog = sys.FindInterface("IDog");
		auto ianimal = sys.FindInterface("IAnimal");

		validate(idog->Base() == ianimal);

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);
	}

	void TestInstancing(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"	(alias Main EntryPoint.Main)"
			"	(function Main (Int32 x) -> (Int32 y):\n"
			"		(y = (x + 1))"
			"	)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestInstancing");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		VM::CPU cpu2;
		AutoFree<VM::IVirtualMachine> vm2(vm.Clone(cpu2));
		
		vm2->Push(0x3);

		Rococo::EXECUTERESULT result = vm2->Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		int x1 = vm2->PopInt32();
		validate(x1 = 4);
	}

	void TestNestedArrayEnumeration(IPublicScriptSystem& ss)
	{
		cstr srcCode =
R"(
(namespace EntryPoint)
	(alias Main EntryPoint.Main)
	(using Sys.Maths)

	(function Main -> (Float32 z):
		(array Vec2 vectors 2)
		(vectors.Push Vec2f (3 5))
		(vectors.Push Vec2f (7 9))

		(foreach v # vectors
			(foreach w # vectors
				(z += v.x)
				(z += v.y)
				(z += w.x)
				(z += w.y)
			)
		)
	)

)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 1,1 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		auto x1 = vm.PopFloat32();
		validate(x1 == 96.0f);
	}

	void TestClosureArg(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			"	(alias Main EntryPoint.Main)"

			"   (archetype Sys.GetInt32 -> (Int32 value))"

			"	(function Eval (closure Sys.GetInt32 f) -> (Int32 value):"
			"		(f -> value)"
			"	)"

			"	(function Main -> (Int32 y):\n"
			"		(Sys.GetInt32 f = "
			"			(closure -> (Int32 y):"
			"				(y = 77)"
			"			)"
			"		)"
			"		(y = (Eval f))"
			"	)";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestInstancing");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		int x1 = vm.PopInt32();
		validate(x1 == 77);
	}

	void TestBadClosureArg(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"	(alias Main EntryPoint.Main)\n"

			"   (archetype Sys.GetInt32 -> (Int32 value))\n"

			"	(function Eval (closure Sys.GetInt32 f) -> (Int32 value):\n"
			"		(Sys.GetInt32 g = f)\n"	
			"		(g -> value)\n"
			"	)\n"

			"	(function Main -> (Int32 y):\n"
			"		(Sys.GetInt32 f = \n"
			"			(closure -> (Int32 y):\n"
			"				(y = 77)\n"
			"			)\n"
			"		)\n"
			"		(y = (Eval f))\n"
			"	)\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestBadClosureArg");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);

		validate (s_logger.ExceptionCount() == 1)

		ParseException ex;
		validate(s_logger.TryGetNextException(ex));
			
		validate(ex.Start().y == 5 && ex.End().y == 5);
	}

	void TestBadClosureArg2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"	(alias Main EntryPoint.Main)\n"

			"   (archetype Sys.GetInt32 -> (Int32 value))\n"

			"	(function Eval (closure Sys.GetInt32 f) -> (Int32 value):\n"
			"		(f -> value)\n"
			"	)\n"

			"	(function Main -> (Int32 y):\n"
			"		(Sys.GetInt32 f = \n"
			"			(closure -> (Int32 y):\n"
			"				(y = 77)\n"
			"			)\n"
			"		)\n"
			"		(Sys.GetInt32 e = f)\n"
			"		(y = (Eval e))\n"
			"	)\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestBadClosureArg2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);

		validate(s_logger.ExceptionCount() == 1)

		ParseException ex;
		validate(s_logger.TryGetNextException(ex));

		validate(ex.Start().y == 13 && ex.End().y == 13);
	}

	void TestBadClosureArg3(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"	(alias Main EntryPoint.Main)\n"

			"   (archetype Sys.GetInt32 -> (Int32 value))\n"

			"	(function Main -> (Int32 y):\n"
			"		(Sys.GetInt32 f = \n"
			"			(closure -> (Int32 y):\n"
			"				(y = 77)\n"
			"			)\n"
			"		)\n"
			"		(array Sys.GetInt32 q 2)\n"
			"		(q.Push f)\n"
			"	)\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestBadClosureArg3");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);

		validate(s_logger.ExceptionCount() == 1)

		ParseException ex;
		validate(s_logger.TryGetNextException(ex));

		validate(ex.Start().y == 11 && ex.End().y == 11);
	}

	void TestClosureCapture(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"	(alias Main EntryPoint.Main)\n"

			"(archetype Sys.GetInt32 -> (Int32 value))\n"

			"(function Main -> (Int32 value):\n"
			"	(Int32 i = 417)"
			"	(Sys.GetInt32 f = \n"
			"		(closure -> (Int32 y):\n"
			"			(y = i)\n"
			"		)\n"
			"	)\n"
			"	(value = (f))"
			""

			")\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestClosureCapture");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_TERMINATED);

		validate(s_logger.ExceptionCount() == 0);

		auto value = vm.PopInt32();
		validate(value == 417);
	}

	void TestClosureCapture2(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"	(alias Main EntryPoint.Main)\n"

			"(archetype Sys.GetInt32 -> (Int32 value))\n"

			"(function Call (closure Sys.GetInt32 f) -> (Int32 result): (result = (f)) )"

			"(function Main -> (Int32 value):\n"
			"	(Int32 i = 2417)"
			"	(Sys.GetInt32 f = \n"
			"		(closure -> (Int32 y):\n"
			"			(y = i)\n"
			"		)\n"
			"	)\n"
			"	(Call f -> value)"
			""

			")\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestClosureCapture2");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_TERMINATED);

		validate(s_logger.ExceptionCount() == 0);

		auto value = vm.PopInt32();
		validate(value == 2417);
	}

	void TestBadClosureArg7(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"	(alias Main EntryPoint.Main)\n"

			"(archetype Sys.GetInt32 -> (Int32 value))\n"

			"(function Main -> (Int32 y):\n"
			"	(Sys.GetInt32 f = \n"
			"		(closure -> (Int32 y):\n"
			"			(y = 77)\n"
			"		)\n"
			"	)\n"
			"	(Sys.GetInt32 g = \n"
			"		(closure -> (Int32 y):\n"
			"			(y = 77)\n"
			"		)\n"
			"	)\n"
			"	(f = g)\n"
			""

			")\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestBadClosureArg7");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);

		validate(s_logger.ExceptionCount() == 1)

		ParseException ex;
		validate(s_logger.TryGetNextException(ex));

		validate(ex.Start().y == 15 && ex.End().y == 15);
	}

	void TestBadClosureArg6(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"	(alias Main EntryPoint.Main)\n"

			"(archetype Sys.GetInt32 -> (Int32 value))\n"

			"(function GetSix -> (Int32 value):\n"
			"	(value = 6)\n"
			")\n"

			"(function Main -> (Int32 y):\n"
			"	(Sys.GetInt32 f = \n"
			"		(closure -> (Int32 y):\n"
			"			(y = 77)\n"
			"		)\n"
			"	)\n"
			"	(map Int32 Sys.GetInt32 q)\n"
			"	(q.Insert 777 f)\n"
			""

			")\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestBadClosureArg6");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);

		validate(s_logger.ExceptionCount() == 1)

		ParseException ex;
		validate(s_logger.TryGetNextException(ex));

		validate(ex.Start().y == 14 && ex.End().y == 14);
	}

	void TestBadClosureArg5(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"	(alias Main EntryPoint.Main)\n"

			"(archetype Sys.GetInt32 -> (Int32 value))\n"

			"(function GetSix -> (Int32 value):\n"
			"	(value = 6)\n"
			")\n"

			"(function Main -> (Int32 y):\n"
			"	(Sys.GetInt32 f = \n"
			"		(closure -> (Int32 y):\n"
			"			(y = 77)\n"
			"		)\n"
			"	)\n"
			"	(list Sys.GetInt32 q)\n"
			"	(q.Append GetSix)\n"
			"	(node head = q.Tail)\n"
			"	(head.Prepend f)\n"
			""
			
			")\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestBadClosureArg5");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);

		validate(s_logger.ExceptionCount() == 1)

		ParseException ex;
		validate(s_logger.TryGetNextException(ex));

		validate(ex.Start().y == 16 && ex.End().y == 16);
	}

	void TestBadClosureArg4(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)\n"
			"	(alias Main EntryPoint.Main)\n"

			"   (archetype Sys.GetInt32 -> (Int32 value))\n"

			"	(function Main -> (Int32 y):\n"
			"		(Sys.GetInt32 f = \n"
			"			(closure -> (Int32 y):\n"
			"				(y = 77)\n"
			"			)\n"
			"		)\n"
			"		(list Sys.GetInt32 q)\n"
			"		(q.Prepend f)\n"
			"	)\n";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1},"TestBadClosureArg4");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);

		validate(s_logger.ExceptionCount() == 1)

			ParseException ex;
		validate(s_logger.TryGetNextException(ex));

		validate(ex.Start().y == 11 && ex.End().y == 11);
	}

	void TestNullArchetypeArg(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint) \n"
			"(function Main -> (Int32 result): \n"
			"		(Call 0 -> result)\n"
			")\n"
			"(function Call (EntryPoint.ToInt fn) -> (Int32 result):  (fn -> result))\n"
			"(archetype EntryPoint.ToInt -> (Int32 y)) \n"
			"(alias Main EntryPoint.Main) \n";
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestNullArchetype");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

   void TestOperatorOverload(IPublicScriptSystem& ss)
   {
      cstr srcCode =
         "(using Sys.Maths) \n"
         "(function AddVec3fVec3f (Vec3 a)(Vec3 b)(Vec3 c)-> : \n"
        "   (c.x = (a.x + b.x))"
        "   (c.y = (a.y + b.y))"
        "   (c.z = (a.z + b.z))"
        ")\n"
         "(namespace EntryPoint) \n"
         "(function Main -> (Float32 cx)(Float32 cy)(Float32 cz): \n"
        "		(Vec3 a = 2 4 6)\n"
        "		(Vec3 b = 5 7 9)\n"
        "		(Vec3 c = a + b)\n"
        "      (cx = c.x)\n"
        "      (cy = c.y)\n"
        "      (cz = c.z)\n"
        ")\n"
         "(alias Main EntryPoint.Main) \n";
      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestOperatorOverload");
      Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

      VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

      vm.Push(0); // Allocate stack space for the Vec3
      vm.Push(0);
      vm.Push(0); 
      EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
      ValidateExecution(result);
      float z = vm.PopFloat32();
      float y = vm.PopFloat32();
      float x = vm.PopFloat32();
      validate(x == 7.0f);
      validate(y == 11.0f);
      validate(z == 15.0f);
   }

   void TestOperatorOverload3(IPublicScriptSystem& ss)
   {
      cstr srcCode =
         "(using Sys.Maths) \n"
         "(function MultiplyVec3fFloat32 (Vec3 a)(Float32 b)(Vec3 c) -> : \n"
        "   (c.x = (a.x * b))"
        "   (c.y = (a.y * b))"
        "   (c.z = (a.z * b))"
        ")\n"
         "(namespace EntryPoint) \n"
         "(function Main -> (Float32 cx)(Float32 cy)(Float32 cz): \n"
        "		(Vec3 a = 2 4 6)\n"
        "		(Vec3 c = a * 3)\n"
        "      (cx = c.x)\n"
        "      (cy = c.y)\n"
        "      (cz = c.z)\n"
        ")\n"
         "(alias Main EntryPoint.Main) \n";
      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestOperatorOverload3");
      Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

      VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

      vm.Push(0); // Allocate stack space for the Vec3
      vm.Push(0);
      vm.Push(0);
      EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
      ValidateExecution(result);
      float z = vm.PopFloat32();
      float y = vm.PopFloat32();
      float x = vm.PopFloat32();
      validate(x == 6.0f);
      validate(y == 12.0f);
      validate(z == 18.0f);
   }

   void TestReturnInterfaceEx(IPublicScriptSystem& ss)
   {
	   cstr code =
		   "(namespace EntryPoint)"
		   "   (using Sys.Maths)"
		   "   (using Sys)"

		   "(interface Sys.IFidelio"
		   "   (Id -> (Int32 id))"
		   ")"

		   "(class Fidelio(implements IFidelio))"

		   "(method Fidelio.Id -> (Int32 id) :"
		   "  (id = 12)"
		   " )"

		   " (method Fidelio.Construct -> :)"

		   " (factory Sys.Fidelio Sys.IFidelio :"
		   " (construct Fidelio)"
		   ")"

		   "(function GetFidelio -> (IFidelio output) :"
		   " (IFidelio g(Fidelio))"
		   " (output = g)"
		   ")"

		   "(function Main -> (Int32 result) :"
		   " (IFidelio f = (GetFidelio))"
		   " (f.Id -> result)"
		   ")"

		   " (alias Main EntryPoint.Main)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(code, -1, Vec2i{ 0,0 }, "TestReturnInterfaceEx");
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(0);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);

	   int32 id = vm.PopInt32();

	   validate(id == 12);
   }

   void TestOperatorOverload2(IPublicScriptSystem& ss)
   {
      cstr srcCode =
         "(using Sys.Maths) \n"
         "(function SubtractVec3fVec3f (Vec3 a)(Vec3 b)(Vec3 c)-> : \n"
        "   (c.x = (a.x - b.x))"
        "   (c.y = (a.y - b.y))"
        "   (c.z = (a.z - b.z))"
        ")\n"
         "(namespace EntryPoint) \n"
         "(function Main -> (Float32 cx)(Float32 cy)(Float32 cz): \n"
        "		(Vec3 a = 2 4 6)\n"
        "		(Vec3 b = 5 7 9)\n"
        "		(Vec3 c)\n"
        "      (c = a - b)\n"
        "      (cx = c.x)\n"
        "      (cy = c.y)\n"
        "      (cz = c.z)\n"
        ")\n"
         "(alias Main EntryPoint.Main) \n";
      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestOperatorOverload2");
      Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

      VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

      vm.Push(0); // Allocate stack space for the Vec3
      vm.Push(0);
      vm.Push(0);
      EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
      ValidateExecution(result);
      float z = vm.PopFloat32();
      float y = vm.PopFloat32();
      float x = vm.PopFloat32();
      validate(x == -3.0f);
      validate(y == -3.0f);
      validate(z == -3.0f);
   }

   void Test1FieldInit(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(struct Arg (Int32 value))"
		   "(function Main -> (Int32 result): \n"
		   "		(Arg arg = 7)\n"
		   "		(result = arg.value)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(1); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();
	   validate(x == 7);
   }

   void TestAssignStringToStruct(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(struct Bitmap (Int32 x)(IString name)) \n"
		   "(function Main -> (Int32 result): \n"
		   "		(IString name = \"Geoff\"\n)"
		   "		(Bitmap bmp = 0 name)\n"
		   "		(bmp.name.Length -> result)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(1); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();
	   validate(x == 5);
   }

   void TestArrayWithEarlyReturn(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(function Main -> (Int32 result): \n"
		   "		(array Int32 indices 4)\n"
		   "		(indices.Push 1)\n"
		   "        (result = 5)\n"
		   "        (foreach i # indices (return))\n"
		   "        (result = 7)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(1); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();
	   validate(x == 5);
   }

   void TestArrayWithEarlyReturn2(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(function Main -> (Int32 result): \n"
		   "		(array Int32 indices 4)\n"
		   "		(indices.Push 1)\n"
		   "        (foreach i # indices  (if (1 == 0) (return)))\n"
		   "        (result = 7)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(1); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();
	   validate(x == 7);
   }

   void TestLoopBreak(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(function Main -> (Int32 result): \n"
		   "		(while (true)\n"
		   "			(Int32 i)\n"
		   "			(break)\n"
		   "        )\n"
		   "       (result = 7)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main)\n";
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(1); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();
	   validate(x == 7);
   }

   void TestLoopFinally(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		R"(
(namespace EntryPoint)
(using Sys)
(using Sys.Type)

(function Main -> (Int32 result):
	(Int32 i = 1)
	(while (i < 10)
		(Sys.Print "Hello Mum!")
		(continue)
		(Sys.Print "Hello Dad!")
	 finally (i += 1)
	)
	(result += i)
)

(alias Main EntryPoint.Main)
		)";
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(1); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();
	   validate(x == 10);
   }

   void TestLoopFinally2(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   R"(
(namespace EntryPoint)
(using Sys)
(using Sys.Type)
(using Sys.IO)

(function Main -> (Int32 result):
	(Int32 i = 0)
	(while (i < 10)
		(#printf "Hello Mum: " i "&n")
		(i += 1)
		(#printf "Hello Dad!" i  "&n")
	 finally
		(#printf "Hello stranger!" i "&n")
		(i += 1)
	)
	(result += i)
)

(alias Main EntryPoint.Main)
		)";
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(1); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();
	   printf("%d", x);
	   validate(x == 10);
   }

#pragma pack(push,1)
   struct Message
   {
	   void* hWnd;
	   int32 uMsg;
	   int32 result;
	   void* lParam;
	   void* wParam;
   };
#pragma pack(pop)


   void TestCPPCallback(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint)"
		   "	(alias Main EntryPoint.Main)"

		   "(struct Message"
		   "	(Pointer hWnd)"
		   "	(Int32 uMsg)"
		   "    (Int32 result)"
		   "	(Pointer lParam)"
		   "	(Pointer wParam)"
		   ")"

		   "(archetype Sys.DispatchEventHandler (Message message)->)"
		   "(function Main -> (Int32 result):"
		   "    (Sys.DispatchEventHandler handler = "
		   "		(closure (Message message)-> :"
		   "			(result = message.uMsg)"
		   "		)"
		   "	)"
		   "    (Sys.Example.Native.DispatchMessage handler)"
		   ")";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   AddNativeCallSecurity(ss, "Sys.Example.Native", __ROCOCO_FUNCTION__);
	   auto& ns = ss.AddNativeNamespace("Sys.Example.Native");

	   struct MessageDispatcher
	   {
		   static void Dispatch(NativeCallEnvironment& nce)
		   {
			   ArchetypeCallback cb;
			   ReadInput(0, cb, nce);

			   Message msg{ nullptr, 1001, 0, nullptr, nullptr };
			   nce.ss.DispatchToSexyClosure(&msg, cb);
		   }
	   };

	   IModule* srcModule = ss.AddTree(*tree);

	   ss.SetNativeCallTarget(srcModule);
	   ss.AddNativeCall(ns, MessageDispatcher::Dispatch, nullptr, "DispatchMessage (Sys.DispatchEventHandler handler) -> ", __FILE__, __LINE__, true, 0);
	   ss.SetNativeCallTarget(nullptr);
	   ss.Compile();

	   const INamespace* entryPointNS = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
	   SetProgramAndEntryPoint(ss.PublicProgramObject(), *entryPointNS, "Main");

	   auto& vm = ss.PublicProgramObject().VirtualMachine();

	   vm.Push(1815); // Allocate stack space for the int32 result
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();
	   validate(x == 1001);
   }

   void TestStructInClassSetFromMethod(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(using Sys.Maths)\n"
		   "(class Bubble (defines Sys.IBubble)(Vec2i span))\n"
		   "(method Bubble.Construct : )\n"
		   "(factory Sys.NewBubble Sys.IBubble : (construct Bubble))\n"
		   "(method Bubble.Expand -> : \n"
		   "    (Vec2i newSpan = 82 93)\n"
		   "	(this.span = newSpan)\n"
		   ")\n"
		   "(method Bubble.GetX -> (Int32 x) : \n"
		   "	(x = this.span.x)\n"
		   ")\n"
		   "(function Main -> (Int32 result): \n"
		   "		(Sys.IBubble bubble (Sys.NewBubble))\n"
		   "        (bubble.Expand)\n"
		   "        (result = bubble.GetX)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(1); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();
	   validate(x == 82);
   }

   void TestMemberwiseConstructWithInterface(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(using Sys.Maths)\n"
		   "(class Bubble (defines Sys.IBubble)(Vec2i span))\n"
		   "(method Bubble.Construct : )\n"
		   "(factory Sys.NewBubble Sys.IBubble : (construct Bubble))\n"
		   "(method Bubble.Expand -> : \n"
		   "    (Vec2i newSpan = 82 93)\n"
		   "	(this.span = newSpan)\n"
		   ")\n"
		   "(method Bubble.GetX -> (Int32 x) : \n"
		   "	(x = this.span.x)\n"
		   ")\n"
		   "(struct CosmicRay (Sys.IBubble bubble) (Int32 i))"
		   "(function Main -> (Int32 result): \n"
		   "		(Sys.IBubble bubble (Sys.NewBubble))\n"
		   "		(CosmicRay ray = bubble 5)\n"
		   "        (ray.bubble.Expand)\n"
		   "        (result = ray.bubble.GetX)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(1); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();

	   validate(x == 82);
   }

   void TestZeroDefaults(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(using Sys.Maths)\n"

		   "(function NullResults -> (Sys.Type.IException ex)(Int32 value)(Bool bValue):\n"
		   ")\n"

		   "(function Main -> (Int32 result): \n"
		   "        (Sys.Type.IException ex)\n"
		   "        (Int32 value)\n"
		   "        (Bool bValue)\n"
		   "		(NullResults -> ex value bValue)\n"
		   "        (if (ex ?) (Sys.Throw 0 \"Bad interface\"))\n"
		   "        (if (value != 0) (Sys.Throw 0 \"Bad int32\"))\n"
		   "        (if (bValue != false) (Sys.Throw 0 \"Bad boolean\"))\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();

	   validate(x == 0); // Should be set to zero by Main
   }

   void TestPublishAPI(IPublicScriptSystem& ss)
   {
	   cstr src = 
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(using Sys.Maths)\n"

		   "(function Main -> (Int32 result): \n"
		   "	(Sys.PublishAPI)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   if (result == EXECUTERESULT_THROWN)
	   {
		   ValidateLogs();
	   }
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();

	   validate(x == 0); // Should be set to zero by Main   
   }

   void TestStringToChar(IPublicScriptSystem& ss)
   {
	   cstr src =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(using Sys.Maths)\n"

		   "(function Main -> (Int32 result): \n"
		   "	(IString meToo = \"Harvey, what have you been up to?\")\n"
		   "    (Int32 c = (meToo 5))\n"
		   "    (result = c)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();

	   validate(x == 'y'); // Should be set to zero by Main   
   }

   void TestStringToCharFail (IPublicScriptSystem& ss)
   {
	   cstr src =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(using Sys.Maths)\n"

		   "(function Main -> (Int32 result): \n"
		   "	(IString meToo = \"Harvey, what have you been up to?\")\n"
		   "    (Int32 c = (meToo 75))\n"
		   "    (result = c)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_THROWN);
	   ValidateLogs();
   }

   void TestArrayOfInterfacesBuilder(IPublicScriptSystem& ss)
   {
	   cstr src =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(using Sys.Maths)\n"

		   "(interface Sys.IRobot\n"
		   " (Think ->)\n"
		   ")\n"

		   "(class Robot\n"
		   " (implements Sys.IRobot)\n"
		   ")\n"

		   "(method Robot.Think -> : )\n"

		   "(method Robot.Construct : )\n"

		   "(factory Sys.NewRobot Sys.IRobot : (construct Robot))\n"

		   "(function Main -> (Int32 result): \n"
		   "	(array Sys.IRobot robots 4)\n"
		   "    (Sys.IRobot robby (Sys.NewRobot))\n"
		   "	(robots.Push robby)\n"
		   "    (foreach robot # robots (robot.Think))\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);

	   int x = vm.PopInt32();
	   validate(x == 0);
	   ValidateLogs();
   }

   void TestEmptyArrayOfInterfaces(IPublicScriptSystem& ss)
   {
	   cstr src =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(using Sys.Maths)\n"

		   "(function Main -> (Int32 result): \n"
		   "	(array Sys.Type.IString items 4)\n"
		   "    (foreach item # items (Sys.Print item))\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);

	   int x = vm.PopInt32();
	   validate(x == 0);
	   ValidateLogs();
   }

   void TestSerialize(IPublicScriptSystem& ss)
   {
	   cstr src =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(using Sys.Maths)\n"
		   "(using Sys.Reflection)\n"

		   "(function Main -> (Int32 result): \n"
		   "	(IExpression s = ' (I32 x 135))\n"
		   "	(Vec2i v)\n"
		   "	(serialize s -> v)\n"
		   "    (result = v.x)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);

	   int x = vm.PopInt32();
	   validate(x == 135);
	   ValidateLogs();
   }

   void TestAppendInterfaceInsideMethod(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(class Robot (defines Sys.Type.IRobot)
			(list IString logItems)
		)

		(method Robot.Construct :
			(list IString logItems)
			(this.logItems = logItems)
		)

		(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))

		(method Robot.AddJim -> (Int32 length):
			(this.logItems.Append "Jim")
			(length = this.logItems.Length)
		)

		(function Main -> (Int32 result):
			(Sys.Type.IRobot robot (Sys.Type.NewRobot))
			(robot.AddJim -> result)
			(robot.AddJim -> result)
			(robot.AddJim -> result)
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int x = vm.PopInt32();
	   validate(x == 3);
   }

   void TestAppendInterfaceInsideMethod2(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(class Robot (defines Sys.Type.IRobot)
			(list IString logItems)
		)

		(method Robot.Construct :
			(list IString logItems)
			(this.logItems = logItems)
		)

		(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))

		(method Robot.AddJim -> (Int32 result):
			(IString memo (Memo "Jim"))
			(this.logItems.Append memo)
			(result = this.logItems.Length)
		)

		(function Main -> (Int32 result):
			(Sys.Type.IRobot robot (Sys.Type.NewRobot))
			(robot.AddJim)
			(robot.AddJim)
			(robot.AddJim -> result)
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int x = vm.PopInt32();
	   if (x != 3)
	   {
		   printf("x = %d\n", x);
	   }
	   validate(x == 3);
   }

   void TestGetAccessorSemantics(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(class Robot (defines Sys.Type.IRobot)
		)

		(method Robot.Construct :
		)

		(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))

		(method Robot.NineThousand -> (Int32 result):
			(result = 9000)
		)

		(function Main -> (Int32 result):
			(IRobot robot (NewRobot))
			(Int32 total = 5 + robot.NineThousand)
			(total += robot.NineThousand)
			(total += (5 + robot.NineThousand))
			(result = total)
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int x = vm.PopInt32();
	   int expectation = 27010;
	   if (x != expectation)
	   {
		   printf("x = %d\n", x);
	   }
	   validate(x == expectation);
   }

   void TestGetAccessorSemantics3(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(class Robot (defines Sys.Type.IRobot)
		)

		(method Robot.Construct :
		)

		(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))

		(method Robot.IsHot -> (Bool result):
			(result = true)
		)

		(method Robot.IsGlowing -> (Bool result):
			(result = true)
		)

		(function Main -> (Int32 result):
			(IRobot robot (NewRobot))
			(Bool isSunny = true)
			(Bool isVeryHot = isSunny and robot.IsHot)
			(if isVeryHot (result = 9005))
			(Bool isVeryVeryHot = robot.IsHot and isSunny)
			(if isVeryVeryHot (result += 9005))
			(Bool isVeryVeryVeryHot = robot.IsHot and robot.IsGlowing)
			(if isVeryVeryVeryHot (result += 9005))
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int x = vm.PopInt32();
	   int expectation = 27015;
	   if (x != expectation)
	   {
		   printf("x = %d\n", x);
	   }
	   validate(x == expectation);
   }

   void TestTernary(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(class Robot (defines Sys.Type.IRobot)
		)

		(method Robot.Construct :
		)

		(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))

		(method Robot.IsHot -> (Bool result):
			(result = true)
		)

		(method Robot.IsGlowing -> (Bool result):
			(result = true)
		)

		(function Main -> (Int32 result):
			(IRobot robot (NewRobot))
			(result = (?? robot.IsHot 9005 0))
			(result += (?? (not robot.IsHot) 0 70))
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int x = vm.PopInt32();
	   int expectation = 9075;
	   if (x != expectation)
	   {
		   printf("x = %d\n", x);
	   }
	   validate(x == expectation);
   }

   void TestBuilderWithGetAccessor(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(class Robot (defines Sys.Type.IRobot)
		)

		(method Robot.Construct :
		)

		(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))

		(method Robot.Name -> (IString name):
			(name = "Geoff")
		)

		(function Main -> (Int32 result):
			(IRobot robot (NewRobot))
			(IStringBuilder sb = (Sys.Type.NewParagraphBuilder))
			(sb robot.Name)
			(result = sb.Length)
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(0); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int x = vm.PopInt32();
	   int expectation = 5;
	   if (x != expectation)
	   {
		   printf("x = %d\n", x);
	   }
	   validate(x == expectation);
   }

   void TestBuilderWithGetAccessor2(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(class Robot (defines Sys.Type.IRobot)
		)

		(method Robot.Construct :
		)

		(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))

		(method Robot.Name -> (IString name):
			(name = "Geoff")
		)

		(function Main -> (Int32 result):
			(IRobot robot (NewRobot))
			(IStringBuilder sb = (Sys.Type.NewParagraphBuilder))
			(#build sb "Hello " robot.Name)
			(result = sb.Length)
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(0); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int x = vm.PopInt32();
	   int expectation = 11;
	   if (x != expectation)
	   {
		   printf("x = %d\n", x);
	   }
	   validate(x == expectation);
   }

   void TestExpressionProxies(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)

		(function Main -> (Int32 result):
			(result = 6 + 5 + 4 + 3 + 2 + 1)
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int x = vm.PopInt32();
	   int expectation = 21;
	   if (x != expectation)
	   {
		   printf("x = %d\n", x);
	   }
	   validate(x == expectation);
   }

   void TestGetAccessorSemantics2(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(class Robot (defines Sys.Type.IRobot)
		)

		(method Robot.Construct :
		)

		(factory Sys.Type.NewRobot Sys.Type.IRobot : (construct Robot))

		(method Robot.NineThousand -> (Int32 result):
			(result = 9000)
		)

		(function Main -> (Int32 result):
			(IRobot robot (NewRobot))
			(Int32 total = (robot.NineThousand + 5))
			(result = total)
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int x = vm.PopInt32();
	   int expectation = 9005;
	   if (x != expectation)
	   {
		   printf("x = %d\n", x);
	   }
	   validate(x == expectation);
   }

   void TestNegateVariable(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(function Main -> (Int32 result):
			(Int32 value = -10)
			(result = -value)
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int x = vm.PopInt32();
	   int expectation = 10;
	   if (x != expectation)
	   {
		   printf("x = %d\n", x);
	   }
	   validate(x == expectation);
   }

   void TestNegateVariable2(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(function Main -> (Float32 result):
			(Float32 value = -10)
			(result = (5 * -value))
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(0.0f); // Allocate stack space for the float x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   float x = vm.PopFloat32();
	   float expectation = 50.0f;
	   if (x != expectation)
	   {
		   printf("x = %f\n", x);
	   }
	   validate(x == expectation);
   }

   void TestNegateVariable3(IPublicScriptSystem& ss)
   {
	   cstr src =
R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(function Main -> (Float64 result):
			(Float64 value = -10)
			(result = 1 - (5 * -value))
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(0.0); // Allocate stack space for the double x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   double x = vm.PopFloat64();
	   double expectation = -49.0;
	   if (x != expectation)
	   {
		   printf("x = %lf\n", x);
	   }
	   validate(x == expectation);
   }

   void TestNegateVariable4(IPublicScriptSystem& ss)
   {
	   cstr src =
R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(function Main -> (Int64 result):
			(Int64 value = 10)
			(result = -40 / -value)
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(0LL); // Allocate stack space for the int64 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int64 x = vm.PopInt64();
	   int64 expectation = 4;
	   if (x != expectation)
	   {
		   printf("x = %lld\n", x);
	   }
	   validate(x == expectation);
   }

   void TestNegateVariable5(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)
		(using Sys.Reflection)
		(using Sys.Type.Strings)

		(function Main -> (Float32 result):
			(Float32 value = 2.0)
			(result = (-5 + -5) / -value)
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(0.0f); // Allocate stack space for the float x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   float x = vm.PopFloat32();
	   float expectation = 5.0f;
	   if (x != expectation)
	   {
		   printf("x = %f\n", x);
	   }
	   validate(x == expectation);
   }

   void TestExpressionProxies2(IPublicScriptSystem& ss)
   {
	   cstr src =
		   R"(
		(namespace EntryPoint)
		(using Sys)
		(using Sys.Type)
		(using Sys.Maths)

		(function Main -> (Int32 result):
			(Int32 x = Sys.Maths.F32.ToInt32 7.0)
			(result = x)
		)

		(alias Main EntryPoint.Main)
)";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);
	   ValidateLogs();

	   int x = vm.PopInt32();
	   int expectation = 7;
	   if (x != expectation)
	   {
		   printf("x = %d\n", x);
	   }
	   validate(x == expectation);
   }

   void TestAddNativeReflectionCall(IPublicScriptSystem& ss)
   {
	   static int callCount = 0;

	   struct Reflector
	   {
		   static void ValidateVec2i(int64* context, ReflectionArguments& args)
		   {
			   int32* lhsValue = (int32*)args.lhsData;
			   validate(*lhsValue == 5);
			   validate(Eq("Int32", args.lhsType.Name()));
			   validate(context != nullptr);
			   validate(*context == 42);
			   const Vec2i* pVec = (const Vec2i*) args.rhsData;
			   validate(pVec != nullptr);
			   validate(pVec->x == 5);
			   validate(pVec->y == 6);
			   callCount++;
		   }

		   static void ValidateFloat64(int64* context, ReflectionArguments& args)
		   {
			   validate(context != nullptr);
			   validate(*context == 42);
			   const double* pValue = (const double*)args.rhsData;
			   validate(pValue != nullptr);
			   validate(*pValue == 7);
			   callCount++;
		   }

		   // This demos how a reflection function should obtain the concrete type from the stub by using InterfaceToInstance on the data argument.
		   static void ValidateStringConstant(int64* context, ReflectionArguments& args)
		   {
			   // Note here, the type is a Null-IString(i.e an IString interface)
			   validate(context != nullptr);
			   validate(*context == 42);
			   InterfacePointer pInterface = (InterfacePointer)args.rhsData;
			   auto* stub = InterfaceToInstance(pInterface);
			   // The type in the stub is a StringConstant, because the IString was initialized to a StringConstant object
			   // If we wanted to do type inference we should inspect stub->Desc->TypeInfo which provides the type of the concrete object (a StringConstant)
			   auto* sc = (CStringConstant*) stub;
			   validate(Eq("St-Bernards", sc->pointer));
			   callCount++;
		   }
	   };

	   int64 context = 42;

	   // In our demo we use 3 different functions for each data type in the sample code, but a real system would probably
	   // make do with one function and then use the type argument in the callback function to decide how to interpret the data
	   // The reflect keyword does not care about the types supplied to the reflection function
	   ss.AddNativeReflectionCall("ValidateVec2i", Reflector::ValidateVec2i, &context);
	   ss.AddNativeReflectionCall("ValidateFloat64", Reflector::ValidateFloat64, &context);
	   ss.AddNativeReflectionCall("ValidateStringConstant", Reflector::ValidateStringConstant, &context);
		
	   cstr src =
		   "(namespace EntryPoint) \n"
		   "(using Sys) \n"
		   "(using Sys.Type) \n"
		   "(using Sys.Maths)\n"
		   "(using Sys.Reflection)\n"

		   "(struct ArchiveContext"
				"(IString fileName)"
		   ")"

		   "(function Main -> (Int32 result): \n"	
		   "	(Vec2i v = 5 6)\n"
		   "    (Int32 reflectionControl = 5)"
		   "	(reflect ValidateVec2i reflectionControl v)\n"
		   "	(Float64 galaxyFactor = 7)\n"
		   "	(reflect ValidateFloat64 reflectionControl galaxyFactor)\n"
		   "    (IString dog = \"St-Bernards\")"
		   "    (reflect ValidateStringConstant reflectionControl dog)"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x

	   vm.Core().SetLogger(&s_logger);
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   validate(result == EXECUTERESULT_TERMINATED);

	   validate(callCount == 3);

	   ValidateLogs();
   }

   // We dont' run this with the other tests, only in itself. By commenting in/out the line (player.NoOperation) we can determine the cost of a million virtual calls
   // This was used to determine that self-modifying-code increased virtual table lookup speed by 2.5x, from 40M calls per sec to 
   // 100M calls per sec on the test platform's i7-11700k @2.5GHz
   void TestVirtualCallSpeed(IPublicScriptSystem& ss)
   {
	   cstr srcCode =
		   "(namespace EntryPoint)"
		   "(using EntryPoint)"
		   "(function Main -> (Int32 result):"
		   "    (IPlayer player (NewPlayer))"
		   "    (for (Int32 i = 0)(i < 1000000)(i += 1)"
		   "	    (player.NoOperation)"
		   "	)"	
		   ")"
		   "(alias Main EntryPoint.Main)"

		   "(interface EntryPoint.IPlayer"
		   "    (NoOperation -> )"
		   ")"

		   "(class Player (implements IPlayer)"
		   "    (Int32 id)"
		   ")"

		   "(method Player.NoOperation -> :"
		   ")"

		   "(method Player.Construct -> :"
		   "	(this.id = 0)"
		   ")"

		   "(factory EntryPoint.NewPlayer IPlayer :"
		   "   (construct Player)"
		   ")";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestClassInstance");
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(0); // Allocate stack space for the int32 result
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();
	   validate(x == 0);
   }


   void TestArrays()
   {
	   TEST(TestArrayNull);
	   TEST(TestArrayInClass);
	   TEST(TestArrayOfInterfacesBuilder);
	   TEST(TestPushStructToArray);
	   TEST(TestArrayInt32);
	   TEST(TestArrayInt32Expand);
	   TEST(TestArrayInt32_2);
	   TEST(TestArrayInt32_3);
	   TEST(TestArrayInt32_4);
	   TEST(TestArrayInt32_5);
	   TEST(TestArrayInt32_6);
	   TEST(TestArrayInt32_7);
	   TEST(TestArrayInt32_8);
	   TEST(TestArrayInt32_9);
	   TEST(TestArrayInt32Reassign);
	   TEST(TestArrayInStructWithThis);
	   TEST(TestArrayRef);
	   TEST(TestReturnArrayRef);
	   TEST(TestReturnArrayRefAndIgnore);
	   TEST(TestArrayElementIsClass);
	   TEST(TestArrayFloat64);
	   TEST(TestArrayStruct);
	   TEST(TestArrayStruct_2);
	   TEST(TestArrayStruct_3);
	   TEST(TestEmptyArrayOfInterfaces);
	   TEST(TestArrayOfInterfacesBuilder);

	   TEST(TestArrayStrongTyping);
	   TEST(TestArrayStrongTyping2);
	   TEST(TestArrayStrongTyping3);

	   TEST(TestNestedArrayEnumeration);

	   TEST(TestArrayPushViaClosure);
	   TEST(TestArrayPushViaClosure2);
	   TEST(TestArrayPushViaClosure3);
	   TEST(TestConstructInArray);
	   TEST(TestArrayForeachElementInArray);

	   TEST(TestArrayForeachEachElementInArrayWithoutIndex);
	   TEST(TestArrayElementDeconstruct);

	   TEST(TestArrayElementDeconstructWhenThrown);
	   TEST(TestArrayWithinArrayDeconstruct);
	   TEST(TestArrayElementLockRef);
	   TEST(TestArrayRefMember);
	   TEST(TestArrayRefMember2);
	   TEST(TestArrayWithEarlyReturn);
	   TEST(TestArrayWithEarlyReturn2);
	   TEST(TestArrayForeachWithinForEach);
	   TEST(TestArrayForeachAndThrow);
	   TEST(TestArrayProxy);
	   TEST(TestArrayClear);
	   TEST(TestArrayPushBool);
	   TEST(TestArrayLast);
	   TEST(TestReturnArrayRefAndIgnore);
   }

   void TestLists()
   {
	   TEST(TestAppendInterfaceInsideMethod);
	   TEST(TestAppendInterfaceInsideMethod2);
	   TEST(TestMapInsertCorrectRefs);
	   TEST(TestLinkedListNodeInline);
	   TEST(TestLinkedListContained);
	   TEST(TestLinkedListOfLists);
	   TEST(TestLinkedListForeach3);
	   TEST(TestLinkedList11);
	   TEST(TestLinkedList);
	   TEST(TestLinkedList2);
	   TEST(TestLinkedList3);
	   TEST(TestLinkedList4);
	   TEST(TestLinkedList6);
	   TEST(TestLinkedList7);
	   TEST(TestLinkedList8);
	   TEST(TestLinkedList9);
	   TEST(TestLinkedList10);
	   
	   TEST(TestLinkedListOfInterfaces);
	   TEST(TestLinkedListOfInterfaces2);
	   TEST(TestLinkedListForeach1);
	   TEST(TestLinkedListForeach2);
	   TEST(TestLinkedListForeach4);
	   TEST(TestLinkedListForeach5);
	   TEST(TestLinkedListForeach6);
	   TEST(TestLinkedListForeach8);
	   
	   TEST(TestLinkedListOfArchetypes);
	   TEST(TestListStruct);
	   TEST(TestListStruct2);
	   TEST(TestListStruct3);
	   
	   TEST(TestListStrongTyping);
	   
	   TEST(TestListDeleteHeadAndThrow);
	   
	   TEST(TestListReverseEnumeration);
	   TEST(TestLinkedListForeach7);
	   TEST(TestListInClass);
   }

   void TestMaps()
   {  
	   TEST(TestMapOverwriteValue);
	   TEST(TestMapInClass);
	   TEST3(TestMapKey);
	   TEST3(TestMapGetKey);
	   TEST3(TestMapGetKey64);
	   TEST3(TestMapStringInsertByVariable);
	   TEST3(TestMapStringInsertByVariable2);
	   TEST(TestEmptyMap);
	   TEST(TestMap);
	   TEST(TestMapStringToString);
	   TEST(TestMap2);
	   TEST(TestMap3);
	   TEST(TestMap4);
	   TEST(TestMap5);
	   TEST(TestMap6);
	   TEST(TestMap7);
	   TEST(TestMap8);
	   TEST(TestArrayAssignEmpty);
	   TEST(TestArrayNull);
	   TEST(TestMapValueInterface);
	   TEST(TestMapOfArchetypes);
	   TEST(TestMapOverwriteValue);
	   TEST(TestMapOverwriteValue64);
	   TEST(TestMapIndexedByInt32);
	   TEST(TestMapIndexedByFloat64);
	   TEST(TestDeleteKey);
	   TEST(TestMapValueStruct);
	   TEST(TestMapValueConstruct);
	   TEST(TestMapForeach1);
	   TEST(TestMapForeach2);
	   TEST(TestMapInStruct);
	   TEST(TestMapInMap);
	   TEST(TestMapStrongTyping);
	   TEST(TestMapThrowAndCleanup);
	   TEST(TestMapThrowAndCleanup2);
	   TEST(TestMapStringToString);
	   TEST3(TestMapKey);
   }

   void TestMaths()
   {
	   TEST(TestNegateVariable);
	   TEST(TestNegateVariable2);
	   TEST(TestNegateVariable3);
	   TEST(TestNegateVariable4);
	   TEST(TestNegateVariable5);

	   TEST(TestIsInfinity);
	   TEST(TestIsFinite);
	   TEST(TestIsNormal);
	   TEST(TestIsQuietNan);
	   TEST(TestIsInfinity64);
	   TEST(TestIsFinite64);
	   TEST(TestIsNormal64);
	   TEST(TestIsQuietNan64);
	   TEST(TestMathSinCos);
	   TEST(TestMathSinCosF);
	   TEST(TestMathTan);
	   TEST(TestMathTanF);
	   TEST(TestMathArcTan);
	   TEST(TestMathArcTanF);
	   TEST(TestMathArcSin);
	   TEST(TestMathArcSinF);
	   TEST(TestMathArcCos);
	   TEST(TestMathArcCosF);
	   TEST(TestMathArcTanGrad);
	   TEST(TestMathArcTanGradF);

	   TEST(TestMathSinhCosh);
	   TEST(TestMathSinhCoshF);
	   TEST(TestMathTanh);
	   TEST(TestMathTanhF);

	   TEST(TestMathExp);
	   TEST(TestMathExpF);
	   TEST(TestMathLogN);
	   TEST(TestMathLogNF);

	   TEST(TestMathLog10);
	   TEST(TestMathLog10F);

	   TEST(TestMathPow);
	   TEST(TestMathPowF);
	   TEST(TestMathAbs);
	   TEST(TestMathAbsF);
	   TEST(TestMathAbsInt32);
	   TEST(TestMathAbsInt64);

	   TEST(TestMathSqrt);
	   TEST(TestMathSqrtF);

	   TEST(TestMathCeil);
	   TEST(TestMathCeilF);
	   TEST(TestMathFloor);
	   TEST(TestMathFloorF);

	   TEST(TestMathModInt32);
	   TEST(TestMathModInt64);

	   TEST(TestMathShiftInt32);
	   TEST(TestMathShiftInt64);

	   TEST(TestMinMaxInt32);
	   TEST(TestMinMaxInt64);
	   TEST(TestMinMaxFloat32);
	   TEST(TestMinMaxFloat64);

	   TEST(TestConstruction);

	   TEST(TestLimitsInt32);
	   TEST(TestLimitsInt64);
	   TEST(TestLimitsFloat32);
	   TEST(TestLimitsFloat64);
   }

	// (foreach-enum <name> <value> in <enum-type>: <body>) runs the body once per enum value in declaration order.
	// <name> is an IString referencing a string constant of the value's name, <value> an Int64 of the value
	void TestForEachEnumNamesInOrder(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 256))
				(foreach-enum name value in Day:
					(sb name)
					(sb ",")
				)
				(Int32 diff = (Sys.Type.Strings.Compare sb "Monday,Tuesday,Wednesday,Thursday,Friday,"))
				(if (diff == 0) (result = sb.Length) else (result = -1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 41);
	}

	void TestForEachEnumValuesInOrder(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int64 acc = 0)
				(foreach-enum name value in Day:
					(acc = ((acc * 10) + value))
				)
				(if (acc == 12345) (result = 1) else (result = 0))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 1);
	}

	void TestForEachEnumNameBuilderRoundTrip(IPublicScriptSystem& ss)
	{
		// The name IString can be appended to a builder and compared back with the name
		cstr body = R"(
			(function Main -> (Int32 result):
				(result = 0)
				(foreach-enum name value in Day:
					(IStringBuilder a = (Sys.Type.NewStringBuilder 64))
					(a name)
					(Int32 diff = (Sys.Type.Strings.Compare a name))
					(if (diff == 0) (result += 1))
				)
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 5);
	}

	void TestForEachEnumInt64Values(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int64 acc = 0)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(foreach-enum name value in BigFlags:
					(acc = (acc + value))
					(sb name)
					(sb " ")
				)
				(result = 0)
				(Int64 expected = 0x100000001)
				(if (acc == expected) (result += 1))
				(Int32 diff = (Sys.Type.Strings.Compare sb "Low High "))
				(if (diff == 0) (result += 10))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 11);
	}

	void TestForEachEnumInt32Widened(IPublicScriptSystem& ss)
	{
		// Access@Sign is -2147483648 in an Int32 enum; it is sign-extended to Int64
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int64 acc = 0)
				(foreach-enum name value in Access:
					(acc = (acc + value))
				)
				(if (acc == -2147483641) (result = 1) else (result = 0))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 1);
	}

	void TestForEachEnumFlags(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int64 acc = 0)
				(Int32 count = 0)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 64))
				(foreach-enum flag bits in Alignment:
					(acc = (acc + bits))
					(count += 1)
					(sb flag)
					(sb " ")
				)
				(result = count)
				(if (acc == 15) (result += 10))
				(Int32 diff = (Sys.Type.Strings.Compare sb "None Top Left Bottom Right "))
				(if (diff == 0) (result += 100))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 115);
	}

	void TestForEachEnumNested(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int64 acc = 0)
				(IStringBuilder sb = (Sys.Type.NewStringBuilder 128))
				(foreach-enum a av in Edge:
					(foreach-enum b bv in Edge:
						(acc = (acc + ((av * 10) + bv)))
						(sb a)
						(sb b)
						(sb ",")
					)
				)
				(result = 0)
				(if (acc == 66) (result += 1))
				(Int32 diff = (Sys.Type.Strings.Compare sb "NorthNorth,NorthSouth,SouthNorth,SouthSouth,"))
				(if (diff == 0) (result += 10))
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 11);
	}

	void TestForEachEnumNameComparedWithLiteral(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(result = 0)
				(foreach-enum name value in Day:
					(IStringBuilder sb = (Sys.Type.NewStringBuilder 32))
					(sb "[")
					(sb name)
					(sb "]")
					(Int32 d = (Sys.Type.Strings.Compare name "Wednesday"))
					(Int32 e = (Sys.Type.Strings.Compare sb "[Wednesday]"))
					(if ((d == 0) and (e == 0)) (result = 1000))
					(if (d == 0) (result += 3))
					(if (e == 0) (result += 30))
				)
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 1033);
	}

	void TestForEachEnumBreak(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int64 acc = 0)
				(foreach-enum name value in Day:
					(IStringBuilder local = (Sys.Type.NewStringBuilder 16))
					(local name)
					(if (value == 3) (break))
					(acc = (acc + value))
				)
				(result = 100)
				(if (acc == 3) (result += 1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 101);
	}

	void TestForEachEnumContinue(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int64 acc = 0)
				(foreach-enum name value in Day:
					(IStringBuilder local = (Sys.Type.NewStringBuilder 16))
					(local name)
					(if (value == 3) (continue))
					(acc = (acc + value))
				)
				(result = 100)
				(if (acc == 12) (result += 1))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 101);
	}

	void TestForEachEnumBreakInnerOnly(IPublicScriptSystem& ss)
	{
		// break in the inner loop leaves only the inner loop; continue in the outer loop skips the rest of the outer body
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int64 acc = 0)
				(foreach-enum a av in Day:
					(if (av == 4) (continue))
					(foreach-enum b bv in Day:
						(if (bv > av) (break))
						(acc = (acc + 1))
					)
					(acc = (acc + 100))
				)
				(if (acc == 411) (result = 1) else (result = 0))
			)
		)";

		// av = 1,2,3,5 contribute 1+2+3+5 inner counts = 11 and 4 x 100
		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 1);
	}

	void TestForEachEnumBreakInsideWhile(IPublicScriptSystem& ss)
	{
		// break inside foreach-enum leaves the foreach-enum, not the enclosing while; break after it leaves the while
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int32 i = 0)
				(Int64 acc = 0)
				(while (i < 3)
					(i += 1)
					(foreach-enum name value in Day:
						(if (value == 2) (break))
						(acc = (acc + value))
					)
					(if (i == 2) (break))
				)
				(result = i)
				(if (acc == 2) (result += 10))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 12);
	}

	void TestForEachEnumInsideWhileContinue(IPublicScriptSystem& ss)
	{
		// A while loop inside the foreach-enum body has its own break and continue
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int64 acc = 0)
				(foreach-enum name value in Day:
					(Int64 j = 0)
					(while (j < value)
						(j = (j + 1))
						(if (j == 2) (continue))
						(acc = (acc + 1))
					)
				)
				(if (acc == 11) (result = 1) else (result = 0))
			)
		)";

		// Inner counts per value 1..5 excluding j == 2: 1,1,2,3,4
		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 1);
	}

	void TestForEachEnumEmptyBody(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum name value in Day:)
				(result = 7)
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 7);
	}

	void TestForEachEnumSeparateColon(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(result = 0)
				(foreach-enum name value in Day :
					(result += 1)
				)
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 5);
	}

	void TestForEachEnumQualifiedType(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(result = 0)
				(foreach-enum name value in Sys.Type.Edge:
					(result += 1)
				)
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 2);
	}

	void TestForEachEnumNamesReusedAfterLoop(IPublicScriptSystem& ss)
	{
		// The loop variables are scoped to the loop, so the same names can be used by a later loop or declaration
		cstr body = R"(
			(function Main -> (Int32 result):
				(result = 0)
				(foreach-enum name value in Day: (result += 1))
				(foreach-enum name value in Edge: (result += 10))
				(Int32 value = 100)
				(result += value)
			)
		)";

		validate(RunEnumFlagsTest(ss, body, __FUNCTION__) == 125);
	}

	void TestForEachEnumReturnFromBody(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Find (IString target) -> (Int32 r):
				(r = -1)
				(foreach-enum name value in Day:
					(IStringBuilder local = (Sys.Type.NewStringBuilder 16))
					(Int32 d = (Sys.Type.Strings.Compare name target))
					(if (d == 0)
						(r = (local.Length + 30))
						(return)
					)
				)
			)

			(function Main -> (Int32 result):
				(result = ((Find "Wednesday") + (Find "Sunday")))
			)
		)";

		validate(RunEnumPreambleTest(ss, body, __FUNCTION__) == 29);
	}

	void TestForEachEnumNoValuesRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(strong Bare (Int32))
			(alias Bare Sys.Type.Bare)

			(function Main -> (Int32 result):
				(foreach-enum name value in Bare: (result = 1))
			)
		)";

		RunEnumFlagsFailTest(ss, body, __FUNCTION__, "foreach-enum requires an enum type, but Bare has no enum values");
	}

	void TestForEachEnumNotStrongRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum name value in Int32: (result = 1))
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "is not a strong type");
	}

	void TestForEachEnumUnknownTypeRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum name value in Nowhere: (result = 1))
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: unknown type Nowhere");
	}

	void TestForEachEnumLowerCaseTypeRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Day day = .Monday)
				(foreach-enum name value in day: (result = 1))
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: unknown type day");
	}

	void TestForEachEnumMissingInRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum name value of Day: (result = 1))
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: expecting 'in'");
	}

	void TestForEachEnumMissingColonRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum name value in Day (result = 1))
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: expecting ':'");
	}

	void TestForEachEnumTooShortRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum name value in)
				(result = 1)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: expecting (foreach-enum <name> <value> in <enum-type>: <body>)");
	}

	void TestForEachEnumCompoundNameRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum (name) value in Day: (result = 1))
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: expecting the name variable to be a plain identifier");
	}

	void TestForEachEnumCapitalValueRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum name Value in Day: (result = 1))
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: expecting the value variable to be a plain identifier beginning with a lower case letter");
	}

	void TestForEachEnumDottedNameRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum name.x value in Day: (result = 1))
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: expecting the name variable to be a plain identifier of alphanumerics");
	}

	void TestForEachEnumSameNamesRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum n n in Day: (result = 1))
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: the name and value variables must differ, but both are n");
	}

	void TestForEachEnumNameClashRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(Int32 name = 0)
				(foreach-enum name value in Day: (result = 1))
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: the name variable name is already defined in this scope");
	}

	void TestForEachEnumValueClashRejected(IPublicScriptSystem& ss)
	{
		// result is the function's output, which is in scope
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum name result in Day:)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: the value variable result is already defined in this scope");
	}

	void TestForEachEnumNestedClashRejected(IPublicScriptSystem& ss)
	{
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum name value in Day:
					(foreach-enum other value in Day: (result = 1))
				)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "foreach-enum: the value variable value is already defined in this scope");
	}

	void TestForEachEnumVariablesOutOfScopeAfterLoop(IPublicScriptSystem& ss)
	{
		// value is not visible after the loop
		cstr body = R"(
			(function Main -> (Int32 result):
				(foreach-enum name value in Day:)
				(Int64 v = value)
			)
		)";

		RunEnumPreambleFailTest(ss, body, __FUNCTION__, "The RHS of the assignment was unrecognized");
	}

	// ---- FastStringBuilder / string factory audit (2026-11-24). Each of these fails on 398cc72d1 and demonstrates one finding in faststringbuilder-audit.md ----

	namespace FsbAudit
	{
		// FsbAudit.Probe (IStringBuilder sb) -> (Int32 flags)(Int32 capacity)
		void ProbeBuilder(NativeCallEnvironment& e)
		{
			InterfacePointer ip;
			ReadInput(0, ip, e);
			auto* sb = (FastStringBuilder*)InterfaceToInstance(ip);
			WriteOutput(0, sb->flags, e);
			WriteOutput(1, sb->capacity, e);
		}

		void AddProbe(IPublicScriptSystem& ss)
		{
			const INamespace& ns = ss.AddNativeNamespace("FsbAudit");
			ss.AddNativeCall(ns, ProbeBuilder, nullptr, "Probe (Sys.Type.IStringBuilder sb) -> (Int32 flags)(Int32 capacity)", __FILE__, __LINE__, false, 0);
		}

		// Runs (function Main -> (Int32 result): ...) and returns the result. Writes the VM result to [execResult]
		int32 Run(IPublicScriptSystem& ss, cstr srcCode, cstr name, EXECUTERESULT& execResult)
		{
			Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, name);
			Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
			AddTestSecurity(ss);

			vm.Push(-1000); // Allocate stack space for the int32 result
			execResult = vm.Execute(VM::ExecutionFlags(false, true));
			return vm.PopInt32();
		}

		int32 RunAndValidate(IPublicScriptSystem& ss, cstr srcCode, cstr name)
		{
			EXECUTERESULT execResult;
			int32 result = Run(ss, srcCode, name, execResult);
			ValidateExecution(execResult);
			return result;
		}
	}

	// Finding 1: an expandable builder (NewStringBuilder 32) grows to capacity 64, so DestructStringBuilder files it in freeList_64 by capacity.
	// The next NewTokenBuilder then hands out a plain FastStringBuilder header whose 'inline' 64 byte buffer lies past the end of its allocation.
	// The probe shows the recycled token builder still carries the Expandable flag. A token builder must never be expandable.
	void TestFsbAuditExpandedBuilderRecycledAsTokenBuilder(IPublicScriptSystem& ss)
	{
		FsbAudit::AddProbe(ss);

		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function WriteFortyChars (IString path) -> :
				(Sys.IO.IFileWriter f = (Sys.IO.WriteToFile path))
				(f "0123456789012345678901234567890123456789")
				(Sys.IO.CloseWriter f)
			)

			(function GrowAndDrop (IString path) -> :
				(IStringBuilder a = (NewStringBuilder 32))
				(Sys.IO.LoadAndAppendToStringBuilder a path 1)
			)

			(function Main -> (Int32 result):
				(IString path = "..\..\gen\temp\fsb.audit.40.txt")
				(WriteFortyChars path)
				(GrowAndDrop path)
				(IStringBuilder t = (NewTokenBuilder))
				(Int32 flags)
				(Int32 capacity)
				(FsbAudit.Probe t -> flags capacity)
				(result = flags)
			)
		)";

		int32 flags = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("Recycled token builder flags: %d\n", flags);
		validate((flags & (int32)StringBuilderFlags::Expandable) == 0);
	}

	// Finding 5: flags are not reset when a pooled builder is recycled, so ThrowIfAppendWouldTruncate leaks into the next NewTokenBuilder
	void TestFsbAuditRecycledBuilderKeepsThrowFlag(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Poison -> :
				(IStringBuilder sb = (NewTokenBuilder))
				(sb.ThrowIfAppendWouldTruncate)
			)

			(function Main -> (Int32 result):
				(Poison)
				(IStringBuilder t = (NewTokenBuilder))
				(result = 0)
				(try
					(
						(for (Int32 i = 0)(i < 70)(i += 1)
							(t "x")
						)
					)
				catch e
					(
						(result = 1)
					)
				)
			)
		)";

		int32 threw = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("Fresh token builder threw on truncation: %d\n", threw);
		validate(threw == 0); // A fresh token builder truncates by default
	}

	// Finding 3: Loader::LockWriter returns sb.buffer rather than sb.buffer + sb.length, so the file overwrites the start of the builder
	// while Unlock sets length = old length + file length. The tail is stale memory
	void TestFsbAuditLoadAndAppendAppends(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IString path = "..\..\gen\temp\fsb.audit.xyz.txt")
				(Sys.IO.IFileWriter f = (Sys.IO.WriteToFile path))
				(f "xyz")
				(Sys.IO.CloseWriter f)

				(IStringBuilder sb = (NewStringBuilder 100))
				(sb "ABC")
				(Sys.IO.LoadAndAppendToStringBuilder sb path 1)
				(Int32 diff = (Sys.Type.Strings.Compare sb "ABCxyz"))
				(result = (sb.Length * 10))
				(if (diff == 0) (result += 1))
			)
		)";

		int32 result = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("Length*10 + matched: %d\n", result);
		validate(result == 61); // Length 6, content "ABCxyz"
	}

	// Finding 4: StripLeft with leftPos >= length zeroes the length but does not return, then subtracts leftPos again,
	// leaving a negative length and writing a nul at buffer[-leftPos]
	void TestFsbAuditStripLeftEverything(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (NewTokenBuilder))
				(sb "abc")
				(sb.StripLeft 3)
				(result = sb.Length)
			)
		)";

		int32 length = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("sb.Length: %d\n", length);
		validate(length == 0);
	}

	// Finding 6: NativeAppendCTime adds SafeFormat's -1 (truncation) to the length, so a builder with fewer than 25 free chars gets length - 1.
	// After the fix an expandable builder (NewStringBuilder) grows to take the 24 char timestamp, and a fixed builder truncates
	void TestFsbAuditAppendCTimeSmallBuilder(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (NewStringBuilder 16))
				(Int32 nChars = (Sys.Time.AppendCTime sb))
				(result = sb.Length)
				(if (nChars < 0) (result -= 100)) // -101 means length -1 and nChars -1
			)
		)";

		int32 length = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("sb.Length: %d\n", length);
		validate(length == 24); // -101 on 398cc72d1: length -1 and nChars -1
	}

	// Finding 6, fixed builder: 50 chars in a token builder leave 13 free, so the timestamp is truncated to 13 chars
	void TestFsbAuditAppendCTimeTokenBuilderTruncates(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (NewTokenBuilder))
				(sb "01234567890123456789012345678901234567890123456789")
				(Int32 nChars = (Sys.Time.AppendCTime sb))
				(result = ((sb.Length * 100) + nChars))
			)
		)";

		int32 result = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("sb.Length * 100 + nChars: %d\n", result);
		validate(result == 6313);
	}

	// Finding 2: the append natives trust IString.Length from any implementation. A Length near INT32_MAX overflowed nextLength and defeated the truncation check.
	// Per the policy that a malicious script may cause a heap error, the fix does not scan for the terminator: it keeps only cheap checks, so a negative length
	// appends nothing and a huge length hits the 1GB expansion cap in 64-bit arithmetic rather than overflowing
	namespace FsbAudit
	{
		cstr LiarSource(cstr lie, cstr factory, cstr mainBody, char* buffer, size_t capacity)
		{
			SafeFormat(buffer, capacity, R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(class Liar (implements Sys.Type.IString) (Int32 length) (Pointer buffer))
			(method Liar.Construct -> :
				(IString s = "ab")
				(this.buffer = s.Buffer)
				(this.length = %s)
			)
			(method Liar.Length -> (Int32 len): (len = this.length))
			(method Liar.Buffer -> (Pointer buffer): (buffer = this.buffer))
			(factory EntryPoint.NewLiar Sys.Type.IString : (construct Liar))

			(function Main -> (Int32 result):
				(IStringBuilder sb = (%s))
				(IString liar (EntryPoint.NewLiar))
				%s
			)
			)", lie, factory, mainBody);
			return buffer;
		}
	}

	void TestFsbAuditLyingIStringLength(IPublicScriptSystem& ss)
	{
		char src[2048];
		cstr srcCode = FsbAudit::LiarSource("-40", "NewParagraphBuilder", "(sb.AppendIString liar) (sb.AppendSubstring liar 0 -1) (result = sb.Length)", src, sizeof src);
		int32 length = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("sb.Length: %d\n", length);
		validate(length == 0); // A negative length is treated as empty
	}

	void TestFsbAuditHugeIStringLengthHitsCap(IPublicScriptSystem& ss)
	{
		char src[2048];
		cstr srcCode = FsbAudit::LiarSource("2147483632", "NewStringBuilder 16", R"(
				(sb "x")
				(result = 0)
				(try
					(
						(sb.AppendIString liar)
					)
				catch e
					(
						(result = 1000)
					)
				)
				(result += sb.Length))", src, sizeof src);
		int32 result = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("threw * 1000 + sb.Length: %d\n", result);
		validate(result == 1001); // The expansion is refused, and the builder is unchanged
	}

	// Finding 7: AppendSubstring computes startPos + charsToAppend in int, which overflows, so the overspill clamp is skipped and
	// AppendString reads from s + 2147483640
	void TestFsbAuditAppendSubstringOverflow(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (NewTokenBuilder))
				(sb.AppendSubstring "abc" 2147483640 20)
				(result = sb.Length)
			)
		)";

		EXECUTERESULT execResult;
		int32 length = FsbAudit::Run(ss, srcCode, __ROCOCO_FUNCTION__, execResult);
		printf("Execution result: %d (TERMINATED=%d, SEH=%d), sb.Length: %d\n", (int)execResult, (int)EXECUTERESULT_TERMINATED, (int)EXECUTERESULT_SEH, length);
		validate(execResult == EXECUTERESULT_TERMINATED); // With the bug the read faults and the VM returns EXECUTERESULT_SEH
		validate(length == 0);
	}

	// Finding 8: CClassSysTypeStringBuilder/FastStringBuilder::AppendAndTruncate call strncpy_s without _TRUNCATE, so text that does not fit
	// raises the CRT invalid parameter handler (process termination by default on MSVC) instead of truncating
	namespace FsbAudit
	{
		static bool s_invalidParameterRaised = false;
		void __cdecl OnInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
		{
			s_invalidParameterRaised = true;
		}
	}

	void TestFsbAuditAppendAndTruncateTruncates(IPublicScriptSystem& ss)
	{
		// The string pool is created by compilation
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer("(namespace EntryPoint)", -1, Vec2i{ 0,0 }, __ROCOCO_FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));
		ss.AddTree(tree());
		ss.Compile();

		FastStringBuilder* sb = ss.CreateAndPopulateFastStringBuilder(to_fstring("ab"), 16);

		FsbAudit::s_invalidParameterRaised = false;
#if defined(_MSC_VER) && defined(_DEBUG)
		int oldReportMode = _CrtSetReportMode(_CRT_ASSERT, 0);
#endif
		_invalid_parameter_handler old = _set_invalid_parameter_handler(FsbAudit::OnInvalidParameter);

		sb->AppendAndTruncate(to_fstring("0123456789012345678901234567890123456789"));

		_set_invalid_parameter_handler(old);
#if defined(_MSC_VER) && defined(_DEBUG)
		_CrtSetReportMode(_CRT_ASSERT, oldReportMode);
#endif

		printf("Invalid parameter raised: %s, length: %d, strlen: %d\n", FsbAudit::s_invalidParameterRaised ? "yes" : "no", sb->length, (int)StringLength(sb->buffer));
		validate(!FsbAudit::s_invalidParameterRaised);
		validate(sb->length == 15);
		validate(StringLength(sb->buffer) == 15);
		validate(StartsWith(sb->buffer, "ab0123456789012"));
	}

	// Finding 9: TFastAllocator<TMemo>::alloc never records a fresh item in 'items', but records every reused one, and free() never removes it.
	// So a memo freed, reused and freed again sits in both 'items' and 'freeItems' and clear() frees it twice when the script system is destroyed.
	namespace FsbAudit
	{
		// Wraps the current sexy allocator, quarantines everything it frees, and counts frees of blocks it has already freed
		struct DoubleFreeDetector : IAllocator
		{
			IAllocator& inner;
			std::vector<void*> live;
			std::vector<void*> quarantined;
			int doubleFrees = 0;

			DoubleFreeDetector(IAllocator& _inner) : inner(_inner) {}

			static bool Remove(std::vector<void*>& v, void* p)
			{
				for (size_t i = 0; i < v.size(); ++i)
				{
					if (v[i] == p)
					{
						v[i] = v.back();
						v.pop_back();
						return true;
					}
				}
				return false;
			}

			static bool Contains(const std::vector<void*>& v, void* p)
			{
				for (auto* q : v) if (q == p) return true;
				return false;
			}

			void* Allocate(size_t capacity) override
			{
				void* p = inner.Allocate(capacity);
				live.push_back(p);
				return p;
			}

			void FreeData(void* data) override
			{
				if (data == nullptr) return;
				if (Remove(live, data))
				{
					quarantined.push_back(data); // Do not release it yet, so the address cannot be reused and hide a second free
					return;
				}

				if (Contains(quarantined, data))
				{
					doubleFrees++;
					return;
				}

				inner.FreeData(data); // Allocated before the detector was installed
			}

			void* Reallocate(void* ptr, size_t capacity) override
			{
				void* p = inner.Reallocate(ptr, capacity);
				if (p != ptr)
				{
					Remove(live, ptr);
					live.push_back(p);
				}
				return p;
			}

			void AtRelease(FN_AllocatorReleaseFunction fn) override
			{
				inner.AtRelease(fn);
			}

			size_t EvaluateHeapSize() override
			{
				return inner.EvaluateHeapSize();
			}

			void ReleaseQuarantine()
			{
				for (auto* p : quarantined) inner.FreeData(p);
				quarantined.clear();
			}
		};
	}

	void TestFsbAuditMemoReuseDoubleFree(IPublicScriptSystem& ss)
	{
		UNUSED(ss);

		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function MakeMemo -> (Int32 len):
				(IString s (Sys.Type.Memo "Hello"))
				(len = s.Length)
			)

			(function Main -> (Int32 result):
				(result = 0)
				(result += (MakeMemo))
				(result += (MakeMemo))
				(result += (MakeMemo))
			)
		)";

		IAllocator& original = Rococo::Memory::GetSexyAllocator();
		FsbAudit::DoubleFreeDetector detector(original);
		Rococo::Memory::SetSexyAllocator(&detector);

		int32 result = 0;

		try
		{
			ProgramInitParameters pip;
			pip.MaxProgramBytes = 32768;
#ifdef _DEBUG
			pip.useDebugLibs = true;
#else
			pip.useDebugLibs = false;
#endif
			CScriptSystemProxy ssp(pip, s_logger);
			auto& ss2 = ssp();
			Secure(ss2);
			result = FsbAudit::RunAndValidate(ss2, srcCode, __ROCOCO_FUNCTION__);
			ss2.PublicProgramObject().FreeLeakedObjects();
		} // the script system and its memo allocator are destroyed here
		catch (...)
		{
			Rococo::Memory::SetSexyAllocator(&original);
			detector.ReleaseQuarantine();
			throw;
		}

		Rococo::Memory::SetSexyAllocator(&original);
		int doubleFrees = detector.doubleFrees;
		detector.ReleaseQuarantine();

		printf("Memo lengths: %d, double frees at script system teardown: %d\n", result, doubleFrees);
		validate(result == 15);
		validate(doubleFrees == 0);
	}

	// Finding 10: the numeric appends format into rep[32] / rep[64]. A width over 31 (allowed up to 255) makes SafeFormat return -1 and the append silently does nothing,
	// even when ThrowIfAppendWouldTruncate is set
	void TestFsbAuditWideFormatAppendsNothing(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (NewParagraphBuilder))
				(sb.SetFormat 0 40 false true)
				(sb.AppendInt32 7)
				(result = sb.Length)
			)
		)";

		int32 length = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("sb.Length: %d\n", length);
		validate(length == 40);
	}

	// Finding 11: StringFindRight clamps a negative leftPos to 0 in an if/else-if chain whose final else holds the search, so a negative leftPos never searches
	void TestFsbAuditFindRightNegativeLeft(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(result = (Sys.Type.Strings.FindRightWithCase "1245BILL6789" -1 11 "BILL"))
			)
		)";

		int32 pos = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("FindRight position: %d\n", pos);
		validate(pos == 4);
	}

	// Finding 1, pooled expandable case: NewStringBuilder 64 takes a pooled header, grows onto a heap buffer, and must go back to the 64 list with its inline buffer restored
	void TestFsbAuditPooledExpandableRecycled(IPublicScriptSystem& ss)
	{
		FsbAudit::AddProbe(ss);

		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function GrowAndDrop -> (Int32 capacity):
				(IStringBuilder a = (NewStringBuilder 64))
				(for (Int32 i = 0)(i < 10)(i += 1)
					(a "0123456789")
				)
				(Int32 flags)
				(FsbAudit.Probe a -> flags capacity)
				(if (a.Length != 100) (capacity = -1))
			)

			(function Main -> (Int32 result):
				(Int32 grownCapacity = (GrowAndDrop))
				(IStringBuilder t = (NewTokenBuilder))
				(for (Int32 i = 0)(i < 10)(i += 1)
					(t "0123456789")
				)
				(Int32 flags)
				(Int32 capacity)
				(FsbAudit.Probe t -> flags capacity)
				(result = 0)
				(if (grownCapacity > 100) (result += 1))
				(if (flags == 4) (result += 10)) // Pool64 only: not expandable, no throw flag
				(if (capacity == 64) (result += 100))
				(if (t.Length == 63) (result += 1000))
			)
		)";

		int32 result = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("Checks passed: %d\n", result);
		validate(result == 1111);
	}

	// Finding 12: Replace with the builder itself as the replacement text. The old in-place algorithm read the replacement while overwriting it
	void TestFsbAuditReplaceSelfAliased(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (NewStringBuilder 64))
				(sb "a-a")
				(sb.Replace 0 "a" sb)
				(Int32 diff = (Sys.Type.Strings.Compare sb "a-a-a-a"))
				(result = sb.Length)
				(if (diff != 0) (result = -1))
			)
		)";

		int32 result = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("sb.Length: %d\n", result);
		validate(result == 7); // Each of the two a's in "a-a" becomes "a-a"
	}

	// Finding 12: a Replace that does not fit a fixed builder throws and leaves the builder as it was, rather than half replaced
	void TestFsbAuditReplaceFixedIsAtomic(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (NewTokenBuilder))
				(sb "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa") // 40 chars
				(result = 0)
				(try
					(
						(sb.Replace 0 "a" "bb")
					)
				catch e
					(
						(result = 1000)
					)
				)
				(Int32 diff = (Sys.Type.Strings.Compare sb "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"))
				(result += sb.Length)
				(if (diff != 0) (result = -1))
			)
		)";

		int32 result = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("threw * 1000 + sb.Length: %d\n", result);
		validate(result == 1040);
	}

	// Replace on an expandable builder grows it
	void TestFsbAuditReplaceExpands(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (NewStringBuilder 8))
				(sb "a.b.c")
				(sb.Replace 0 "." "<--->")
				(Int32 diff = (Sys.Type.Strings.Compare sb "a<--->b<--->c"))
				(result = sb.Length)
				(if (diff != 0) (result = -1))
			)
		)";

		int32 result = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("sb.Length: %d\n", result);
		validate(result == 13);
	}

	// Low severity: AppendChar truncated values above 255 to a char. They are now ignored, as 0 is
	void TestFsbAuditAppendCharOutOfRange(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (NewTokenBuilder))
				(sb.AppendChar 256)
				(sb.AppendChar 65)
				(sb.AppendChar 0)
				(sb.AppendChar -1)
				(result = sb.Length)
			)
		)";

		int32 result = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("sb.Length: %d\n", result);
		validate(result == 1);
	}

	// Low severity: SetLength beyond the current length wrote to output 0 of a method that has no outputs. It is now a no-op
	void TestFsbAuditSetLengthLongerIsNoOp(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(Int32 sentinel = 77)
				(IStringBuilder sb = (NewTokenBuilder))
				(sb "abc")
				(sb.SetLength 10)
				(result = ((sb.Length * 100) + sentinel))
			)
		)";

		int32 result = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("sb.Length * 100 + sentinel: %d\n", result);
		validate(result == 377);
	}

	// Low severity: MakeSysSlashes called ToSysPath on the whole buffer once per char. One pass gives the same result
	void TestFsbAuditMakeSysSlashes(IPublicScriptSystem& ss)
	{
		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (NewTokenBuilder))
				(sb "a/b/c")
				(Sys.Type.Strings.MakeSysSlashes sb)
				(result = (Sys.Type.Strings.FindLeftWithCase sb 0 "/"))
				(if (result == -1) (result = sb.Length))
			)
		)";

		int32 result = FsbAudit::RunAndValidate(ss, srcCode, __ROCOCO_FUNCTION__);
		printf("Position of / or length: %d\n", result);
		validate(result == 5); // No forward slashes remain
	}

	// Finding 9: ExpandStringBuilder set the new capacity before allocating, so a failed allocation left a builder claiming capacity it did not have.
	// The allocator below fails one request size, which the doubling of a 17 char builder reaches at 17 * 512 bytes
	namespace FsbAudit
	{
		struct FailingAllocator : IAllocator
		{
			IAllocator& inner;
			size_t failSize;
			int failures = 0;

			FailingAllocator(IAllocator& _inner, size_t _failSize) : inner(_inner), failSize(_failSize) {}

			void* Allocate(size_t capacity) override
			{
				if (capacity == failSize)
				{
					failures++;
					return nullptr; // As the default sexy allocator does when malloc fails
				}
				return inner.Allocate(capacity);
			}

			void FreeData(void* data) override { inner.FreeData(data); }
			void* Reallocate(void* ptr, size_t capacity) override { return inner.Reallocate(ptr, capacity); }
			void AtRelease(FN_AllocatorReleaseFunction fn) override { inner.AtRelease(fn); }
			size_t EvaluateHeapSize() override { return inner.EvaluateHeapSize(); }
		};
	}

	void TestFsbAuditExpansionFailureLeavesBuilderIntact(IPublicScriptSystem& ss)
	{
		UNUSED(ss);

		cstr srcCode = R"(
			(namespace EntryPoint)
			(alias Main EntryPoint.Main)
			(using Sys.Type)

			(function Main -> (Int32 result):
				(IStringBuilder sb = (NewStringBuilder 17))
				(sb "x")
				(result = 0)
				(try
					(
						(for (Int32 i = 0)(i < 20)(i += 1)
							(sb.AppendIString sb) // Doubles the length each time
						)
					)
				catch e
					(
						(result = 1000000)
					)
				)
				(Int32 flags)
				(Int32 capacity)
				(FsbAudit.Probe sb -> flags capacity)
				(result += (capacity * 100))
				(Int32 pos = (Sys.Type.Strings.FindLeftWithCase sb 0 "y"))
				(if (pos == -1) (result += 1))
				(result += (sb.Length - 4096))
			)
		)";

		IAllocator& original = Rococo::Memory::GetSexyAllocator();
		FsbAudit::FailingAllocator failing(original, 17 * 512);
		Rococo::Memory::SetSexyAllocator(&failing);

		int32 result = 0;

		try
		{
			ProgramInitParameters pip;
			pip.MaxProgramBytes = 32768;
#ifdef _DEBUG
			pip.useDebugLibs = true;
#else
			pip.useDebugLibs = false;
#endif
			CScriptSystemProxy ssp(pip, s_logger);
			auto& ss2 = ssp();
			Secure(ss2);
			FsbAudit::AddProbe(ss2);
			result = FsbAudit::RunAndValidate(ss2, srcCode, __ROCOCO_FUNCTION__);
		}
		catch (...)
		{
			Rococo::Memory::SetSexyAllocator(&original);
			throw;
		}

		Rococo::Memory::SetSexyAllocator(&original);

		printf("Result: %d, failed allocations: %d\n", result, failing.failures);
		validate(failing.failures >= 1);
		validate(result == 1000000 + 4352 * 100 + 1); // Threw; capacity still 17 * 256 = 4352; length 4096 of x's, unchanged
	}

	void RunFsbAuditTests()
	{
		TEST3(TestFsbAuditExpandedBuilderRecycledAsTokenBuilder);
		TEST(TestFsbAuditRecycledBuilderKeepsThrowFlag);
		TEST3(TestFsbAuditLoadAndAppendAppends);
		TEST(TestFsbAuditStripLeftEverything);
		TEST(TestFsbAuditAppendCTimeSmallBuilder);
		TEST(TestFsbAuditAppendCTimeTokenBuilderTruncates);
		TEST(TestFsbAuditLyingIStringLength);
		TEST(TestFsbAuditHugeIStringLengthHitsCap);
		TEST(TestFsbAuditAppendSubstringOverflow);
		TEST(TestFsbAuditAppendAndTruncateTruncates);
		TEST(TestFsbAuditMemoReuseDoubleFree);
		TEST(TestFsbAuditWideFormatAppendsNothing);
		TEST(TestFsbAuditFindRightNegativeLeft);
		TEST(TestFsbAuditPooledExpandableRecycled);
		TEST(TestFsbAuditReplaceSelfAliased);
		TEST(TestFsbAuditReplaceFixedIsAtomic);
		TEST(TestFsbAuditReplaceExpands);
		TEST(TestFsbAuditAppendCharOutOfRange);
		TEST(TestFsbAuditSetLengthLongerIsNoOp);
		TEST(TestFsbAuditMakeSysSlashes);
		TEST(TestFsbAuditExpansionFailureLeavesBuilderIntact);
	}

	void RunPositiveSuccesses()
	{
		validate(true);

		TEST(TestNativeHandle);

		TEST(TestTypenames);

		TEST(TestDefaultParameter);
		TEST(TestStrongNumber);
		TEST(TestStrongNumber2);
		TEST(TestStrongNumber3);
		TEST(TestStrongNumber4);
		TEST(TestStrongNumber5);
		TEST(TestStrongNumber6);
		TEST(TestStrongNumber7);

		TEST(TestNullOutInterface);
		TEST(TestMutableArgFunction);
		TEST(TestConstArgFunction);
		TEST3(Test1FieldInit);
		TEST(TestRightSearchSubstring);
		TEST(TestStringConstant);

		TEST(TestMemberwiseInit);
		TEST(TestInternalDestructorsCalled);
		TEST(TestTryFinallyWithoutThrow);
		TEST(TestDynamicCast);

		TEST(TestDerivedInterfaces2);
		TEST(TestCatch);
		TEST(TestCatchArg);

		TEST(TestStructWithInterface);

		TEST(TestCatchInstanceArg);
		TEST(TestTryWithoutThrow);

		TEST(TestSearchSubstring);
		
		TEST(TestAppendSubstring);
		TEST(TestStringbuilderTruncate);

		TEST(TestOperatorOverload3);
		TEST(TestStaticCast1);
		TEST(TestCreateNamespace);
		TEST(TestAssignInt32Literal);
		TEST(TestAssignFloat32Literal);
		TEST(TestAssignFloat64Literal);
		TEST(TestAssignInt64Literal);
		TEST(TestAssignInt32Variable);
		TEST(TestAssignInt64Variable);
		TEST(TestAssignFloat32Variable);
		TEST(TestAssignFloat64Variable);
		TEST(TestAssignMatrixVariable);
		TEST(TestAssignVectorVariableByRef);

		TEST(TestBuilderWithGetAccessor);
		TEST(TestBuilderWithGetAccessor2);

		TEST(TestCompareGetAccessorVsCompound);

		TEST(TestPublishAPI);

		TEST(TestClassAttribute);

		TEST(TestNullIString);

	//	TEST(TestPackage);

		TEST(TestMacroExpandCatch);

		TEST(TestUseDefaultNamespace);
		TEST(TestUseDefaultNamespace2);

		TEST(TestClamp1);
		TEST(TestClamp2);
		TEST(TestClamp3);

		TEST(TestSearchSubstring);
		TEST(TestSetCase);

		TEST(TestStringToChar);
		TEST(TestStringToCharFail);
		TEST(TestCallPrivateMethodviaDispatch);
		TEST(TestCallPrivateMethod);
		TEST(TestCallPrivateMethod2);

		TEST(TestDynamicCast2);

		TEST(TestDynamicDispatch);
		TEST(TestPushSecondInterface);
		TEST(TestStructArgFromStructArg);
		TEST(TestCompareInterfaces);
		TEST(TestZeroDefaults);
		TEST(TestMemberwiseConstructWithInterface);
		TEST(TestStructInClassSetFromMethod);
		TEST(TestStaticCast);
		TEST(TestCompareStruct);
		TEST(TestInterfaceForNull);
		TEST(TestFactoryReturnsBaseInterface);
		TEST(TestLoopBreak);
		TEST3(TestLoopFinally);
		TEST(TestBadClosureArg);
		TEST(TestNullArchetypeArg);
		TEST(TestGetArchetypeName);

		TEST(TestBadClosureArg7);
		TEST(TestBadClosureArg6);
		TEST(TestBadClosureArg5);
		TEST(TestBadClosureArg4);
		TEST(TestBadClosureArg3);
		TEST(TestBadClosureArg2);

		TEST(TestIndexOf);
		TEST(TestTransformParent);

		TEST(TestMacroSiblings2);

		TEST(TestStartsWith);

		TEST(TestMethodFromClosure);

		TEST(TestEmptyMap);

		TEST(TestDeltaOperators);
		TEST(TestDeltaOperators2);
		TEST(TestDeltaOperators3);
		TEST(TestDeltaOperators4);
		TEST(TestDeltaOperators5);

		TEST(TestAssignStringToStruct);
		TEST(TestCaptureStruct);
		TEST(TestAddRefWithLocalVariable);
		TEST(TestGetSysMessage);
		TEST(TestNullMemberInit);
		TEST(TestConstructor);
		TEST(TestAssignDerivatives);
		TEST(TestBitwiseOr);
		TEST(TestBitwiseOrInt64VarLiteral);
		TEST(TestBitwiseOrLiteralFold);
		TEST(TestBitwiseOrVariables);
		TEST(TestBitwiseOrNegative);
		TEST(TestBitwiseOrNested);
		TEST(TestBitwiseOrFloat32LiteralRejected);
		TEST(TestBitwiseOrFloat32VarRejected);
		TEST(TestBitwiseOrFloat64LiteralRejected);
		TEST(TestBitwiseOrFloat64VarRejected);
		TEST(TestBitwiseOrAssignAndArg);
		TEST(TestOperatorOverload2);
		TEST(TestStructWithVec4f);
		TEST(TestBooleanVarCompare);
		TEST(TestBooleanCompareVarToCompound);
		TEST(TestAssignDerivativeFromRef);
		TEST(TestMemberwise2);
		TEST(TestNullObject);
		TEST(TestNullArchetype);
		TEST(TestOperatorOverload);

		//	TEST(TestArrayOfArchetypes); // -> currently disabled, since arrays are 32-bit and 64-bits only, and closures are 128 bits.
		TEST(TestClosureCapture);
		TEST(TestClosureCapture2);
		TEST(TestClosureAndThisMember);
		TEST(TestClosureAndThis);
		TEST(TestArchetypePropagation);
		TEST(TestArchetypeReturn);
		TEST(TestArchetypeReturnFromMultipleOutput);
		TEST(TestEmbeddedArchetype);
		TEST(TestClosure);
		TEST(TestClosureWithVariable);
		TEST(TestReturnClosureWithVariableSucceed);
		TEST(TestArchetypeCall);

		TEST(TestClosureArg);

		TEST(TestDefaultNullMethod);

		TEST(TestGlobalInt32);
		TEST(TestGlobalInt32_2);
		TEST(TestGlobalInt32_3);

		TEST(TestStructWithInterface);
		TEST(TestMinimumConstruct);

		TEST(TestLocalVariable);
		TEST(TestBooleanLiteralVsLiteralMatches);
		TEST(TestBooleanMismatch);
		TEST(TestBooleanVariableVsLiteralMatches);
		TEST(TestBooleanVariableVsVariable);
		TEST(TestBooleanAssignByTwoVariables);
		TEST(TestBooleanExpressions);
		TEST(TestBooleanCompoundExpressions1);
		TEST(TestBooleanCompoundExpressions2);
		TEST(TestBooleanCompoundExpressions3);
		TEST(TestBooleanCompoundExpressions4);
		TEST(TestBooleanCompoundExpressions5);
		TEST(TestBooleanCompoundExpressions6);
		TEST(TestBooleanCompoundExpressions7);
		TEST(TestBooleanCompoundExpressions8);
		TEST(TestBooleanCompoundExpressions9);
		TEST(TestBooleanCompoundExpressions10);
		TEST(TestBooleanCompoundExpressions11);
		TEST(TestBooleanCompoundExpressions12);
		TEST(TestIfThen1);
		TEST(TestIfThen2);
		TEST(TestFloatArithmetic);
		TEST(TestDoubleArithmetic0);
		TEST(TestDoubleArithmetic1);
		TEST(TestDoubleArithmetic2);
		TEST(TestDoubleArithmetic3);
		TEST(TestDoubleArithmetic4);
		TEST(TestInt32Arithmetic);
		TEST(TestInt64Arithmetic);
		TEST(TestInt32CompoundArithmetic);
		TEST(TestFloatCompoundArithmetic);
		TEST(TestFloatLiteralArithmetic);
		TEST(TestFloatArithmeticByMember);
		TEST(TestIfThen3);
		TEST(TestFunctionCall1);
		TEST(TestFunctionCall2);
		TEST(TestFunctionCall3);
		TEST(TestFunctionCall4);
		TEST(TestFunctionCallWithOutParameter);

		TEST(TestFunctionCallRecursion1);
		TEST(TestFunctionCallMultiOutput1);
		TEST(TestStructure);
		TEST(TestStructure2);
		TEST(TestStructure3);
		TEST(TestStructure4);

		TEST(TestSerialize);

		TEST3(TestForStatement);
		TEST(TestWhileLoop1);
		TEST(TestNestedWhileLoops);
		TEST(TestWhileLoopBreak);
		TEST(TestWhileLoopContinue);
		TEST(TestWhileLoopContinueWithStruct);
		TEST(TestDoWhile);
		TEST(TestDoWhileBreak);
		TEST(TestDoWhileContinue);
		TEST(TestArchetypeDeclaration);

		TEST(TestNativeCall);
		TEST(TestNativeCall2);
		TEST(TestNativeCall3);
		TEST(TestInterfaceDefinition);

		TEST(TestConstructor);
		TEST(TestClassDefinition);

		TEST(TestNullMember);
		TEST(TestNullRefInit);

		TEST(TestRecti1);
		TEST(TestRecti2);
		TEST(TestRecti3);
		TEST(TestRecti4);

		TEST(TestTypedef);

		TEST(TestStructStringPassByRef);

		TEST(TestEmptyString);

		TEST(TestYield);
		TEST(TestStructByRefAssign);
		TEST(TestAssignVectorComponentsFromParameters);

		TEST(TestAssignPointerFromFunction);

		TEST(TestClassExtendsInterface);

		TEST(TestStringAndBuild);

		TEST(TestMultipleDerivation2);
		TEST(TestMultipleDerivation);

		TEST(TestCompareGetAccessorWithOne);
		TEST(TestCompareGetAccessorWithOne2);

		TEST(TestCaptureInFunctionAllInClosure);

		TEST(TestStringMember);
		TEST(TestStringMember2);

		TEST(TestStructCopyRef);

		TEST(TestClassDefinesInterface);

		TEST(TestInterfacePropagation);
		TEST(TestInstancePropagation);
		TEST(TestInstanceMemberPropagation);
		TEST(TestMultipleInterfaces);

		TEST(TestMemoString);
		TEST(TestMemoString2);

		TEST(TestPrint);
		TEST(TestPrintViaInstance);
		TEST(TestNullString);

		TEST(TestAssignDerivatives);
		TEST(TestStringMember3);
		TEST(TestClassInstance);
		TEST(TestDestructor);
		TEST(TestStringBuilder);
		TEST(TestInlinedFactory);
		TEST(TestDerivedInterfaces);
		TEST(TestVirtualFromVirtual);
		TEST(TestSizeOf);

		// TEST(TestInstancing); // Disabled until we have total compilation. JIT requires a PC change

		TEST(TestLambda);
		TEST(TestMemberwiseInit);
		TEST(TestInternalDestructorsCalled);
		TEST(TestTryFinallyWithoutThrow);

		TEST(TestDynamicCast);
		TEST(TestExceptionDestruct);
		TEST(TestDerivedInterfaces2);
		TEST(TestCatch);
		TEST(TestCatchArg);

		TEST(TestStructWithInterface);

		TEST(TestCatchInstanceArg);
		TEST(TestTryWithoutThrow);


		TEST(TestSearchSubstring);
		TEST(TestAppendSubstring);
		TEST(TestStringbuilderTruncate);

		TEST(TestEssentialInterface);

		TestMaths();

		TEST(TestStringBuilderBig);

		TEST(TestPushStringLen);

		TEST(TestRefTypesInsideClosure);

		TEST(TestMeshStruct3);
		TEST(TestMeshStruct2);
		TEST(TestMeshStruct);

		TEST(TestModuleCount);

		TEST(TestMacro);

		TEST(TestExpressionArg);
		TEST(TestReflectionGetParent);
		TEST(TestReflectionGetChild);
		TEST(TestReflectionGetAtomic);

		TEST(TestMacroAsArgument1);
		TEST(TestSubstitution);
		TEST(TestReflectionGetCurrentExpression);

		TEST(TestClosureInputToStruct);

		TEST(TestSysThrow);
		TEST(TestSysThrow2);
		TEST(TestConstructFromInterface);
		TEST(TestInternalDestructorsCalled2);
		TEST(TestReflectionGetChild_BadIndex);
		TEST(TestStringBuilderLength1);
		TEST(TestAddNativeReflectionCall);

		TEST(TestRaw);
		TEST(TestPartialCompiles);
		TEST3(TestConsoleOutput2);

		TEST(TestMacroSiblings);

		TEST3(TestCreateDeclarations);

		TEST(TestAssignNotItself);
		TEST(TestTypeOf);
		TEST3(TestTypeOfClass);
		TEST(TestEnumerateInterfaces);
		TEST(TestGetInterfaceName);
		TEST(TestGetMethodCount);
		TEST(TestGetMethodArgCounts);
		TEST(TestAppendMethodName);
		TEST(TestGetMethodGetInputTypeAndName);
		TEST(TestGetMethodGetOutputTypeAndName);
		TEST(TestGetMethodGetInputType);
		TEST(TestGetMethodGetOutputType);
		TEST(TestIsMethodInputOfType);
		TEST(TestIsMethodOutputOfType);
		TEST(TestInvokeMethodViaReflection);

		TEST(TestDeepCatch);
		TEST(TestReturnInterfaceEx);
		
		TEST(TestThrowFromCatch);
		TEST(TestStringArray);
		TEST(TestGetAccessorSemantics);
		TEST(TestGetAccessorSemantics2);
		TEST(TestGetAccessorSemantics3);
		TEST(TestTernary);
		TEST(TestExpressionProxies);
		TEST(TestExpressionProxies2);
		TEST3(TestLoopFinally2);
		TEST(TestFunctionCallRecursion2);
		TEST(TestStringSplit);
		TEST(TestMeshStruct4);
		TEST(TestPrintModules);
		TEST(TestPrintStructs);
		TEST3(TestConsoleOutput3);
		TEST3(TestConsoleOutput);
		TEST(TestMacroSiblings3Throws);
		TEST3(TestConsoleOutput4);
		TEST3(TestStringReplace);
		TEST(TestExpressionAppendTo);

		TEST(TestEssentialInterface);
		TEST2(TestCoroutine1);
		TEST2(TestCoroutineCompleteOne);
		TEST2(TestCoroutineCompleteLastWhileDormant);
		TEST2(TestCoroutineReleaseAllThenContinue);
		TEST2(TestCoroutineThrowInRun);
		TEST2(TestCoroutineReleaseDormantNoLeak);
		TEST2(TestCoroutineAddDuringRun);
		TEST2(TestCoroutineReleaseSelfDuringRun);
		TEST2(TestCoroutineReleaseAllDuringRun);
		TEST2(TestCoroutineNestedContinue);
		TEST2(TestCoroutineYieldZero);
		TEST2(TestCoroutineStackOverflow);
		TEST(TestVMStackOverflow);
		TEST3(TestTopLevelMacro2);
		TEST(TestCPPCallback);
		TEST(TestRock);

		TEST(TestConstants);
		TEST(TestConstants2);
		TEST(TestConstants3);
		TEST(TestConstants4);
		TEST(TestConstants5);
		TEST(TestConstants6);
		TEST(TestConstants7);
		TEST(TestConstants8);
		TEST(TestConstants9);
		TEST(TestConstantsCompoundEnum);
		TEST(TestConstantsCompoundEnumMember);
		TEST(TestConstantsCompoundEnumArg);
		TEST(TestEnumQualified);
		TEST(TestEnumQualifiedNamespace);
		TEST(TestEnumQualifiedCompound);
		TEST(TestEnumQualifiedMember);
		TEST(TestEnumQualifiedArg);
		TEST(TestEnumQualifiedCompare);
		TEST(TestEnumQualifiedArray);
		TEST(TestEnumQualifiedList);
		TEST(TestEnumQualifiedMap);
		TEST(TestEnumQualifiedUnknownValue);
		TEST(TestEnumQualifiedWrongType);
		TEST(TestEnumQualifiedToInt32);
		TEST(TestEnumQualifiedUnknownType);
		TEST(TestEnumQualifiedBadSyntax);
		TEST(TestEnumQualifiedArithmetic);
		TEST(TestEnumQualifiedAsFunction);
		TEST(TestEnumOrInit);
		TEST(TestEnumOrAssign);
		TEST(TestEnumOrMember);
		TEST(TestEnumOrArg);
		TEST(TestEnumOrReturn);
		TEST(TestEnumOrCompare);
		TEST(TestEnumOrInt64);
		TEST(TestEnumOrMixedConstants);
		TEST(TestEnumOrMixedVariables);
		TEST(TestEnumOrIntLiteral);
		TEST(TestEnumOrIntVariable);
		TEST(TestEnumOrIntoInt32);
		TEST(TestEnumOrIntoInt64);
		TEST(TestEnumOrInt64IntoInt64);
		TEST(TestEnumOrInt64IntoInt32);
		TEST(TestEnumOrUnqualifiedIntoInt32);
		TEST(TestEnumOrFloat);
		TEST(TestEnumOrFloatEnum);
		TEST(TestEnumOrOtherOperators);
		TEST(TestEnumOrOtherOperatorsVariable);
		TEST(TestDeltaOrInt32);
		TEST(TestDeltaClearInt32);
		TEST(TestDeltaInt32SignBit);
		TEST(TestDeltaInt64);
		TEST(TestDeltaOrEnum);
		TEST(TestDeltaClearEnum);
		TEST(TestDeltaEnumRoundTrip);
		TEST(TestEnumConstantExponentLikeName);
		TEST(TestDeltaOrFloatRejected);
		TEST(TestDeltaClearFloatRejected);
		TEST(TestDeltaOrEnumIntRejected);
		TEST(TestDeltaOrEnumOtherTypeRejected);
		TEST(TestDeltaClearEnumOtherTypeRejected);
		TEST(TestDeltaAddEnumRejected);
		TEST(TestStrongMathFloat);
		TEST(TestStrongMathInt);
		TEST(TestStrongMathExpressions);
		TEST(TestStrongFlagsInt);
		TEST(TestStrongFlagsFloatRejected);
		TEST(TestEnumFlagTestDot);
		TEST(TestEnumFlagTestQualified);
		TEST(TestEnumFlagTestInt32);
		TEST(TestEnumFlagTestInt64);
		TEST(TestEnumFlagTestMember);
		TEST(TestEnumFlagTestLogic);
		TEST(TestEnumFlagTestMultiBit);
		TEST(TestEnumFlagTestDelta);
		TEST(TestEnumFlagTestWhile);
		TEST(TestEnumFlagTestUnknownKey);
		TEST(TestEnumFlagTestOtherEnum);
		TEST(TestEnumFlagTestInt64VarInt32Enum);
		TEST(TestEnumFlagTestInt32VarInt64Enum);
		TEST(TestEnumFlagTestFloatVar);
		TEST(TestEnumFlagTestNonEnumStrong);
		TEST(TestEnumFlagTestIntDot);
		TEST(TestEnumFlagTestLiteral);
		TEST(TestEnumToIntInit);
		TEST(TestEnumToIntAssign);
		TEST(TestEnumToIntMember);
		TEST(TestEnumToIntArg);
		TEST(TestEnumToIntWiden);
		TEST(TestEnumToInt64);
		TEST(TestEnumToIntNarrow);
		TEST(TestEnumToIntFloat);
		TEST(TestEnumToIntUnqualified);
		TEST(TestEnumToIntArgUnqualified);
		TEST(TestEnumToOtherEnum);
		TEST(TestEnumTernaryDot);
		TEST(TestEnumTernaryQualified);
		TEST(TestEnumTernaryVariables);
		TEST(TestEnumTernaryContexts);
		TEST(TestEnumTernaryOr);
		TEST(TestEnumTernaryToInt);
		TEST(TestEnumTernaryOtherEnum);
		TEST(TestEnumTernaryLiteral);
		TEST(TestEnumTernaryIntVariable);
		TEST(TestEnumTernaryUnqualifiedToInt);
		TEST(TestEnumArrayShorthandDot);
		TEST(TestEnumArrayShorthandQualified);
		TEST(TestEnumArrayShorthandWrongEnum);
		TEST(TestArrayShorthandSetInt32);
		TEST(TestEnumAppendNamed);
		TEST(TestEnumAppendAllValues);
		TEST(TestEnumAppendNoName);
		TEST(TestEnumAppendSignBit);
		TEST(TestEnumAppendInt64);
		TEST(TestEnumAppendField);
		TEST(TestEnumAppendToExisting);
		TEST(TestEnumAppendBuilderParameter);
		TEST(TestEnumAppendNullBuilderThrows);
		TEST(TestEnumAppendPlainStrongRejected);
		TEST(TestEnumAppendJoulesRejected);
		TEST(TestEnumAppendIntRejected);
		TEST(TestEnumAppendFloatRejected);
		TEST(TestEnumAppendUnknownRejected);
		TEST(TestEnumAppendConstantRejected);
		TEST(TestEnumAppendIStringTargetRejected);
		TEST(TestEnumAppendIntTargetRejected);
		TEST(TestEnumAppendUnknownTargetRejected);
		TEST(TestEnumAppendMissingOperandRejected);
		TEST(TestEnumAppendTooManyOperandsRejected);
		TEST(TestEnumAppendCompoundOperandRejected);
		TEST(TestEnumToStringRemoved);
		TEST(TestEnumFromStringRoundTrip);
		TEST(TestEnumFromStringLiteral);
		TEST(TestEnumFromStringVariable);
		TEST(TestEnumFromStringBuilder);
		TEST(TestEnumFromStringField);
		TEST(TestEnumFromStringNoMatchUnchanged);
		TEST(TestEnumFromStringDeclarationNoMatch);
		TEST(TestEnumFromStringCaseSensitive);
		TEST(TestEnumFromStringEmptyAndNull);
		TEST(TestEnumFromStringInt64);
		TEST(TestEnumFromStringIntTargetRejected);
		TEST(TestEnumFromStringIntDeclarationRejected);
		TEST(TestEnumFromStringJoulesRejected);
		TEST(TestEnumFromStringNonBoolRejected);
		TEST(TestEnumFromStringUnknownBoolRejected);
		TEST(TestEnumFromStringNonStringRejected);
		TEST(TestEnumFromStringUnknownStringRejected);
		TEST(TestEnumFromStringSubexpressionRejected);
		TEST(TestEnumFromStringArgumentRejected);
		TEST(TestEnumFromStringStandaloneRejected);
		TEST(TestEnumFromStringMissingArrowRejected);
		TEST(TestEnumFromStringMissingBoolRejected);
		TEST(TestEnumFromStringNoArrowNoBoolRejected);
		TEST(TestEnumFromStringDeclarationNoBoolRejected);
		TEST(TestEnumFlagsDeclaration);
		TEST(TestEnumFlagsSequential);
		TEST(TestEnumFlagsMultiBit);
		TEST(TestEnumFlagsPlainEnumDuplicatesStillAllowed);
		TEST(TestEnumIsFlaggedFlags);
		TEST(TestEnumIsFlaggedEnum);
		TEST(TestEnumIsFlaggedField);
		TEST(TestEnumFlagsAppendList);
		TEST(TestEnumFlagsAppendToExisting);
		TEST(TestEnumFlagsAppendZeroNoName);
		TEST(TestEnumFlagsAppendUncoveredBits);
		TEST(TestEnumFlagsAppendMultiBitPartial);
		TEST(TestEnumFlagsAppendInt64);
		TEST(TestEnumFlaggerEnumAppend);
		TEST(TestEnumFlagsAppendTruncates);
		TEST(TestEnumFlagsAppendExpands);
		TEST(TestEnumFlagsFromStringLiteral);
		TEST(TestEnumFlagsFromStringDelimiters);
		TEST(TestEnumFlagsFromStringVariable);
		TEST(TestEnumFlagsFromStringBadTokenUnchanged);
		TEST(TestEnumFlagsFromStringEmpty);
		TEST(TestEnumFlagsFromStringRepeated);
		TEST(TestEnumFlagsFromStringZeroName);
		TEST(TestEnumFlagsFromStringInt64);
		TEST(TestEnumFlagsFromStringLongList);
		TEST(TestEnumFlagsFromStringLongToken);
		TEST(TestEnumFlagsFromStringRoundTrip);
		TEST(TestEnumFromStringNonFlaggerListRejected);
		TEST(TestEnumCaseCollisionEnumRejected);
		TEST(TestEnumCaseCollisionFlagsRejected);
		TEST(TestEnumCaseCollisionAcrossDeclarationsRejected);
		TEST(TestEnumCaseDistinctNamesAllowed);
		TEST(TestEnumFlagsCombinationRejected);
		TEST(TestEnumFlagsDuplicateRejected);
		TEST(TestEnumFlagsDuplicateZeroRejected);
		TEST(TestEnumFlagsSequentialOverlapRejected);
		TEST(TestEnumFlagsNegativeRejected);
		TEST(TestEnumFlagsAfterEnumRejected);
		TEST(TestEnumFlagsTooFewElementsRejected);
		TEST(TestEnumFlagsUnknownTypeRejected);
		TEST(TestEnumIsFlaggedPlainStrongRejected);
		TEST(TestEnumIsFlaggedIntRejected);
		TEST(TestEnumIsFlaggedUnknownTypeRejected);
		TEST(TestEnumIsFlaggedVariableOperandRejected);
		TEST(TestEnumIsFlaggedTargetRejected);
		TEST(TestEnumIsFlaggedAssignTargetRejected);
		TEST(TestEnumIsFlaggedMissingOperandRejected);
		TEST(TestEnumIsFlaggedStandaloneRejected);
		TEST(TestForEachEnumNamesInOrder);
		TEST(TestForEachEnumValuesInOrder);
		TEST(TestForEachEnumNameBuilderRoundTrip);
		TEST(TestForEachEnumInt64Values);
		TEST(TestForEachEnumInt32Widened);
		TEST(TestForEachEnumFlags);
		TEST(TestForEachEnumNested);
		TEST(TestForEachEnumNameComparedWithLiteral);
		TEST(TestForEachEnumBreak);
		TEST(TestForEachEnumContinue);
		TEST(TestForEachEnumBreakInnerOnly);
		TEST(TestForEachEnumBreakInsideWhile);
		TEST(TestForEachEnumInsideWhileContinue);
		TEST(TestForEachEnumEmptyBody);
		TEST(TestForEachEnumSeparateColon);
		TEST(TestForEachEnumQualifiedType);
		TEST(TestForEachEnumNamesReusedAfterLoop);
		TEST(TestForEachEnumReturnFromBody);
		TEST(TestForEachEnumNoValuesRejected);
		TEST(TestForEachEnumNotStrongRejected);
		TEST(TestForEachEnumUnknownTypeRejected);
		TEST(TestForEachEnumLowerCaseTypeRejected);
		TEST(TestForEachEnumMissingInRejected);
		TEST(TestForEachEnumMissingColonRejected);
		TEST(TestForEachEnumTooShortRejected);
		TEST(TestForEachEnumCompoundNameRejected);
		TEST(TestForEachEnumCapitalValueRejected);
		TEST(TestForEachEnumDottedNameRejected);
		TEST(TestForEachEnumSameNamesRejected);
		TEST(TestForEachEnumNameClashRejected);
		TEST(TestForEachEnumValueClashRejected);
		TEST(TestForEachEnumNestedClashRejected);
		TEST(TestForEachEnumVariablesOutOfScopeAfterLoop);

		TEST(TestEnumByCPP);

		TEST(TestFactoryReturnEmpty);

		TEST(TestStructureAssignToAtomicCallReturn);
		TEST(TestStructureAssignToAtomicCallReturn2);
		TEST(TestStructureAssignToAtomicMethodCallReturn);
		TEST(TestStructureAssignToAtomicMethodCallReturn2);
		TEST(TestStructureAssignToCompoundMethodCallReturn);
		TEST(TestStructureAssignToCompoundMethodCallReturn2);
		TEST(TestStructureAssignToSingleArgumentCompoundCallReturn);
		TEST(TestStructureAssignToSingleArgumentCompoundCallReturn2);

		RunFsbAuditTests(); // FastStringBuilder audit and fixes
	}

	void RunPositiveFailures()
	{	
		TEST(TestNativeHandleSecurity);
		TEST(TestInvokeMethodViaReflectionFails);
		TEST(TestThrowInConstructor);
		TEST(TestMissingMethod);
		TEST(TestDuplicateVariable);
		TEST(TestDuplicateFunctionError);
		TEST(TestDuplicateStructureError);
		TEST(TestBigNamespaceError);
		TEST(TestBigFunctionError);
		TEST(TestBigStructureError);
		TEST(TestBigInterfaceError);
		TEST(TestBigArchetypeError);
		TEST(TestDuplicateInterfaceError);
		TEST(TestDuplicateArchetypeError);
		TEST(TestReturnClosureWithVariableFail);
		TEST(TestDestructorThrows);
		TEST(TestDoubleArrowsInFunction);
		TEST(TestIgnoreOutputInterface);
		TEST(TestTypeInference1);
	}

	void RunGotoTests()
	{
		TEST(TestForwardGotoSiblingLabelWithFailure);
		TEST(TestBackwardGotoParentLabelWithCleanup);
		TEST(TestBackwardGotoParentLabelWithFailure);
		TEST(TestBackwardGotoParentLabel);
		TEST(TestBackwardGotoSiblingLabelWithCleanup);
		TEST(TestBackwardGotoSiblingLabel);
		TEST(TestForwardGotoSiblingLabel);
		TEST(TestForwardGotoSiblingLabelFailure);
	}

	void RunTests()
	{
		int64 start, end, hz;
		start = Time::TickCount();

		RunPositiveSuccesses();	
		RunGotoTests();
		RunPositiveFailures();
		TestArrays();
		TestLists();
		TestLists();
		TestMaps();goto finish;
	finish:
		end = Time::TickCount();
		hz = Time::TickHz();

		double dt = (double)(end - start) / (double)hz;
		printf("\n%d tests completed in %.2f seconds (%.1f tests per second)\n", testCount, dt, testCount / dt);
	}
}

#include <rococo.allocators.h>
using namespace Rococo::Memory;

int main(int argc, char* argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	AllocatorLogFlags flags;
	flags.LogDetailedMetrics = false;
	flags.LogLeaks = true;
	flags.LogOnModuleExit = true;
	SetAllocatorLogFlags(flags);

	AutoFree<IAllocatorSupervisor> sexyAllocator = CreateBlockAllocator(5120, 0, "script-test");
	SetSexyAllocator(sexyAllocator);

	Rococo::OS::SetBreakPoints(Rococo::OS::Flags::BreakFlag_All);
//	Rococo::OS::SetBreakPoints(Rococo::OS::Flags::BreakFlag_None);

//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	try
	{
		RunTests();

		size_t heapSize = sexyAllocator->EvaluateHeapSize();
		if (heapSize)
		{
			printf("Estimated memory consumption: %llu KB\n", heapSize / 1_kilobytes);
		}
	}
	catch (IException& ex)
	{
		if (ex.ErrorCode() != 0)
		{
			char numericMessage[256];
			Rococo::OS::FormatErrorMessage(numericMessage, sizeof numericMessage, ex.ErrorCode());
			if (numericMessage[0] != 0)
			{
				printf("\nSystem Error: %d %s. %s\n", ex.ErrorCode(), numericMessage, ex.Message());
			}
		}
		else
		{
			printf("\nError: %s\n", ex.Message());
		}
	}

	try
	{
		ValidateLogs(false);
	}
	catch (...)
	{
		return -1;
	}

	return 0;
}
