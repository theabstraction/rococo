// sexy.s-parser.test.cpp : Defines the entry point for the console application.
#include <rococo.os.win32.h>

#include "sexy.lib.s-parser.h"
#include "sexy.lib.util.h"

#include "sexy.types.h"
#include "sexy.strings.h"

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "sexy.s-parser.h"

#include <rococo.api.h>

#define VALIDATE(_Expression) if (!(_Expression)) { ShowFailure(#_Expression, __FILE__, __LINE__); Stop(); }
#define VALIDATE_EQUAL(_Expression1, _Expression2) if ((_Expression1) != (_Expression2)) { ShowFailureEq(_Expression1, _Expression2, __FILE__, __LINE__); Stop(); }

using namespace Rococo;
using namespace Rococo::Sex;

namespace
{
	struct Logger: public Rococo::ILog
	{
		virtual void Write(cstr text) 
		{
			WriteToStandardOutput(text);
		}

		void OnUnhandledException(int errorCode, cstr exceptionType, cstr message, void* exceptionInstance) 
		{
			WriteToStandardOutput(("%s: code %d\nMessage: %s\n"), exceptionType, errorCode, message);
		}

		void OnJITCompileException(Sex::ParseException& ex)
		{
		}
	} s_logger;

	void Stop()
	{
		if (Rococo::OS::IsDebugging())
		{
         Rococo::OS::TripDebugger();
		}
		else
		{
			exit(-1);
		}
	}

	void ShowFailure(const char* expression, const char* filename, int lineNumber)
	{
		printf("Validation failed in %s[%d]: %s\r\n", filename, lineNumber, expression);
	}

	void ShowFailureEq(int expression1, int expression2, const char* filename, int lineNumber)
	{
		printf("Validation failed in %s[%d]: %d != %d\r\n", filename, lineNumber, expression1, expression2);
	}

	void ShowFailureEq(size_t expression1, size_t expression2, const char* filename, int lineNumber)
	{
		printf("Validation failed in %s[%d]: %llu != %llu\r\n", filename, lineNumber, (uint64) expression1, (uint64) expression2);
	}

	#define WRAP(x) Wrap(#x, x);
	#define TEST(x) Test(#x, x);

	typedef void (*FN_TEST)();
	typedef void (*FN_TEST2)(ISParser& parser);

	void Wrap(const char* name, FN_TEST test)
	{
		printf("<<<<< %s\r\n", name);

		try
		{
			test();
		}
		catch (std::exception& e)
		{
			printf("Error %s\r\n", e.what());
			exit(-1);
		}

		printf("%s >>>>>\r\n\r\n", name);
	}

	void Test(const char* name, FN_TEST2 test)
	{
		printf("<<<<< %s\r\n", name);

		try
		{		
			Auto<ISParser> parser = Sexy_CreateSexParser_2_0(Rococo::Memory::CheckedAllocator());
			test(parser());
		}
		catch (std::exception& e)
		{
			printf("Error %s\r\n", e.what());
			exit(-1);
		}

		printf("%s >>>>>\r\n\r\n", name);
	}

	void TestIsTrue()
	{
		VALIDATE(true);
	}

	void TestGoodReferenceCounts()
	{
		ISParser* parser = Sexy_CreateSexParser_2_0(Rococo::Memory::CheckedAllocator());
		VALIDATE(parser != NULL);
		VALIDATE(parser->AddRef() == 2);
		VALIDATE(parser->Release() == 1);
		VALIDATE(parser->Release() == 0);
	}

	void TestCreateNullTreeWithSource(ISParser& parser)
	{
      ISourceCode* nullSrc = parser.DuplicateSourceBuffer(("dogs"), 5, Vec2i{ 3,4 }, ("null"));
		VALIDATE(nullSrc->AddRef() == 2);
		VALIDATE(nullSrc->Release() == 1);

		ISParserTree* tree = parser.CreateTree(*nullSrc);
		VALIDATE(nullSrc->Release() == 1);
		VALIDATE(nullSrc->AddRef() == 2);

		VALIDATE(AreEqual(tree->Source().Name(), ("null")));
		VALIDATE(tree->Source().Origin().x == 3);
		VALIDATE(tree->Source().Origin().y == 4);
		VALIDATE(tree->Source().SourceLength() == 5);
		VALIDATE(Compare(tree->Source().SourceStart(), ("dogs"))  == 0);

		VALIDATE(tree->AddRef() == 2);
		VALIDATE(tree->Release() == 1);

		VALIDATE(&tree->Parser() == &parser);

		VALIDATE(tree->Release() == 0);
		VALIDATE(nullSrc->Release() == 0);

      ISourceCode* nullSrc2 = parser.ProxySourceBuffer(("kid"), 4, Vec2i{ 7,9 }, ("spanner"));
		VALIDATE(nullSrc2->AddRef() == 2);
		VALIDATE(nullSrc2->Release() == 1);

		ISParserTree* tree2 = parser.CreateTree(*nullSrc2);
		VALIDATE(nullSrc2->Release() == 1);
		VALIDATE(nullSrc2->AddRef() == 2);

		VALIDATE(AreEqual(tree2->Source().Name(), ("spanner")));
		VALIDATE(tree2->Source().Origin().x == 7);
		VALIDATE(tree2->Source().Origin().y == 9);
		VALIDATE(tree2->Source().SourceLength() == 4);
		VALIDATE(Compare(tree2->Source().SourceStart(),("kid")) == 0);

		VALIDATE(tree2->AddRef() == 2);
		VALIDATE(tree2->Release() == 1);

		VALIDATE(&tree2->Parser() == &parser);

		VALIDATE(tree2->Release() == 0);
		VALIDATE(nullSrc2->Release() == 0);
	}

	void TestCreateTreeWithRootNode(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("()"),2,Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);
		VALIDATE(tree->Root().Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(tree->Root().NumberOfElements() == 1);
		const ISExpression& child = tree->Root().GetElement(0);
		VALIDATE(&child != NULL);
		VALIDATE(child.Type() == EXPRESSION_TYPE_NULL);
		VALIDATE(child.NumberOfElements() == 0);
		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}

	void TestCreateTreeWithNodeAndDatum(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("(tuppence)"),-1,Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);
		VALIDATE(tree->Root().Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(tree->Root().NumberOfElements() == 1);
		const ISExpression& child = tree->Root().GetElement(0);
		VALIDATE(&child != NULL);
		VALIDATE(child.Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(child.String() == NULL);
		VALIDATE(child.NumberOfElements() == 1);
		const ISExpression& leaf = child.GetElement(0);
		VALIDATE(leaf.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf.String()->Length == 8);
		VALIDATE(Compare(leaf.String(), ("tuppence")) == 0);
		VALIDATE(leaf.NumberOfElements() == 0);
		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}

	void TestCreateTreeWithNodeAndData(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("(tuppence pooh)"),-1,Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);
		VALIDATE(tree->Root().Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(tree->Root().NumberOfElements() == 1);

		const ISExpression& child = tree->Root().GetElement(0);
		VALIDATE(&child != NULL);
		VALIDATE(child.Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(child.String() == NULL);
		VALIDATE(child.NumberOfElements() == 2);

		const ISExpression& leaf1 = child.GetElement(0);

		VALIDATE(&leaf1 != NULL);
		VALIDATE(leaf1.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf1.String()->Length == 8);
		VALIDATE(Compare(leaf1.String(), ("tuppence")) == 0);
		VALIDATE(leaf1.NumberOfElements() == 0);

		const ISExpression& leaf2 = child.GetElement(1);
		VALIDATE(&leaf2 != NULL);
		VALIDATE(leaf2.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf2.String()->Length == 4);
		VALIDATE(Compare(leaf2.String(), ("pooh")) == 0);
		VALIDATE(leaf2.NumberOfElements() == 0);

		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}

	void TestCreateTreeWithNodesAndData(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("(tuppence (pooh poor))"),-1,Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);
		VALIDATE(tree->Root().Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(tree->Root().NumberOfElements() == 1);

		const ISExpression& child = tree->Root().GetElement(0);
		VALIDATE(&child != NULL);
		VALIDATE(child.Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(child.String() == NULL);
		VALIDATE(child.NumberOfElements() == 2);

		const ISExpression& leaf1 = child.GetElement(0);
		VALIDATE(&leaf1 != NULL);
		VALIDATE(leaf1.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf1.String()->Length == 8);
		VALIDATE(Compare(leaf1.String(), ("tuppence")) == 0);
		VALIDATE(leaf1.NumberOfElements() == 0);

		const ISExpression& leaf2 = child.GetElement(1);
		VALIDATE(&leaf2 != NULL);
		VALIDATE(leaf2.Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(leaf2.String() == NULL);
		VALIDATE(leaf2.NumberOfElements() == 2);

		const ISExpression& leaf3 = leaf2.GetElement(0);
		VALIDATE(&leaf3 != NULL);
		VALIDATE(leaf3.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf3.String()->Length == 4);
		VALIDATE(Compare(leaf3.String(), ("pooh")) == 0);
		VALIDATE(leaf3.NumberOfElements() == 0);

		const ISExpression& leaf4 = leaf2.GetElement(1);
		VALIDATE(&leaf4 != NULL);
		VALIDATE(leaf4.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf4.String()->Length == 4);
		VALIDATE(Compare(leaf4.String(), ("poor")) == 0);
		VALIDATE(leaf4.NumberOfElements() == 0);

		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}

	void TestCreateTreeWithComments(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("/* haha */ (tuppence /* hehe */ (pooh /*aye lad*/ poor)) // So bone it hurts"),-1,Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);
		VALIDATE(tree->Root().Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(tree->Root().NumberOfElements() == 1);

		const ISExpression& child = tree->Root().GetElement(0);
		VALIDATE(&child != NULL);
		VALIDATE(child.Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(child.String() == NULL);
		VALIDATE(child.NumberOfElements() == 2);

		const ISExpression& leaf1 = child.GetElement(0);
		VALIDATE(&leaf1 != NULL);
		VALIDATE(leaf1.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf1.String()->Length == 8);
		VALIDATE(Compare(leaf1.String(), ("tuppence")) == 0);
		VALIDATE(leaf1.NumberOfElements() == 0);

		const ISExpression& leaf2 = child.GetElement(1);
		VALIDATE(&leaf2 != NULL);
		VALIDATE(leaf2.Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(leaf2.String() == NULL);
		VALIDATE(leaf2.NumberOfElements() == 2);

		const ISExpression& leaf3 = leaf2.GetElement(0);
		VALIDATE(&leaf3 != NULL);
		VALIDATE(leaf3.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf3.String()->Length == 4);
		VALIDATE(Compare(leaf3.String(), ("pooh")) == 0);
		VALIDATE(leaf3.NumberOfElements() == 0);

		const ISExpression& leaf4 = leaf2.GetElement(1);
		VALIDATE(&leaf4 != NULL);
		VALIDATE(leaf4.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf4.String()->Length == 4);
		VALIDATE(Compare(leaf4.String(), ("poor")) == 0);
		VALIDATE(leaf4.NumberOfElements() == 0);

		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}

	void TestCreateTreeWithCommentsAndString(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("/* haha */ (tuppence /* hehe */ (pooh /*aye lad*/ poor) \"chavs\") // So bone it hurts"),-1,Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);
		VALIDATE(tree->Root().Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(tree->Root().NumberOfElements() == 1);

		const ISExpression& child = tree->Root().GetElement(0);
		VALIDATE(&child != NULL);
		VALIDATE(child.Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(child.String() == NULL);
		VALIDATE(child.NumberOfElements() == 3);

		const ISExpression& leaf1 = child.GetElement(0);
		VALIDATE(&leaf1 != NULL);
		VALIDATE(leaf1.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf1.String()->Length == 8);
		VALIDATE(Compare(leaf1.String(), ("tuppence")) == 0);
		VALIDATE(leaf1.NumberOfElements() == 0);

		const ISExpression& leaf2 = child.GetElement(1);
		VALIDATE(&leaf2 != NULL);
		VALIDATE(leaf2.Type() == EXPRESSION_TYPE_COMPOUND);
		VALIDATE(leaf2.String() == NULL);
		VALIDATE(leaf2.NumberOfElements() == 2);

		const ISExpression& leaf3 = leaf2.GetElement(0);
		VALIDATE(&leaf3 != NULL);
		VALIDATE(leaf3.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf3.String()->Length == 4);
		VALIDATE(Compare(leaf3.String(), ("pooh")) == 0);
		VALIDATE(leaf3.NumberOfElements() == 0);

		const ISExpression& leaf4 = leaf2.GetElement(1);
		VALIDATE(&leaf4 != NULL);
		VALIDATE(leaf4.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(leaf4.String()->Length == 4);
		VALIDATE(Compare(leaf4.String(), ("poor")) == 0);
		VALIDATE(leaf4.NumberOfElements() == 0);

		const ISExpression& leaf5 = child.GetElement(2);
		VALIDATE(&leaf5 != NULL);
		VALIDATE(leaf5.Type() == EXPRESSION_TYPE_STRING_LITERAL);
		VALIDATE(Compare(leaf5.String(), ("chavs")) == 0);
		VALIDATE(leaf5.NumberOfElements() == 0);

		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}

	void TestTreeWithErrorHandling1(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("(tuppence"),-1,Vec2i{ 0,0 }, ("nil"));

		try
		{
			ISParserTree* tree = parser.CreateTree(*src);
			VALIDATE(false);
		}
		catch(ParseException& e)
		{
			VALIDATE(e.Start().x == 0);
			VALIDATE(e.End().x == 8);
			VALIDATE(GetSubString(e.Message(), ("missing")) != NULL);	
		}

		VALIDATE(src->Release() == 0);
	}

	void TestTreeWithErrorHandling2(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("(tuppence))"),-1,Vec2i{ 0,0 }, ("nil"));

		try
		{
			ISParserTree* tree = parser.CreateTree(*src);
			VALIDATE(false);
		}
		catch(ParseException& e)
		{
			VALIDATE(e.Start().x == 0);
			VALIDATE(e.End().x == 10);
			VALIDATE(GetSubString(e.Message(), ("Too many")) != NULL);
		}

		VALIDATE(src->Release() == 0);
	}

	void TestTreeWithErrorHandling3(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("\"shadow"),-1,Vec2i{ 0,0 }, ("nil"));

		try
		{
			ISParserTree* tree = parser.CreateTree(*src);
			VALIDATE(false);
		}
		catch(ParseException& e)
		{
			VALIDATE(e.Start().x == 0);
			VALIDATE(e.End().x == 6);
			VALIDATE(GetSubString(e.Message(), ("Literal string")) != NULL);		
		}

		VALIDATE(src->Release() == 0);
	}

	void TestTreeWithErrorHandling4(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("/*shadow"),-1,Vec2i{ 0,0 }, ("nil"));

		try
		{
			ISParserTree* tree = parser.CreateTree(*src);
			VALIDATE(false);
		}
		catch(ParseException& e)
		{
			VALIDATE(e.Start().x == 0);
			VALIDATE(e.End().x == 7);
			VALIDATE(GetSubString(e.Message(), ("Paragraph")) != NULL);	
		}

		VALIDATE(src->Release() == 0);
	}

	void TestTreeWithErrorHandling5(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("(/"),-1,Vec2i{ 0,0 }, ("nil"));

		try
		{
			ISParserTree* tree = parser.CreateTree(*src);
			VALIDATE(false);
		}
		catch(ParseException& e)
		{
			VALIDATE(e.Start().x == 0);
			VALIDATE(e.End().x == 1);
			VALIDATE(GetSubString(e.Message(), ("missing")) != NULL);		
		}

		VALIDATE(src->Release() == 0);
	}

	void TestTreeAtomicAB(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("AB"),-1,Vec2i{ 0,0 }, ("nil"));

		ISParserTree* tree = parser.CreateTree(*src);

		VALIDATE(tree->Root().NumberOfElements() == 1);
		const ISExpression& child = tree->Root().GetElement(0);
		VALIDATE(child.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(child.String()->Length == 2);
		VALIDATE(Compare(child.String(), ("AB")) == 0);
		VALIDATE(child.Start().x == 0);
		VALIDATE(child.End().x == 1);

		tree->Release();

		VALIDATE(src->Release() == 0);
	}

	void TestTreeAtomicA(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("A"),-1,Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);

		VALIDATE(tree->Root().NumberOfElements() == 1);
		const ISExpression& child = tree->Root().GetElement(0);
		VALIDATE(child.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(child.String()->Length == 1);
		VALIDATE(Compare(child.String(), ("A")) == 0);

		tree->Release();

		VALIDATE(src->Release() == 0);
	}

	void TestTreeAtomicSlash(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("/"),-1,Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);

		VALIDATE(tree->Root().NumberOfElements() == 1);
		const ISExpression& child = tree->Root().GetElement(0);
		VALIDATE(child.Type() == EXPRESSION_TYPE_ATOMIC);
		VALIDATE(Compare(child.String(), ("/")) == 0);

		tree->Release();

		VALIDATE(src->Release() == 0);
	}

	void TestString1(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("\"boo&r&n&t&q&&&0&a&b&f&v?\""),-1,Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);

		VALIDATE(tree->Root().NumberOfElements() == 1);
		const ISExpression& child = tree->Root().GetElement(0);
		VALIDATE(child.Type() == EXPRESSION_TYPE_STRING_LITERAL);
		cstr result = child.String()->Buffer;
		VALIDATE(Compare(result, ("boo"), 3) == 0);
		VALIDATE(result[3] == '\r');
		VALIDATE(result[4] == '\n');
		VALIDATE(result[5] == '\t');
		VALIDATE(result[6] == '\"');
		VALIDATE(result[7] == '&');
		VALIDATE(result[8] == '\0');
		VALIDATE(result[9] == '\a');
		VALIDATE(result[10] == '\b');
		VALIDATE(result[11] == '\f');
		VALIDATE(result[12] == '\v');
		VALIDATE(result[13] == '?');

		tree->Release();

		VALIDATE(src->Release() == 0);
	}

	void TestBadString1(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("\"boo&r&K\""),-1,Vec2i{ 0,0 }, ("nil"));

		try
		{
			parser.CreateTree(*src);
		}
		catch (ParseException& e)
		{
			VALIDATE(e.End().x == 7);
			VALIDATE(GetSubString(e.Message(), ("Unrecognized")) != NULL);
		}
		
		VALIDATE(src->Release() == 0);
	}

	void TestBadString2(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("\"boo&r&x\""),-1,Vec2i{ 0,0 }, ("nil"));

		try
		{
			parser.CreateTree(*src);
		}
		catch (ParseException& e)
		{
			VALIDATE(e.End().x == 7);
			VALIDATE(GetSubString(e.Message(), ("trailing hexes")) != NULL);
		}

		VALIDATE(src->Release() == 0);
	}

	void TestBadString3(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("\"&xAB"),-1,Vec2i{ 0,0 }, ("nil"));

		try
		{
			parser.CreateTree(*src);
		}
		catch (ParseException& e)
		{
#ifdef char_IS_WIDE
			VALIDATE(e.End().x == 2);
			VALIDATE(GetSubString(e.Message(), ("trailing hexes")) != NULL);
#else
			VALIDATE(e.End().x == 2);
			VALIDATE(GetSubString(e.Message(), ("double quote")) != NULL);
#endif
			
		}

		VALIDATE(src->Release() == 0);
	}

	void TestString2(ISParser& parser)
	{
#ifdef char_IS_WIDE
		cstr srcString = ("\"&x0041\"");
#else
		cstr srcString = ("\"&x41\"");
#endif
		ISourceCode* src = parser.ProxySourceBuffer(srcString,-1,Vec2i{ 0,0 }, ("nil"));

		ISParserTree* tree = parser.CreateTree(*src);
		cr_sex element = tree->Root().GetElement(0);
		VALIDATE(element.Type() == EXPRESSION_TYPE_STRING_LITERAL);
		VALIDATE(element.String()->Length == 1);		
		VALIDATE(element.String()->Buffer[0] == (char) 'A');
		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}

	void TestStringOffsets1(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("a"), -1, Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);

		VALIDATE_EQUAL(tree->Root().NumberOfElements(), 1);

		cr_sex a = tree->Root().GetElement(0);

		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}

	void TestStringOffsets4(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("\t\n a \r\n"), -1, Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);

		VALIDATE_EQUAL(tree->Root().NumberOfElements(), 1);

		cr_sex a = tree->Root().GetElement(0);

		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}

	void TestStringOffsets2(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("()"), -1, Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);

		VALIDATE_EQUAL(tree->Root().NumberOfElements(), 1);

		cr_sex a = tree->Root().GetElement(0);

		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}

	void TestStringOffsets5(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer((" ( ) "), -1, Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);

		VALIDATE_EQUAL(tree->Root().NumberOfElements(), 1);

		cr_sex a = tree->Root().GetElement(0);

		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}

	void TestStringOffsets3(ISParser& parser)
	{
		ISourceCode* src = parser.ProxySourceBuffer(("(a) ((bc de) )"), -1, Vec2i{ 0,0 }, ("nil"));
		ISParserTree* tree = parser.CreateTree(*src);

		VALIDATE_EQUAL(tree->Root().NumberOfElements(), 2);

		cr_sex achildcontainer = tree->Root().GetElement(0);
		VALIDATE_EQUAL(achildcontainer.NumberOfElements(), 1);

		cr_sex aexpr = achildcontainer.GetElement(0);

		cr_sex bcdeContainerContainer = tree->Root().GetElement(1);

		cr_sex bcdeContainer = bcdeContainerContainer.GetElement(0);

		cr_sex bc = bcdeContainer.GetElement(0);

		VALIDATE(tree->Release() == 0);
		VALIDATE(src->Release() == 0);
	}
}

int main(int argc, char* argv[])
{
	TEST(TestStringOffsets1);
	TEST(TestStringOffsets2);
	TEST(TestStringOffsets3);
	TEST(TestStringOffsets4);
	TEST(TestStringOffsets5);
	TEST(TestTreeWithErrorHandling1);
	WRAP(TestIsTrue);
	WRAP(TestGoodReferenceCounts);
	TEST(TestCreateNullTreeWithSource);
	TEST(TestCreateTreeWithRootNode);
	TEST(TestCreateTreeWithNodeAndDatum);
	TEST(TestCreateTreeWithNodeAndData);
	TEST(TestCreateTreeWithNodesAndData);
	TEST(TestCreateTreeWithComments);
	TEST(TestCreateTreeWithCommentsAndString);
	TEST(TestTreeWithErrorHandling2);
	TEST(TestTreeWithErrorHandling3);
	TEST(TestTreeWithErrorHandling4);
	TEST(TestTreeWithErrorHandling5);
	TEST(TestTreeAtomicAB);
	TEST(TestTreeAtomicA);
	TEST(TestTreeAtomicSlash);	
	TEST(TestString1);
	TEST(TestBadString1);
	TEST(TestBadString2);
	TEST(TestBadString3);
	TEST(TestString2);
	
	return 0;
}

