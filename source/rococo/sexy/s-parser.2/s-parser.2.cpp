#include <rococo.api.h>
#include <rococo.os.h>
#include <rococo.strings.h>
#include <rococo.time.h>

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <memory>

#ifdef _WIN32
#include <malloc.h>
#else
#define <stdlib.h>
#endif

#include <sexy.types.h>

#include <new>

#include <filesystem>

#include <sexy.s-parser.h>

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>

using namespace Rococo;
using namespace Rococo::Strings;

// Experimental s-parser. Motivation - be able to iterate through an s source file using functions without allocating heap mempry

using namespace Rococo::Sex;

void validate(bool condition, cstr function, int line)
{
	if (!condition) Throw(0, "Condition false: %s %d", function, line);
}

#define VALIDATE(x) validate(x, __FUNCTION__, __LINE__);

void GenerateTestSFile(cstr filename, int rootChildren)
{
	FILE* fp = fopen(filename, "wb");
	if (fp != nullptr)
	{
		for (int i = 0; i < rootChildren; ++i)
		{
			fprintf(fp, "%d (function baba (Float32 y) -> (Float32 x): (chip chap shot shop 1.224 \"ayup\"))\n", i);
		}

		fclose(fp);
	}
}

void TestGenerated()
{
	cstr filename = "tmp_generated.sxy";
//	GenerateTestSFile(filename, 300000);

	using namespace std::filesystem;

	path p = filename;
	auto len = file_size(p);

	AutoFree<IAllocatorSupervisor> allocator(Rococo::Memory::CreateBlockAllocator(16, 0));

	Auto<ISParser> sparser = Sexy_CreateSexParser_2_0(*allocator, 32768);

	WideFilePath u16filename;
	Assign(u16filename, filename);
	auto* s = sparser->LoadSource(u16filename, { 1,1 });

	auto start = Rococo::Time::TickCount();

	auto* tree = sparser->CreateTree(*s);

	tree->Release();

	auto now = Rococo::Time::TickCount();
	double dt = (now - start) / (double)Rococo::Time::TickHz();
	printf("SBlockAllocator performance:\n");
	printf("Cpu cost of processing %llu kb: %.3f seconds\n", len / 1024, dt);
	printf("Through put: %.0f MB/s\n\n", len / (1048576.0 * dt));

	s->Release();

//	_unlink(filename);
}

void T5()
{
	AutoFree<IAllocatorSupervisor> allocator(Rococo::Memory::CreateBlockAllocator(16, 0));

	ISParser* sparser = Sexy_CreateSexParser_2_0(*allocator, 32768);

	auto t1 = "\"&x20&&abcdef\" a b cd (qcumber)"_fstring;
	ISourceCode* src = sparser->DuplicateSourceBuffer(t1.buffer, t1.length, Vec2i{ 1,1 }, "t1");
	VALIDATE(Eq(src->Name(), "t1"));
	VALIDATE(src->SourceLength() == t1.length);
	VALIDATE(src->SourceStart() != t1.buffer);

	auto* tree = sparser->CreateTree(*src);

	VALIDATE(tree->Root().NumberOfElements() == 5);
	VALIDATE(Eq(tree->Root()[0].c_str(), " &abcdef"));
	VALIDATE(Eq(tree->Root()[1].c_str(), "a"));
	VALIDATE(Eq(tree->Root()[2].c_str(), "b"));
	VALIDATE(Eq(tree->Root()[3].c_str(), "cd"));
	auto& q = tree->Root()[4];
	VALIDATE(q.NumberOfElements() == 1);
	VALIDATE(Eq(q[0].c_str(), "qcumber"));

	VALIDATE(&tree->Source() == src);
	VALIDATE(0 == tree->Release());
	VALIDATE(0 == src->Release());
	VALIDATE(0 == sparser->Release());
}

int main()
{
	try
	{
		T5();
		TestGenerated();
	}
	catch (IException& ex)
	{
		printf("Exception code %d:\n %s\n", ex.ErrorCode(), ex.Message());
		return ex.ErrorCode();
	}

	return 0;
}

