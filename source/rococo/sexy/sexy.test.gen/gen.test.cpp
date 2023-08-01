#include "sexy.types.h"
#include "sexy.strings.h"

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

#include <sexy.lib.script.h>
#include <rococo.api.h>


using namespace Rococo;

namespace Sys
{
   struct IZoo;
}

#include "..\sexy.bennyhill.test\tests\generated\sb.test.sxh.h"

Sys::Animals::ITiger* FactoryConstructSysAnimalsGetTigerByName(Sys::Animals::ITiger* _context, const fstring& _tigerName)
{
   return _context;
}

void ShowFailure(const char* expression, const char* filename, int lineNumber)
{
   printf("Validation failed in %s[%d]: %s\r\n", filename, lineNumber, expression);
}

#define validate(_Expression) if (!(_Expression)) { ShowFailure(#_Expression, __FILE__, __LINE__); Abort(); }

#include "..\sexy.bennyhill.test\tests\generated\sb.test.sxh.inl"

void Abort()
{
   if (Rococo::OS::IsDebugging())
      Rococo::OS::TripDebugger();
   else
      exit(-1);
}

struct CLogger : public ILog
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
      return (int32)exceptions.size();
   }

   void Write(cstr text)
   {
      WriteToStandardOutput(("%s"), text);
   }

   void OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance)
   {
      ParseException ex(Vec2i{ 0,0 }, Vec2i{ 0,0 }, exceptionType, message, (""), NULL);
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

void PrintExpression(cr_sex s, int &totalOutput, int maxOutput)
{
   switch (s.Type())
   {
   case EXPRESSION_TYPE_ATOMIC:
      totalOutput += WriteToStandardOutput((" %s"), (cstr)s.c_str());
      break;
   case EXPRESSION_TYPE_NULL:
      totalOutput += WriteToStandardOutput(("()"));
      break;
   case EXPRESSION_TYPE_STRING_LITERAL:
      totalOutput += WriteToStandardOutput((" \"%s\""), (cstr)s.c_str());
      break;
   case EXPRESSION_TYPE_COMPOUND:

      totalOutput += WriteToStandardOutput((" ("));

      for (int i = 0; i < s.NumberOfElements(); ++i)
      {
         if (totalOutput > maxOutput)
         {
            return;
         }

         cr_sex child = s.GetElement(i);
         PrintExpression(child, totalOutput, maxOutput);
      }

      totalOutput += WriteToStandardOutput((" )"));
   }
}


void PrintParseException(const ParseException& e)
{
   WriteToStandardOutput(("Parse error\r\nSource: %s\r\nExpression: (%d,%d) to (%d,%d)\r\nReason: %s\r\n"), e.Name(), e.Start().x, e.Start().y, e.End().x, e.End().y, e.Message());

   int depth = 0;
   for (const ISExpression* s = e.Source(); s != NULL; s = s->GetOriginal())
   {
      if (depth++ > 0)  WriteToStandardOutput("Macro expansion %d:\r\n", depth);

      int totalOutput = 0;
      PrintExpression(*s, totalOutput, 1024);

      if (totalOutput > 1024) WriteToStandardOutput(("..."));

      WriteToStandardOutput(("\r\n"));
   }
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

void ValidateLogs()
{
   if (s_logger.ExceptionCount() > 0)
   {
      ParseException ex;
      while (s_logger.TryGetNextException(ex))
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

int main(int argc, char* argv[])
{
   Auto<ISourceCode> headerCode;
   Auto<ISourceCode> mainCode;
   Auto<ISParserTree> headerTree;
   Auto<ISParserTree> mainTree;

   class TigerPup: public Sys::Animals::ITigerPup 
   {
      virtual void AppendName(Rococo::IStringPopulator& builder)
      {
         builder.Populate("Geoff");
      }
   };

   class TigerBoss: public Sys::Animals::ITiger
   {
      TigerPup pup;

      virtual Sys::Animals::ITigerPup* MakeBabies()
      {
         return &pup;
      }
   } tiger;

   try
   {
      ProgramInitParameters pip;
      pip.MaxProgramBytes = 32768;
	  AutoFree<IAllocatorSupervisor> allocator = Rococo::Memory::CreateBlockAllocator(16, 0);
      CScriptSystemProxy ssp(pip, s_logger, *allocator);
      validate(&ssp() != NULL);
      validate(&ssp().PublicProgramObject() != NULL);

      auto& ss = ssp();

      headerCode = ss.SParser().LoadSource("..\\sexy.bennyhill.test\\tests\\generated\\sb.test.sxh.sxy", Vec2i{ 0,0 });
      headerTree = ss.SParser().CreateTree(headerCode());

      cstr srcCode =
         "(namespace EntryPoint)"
         "(using Sys.Animals)"
         "(using Sys.Type)"
         "(function Main (Int32 id) -> (Int32 exitCode):"
         "   (ITiger boss (GetTigerByName \"Aslan\"))"
         "   (ITigerPup pup = boss.MakeBabies)"
         "   (IStringBuilder sb (StringBuilder 256))"
         "   (pup.AppendName sb)"
         " )"
         "(alias Main EntryPoint.Main)";

      mainCode = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, ("main"));
      mainTree = ss.SParser().CreateTree(mainCode());

      ss.AddTree(headerTree());
      ss.AddTree(mainTree());

      Sys::Animals::AddNativeCalls_SysAnimalsITiger(ss, &tiger);
      Sys::Animals::AddNativeCalls_SysAnimalsITigerPup(ss, nullptr);

      ss.Compile();

      const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(("EntryPoint"));
      validate(ns != NULL);
      validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, ("Main")));

      auto& vm = ss.PublicProgramObject().VirtualMachine();

      vm.Push(1); // Allocate stack space for the int32 x
      EXECUTERESULT result = vm.Execute(VM::ExecutionFlags(false, true));
      ValidateExecution(result);
   }
   catch (ParseException& ex)
   {
      PrintParseException(ex);
      exit(-1);
   }
}