/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	http://www.sexiestengine.com

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	and 'Society of Demons.'

	1. This software is open-source. It can be freely copied, distributed, compiled and the compilation executed.
	
	1.a Modification of the software is not allowed where such modifcations fail to include this exact comment header in each source file, or alter, reduce or extend the Sexy language.
	The purpose of this limitation is to prevent different incompatible versions of the Sexy language emerging. 

	1.b You are allowed to fix bugs and implement performance improvements providing you inform Sexy's copyright owner via email. Such changes may then be incorporated in
	later versions of Sexy without compromising Sexy's copyright license.
	
	2. You are not permitted to copyright derivative versions of the source code. You are free to compile the code into binary libraries and include the binaries in a commercial application. 

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM “AS IS” WITHOUT
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

#include <tchar.h>
#include <stdio.h>
#include <intrin.h>
#include <memory.h>
#include <vector>
#include <string.h>


#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"

#ifdef SEXCHAR_IS_WIDE
# define _UNICODE
# define UNICODE
#endif

#include <windows.h>

#undef min
#undef max

#include <limits>

#include "sexy.lib.util.h"

#include "Sexy.Script.h"
#include "Sexy.S-Parser.h"
#include "..\\stc\\stccore\\Sexy.Compiler.h"
#include "Sexy.VM.h"
#include "sexy.vm.cpu.h"

#define validate(_Expression) if (!(_Expression)) { ShowFailure(#_Expression, __FILE__, __LINE__); Abort(); }

#define TEST(test) Test(#test, test)

using namespace Sexy;
using namespace Sexy::Sex;
using namespace Sexy::Script;
using namespace Sexy::Compiler;

namespace
{
	void FormatSysMessage(SEXCHAR* text, size_t capacity, int msgNumber)
	{
		if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, msgNumber, 0, text, (DWORD) capacity, NULL))
		{
			StringPrint(text, capacity, SEXTEXT("Code %d ( 0x%x )"), msgNumber, msgNumber);
		}
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

		void Write(csexstr text)
		{
			WriteToStandardOutput(text);
		}

		void OnUnhandledException(int errorCode, csexstr exceptionType, csexstr message, void* exceptionInstance) 
		{
			ParseException ex(Vec2i{ 0,0 },Vec2i{ 0,0 }, exceptionType, message, SEXTEXT(""), NULL);
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
	} s_logger;

	void Abort()
	{
		if (IsDebuggerPresent())
			__debugbreak();
		else
			exit(-1); 
	}

	void PrintStructures(IPublicScriptSystem& ss, ILog& log)
	{
		const IPublicProgramObject& obj = ss.PublicProgramObject();
		for(int i = 0; i < obj.ModuleCount(); ++i)
		{
			const IModule& m = obj.GetModule(i);

			SEXCHAR msg[256];
			StringPrint(msg, 256, SEXTEXT("\r\nModule %s"), m.Name()); 
			log.Write(msg);

			for(int j = 0; j < m.StructCount(); ++j)
			{
				const IStructure& s = m.GetStructure(j);

				StringPrint(msg, 256, SEXTEXT("\r\nstruct %s - %d bytes"), s.Name(), s.SizeOfStruct()); 
				log.Write(msg);

				for(int k = 0; k < s.MemberCount(); ++k)
				{
					const IMember& member = s.GetMember(k);
					StringPrint(msg, 256, SEXTEXT("  %s %s"), member.UnderlyingType()->Name(), member.Name()); 
					log.Write(msg);
				}
			}
		}
	}


	void ShowFailure(const char* expression, const char* filename, int lineNumber)
	{
		printf("Validation failed in %s[%d]: %s\r\n", filename, lineNumber, expression);
	}

	void PrintExpression(cr_sex s, int &totalOutput, int maxOutput)
	{
		switch (s.Type())
		{
		case EXPRESSION_TYPE_ATOMIC:
			totalOutput += WriteToStandardOutput(SEXTEXT(" %s"), (csexstr) s.String()->Buffer);
			break;
		case EXPRESSION_TYPE_STRING_LITERAL:
			totalOutput += WriteToStandardOutput(SEXTEXT(" \"%s\""), (csexstr) s.String()->Buffer);
			break;
		case EXPRESSION_TYPE_COMPOUND:
			
			totalOutput += WriteToStandardOutput(SEXTEXT(" ("));

			for(int i = 0; i < s.NumberOfElements(); ++i)
			{
				if (totalOutput > maxOutput) 
				{
					return;
				}
				
				cr_sex child = s.GetElement(i);
				PrintExpression(child, totalOutput, maxOutput);								
			}
			
			totalOutput += WriteToStandardOutput(SEXTEXT(" )"));
		}				
	}

	void PrintParseException(const ParseException& e)
	{
		WriteToStandardOutput(SEXTEXT("Parse error\r\nSource: %s\r\nExpression: (%d,%d) to (%d,%d)\r\nReason: %s\r\n"), e.Name(), e.Start().x, e.Start().y, e.End().x, e.End().y, e.Message());

		for (const ISExpression* s = e.Source(); s != NULL; s = s->GetOriginal())
		{
			if (s->TransformDepth() > 0)  WriteToStandardOutput(SEXTEXT("Macro expansion %d:\r\n"), s->TransformDepth());

			int totalOutput = 0;
			PrintExpression(*s, totalOutput, 1024);

			if (totalOutput > 1024) WriteToStandardOutput(SEXTEXT("..."));

			WriteToStandardOutput(SEXTEXT("\r\n"));
		}
	}

	void ValidateLogs()
	{
		if (s_logger.ExceptionCount() > 0)
		{
			ParseException ex;
			while(s_logger.TryGetNextException(ex))
			{
				PrintParseException(ex);
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

		WriteToStandardOutput(SEXTEXT("\n------------%s #%lld ---------------\n"), f.Name(), section.Id);

		size_t start = ss.PublicProgramObject().ProgramMemory().GetFunctionAddress(section.Id);
		size_t programLength = ss.PublicProgramObject().ProgramMemory().GetFunctionLength(section.Id);
		const uint8* code = ss.PublicProgramObject().ProgramMemory().StartOfMemory() + start;
		size_t i = 0;
		while(i < programLength)
		{
			VM::IDisassembler::Rep rep;
			disassembler.Disassemble(code + i, OUT rep);

			WriteToStandardOutput(SEXTEXT("%8llu %s %s\n"), i, rep.OpcodeText, rep.ArgText);

			validate (rep.ByteCount != 0);
			i += rep.ByteCount;
		}

		WriteToStandardOutput(SEXTEXT("\n\n"));
	}

	void Disassemble(csexstr fname, const IModule& module, IPublicScriptSystem& ss)
	{
		const IFunction* f = module.FindFunction(fname);
		validate(f != NULL);

		VM::IDisassembler* disassembler = ss.PublicProgramObject().VirtualMachine().Core().CreateDisassembler();	
			Disassemble(*disassembler, *f, ss);
		disassembler->Free();
	}

	void Test(const char* name, FN_TEST fnTest)
	{
		printf("<<<<<< %s\r\n", name);
		
		ProgramInitParameters pip;
		pip.MaxProgramBytes = 32768;
		CScriptSystemProxy ssp(pip, s_logger);
		
		if (&ssp() == NULL) exit(-1);

		try
		{
			fnTest(ssp());
			ValidateLogs();
		}
		catch (STCException& e)
		{
			WriteToStandardOutput(SEXTEXT("Error: %s\r\nSource: %s\r\n.Code %d"), e.Message(), e.Source(), e.Code());
			exit(e.Code());
		}
		catch (ParseException& e)
		{
			PrintParseException(e);
			exit(-1);
		}
		catch (IException& ose)
		{
         if (ose.ErrorCode() != 0)
         {
            SEXCHAR osMessage[256];
            FormatSysMessage(osMessage, 256, ose.ErrorCode());
            WriteToStandardOutput(SEXTEXT("Error code%d~%x,%s\r\n%s\r\n"), ose.ErrorCode(), ose.ErrorCode(), ose.Message(), osMessage);
         }
         else
         {
            WriteToStandardOutput(SEXTEXT("%s"), ose.Message());
         }
			exit(-1);
		}
		catch(std::exception& stdex)
		{
			printf("std::exception: %s\r\n", stdex.what());
			exit(-1);
		}

		printf("%s >>>>>>\r\n\r\n", name);
	}

	void TestConstruction()
	{
		try
		{
			ProgramInitParameters pip;
			pip.MaxProgramBytes = 32768;
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
			SEXCHAR osMessage[256];
			FormatSysMessage(osMessage, 256, ose.ErrorCode());
			WriteToStandardOutput(SEXTEXT("OS Error. %s\r\n%s\r\n"), ose.Message(), osMessage);
			exit(-1);
		}
	}

	void TestCreateNamespace(IPublicScriptSystem& ss)
	{
		Auto<ISourceCode> sc(ss.SParser().ProxySourceBuffer(SEXTEXT("(namespace Sys.Data)"), -1, Vec2i{ 0,0 }, SEXTEXT("test2")));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();
	}

	bool SetProgramAndEntryPoint(IPublicProgramObject& object, const INamespace& ns, csexstr fname)
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

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		return ss.PublicProgramObject().VirtualMachine();
	}

	void TestAssignInt32Literal(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 exitCode)(Int32 result): (exitCode = 1)(result = 0xABCDEF01))")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignInt32Literal"));
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

	void TestAssignFloat32Literal(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 exitCode)(Float32 result)(Float32 result2): (exitCode = -125)(result = 1.7)(result2 = 1.0e15))")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignFloat32Literal"));
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

	void TestAssignFloat64Literal(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 exitCode)(Float64 result)(Float64 result2):(exitCode = 0xAB)\n(result = 1.73)(result2 = -1.0e-15))")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignFloat64Literal"));
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
      csexstr srcCode =
         SEXTEXT("(namespace EntryPoint)")
         SEXTEXT("(using Sys.Maths)")
         SEXTEXT("(function Main (Int32 id) -> (Int32 exitCode): ")
         SEXTEXT("  (Matrix4x4 m)")
         SEXTEXT("  (Matrix4x4 n)")
         SEXTEXT("  (m = n)")
         SEXTEXT(")")
         SEXTEXT("(alias Main EntryPoint.Main)");
      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignMatrixVariable"));
      Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

      VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
      vm.Push(100); // Allocate stack space for the int32 exitCode
      vm.Push(101); // Allocate stack space for the int32 result

      auto result = vm.Execute(VM::ExecutionFlags(false, true));
      ValidateExecution(result);
   }

   void TestAssignVectorVariableByRef(IPublicScriptSystem& ss)
   {
      csexstr srcCode =
         SEXTEXT("(namespace EntryPoint)")
         SEXTEXT("(using Sys.Maths)")
         SEXTEXT("(function InitVec (Vec3 v) -> :  ")
         SEXTEXT("  (Vec3 n = 1 2 3)") 
         SEXTEXT("  (v = n)")
         SEXTEXT(")")
         SEXTEXT("(function Main (Int32 id) -> (Float32 a)(Float32 b)(Float32 c): ")
         SEXTEXT("  (Vec3 v)")
         SEXTEXT("  (InitVec v)")
         SEXTEXT("  (a = v.x)")
         SEXTEXT("  (b = v.y)")
         SEXTEXT("  (c = v.z)")
         SEXTEXT(")")
         SEXTEXT("(alias Main EntryPoint.Main)");
      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignVectorVariableByRef"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 exitCode)(Int64 result):")
			SEXTEXT("   (exitCode = -75)(result = 0x800FCDCDEFEFABCD)   )\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignInt64Literal"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));


		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 exitCode
		vm.Push(0x0123456712345678); // Allocate stack space for the int64 result
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int64 result = vm.PopInt64();
		int32 exitCode = vm.PopInt32();		
		validate(exitCode == -75);
		validate(result == 0x800FCDCDEFEFABCD);
	}

	void TestAssignInt64Variable(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int64 exitCode)(Int64 result): (result = 0xABCDEF01CDEF0123)(exitCode = result))")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignInt64Variable"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 exitCode)(Int32 result): (result = 0xABCDEF01)(exitCode = result))")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignInt32Variable"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Float32 exitCode)(Float32 result): (result = 0.62)(exitCode = result))")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignFloat32Variable"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Float64 exitCode)(Float64 result): (result = 0.62)(exitCode = result))")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignFloat64Variable"));
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

	void TestLocalVariable(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 exitCode):\n")
			SEXTEXT("     (Int32 result) (result = 0xABCDEF01) (exitCode = result)   )\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLocalVariable"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		vm.Push(100); // Allocate stack space for the int32 exitCode
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 exitCode = vm.PopInt32();
		validate(exitCode == 0xABCDEF01);
	}

	void TestBooleanLiteralVsLiteralMatches(IPublicScriptSystem& ss)
	{
		csexstr srcCode =			
			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z)(Bool a)(Bool b)(Bool c)(Bool d)(Bool e):\n")
			SEXTEXT("     (x = (0 > 72))\n")
			SEXTEXT("     (y = (16 > 0))\n")
			SEXTEXT("     (z = (10 < 12))\n")
			SEXTEXT("     (a = (5 < 4))\n")
			SEXTEXT("     (b = (0xABCD <= 0xABCC))\n")
			SEXTEXT("     (c = (0xABCD <= 0xABCD))\n")
			SEXTEXT("     (d = (0x000000000000ABCD >= 0x000000000000ABCE))\n")
			SEXTEXT("     (e = (1.0e100 != 1.0e-100))   )\n")
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanLiteralVsLiteralMatches"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (x = (72 > 72.1))   )\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanMismatch"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
		
		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanVariableVsLiteralMatches(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z):\n")
			SEXTEXT("     (Int32 temp)\n")
			SEXTEXT("     (temp = 25)\n")
			SEXTEXT("     (x = (temp > 25))\n")
			SEXTEXT("     (y = (24 != temp))\n")
			SEXTEXT("     (Float32 temp2)\n")
			SEXTEXT("     (temp2 = 24.1)\n")
			SEXTEXT("     (z = (24.2 >= temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanVariableVsLiteralMatches"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z):")
			SEXTEXT("     (Int32 temp1 = 25)\n")
			SEXTEXT("     (Int32 temp2 = 24)\n")
			SEXTEXT("     (x = (temp1 >= temp2))\n")
			SEXTEXT("     (y = (temp1 == temp2))\n")
			SEXTEXT("     (z = (temp1 < temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanVariableVsVariable"));
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

	void TestBooleanExpressions(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z)(Bool a)(Bool b)(Bool c):")
			SEXTEXT("     (Bool temp1 = 0)\n")
			SEXTEXT("     (Bool temp2 = 1)\n")
			SEXTEXT("     (Bool temp3 = 1)\n")
			SEXTEXT("     (a = (temp3 and 1))\n")
			SEXTEXT("     (b = (temp3 or  temp2))\n")
			SEXTEXT("     (c = (temp3 xor temp2))\n")
			SEXTEXT("     (x = (temp1 and temp2))\n")
			SEXTEXT("     (y = (temp1 or  temp2))\n")
			SEXTEXT("     (z = (temp1 xor temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanExpressions"));
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

	void TestBooleanCompoundExpressions1(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (Bool temp1 = 0)\n")
			SEXTEXT("     (Bool temp2 = 1)\n")
			SEXTEXT("     (x = ((temp2 and temp1) xor temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions1"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions2(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (Bool temp1 = 0)\n")
			SEXTEXT("     (Bool temp2 = 1)\n")
			SEXTEXT("     (x = ((temp2 xor temp1) xor temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanCompoundExpressions3(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (Bool temp1 = 0)\n")
			SEXTEXT("     (Bool temp2 = 1)\n")
			SEXTEXT("     (x = ((temp2 and temp1) or temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions3"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions4(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (Bool temp1 = 0)\n")
			SEXTEXT("     (Bool temp2 = 1)\n")
			SEXTEXT("     (x = ((temp2 or temp1) and temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions4"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions5(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (Bool temp1 = 0)\n")
			SEXTEXT("     (Bool temp2 = 1)\n")
			SEXTEXT("     (x = ((temp2 and temp1) and temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions5"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanCompoundExpressions6(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (Bool temp1 = 0)\n")
			SEXTEXT("     (Bool temp2 = 1)\n")
			SEXTEXT("     (x = (temp2 and (temp2 and temp1)))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions6"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanCompoundExpressions7(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (Bool temp1 = 0)\n")
			SEXTEXT("     (Bool temp2 = 1)\n")
			SEXTEXT("     (x = (temp2 or (temp2 and temp1)))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions7"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions8(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (Bool temp1 = 0)\n")
			SEXTEXT("     (Bool temp2 = 1)\n")
			SEXTEXT("     (x = (true and (true and temp1)))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions8"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanCompoundExpressions9(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (Bool temp1 = 0)\n")
			SEXTEXT("     (Bool temp2 = 1)\n")
			SEXTEXT("     (x = ((true or false) and (true and temp1)))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions9"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestBooleanCompoundExpressions10(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (Bool temp1 = 0)\n")
			SEXTEXT("     (Bool temp2 = 1)\n")
			SEXTEXT("     (x = ((((temp2 and (temp2 or (temp1))) and (true and (temp1 or temp2))))))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions10"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions11(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (x = ((((true and (7 >= 6)) xor (true and (4.2 < 3.3))))))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions11"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestBooleanCompoundExpressions12(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Bool x):\n")
			SEXTEXT("     (Float32 temp1 = 0.5)\n")
			SEXTEXT("     (Float32 temp2 = 0.25)\n")
			SEXTEXT("     (x = (((true and (temp1 >= temp2)) xor (true and (temp2 != temp1)))))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBooleanCompoundExpressions12"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestIfThen1(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 x):\n")
			SEXTEXT("     (Float32 temp1 = 0.5)\n")
			SEXTEXT("     (Float32 temp2 = 0.25)\n")
			SEXTEXT("     (if (temp2 < temp1)\n")
			SEXTEXT("				(x = 74)\n")
			SEXTEXT("			 else\n")
			SEXTEXT("				(x = 77)")
			SEXTEXT("     )\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestIfThen1"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 74);
	}

	void TestIfThen2(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 x):\n")
			SEXTEXT("     (Float32 temp1 = 0.5)\n")
			SEXTEXT("     (Float32 temp2 = 0.25)\n")
			SEXTEXT("     (if ((temp1 > 0.3) and (temp2 < temp1))\n")
			SEXTEXT("				(if (temp2 > 0) (x = 74) else (x = 73))\n")
			SEXTEXT("			 else\n")
			SEXTEXT("				(x = 77)")
			SEXTEXT("     )\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestIfThen2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the int32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 74);
	}

   void TestFloatArithmeticByMember(IPublicScriptSystem& ss)
   {
      csexstr srcCode =
         SEXTEXT("(namespace EntryPoint)\n")
         SEXTEXT("(function Main -> (Float32 x)(Float32 y)(Float32 z)(Float32 w):\n")
         SEXTEXT("     (Sys.Maths.Vec2 screenSpan = 10 20)\n")

         SEXTEXT("     (Float32 halfX = (0.5 * screenSpan.x))\n")
         SEXTEXT("     (Float32 halfY = (0.5 * screenSpan.y))\n")
         SEXTEXT("     (Float32 x0)\n")
         SEXTEXT("     (Float32 x1)\n")
         SEXTEXT("     (x0 = (halfX - 20))\n")
         SEXTEXT("     (x1 = (halfY + 20))\n")

         SEXTEXT("     (x = x0)\n")
         SEXTEXT("     (y = x1)\n")
         SEXTEXT("	  (z = (x0 + x1))\n")
         SEXTEXT("     (w = 0)\n")
         SEXTEXT(")\n")
         SEXTEXT("(alias Main EntryPoint.Main)");
      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFloatArithmeticByMember"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float32 x)(Float32 y)(Float32 z)(Float32 w):\n")
			SEXTEXT("     (Float32 temp1 = 0.5)\n")
			SEXTEXT("     (Float32 temp2 = 0.25)\n")
			SEXTEXT("     (x = (temp1 - temp2))\n")
			SEXTEXT("			(y = (temp1 + temp2))\n")
			SEXTEXT("     (z = (temp1 * temp2))\n")
			SEXTEXT("			(w = (temp1 / temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFloatArithmetic1"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float64 x):\n")
			SEXTEXT("     (Float64 temp1 = 0.5)\n")
			SEXTEXT("     (x = (temp1 - 0.1))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDoubleArithmetic0"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float64 x):\n")
			SEXTEXT("     (Float64 temp1 = 0.5)\n")
			SEXTEXT("     (Float64 temp2 = 0.25)\n")
			SEXTEXT("     (x = (temp1 - temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDoubleArithmetic1"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float64 x):\n")
			SEXTEXT("     (Float64 temp1 = 0.5)\n")
			SEXTEXT("     (Float64 temp2 = 0.25)\n")
			SEXTEXT("     (x = (temp1 + temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDoubleArithmetic2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0); // Allocate stack space for the float64 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float64 x = vm.PopFloat64();
		validate(x == 0.75);
	}

	void TestDoubleArithmetic3(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float64 x):\n")
			SEXTEXT("     (Float64 temp1 = 0.5)\n")
			SEXTEXT("     (Float64 temp2 = 0.25)\n")
			SEXTEXT("     (x = (temp1 * temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDoubleArithmetic3"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0); // Allocate stack space for the float64 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float64 x = vm.PopFloat64();
		validate(x == 0.125);
	}

	void TestDoubleArithmetic4(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float64 x):\n")
			SEXTEXT("     (Float64 temp1 = 0.5)\n")
			SEXTEXT("     (Float64 temp2 = 0.25)\n")
			SEXTEXT("	  (x = (temp1 / temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDoubleArithmetic4"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0); // Allocate stack space for the float64 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float64 x = vm.PopFloat64();
		validate(x == 2.0);
	}

	void TestInt32Arithmetic(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 x)(Int32 y)(Int32 z)(Int32 w):\n")
			SEXTEXT("     (Int32 temp1 = 8)\n")
			SEXTEXT("     (Int32 temp2 = 4)\n")
			SEXTEXT("     (x = (temp1 - temp2))\n")
			SEXTEXT("			(y = (temp1 + temp2))\n")
			SEXTEXT("     (z = (temp1 * temp2))\n")
			SEXTEXT("			(w = (temp1 / temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestInt32Arithmetic"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int64 x)(Int64 y)(Int64 z)(Int64 w):\n")
			SEXTEXT("     (Int64 temp1 = 8)\n")
			SEXTEXT("     (Int64 temp2 = 4)\n")
			SEXTEXT("     (x = (temp1 - temp2))\n")
			SEXTEXT("			(y = (temp1 + temp2))\n")
			SEXTEXT("     (z = (temp1 * temp2))\n")
			SEXTEXT("			(w = (temp1 / temp2))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestInt64Arithmetic"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("     (Int32 temp1 = 8)")
			SEXTEXT("     (Int32 temp2 = 4)")
			SEXTEXT("			(result = ((temp1 / temp2) + (temp1 * temp2)))")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestInt32CompoundArithmetic"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 i = vm.PopInt32();
		validate(i == 34);
	}

	void TestFloatCompoundArithmetic(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float32 x)(Float32 y)(Float32 z)(Float32 w):\n")
			SEXTEXT("     (Float32 temp1 = 0.5)\n")
			SEXTEXT("     (Float32 temp2 = 0.25)\n")
			SEXTEXT("     (x = ((temp1)))\n")
			SEXTEXT("			(y = (temp1 - (temp1 + temp2)))\n")
			SEXTEXT("     (z = ((temp1 * temp2) + temp2))\n")
			SEXTEXT("			(w = ((temp1 / temp2) + (temp1 * temp2)))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFloatCompoundArithmetic"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float32 x)(Float32 y)(Float32 z)(Float32 w):\n")
			SEXTEXT("     (Float32 temp1 = 0.5)\n")
			SEXTEXT("     (Float32 temp2 = 0.25)\n")
			SEXTEXT("     (x = (((7))))\n")
			SEXTEXT("			(y = (x - (2 + 3)))\n")
			SEXTEXT("     (z = ((4 * 8) + 4))\n")
			SEXTEXT("			(w = ((9 / 3) + (2 * 7)))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFloatCompoundArithmetic"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 x):\n")
			SEXTEXT("     (Float32 temp1 = 1)\n")
			SEXTEXT("     (Float32 temp2 = 2)\n")
			SEXTEXT("     (if ((temp1 - temp2) >= 0.25) (x = 5) else (x = 7)) \n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestIfThen3"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the float32 x
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 7);
	}

	void TestFunctionCall1(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float32 exitValue1):\n")
			SEXTEXT("    (exitValue1 = (GenerateResult))")
			SEXTEXT(")\n")
			SEXTEXT("(function GenerateResult -> (Float32 x):\n")
			SEXTEXT("     (x = 5)\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFunctionCall1"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0f); // Allocate stack space for the float32 exitValue1
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		float32 x = vm.PopFloat32();
		validate(x == 5.0f);
	}

	void TestFunctionCall2(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 exitValue):\n")
			SEXTEXT("    (exitValue = (1 + (GenerateResult)))")
			SEXTEXT("    (exitValue = (3 + (GenerateResult)))")
			SEXTEXT("    (exitValue = (5 + (GenerateResult)))")
			SEXTEXT(")\n")
			SEXTEXT("(function GenerateResult -> (Int32 x):\n")
			SEXTEXT("     (x = 5)\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFunctionCall2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the Int32 exitValue
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 z = vm.PopInt32();
		validate(z == 10);
	}

	void TestFunctionCall2_(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float32 exitValue1)(Float32 exitValue2)(Float32 exitValue3):\n")
			SEXTEXT("    (exitValue1 = (1 + (GenerateResult)))")
			SEXTEXT("    (exitValue2 = (3 + (GenerateResult)))")
			SEXTEXT("    (exitValue3 = (5 + (GenerateResult)))")
			SEXTEXT(")\n")
			SEXTEXT("(function GenerateResult -> (Float32 x):\n")
			SEXTEXT("     (x = 5)\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFunctionCall2_"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 exitValue1):\n")
			SEXTEXT("    (exitValue1 = (Square 7))")
			SEXTEXT(")\n")
			SEXTEXT("(function Square (Int32 x) -> (Int32 y):\n")
			SEXTEXT("     (y = (x * x))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFunctionCall3"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100); // Allocate stack space for the float32 exitValue1
		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));
		int32 x = vm.PopInt32();
		validate(x == 49);
	}

	void TestFunctionCall4(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float32 x) (Int32 y) (Float64 z) (Int64 w) (Bool isOK):\n")
			SEXTEXT("    (Float32 a = 5)")
			SEXTEXT("    (Float64 b = 6)")
			SEXTEXT("    (Int32 c = 7)")
			SEXTEXT("    (Int64 d = 8)")
			SEXTEXT("    (Bool e = true)")
			SEXTEXT("    (F a b c d e -> x y z w isOK)")
			SEXTEXT(")\n")
			SEXTEXT("(function F (Float32 a) (Float64 b) (Int32 c) (Int64 d) (Bool e) -> (Float32 x) (Int32 y) (Float64 z) (Int64 w) (Bool isOK):\n")
			SEXTEXT("     (x = (a * a))")
			SEXTEXT("     (y = (c * c))")
			SEXTEXT("     (z = (b * b))")
			SEXTEXT("     (w = (d * d))")
			SEXTEXT("     (isOK = (false xor e))")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFunctionCall4"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 exitValue1):\n")
			SEXTEXT("    (exitValue1 = (Factorial 5))")
			SEXTEXT(")\n")
			SEXTEXT("(function Factorial (Int32 x) -> (Int32 result):\n")
			SEXTEXT("     (if (x <= 1) (result = 1) else (result = (x * (Factorial(x - 1)))))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFunctionCallRecursion1"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 exitValue1):\n")
			SEXTEXT("    (exitValue1 = (MyFunction1 5000))")
			SEXTEXT(")\n")
			SEXTEXT("(function MyFunction1 (Int32 x) -> (Int32 result):\n")
			SEXTEXT("     (if (x > 100) (result = (MyFunction2 x)) else (result = x))\n")
			SEXTEXT(")\n")
			SEXTEXT("(function MyFunction2 (Int32 x) -> (Int32 result):\n")
			SEXTEXT("     (if (x > 100) (result = (MyFunction1 (x / 2))) else (result = x))\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFunctionCallRecursion2"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 x)(Float64 y):\n")
			SEXTEXT("    (MyFunction 5000 -> x y)")
			SEXTEXT(")\n")
			SEXTEXT("(function MyFunction (Int32 x) -> (Int32 result1)(Float64 result2):\n")
			SEXTEXT("     (result1 = (x * 2))\n")
			SEXTEXT("     (result2 = 25)\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestFunctionCallMultiOutput1"));
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

	void TestStructure(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Float32 result):\n")
			SEXTEXT("	 (Vector4 v)")
			SEXTEXT("    (v.x = 12)")
			SEXTEXT("    (v.y = 10)")
			SEXTEXT("    (v.z = 2)")
			SEXTEXT("    (if (v.z == 2) (result = (v.x * v.y)))")
			SEXTEXT(")\n")
			SEXTEXT("(struct Vector4\n")
			SEXTEXT("	(Float32 x y z)\n")
			SEXTEXT("	(Float32 w)\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStructure"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(100.0f); // Allocate stack space for the float32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		float32 x = vm.PopFloat32();
		validate(x == 120.0f);
	}

	void TestStructure2(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 result):\n")
			SEXTEXT("		 (Vector3i v)\n")
			SEXTEXT("    (v.x = 3)\n")
			SEXTEXT("    (v.y = 5)\n")
			SEXTEXT("    (v.z = 7)\n")
			SEXTEXT("    (result = (SumAndProduct3i v))\n")
			SEXTEXT(")\n")
			SEXTEXT("(function SumAndProduct3i (Vector3i a) -> (Int32 sum):\n")
			SEXTEXT("		 (sum = ((a.x + a.y) * a.z))\n")
			SEXTEXT(")\n")
			SEXTEXT("(struct Vector3i\n")
			SEXTEXT("     (Int32 x y z)\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStructure2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		const IModule& m = ss.PublicProgramObject().GetModule(0);

		vm.Push(100); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 56);
	}

	void TestStructure3(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 result):\n")
			SEXTEXT("		 (Vector3i v)\n")
			SEXTEXT("		 (Vector3i w)\n")
			SEXTEXT("    (v.x = 2)\n")
			SEXTEXT("    (v.y = 3)\n")
			SEXTEXT("    (v.z = 4)\n")
			SEXTEXT("    (w.x = 5)\n")
			SEXTEXT("    (w.y = 7)\n")
			SEXTEXT("    (w.z = 9)\n")
			SEXTEXT("    (result = (Dot3i v w))\n")
			SEXTEXT(")\n")
			SEXTEXT("(function Dot3i (Vector3i a)(Vector3i b) -> (Int32 product):\n")
			SEXTEXT("		 (product = ((a.z * b.z) + ((a.x * b.x) + (a.y * b.y))))\n")
			SEXTEXT(")\n")
			SEXTEXT("(struct Vector3i\n")
			SEXTEXT("     (Int32 x y z)\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStructure3"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("    (alias Main EntryPoint.Main)")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("	 (Quat4i a)")
			SEXTEXT("	 (Quat4i b)")
			SEXTEXT("    (a.v.x = 2)")
			SEXTEXT("    (a.v.y = 3)")
			SEXTEXT("    (a.v.z = 4)")
			SEXTEXT("    (a.s = 5)")
			SEXTEXT("    (b.v.x = 5)")
			SEXTEXT("    (b.v.y = 7)")
			SEXTEXT("    (b.v.z = 9)")
			SEXTEXT("    (b.s   = 11)")
			SEXTEXT("    (Quat4i sum)")
			SEXTEXT("    (AddQuat4i a b sum)")
			SEXTEXT("    (result = 1)")
			SEXTEXT(")")
			SEXTEXT("(function AddQuat4i (Quat4i a)(Quat4i b)(Quat4i sum) -> :")
			SEXTEXT("	 (sum.v.x = (a.v.x + b.v.x))")
			SEXTEXT("	 (sum.v.y = (a.v.y + b.v.y))")
			SEXTEXT("	 (sum.v.z = (a.v.z + b.v.z))")
			SEXTEXT("	 (sum.s   = (a.s   + b.s))")
			SEXTEXT(")")
			SEXTEXT("(struct Vector3i")
			SEXTEXT("     (Int32 x y z)")
			SEXTEXT(")")
			SEXTEXT("(struct Quat4i")
			SEXTEXT("     (Vector3i v)(Int32 s)")
			SEXTEXT(")")
			SEXTEXT("(struct Quat4ia")
			SEXTEXT("     (Vec3i v)(Int32 s)")
			SEXTEXT(")")
			SEXTEXT("(struct Quat4ib")
			SEXTEXT("     (Test.Maths.Vec3i v)(Int32 s)")
			SEXTEXT(")")
			SEXTEXT("(using Test.Maths)")
			SEXTEXT("(namespace Test)")
			SEXTEXT("(namespace Test.Maths)")
			SEXTEXT("    (alias Vector3i Test.Maths.Vec3i)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStructure4"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 result):\n")
			SEXTEXT("		 (Int32 count = 0)\n")
			SEXTEXT("		 (result = 10)\n")
			SEXTEXT("		 (while (count < 2)\n")
			SEXTEXT("       (count = (count + 1)) (result = (result + 1)) \n")
			SEXTEXT("    ) \n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestWhileLoop1"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 12);
	}

	void TestWhileLoopBreak(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 result):\n")
			SEXTEXT("		 (Int32 count = 0)\n")
			SEXTEXT("		 (result = 10)\n")
			SEXTEXT("		 (while (count < 6)\n")
			SEXTEXT("       (count = (count + 1)) \n")
			SEXTEXT("       (if (count == 4) (break)) \n")
			SEXTEXT("       (result = (result + 1)) \n")
			SEXTEXT("    ) \n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestWhileLoopBreak"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 result):\n")
			SEXTEXT("		 (Int32 count = 0)\n")
			SEXTEXT("		 (result = 10)\n")
			SEXTEXT("		 (while (count < 6)\n")
			SEXTEXT("       (count = (count + 1)) \n")
			SEXTEXT("       (if (count == 4) (continue)) \n")
			SEXTEXT("       (result = (result + 1)) \n")
			SEXTEXT("    ) \n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestWhileLoopContinue"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 result):\n")
			SEXTEXT("		 (Int32 count = 15)\n")
			SEXTEXT("		 (result = 10)\n")
			SEXTEXT("		 (do (result = (result + 1))\n")
			SEXTEXT("      (count = (count - 1))")
			SEXTEXT("       while (count > 0) \n")
			SEXTEXT("    ) \n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDoWhile"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 result):\n")
			SEXTEXT("		 (Int32 count = 15)\n")
			SEXTEXT("		 (result = 10)\n")
			SEXTEXT("		 (do (result = (result + 1))\n")
			SEXTEXT("      (count = (count - 1))")
			SEXTEXT("      (if (count == 2) (break))")
			SEXTEXT("       while (count > 0) \n")
			SEXTEXT("    ) \n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDoWhileBreak"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 result):\n")
			SEXTEXT("		 (Int32 count = 15)\n")
			SEXTEXT("		 (result = 10)\n")
			SEXTEXT("		 (do (count = (count - 1))\n")
			SEXTEXT("        (if (count == 2) (continue))")
			SEXTEXT("        (result = (result + 1))\n")			
			SEXTEXT("       while (count > 0) \n")
			SEXTEXT("    ) \n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDoWhileContinue"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> (Int32 result):\n")
			SEXTEXT("		 (Int32 count = 0)\n")
			SEXTEXT("		 (Int32 i = 0)\n")
			SEXTEXT("		 (result = 10)\n")
			SEXTEXT("		 (while (count < 2)\n")
			SEXTEXT("       (count = (count + 1)) \n")
			SEXTEXT("       (i = 0) \n")
			SEXTEXT("       (while (i < 2) (i = (i + 1)) (result = (result + 1)) ) \n")
			SEXTEXT("    ) \n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestNestedWhileLoops"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint) \n")
			SEXTEXT("(function Main -> (Int32 result): \n")
			SEXTEXT("		 (result = 1) \n")
			SEXTEXT(") \n")
			SEXTEXT("(archetype EntryPoint.FloatToFloat (Float32 x) -> (Float32 y)) \n")
			SEXTEXT("(alias Main EntryPoint.Main) \n");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArchetypeDeclaration"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1);

		const INamespace* testSpace = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(testSpace != NULL);

		const IArchetype* decl = testSpace->FindArchetype(SEXTEXT("FloatToFloat"));
		validate(decl != NULL);
		validate(decl->NumberOfInputs() == 1);
		validate(decl->NumberOfOutputs() == 1);
		validate(&decl->GetArgument(0) == &decl->GetArgument(1) && decl->GetArgument(0).VarType() == VARTYPE_Float32);
	}

	void TestNullArchetype(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint) \n")
			SEXTEXT("(function Main -> (Int32 result): \n")
			SEXTEXT("		(EntryPoint.FloatToInt f)\n")
			SEXTEXT("		(f 1 ->	result) \n")
			SEXTEXT(") \n")
			SEXTEXT("(archetype EntryPoint.FloatToInt (Float32 x) -> (Int32 y)) \n")
			SEXTEXT("(alias Main EntryPoint.Main) \n");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestNullArchetype"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint) \n")
			SEXTEXT("(function Main -> (Int32 result): \n")
			SEXTEXT("		(EntryPoint.INode node)\n")
			SEXTEXT("		(EntryPoint.INode next = node.Next) \n")
			SEXTEXT("       (result = next.Value) \n")
			SEXTEXT(") \n")
			SEXTEXT("(interface EntryPoint.INode \n")
		    SEXTEXT("   (Next -> (EntryPoint.INode next)) \n")
			SEXTEXT("   (Value -> (Int32 id)) \n")
		    SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main) \n");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestDefaultNullMethod"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint) \n")
			SEXTEXT("(using EntryPoint) \n")
			SEXTEXT("(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y) ) \n")
			SEXTEXT("(function Main -> (Int32 result): \n")
			SEXTEXT("		 (IntToInt f = Square) \n")
			SEXTEXT("    (result = (f 5)) \n")
			SEXTEXT(") \n")
			SEXTEXT("(function Square (Int32 x) -> (Int32 result): \n")
			SEXTEXT("		 (result = (x * x)) \n")
			SEXTEXT(") \n")			
			SEXTEXT("(alias Main EntryPoint.Main) \n");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArchetypeCall"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

// 		IModule& m = ss.PublicProgramObject().GetModule(0);
// 		Disassemble(SEXTEXT("Main"), m, ss);

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 25);
	}

	void TestArchetypePropagation(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint) \n")
			SEXTEXT("(using EntryPoint) \n")
			SEXTEXT("(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y)) \n")
			SEXTEXT("(function Main -> (Int32 result): \n")
			SEXTEXT("    (result = (Call Square 6)) \n")
			SEXTEXT(") \n")
			SEXTEXT("(function Square (Int32 x) -> (Int32 result): \n")
			SEXTEXT("		 (result = (x * x)) \n")
			SEXTEXT(") \n")			
			SEXTEXT("(function Call (EntryPoint.IntToInt f)(Int32 x) -> (Int32 result): \n")
			SEXTEXT("		 (result = (f x)) \n")
			SEXTEXT(") \n")			
			SEXTEXT("(alias Main EntryPoint.Main) \n");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArchetypePropagation"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 36);
	}

	void TestArchetypeReturn(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint) \n")
			SEXTEXT("(using EntryPoint) \n")
			SEXTEXT("(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y)) \n")
			SEXTEXT("(function Main -> (Int32 result): \n")
			SEXTEXT("		 (IntToInt f = (GetMathsFunction)) \n")
			SEXTEXT("    (result = (f 7)) \n")
			SEXTEXT(") \n")
			SEXTEXT("(function Square (Int32 x) -> (Int32 result): \n")
			SEXTEXT("		 (result = (x * x)) \n")
			SEXTEXT(") \n")			
			SEXTEXT("(function GetMathsFunction -> (EntryPoint.IntToInt f): \n")
			SEXTEXT("		 (f = Square) \n")
			SEXTEXT(") \n")			
			SEXTEXT("(alias Main EntryPoint.Main) \n");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArchetypeReturn"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint) \n")
			SEXTEXT("(using EntryPoint) \n")
			SEXTEXT("(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y)) \n")
			SEXTEXT("(function Main -> (Int32 result): \n")
			SEXTEXT("		 (IntToInt f) \n")
			SEXTEXT("    (GetMathsFunction -> f)")
			SEXTEXT("    (result = (f 7)) \n")
			SEXTEXT(") \n")
			SEXTEXT("(function Square (Int32 x) -> (Int32 result): \n")
			SEXTEXT("		 (result = (x * x)) \n")
			SEXTEXT(") \n")			
			SEXTEXT("(function GetMathsFunction -> (EntryPoint.IntToInt f): \n")
			SEXTEXT("		 (f = Square) \n")
			SEXTEXT(") \n")			
			SEXTEXT("(alias Main EntryPoint.Main) \n");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArchetypeReturnFromMultipleOutput"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(using EntryPoint)")
			SEXTEXT("(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y) )")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("		 (Job job)")
			SEXTEXT("    (job.f = Square)")
			SEXTEXT("    (job.arg = 9)")
			SEXTEXT("    (result = (Call job))")
			SEXTEXT(")")
			SEXTEXT("(function Square (Int32 x) -> (Int32 result):")
			SEXTEXT("		 (result = (x * x))")
			SEXTEXT(")")			
			SEXTEXT("(function Call (Job job) -> (Int32 result):")
			SEXTEXT("		 (result = (job.f job.arg))")
			SEXTEXT(")")		
			SEXTEXT("(struct Job")
			SEXTEXT("		 (IntToInt f)")
			SEXTEXT("		 (Int32 arg)")
			SEXTEXT(")")			
			SEXTEXT("(alias Main EntryPoint.Main) \n");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestEmbeddedArchetype"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(using EntryPoint)")
			SEXTEXT("(archetype EntryPoint.IntToInt (Int32 x) -> (Int32 y))")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("		(IntToInt f = ")
			SEXTEXT("			(closure (Int32 x) -> (Int32 y):")
			SEXTEXT("				(y = (x * x))")
			SEXTEXT("			)")
			SEXTEXT("		)")
			SEXTEXT("   (result = (f 7))")
			SEXTEXT(")")	
			SEXTEXT("(alias Main EntryPoint.Main)");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestClosure"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 49);
	}

	void TestClosureWithVariable(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(using EntryPoint)")
			SEXTEXT("(archetype EntryPoint.VoidToInt -> (Int32 y))")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("		(Int32 a = 2)")
			SEXTEXT("		(VoidToInt f = ")
			SEXTEXT("			(closure -> (Int32 y):")
			SEXTEXT("				(y = (a * 7))")
			SEXTEXT("			)")
			SEXTEXT("		)")
			SEXTEXT("   (result = (f))")
			SEXTEXT(")")	
			SEXTEXT("(alias Main EntryPoint.Main)");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestClosureWithVariable"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(using EntryPoint)")
			SEXTEXT("(archetype EntryPoint.VoidToInt -> (Int32 y))")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("		(VoidToInt f = (CallInternalClosure))")
			SEXTEXT("   (result = (f))")
			SEXTEXT(")")	
			SEXTEXT("(function CallInternalClosure -> (VoidToInt result):")
			SEXTEXT("    (result = (closure -> (Int32 y): (y = (3 * 7))))")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestReturnClosureWithVariableSucceed"));
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

		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(using EntryPoint)")
			SEXTEXT("(archetype EntryPoint.VoidToInt -> (Int32 y))")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("		(VoidToInt f = (CallInternalClosure))")
			SEXTEXT("   (result = (f))")
			SEXTEXT(")")	
			SEXTEXT("(function CallInternalClosure -> (VoidToInt result):")
			SEXTEXT("    (Int32 a = 4)")
			SEXTEXT("    (result = (closure -> (Int32 y): (y = (a * 7))))")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestReturnClosureWithVariableFails"));
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
				LARGE_INTEGER int64Hz;
				QueryPerformanceFrequency(&int64Hz);
				int64 hz = int64Hz.QuadPart;				
				WriteOutput(0, hz, e);
			}

			static void CpuTime(NativeCallEnvironment& e)
			{
				LARGE_INTEGER i64count;
				QueryPerformanceCounter(&i64count);
				int64 count = i64count.QuadPart;
				WriteOutput(0, count, e);
			}
		};

		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Time"));

		try
		{
			ss.AddNativeCall(ns, ANON::CpuHz, NULL, SEXTEXT("CpuHz -> (Int64 hz)"));
			ss.AddNativeCall(ns, ANON::CpuTime, NULL, SEXTEXT("CpuTime -> (Int64 count)"));
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}	

		csexstr srcCode =
			SEXTEXT("(function Main -> (Int64 result):")
			SEXTEXT("		(result = (Sys.Time.CpuHz))")
			SEXTEXT(")")				
			SEXTEXT("(namespace EntryPoint)(alias Main EntryPoint.Main)");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestNativeCall"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x1000000000000000); // Allocate stack space for the int64 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int64 x = vm.PopInt64();

		LARGE_INTEGER int64Hz;
		QueryPerformanceFrequency(&int64Hz);
		int64 hz = int64Hz.QuadPart;	

		validate(x == hz);
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

		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Maths"));

		try
		{
			ss.AddNativeCall(ns, ANON::Square, NULL, SEXTEXT("Square (Int32 x)-> (Int32 y)"));
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}	

		csexstr srcCode =
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("		(result = (Sys.Maths.Square 11))")
			SEXTEXT(")")				
			SEXTEXT("(namespace EntryPoint)(alias Main EntryPoint.Main)");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestNativeCall2"));
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

		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Maths"));

		try
		{
			ss.AddNativeCall(ns, ANON::George, NULL, SEXTEXT("George (Int32 x)(Int32 y)-> (Int32 pxy)(Int32 dxy)"));
		}
		catch (IException& ex)
		{
			s_logger.Write(ex.Message());
			validate(false);
		}	

		csexstr srcCode =
			SEXTEXT("(function Main -> (Int32 result1)(Int32 result2):")
			SEXTEXT("		(Sys.Maths.George 11 9 -> result1 result2 )")
			SEXTEXT(")")				
			SEXTEXT("(namespace EntryPoint)(alias Main EntryPoint.Main)");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestNativeCall3"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result1):")
			SEXTEXT(")")				
			SEXTEXT("(alias Main EntryPoint.Main)")

			SEXTEXT("(interface EntryPoint.IPlayer")				
			SEXTEXT("    (GetId -> (Int32 value))")
			SEXTEXT("    (SetId (Int32 value) ->)")
			SEXTEXT(")");
			
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestInterfaceDefinition"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		const IInterface* i = ns->FindInterface(SEXTEXT("IPlayer"));
		validate(i != NULL);		
		validate(i->MethodCount() == 2);
		const IArchetype& a1 = i->GetMethod(0);
		const IArchetype& a2 = i->GetMethod(1);
		validate(AreEqual(a1.Name(), SEXTEXT("GetId")));
		validate(AreEqual(a2.Name(), SEXTEXT("SetId")));
		validate(a1.NumberOfInputs() == 1);
		validate(a1.NumberOfOutputs() == 1);
		validate(a2.NumberOfInputs() == 2);
		validate(a2.NumberOfOutputs() == 0);
		validate(AreEqual(a1.GetArgName(0), SEXTEXT("value")));
		validate(AreEqual(a2.GetArgName(0), SEXTEXT("value")));
		validate(a1.GetArgument(0).VarType() == VARTYPE_Int32);
		validate(a2.GetArgument(0).VarType() == VARTYPE_Int32);
	} 

	void TestClassDefinition(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result1):")
			SEXTEXT(")")				
			SEXTEXT("(alias Main EntryPoint.Main)")

			SEXTEXT("(interface EntryPoint.IPlayer")				
			SEXTEXT("    (GetId -> (Int32 value))")
			SEXTEXT("    (SetId (Int32 value) ->)")
			SEXTEXT(")")

			SEXTEXT("(class Player (implements EntryPoint.IPlayer)")				
			SEXTEXT("    (Int32 id)")
			SEXTEXT(")")
			
			SEXTEXT("(method Player.GetId -> (Int32 value):")				
			SEXTEXT("    (value = this.id)")
			SEXTEXT(")")
			
			SEXTEXT("(method Player.SetId (Int32 value) -> :")				
			SEXTEXT("    (this.id = value)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestClassDefinition"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		const IModule& m = ss.PublicProgramObject().GetModule(ss.GetIntrinsicModuleCount());
		const IStructure* s = m.FindStructure(SEXTEXT("Player"));

		validate(s != NULL);
		validate(s->InterfaceCount() == 1);
		const IInterface& i1 = s->GetInterface(0);
		validate(&i1 != NULL);

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		const IInterface* i2 = ns->FindInterface(SEXTEXT("IPlayer"));

		validate(i2 != NULL);
		validate(&i1 == i2);
	} 

	void TestMissingMethod(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result1):")
			SEXTEXT(")")				
			SEXTEXT("(alias Main EntryPoint.Main)")

			SEXTEXT("(interface EntryPoint.IPlayer")				
			SEXTEXT("    (GetId -> (Int32 value))")
			SEXTEXT("    (SetId (Int32 value) ->)")
			SEXTEXT(")")

			SEXTEXT("(class Player (implements EntryPoint.IPlayer)")				
			SEXTEXT("    (Int32 id)")
			SEXTEXT(")")

			SEXTEXT("(method Player.GetId -> (Int32 value):")				
			SEXTEXT("    (value = this.id)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMissingMethod"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			validate(false);
		}
		catch (ParseException& e)
		{
			csexstr msg = e.Message();
			validate(GetSubString(msg, SEXTEXT("Expecting method")) != NULL);
			validate(GetSubString(msg, SEXTEXT("Player.SetId")) != NULL);
		}
	} 

   void TestDuplicateVariable(IPublicScriptSystem& ss)
   {
      csexstr srcCode =
         SEXTEXT("(namespace EntryPoint)\n")
         SEXTEXT("(function Main -> (Int32 exitCode):\n")
         SEXTEXT("  (Int32 a = 1)")
         SEXTEXT("  (Int32 a = 1)")
         SEXTEXT(")\n")
         SEXTEXT("(alias Main EntryPoint.Main)\n");

      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDuplicateVariable"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("    (Player player)")
			SEXTEXT("    (player.SetId 1812)")
			SEXTEXT("    (player.GetId -> result)")
			SEXTEXT(")")				
			SEXTEXT("(alias Main EntryPoint.Main)")

			SEXTEXT("(interface EntryPoint.IPlayer")				
			SEXTEXT("    (GetId -> (Int32 value))")
			SEXTEXT("    (SetId (Int32 value) ->)")
			SEXTEXT(")")

			SEXTEXT("(class Player (implements EntryPoint.IPlayer)")				
			SEXTEXT("    (Int32 id)")
			SEXTEXT(")")

			SEXTEXT("(method Player.GetId -> (Int32 value):")				
			SEXTEXT("    (value = this.id)")
			SEXTEXT(")")

			SEXTEXT("(method Player.SetId (Int32 value) -> :")				
			SEXTEXT("    (this.id = value)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestClassInstance"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("    (Player player (1812))")
			SEXTEXT("    (player.GetId -> result)")
			SEXTEXT(")")				
			SEXTEXT("(alias Main EntryPoint.Main)")

			SEXTEXT("(interface EntryPoint.IPlayer")				
			SEXTEXT("    (GetId -> (Int32 value))")
			SEXTEXT("    (SetId (Int32 value) ->)")
			SEXTEXT(")")

			SEXTEXT("(class Player (implements EntryPoint.IPlayer)")				
			SEXTEXT("    (Int32 id)")
			SEXTEXT(")")

			SEXTEXT("(method Player.GetId -> (Int32 value):")				
			SEXTEXT("    (value = this.id)")
			SEXTEXT(")")

			SEXTEXT("(method Player.SetId (Int32 value) -> :")				
			SEXTEXT("    (this.id = value)")
			SEXTEXT(")")

			SEXTEXT("(method Player.Construct (Int32 value): ")				
			SEXTEXT("    (this.id = value)")
			SEXTEXT(")")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestConstructor"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("")			
			SEXTEXT("    (EntryPoint.IPlayer player)")
			SEXTEXT("    (player.GetId -> result)")
			SEXTEXT(")")				
			SEXTEXT("(alias Main EntryPoint.Main)")

			SEXTEXT("(interface EntryPoint.IPlayer")				
			SEXTEXT("    (GetId -> (Int32 value))")
			SEXTEXT("    (SetId (Int32 value) ->)")
			SEXTEXT(")")

			SEXTEXT("(class Player (implements EntryPoint.IPlayer)")				
			SEXTEXT("    (Int32 id)")
			SEXTEXT(")")

			SEXTEXT("(method Player.GetId -> (Int32 value):")				
			SEXTEXT("    (value = this.id)")
			SEXTEXT(")")

			SEXTEXT("(method Player.SetId (Int32 value) -> :")				
			SEXTEXT("    (this.id = value)")
			SEXTEXT(")")

			SEXTEXT("(method Player.Construct (Int32 value): ")				
			SEXTEXT("    (this.id = value)")
			SEXTEXT(")")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestNullObject"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):)")	
			SEXTEXT("(function Main -> (Int32 result):)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDuplicateFunctionError"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			validate(false);
		}
		catch (ParseException& e)
		{
			validate(AreEqual(e.Message(), SEXTEXT("Function Main already defined in TestDuplicateFunctionError")));
		}
	}

	void TestDuplicateStructureError(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):)")	
			SEXTEXT("(struct Vector3 (Float32 x y z))")	
			SEXTEXT("(struct Vector3 (Float32 x y z))")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDuplicateStructureError"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			validate(false);
		}
		catch (ParseException& e)
		{
			validate(AreEqual(e.Message(), SEXTEXT("Duplicate structure definition for Vector3")));
		}
	}

	void TestBigNamespaceError(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(namespace T134567890123456789012345678901234567890123456789012345678901234TX3456789012345678901234567890123456789012345678901234567890123)") // 127 chars is ok
			SEXTEXT("(namespace T234567890123456789012345678901234567890123456789012345678901234T234567890123456789012345678901234567890123456789012345678901234)") // 128 chars is not
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBigNamespaceError"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(), SEXTEXT("T234"), 4));
		}
	}

	void TestBigFunctionError(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function E2345678901234567890123456789012TX34567890123456789012345678901 -> (Int32 x):)") // 63 chars is ok
			SEXTEXT("(function T2345678901234567890123456789012T2345678901234567890123456789012 -> (Int32 x):)") // 64 chars is not
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBigFunctionError"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(), SEXTEXT("T234"), 4));
		}
	}

	void TestBigStructureError(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(struct TX34567890123456789012345678901 (Int32 x))") // 31 chars is ok
			SEXTEXT("(struct T2345678901234567890123456789012 (Int32 x))") // 32 chars is not
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBigStructureError"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(), SEXTEXT("T234"), 4));
		}
	}

	void TestBigInterfaceError(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(interface EntryPoint.TX34567890123456789012345678901 (GetId -> (Int32 id)))") // 31 chars is ok
			SEXTEXT("(interface EntryPoint.T2345678901234567890123456789012 (GetId -> (Int32 id)))") // 32 chars is not
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBigInterfaceError"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(), SEXTEXT("EntryPoint.T234"), 9));
		}
	}

	void TestBigArchetypeError(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(archetype EntryPoint.TX34567890123456789012345678901 (Int32 x) -> (Int32 y))") // 31 chars is ok
			SEXTEXT("(archetype EntryPoint.T2345678901234567890123456789012 (Int32 x) -> (Int32 y))") // 32 chars is not
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestBigArchetypeError"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(), SEXTEXT("EntryPoint.T234"), 9));
		}
	}

	void TestDuplicateInterfaceError(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(interface EntryPoint.A (GetId -> (Int32 id)))") 
			SEXTEXT("(interface EntryPoint.A (GetId -> (Int32 id)))")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDuplicateInterfaceError"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(), SEXTEXT("EntryPoint.A")));
		}
	}

	void TestDuplicateArchetypeError(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(archetype EntryPoint.A -> (Int32 id))") 
			SEXTEXT("(archetype EntryPoint.A -> (Int32 id))")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDuplicateArchetypeError"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		
			validate(false);
		}
		catch (ParseException& e)
		{			
			validate(AreEqual(e.Specimen(), SEXTEXT("EntryPoint.A")));
		}
	}

	void TestTryWithoutThrow(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")		
			SEXTEXT("		(")
			SEXTEXT("			try ( result = 15)")   
			SEXTEXT("     catch e ( result = (result + 7) )")
			SEXTEXT("   )")
			SEXTEXT(")")			
			SEXTEXT("(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))")
			SEXTEXT("(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))")
			SEXTEXT("(class TestException (implements Sys.Type.IException))")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestTryWithoutThrow"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")		
			SEXTEXT("		(")
			SEXTEXT("			try ( result = 15)")   
			SEXTEXT("     catch e ( result = (result + 7) )")
			SEXTEXT("     finally ( result = (result - 5) )")
			SEXTEXT("   )")
			SEXTEXT(")")			
			SEXTEXT("(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))")
			SEXTEXT("(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))")
			SEXTEXT("(class TestException (implements Sys.Type.IException))")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestTryFinallyWithoutThrow"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")		
			SEXTEXT("   (result = 12)")
			SEXTEXT("		(try")
			SEXTEXT("			(")
			SEXTEXT("				(TestException ex)(throw ex) (result = 11)")
			SEXTEXT("			)")   
			SEXTEXT("		catch e")
			SEXTEXT("			(")
			SEXTEXT("				(result = (result + 15))")
			SEXTEXT("			)")
			SEXTEXT("		)")
			SEXTEXT(")")			
			SEXTEXT("(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))")
			SEXTEXT("(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))")
			SEXTEXT("(class TestException (implements Sys.Type.IException))")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestCatch"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 27);
	}

	void TestDeepCatch(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")		
			SEXTEXT("   (result = 12)")
			SEXTEXT("		(")			
			SEXTEXT("			try ( (Double 11 -> result) )")   
			SEXTEXT("     catch e ( result = (result + 15) )")
			SEXTEXT("   )")
			SEXTEXT(")")			
			SEXTEXT("(function Double (Int32 x) -> (Int32 y):")		
			SEXTEXT("   (Int32 z)")
			SEXTEXT("   (TestException ex)(throw ex)")			
			SEXTEXT("   (y = (x * x))")
			SEXTEXT(")")			
			SEXTEXT("(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))")
			SEXTEXT("(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))")
			SEXTEXT("(class TestException (implements Sys.Type.IException))")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDeepCatch"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")		
			SEXTEXT("   (Robot robby)")
			SEXTEXT("   (result = 7)")
			SEXTEXT(")")			
			SEXTEXT("(alias Main EntryPoint.Main)")

			SEXTEXT("(class Robot)")
			SEXTEXT("(method Robot.Destruct -> : (Sys.InvokeTest))")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDestructor"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		g_destructorTestInt = 0;

		struct ANON {	static void InvokeTest(NativeCallEnvironment& e)
		{				
				g_destructorTestInt = 1;
		}};

		const INamespace& nsSys = ss.AddNativeNamespace(SEXTEXT("Sys"));

		try
		{
			ss.AddNativeCall(nsSys, ANON::InvokeTest, NULL, SEXTEXT("InvokeTest ->"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("(TestException ex)")
			SEXTEXT("(try")
			SEXTEXT("   ((Robot robby)(throw ex))")
			SEXTEXT("catch e ((result = 7)))")
			SEXTEXT(")")			
			SEXTEXT("(alias Main EntryPoint.Main)")

			SEXTEXT("(class Robot)")
			SEXTEXT("(method Robot.Destruct -> : (Sys.InvokeTest))")
			SEXTEXT("(class TestException (implements Sys.Type.IException))")
			SEXTEXT("(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))")			
			SEXTEXT("(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestExceptionDestruct"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		g_destructorTestInt = 0;

		struct ANON {	static void InvokeTest(NativeCallEnvironment& e)
		{				
			g_destructorTestInt = 1;
		}};

		const INamespace& nsSys = ss.AddNativeNamespace(SEXTEXT("Sys"));

		try
		{
			ss.AddNativeCall(nsSys, ANON::InvokeTest, NULL, SEXTEXT("InvokeTest ->"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("(TestException ex)")
			SEXTEXT("(try")
			SEXTEXT("   ((Robot robby)(throw ex))")
			SEXTEXT("catch e ((result = 7)))")
			SEXTEXT(")")			
			SEXTEXT("(alias Main EntryPoint.Main)")

			SEXTEXT("(class Robot)")
			SEXTEXT("(method Robot.Destruct -> : (TestException ex)(throw ex))")
			SEXTEXT("(class TestException (implements Sys.Type.IException))")
			SEXTEXT("(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))")			
			SEXTEXT("(method TestException.Message -> (Sys.Type.IString s): (s = \"To be expected\"))")			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDestructorThrows"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		s_logger.Write(SEXTEXT("Expecting a complaint about an exception thrown by a destructor:"));
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(EXECUTERESULT_THROWN == result);
	}

	void TestThrowFromCatch(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("(TestException ex)")
			SEXTEXT("(try")
			SEXTEXT("		(try")
			SEXTEXT("			((result = 1)(Robot robby)(throw ex))")
			SEXTEXT("		 catch e1")
			SEXTEXT("			((result = (result + 3))(TestException ex1)(throw ex1))")
			SEXTEXT("		)")		
			SEXTEXT(" catch e2")		
			SEXTEXT("			((result = (result + 7)))")
			SEXTEXT("		)")
			SEXTEXT(")")			
			SEXTEXT("(alias Main EntryPoint.Main)")

			SEXTEXT("(class Robot)")
			SEXTEXT("(method Robot.Destruct -> : )")
			SEXTEXT("(class TestException (implements Sys.Type.IException))")
			SEXTEXT("(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = 0))")			
			SEXTEXT("(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestThrowFromCatch"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 11);
	}

	void TestSizeOf(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	  (Robot robby)")
			SEXTEXT("	  (result = (sizeof robby))")
			SEXTEXT(")")			
			SEXTEXT("(alias Main EntryPoint.Main)")
			SEXTEXT("(class Robot (Int32 electroBrainNumber))")	
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestSizeOf"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();

		int robbysize = sizeof(int32) + sizeof(size_t) + sizeof(int32); // The member electroBrainNumber + the vTable + the allocSize integer
		validate(x == robbysize); 
	}

	void TestCatchArg(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")		
			SEXTEXT("   (result = 12)")
			SEXTEXT("		(")			
			SEXTEXT("			try ( (TestException ex)(SetErrorCode ex 1943)(throw ex) (result = 11) )")   
			SEXTEXT("     catch e ( (e.ErrorCode -> result) )")
			SEXTEXT("   )")
			SEXTEXT(")")			
			SEXTEXT("(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = this.errorCode))")
			SEXTEXT("(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))")
			SEXTEXT("(function SetErrorCode (TestException ex) (Int32 errorCode) -> : (ex.errorCode = errorCode))")
			SEXTEXT("(class TestException (implements Sys.Type.IException) (Int32 errorCode))")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestCatchArg"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")		
			SEXTEXT("	(TestException ex)")
			SEXTEXT(" (SetErrorCode ex 1943)")   
			SEXTEXT(" (result = (ex.ErrorCode))")
			SEXTEXT(")")			
			SEXTEXT("(method TestException.ErrorCode -> (Int32 errorCode): (errorCode = this.errorCode))")
			SEXTEXT("(method TestException.Message -> (Sys.Type.IString s): (s = \"This is to be expected\"))")
			SEXTEXT("(function SetErrorCode (TestException ex) (Int32 errorCode) -> : (ex.errorCode = errorCode))")
			SEXTEXT("(class TestException (implements Sys.Type.IException) (Int32 errorCode))")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestCatchInstanceArg"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")		
			SEXTEXT("	(Robot robby)")
			SEXTEXT(" (robby.brainState = 7)")   
			SEXTEXT(" (RobotThink robby)")
			SEXTEXT(" (result = robby.brainState)")
			SEXTEXT(")")
			SEXTEXT("(function RobotThink (Robot x) -> :")		
			SEXTEXT(" (IncBrainState x)")   
			SEXTEXT(")")
			SEXTEXT("(function IncBrainState (Robot x) -> :")		
			SEXTEXT(" (x.brainState = (x.brainState + 1))")   
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			SEXTEXT("(class Robot (Int32 brainState))")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestInstancePropagation"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")		
			SEXTEXT("	(Robot robby)")
			SEXTEXT(" (robby.brainState = 7)")   
			SEXTEXT(" (RobotThink robby)")
			SEXTEXT(" (result = robby.brainState)")
			SEXTEXT(")")
			SEXTEXT("(function RobotThink (Robot x) -> :")		
			SEXTEXT(" (IncBrainState x)")   
			SEXTEXT(")")
			SEXTEXT("(function IncBrainState (Robot x) -> :")		
			SEXTEXT(" (x.brainState = (Inc x.brainState))")   
			SEXTEXT(" (Inc x.brainState -> x.brainState)")   
			SEXTEXT(")")
			SEXTEXT("(function Inc (Int32 x) -> (Int32 y):")		
			SEXTEXT(" (y = (x + 1))")   
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			SEXTEXT("(class Robot (Int32 brainState))")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestInstanceMemberPropagation"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 9);
	}

	void TestInterfacePropagation(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")		
			SEXTEXT("	(EntryPoint.IRobot robby(EntryPoint.MakeRobot 7))")
			SEXTEXT(" (RobotThink robby)")
			SEXTEXT(" (robby.GetBrainState -> result)")
			SEXTEXT(")")
			SEXTEXT("(function RobotThink (EntryPoint.IRobot x) -> :")		
			SEXTEXT(" (IncRobotBrainState x)")   
			SEXTEXT(")")
			SEXTEXT("(function IncRobotBrainState (EntryPoint.IRobot x) -> :")		
			SEXTEXT(" (x.SetBrainState ((x.GetBrainState) + 1))")   
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			SEXTEXT("(interface EntryPoint.IRobot")
			SEXTEXT("  (GetBrainState -> (Int32 value))")
			SEXTEXT("  (SetBrainState (Int32 value) ->)")
			SEXTEXT(")")
			SEXTEXT("(class Robot (implements EntryPoint.IRobot) (Int32 brainState))")
			SEXTEXT("(method Robot.GetBrainState -> (Int32 value):")				
			SEXTEXT("    (value = this.brainState)")
			SEXTEXT(")")
			SEXTEXT("(method Robot.SetBrainState (Int32 value)-> :")				
			SEXTEXT("    (this.brainState = value)")
			SEXTEXT(")")
			SEXTEXT("(method Robot.Construct (Int32 value): ")				
			SEXTEXT("    (this.brainState = value)")
			SEXTEXT(")")
			SEXTEXT("(factory EntryPoint.MakeRobot EntryPoint.IRobot (Int32 brainState): (construct Robot brainState))")	
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestInterfacePropagation"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)(namespace Test)")
			SEXTEXT("")
			SEXTEXT("(function Main -> (Int32 result):")		
			SEXTEXT(" (Test.IRobot robby(Test.MakeRobot 7))")	
			SEXTEXT(" (robby.GetBrainState -> result)")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			SEXTEXT("")
			SEXTEXT("(interface Test.IRobot")
			SEXTEXT("  (GetBrainState -> (Int32 value))")
			SEXTEXT(")")
			SEXTEXT("(interface Test.IExistence")
			SEXTEXT("  (Exists -> (Bool value))")
			SEXTEXT(")")
			SEXTEXT("(class Robot (implements Test.IExistence) (implements Test.IRobot) (Int32 brainState))")
			SEXTEXT("(method Robot.GetBrainState -> (Int32 value):")				
			SEXTEXT("    (value = this.brainState)")
			SEXTEXT(")")
			SEXTEXT("(method Robot.Exists -> (Bool value):")				
			SEXTEXT("    (value = true)")
			SEXTEXT(")")
			SEXTEXT("(method Robot.Construct (Int32 value): ")				
			SEXTEXT("    (this.brainState = value)")
			SEXTEXT(")")
			SEXTEXT("(factory Test.MakeRobot Test.IRobot (Int32 brainState) : (construct Robot brainState))")	
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMultipleInterfaces"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")
			SEXTEXT("")
			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT(" (IString s)")	
			SEXTEXT(" (s.Length -> result)")
			SEXTEXT(")")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMultipleInterfaces"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")
			SEXTEXT("")
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT(" (IString s = \"hello world\")")	
			SEXTEXT(" (s.Length -> result)")
			SEXTEXT(")")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStringConstant"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")
			SEXTEXT("")
			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT(" (Sys.Print \"Hello World\" -> result)")
			SEXTEXT(")")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestPrint"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 11);
	}

	void TestPrintViaInstance(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("  (alias Main EntryPoint.Main)")
			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("  (IString message = \"Bilbo Baggins\")")
			SEXTEXT("  (Sys.Print message -> result)")
			SEXTEXT(")")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestPrintViaInstance"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")
			SEXTEXT("")
			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT(" (Sys.Type.IString s(Sys.Type.Memo \"Hello World\"))")
			SEXTEXT(" (Sys.Print s -> result)")
			SEXTEXT(")")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMemoString"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));
		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 11);
	}

	void TestMemoString2(IPublicScriptSystem& ss)
	{
		// Just like TestMemoString, but with a (using ...) directive
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")
			SEXTEXT("")
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("")
			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT(" (IString s(Memo \"Hello World\"))")
			SEXTEXT(" (Sys.Print s -> result)")
			SEXTEXT(")")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMemoString2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 11);
	}

	void TestDynamicCast(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")
			SEXTEXT("")
			SEXTEXT("(namespace Stuff) (using Stuff)")
			SEXTEXT("")
			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (Robot robby (9000))")
			SEXTEXT("  (Foobar robby -> result)")
			SEXTEXT(")")
			SEXTEXT("(interface Stuff.IRobot")
			SEXTEXT("  (Id -> (Int32 id))")
			SEXTEXT(")")
			SEXTEXT("(interface Stuff.IExistence")
			SEXTEXT("  (ExistenceNumber -> (Int32 value))")
			SEXTEXT(")")
			SEXTEXT("(class Robot")
			SEXTEXT("  (implements Stuff.IExistence)")
			SEXTEXT("  (implements Stuff.IRobot)")
			SEXTEXT("  (Int32 id)")
			SEXTEXT(")")
			SEXTEXT("(function Foobar (Stuff.IExistence entity) -> (Int32 result):")
			SEXTEXT("	 (cast entity -> IRobot robot)")
			SEXTEXT("  (result = ((robot.Id) + (entity.ExistenceNumber)))")
			SEXTEXT(")")
			SEXTEXT("(method Robot.Construct (Int32 id): (this.id = id))")
			SEXTEXT("(method Robot.Id -> (Int32 id): (id = this.id))")
			SEXTEXT("(method Robot.ExistenceNumber -> (Int32 value): (value = 1234))")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDynamicCast"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 10234);
	}

	void TestInlinedFactory(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")
			SEXTEXT("")
			SEXTEXT("(namespace Stuff) (using Stuff)")
			SEXTEXT("")
			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (IRobot robby (Dreambot 9000))")
			SEXTEXT("  (robby.Id -> result)")
			SEXTEXT(")")
			SEXTEXT("(interface Stuff.IRobot")
			SEXTEXT("  (Id -> (Int32 id))")
			SEXTEXT(")")
			SEXTEXT("(class Robot")
			SEXTEXT("  (implements Stuff.IRobot)")
			SEXTEXT("  (Int32 id)")
			SEXTEXT(")")
			SEXTEXT("(method Robot.Construct (Int32 id): (this.id = id))")
			SEXTEXT("(method Robot.Id -> (Int32 id): (id = this.id))")
			SEXTEXT("(factory Stuff.Dreambot Stuff.IRobot (Int32 id): (construct Robot id))")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestInlinedFactory"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 9000);
	}

	void TestRecti1(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("(using Sys.Maths)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (Recti rect = (1 2) (3 4))")
			SEXTEXT("  (result = rect.topLeft.x)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestRecti1"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1);
	}

	void TestRecti2(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("(using Sys.Maths)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (Recti rect = (1 2) (3 4))")
			SEXTEXT("  (result = rect.topLeft.y)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestRecti2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 2);
	}

	void TestRecti3(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("(using Sys.Maths)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (Recti rect = (1 2) (3 4))")
			SEXTEXT("  (result = rect.bottomRight.x)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestRecti3"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 3);
	}

	void TestRecti4(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("(using Sys.Maths)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (Recti rect = (1 2) (3 4))")
			SEXTEXT("  (result = rect.bottomRight.y)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestRecti4"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestWindow(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.OS.Win32)")
			SEXTEXT("(using Sys.OS.Win32.Windows)")
			SEXTEXT("(using Sys.OS.Thread)")
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("(using Sys.Maths)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (Recti rect = (40 520) (680 40))")
			SEXTEXT("  (IWin32Window window (CreateWindow \"Sexy Test Window\" rect))")
			SEXTEXT("  (window.Show)")

			SEXTEXT("  (IThread mainThread (GetCurrentThread))")
			SEXTEXT("  (Int32 limit = 1)")
			SEXTEXT("  (while ((not mainThread.IsQuitting) and (limit > 0))")
			SEXTEXT("    (mainThread.Wait 1000)")
			SEXTEXT("    (limit = (limit - 1))")
			SEXTEXT("  )")

			SEXTEXT("  (result = 320)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestWindow"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 320);
	}

	void TestReflectionGetCurrentExpression(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("(using Sys.Reflection)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (IExpression s = GetCurrentExpression)")
			SEXTEXT("  (s.ChildCount -> result)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestReflectionGetCurrentExpression"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestReflectionGetParent(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("(using Sys.Reflection)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (IExpression s = GetCurrentExpression)")
			SEXTEXT("  (IExpression parent = s.Parent)")
			SEXTEXT("  (parent.ChildCount -> result)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestReflectionGetParent"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 8);
	}

	void TestReflectionGetChild_BadIndex(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("(using Sys.Reflection)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias ProtectedMain EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (IExpression s = GetCurrentExpression)")
			SEXTEXT("  (IExpression child = (s.Child -1))")
			SEXTEXT("  (child.ChildCount -> result)")
			SEXTEXT(")")

			SEXTEXT("(function ProtectedMain -> (Int32 result):")
			SEXTEXT("	(try")
			SEXTEXT("		(Main -> result)")
			SEXTEXT("	catch ex")
			SEXTEXT("		(")
			SEXTEXT("			(Int32 len)")
			SEXTEXT("			(Sys.Print ex.Message -> len)")
			SEXTEXT("			(result = 1001)")
			SEXTEXT("		)")
			SEXTEXT("	)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestReflectionGetChild_BadIndex"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 1001);
	}

	void TestReflectionGetChild(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("(using Sys.Reflection)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (IExpression s = GetCurrentExpression)") // s = (IExpression s = GetCurrentExpression)
			SEXTEXT("  (IExpression child = (s.Child 0))")  // child 0 is 'IExpression' which is atomic and therefore has no children
			SEXTEXT("  (child.ChildCount -> result)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestReflectionGetChild"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestReflectionGetAtomic(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)")
			SEXTEXT("(using Sys.Reflection)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (IExpression s = GetCurrentExpression)") // s = (IExpression s = GetCurrentExpression)
			SEXTEXT("  (IExpression child = (s.Child 0))")  // child 0 is 'IExpression' which is atomic and therefore has no children
			SEXTEXT("  (IString name = child.Text)")
			SEXTEXT("  (Sys.Print name -> result)") // returns the strlen of 'IExpression'
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestReflectionGetAtomic"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 11);
	}

	void TestNullMember(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (result = (sizeof Lapdancer))")
			SEXTEXT(")")

			SEXTEXT("(interface EntryPoint.IPole")
			SEXTEXT("  (Index -> (Int32 value))")
			SEXTEXT(")")

			SEXTEXT("(class Pole")
			SEXTEXT("  (implements EntryPoint.IPole)")
			SEXTEXT("  (Int32 index)")
			SEXTEXT(")")

			SEXTEXT("(method Pole.Index -> (Int32 index): (index = this.index))")

			SEXTEXT("(struct Lapdancer")
			SEXTEXT("  (EntryPoint.IPole pole)") // This creates two members, the concrete IPole, initially a null-object and a reference to the interface
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestNullMember"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 3 * sizeof(size_t) + 2 * sizeof(int32)); // Lapdancer's pole is null object, ergo has capacity to be reallocated as a Pole, which has 4 fields, two pointers and two int32s. We also need a pointer to the correct interface alongside the member
	}

	void TestNullMemberInit(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("  (Lapdancer lapdancer)")
			SEXTEXT("  (lapdancer.pole.Index -> result)")
			SEXTEXT(")")

			SEXTEXT("(interface EntryPoint.IPole")
			SEXTEXT("  (Index -> (Int32 value))")
			SEXTEXT(")")

			SEXTEXT("(class Pole")
			SEXTEXT("  (implements EntryPoint.IPole)")
			SEXTEXT("  (Int32 index)")
			SEXTEXT(")")

			SEXTEXT("(method Pole.Index -> (Int32 index): (index = this.index))")

			SEXTEXT("(struct Lapdancer")
			SEXTEXT("  (EntryPoint.IPole pole)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestNullMemberInit"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
		validate(ns != NULL);
		validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

		VM::IVirtualMachine& vm = ss.PublicProgramObject().VirtualMachine();

		vm.Push(0); // Allocate stack space for the int32 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 0);
	}

	void TestSysThrow(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("  (alias Main EntryPoint.Main)")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("(try")
			SEXTEXT("		(")
			SEXTEXT("			(Sys.Throw -1 \"Wobbly\")")
			SEXTEXT("		)")		
			SEXTEXT(" catch e")		
			SEXTEXT("		(")
			SEXTEXT("			(e.ErrorCode -> result)")
			SEXTEXT("           (Int32 nChars)")
			SEXTEXT("			(Sys.Print e.Message -> nChars)")
			SEXTEXT("		)")		
			SEXTEXT("  )")
			SEXTEXT(")")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestSysThrow"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == -1);
	}

	void TestSysThrow2(IPublicScriptSystem& ss) // like sys.throw, but adds a stack spanner in the works
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("  (alias Main EntryPoint.Main)")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("(try")
			SEXTEXT("		(")
			SEXTEXT("			(Int32 stackSpanner)")
			SEXTEXT("			(Sys.Throw -1 \"Wobbly\")")
			SEXTEXT("		)")		
			SEXTEXT(" catch e")		
			SEXTEXT("		(")
			SEXTEXT("			(e.ErrorCode -> result)")
			SEXTEXT("           (Int32 nChars)")
			SEXTEXT("			(Sys.Print e.Message -> nChars)")
			SEXTEXT("		)")		
			SEXTEXT("  )")
			SEXTEXT(")")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestSysThrow"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("  (alias Main EntryPoint.Main)")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("(Tester tester ())")
			SEXTEXT("     (tester.Think)")
			SEXTEXT("     (tester.Id -> result)")
			SEXTEXT(")")
			SEXTEXT("(interface EntryPoint.ITest")
			SEXTEXT("       (Id -> (Int32 id))")
			SEXTEXT("       (Think ->)")
			SEXTEXT("       (IncId ->)")
			SEXTEXT(")")
			SEXTEXT("(class Tester")
			SEXTEXT("       (implements EntryPoint.ITest)")
			SEXTEXT("       (Int32 id)")
			SEXTEXT(")")
			SEXTEXT("(method Tester.Id -> (Int32 id): (id = this.id))")
			SEXTEXT("(method Tester.IncId -> : (this.id = (this.id + 1)))")
			SEXTEXT("(method Tester.Think -> : (this.IncId))")
			SEXTEXT("(method Tester.Construct : (this.id = 6))")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestVirtualFromVirtual"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("  (alias Main EntryPoint.Main)")
			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("	(EntryPoint.ITest tester = GetNullRef)")
			SEXTEXT("   (tester.Id -> result)")
			SEXTEXT(")")
			SEXTEXT("(interface EntryPoint.ITest")
			SEXTEXT("       (Id -> (Int32 id))")
			SEXTEXT(")")
			SEXTEXT("(class Tester")
			SEXTEXT("       (implements EntryPoint.ITest)")
			SEXTEXT("       (Int32 id)")
			SEXTEXT(")")
			SEXTEXT("(method Tester.Id -> (Int32 id): (id = this.id))")
			SEXTEXT("(method Tester.Construct : (this.id = 6))")
			SEXTEXT("(function GetNullRef -> (EntryPoint.ITest testRef): )")			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestNullRefInit"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("  (alias Main EntryPoint.Main)")
			SEXTEXT("(using Sys.Reflection)")

			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("	(IScriptSystem ss = Sys.Reflection.GetScriptSystem)")
			SEXTEXT("   (ss.ModuleCount -> result)")
			SEXTEXT(")")			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestModuleCount"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("  (alias Main EntryPoint.Main)")
			SEXTEXT("(using Sys.Reflection)")
			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("	(IScriptSystem ss = Sys.Reflection.GetScriptSystem)")
			SEXTEXT("	(Int32 moduleCount)")
			SEXTEXT("   (ss.ModuleCount -> moduleCount)")
			SEXTEXT("   (#for (Int32 i = 0) (i < moduleCount) (i = (i + 1))")
			SEXTEXT("		(IModule module = (ss.Module i))")
			SEXTEXT("		(Int32 len)")
			SEXTEXT("		(Sys.Print module.Name -> len)")
			SEXTEXT("	)")
			SEXTEXT(")")			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestPrintModules"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestPrintStructs(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("  (alias Main EntryPoint.Main)")
			SEXTEXT("(using Sys.Reflection)")
			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("	(IScriptSystem ss = Sys.Reflection.GetScriptSystem)")
			SEXTEXT("	(Int32 moduleCount = ss.ModuleCount)")

			SEXTEXT("   (#for (Int32 i = 0)  (i < moduleCount)  (#inc i)")
			SEXTEXT("			(IModule module = (ss.Module i))")

			SEXTEXT("			(#for (Int32 j = 0) (j < module.StructCount) (#inc j)")
			SEXTEXT("				(IStructure s = (module.Structure j))")
			SEXTEXT("				(Int32 len)")
			SEXTEXT("				(Sys.Print s.Name -> len)")
			SEXTEXT("			)")
			SEXTEXT("	)")
			SEXTEXT(")")			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestPrintStructs"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestMacro(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
  
		// Define a macro called 'square' instanced as #square that takes an input IExpression of 'in' and builds an expression 'out', which substitutes for the instance #square

		// square usage: (#square f) evaluates to ((f) * (f))
		SEXTEXT("(macro Sys.Maths.square in out")
		SEXTEXT("	(Sys.ValidateSubscriptRange in.ChildCount 2 3 \"macro square supports one argument only\")")
		SEXTEXT("	(IExpressionBuilder lhs = out.AddCompound)")
		SEXTEXT("	(lhs.Copy (in.Child 1))")
		SEXTEXT("	(out.AddAtomic \"*\")")
		SEXTEXT("	(IExpressionBuilder rhs = out.AddCompound)")
		SEXTEXT("	(rhs.Copy (in.Child 1))")
		SEXTEXT(")")

		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Int32 i = 7)")
		SEXTEXT("	(result = (#square i))")
		SEXTEXT(")")		
					;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMacro"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 49);
	}

	void TestExpressionArg(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(IExpression s = '($1 = ($1 * 2)) )")
		SEXTEXT("	(s.ChildCount -> result)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestExpressionArg"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 3);
	}

	void TestSubstitution(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(result = 7)")
		SEXTEXT("	(#EntryPoint.double result)")
		SEXTEXT(")")

		SEXTEXT("(macro EntryPoint.double in out")
		SEXTEXT("	(IExpression s = '($1 = ($1 * 2)) )")
		SEXTEXT("	(out.Substitute in s)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestSubstitution"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 14);
	}

	void TestStringBuilder(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
		SEXTEXT("(using Sys.Type.Formatters)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sys.Type.IStringBuilder s (Sys.Type.StringBuilder 256))")
		SEXTEXT("	(s.AppendIString \"Hello World!\")")
		SEXTEXT("	(Sys.Print s -> result)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStringBuilder"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 12);
	}

	void TestStringBuilderBig(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
		SEXTEXT("(using Sys.Type.Formatters)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sys.Type.IStringBuilder s (Sys.Type.StringBuilder 256))")
		SEXTEXT("	(#Sys.Type.build  s \"Hello World! \" NewLine)")
		SEXTEXT("	(#Sys.Type.build  s \"Decimal: \" Decimal 1234 \". Hex: 0x\" Hex 1234 NewLine)")
		SEXTEXT("	(#Sys.Type.build  s \"E form: \" SpecE 3.e-2 \". F form: \" SpecF 3.e-2 \". G form: \" SpecG 3.e-2 NewLine)")
		SEXTEXT("	(cast s -> IString str)")
		SEXTEXT("	(Sys.Print str -> result)")
		SEXTEXT("	(s.Clear) (s.SetFormat 4 4 false false)")
		SEXTEXT("	(#Sys.Type.build  s \"Hello World! \" NewLine)")
		SEXTEXT("	(#Sys.Type.build  s \"Decimal: \" Decimal 1234 \". Hex: 0x\" Hex 1234 NewLine)")
		SEXTEXT("	(#Sys.Type.build  s \"E form: \" SpecE 3.e-2 \". F form: \" SpecF 3.e-2 \". G form: \" SpecG 3.e-2 NewLine)")

		SEXTEXT("	(Sys.Print s -> result)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStringBuilderBig"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x > 11);
	}

	void TestRefTypesInsideClosure(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
		SEXTEXT("(using Sys.Type.Formatters)")

		SEXTEXT("(archetype EntryPoint.VoidToVoid ->)")

		SEXTEXT("(function GoPrint (Sys.Type.IStringBuilder s) -> :")
		SEXTEXT("	(VoidToVoid g = ")
		SEXTEXT("			(closure -> :")
		SEXTEXT("				(Sys.Print s)") // tests that the compiler passes an upvalue reference correctly
		SEXTEXT("			)")
		SEXTEXT("	)")
		SEXTEXT("(g)")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sys.Type.IStringBuilder s (Sys.Type.StringBuilder 256))")
		SEXTEXT("	(#Sys.Type.build  s \"Hello World! \" NewLine)")
		SEXTEXT("	(GoPrint s)(result = 12)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStringBuilderInsideClosure"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 12);
	}

	void TestDerivedInterfaces(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT("	(alias Main EntryPoint.Main)")

		SEXTEXT("(using EntryPoint)")

		SEXTEXT("(interface EntryPoint.IDog")
		SEXTEXT("	(Bark -> (Int32 value))")
		SEXTEXT(")")

		SEXTEXT("(interface EntryPoint.IRobotDog")
		SEXTEXT("	(extends EntryPoint.IDog)")
		SEXTEXT("	(Powerup -> (Int32 value))")
		SEXTEXT(")")
		SEXTEXT("")
		SEXTEXT("(class Doggy (implements IRobotDog))")

		SEXTEXT("(method Doggy.Powerup -> (Int32 value): (value = 5))")
		SEXTEXT("(method Doggy.Bark -> (Int32 value): (value = 12))")
   
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Doggy dog)")
		SEXTEXT("	(result = ((dog.Bark) + (dog.Powerup)))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDerivedInterfaces"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT("	(alias Main EntryPoint.Main)")

		SEXTEXT("(using EntryPoint)")

		SEXTEXT("(interface EntryPoint.IDog")
		SEXTEXT("	(Bark -> (Int32 value))")
		SEXTEXT(")")

		SEXTEXT("(interface EntryPoint.IRobotDog")
		SEXTEXT("	(extends EntryPoint.IDog)")
		SEXTEXT("	(Powerup -> (Int32 value))")
		SEXTEXT(")")
		SEXTEXT("")
		SEXTEXT("(class Doggy (implements IRobotDog))")

		SEXTEXT("(method Doggy.Powerup -> (Int32 value): (value = 5))")
		SEXTEXT("(method Doggy.Bark -> (Int32 value): (value = 12))")
   
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Doggy dog)")
		SEXTEXT("	(cast dog -> EntryPoint.IDog k9)")
		SEXTEXT("	(result = ((k9.Bark) + (dog.Powerup)))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDerivedInterfaces2"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
		SEXTEXT("(using Sys.Type.Formatters)")
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(IString opera = \"The Magic Flute\")")
		SEXTEXT("	(IString instrument = \"Flute\")")
		SEXTEXT("	(result = (Strings.FindLeftNoCase opera 4 instrument))") // the substring occurs at position 10 in the containing string
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestSearchSubstring"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
		SEXTEXT("(using Sys.Type.Formatters)")
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(IString musical = \"The tapdancing tapper\")")
		SEXTEXT("	(IString instrument = \"tap\")")
		SEXTEXT("	(result = (Strings.FindRightWithCase musical ((musical.Length) - 1) instrument))") // the substring occurs at position 10 in the containing string
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestSearchSubstring"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 15);
	}

	void TestAppendSubstring(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
		SEXTEXT("(using Sys.Type.Formatters)")
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sys.Type.IStringBuilder s (Sys.Type.StringBuilder 256))")
		SEXTEXT("	(s.AppendSubstring \"The Magic Flute\" 4 5)")
		SEXTEXT("	(s.AppendSubstring \"The Magic Flute\" 3 -1)")
		SEXTEXT("	(s.AppendSubstring \"The Magic Flute\" 11 10)")
		SEXTEXT("	(Sys.Print s -> result)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAppendSubstring"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
		SEXTEXT("(using Sys.Type.Formatters)")
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sys.Type.IStringBuilder s (Sys.Type.StringBuilder 256))")
		SEXTEXT("	(s \"The Magic Flute\")")
		SEXTEXT("	(s.SetLength 8)")
		SEXTEXT("	(Sys.Print s -> result)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStringbuilderTruncate"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
		SEXTEXT("(using Sys.Type.Formatters)")
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sys.Type.IStringBuilder s (Sys.Type.StringBuilder 256))")
		SEXTEXT("	(s \"The Magic Flute\")")
		SEXTEXT("	(s.ToUpper 0 9)")
		SEXTEXT("	(s.ToLower 10 15)")
		SEXTEXT("	(Sys.Print s -> result)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestSetCase"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Maths)")
		SEXTEXT("(using Sys.Reflection)")
		SEXTEXT("(using Sys.Type.Formatters)")
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Int32 len = 0)")
		SEXTEXT("	(IStringBuilder sb (StringBuilder 256))")
		SEXTEXT("	(Strings.ForEachSubstring f = ")
		SEXTEXT("		(closure (IStringBuilder sb)(Int32 index) -> :")
		SEXTEXT("			(len = (len + (Sys.Print sb)))")
		SEXTEXT("			(sb.SetLength 0)")
		SEXTEXT("		)")
		SEXTEXT("	)")
		SEXTEXT("	(Strings.Split sb \"192.168.1.2\" \".\" f)")
		SEXTEXT("	(result = len)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStringSplit"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 8);
	}

	void TestMallocAligned(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(IBuffer b(AllocAligned 256 1))")
		SEXTEXT("	(b.Capacity -> result)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMallocAligned"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 256);
	}

	void TestArrayInt32(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a 4)")
		SEXTEXT("	(result = a.Capacity)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInt32"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 4);
	}

	void TestArrayOfArchetypes(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(archetype Sys.OneToOne (Int32 x) -> (Int32 y))")

			SEXTEXT("(function DoubleInt (Int32 x) -> (Int32 y): (y = (2 * x)))")

			SEXTEXT("(function Main -> (Int32 result):")			
			SEXTEXT("	(array Sys.OneToOne a 4)")
			SEXTEXT("   (a.Push DoubleInt)")
			SEXTEXT("   (Sys.OneToOne f = (a 0))")
			SEXTEXT("	(result = (f 17))")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestArrayArchetype"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a 4)")
		SEXTEXT("	(result = a.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInt32_2"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a 4)")
		SEXTEXT("	(a.Push 1812)")
		SEXTEXT("	(result = (a 0))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInt32_3"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a 4)")
		SEXTEXT("	(a.Push 1)")
		SEXTEXT("	(a.Push 2)")
		SEXTEXT("	(a.Push 3)")
		SEXTEXT("	(a.Push 4)")
		SEXTEXT("	(try")
		SEXTEXT("		(")
		SEXTEXT("			(a.Push 5)")
		SEXTEXT("		)")
		SEXTEXT("	catch ex")
		SEXTEXT("		(")
		SEXTEXT("			(Sys.Print ex.Message)")
		SEXTEXT("		)")
		SEXTEXT("	)")
		SEXTEXT("	(result = (a 3))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInt32_4"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 4);
	}

	void TestArrayInt32_5(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a 4)")
		SEXTEXT("	(a.Push 2492)")
		SEXTEXT("	(a.Push 1232)")
		SEXTEXT("	(a 1 1470)")
		SEXTEXT("	(a.Push 1132)")
		SEXTEXT("	(result = (a 1))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInt32_5"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a 4)")
		SEXTEXT("	(a.Push 2492)")
		SEXTEXT("	(Int32 x = a.Length)")
		SEXTEXT("	(a.Pop)")
		SEXTEXT("	(result = (x + a.Length))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInt32_6"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a 4)")
		SEXTEXT("	(a.Push 2492)")
		SEXTEXT("	(result = a.PopOut)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInt32_7"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(function PassThrough (Int32 x) -> (Int32 y):")
		SEXTEXT("	(y = x)")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a 4)")
		SEXTEXT("	(a.Push 2492)")
		SEXTEXT("	(result = (PassThrough (a 0)))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInt32_8"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(function PassThrough (Float64 x) -> (Float64 y):")
		SEXTEXT("	(y = x)")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Float64 a 4)")
		SEXTEXT("	(a.Push 2492.7)")
		SEXTEXT("	(Sys.Type.IStringBuilder sb(StringBuilder 32))")
		SEXTEXT("	(sb Formatters.SpecG)")
		SEXTEXT("	(sb (a 0))")
		SEXTEXT("	(Sys.Print sb)")
		SEXTEXT("	(result = sb.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayFloat64"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 6);
	}

	
	void TestArrayInt32_9(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a 4)")
		SEXTEXT("	(a 3 2492)")
		SEXTEXT("	(result = a.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInt32_9"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Vec3 (Float32 x y z))")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Vec3 a 4)")
		SEXTEXT("	(Vec3 v = 1 2 3)")
		SEXTEXT("	(a.Push v)")
		SEXTEXT("	(result = a.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayStruct"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Vec3i (Int32 x y z))")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Vec3i a 4)")
		SEXTEXT("	(Vec3i v = 1 2 3)")
		SEXTEXT("	(a.Push v)")
		SEXTEXT("	(Vec3i w)")
		SEXTEXT("	(w = (a 0))")
		SEXTEXT("	(result = w.z)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayStruct_2"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Vec3i (Int32 x y z))")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Vec3i a 4)")
		SEXTEXT("	(Vec3i v = 1 2 3)")
		SEXTEXT("	(a 3 v)")
		SEXTEXT("	(Vec3i w)")
		SEXTEXT("	(w = (a 3))")
		SEXTEXT("	(result = w.y)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayStruct_2"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(IString boo = \"boo boo\")")
		SEXTEXT("	(IString message = (Sys.Type.GetSysMessage(boo.Buffer)))")
		SEXTEXT("	(result = (message.Length))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestGetSysMessage"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(interface Sys.Type.IDog (Bark -> ))")
		SEXTEXT("(class Dog (implements Sys.Type.IDog))")
		SEXTEXT("(method Dog.Bark -> : )")
		SEXTEXT("(factory Sys.Type.BitchSpawn Sys.Type.IDog : (construct Dog))")

		SEXTEXT("(method Dog.Construct -> : (Sys.Print \"Dog created\"))")
		SEXTEXT("(method Dog.Destruct -> : (Sys.Print \"Dog destroyed\"))")

		SEXTEXT("(struct Zoo (Int32 zookeeperId) (IDog dog))")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Zoo zoo)")
		SEXTEXT("	(zoo.dog = (Sys.Type.BitchSpawn))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestInternalDestructorsCalled"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestInternalDestructorsCalled2(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(interface Sys.Type.IDog (Bark -> ))")
		SEXTEXT("(class Dog (implements Sys.Type.IDog))")
		SEXTEXT("(method Dog.Bark -> : )")
		SEXTEXT("(factory Sys.Type.BitchSpawn Sys.Type.IDog : (construct Dog))")

		SEXTEXT("(method Dog.Construct -> : (Sys.Print \"Dog created\"))")
		SEXTEXT("(method Dog.Destruct -> : (Sys.Print \"Dog destroyed\"))")

		SEXTEXT("(struct Zoo (Int32 zookeeperId) (IDog dog))")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(try")
		SEXTEXT("		(")
		SEXTEXT("			(Zoo zoo)")
		SEXTEXT("			(zoo.dog = (Sys.Type.BitchSpawn))")
		SEXTEXT("			(Sys.Throw -1 \"Test\")")
		SEXTEXT("		)")
		SEXTEXT("	catch ex")
		SEXTEXT("		(")
		SEXTEXT("		)")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestInternalDestructorsCalled2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestArrayRef(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(function FillArray (array Int32 a) -> : ")
		SEXTEXT("	(a 3 7)")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a 4)")
		SEXTEXT("	(FillArray a)")
		SEXTEXT("	(result = (a 3))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayRef"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 7);
	}

	void TestArrayStrongTyping(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(function FillArray (array Int32 a) -> : ")
		SEXTEXT("	(a 3 7)")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Float32 a 4)")
		SEXTEXT("	(FillArray a)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayStrongTyping"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		ParseException ex;
		if (!s_logger.TryGetNextException(ex))
			validate(false);
	}

	void TestArrayStrongTyping2(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(interface Sys.Type.IDog (Bark (array Int32 barkAngles) ->))")
		SEXTEXT("(class Rover (implements Sys.Type.IDog))")
		SEXTEXT("(method Rover.Bark (array Float32 barkAngles) -> :)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Rover rover)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayStrongTyping2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());	
			validate(false);	
		}
		catch(ParseException&)
		{
		}		
	}

	void TestArrayStrongTyping3(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(interface Sys.Type.IDog (Bark (array Int32 barkAngles) ->))")
		SEXTEXT("(class Rover (implements Sys.Type.IDog))")
		SEXTEXT("(method Rover.Bark (array Int32 barkAngles) -> :)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Rover rover)")
		SEXTEXT("	(array Float32 a 4)")
		SEXTEXT("	(rover.Bark a)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayStrongTyping3"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		ParseException ex;
		if (!s_logger.TryGetNextException(ex))
			validate(false);
	}

	void TestArrayInStruct(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct DogKennel (array Int32 dogIds))")

		SEXTEXT("(method DogKennel.Construct (Int32 capacity) ->")
		SEXTEXT("	(construct dogIds capacity): ")				
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(DogKennel kennel (4))")
		SEXTEXT("	(kennel.dogIds.Push 1812)")
		SEXTEXT("	(result = (kennel.dogIds 0))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInStruct"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 1812);
	}

	void TestArrayInStruct2(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct DogKennel (array Int32 dogIds))")
		SEXTEXT("(struct Sanctuary (DogKennel kennel))")

		SEXTEXT("(method DogKennel.Construct (Int32 capacity)")
		SEXTEXT("	-> (construct dogIds capacity) ")		
		SEXTEXT("	:")
		SEXTEXT(")")

		SEXTEXT("(method Sanctuary.Construct (Int32 kennelCapacity)")
		SEXTEXT("	-> (construct kennel kennelCapacity)")		
		SEXTEXT("	:")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sanctuary sanctuary (4) )")
		SEXTEXT("	(sanctuary.kennel.dogIds.Push 1812)")
		SEXTEXT("	(result = (sanctuary.kennel.dogIds 0))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInStruct2"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct DogKennel (Int32 dogId))")
		SEXTEXT("(struct Sanctuary (array DogKennel kennels))")

		SEXTEXT("(method DogKennel.Construct (Int32 id): ")
		SEXTEXT("	(this.dogId = id)")
		SEXTEXT(")")

		SEXTEXT("(method Sanctuary.Construct (Int32 maxKennels)")
		SEXTEXT("	-> (construct kennels maxKennels): )")		
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sanctuary sanctuary (4) )")
		SEXTEXT("	(sanctuary.kennels.Push 1812)") // This should invoke the DogKennel constructor into the memory slot of the first array element
		SEXTEXT("	(result = (sanctuary.kennels 0 dogId))") // Grab dogId from sanctuary.kennels(0)
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestConstructInArray"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 1812);
	}

	void TestArrayForeachOnce(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct DogKennel (array Int32 dogIds))")
		SEXTEXT("(struct Sanctuary (array DogKennel kennels))")

		SEXTEXT("(method DogKennel.Construct (Int32 capacity)")
		SEXTEXT("	-> (construct dogIds capacity) ")		
		SEXTEXT("	:")
		SEXTEXT(")")

		SEXTEXT("(method Sanctuary.Construct (Int32 kennelCapacity)")
		SEXTEXT("	-> (construct kennels kennelCapacity)")		
		SEXTEXT("	:")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sanctuary sanctuary (4) )") // Create a sanctuary with a capacity of 4 kennels
		SEXTEXT("	(sanctuary.kennels.Push 4)") // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		SEXTEXT("	(foreach i k # (sanctuary.kennels 0 0)") // Get a ref to the first kennel
		SEXTEXT("		(k.dogIds.Push 1812)") // Put a new dog id in the first kennel
		SEXTEXT("		(result = (k.dogIds 0))") // Return the first dog id
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayInStruct4"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct DogKennel (array Int32 dogIds))")
		SEXTEXT("(struct Sanctuary (array DogKennel kennels))")

		SEXTEXT("(method DogKennel.Construct (Int32 capacity)")
		SEXTEXT("	-> (construct dogIds capacity) ")		
		SEXTEXT("	:")
		SEXTEXT(")")

		SEXTEXT("(method Sanctuary.Construct (Int32 kennelCapacity)")
		SEXTEXT("	-> (construct kennels kennelCapacity)")		
		SEXTEXT("	:")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sanctuary sanctuary (4) )") // Create a sanctuary with a capacity of 4 kennels
		SEXTEXT("	(sanctuary.kennels.Push 4)") // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		SEXTEXT("	(sanctuary.kennels.Push 4)") // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		SEXTEXT("	(sanctuary.kennels.Push 4)") // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		SEXTEXT("	(sanctuary.kennels.Push 4)") // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		SEXTEXT("	(foreach i k # (sanctuary.kennels 0 3)") // Enumerate over all 4 kennesl
		SEXTEXT("		(k.dogIds.Push i)") // Put a new dog id in the kennel
		SEXTEXT("		(k.dogIds.Push i)") // Put a new dog id in the kennel
		SEXTEXT("		(k.dogIds.Push i)") // Put a new dog id in the kennel
		SEXTEXT("		(k.dogIds.Push i)") // Put a new dog id in the kennel
		SEXTEXT("		(foreach j d # (k.dogIds 0 3)") // Enumerate through kennel
		SEXTEXT("			(result = (result + d))") // Sum the dogid to result
		SEXTEXT("		)") // Sum the dogid to result
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayForeachWithinForEach"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 24);
	}

	void TestArrayForeachAndThrow(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Sanctuary (array Int32 kennelIds))")

		SEXTEXT("(method Sanctuary.Construct (Int32 capacity)")
		SEXTEXT("	-> (construct kennelIds capacity)")		
		SEXTEXT("	:")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sanctuary sanctuary (10) )") // Create a sanctuary with a capacity of 10 kennels
		SEXTEXT("	(sanctuary.kennelIds 9 0)") // Define and null the ten entries by setting entry 9 to 0

		SEXTEXT("	(try")
		SEXTEXT("		(")
		SEXTEXT("			(foreach i k # (sanctuary.kennelIds 0 9)") // Enumerate over all 10 kennels
		SEXTEXT("				(Sys.Throw -1 \"Test: foreach throw\")")
		SEXTEXT("				(result = (result + i))") // Sum the index to result
		SEXTEXT("			)")
		SEXTEXT("		)")
		SEXTEXT("	catch ex")
		SEXTEXT("		(")
		SEXTEXT("			(Sys.Print ex.Message)")
		SEXTEXT("			(result = 1999)")
		SEXTEXT("		)")
		SEXTEXT("	)")
		SEXTEXT("	(sanctuary.kennelIds.Pop)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayForeachAndThrow"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Sanctuary (array Int32 kennelIds))")

		SEXTEXT("(method Sanctuary.Construct (Int32 capacity)")
		SEXTEXT("	-> (construct kennelIds capacity)")		
		SEXTEXT("	:")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Sanctuary sanctuary (10) )") // Create a sanctuary with a capacity of 10 kennels
		SEXTEXT("	(sanctuary.kennelIds 9 0)") // Define and null the ten entries by setting entry 9 to 0

		SEXTEXT("	(foreach i k # (sanctuary.kennelIds 0 9)") // Enumerate over all 10 kennels
		SEXTEXT("		(try")
		SEXTEXT("			(")
		SEXTEXT("				(Sys.Throw -1 \"Test: foreach2 throw\")")
		SEXTEXT("			)")
		SEXTEXT("		catch ex")
		SEXTEXT("			(")
		SEXTEXT("				(result = (result + i))") // Sum the index to result")
		SEXTEXT("			)")
		SEXTEXT("		)")		
		SEXTEXT("	)")		
		SEXTEXT("	(sanctuary.kennelIds.Pop)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayForeachAndThrow2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 45);
	}

	void TestArrayForeachEachElementInArray(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a (10) )")
		SEXTEXT("	(#for (Int32 i = 0) (i < 10) (#inc i)")
		SEXTEXT("		(a.Push (i + 10))")
		SEXTEXT("	)")

		SEXTEXT("	(foreach i k # a") 
		SEXTEXT("		(result = (result + k))")
		SEXTEXT("	)")		
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayForeachEachElementInArray"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Int32 a (10) )")
		SEXTEXT("	(#for (Int32 i = 0) (i < 10) (#inc i)")
		SEXTEXT("		(a.Push (i + 10))")
		SEXTEXT("	)")

		SEXTEXT("	(foreach k # a") 
		SEXTEXT("		(result = (result + k))")
		SEXTEXT("	)")		
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayForeachEachElementInArrayWithoutIndex"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		
		int32 x = vm.PopInt32();
		validate(x == 145);
	}

	void TestArrayElementDeconstruct(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(interface EntryPoint.ITest (Id -> (Int32 id)))")
		SEXTEXT("(class Test (implements EntryPoint.ITest) (Int32 id))")
		SEXTEXT("(method Test.Construct (Int32 id): (this.id = id))")
		SEXTEXT("(method Test.Id -> (Int32 id): (id = this.id))")
		SEXTEXT("(method Test.Destruct -> : (Sys.Print \"Test finished\"))")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Test a (2) )")
		SEXTEXT("	(a.Push 12)")
		SEXTEXT("	(a.Push 14)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayElementDeconstruct"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestArrayWithinArrayDeconstruct(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(interface EntryPoint.ITest (Id -> (Int32 id)))")
		SEXTEXT("(class Test (implements EntryPoint.ITest) (Int32 id))")
		SEXTEXT("(method Test.Construct (Int32 id): (this.id = id))")
		SEXTEXT("(method Test.Id -> (Int32 id): (id = this.id))")
		SEXTEXT("(method Test.Destruct -> : (Sys.Print \"Test destructed\"))")

		SEXTEXT("(struct Axis (array Test tests))")
		SEXTEXT("(method Axis.Construct (Int32 testsPerAxis) -> (construct tests testsPerAxis): )")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Axis axes (3) )")
		SEXTEXT("	(axes.Push 3)")
		SEXTEXT("	(axes.Push 3)")
		SEXTEXT("	(axes.Push 3)")
		SEXTEXT("	(foreach axis # axes ")
		SEXTEXT("		(axis.tests.Push 1)")
		SEXTEXT("		(axis.tests.Push 2)")
		SEXTEXT("		(axis.tests.Push 3)")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayWithinArrayDeconstruct"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
	}

	void TestArrayElementDeconstructWhenThrown(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(interface EntryPoint.ITest (Id -> (Int32 id)))")
		SEXTEXT("(class Test (implements EntryPoint.ITest) (Int32 id))")
		SEXTEXT("(method Test.Construct (Int32 id): (this.id = id))")
		SEXTEXT("(method Test.Id -> (Int32 id): (id = this.id))")
		SEXTEXT("(method Test.Destruct -> : (Sys.Print \"Test finished\"))")

		SEXTEXT("(function Main2 -> (Int32 result):")
		SEXTEXT("	(array Test a (2) )")
		SEXTEXT("	(a.Push 12)")
		SEXTEXT("	(a.Push 14)")
		SEXTEXT("	(Sys.Throw 747 \"This should trigger the autodestruct sequence\")")
		SEXTEXT(")")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(try")
		SEXTEXT("		(")
		SEXTEXT("			(Main2 -> result)")
		SEXTEXT("		)")
		SEXTEXT("	catch ex")
		SEXTEXT("		(")
		SEXTEXT("			(result = ex.ErrorCode)")
		SEXTEXT("		)")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayElementDeconstructWhenThrown"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 747);
	}

	void TestArrayElementLockRef(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Vec4 (Int32 x y z w))")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(array Vec4 vectors 5)")
		SEXTEXT("	(vectors.Push Vec4 (1 2 3 0))")
		SEXTEXT("	(foreach v # (vectors 0)")
		SEXTEXT("		(result = v.z)")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestArrayElementLockRef"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 3);
	}

	void TestLinkedList(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Int32 a)")
		SEXTEXT("	(result = a.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedList"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Int32 a)")
		SEXTEXT("	(a.Append 25)")
		SEXTEXT("	(result = a.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedList2"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Int32 a)")
		SEXTEXT("	(a.Append 25)")
		SEXTEXT("	(node n = a.Tail)")
		SEXTEXT("	(result = (n.Value + a.Length))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedList3"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Int32 a)")
		SEXTEXT("	(a.Prepend 25)")
		SEXTEXT("	(node n = a.Head)")
		SEXTEXT("	(result = ((3 + n.Value) + (a.Length + 2)))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedList4"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Int32 a)")
		SEXTEXT("	(try")
		SEXTEXT("		(")
		SEXTEXT("			(node n = a.Head)")
		SEXTEXT("			(result = n.Value)")
		SEXTEXT("		)")
		SEXTEXT("	catch ex")
		SEXTEXT("		(")
		SEXTEXT("			(result = 39)")
		SEXTEXT("			(Sys.Print ex.Message)")
		SEXTEXT("		)")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedList6"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Int32 a)")
		SEXTEXT("	(a.Prepend 17)")
		SEXTEXT("	(a.Prepend 34)")
		SEXTEXT("	(node head = a.Head)")
		SEXTEXT("	(node tail = head.Next)")
		SEXTEXT("	(result = tail.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedList7"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Int32 a)")
		SEXTEXT("	(a.Append 17)")
		SEXTEXT("	(a.Append 34)")
		SEXTEXT("	(node tail = a.Tail)")
		SEXTEXT("	(node head = tail.Previous)")
		SEXTEXT("	(result = head.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedList8"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Int32 a)")
		SEXTEXT("	(a.Append 17)")
		SEXTEXT("	(a.Append 34)")
		SEXTEXT("	(node head = a.Head)")
		SEXTEXT("	(head.Append 32)") // This inserts an element 32 between 17 and 34
		SEXTEXT("	(node tail = a.Tail)")
		SEXTEXT("	(node newNode = tail.Previous)")
		SEXTEXT("	(result = newNode.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedList9"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Int32 a)")
		SEXTEXT("	(a.Append 17)")
		SEXTEXT("	(a.Append 34)")
		SEXTEXT("	(node n = a.Tail)")
		SEXTEXT("	(n.Pop)")
		SEXTEXT("	(node tail = a.Tail)")
		SEXTEXT("	(result = tail.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedList10"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Stuff (list Int32 a))")

		SEXTEXT("(method Stuff.Construct -> (construct a()) : )")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Stuff stuff())")
		SEXTEXT("	(stuff.a.Append 17)")
		SEXTEXT("	(stuff.a.Prepend 34)")
		SEXTEXT("	(node tail = stuff.a.Tail)")
		SEXTEXT("	(result = (result + tail.Value))")
		SEXTEXT("	(foreach n # stuff.a (result = (result + n.Value)))")
		SEXTEXT("	(result = (result + stuff.a.Length))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedList11"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 70);
	}

	void TestLinkedListOfArchetypes(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(archetype Sys.OneToOne (Int32 x) -> (Int32 y))")

			SEXTEXT("(function DoubleInt (Int32 x) -> (Int32 y): (y = (2 * x)))")

			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("	(list Sys.OneToOne a)")
			SEXTEXT("	(a.Append DoubleInt)")
			SEXTEXT("	(node head = a.Head)")
			SEXTEXT("	(Sys.OneToOne f = head.Value)")
			SEXTEXT("   (result = (f 17))")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestLinkedListOfArchetypes"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(function AppendThree (list Int32 a) -> : ")
		SEXTEXT("	(a.Prepend 17)")
		SEXTEXT("	(a.Prepend 34)")
		SEXTEXT("	(a.Prepend 9)")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Int32 a)")
		SEXTEXT("	(AppendThree a)")
		SEXTEXT("	(node head = a.Head)")
		SEXTEXT("	(result = head.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedList12"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 9);
	}

	void TestLinkedListOfLists(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Dog (list Int32 boneIds))")

		SEXTEXT("(method Dog.Construct -> (construct boneIds ()) : )")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Dog dogs)")
		SEXTEXT("	(dogs.Append ())")
		SEXTEXT("	(node head = dogs.Head)")
		SEXTEXT("	(Dog firstDog = & head)")
		SEXTEXT("	(firstDog.boneIds.Append 27)")
		SEXTEXT("	(node bone = firstDog.boneIds.Head)")
		SEXTEXT("	(result = bone.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedListOfLists"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")

		SEXTEXT("	(list Int32 a)")

		SEXTEXT("	(a.Append 17)")
		SEXTEXT("	(a.Append 34)")
		SEXTEXT("	(a.Append 333)")

		SEXTEXT("	(foreach i n # a ")
		SEXTEXT("		(result = (i + (result + n.Value)))")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedListForeach1"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")

		SEXTEXT("	(list Int32 a)")

		SEXTEXT("	(a.Append 17)")
		SEXTEXT("	(a.Append 34)")
		SEXTEXT("	(a.Append 333)")

		SEXTEXT("	(foreach n # a ")
		SEXTEXT("		(result = (result + n.Value))")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedListForeach2"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")

		SEXTEXT("	(list Int32 a)")

		SEXTEXT("	(a.Append 17)")
		SEXTEXT("	(a.Append 34)")
		SEXTEXT("	(a.Append 333)")

		SEXTEXT("	(a.Clear)")

		SEXTEXT("	(foreach n # a ")
		SEXTEXT("	)")
		SEXTEXT("	(result = a.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedListForeach3"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")

		SEXTEXT("	(list Int32 a)")

		SEXTEXT("	(a.Append 17)")
		SEXTEXT("	(a.Append 34)")
		SEXTEXT("	(a.Append 333)")

		SEXTEXT("	(foreach n # a ")
		SEXTEXT("		(n.Pop)")
		SEXTEXT("	)")
		SEXTEXT("	(result = a.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedListForeach4"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")

		SEXTEXT("	(list Int32 a)")

		SEXTEXT("	(a.Append 17)")
		SEXTEXT("	(a.Append 34)")
		SEXTEXT("	(a.Append 333)")

		SEXTEXT("	(foreach n # a ")
		SEXTEXT("		(result = (result + 1))")
		SEXTEXT("		(a.Clear)")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedListForeach5"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")

		SEXTEXT("	(list Int32 a)")

		SEXTEXT("	(a.Append 17)")
		SEXTEXT("	(a.Append 34)")
		SEXTEXT("	(a.Append 333)")

		SEXTEXT("	(foreach n # a ")
		SEXTEXT("		(try")
		SEXTEXT("			(")
		SEXTEXT("				(Sys.Throw n.Value \"thrown in foreach \")")
		SEXTEXT("			)")
		SEXTEXT("		catch ex")
		SEXTEXT("			(")
		SEXTEXT("				(result = (result + 1))")
		SEXTEXT("				(Sys.Print ex.Message)")
		SEXTEXT("			)")
		SEXTEXT("		)")
		SEXTEXT("		(result = (result + 10))")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedListForeach6"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")

		SEXTEXT("	(list Int32 a)")

		SEXTEXT("	(a.Append 17)")
		SEXTEXT("	(a.Append 34)")
		SEXTEXT("	(a.Append 333)")

		SEXTEXT("	(try")
		SEXTEXT("		(")
		SEXTEXT("			(foreach n # a (Sys.Throw n.Value \"thrown in foreach\"))")
		SEXTEXT("		)")
		SEXTEXT("	catch ex")
		SEXTEXT("		(")
		SEXTEXT("			(result = (result + 1))")
		SEXTEXT("			(Sys.Print ex.Message)")
		SEXTEXT("		)")
		SEXTEXT("	)")
		SEXTEXT("	(result = (result + 10))")

		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestLinkedListForeach7"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 11);
	}

	void TestListStruct(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Vec3 (Float32 x y z))")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Vec3 a)")
		SEXTEXT("	(Vec3 v = 1 2 3)")
		SEXTEXT("	(a.Append v)")
		SEXTEXT("	(result = a.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestListStruct"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Vec3 (Int32 x y z))")
 
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Vec3 a)")
		SEXTEXT("	(Vec3 v = 1 2 3)")
		SEXTEXT("	(a.Append v)")
		SEXTEXT("	(node n = a.Head)")
		SEXTEXT("	(Vec3 val = & n)")
		SEXTEXT("	(result = val.z)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestListStruct2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 3);
	}

	void TestListStrongTyping(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(function FillArray (list Int32 a) -> : ")
		SEXTEXT("	(a.Append 37)")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(list Float32 a)")
		SEXTEXT("	(FillArray a)")
		SEXTEXT("	(node n = a.Head)")
		SEXTEXT("	(result = n.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestListStrongTyping"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		ParseException ex;
		if (!s_logger.TryGetNextException(ex))
			validate(false);
	}

	void TestMap(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map IString Int32 a)")
		SEXTEXT("	(result = a.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMap"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 0);
	}

	void TestMapOfArchetypes(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(archetype Sys.OneToOne (Int32 x) -> (Int32 y))")

			SEXTEXT("(function DoubleInt (Int32 x) -> (Int32 y): (y = (2 * x)))")

			SEXTEXT("(function Main -> (Int32 result):")
			SEXTEXT("	(map IString Sys.OneToOne a)")
			SEXTEXT("	(a.Insert \"Joe\" DoubleInt)")
			SEXTEXT("	(node n = (a \"Joe\"))")
			SEXTEXT("   (Sys.OneToOne f = n.Value)")
			SEXTEXT("	(result = (f 17))")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestMapOfArchetypes"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 34);
	}

	void TestMap2(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map IString Int32 a)")
		SEXTEXT("	(a.Insert \"Joe\" 90 )")
		SEXTEXT("	(result = a.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMap2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 1);
	}

	void TestMap3(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map IString Int32 a)")
		SEXTEXT("	(node n = (a \"Joe\"))")
		SEXTEXT("	(if (n.Exists)")
		SEXTEXT("		(result = 1)")
		SEXTEXT("	else")
		SEXTEXT("		(result = 2)")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMap3"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map IString Int32 a)")
		SEXTEXT("	(a.Insert \"Joe\" 90 )")
		SEXTEXT("	(node n = (a \"Joe\"))")
		SEXTEXT("	(if (n.Exists)")
		SEXTEXT("		(result = 3)")
		SEXTEXT("	else")
		SEXTEXT("		(result = 4)")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMap4"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map IString Int32 a)")
		SEXTEXT("	(node n = (a \"Joe\"))")
		SEXTEXT("	(result = n.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMap5"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		ParseException ex;
		if (!s_logger.TryGetNextException(ex))
			validate(false);
	}

	void TestMap6(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map IString Int32 a)")
		SEXTEXT("	(a.Insert \"Joe\" 90)")
		SEXTEXT("	(node n = (a \"Joe\"))")
		SEXTEXT("	(result = n.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMap6"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map IString Int32 a)")
		SEXTEXT("	(a.Insert \"Joe\" 90)")
		SEXTEXT("	(a.Insert \"Alice\" 37)")
		SEXTEXT("	(a.Insert \"Fred\" 65)")
		SEXTEXT("	(node n = (a \"Joe\"))")
		SEXTEXT("	(result = n.Value)")
		SEXTEXT("	(node m = (a \"Fred\"))")
		SEXTEXT("	(result = (result + m.Value))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMap7"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 155);
	}

	void TestMapOverwriteValue(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map IString Int32 a)")
		SEXTEXT("	(a.Insert \"Joe\" 90)")
		SEXTEXT("	(a.Insert \"Joe\" 239)")
		SEXTEXT("	(node m = (a \"Joe\"))")
		SEXTEXT("	(result = m.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapOverwriteValue"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map IString Float64 a)")
		SEXTEXT("	(a.Insert \"Joe\" 90)")
		SEXTEXT("	(a.Insert \"Joe\" 239)")
		SEXTEXT("	(node m = (a \"Joe\"))")
		SEXTEXT("	(if (m.Value == 239) (result = 1) else (result = 0))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapOverwriteValue64"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map Int32 Int32 a)")
		SEXTEXT("	(a.Insert 45 48)")
		SEXTEXT("	(node n = (a 45))")
		SEXTEXT("	(result = n.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapIndexedByInt32"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map Float64 Int32 a)")
		SEXTEXT("	(a.Insert 45 48)")
		SEXTEXT("	(node n = (a 45))")
		SEXTEXT("	(result = n.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapIndexedByFloat64"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map Float32 Int32 a)")
		SEXTEXT("	(a.Insert 45 48)")
		SEXTEXT("	(a.Insert 37 -248)")
		SEXTEXT("	(node n = (a 37))")
		SEXTEXT("	(n.Pop)")
		SEXTEXT("	(node m = (a 45))")
		SEXTEXT("	(result = (m.Value + a.Length))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDeleteKey"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 49);
	}

	void TestMapValueStruct(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Vec4 (Int32 x y z w))")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map Int32 Vec4 a)")
		SEXTEXT("	(a.Insert 45 Vec4 (1 2 3 4))")
		SEXTEXT("	(node n = (a 45))")
		SEXTEXT("	(Vec4 v = & n)")
		SEXTEXT("	(result = v.y)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapValueStruct"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());

		int x = vm.PopInt32();
		validate(x == 2);
	}

	void TestMapValueConstruct(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Vec4 (Int32 x y z w))")
		SEXTEXT("(method Vec4.Construct (Int32 x) (Int32 y) (Int32 z) -> : (this.x = x) (this.y = y) (this.z = z) (this.w = 1))") 
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map Int32 Vec4 a)")
		SEXTEXT("	(a.Insert 45 Vec4 (1 2 3 4))")
		SEXTEXT("	(node n = (a 45))")
		SEXTEXT("	(Vec4 v = & n)")
		SEXTEXT("	(result = v.z)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapValueConstruct"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Vec4 (Int32 x y z w))")
		SEXTEXT("(method Vec4.Construct (Int32 x) (Int32 y) (Int32 z) -> : (this.x = x) (this.y = y) (this.z = z) (this.w = 1))") 
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map Int32 Vec4 a)")
		SEXTEXT("	(a.Insert 45 Vec4 (1 2 3))")
		SEXTEXT("	(a.Insert 23 Vec4 (5 6 7))")
		SEXTEXT("	(a.Insert -6 Vec4 (8 9 10))")
		SEXTEXT("	(foreach n # a ")
		SEXTEXT("		(Vec4 v = & n)")
		SEXTEXT("		(result = (result + v.x))")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapForeach1"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Vec4Map (map Int32 Vec4 a))")
		SEXTEXT("(method Vec4Map.Construct -> (construct a () ) : )")

		SEXTEXT("(struct Vec4 (Int32 x y z w))")
		SEXTEXT("(method Vec4.Construct (Int32 x) (Int32 y) (Int32 z) -> : (this.x = x) (this.y = y) (this.z = z) (this.w = 1))") 
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(Vec4Map m () )")
		SEXTEXT("	(m.a.Insert 45 Vec4 (1 2 3))")
		SEXTEXT("	(m.a.Insert 23 Vec4 (5 6 7))")
		SEXTEXT("	(m.a.Insert -6 Vec4 (8 9 10))")
		SEXTEXT("	(foreach n # m.a ")
		SEXTEXT("		(Vec4 v = & n)")
		SEXTEXT("		(result = (result + v.x))")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapInStruct"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
		
		SEXTEXT("(struct Vec4 (Int32 x y z w))")
		SEXTEXT("(method Vec4.Construct (Int32 x) (Int32 y) (Int32 z) -> : (this.x = x) (this.y = y) (this.z = z) (this.w = 1))") 

		SEXTEXT("(struct Vec4Map (map Int32 Vec4 a))")
		SEXTEXT("(method Vec4Map.Construct -> (construct a () ) : )")

		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map Int32 Vec4Map m)")
		SEXTEXT("	(m.Insert 45 Vec4Map ())")
		SEXTEXT("	(node n = (m 45))")
		SEXTEXT("	(Vec4Map subMap = & n)")
		SEXTEXT("	(subMap.a.Insert 22 Vec4 (199 233 455))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapInMap"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		validate(ss.ValidateMemory());
	}

	void TestMapCall(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(function Insert55v52 (map Int32 Int32 x) -> : (x.Insert 55 52))")
		
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map Int32 Int32 m)")
		SEXTEXT("	(Insert55v52 m)")
		SEXTEXT("	(node n = (m 55))")
		SEXTEXT("	(result = n.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapCall"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(function Insert55v52 (map Int64 Int64 a) -> : ")
		SEXTEXT("	(a.Insert 55 52)")
		SEXTEXT(")")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map Int32 Float32 a)")
		SEXTEXT("	(Insert55v52 a)")
		SEXTEXT("	(node n = (a 55))")
		SEXTEXT("	(result = n.Value)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapStrongTyping"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		ParseException ex;
		if (!s_logger.TryGetNextException(ex))
			validate(false);
	}

	void TestMapThrowAndCleanup(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(try")
		SEXTEXT("		(")
		SEXTEXT("			(map IString Int32 a)")
		SEXTEXT("			(a.Insert \"Joe\" 90 )")
		SEXTEXT("			(Sys.Throw 7001 \"Ayup\")")
		SEXTEXT("			(result = a.Length)")
		SEXTEXT("		)")
		SEXTEXT("	catch ex")
		SEXTEXT("		(")
		SEXTEXT("			(result = ex.ErrorCode)")
		SEXTEXT("		)")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapThrowAndCleanup"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map IString Int32 a)")
		SEXTEXT("	(a.Insert \"Joe\" 90 )")
		SEXTEXT("	(try")
		SEXTEXT("		(")	
		SEXTEXT("			(foreach v # a (Sys.Throw 7051 \"Ayup\"))")	
		SEXTEXT("		)")
		SEXTEXT("	catch ex")
		SEXTEXT("		(")
		SEXTEXT("			(result = ex.ErrorCode)")
		SEXTEXT("		)")
		SEXTEXT("	)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapThrowAndCleanup2"));
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
		csexstr srcCode =
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(struct Vec4 (Int32 x y z w))")
		SEXTEXT("(method Vec4.Construct (Int32 x) (Int32 y) (Int32 z) -> : (this.x = x) (this.y = y) (this.z = z) (this.w = 1))") 
  
		SEXTEXT("(function Main -> (Int32 result):")
		SEXTEXT("	(map Int32 Vec4 a)")
		SEXTEXT("	(a.Insert 45 Vec4 (1 2 3))")
		SEXTEXT("	(a.Insert 23 Vec4 (5 6 7))")
		SEXTEXT("	(a.Insert -6 Vec4 (8 9 10))")
		SEXTEXT("	(foreach n # a ")
		SEXTEXT("		(n.Pop)")
		SEXTEXT("	)")
		SEXTEXT("	(result = a.Length)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMapForeach2"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(alias Sys.Type.Int32 Sys.Type.Degrees)")
  
		SEXTEXT("(function Main -> (Degrees result):")
		SEXTEXT("	(result = 360)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestTypedef"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x) (Float32 y):")
		SEXTEXT("	(Sys.Maths.F32.Sin 0.5 -> x)")
		SEXTEXT("	(Sys.Maths.F32.Cos 0.5 -> y)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathSinCosF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x) (Float64 y):")
		SEXTEXT("	(Sys.Maths.F64.Sin 0.5 -> x)")
		SEXTEXT("	(Sys.Maths.F64.Cos 0.5 -> y)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathSinCos"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.Tan 0.5 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathTan"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.Tan 0.5 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathTanF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.ArcTan 0.5 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathArcTanF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.ArcTan 0.5 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathArcTan"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.ArcTanYX 1 2 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathArcTanGradF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.ArcTanYX 1 2 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathArcTanGrad"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.ArcSin 0.5 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathArcSin"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.ArcSin 0.5 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathArcSinF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.ArcCos 0.5 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathArcCos"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.ArcCos 0.5 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathArcCosF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x) (Float32 y):")
		SEXTEXT("	(Sys.Maths.F32.Sinh 0.5 -> x)")
		SEXTEXT("	(Sys.Maths.F32.Cosh 0.5 -> y)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathSinhCoshF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x) (Float64 y):")
		SEXTEXT("	(Sys.Maths.F64.Sinh 0.5 -> x)")
		SEXTEXT("	(Sys.Maths.F64.Cosh 0.5 -> y)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathSinhCosh"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.Tanh 0.5 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathTanh"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.Tanh 0.5 -> x)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathTanhF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.Exp 1.0 -> x)")
		SEXTEXT("	(x = (x - 2.7182818284590452353602874713527) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathExp"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.Exp 1.0 -> x)")
		SEXTEXT("	(x = (x - 2.7182818284590452353602874713527) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathExpF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.LogN 2.0 -> x)")
		SEXTEXT("	(x = (x - 0.69314718055994530941723212145818) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathLogN"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.LogN 2 -> x)")
		SEXTEXT("	(x = (x - 0.69314718055994530941723212145818) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathLogNF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.Log10 1000.0 -> x)")
		SEXTEXT("	(x = (x - 3) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathLog10"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.Log10 100000 -> x)")
		SEXTEXT("	(x = (x - 5) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathLog10F"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.Power 3.0 4.0 -> x)")
		SEXTEXT("	(x = (x - 81) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathPow"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.Power 7.0 2.0 -> x)")
		SEXTEXT("	(x = (x - 49.0) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathPowF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.Abs -5 -> x)")
		SEXTEXT("	(x = (x - 5) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathAbsF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.Abs 12 -> x)")
		SEXTEXT("	(x = (x - (Sys.Maths.F64.Abs -12) ))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathAbs"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.SquareRoot 25 -> x)")
		SEXTEXT("	(x = (x - 5) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathSqrtF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.SquareRoot 144 -> x)")
		SEXTEXT("	(x = (x - 12))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathSqrt"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 x):")
		SEXTEXT("	(Sys.Maths.I32.Abs 12 -> x)")
		SEXTEXT("	(x = (x - (Sys.Maths.I32.Abs -12) ))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathAbsInt32"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int64 x):")
		SEXTEXT("	(Sys.Maths.I64.Abs 12 -> x)")
		SEXTEXT("	(x = (x - (Sys.Maths.I64.Abs -12) ))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathAbsInt64"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0x17FFF7FFFL); // Allocate stack space for the int64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int64 x = vm.PopInt64();

		validate(x == 0)
	}

	void TestMathCeilF(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.Ceiling 25.00123 -> x)")
		SEXTEXT("	(x = (x - 26) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathCeilF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.Ceiling 17.98 -> x)")
		SEXTEXT("	(x = (x - 18))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathCeil"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float32 x):")
		SEXTEXT("	(Sys.Maths.F32.Floor 25.00123 -> x)")
		SEXTEXT("	(x = (x - 25) )")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathFloorF"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Float64 x):")
		SEXTEXT("	(Sys.Maths.F64.Floor 17.98 -> x)")
		SEXTEXT("	(x = (x - 17))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathFloor"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 rem):")
			SEXTEXT("	(Sys.Maths.I32.Mod 16 9 -> rem)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathModInt32"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int64 rem):")
			SEXTEXT("	(Sys.Maths.I64.Mod 12 5 -> rem)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathModInt64"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0x17FFF7FFFL); // Allocate stack space for the int64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int64 rem = vm.PopInt64();
		int64 quot = vm.PopInt64();

		validate(rem == 2);
	}

	void TestMathShiftInt64(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int64 x):")
		SEXTEXT("	(x = 0)")
		SEXTEXT("	(x = (x + (Sys.Maths.I64.LeftShift 3 2)))")
		SEXTEXT("	(x = (x + (Sys.Maths.I64.RightShift 32 5)))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathShiftInt64"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0x37FFF7FFF); // Allocate stack space for the int64 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int64 x = vm.PopInt64();

		validate(x == 13);
	}

	void TestMathShiftInt32(IPublicScriptSystem& ss)
	{
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")
  
		SEXTEXT("(function Main -> (Int32 x):")
		SEXTEXT("	(x = 0)")
		SEXTEXT("	(x = (x + (Sys.Maths.I32.LeftShift 3 2)))")
		SEXTEXT("	(x = (x + (Sys.Maths.I32.RightShift 32 5)))")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMathShiftInt32"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Int32 x):")
			SEXTEXT("	(Int32 y = (Sys.Maths.I32.MinOf 5 7))")
			SEXTEXT("	(Int32 z = (Sys.Maths.I32.MinOf -1 14))")
			SEXTEXT("	(Sys.Maths.I32.MaxOf y z -> x)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestMinMaxInt32"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Float32 x):")
			SEXTEXT("	(Float32 y = (Sys.Maths.F32.MinOf 5 7))")
			SEXTEXT("	(Float32 z = (Sys.Maths.F32.MinOf -1 14))")
			SEXTEXT("	(Sys.Maths.F32.MaxOf y z -> x)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestMinMaxFloat32"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Int64 x):")
			SEXTEXT("	(Int64 y = (Sys.Maths.I64.MinOf 5 7))")
			SEXTEXT("	(Int64 z = (Sys.Maths.I64.MinOf -1 14))")
			SEXTEXT("	(Sys.Maths.I64.MaxOf y z -> x)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestMinMaxInt64"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Float64 x):")
			SEXTEXT("	(Float64 y = (Sys.Maths.F64.MinOf 5 7))")
			SEXTEXT("	(Float64 z = (Sys.Maths.F64.MinOf -1 14))")
			SEXTEXT("	(Sys.Maths.F64.MaxOf y z -> x)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestMinMaxFloat64"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Int32 x)(Int32 y):")
			SEXTEXT("	(x = Sys.Maths.I32.MinValue)")
			SEXTEXT("	(y = Sys.Maths.I32.MaxValue)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestLimitsInt32"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):")
			SEXTEXT("	(Sys.Maths.F32.IsInfinity (1 / 0) -> x)")
			SEXTEXT("	(Sys.Maths.F32.IsInfinity (-1 / 0) -> y)")
			SEXTEXT("	(Sys.Maths.F32.IsInfinity (0 / 0) -> z)")
			SEXTEXT("	(Sys.Maths.F32.IsInfinity 3 -> w)")
			SEXTEXT(")");
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestIsInfinity"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z):")
			SEXTEXT("	(Sys.Maths.F32.IsQuietNan (0 / 0) -> x)")
			SEXTEXT("	(Sys.Maths.F32.IsQuietNan (1 / 0) -> y)")
			SEXTEXT("	(Sys.Maths.F32.IsQuietNan 2 -> z)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestIsQuietNan"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):")
			SEXTEXT("	(Sys.Maths.F32.IsFinite (0 / 0) -> x)")
			SEXTEXT("	(Sys.Maths.F32.IsFinite (1 / 0) -> y)")
			SEXTEXT("	(Sys.Maths.F32.IsFinite 0 -> z)")
			SEXTEXT("	(Sys.Maths.F32.IsFinite 1 -> w)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestIsFinite"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):")
			SEXTEXT("	(Sys.Maths.F32.IsNormal (0 / 0) -> x)")
			SEXTEXT("	(Sys.Maths.F32.IsNormal (1 / 0) -> y)")
			SEXTEXT("	(Sys.Maths.F32.IsNormal 0 -> z)")
			SEXTEXT("	(Sys.Maths.F32.IsNormal 1 -> w)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestIsNormal"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):")
			SEXTEXT("	(Sys.Maths.F64.IsInfinity (1 / 0) -> x)")
			SEXTEXT("	(Sys.Maths.F64.IsInfinity (-1 / 0) -> y)")
			SEXTEXT("	(Sys.Maths.F64.IsInfinity (0 / 0) -> z)")
			SEXTEXT("	(Sys.Maths.F64.IsInfinity 3 -> w)")
			SEXTEXT(")");
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestIsInfinity64"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z):")
			SEXTEXT("	(Sys.Maths.F64.IsQuietNan (0 / 0) -> x)")
			SEXTEXT("	(Sys.Maths.F64.IsQuietNan (1 / 0) -> y)")
			SEXTEXT("	(Sys.Maths.F64.IsQuietNan 2 -> z)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestIsQuietNan64"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):")
			SEXTEXT("	(Sys.Maths.F64.IsFinite (0 / 0) -> x)")
			SEXTEXT("	(Sys.Maths.F64.IsFinite (1 / 0) -> y)")
			SEXTEXT("	(Sys.Maths.F64.IsFinite 0 -> z)")
			SEXTEXT("	(Sys.Maths.F64.IsFinite 1 -> w)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestIsFinite64"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Bool x)(Bool y)(Bool z)(Bool w):")
			SEXTEXT("	(Sys.Maths.F64.IsNormal (0 / 0) -> x)")
			SEXTEXT("	(Sys.Maths.F64.IsNormal (1 / 0) -> y)")
			SEXTEXT("	(Sys.Maths.F64.IsNormal 0 -> z)")
			SEXTEXT("	(Sys.Maths.F64.IsNormal 1 -> w)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestIsNormal64"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Float32 x)(Float32 y):")
			SEXTEXT("	(x = Sys.Maths.F32.MinValue)")
			SEXTEXT("	(y = Sys.Maths.F32.MaxValue)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestLimitsFloat32"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Float64 x)(Float64 y):")
			SEXTEXT("	(x = Sys.Maths.F64.MinValue)")
			SEXTEXT("	(y = Sys.Maths.F64.MaxValue)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestLimitsFloat64"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT(" (alias Main EntryPoint.Main)")

			SEXTEXT("(using Sys.Type)")

			SEXTEXT("(function Main -> (Int64 x)(Int64 y):")
			SEXTEXT("	(x = Sys.Maths.I64.MinValue)")
			SEXTEXT("	(y = Sys.Maths.I64.MaxValue)")
			SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestLimitsInt64"));
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
		csexstr srcCode = 
		SEXTEXT("(namespace EntryPoint)")
		SEXTEXT(" (alias Main EntryPoint.Main)")
  
		SEXTEXT("(using Sys.Type)")

		SEXTEXT("(namespace Bot)")

		SEXTEXT("(interface Bot.IRobotBrain (Id -> (Int32 id)))")
		SEXTEXT("(interface Bot.IRobot (Brain -> (Bot.IRobotBrain brain)))")

		SEXTEXT("(class Robby (implements Bot.IRobot) (RobotBrain brain))")
		SEXTEXT("(class RobotBrain (implements Bot.IRobotBrain))")

		SEXTEXT("(method RobotBrain.Id -> (Int32 id): (id = 9000))")
		SEXTEXT("(method Robby.Brain -> (Bot.IRobotBrain brain): (brain = this.brain))")
  
		SEXTEXT("(function Main -> (Int32 exitCode):")
		SEXTEXT("	(Robby robot)")
		SEXTEXT("	(Bot.IRobotBrain brain = robot.Brain)")
		SEXTEXT("	(brain.Id -> exitCode)")
		SEXTEXT(")");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestReturnInterface"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	(result = 7)")
			SEXTEXT(")")			
			SEXTEXT("(alias Main EntryPoint.Main)")

			SEXTEXT("(function F (Int32 x) -> (Int32 y) -> : (y = x))")			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestDoubleArrowsInFunction"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		try
		{
			VM::IVirtualMachine& vm = StandardTestInit(ss, tree());	
			vm.Push(0); // Allocate stack space for the int32 result
			EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
			validate(false);
		}
		catch (ParseException&)
		{
			
		}
	}

	void TestMemberwiseInit(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(struct GameObject (Int32 value) (IString name) (Int32 id))")	
			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	(GameObject obj = 7 \"dog\" 12)")
			SEXTEXT("	(Sys.Print obj.name)")
			SEXTEXT("	(result = obj.name.Length)")
			SEXTEXT(")")			
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMemberwiseInit"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 3);		
	}

	void TestStructStringPassByRef(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(struct GameObject (Int32 value) (IString name) (Int32 id))")	
			SEXTEXT("(function PrintGameObject (GameObject obj)-> (Int32 length) : ")	
			SEXTEXT("	(Sys.Print obj.name -> length)")	
			SEXTEXT(")")	
			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	(GameObject obj = 7 \"dog\" 12)")
			SEXTEXT("	(PrintGameObject obj -> result)")
			SEXTEXT(")")			
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStructStringPassByRef"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 3);		
	}

	void TestEmptyString(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
	
			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	(IString s = \"\")")
			SEXTEXT("	(Sys.Print s -> result)")
			SEXTEXT(")")			
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestEmptyString"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 0);		
	}

	void TestMeshStruct(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")

			SEXTEXT("(struct MeshInstance")
			SEXTEXT("	(IString name)")
			SEXTEXT("	(IString modelName)")
			SEXTEXT("	(Sys.Maths.Matrix4x4 worldMatrix)")
			SEXTEXT(")")

			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	(MeshInstance instance = ")
			SEXTEXT("		\"geoff\" \"steve\"") 
			SEXTEXT("		(")
			SEXTEXT("			(-1.000000 0.000000 -0.000000 -1.239238)")
			SEXTEXT("			(-0.000000 -1.000000 0.000796 3.846236)")
			SEXTEXT("			(0.000000 0.000796 1.000000 0.506750)")
			SEXTEXT("			(0.000000 0.000000 0.000000 1.000000)")
			SEXTEXT("		)")
			SEXTEXT("	)")
			SEXTEXT("	(Sys.Print instance.name)")
			SEXTEXT("	(Sys.Print instance.modelName)")
			SEXTEXT(")")			
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMeshStruct"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 0);		
	}

	void TestMeshStruct2(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")

			SEXTEXT("(struct MeshInstance")
			SEXTEXT("	(IString name)")
			SEXTEXT("	(IString modelName)")
			SEXTEXT("	(Sys.Maths.Matrix4x4 worldMatrix)")
			SEXTEXT(")")

			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	(MeshInstance instance = ")
			SEXTEXT("		\"geoff\" \"steve\"") 
			SEXTEXT("		(")
			SEXTEXT("			(-1.000000 0.000000 -0.000000 -1.239238)")
			SEXTEXT("			(-0.000000 -1.000000 0.000796 3.846236)")
			SEXTEXT("			(0.000000 0.000796 1.000000 0.506750)")
			SEXTEXT("			(0.000000 0.000000 0.000000 1.000000)")
			SEXTEXT("		)")
			SEXTEXT("	)")
			SEXTEXT("	(Cat instance -> result)")
			SEXTEXT(")")	
			SEXTEXT("(function Cat(MeshInstance instance) -> (Int32 result):")	
			SEXTEXT("	(Dog instance.worldMatrix -> result)")
			SEXTEXT(")")	
			SEXTEXT("(function Dog(Sys.Maths.Matrix4x4 m) -> (Int32 result):")	
			SEXTEXT("	(if (m.r2.y == 0.000796) (result = 7) else (result = 9))")
			SEXTEXT(")")	
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMeshStruct2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 7);		
	}

	void TestMeshStruct3(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")

			SEXTEXT("(struct MeshInstance")
			SEXTEXT("	(IString name)")
			SEXTEXT("	(IString modelName)")
			SEXTEXT("	(Sys.Maths.Matrix4x4 worldMatrix)")
			SEXTEXT(")")

			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	(MeshInstance instance = ")
			SEXTEXT("		\"geoff\" \"steve\"") 
			SEXTEXT("		(")
			SEXTEXT("			(-1.000000 0.000000 -0.000000 -1.239238)")
			SEXTEXT("			(-0.000000 -1.000000 0.000796 3.846236)")
			SEXTEXT("			(0.000000 0.000796 1.000000 0.506750)")
			SEXTEXT("			(0.000000 0.000000 0.000000 1.000000)")
			SEXTEXT("		)")
			SEXTEXT("	)")
			SEXTEXT("	(PrintModelName instance -> result)")
			SEXTEXT(")")	
			SEXTEXT("(function PrintModelName(MeshInstance instance) -> (Int32 result):")	
			SEXTEXT("	(PrintString instance.modelName -> result)")
			SEXTEXT(")")	
			SEXTEXT("(function PrintString(IString s) -> (Int32 result):")	
			SEXTEXT("	(Sys.Print s -> result)")
			SEXTEXT(")")	
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMeshStruct3"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 5);		
	}

	void TestMeshStruct4(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type.Formatters)")
			SEXTEXT("(namespace EntryPoint)")

			SEXTEXT("(struct MeshInstance")
			SEXTEXT("	(IString name)")
			SEXTEXT("	(IString modelName)")
			SEXTEXT("	(Sys.Maths.Matrix4x4 worldMatrix)")
			SEXTEXT(")")

			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	(MeshInstance instance = ")
			SEXTEXT("		\"geoff\" \"steve\"") 
			SEXTEXT("		(")
			SEXTEXT("			(-1.000000 0.000000 -0.000000 -1.239238)")
			SEXTEXT("			(-0.000000 -1.000000 0.000796 3.846236)")
			SEXTEXT("			(0.000000 0.000796 1.000000 0.506750)")
			SEXTEXT("			(0.000000 0.000000 0.000000 1.000000)")
			SEXTEXT("		)")
			SEXTEXT("	)")
			SEXTEXT("	(Sys.Type.IStringBuilder s (Sys.Type.StringBuilder 256))")
			SEXTEXT("	(Pointer pName = instance.name.Buffer)")
			SEXTEXT("	(#Sys.Type.build s \"Hello World!\" pName NewLine)")
			SEXTEXT("	(Sys.Print s)")
			SEXTEXT("	(PrintModelName instance)")
			SEXTEXT("	(result = 7)")
			SEXTEXT(")")	
			SEXTEXT("(function PrintModelName(MeshInstance instance) -> (Int32 result):")	
			SEXTEXT("	(PrintString instance -> result)")
			SEXTEXT(")")	
			SEXTEXT("(function PrintString (MeshInstance instance) -> (Int32 result):")	
			SEXTEXT("	(Sys.Type.IStringBuilder s (Sys.Type.StringBuilder 256))")
			SEXTEXT("	(Pointer pName = instance.name.Buffer)")
			SEXTEXT("	(#Sys.Type.build s \"Hello World!\" pName NewLine)")
			SEXTEXT("	(Sys.Print s)")
			SEXTEXT(")")	
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestMeshStruct4"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		ValidateExecution(vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 7);		
	}

	void TestYield(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")

			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	(yield)")	
			SEXTEXT("	(result = 7)")
			SEXTEXT(")")	
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestYield"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		validate(EXECUTERESULT_YIELDED == vm.Execute(VM::ExecutionFlags(false, true)));
		validate(EXECUTERESULT_TERMINATED == vm.ContinueExecution(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 7);		
	}

	void TestStructByRefAssign(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(struct Cube (Int32 x y z))")
			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	(Cube a = 7 12 23)")	
			SEXTEXT("	(Cube b = 13 15 17)")	
			SEXTEXT("	(AssignCubeBtoA a b)")	
			SEXTEXT("	(result = a.x)")
			SEXTEXT(")")	
			SEXTEXT("(function AssignCubeBtoA (Cube a)(Cube b)-> :")
			SEXTEXT("	(a.x = b.x)")
			SEXTEXT("	(a.y = b.y)")
			SEXTEXT("	(a.z = b.z)")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestStructByRefAssign"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		validate(EXECUTERESULT_TERMINATED == vm.Execute(VM::ExecutionFlags(false, true)));

		int32 value = vm.PopInt32();
		validate(value == 13);		
	}

	void TestAssignVectorComponentsFromParameters(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(using Sys.Maths)")
			SEXTEXT("(function Main -> (Float32 result):")	
			SEXTEXT("	(SetVec 7.0 12.5 23.5 -> result)")	
			SEXTEXT(")")	
			SEXTEXT("(function SetVec (Float32 x)(Float32 y)(Float32 z)->(Float32 sum) :")
			SEXTEXT("	(Vec3 pos = x y z)")
			SEXTEXT("	(sum = (pos.y + pos.z))")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignVectorComponentsFromParameters"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		validate(EXECUTERESULT_TERMINATED == vm.Execute(VM::ExecutionFlags(false, true)));

		float value = vm.PopFloat32();
		validate(value == 36.0f);		
	}

	void TestConstructFromInterface(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(using Sys.Maths)")
			SEXTEXT("(using EntryPoint)")
			SEXTEXT("(interface EntryPoint.IFidelio")
			SEXTEXT("	(Id -> (Int32 id))")
			SEXTEXT(")")
			SEXTEXT("(class Fidelio (implements IFidelio))")
			SEXTEXT("(method Fidelio.Id -> (Int32 id): (id = 12))")
			SEXTEXT("(class Operas (ref IFidelio f))")
			SEXTEXT("(method Operas.Construct (IFidelio f)-> : (this.f = f))")
			SEXTEXT("(function Main -> (Int32 result):")	
			SEXTEXT("	(Fidelio f)")	
			SEXTEXT("	(Operas operas (f))")	
			SEXTEXT("	(operas.f.Id -> result)")
			SEXTEXT(")")	
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestConstructFromInterface"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);

		int value = vm.PopInt32();
		validate(value == 12);		
	}

	void TestAssignPointerFromFunction(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")

			SEXTEXT("(function Main -> (Pointer result):")	
			SEXTEXT("	(Pointer p = (Test.GetPointer 9))")	
			SEXTEXT("	(result = p)")	
			SEXTEXT(")")	
			SEXTEXT("(alias Main EntryPoint.Main)")		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestAssignPointerFromFunction"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		struct ANON
		{
			static void GetPointer(NativeCallEnvironment& e)
			{
				void* ptr = (void*) 0x12345678;
				WriteOutput(0, ptr, e);
			}
		};

		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Test"));
		ss.AddNativeCall(ns, ANON::GetPointer, NULL, SEXTEXT("GetPointer (Int32 id) -> (Pointer pointer)"), false);
		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push((void*) 0); // Allocate stack space for the int32 result

		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);

		void* value = vm.PopPointer();
		validate(value == (void*) 0x12345678);		
	}

	void TestMinimumConstruct(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(class Eros)")
			SEXTEXT(" (method Eros.Construct : )")
			SEXTEXT("(function Main -> :")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestMinimumConstruct"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);
	}

	void TestRaw(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(' man of bronze \"talos\")")
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> :")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestRaw"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);
	}

	void TestGlobalInt32(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(global Int32 i = 6)")
			SEXTEXT("(namespace EntryPoint)")	
			SEXTEXT("(function Main -> (Int32 x):")
			SEXTEXT("	(global i -> x)")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestGlobalInt32"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // add our output to the stack

		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int x = vm.PopInt32();
		
		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);
		
		validate(x == 6);
	}

	void TestGlobalInt32_2(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(global Int32 i = 6)")
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 x):")
			SEXTEXT("	(Int32 value = 4)")
			SEXTEXT("	(global i <- value)")
			SEXTEXT("	(global i -> x)")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestGlobalInt32_2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // add our output to the stack

		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int x = vm.PopInt32();

		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);

		validate(x == 4);
	}

	void TestGlobalInt32_3(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(global Int64 i = 6)")
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int64 x):")
			SEXTEXT("	(global i <- 4)")
			SEXTEXT("	(global i -> x)")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestGlobalInt32_3"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push((int64)0x3); // add our output to the stack

		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int64 x = vm.PopInt64();

		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);

		validate(x == 4);
	}

	void TestMacroAsArgument1(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(macro Sys.Seventeen in out (out.AddAtomic \"0x11\"))")

			SEXTEXT("(function PassThrough (Int32 x)->(Int32 y) : (y = x))")
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> (Int32 exitCode):")
			SEXTEXT("	(exitCode = (PassThrough (#Sys.Seventeen)))")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestMacroAsArgument1"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x00000001);
		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();

		int32 exitCode = vm.PopInt32();
		validate(exitCode == 17);

		validate(result == Sexy::EXECUTERESULT_TERMINATED);
	}

	void TestMultipleDerivation(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(interface Sys.IBase")
			SEXTEXT("	(A(Int64 i) -> )")
			SEXTEXT(")")

			SEXTEXT("(interface Sys.IDerived")
			SEXTEXT("	(extends Sys.IBase)")
			SEXTEXT(")")

			SEXTEXT("(class Impl")
			SEXTEXT("	(implements Sys.IDerived)")
			SEXTEXT(")")

			SEXTEXT("(method Impl.A(Int64 i) -> : )")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> :")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestMultipleDerivation"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);
	}

	void TestMultipleDerivation2(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(interface Sys.IBase")
			SEXTEXT("	(A(Int64 i) -> (Bool result))")
			SEXTEXT(")")

			SEXTEXT("(interface Sys.IDerived")
			SEXTEXT("	(extends Sys.IBase)")
			SEXTEXT(")")

			SEXTEXT("(class Impl")
			SEXTEXT("	(implements Sys.IDerived)")
			SEXTEXT(")")

			SEXTEXT("(method Impl.A(Int64 i) -> (Bool result): (result = 0))")

			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("(function Main -> :")
			SEXTEXT(")")
			SEXTEXT("(alias Main EntryPoint.Main)")
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestMultipleDerivation2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);
	}

	void TestClassDefinesInterface(IPublicScriptSystem& ss)
	{
		/*csexstr srcCode = This is the equivalent code we want to achieve
			SEXTEXT("(using Sys.Type)\n")
			SEXTEXT("(interface Sys.IDog\n")
			SEXTEXT("	(Woof -> (IString bark))\n")
			SEXTEXT(")\n")
			SEXTEXT("(class Dog (implements Sys.IDog)\n")
			SEXTEXT(")\n")
			SEXTEXT("(method Dog.Woof -> (IString bark): \n")
			SEXTEXT("	(bark = \"&nWoof&n&n\")\n")
			SEXTEXT(")\n")
			SEXTEXT("(method Dog.Construct : )\n")
			SEXTEXT("(factory Sys.Dog Sys.IDog  : (construct Dog))\n")
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> :\n")
			SEXTEXT("	(Sys.IDog dog(Sys.Dog))\n")
			SEXTEXT("	(Sys.Print dog.Woof)\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)\n");*/

		csexstr srcCode =
			SEXTEXT("(using Sys.Type)\n")
			SEXTEXT("(class Dog (defines Sys.IDog)\n")
			SEXTEXT(")\n")
			SEXTEXT("(method Dog.Woof -> (IString bark): \n")
			SEXTEXT("	(bark = \"&nWoof&n&n\")\n")
			SEXTEXT(")\n")
			SEXTEXT("(method Dog.Construct : )\n")
			SEXTEXT("(factory Sys.Dog Sys.IDog  : (construct Dog))\n")
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> :\n")
			SEXTEXT("	(Sys.IDog dog(Sys.Dog))\n")
			SEXTEXT("	(Sys.Print dog.Woof)\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)\n");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestClassDefinesInterface"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);
	}

	void TestClassExtendsInterface(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(using Sys.Type)\n")
			SEXTEXT("(interface Sys.IAnimal\n")
			SEXTEXT("	(Name -> (IString name))\n")
			SEXTEXT(")\n")
			SEXTEXT("(class Dog (defines Sys.IDog extends Sys.IAnimal)\n")
			SEXTEXT(")\n")
			SEXTEXT("(method Dog.Woof -> (IString bark): \n")
			SEXTEXT("	(bark = \"&nWoof&n&n\")\n")
			SEXTEXT(")\n")
			SEXTEXT("(method Dog.Name -> (IString name): \n")
			SEXTEXT("	(name = \"&nDigby&n&n\")\n")
			SEXTEXT(")\n")
			SEXTEXT("(method Dog.Construct : )\n")
			SEXTEXT("(factory Sys.Dog Sys.IDog  : (construct Dog))\n")
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("(function Main -> :\n")
			SEXTEXT("	(Sys.IDog dog(Sys.Dog))\n")
			SEXTEXT("	(Sys.Print dog.Woof)\n")
			SEXTEXT("	(Sys.Print dog.Name)\n")
			SEXTEXT(")\n")
			SEXTEXT("(alias Main EntryPoint.Main)\n");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestClassExtendsInterface"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		auto& sys = *ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("Sys"));
		auto idog = sys.FindInterface(SEXTEXT("IDog"));
		auto ianimal = sys.FindInterface(SEXTEXT("IAnimal"));

		validate(idog->Base() == ianimal);

		Sexy::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);
	}

	void TestInstancing(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("	(alias Main EntryPoint.Main)")
			SEXTEXT("	(function Main (Int32 x) -> (Int32 y):\n")
			SEXTEXT("		(y = (x + 1))")
			SEXTEXT("	)");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestInstancing"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		VM::CPU cpu2;
		AutoFree<VM::IVirtualMachine> vm2(vm.Clone(cpu2));
		
		vm2->Push(0x3);

		Sexy::EXECUTERESULT result = vm2->Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);

		int x1 = vm2->PopInt32();
		validate(x1 = 4);
	}

	void TestClosureArg(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)")
			SEXTEXT("	(alias Main EntryPoint.Main)")

			SEXTEXT("   (archetype Sys.GetInt32 -> (Int32 value))")

			SEXTEXT("	(function Eval (closure Sys.GetInt32 f) -> (Int32 value):")
			SEXTEXT("		(f -> value)")
			SEXTEXT("	)")

			SEXTEXT("	(function Main -> (Int32 y):\n")
			SEXTEXT("		(Sys.GetInt32 f = ")
			SEXTEXT("			(closure -> (Int32 y):")
			SEXTEXT("				(y = 77)")
			SEXTEXT("			)")
			SEXTEXT("		)")
			SEXTEXT("		(y = (Eval f))")
			SEXTEXT("	)");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestInstancing"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x00000001);
		auto result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Sexy::EXECUTERESULT_TERMINATED);

		int x1 = vm.PopInt32();
		validate(x1 = 77);
	}

	void TestBadClosureArg(IPublicScriptSystem& ss)
	{
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("	(alias Main EntryPoint.Main)\n")

			SEXTEXT("   (archetype Sys.GetInt32 -> (Int32 value))\n")

			SEXTEXT("	(function Eval (closure Sys.GetInt32 f) -> (Int32 value):\n")
			SEXTEXT("		(Sys.GetInt32 g = f)\n")	
			SEXTEXT("		(g -> value)\n")
			SEXTEXT("	)\n")

			SEXTEXT("	(function Main -> (Int32 y):\n")
			SEXTEXT("		(Sys.GetInt32 f = \n")
			SEXTEXT("			(closure -> (Int32 y):\n")
			SEXTEXT("				(y = 77)\n")
			SEXTEXT("			)\n")
			SEXTEXT("		)\n")
			SEXTEXT("		(y = (Eval f))\n")
			SEXTEXT("	)\n");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestBadClosureArg"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("	(alias Main EntryPoint.Main)\n")

			SEXTEXT("   (archetype Sys.GetInt32 -> (Int32 value))\n")

			SEXTEXT("	(function Eval (closure Sys.GetInt32 f) -> (Int32 value):\n")
			SEXTEXT("		(f -> value)\n")
			SEXTEXT("	)\n")

			SEXTEXT("	(function Main -> (Int32 y):\n")
			SEXTEXT("		(Sys.GetInt32 f = \n")
			SEXTEXT("			(closure -> (Int32 y):\n")
			SEXTEXT("				(y = 77)\n")
			SEXTEXT("			)\n")
			SEXTEXT("		)\n")
			SEXTEXT("		(Sys.GetInt32 e = f)\n")
			SEXTEXT("		(y = (Eval e))\n")
			SEXTEXT("	)\n");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestBadClosureArg2"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("	(alias Main EntryPoint.Main)\n")

			SEXTEXT("   (archetype Sys.GetInt32 -> (Int32 value))\n")

			SEXTEXT("	(function Main -> (Int32 y):\n")
			SEXTEXT("		(Sys.GetInt32 f = \n")
			SEXTEXT("			(closure -> (Int32 y):\n")
			SEXTEXT("				(y = 77)\n")
			SEXTEXT("			)\n")
			SEXTEXT("		)\n")
			SEXTEXT("		(array Sys.GetInt32 q 2)\n")
			SEXTEXT("		(q.Push f)\n")
			SEXTEXT("	)\n");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestBadClosureArg3"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("	(alias Main EntryPoint.Main)\n")

			SEXTEXT("(archetype Sys.GetInt32 -> (Int32 value))\n")

			SEXTEXT("(function Main -> (Int32 value):\n")
			SEXTEXT("	(Int32 i = 417)")
			SEXTEXT("	(Sys.GetInt32 f = \n")
			SEXTEXT("		(closure -> (Int32 y):\n")
			SEXTEXT("			(y = i)\n")
			SEXTEXT("		)\n")
			SEXTEXT("	)\n")
			SEXTEXT("	(value = (f))")
			SEXTEXT("")

			SEXTEXT(")\n");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestClosureCapture"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("	(alias Main EntryPoint.Main)\n")

			SEXTEXT("(archetype Sys.GetInt32 -> (Int32 value))\n")

			SEXTEXT("(function Call (closure Sys.GetInt32 f) -> (Int32 result): (result = (f)) )")

			SEXTEXT("(function Main -> (Int32 value):\n")
			SEXTEXT("	(Int32 i = 2417)")
			SEXTEXT("	(Sys.GetInt32 f = \n")
			SEXTEXT("		(closure -> (Int32 y):\n")
			SEXTEXT("			(y = i)\n")
			SEXTEXT("		)\n")
			SEXTEXT("	)\n")
			SEXTEXT("	(Call f -> value)")
			SEXTEXT("")

			SEXTEXT(")\n");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestClosureCapture2"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("	(alias Main EntryPoint.Main)\n")

			SEXTEXT("(archetype Sys.GetInt32 -> (Int32 value))\n")

			SEXTEXT("(function Main -> (Int32 y):\n")
			SEXTEXT("	(Sys.GetInt32 f = \n")
			SEXTEXT("		(closure -> (Int32 y):\n")
			SEXTEXT("			(y = 77)\n")
			SEXTEXT("		)\n")
			SEXTEXT("	)\n")
			SEXTEXT("	(Sys.GetInt32 g = \n")
			SEXTEXT("		(closure -> (Int32 y):\n")
			SEXTEXT("			(y = 77)\n")
			SEXTEXT("		)\n")
			SEXTEXT("	)\n")
			SEXTEXT("	(f = g)\n")
			SEXTEXT("")

			SEXTEXT(")\n");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestBadClosureArg7"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("	(alias Main EntryPoint.Main)\n")

			SEXTEXT("(archetype Sys.GetInt32 -> (Int32 value))\n")

			SEXTEXT("(function GetSix -> (Int32 value):\n")
			SEXTEXT("	(value = 6)\n")
			SEXTEXT(")\n")

			SEXTEXT("(function Main -> (Int32 y):\n")
			SEXTEXT("	(Sys.GetInt32 f = \n")
			SEXTEXT("		(closure -> (Int32 y):\n")
			SEXTEXT("			(y = 77)\n")
			SEXTEXT("		)\n")
			SEXTEXT("	)\n")
			SEXTEXT("	(map Int32 Sys.GetInt32 q)\n")
			SEXTEXT("	(q.Insert 777 f)\n")
			SEXTEXT("")

			SEXTEXT(")\n");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestBadClosureArg6"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("	(alias Main EntryPoint.Main)\n")

			SEXTEXT("(archetype Sys.GetInt32 -> (Int32 value))\n")

			SEXTEXT("(function GetSix -> (Int32 value):\n")
			SEXTEXT("	(value = 6)\n")
			SEXTEXT(")\n")

			SEXTEXT("(function Main -> (Int32 y):\n")
			SEXTEXT("	(Sys.GetInt32 f = \n")
			SEXTEXT("		(closure -> (Int32 y):\n")
			SEXTEXT("			(y = 77)\n")
			SEXTEXT("		)\n")
			SEXTEXT("	)\n")
			SEXTEXT("	(list Sys.GetInt32 q)\n")
			SEXTEXT("	(q.Append GetSix)\n")
			SEXTEXT("	(node head = q.Tail)\n")
			SEXTEXT("	(head.Prepend f)\n")
			SEXTEXT("")
			
			SEXTEXT(")\n");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestBadClosureArg5"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint)\n")
			SEXTEXT("	(alias Main EntryPoint.Main)\n")

			SEXTEXT("   (archetype Sys.GetInt32 -> (Int32 value))\n")

			SEXTEXT("	(function Main -> (Int32 y):\n")
			SEXTEXT("		(Sys.GetInt32 f = \n")
			SEXTEXT("			(closure -> (Int32 y):\n")
			SEXTEXT("				(y = 77)\n")
			SEXTEXT("			)\n")
			SEXTEXT("		)\n")
			SEXTEXT("		(list Sys.GetInt32 q)\n")
			SEXTEXT("		(q.Prepend f)\n")
			SEXTEXT("	)\n");

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {1,1}, SEXTEXT("TestBadClosureArg4"));
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
		csexstr srcCode =
			SEXTEXT("(namespace EntryPoint) \n")
			SEXTEXT("(function Main -> (Int32 result): \n")
			SEXTEXT("		(Call 0 -> result)\n")
			SEXTEXT(")\n")
			SEXTEXT("(function Call (EntryPoint.ToInt fn) -> (Int32 result):  (fn -> result))\n")
			SEXTEXT("(archetype EntryPoint.ToInt -> (Int32 y)) \n")
			SEXTEXT("(alias Main EntryPoint.Main) \n");
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, SEXTEXT("TestNullArchetype"));
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
      csexstr srcCode =
         SEXTEXT("(using Sys.Maths) \n")
         SEXTEXT("(namespace EntryPoint) \n")
         SEXTEXT("(function Main -> (Float32 cx)(Float32 cy)(Float32 cz): \n")
         SEXTEXT("		(Vec3 a = 2 4 6)\n")
         SEXTEXT("		(Vec3 b = 5 7 9)\n")
         SEXTEXT("		(Vec3 c = a + b)\n")
         SEXTEXT("      (cx = c.x)\n")
         SEXTEXT("      (cy = c.y)\n")
         SEXTEXT("      (cz = c.z)\n")
         SEXTEXT(")\n")
         SEXTEXT("(alias Main EntryPoint.Main) \n");
      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestOperatorOverload"));
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

   void TestOperatorOverload2(IPublicScriptSystem& ss)
   {
      csexstr srcCode =
         SEXTEXT("(using Sys.Maths) \n")
         SEXTEXT("(namespace EntryPoint) \n")
         SEXTEXT("(function Main -> (Float32 cx)(Float32 cy)(Float32 cz): \n")
         SEXTEXT("		(Vec3 a = 2 4 6)\n")
         SEXTEXT("		(Vec3 b = 5 7 9)\n")
         SEXTEXT("		(Vec3 c = (a - b))\n")
         SEXTEXT("      (cx = c.x)\n")
         SEXTEXT("      (cy = c.y)\n")
         SEXTEXT("      (cz = c.z)\n")
         SEXTEXT(")\n")
         SEXTEXT("(alias Main EntryPoint.Main) \n");
      Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("TestOperatorOverload2"));
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

	void RunCollectionTests()
	{
		TEST(TestArrayInt32);
		TEST(TestArrayInt32_2);
		TEST(TestArrayInt32_3);
		TEST(TestArrayInt32_4);
		TEST(TestArrayInt32_5);
		TEST(TestArrayInt32_6);
		TEST(TestArrayInt32_7);
		TEST(TestArrayInt32_8);
		TEST(TestArrayInt32_9);
		TEST(TestArrayFloat64);
		TEST(TestArrayStruct);
		TEST(TestArrayStruct_2);
		TEST(TestArrayStruct_3);

		TEST(TestArrayStrongTyping);
		TEST(TestArrayStrongTyping2);
		TEST(TestArrayStrongTyping3);

		TEST(TestConstructInArray);
		TEST(TestArrayForeachOnce);
		TEST(TestArrayForeachWithinForEach);
		TEST(TestArrayForeachAndThrow);
		TEST(TestArrayForeachEachElementInArray);
		TEST(TestArrayForeachEachElementInArrayWithoutIndex);
		TEST(TestArrayElementDeconstruct);
		TEST(TestArrayElementDeconstructWhenThrown);
		TEST(TestArrayWithinArrayDeconstruct);
		TEST(TestArrayElementLockRef);

		TEST(TestLinkedList);
		TEST(TestLinkedList2);
		TEST(TestLinkedList3);
		TEST(TestLinkedList4);
		TEST(TestLinkedList6);
		TEST(TestLinkedList7);
		TEST(TestLinkedList8);
		TEST(TestLinkedList9);
		TEST(TestLinkedList10);
		TEST(TestLinkedList11);
		TEST(TestLinkedListForeach1);
		TEST(TestLinkedListForeach2);
		TEST(TestLinkedListForeach3);
		TEST(TestLinkedListForeach4);
		TEST(TestLinkedListForeach5);
		TEST(TestLinkedListForeach6);
		TEST(TestLinkedListForeach7);

		TEST(TestLinkedListOfLists);

		TEST(TestListStruct);
		TEST(TestListStruct2);

		TEST(TestListStrongTyping);

		TEST(TestMap);
		TEST(TestMap2);
		TEST(TestMap3);
		TEST(TestMap4);
		TEST(TestMap5);
		TEST(TestMap6);
		TEST(TestMap7);
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
	}

	void TestMaths()
	{
		TEST(TestLimitsInt32);
		TEST(TestLimitsInt64);
		TEST(TestLimitsFloat32);
		TEST(TestLimitsFloat64);
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
	}

	void RunPositiveSuccesses()
	{
		validate(true);

		TEST(TestNullArchetypeArg);
		TEST(TestMacroAsArgument1);

	//	TEST(TestArrayOfArchetypes); // -> currently disabled, since arrays are 32-bit and 64-bits only, and closures are 128 bits.
		TEST(TestMapOfArchetypes);
		TEST(TestLinkedListOfArchetypes);
		TEST(TestClosureCapture);
		TEST(TestClosureCapture2);
		TEST(TestArchetypePropagation);
		TEST(TestArchetypeReturn);
		TEST(TestArchetypeReturnFromMultipleOutput);
		TEST(TestEmbeddedArchetype);
		TEST(TestClosure);
		TEST(TestClosureWithVariable);
		TEST(TestReturnClosureWithVariableSucceed);	
		TEST(TestNullArchetype);
		TEST(TestArchetypeCall);

		TEST(TestRefTypesInsideClosure);

		TEST(TestBadClosureArg7);
		TEST(TestBadClosureArg6);
		TEST(TestBadClosureArg5);
		TEST(TestBadClosureArg4);
		TEST(TestBadClosureArg3);
		TEST(TestBadClosureArg);
		TEST(TestBadClosureArg2);

		TEST(TestClosureArg);

		TEST(TestDefaultNullMethod);

		TEST(TestStringSplit);
		TEST(TestSearchSubstring);
		TEST(TestRightSearchSubstring);
		TEST(TestSetCase);

		TEST(TestGlobalInt32);
		TEST(TestGlobalInt32_2);
		TEST(TestGlobalInt32_3);

		TestMaths();

		TEST(TestRaw);
		TestConstruction();
		TEST(TestMinimumConstruct);
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
		TEST(TestLocalVariable);
		TEST(TestBooleanLiteralVsLiteralMatches);
		TEST(TestBooleanMismatch);
		TEST(TestBooleanVariableVsLiteralMatches);
		TEST(TestBooleanVariableVsVariable);
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
		
		TEST(TestFunctionCallRecursion1);
		TEST(TestFunctionCallRecursion2);
		TEST(TestFunctionCallMultiOutput1);
		TEST(TestStructure);
		TEST(TestStructure2);
		TEST(TestStructure3);
		TEST(TestStructure4);
		
		TEST(TestWhileLoop1);
		TEST(TestNestedWhileLoops);
		TEST(TestWhileLoopBreak);
		TEST(TestWhileLoopContinue);
		TEST(TestDoWhile);
		TEST(TestDoWhileBreak);
		TEST(TestDoWhileContinue);	
		TEST(TestArchetypeDeclaration);

		TEST(TestNativeCall);
		TEST(TestNativeCall2);
		TEST(TestNativeCall3);
		TEST(TestInterfaceDefinition);			
	
		TEST(TestConstructor);
		TEST(TestDestructor);
		TEST(TestNullObject);		
		TEST(TestClassDefinition);
		TEST(TestClassInstance);
		TEST(TestDynamicCast);
		TEST(TestDerivedInterfaces);
		TEST(TestDerivedInterfaces2);

		TEST(TestVirtualFromVirtual);
				
		TEST(TestCatch);	
		TEST(TestCatchArg);	
				
		TEST(TestTryFinallyWithoutThrow);
		TEST(TestDeepCatch);
		TEST(TestExceptionDestruct);
		TEST(TestThrowFromCatch);
		
		TEST(TestSizeOf);
		TEST(TestCatchInstanceArg);
		TEST(TestTryWithoutThrow);
		TEST(TestInterfacePropagation);	
		TEST(TestInstancePropagation);
		TEST(TestInstanceMemberPropagation);
		TEST(TestMultipleInterfaces);

		TEST(TestNullMember);
		TEST(TestNullMemberInit);
		TEST(TestNullRefInit);

		TEST(TestMemoString);		
		TEST(TestMemoString2);
		TEST(TestStringConstant);
		TEST(TestPrint);		
		TEST(TestPrintViaInstance);
		TEST(TestNullString);

		TEST(TestInlinedFactory);
		
		TEST(TestRecti1);
		TEST(TestRecti2);
		TEST(TestRecti3);
		TEST(TestRecti4);

		TEST(TestExpressionArg);
		TEST(TestSubstitution);

		TEST(TestMacro);

		TEST(TestReflectionGetCurrentExpression);
		TEST(TestReflectionGetParent);
		TEST(TestReflectionGetChild_BadIndex);					
		TEST(TestReflectionGetChild);
		TEST(TestReflectionGetAtomic);

		TEST(TestModuleCount);
		TEST(TestPrintModules);
		TEST(TestPrintStructs);
		
		TEST(TestSysThrow);
		TEST(TestSysThrow2);

		TEST(TestStringBuilder);
		TEST(TestStringBuilderBig);

		TEST(TestSearchSubstring);
		TEST(TestRightSearchSubstring);
		TEST(TestAppendSubstring);
		TEST(TestStringbuilderTruncate);

		TEST(TestMallocAligned);
		TEST(TestGetSysMessage);
		
		TEST(TestInternalDestructorsCalled);
		TEST(TestInternalDestructorsCalled2);

		TEST(TestTypedef);

		TEST(TestReturnInterface);

		TEST(TestMemberwiseInit);

		TEST(TestStructStringPassByRef);

		TEST(TestMeshStruct4);
		TEST(TestMeshStruct3);
		TEST(TestMeshStruct2);
		TEST(TestMeshStruct);
		TEST(TestEmptyString);

		TEST(TestYield);
		TEST(TestStructByRefAssign);
		TEST(TestAssignVectorComponentsFromParameters);
		TEST(TestConstructFromInterface);

		TEST(TestAssignPointerFromFunction);

		TEST(TestClassDefinesInterface);
		TEST(TestClassExtendsInterface);

		TEST(TestMultipleDerivation2);
		TEST(TestMultipleDerivation);

		// TEST(TestInstancing); // Disabled until we have total compilation. JIT requires a PC change
	}

	void RunPositiveFailures()
	{		
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
	}
	
	void RunTests()
	{	
      TEST(TestOperatorOverload);
 // -> TODO     TEST(TestOperatorOverload2);
      TEST(TestLinkedList11);
		RunPositiveSuccesses();	
		RunCollectionTests();
		RunPositiveFailures(); 
	}
}

int main(int argc, char* argv[])
{
   Sexy::OS::SetBreakPoints(Sexy::OS::BreakFlag_All);
	RunTests();
	return 0;
}


