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
#include <rococo.sexy.api.h>

#include <stdio.h>

#ifdef _WIN32
# include <intrin.h>
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

#include "sexy.lib.util.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexy.script.h"
#include "sexy.s-parser.h"
#include "sexy.compiler.h"
#include <rococo.api.h>
#include <sexy.dispatch.inl>
#include <rococo.package.h>
#include <rococo.os.h>

#define validate(_Expression) if (!(_Expression)) { ShowFailure(#_Expression, __FILE__, __LINE__); Abort(); }

#define TEST(test) Test(#test, test, false)
#define TEST2(test) Test(#test, test, true)

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Script;
using namespace Rococo::Compiler;

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
			WriteToStandardOutput("%s", text);
		}

		void OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance) 
		{
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
		printf("Validation failed in %s[%d]: %s\r\n", filename, lineNumber, expression);
	}

	void PrintExpression(cr_sex s, int &totalOutput, int maxOutput)
	{
		switch (s.Type())
		{
		case EXPRESSION_TYPE_ATOMIC:
			totalOutput += WriteToStandardOutput(" %s", (cstr) s.String()->Buffer);
			break;
      case EXPRESSION_TYPE_NULL:
         totalOutput += WriteToStandardOutput("()");
         break;
		case EXPRESSION_TYPE_STRING_LITERAL:
			totalOutput += WriteToStandardOutput(" \"%s\"", (cstr) s.String()->Buffer);
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
		WriteToStandardOutput("\r\nParse error:\r\nSource: %s\r\nExpression: (%d,%d) to (%d,%d)\r\nReason: %s\r\n", e.Name(), e.Start().x, e.Start().y, e.End().x, e.End().y, e.Message());

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

	void Test(const char* name, FN_TEST fnTest, bool addinCoroutines)
	{
		printf("<<<<<< %s\r\n", name);
		
		ProgramInitParameters pip;
		pip.addCoroutineLib = addinCoroutines;
		pip.useDebugLibs = addinCoroutines;
		pip.MaxProgramBytes = 32768;

#ifdef _DEBUG
		pip.useDebugLibs = true;
#endif
		IAllocator& allocator = Rococo::Memory::CheckedAllocator();
		CScriptSystemProxy ssp(pip, s_logger, allocator);
		
      auto& ss = ssp();
		if (!IsPointerValid(&ss)) exit(-1);

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
            char osMessage[256];
            FormatSysMessage(osMessage, 256, ose.ErrorCode());
            WriteToStandardOutput("Error code%d~%x,%s\r\n%s\r\n", ose.ErrorCode(), ose.ErrorCode(), ose.Message(), osMessage);
         }
         else
         {
            WriteToStandardOutput("%s", ose.Message());
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

	void TestConstruction(IPublicScriptSystem& ss)
	{
		try
		{
			ProgramInitParameters pip;
			pip.MaxProgramBytes = 32768;
#ifdef _DEBUG
			pip.useDebugLibs = true;
#endif
			IAllocator& allocator = Rococo::Memory::CheckedAllocator();
			CScriptSystemProxy ssp(pip, s_logger, allocator);
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
		Auto<ISourceCode> sc(ss.SParser().ProxySourceBuffer("(namespace Sys.Data)", -1, Vec2i{ 0,0 },"test2"));
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		ss.AddTree(tree());
		ss.Compile();
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());
	   vm.Push(99); // Allocate stack space for the int32 exitcode
	   vm.Push(101); // Allocate stack space for the int32 id

	   auto result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);

	   int32 id = vm.PopInt32();
	   int32 exitCode = vm.PopInt32();
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanVariableVsVariable");
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestBooleanExpressions");
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(1); // Allocate stack space for the int32 x
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int32 x = vm.PopInt32();
		validate(x == 42);
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestWhileLoopBreak");
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestWhileLoopContinue");
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDoWhile");
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDoWhileBreak");
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
		validate(&decl->GetArgument(0) == &decl->GetArgument(1) && decl->GetArgument(0).VarType() == VARTYPE_Float32);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
			ss.AddNativeCall(ns, ANON::CpuHz, NULL,"CpuHz -> (Int64 hz)");
			ss.AddNativeCall(ns, ANON::CpuTime, NULL,"CpuTime -> (Int64 count)");
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestNativeCall");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x1000000000000000LL); // Allocate stack space for the int64 result
		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);
		int64 x = vm.PopInt64();
		validate(170 == x);
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
			ss.AddNativeCall(ns, ANON::Square, NULL,"Square (Int32 x)-> (Int32 y)");
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
			ss.AddNativeCall(ns, ANON::George, NULL,"George (Int32 x)(Int32 y)-> (Int32 pxy)(Int32 dxy)");
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

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		const IInterface* i = ns->FindInterface("IPlayer");
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
		validate(a1.GetArgument(0).VarType() == VARTYPE_Int32);
		validate(a2.GetArgument(0).VarType() == VARTYPE_Int32);
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

		const IModule& m = ss.PublicProgramObject().GetModule(ss.GetIntrinsicModuleCount());
		const IStructure* s = m.FindStructure("Player");

		validate(s != NULL);
		validate(s->InterfaceCount() == 1);
		const IInterface& i1 = s->GetInterface(0);
		validate(&i1 != NULL);

		const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace("EntryPoint");
		const IInterface* i2 = ns->FindInterface("IPlayer");

		validate(i2 != NULL);
		validate(&i1 == i2);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestNullObject");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());	

      AutoFree<VM::IDisassembler> dis ( vm.Core().CreateDisassembler() );

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
			validate(false);
		}
		catch (ParseException& e)
		{
			validate(AreEqual(e.Message(),"Duplicate structure definition for Vector3"));
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
			"		("
			"			try ( result = 15)"   
			"   catch e ( result = (result + 7) )"
			"   finally ( result = (result - 5) )"
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
			"(function Main -> (Int32 result):"
			"   (Sys.IRobot robby (Sys.NewRobot))"
			"   (result = 7)"
			")"
			"(alias Main EntryPoint.Main)"

			"(class Robot (defines Sys.IRobot))"
			"(method Robot.Destruct -> : (Sys.InvokeTest))"
			"(method Robot.Null -> : )"
			"(method Robot.Construct -> : )"
			"(factory Sys.NewRobot Sys.IRobot : (construct Robot))"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestDestructor");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		g_destructorTestInt = 0;

		struct ANON {	static void InvokeTest(NativeCallEnvironment& e)
		{				
				g_destructorTestInt = 1;
		}};

		const INamespace& nsSys = ss.AddNativeNamespace("Sys");

		try
		{
			ss.AddNativeCall(nsSys, ANON::InvokeTest, NULL,"InvokeTest ->");
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
			g_destructorTestInt = 1;
		}};

		const INamespace& nsSys = ss.AddNativeNamespace("Sys");

		try
		{
			ss.AddNativeCall(nsSys, ANON::InvokeTest, NULL,"InvokeTest ->");
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMultipleInterfaces");
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestStringConstant");
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestPrint");
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
		cstr srcCode =
			"(namespace EntryPoint)"
			"  (alias Main EntryPoint.Main)"
			"(using Sys.Type)"

			"(function Main -> (Int32 result):"
			"  (IString message = \"Bilbo Baggins\")"
			"  (Sys.Print message -> result)"
			")"		
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestPrintViaInstance");
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
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"
			""
			"(using Sys.Type)"
			""
			"(function Main -> (Int32 result):"
			"  (IStringBuilder sb = NewPathBuilder)"
			"  (#build sb \"Hello World\")"
			"  (IString text = sb)"
			"  (Sys.Print text -> result)"
			")"
			
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
			"   (#for (Int32 i = 0) (i < moduleCount) (i = (i + 1))"
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

			"   (#for (Int32 i = 0)  (i < moduleCount)  (#inc i)"
			"			(IModule module = (ss.Module i))"

			"			(#for (Int32 j = 0) (j < module.StructCount) (#inc j)"
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
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
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
		")"

		"(function Main -> (Int32 result):"
		"	(Int32 i = 7)"
		"	(result = (#square i))"
		")"		
					;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMacro");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 49);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 0);
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
			"	(#for  (Int32 i = 0)   (i < s.ChildCount)  (i += 1)" 
			"       (IExpression child = (s i))"
			"		(child.AppendTextTo sb)"
			"   )"
			"   (Sys.Print sb -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Maths)"
		"(using Sys.Reflection)"
  
		"(function Main -> (Int32 result):"
		"	(result = 7)"
		"	(#EntryPoint.double result)"
		")"

		"(macro EntryPoint.double in out"
		"	(IExpression s = '($1 = ($1 * 2)) )"
		"	(out.Substitute in s)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestSubstitution");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());		

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

		int x = vm.PopInt32();
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		"	(IString musical = \"The tapdancing tapper\")"
		"	(IString instrument = \"tap\")"
		"	(result = (Strings.FindRightWithCase musical ((musical.Length) - 1) instrument))" // the substring occurs at position 10 in the containing string
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestSearchSubstring");
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

	void TestMallocAligned(IPublicScriptSystem& ss)
	{
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(IBuffer b(AllocAligned 256 1))"
		"	(b.Capacity -> result)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestMallocAligned");
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
		cstr srcCode =
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"
		"	(array Int32 a 4)"
		"	(result = a.Capacity)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInt32");
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
		"	(try"
		"		("
		"			(a.Push 5)"
		"		)"
		"	catch ex"
		"		("
		"			(Sys.Print ex.Message)"
		"		)"
		"	)"
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
		"	(a 1 1470)"
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

	void TestArrayPushOverflow(IPublicScriptSystem& ss)
	{
		cstr srcCode =
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(struct Haha (Float64 a b c d))"

			"(function Main -> (Int32 result):"
			"	(array Haha laughs 2)"
			"	(Haha haha = 1 2 3 4)"
			"	(laughs.Push haha)"
			"	(laughs.Push haha)"
			"	(laughs.Push haha)"
			"	(result = laughs.Length)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },  __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		validate(result == EXECUTERESULT_THROWN);
		s_logger.Clear();
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		"	(a 3 2492)"
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
		"	(Vec3i w)"
		"	(w = (a 0))"
		"	(result = w.z)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayStruct_2");
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
		"	(a 3 v)"
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
		"	(a 3 7)"
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		"(method Sanctuary.Construct (Int32 maxKennels)"
		"	-> (construct kennels maxKennels): )"		
  
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

	void TestArrayForeachOnce(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct DogKennel (array Int32 dogIds))"
		"(struct Sanctuary (array DogKennel kennels))"

		"(method DogKennel.Construct (Int32 capacity)"
		"	-> (construct dogIds capacity) "	
		"	:"
		")"

		"(method Sanctuary.Construct (Int32 kennelCapacity)"
		"	-> (construct kennels kennelCapacity)"		
		"	:"
		")"
  
		"(function Main -> (Int32 result):"
		"	(Sanctuary sanctuary (4) )" // Create a sanctuary with a capacity of 4 kennels
		"	(sanctuary.kennels.Push 4)" // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		"	(foreach i k # (sanctuary.kennels 0 0)" // Get a ref to the first kennel
		"		(k.dogIds.Push 1812)" // Put a new dog id in the first kennel
		"		(result = (k.dogIds 0))" // Return the first dog id
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayInStruct4");
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int32 x = vm.PopInt32();
		validate(x == 4);
	}

	void TestArrayForeachWithinForEach(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"

		"(struct DogKennel (array Int32 dogIds))"
		"(struct Sanctuary (array DogKennel kennels))"

		"(method DogKennel.Construct (Int32 capacity)"
		"	-> (construct dogIds capacity) "	
		"	:"
		")"

		"(method Sanctuary.Construct (Int32 kennelCapacity)"
		"	-> (construct kennels kennelCapacity)"		
		"	:"
		")"
  
		"(function Main -> (Int32 result):"
		"	(Sanctuary sanctuary (4) )" // Create a sanctuary with a capacity of 4 kennels
		"	(sanctuary.kennels.Push 4)" // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		"	(sanctuary.kennels.Push 4)" // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		"	(sanctuary.kennels.Push 4)" // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		"	(sanctuary.kennels.Push 4)" // Push/construct a new kennel in the sanctuary using 4 as argument to constructor
		"	(foreach i k # (sanctuary.kennels 0 3)" // Enumerate over all 4 kennesl
		"		(k.dogIds.Push i)" // Put a new dog id in the kennel
		"		(k.dogIds.Push i)" // Put a new dog id in the kennel
		"		(k.dogIds.Push i)" // Put a new dog id in the kennel
		"		(k.dogIds.Push i)" // Put a new dog id in the kennel
		"		(foreach j d # (k.dogIds 0 3)" // Enumerate through kennel
		"			(result = (result + d))" // Sum the dogid to result
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
		validate(x == 24);
	}

	void TestArrayForeachAndThrow(IPublicScriptSystem& ss)
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

		"	(try"
		"		("
		"			(foreach i k # (sanctuary.kennelIds 0 9)" // Enumerate over all 10 kennels
		"				(Sys.Throw -1 \"Test: foreach throw\")"
		"				(result = (result + i))" // Sum the index to result
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

	void TestArrayForeachEachElementInArray(IPublicScriptSystem& ss)
	{
		cstr srcCode = 
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
 
		"(function Main -> (Int32 result):"
		"	(array Int32 a (10) )"
		"	(#for (Int32 i = 0) (i < 10) (#inc i)"
		"		(a.Push (i + 10))"
		"	)"

		"	(foreach i k # a"
		"		(result = (result + k))"
		"	)"		
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestArrayForeachEachElementInArray");
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
		"	(#for (Int32 i = 0) (i < 10) (#inc i)"
		"		(a.Push (i + 10))"
		"	)"

		"	(foreach k # a"
		"		(result = (result + k))"
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		"(method Axis.Construct (Int32 testsPerAxis) -> (construct tests testsPerAxis):)\n"
 
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

		"(struct Stuff (list Int32 a))"

		"(method Stuff.Construct -> (construct a()) : )"
  
		"(function Main -> (Int32 result):"
		"	(Stuff stuff())"
		"	(stuff.a.Append 17)"
		"	(stuff.a.Prepend 34)"
		"	(node tail = stuff.a.Tail)"
		"	(result = (result + tail.Value))"
		"	(foreach n # stuff.a (result = (result + n.Value)))"
		"	(result = (result + stuff.a.Length))"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedList11");
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
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(class Cat (defines Sys.ICat))"
			"(method Cat.Construct : )"
			"(method Cat.Id -> (Int32 id): (id = 5))"
			"(factory Sys.NewCat Sys.ICat : (construct Cat))"

			"(function Main -> (Int32 result):"
			"	(Sys.ICat cat (Sys.NewCat))"
			"	(list Sys.ICat cats)"
			"	(cats.Append cat)"
			"   (node tail = cats.Tail)"
			"   (Sys.ICat tiddles = tail.Value)"
			"   (tiddles.Id -> result)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		"(method Dog.Construct -> (construct boneIds ()) : )"
  
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
		"(namespace EntryPoint)"
		" (alias Main EntryPoint.Main)"
  
		"(using Sys.Type)"
  
		"(function Main -> (Int32 result):"

		"	(list Int32 a)"

		"	(a.Append 17)"
		"	(a.Append 34)"
		"	(a.Append 333)"

		"	(foreach n # a "
		"		(try"
		"			("
		"				(Sys.Throw n.Value \"thrown in foreach \")"
		"			)"
		"		catch ex"
		"			("
		"				(result = (result + 1))"
		"				(Sys.Print ex.Message)"
		"			)"
		"		)"
		"		(result = (result + 10))"
		"	)"
		")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 },"TestLinkedListForeach6");
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
			"	(a.Append 34)"
			"	(a.Append 333)"

			"	(foreach n # a"
			"		(result = n.Value)"
			"		(break)"
			"	)"	
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
			"(namespace EntryPoint)"
			" (alias Main EntryPoint.Main)"

			"(using Sys.Type)"

			"(interface Sys.I1 (GetI2 -> (Sys.I2 i2)))"
			"(interface Sys.I2 (Get6 -> (Int32 value)))"

			"(class Apple (implements Sys.I1)(implements Sys.I2))"
			"(method Apple.Construct : )"
			"(factory Sys.NewApple Sys.I1 : (construct Apple))"
			"(method Apple.GetI2 -> (Sys.I2 i2): (i2 = this))"
			"(method Apple.Get6 -> (Int32 value): (value = 6))"

			"(function Main -> (Int32 result):"
			"	(Sys.I1 apple (Sys.NewApple))"
			"	(Sys.I2 i2 = apple.GetI2)"
			"	(result = i2.Get6)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0); // Allocate stack space for the int32 result

		EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateExecution(result);

		int x = vm.PopInt32();
		validate(x == 5);
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
			"	(Sys.ITest value = & n)"
			"	(result = value.Id)"
			")";

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		"(method Vec4Map.Construct -> (construct a () ) : )"

		"(struct Vec4 (Int32 x y z w))"
		"(method Vec4.Construct (Int32 x) (Int32 y) (Int32 z) -> : (this.x = x) (this.y = y) (this.z = z) (this.w = 1))" 
  
		"(function Main -> (Int32 result):"
		"	(Vec4Map m () )"
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
		"(method Vec4Map.Construct -> (construct a () ) : )"

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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		ss.AddNativeCall(ns, ANON::GetPointer, NULL,"GetPointer (Int32 id) -> (Pointer pointer)", false);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0}, __FUNCTION__);
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
		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		const IModule& m = ss.PublicProgramObject().GetModule(0);

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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		int x = vm.PopInt32();
		validate(x == 4);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
			"(method World.Construct -> (construct entities 2) : )"
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // add our output to the stack

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int x = vm.PopInt32();

		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		validate(x == 3);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x3); // add our output to the stack

		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));

		int x = vm.PopInt32();

		ValidateLogs();
		validate(result == Rococo::EXECUTERESULT_TERMINATED);

		validate(x == 11);
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
			"(macro Sys.Seventeen in out (out.AddAtomic \"0x11\"))"

			"(function PassThrough (Int32 x)->(Int32 y) : (y = x))"
			"(namespace EntryPoint)"
			"(function Main -> (Int32 exitCode):"
			"	(Int32 i = 6)"
			"	(Int32 j = (i | 3))"	
			"   (exitCode = j)"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, "TestMacroAssignBinary");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

		vm.Push(0x00000001);
		Rococo::EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
		ValidateLogs();

		int32 exitCode = vm.PopInt32();
		validate(exitCode == 7);

		validate(result == Rococo::EXECUTERESULT_TERMINATED);
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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		cstr srcCode =
			"(macro Sys.Seventeen in out (out.AddAtomic \"0x11\"))"

			"(function PassThrough (Int32 x)->(Int32 y) : (y = x))"
			"(namespace EntryPoint)"
			"(function Main -> (Int32 exitCode):"
			"	(exitCode = (PassThrough (#Sys.Seventeen)))"
			")"
			"(alias Main EntryPoint.Main)"
			;

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i {0,0},"TestMacroAsArgument1");
		Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

		VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

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

		Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		validate(x1 = 77);
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
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(1); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
	   ValidateExecution(result);
	   int32 x = vm.PopInt32();
	   validate(x == 7);
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
		   "    (Sys.Native.DispatchMessage handler)"
		   ")";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   auto& ns = ss.AddNativeNamespace("Sys.Native");

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

	   ss.AddNativeCall(ns, MessageDispatcher::Dispatch, nullptr, "DispatchMessage (Sys.DispatchEventHandler handler) -> ", true);

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

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

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, __FUNCTION__);
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
		   "	(Sys.Native.PublishAPI)\n"
		   ")\n"
		   "(alias Main EntryPoint.Main) \n";

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __FUNCTION__);
	   Auto<ISParserTree> tree(ss.SParser().CreateTree(sc()));

	   VM::IVirtualMachine& vm = StandardTestInit(ss, tree());

	   vm.Push(77); // Allocate stack space for the int32 x
	   EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
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

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

	   Auto<ISourceCode> sc = ss.SParser().ProxySourceBuffer(src, -1, Vec2i{ 0,0 }, __FUNCTION__);
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

   void RunCollectionTests()
   {
	   TEST(TestMap);
	   TEST(TestMap2);
	   TEST(TestMap3);
	   TEST(TestMap4);
	   TEST(TestMap5);
	   TEST(TestMap6);
	   TEST(TestMap7);
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

	   TEST(TestArrayElementIsClass);
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
	   TEST(TestLinkedListOfInterfaces);
	   TEST(TestLinkedListOfInterfaces2);
	   TEST(TestLinkedListForeach1);
	   TEST(TestLinkedListForeach2);
	   TEST(TestLinkedListForeach3);
	   TEST(TestLinkedListForeach4);
	   TEST(TestLinkedListForeach5);
	   TEST(TestLinkedListForeach6);
	   TEST(TestLinkedListForeach7);
	   TEST(TestLinkedListForeach8);
	   TEST(TestLinkedListOfArchetypes);
	   TEST(TestLinkedListOfLists);

	   TEST(TestListStruct);
	   TEST(TestListStruct2);
	   TEST(TestListStruct3);

	   TEST(TestListStrongTyping);
   }

   void TestMaths()
   {
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

	void RunPositiveSuccesses()
	{
		validate(true);

		TEST(TestPackage);

		TEST(TestUseDefaultNamespace);
		TEST(TestUseDefaultNamespace2);

		TEST(TestStringSplit);

		TEST(TestSerialize);

		TEST(TestEmptyArrayOfInterfaces);
		TEST(TestArrayOfInterfacesBuilder);

		TEST(TestClamp1);
		TEST(TestClamp2);
		TEST(TestClamp3);

		TEST(TestArrayClear);

		TEST(TestSearchSubstring);
		TEST(TestRightSearchSubstring);
		TEST(TestSetCase);

		TEST(TestStringToChar);
		TEST(TestStringToCharFail);
		TEST(TestCallPrivateMethodviaDispatch);
		TEST(TestCallPrivateMethod);
		TEST(TestCallPrivateMethod2);

		TEST(TestPublishAPI);
		TEST(TestStaticCast1);
		TEST(TestDynamicCast2);
		TEST(TestExpressionAppendTo);

		TEST2(TestCoroutine1);

		TEST(TestStringBuilderLength1);

		TEST(TestDynamicDispatch);
		TEST(TestPushSecondInterface);
		TEST(TestMapValueInterface);
		TEST(TestStructArgFromStructArg);
		TEST(TestStringArray);
		TEST(TestCompareInterfaces);
		TEST(TestListReverseEnumeration);
		TEST(TestZeroDefaults);
		TEST(TestMemberwiseConstructWithInterface);
		TEST(TestStructInClassSetFromMethod);
		TEST(TestStaticCast);
		TEST(TestCompareStruct);
		TEST(TestInterfaceForNull);
		TEST(TestFactoryReturnsBaseInterface);
		TEST(TestLoopBreak);
		TEST(TestBadClosureArg);
		TEST(TestNullArchetypeArg);
		TEST(TestBadClosureArg7);
		TEST(TestBadClosureArg6);
		TEST(TestBadClosureArg5);
		TEST(TestBadClosureArg4);
		TEST(TestBadClosureArg3);
		TEST(TestBadClosureArg2);

		TEST(TestMemberwiseInit);

		TEST(TestCPPCallback);

		TEST(TestMethodFromClosure);

		TEST(TestCPPCallback);

		TEST(TestDeltaOperators);
		TEST(TestDeltaOperators2);
		TEST(TestDeltaOperators3);
		TEST(TestDeltaOperators4);

		TEST(TestArrayRefMember);

		TEST(TestArrayWithEarlyReturn);
		TEST(TestArrayWithEarlyReturn2);
		TEST(TestAssignStringToStruct);
		TEST(TestCaptureStruct);
		TEST(TestConstructFromInterface);
		TEST(TestArrayForeachOnce);
		TEST(TestAddRefWithLocalVariable);
		TEST(TestGetSysMessage);
		TEST(TestNullMemberInit);
		TEST(TestConstructor);
		TEST(TestAssignDerivatives);
		TEST(TestBitwiseOr);
		TEST(TestOperatorOverload2);
		TEST(TestStructWithVec4f);
		TEST(TestBooleanCompareVarToCompound);
		TEST(TestAssignDerivativeFromRef);
		TEST(TestMemberwise2);
		TEST(TestNullObject);
		TEST(TestNullArchetype);
		TEST(TestOperatorOverload3);
		TEST(TestOperatorOverload);

		//	TEST(TestArrayOfArchetypes); // -> currently disabled, since arrays are 32-bit and 64-bits only, and closures are 128 bits.
		TEST(TestClosureCapture);
		TEST(TestClosureCapture2);
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

		TEST(TestRaw);
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
		TEST(TestClassDefinition);

		TEST(TestNullMember);
		TEST(TestNullRefInit);

		TEST(TestRecti1);
		TEST(TestRecti2);
		TEST(TestRecti3);
		TEST(TestRecti4);

		TEST(TestMallocAligned);

		TEST(TestTypedef);

		TEST(TestStructStringPassByRef);

		TEST(TestEmptyString);

		TEST(TestYield);
		TEST(TestStructByRefAssign);
		TEST(TestAssignVectorComponentsFromParameters);

		TEST(TestAssignPointerFromFunction);

		TEST(TestClassExtendsInterface);

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
		TEST(TestStringConstant);
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

		TEST(TestMemberwiseInit);

		TEST(TestInternalDestructorsCalled);
		TEST(TestInternalDestructorsCalled2);

		TEST(TestTryFinallyWithoutThrow);
		TEST(TestDeepCatch);

		TEST(TestDynamicCast);
		TEST(TestExceptionDestruct);

		TEST(TestDerivedInterfaces2);

		TEST(TestCatch);
		TEST(TestCatchArg);

		TEST(TestStructWithInterface);

		TEST(TestThrowFromCatch);

		TEST(TestCatchInstanceArg);
		TEST(TestTryWithoutThrow);

		TEST(TestReturnInterfaceEx);

		TEST(TestSysThrow);
		TEST(TestSysThrow2);

		TEST(TestSearchSubstring);
		TEST(TestRightSearchSubstring);
		TEST(TestAppendSubstring);
		TEST(TestStringbuilderTruncate);

		TEST(TestReflectionGetChild_BadIndex);

		TEST(TestEssentialInterface);

		TestMaths();

		TEST(TestStringBuilderBig);

		TEST(TestRefTypesInsideClosure);

		TEST(TestMeshStruct4);
		TEST(TestMeshStruct3);
		TEST(TestMeshStruct2);
		TEST(TestMeshStruct);

		TEST(TestPrintModules);

		TEST(TestModuleCount);

		TEST(TestPrintStructs);

		TEST(TestMacro);

		TEST(TestExpressionArg);
		TEST(TestReflectionGetParent);
		TEST(TestReflectionGetChild);
		TEST(TestReflectionGetAtomic);

		TEST(TestMacroAsArgument1);
		TEST(TestSubstitution);
		TEST(TestReflectionGetCurrentExpression);
	}

	void RunPositiveFailures()
	{
		TEST(TestThrowInConstructor);
		TEST(TestArrayPushOverflow);
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

	void RunTests()
	{
		int64 start, end, hz;
		start = OS::CpuTicks();

		TEST(TestClosureAndThis);

		RunPositiveSuccesses();
		RunPositiveFailures();	
		RunCollectionTests();

		end = OS::CpuTicks();
		hz = OS::CpuHz();

		double dt = (double)(end - start) / (double)hz;
		printf("\nAll tests completed in %.2f seconds\n", dt);
	}
}

int main(int argc, char* argv[])
{
	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);

	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	try
	{
		RunTests();
	}
	catch (IException& ex)
	{
		printf("Unhandled exception:\n%s", ex.Message());
	}
	return 0;
}
