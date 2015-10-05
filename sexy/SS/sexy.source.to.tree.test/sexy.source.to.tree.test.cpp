#include "sexy.source.to.tree.stdafx.h"

#include <rococo.win32.target.win7.h>

#ifdef SEXCHAR_IS_WIDE
# define _UNICODE
# define UNICODE
#endif

#include "Sexy.Types.h"

#include <stdio.h>
#include <tchar.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <intrin.h>
#include <memory.h>
#include <vector>

#include "sexy.lib.util.h"
#include "Sexy.S-Parser.h"
#include "sexy.lib.s-parser.h"

using namespace Sexy;
using namespace Sexy::Sex;

void PrintExpression(cr_sex s, int &totalOutput, int maxOutput)
{
	switch (s.Type())
	{
	case EXPRESSION_TYPE_ATOMIC:
		totalOutput += PrintToStandardOutput(SEXTEXT(" %s"), (csexstr) s.String()->Buffer);
		break;
	case EXPRESSION_TYPE_STRING_LITERAL:
		totalOutput += PrintToStandardOutput(SEXTEXT(" \"%s\""), (csexstr) s.String()->Buffer);
		break;
	case EXPRESSION_TYPE_COMPOUND:
			
		totalOutput += PrintToStandardOutput(SEXTEXT(" ("));

		for(int i = 0; i < s.NumberOfElements(); ++i)
		{
			if (totalOutput > maxOutput) 
			{
				return;
			}
				
			cr_sex child = s.GetElement(i);
			PrintExpression(child, totalOutput, maxOutput);								
		}
			
		totalOutput += PrintToStandardOutput(SEXTEXT(" )"));
	}				
}

void PrintParseException(const ParseException& e)
{
	PrintToStandardOutput(SEXTEXT("Parse error\r\nSource: %s\r\nExpression: (%d,%d) to (%d,%d)\r\nReason: %s\r\n"), e.Name(), e.Start().X, e.Start().Y, e.End().X, e.End().Y, e.Message());

	for (const ISExpression* s = e.Source(); s != NULL; s = s->GetOriginal())
	{
		if (s->TransformDepth() > 0)  PrintToStandardOutput(SEXTEXT("Macro expansion %d:\r\n"), s->TransformDepth());

		int totalOutput = 0;
		PrintExpression(*s, totalOutput, 1024);

		if (totalOutput > 1024) PrintToStandardOutput(SEXTEXT("..."));

		PrintToStandardOutput(SEXTEXT("\r\n"));
	}
}

void FormatSysMessage(SEXCHAR* text, size_t capacity, int msgNumber)
{
	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, msgNumber, 0, text, (DWORD) capacity, NULL))
	{
		StringPrint(text, capacity, SEXTEXT("Code %d ( 0x%x )"), msgNumber, msgNumber);
	}
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
		SEXCHAR osMessage[256];
		FormatSysMessage(osMessage, 256, ose.ErrorCode());
		PrintToStandardOutput(SEXTEXT("OS Error. %s\r\n%s\r\n"), ose.Message(), osMessage);
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
	csexstr filepath = SEXTEXT("\\Dev\\sexiest\\content\\models\\level 2\\architect354.model.sx");
	
	Auto<ISourceCode> source = ss.LoadSource(filepath, SourcePos(0,0));

	PrintDuration("loading ascii as unicode image", GetTicks() - now);
	now = GetTicks();

	Auto<ISParserTree> tree = ss.CreateTree(source());

	PrintDuration("source to tree", GetTicks() - now);
}