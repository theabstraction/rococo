// parser.gen.cpp : Defines the entry point for the console application.
//

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>
#include <sexy.lib.script.h>

#include <rococo.api.h>
#include <rococo.strings.h>

#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>

#include <stdio.h>
#include <tchar.h>

#include <vector>

using namespace Rococo;
using namespace Sexy;
using namespace Sexy::Sex;

/*
bool operator == (cr_sex s, const wchar_t* atomicMatch)
{
   if (s.Type() == EXPRESSION_TYPE_ATOMIC || s.Type() == EXPRESSION_TYPE_STRING_LITERAL)
   {
      return wcscmp(s.String()->Buffer, atomicMatch) == 0;
   }

   return false;
}
*/

cr_sex ValidateDirective(cr_sex s, const wchar_t* sdirective)
{
   Sexy::Sex::AssertCompound(s);
   if (s[0] != sdirective)
   {
      wchar_t msg[1024];
      SafeFormat(msg, _TRUNCATE, L"Expecting %s", sdirective);
      Throw(s, msg);
   }

   return s;
}


void WriteCppFile(cr_sex s)
{
   if (s.NumberOfElements() < 1) Throw(s, L"Expecting at least one statement in the sexy file");
   
   Sexy::Sex::AssertCompound(s[0]);

   cr_sex cppInstance = ValidateDirective(s[0], L"cpp");
   cr_sex sClass = GetAtomicArg(cppInstance, 1);
   cr_sex sInstance = GetAtomicArg(cppInstance, 2);

   printf("namespace {\n    void _parseGen_ParseFile(%S& %S)\n    {\n", sClass.String()->Buffer, sInstance.String()->Buffer);

   printf("_to_be_completed");

   printf("}\n}");
}

int main()
{
   std::vector<wchar_t> buffer;
   buffer.reserve(8_kilobytes);

   wchar_t line[1024];

   while (_getws_s(line))
   {
      const wchar_t* p = line;
      while (*p != 0)
      {
         buffer.push_back(*p);
         p++;
      }

      buffer.push_back(L'\n');
   }

   buffer.push_back(0);

   const wchar_t* src = &buffer[0];

   printf("/*\n%S\n*/\n", src);

   CSParserProxy sparser;
   Auto<ISourceCode> source( sparser->ProxySourceBuffer(src, (int32) buffer.size(), SourcePos(1, 1), L"stdin") );
   Auto<ISParserTree> tree;

   try
   {
      tree = sparser->CreateTree(*source);
      WriteCppFile(tree->Root());
   }
   catch (Sexy::Sex::ParseException& ex)
   {
      wprintf(L"ParseException\n\nName: %s\nMessage: %s\nAt line %d, col %d\n", ex.Name(), ex.Message(), ex.Start().Y, ex.End().X);
   }
  
   return 0;
}

