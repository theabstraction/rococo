#include <rococo.api.h>
#include <rococo.handles.h>
#include <rococo.maths.h>

using namespace Rococo;

void Validate(bool x, cstr message) { if (!x) Throw(0, "%s", message); }
#define VALIDATE(x) Validate(x, #x);

void TestHandles()
{
	// HandleTable<int32, 0x0000000FFFFFFFFF, 36> t0("t0", 0, nullptr); -> should throw
	HandleTable<int32, 8> t1("t1", 2);
	auto h0 = t1.CreateNew();
	auto h1 = t1.CreateNew();

	auto hFirst = t1.GetFirstHandle();
	VALIDATE(hFirst == h0);

	auto hNext = t1.GetNextHandle(hFirst);
	VALIDATE(hNext == h1);

	hNext = t1.GetNextHandle(hNext);
	VALIDATE(!hNext);

	t1.Destroy(h0, 0);
}

void TestMaths()
{
	Triangle t
	{
		Vec3 { 0, 1.0f, 0.0f},
		Vec3 { -1.0f, 0.0f, 0.0f},
		Vec3 { 1.0f, 0.0f, 0.0f}
	};

	Vec3 start = { 0, 0.25f, 1.0f };
	Vec3 dir = { 0, 0, -1.0f };

	Collision c = CollideLineAndTriangle(t, start, dir);

	VALIDATE(c.t == 1.0f);

	c = CollideLineAndTriangle(t, Vec3{ 1.0f, 1.0f, 1.0f }, dir);

	VALIDATE(c.contactType == ContactType_None);

	Triangle s
	{
		Vec3 { 13.9650002f, -2.51f, 1.45f},
		Vec3 { 13.9650002f, -2.51f, 1.51f },
		Vec3 { 13.9650002f,  -2.45f, 1.51f },
	};

	Vec3 eye2{ 13.0848932f, -2.31210995f, 1.64999998f };
	Vec3 dir2{ 0.961057663f, -0.199900240f, -0.190809026f };
	c = CollideLineAndTriangle(s, eye2, dir2);

	auto time = c.t;
}

void PerformSanityTests()
{
	TestHandles();
	TestMaths();
}