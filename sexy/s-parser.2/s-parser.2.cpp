#include <rococo.api.h>
#include <rococo.strings.h>

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <memory>
#include <malloc.h>

#include <sexy.types.h>

#include <new>

#include <filesystem>

#include <sexy.s-parser.h>

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>

using namespace Rococo;

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

	using namespace std::experimental::filesystem;

	path p = filename;
	auto len = file_size(p);

	AutoFree<IAllocatorSupervisor> allocator(Rococo::Memory::CreateBlockAllocator(16, 0));

	Auto<ISParser> sparser = Sexy_CreateSexParser_2_0(*allocator, 32768);
	auto* s = sparser->LoadSource(filename, { 1,1 });

	auto start = Rococo::OS::CpuTicks();

	auto* tree = sparser->CreateTree(*s);

	tree->Release();

	auto now = Rococo::OS::CpuTicks();
	double dt = (now - start) / (double)Rococo::OS::CpuHz();
	printf("SBlockAllocator performance:\n");
	printf("Cpu cost of processing %llu kb: %.3f seconds\n", len / 1024, dt);
	printf("Through put: %.0f MB/s\n\n", len / (1048576.0 * dt));

	s->Release();

	Rococo::Sex::CSParserProxy ppp;
	Auto<ISourceCode> src2 = ppp->LoadSource(filename, Vec2i{ 1,1 });

	start = Rococo::OS::CpuTicks();
	ISParserTree* tree2 = ppp->CreateTree(*src2);
	tree2->Release();
	now = Rococo::OS::CpuTicks();

	dt = (now - start) / (double)Rococo::OS::CpuHz();
	printf("Old School performance:\n");
	printf("Cpu cost of processing %llu kb: %.3f seconds\n", len / 1024, dt);
	printf("Through put: %.0f MB/s\n\n", len / (1048576.0 * dt));

//	_unlink(filename);
}

void T5()
{
	AutoFree<IAllocatorSupervisor> allocator(Rococo::Memory::CreateBlockAllocator(16, 0));

	ISParser* sparser = Sexy_CreateSexParser_2_0(*allocator, 32768);

	auto t1 = "\"&x20abcdef\" a b cd (qcumber)"_fstring;
	ISourceCode* src = sparser->DuplicateSourceBuffer(t1.buffer, t1.length, Vec2i{ 1,1 }, "t1");
	VALIDATE(Eq(src->Name(), "t1"));
	VALIDATE(src->SourceLength() == t1.length);
	VALIDATE(src->SourceStart() != t1.buffer);

	auto* tree = sparser->CreateTree(*src);

	VALIDATE(tree->Root().NumberOfElements() == 5);
	VALIDATE(Eq(tree->Root()[0].String()->Buffer, " abcdef"));
	VALIDATE(Eq(tree->Root()[1].String()->Buffer, "a"));
	VALIDATE(Eq(tree->Root()[2].String()->Buffer, "b"));
	VALIDATE(Eq(tree->Root()[3].String()->Buffer, "cd"));
	auto& q = tree->Root()[4];
	VALIDATE(q.NumberOfElements() == 1);
	VALIDATE(Eq(q[0].String()->Buffer, "qcumber"));

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

