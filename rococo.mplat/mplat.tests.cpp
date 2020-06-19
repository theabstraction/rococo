#include <rococo.api.h>
#include <rococo.handles.h>

using namespace Rococo;

void Validate(bool x, cstr message) { if (!x) Throw(0, "%s", message); }
#define VALIDATE(x) Validate(x, #x);

void TestHandles()
{
	// HandleTable<int32, 0x0000000FFFFFFFFF, 36> t0("t0", 0, nullptr); -> should throw
	HandleTable<int32, 0x0000000FFFFFFFFF, 36> t1("t1", 2, nullptr);
	auto h0 = t1.CreateNew();
	auto h1 = t1.CreateNew();

	auto hFirst = t1.GetFirstHandle();
	VALIDATE(hFirst == h0);

	auto hNext = t1.GetNextHandle(hFirst);
	VALIDATE(hNext == h1);

	hNext = t1.GetNextHandle(hNext);
	VALIDATE(!hNext);

	t1.Invalidate(h0);
}

void PerformSanityTests()
{
	TestHandles();
}