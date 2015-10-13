#include <rococo.types.h>
#include <rococo.maths.h>
#include <rococo.maths.inl>
#include <wchar.h>

#include <math.h>

#ifdef _DEBUG
# pragma comment(lib, "rococo.maths.debug.lib")
# pragma comment(lib, "rococo.os.win32.debug.lib")
#else
# pragma comment(lib, "rococo.maths.lib")
# pragma comment(lib, "rococo.os.win32.lib")
#endif

#include <stdio.h>

using namespace Rococo;

void validate(bool condition, const char* expr, const char* filename, int lineNumber)
{
	if (!condition)
	{
		Throw(0, L"Error validating %S at %S @line %d", expr, filename, lineNumber);
	}
}

bool Approx(float f, float g)
{
	return ::fabsf(f - g) < 0.0001f;
}

#define VALIDATE(p) validate(p, #p, __FILE__, __LINE__)

void test()
{
	wprintf(L"rococo.maths.test running...\n");

	// (x + 7)(2x + 4) = 2x^2 + 18x + 28

	float x0, x1;
	VALIDATE(TryGetRealRoots(x0, x1, 2.0f, 18.0f, 28.0f));
	VALIDATE(x1 == -7.0f && x0 == -2.0f);

	// 3x - 6
	VALIDATE(TryGetRealRoots(x0, x1, 0.0f, 3.0f, -6.0f));
	VALIDATE(x0 == x1 && x0 == 2.0f);

	// x2 + 1 = 0, has imaginary roots (x + i) (x - i)
	VALIDATE(!TryGetRealRoots(x0, x1, 1.0f, 0.0f, 1.0f));

	float t0, t1;
	VALIDATE(TryGetIntersectionLineAndSphere(t0, t1, Vec3{ 1, 0, 0 }, Vec3{ 21, 0, 0 }, Sphere{ Vec3 {7,0,0}, 2.0f }));
	VALIDATE(Approx(t0, 0.2f) && Approx(t1, 0.4f));

	VALIDATE(!TryGetIntersectionLineAndSphere(t0, t1, Vec3{ 1, 0, 0 }, Vec3{ 21, 0, 0 }, Sphere{ Vec3{ 7,4,0 }, 2.0f }));

	wprintf(L"rococo.maths.test finished\n");
}

int main(int argc, char* argv[])
{
	try
	{
		test();
	}
	catch (IException& ex)
	{
		wprintf(L"%s\n", ex.Message());
	}
}

