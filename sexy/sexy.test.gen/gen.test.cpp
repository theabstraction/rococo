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

#ifdef SEXCHAR_IS_WIDE
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

   void Write(csexstr text)
   {
      WriteToStandardOutput(SEXTEXT("%s"), text);
   }

   void OnUnhandledException(int errorCode, csexstr exceptionType, csexstr message, void* exceptionInstance)
   {
      ParseException ex(Vec2i{ 0,0 }, Vec2i{ 0,0 }, exceptionType, message, SEXTEXT(""), NULL);
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
      totalOutput += WriteToStandardOutput(SEXTEXT(" %s"), (csexstr)s.String()->Buffer);
      break;
   case EXPRESSION_TYPE_NULL:
      totalOutput += WriteToStandardOutput(SEXTEXT("()"));
      break;
   case EXPRESSION_TYPE_STRING_LITERAL:
      totalOutput += WriteToStandardOutput(SEXTEXT(" \"%s\""), (csexstr)s.String()->Buffer);
      break;
   case EXPRESSION_TYPE_COMPOUND:

      totalOutput += WriteToStandardOutput(SEXTEXT(" ("));

      for (int i = 0; i < s.NumberOfElements(); ++i)
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
      CScriptSystemProxy ssp(pip, s_logger);
      validate(&ssp() != NULL);
      validate(&ssp().PublicProgramObject() != NULL);

      auto& ss = ssp();

      headerCode = ss.SParser().LoadSource("..\\sexy.bennyhill.test\\tests\\generated\\sb.test.sxh.sxy", Vec2i{ 0,0 });
      headerTree = ss.SParser().CreateTree(headerCode());

      csexstr srcCode =
         SEXTEXT("(namespace EntryPoint)")
         SEXTEXT("(using Sys.Animals)")
         SEXTEXT("(using Sys.Type)")
         SEXTEXT("(function Main (Int32 id) -> (Int32 exitCode):")
         SEXTEXT("   (ITiger boss (GetTigerByName \"Aslan\"))")
         SEXTEXT("   (ITigerPup pup = boss.MakeBabies)")
         SEXTEXT("   (IStringBuilder sb (StringBuilder 256))")
         SEXTEXT("   (pup.AppendName sb)")
         SEXTEXT(" )")
         SEXTEXT("(alias Main EntryPoint.Main)");

      mainCode = ss.SParser().ProxySourceBuffer(srcCode, -1, Vec2i{ 0,0 }, SEXTEXT("main"));
      mainTree = ss.SParser().CreateTree(mainCode());

      ss.AddTree(headerTree());
      ss.AddTree(mainTree());

      Sys::Animals::AddNativeCalls_SysAnimalsITiger(ss, &tiger);
      Sys::Animals::AddNativeCalls_SysAnimalsITigerPup(ss, nullptr);

      ss.Compile();

      const INamespace* ns = ss.PublicProgramObject().GetRootNamespace().FindSubspace(SEXTEXT("EntryPoint"));
      validate(ns != NULL);
      validate(SetProgramAndEntryPoint(ss.PublicProgramObject(), *ns, SEXTEXT("Main")));

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