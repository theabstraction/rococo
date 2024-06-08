#include <rococo.types.h>
#include <rococo.debugging.h>
#include <rococo.os.h>
#include <rococo.json.h>
#include <rococo.strings.h>
#include <stdio.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::JSon;

void validateEq(cstr a, cstr b, cstr function, int line)
{
	if (a == nullptr && b == nullptr)
	{
		return;
	}

	if (a != nullptr && b == nullptr)
	{
		Throw(0, "\t%s #%d: Validation failure. LHS(%s) != RHS(<null>)", function, line, a);
	}

	if (a == nullptr && b != nullptr)
	{
		Throw(0, "\t%s #%d: Validation failure. LHS(<null>) != RHS(%s)", function, line, b);
	}

	if (!Eq(a, b))
	{
		Throw(0, "\t%s #%d: Validation failure. LHS(%s) != RHS(%s)", function, line, a, b);
	}
}

void validateEq(int32 a, int32 b, cstr function, int line)
{
	if (a != b)
	{
		Throw(0, "\t%s #%d: Validation failure. LHS(%d) != RHS(%d)", function, line, a, b);
	}
}

void validateEq(float a, float b, cstr function, int line)
{
	if (a != b)
	{
		Throw(0, "\t%s #%d: Validation failure. LHS(%f) != RHS(%f)", function, line, a, b);
	}
}

void validateEq(bool a, bool b, cstr function, int line)
{
	if (a != b)
	{
		Throw(0, "\t%s #%d: Validation failure. LHS(%s) != RHS(%s)", function, line, a ? "true" : "false", b ? "true" : "false");
	}
}

#define ValidateEq(a, b) validateEq(a, b, __FUNCTION__, __LINE__);

#define TEST(f) { printf("%s...", #f); f(); printf("completed\n"); }

void ParseSimpleJson()
{
	cstr davidJson = R"(
{
	name : "David",
	age : 99,
	sex : "bloke",
	profession : "actor",
	skill : 8.7,
	active : false,
}
)";

	AutoFree<IJSonParserSupervisor> jp = CreateJSonParser();
	AutoFree<INameValueTreeSupervisor> tree = jp->Parse(davidJson);
	auto& root = tree->Root();

	ValidateEq(root["name"], "David");
	ValidateEq(root["sex"], "bloke");
	ValidateEq(root["profession"], "actor");

	bool wasFound;
	ValidateEq(root.TryGet("age", OUT wasFound, -1), 99);
	ValidateEq(root.TryGet("skill", OUT wasFound, 0.0f), 8.7f);
	ValidateEq(root.TryGet("active", OUT wasFound, true), false);
}

int MainProtected(int, char*[])
{
	TEST(ParseSimpleJson);
	return 0;
}

struct Formatter : Debugging::IStackFrameFormatter
{
	bool isEnough = false;
	void Format(const Debugging::StackFrame& sf) override
	{
		if (isEnough)
		{
			return;
		}

		if (sf.depth == 0)
		{
			return;
		}

		char buf[1024];
		Debugging::FormatStackFrame(buf, sizeof buf, sf.address);
		if (StartsWith(buf, "MainProtected"))
		{
			isEnough = true;
		}
		printf("SF #%d: %s\n", sf.depth, buf);
	}
};

int main(int argc, char* argv[])
{
	try
	{
		return MainProtected(argc, argv);
	}
	catch (IException& ex)
	{
		char err[256];
		OS::FormatErrorMessage(err, sizeof err, ex.ErrorCode());

		if (ex.ErrorCode() != 0)
		{
			printf("Error: %s.\nCode %d: %s\n", ex.Message(), ex.ErrorCode(), err);
		}
		else
		{
			printf("Error: %s.\nCode %d\n", ex.Message(), ex.ErrorCode());
		}

		auto* sf = ex.StackFrames();
		if (sf)
		{
			Formatter formatter;
			sf->FormatEachStackFrame(formatter);
		}

		return ex.ErrorCode() == 0 ? -1 : ex.ErrorCode();
	}
}
