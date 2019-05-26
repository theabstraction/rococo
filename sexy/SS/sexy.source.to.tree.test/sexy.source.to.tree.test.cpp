#include "sexy.source.to.tree.stdafx.h"

#include <rococo.win32.target.win7.h>

#ifdef char_IS_WIDE
# define _UNICODE
# define UNICODE
#endif

#include "Sexy.Types.h"
#include <rococo.strings.h>

#include <stdio.h>
#include <tchar.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <intrin.h>
#include <memory.h>
#include <vector>

#include "sexy.lib.util.h"
#include "sexy.s-parser.h"
#include "sexy.lib.s-parser.h"

using namespace Rococo;
using namespace Rococo::Sex;

void PrintExpression(cr_sex s, int &totalOutput, int maxOutput)
{
	switch (s.Type())
	{
	case EXPRESSION_TYPE_ATOMIC:
		totalOutput += WriteToStandardOutput((" %s"), (cstr) s.String()->Buffer);
		break;
	case EXPRESSION_TYPE_STRING_LITERAL:
		totalOutput += WriteToStandardOutput((" \"%s\""), (cstr) s.String()->Buffer);
		break;
	case EXPRESSION_TYPE_COMPOUND:
			
		totalOutput += WriteToStandardOutput((" ("));

		for(int i = 0; i < s.NumberOfElements(); ++i)
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

void FormatSysMessage(char* text, size_t capacity, int msgNumber)
{
#ifdef char_IS_WIDE
	if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, msgNumber, 0, text, (DWORD) capacity, NULL))
	{
		StringPrint(text, capacity, ("Code %d ( 0x%x )"), msgNumber, msgNumber);
	}
#else
   if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, msgNumber, 0, text, (DWORD)capacity, NULL))
   {
      Rococo::SafeFormat(text, capacity, ("Code %d ( 0x%x )"), msgNumber, msgNumber);
   }
#endif
}

void RunTest(ISParser& ss);

void RunTests()
{
	printf("<<<<<< Sexy Expression Tree Generation Performance Test\r\n");
		
	try
	{
		CSParserProxy spp;
		RunTest(spp());
	}
	catch (ParseException& e)
	{
		PrintParseException(e);
		exit(-1);
	}
	catch (IException& ose)
	{
		char osMessage[256];
		FormatSysMessage(osMessage, 256, ose.ErrorCode());
		WriteToStandardOutput(("OS Error. %s\r\n%s\r\n"), ose.Message(), osMessage);
		exit(-1);
	}
	catch(std::exception& stdex)
	{
		printf("std::exception: %s\r\n", stdex.what());
		exit(-1);
	}

	printf("Sexy Expression Tree Generation Performance Test >>>>>>\r\n\r\n");
}

int _tmain(int argc, _TCHAR* argv[])
{
	RunTests();
	return 0;
}

int64 GetTicks()
{
	LARGE_INTEGER i;
	QueryPerformanceCounter(&i);
	return i.QuadPart;
}

double GetDuration(int64 deltaTicks)
{
	LARGE_INTEGER hz;
	QueryPerformanceFrequency(&hz);
	return ((double) deltaTicks) / (double) hz.QuadPart;
}

void PrintDuration(const char* section, int64 deltaTicks)
{
	printf("%s: %g ms\n", section, 1000.0f * GetDuration(deltaTicks));
}

void RunTest(ISParser& ss)
{
	int64 now = GetTicks();
	cstr filepath = ("\\Dev\\sexiest\\content\\models\\level 2\\architect354.model.sx");
	
	Auto<ISourceCode> source = ss.LoadSource(filepath, Vec2i{ 0,0 });

	PrintDuration("loading ascii as unicode image", GetTicks() - now);
	now = GetTicks();

	Auto<ISParserTree> tree = ss.CreateTree(source());

	PrintDuration("source to tree", GetTicks() - now);
}