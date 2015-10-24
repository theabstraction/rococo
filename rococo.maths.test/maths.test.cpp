#include <rococo.types.h>
#include <rococo.maths.h>
#include <wchar.h>
#include <DirectXMath.h>

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

void ValidatePolynomialLib()
{
	// (x + 7)(2x + 4) = 2x^2 + 18x + 28

	float x0, x1;
	VALIDATE(TryGetRealRoots(x0, x1, 2.0f, 18.0f, 28.0f));
	VALIDATE(x1 == -7.0f && x0 == -2.0f);

	// 3x - 6
	VALIDATE(TryGetRealRoots(x0, x1, 0.0f, 3.0f, -6.0f));
	VALIDATE(x0 == x1 && x0 == 2.0f);

	// x2 + 1 = 0, has imaginary roots (x + i) (x - i)
	VALIDATE(!TryGetRealRoots(x0, x1, 1.0f, 0.0f, 1.0f));
}

void ValidateCollisionLib()
{
	float t0, t1;
	VALIDATE(TryGetIntersectionLineAndSphere(t0, t1, Vec3{ 1, 0, 0 }, Vec3{ 21, 0, 0 }, Sphere{ Vec3 {7,0,0}, 2.0f }));
	VALIDATE(Approx(t0, 0.2f) && Approx(t1, 0.4f));

	VALIDATE(!TryGetIntersectionLineAndSphere(t0, t1, Vec3{ 1, 0, 0 }, Vec3{ 21, 0, 0 }, Sphere{ Vec3{ 7,4,0 }, 2.0f }));
}

void ValidateMatrixLib()
{
	using namespace DirectX;

	Vec4 centre{ 3.0f, 4.0f, 5.0f, 1.0f };
	Matrix4x4  T = Matrix4x4::Translate(-1.0f * centre);

	Vec4 v = T * centre;
	VALIDATE(v.x == 0 && v.y == 0 && v.z == 0);

	Degrees phi{ 90.0f };

	Matrix4x4 Rx = Matrix4x4::RotateRHAnticlockwiseX(phi);

	Vec4 rotatedCentre = Rx * centre;
 
	VALIDATE(Approx(rotatedCentre.x, centre.x));
	VALIDATE(Approx(rotatedCentre.y, -centre.z));
	VALIDATE(Approx(rotatedCentre.z, centre.y)); 

	Degrees theta{ 90.0f };
	Matrix4x4 Rz = Matrix4x4::RotateRHAnticlockwiseZ(theta);
	rotatedCentre = Rz *  centre;

	VALIDATE(Approx(rotatedCentre.x, -centre.y));
	VALIDATE(Approx(rotatedCentre.y, centre.x));
	VALIDATE(Approx(rotatedCentre.z, centre.z));

	Matrix4x4 RxRz = Rx * Rz;

	rotatedCentre = RxRz * centre;

	VALIDATE(Approx(rotatedCentre.x, -centre.y));
	VALIDATE(Approx(rotatedCentre.y, -centre.z));
	VALIDATE(Approx(rotatedCentre.z, centre.x));

	RxRz = Matrix4x4::RotateRHAnticlockwiseZ(Degrees{ 90.0 });

	Matrix4x4 moveCentreToOrigin = Matrix4x4::Translate(-1.0f * centre);
	Matrix4x4 shiftVertical = Matrix4x4::Translate(Vec3{ 0, 0, Metres{ 100.0f } });

	auto R0 = shiftVertical * RxRz * moveCentreToOrigin;

	Vec4 p{ 5.0f, 8.0f, 10.0f, 1.0 }; // after origin is moved, p -> (2 4 5), Rotate anticlockwise by 90 degrees -> (-4 2 5). Shift down by 100 to get (-4 2 105)
	Vec4 pPrimed = R0 * p;

	VALIDATE(Approx(pPrimed.x,  -4.0f));
	VALIDATE(Approx(pPrimed.y,   2.0f));
	VALIDATE(Approx(pPrimed.z, 105.0f));

	float scale = 2.0f;
	float aspectRatio = 1.5f;
	Matrix4x4 aspectCorrect = Matrix4x4::Scale(scale * aspectRatio, scale, scale);

	auto R1 = aspectCorrect * R0;

	pPrimed = R1 * p;

	VALIDATE(Approx(pPrimed.x, -12.0f));
	VALIDATE(Approx(pPrimed.y, 4.0f));
	VALIDATE(Approx(pPrimed.z, 210.0f));

	Matrix4x4 invR1 = InvertMatrix(R1);

	Vec4 pPrimedInv = invR1 * pPrimed;
	VALIDATE(Approx(pPrimedInv.x,  5.0f));
	VALIDATE(Approx(pPrimedInv.y,  8.0f));
	VALIDATE(Approx(pPrimedInv.z, 10.0f));

	XMMATRIX xortho = XMMatrixTranspose(XMMatrixOrthographicLH(2.0f, 2.0f, 0.0f, 1000.0f));

	Matrix4x4 ortho;
	XMStoreFloat4x4(ortho, xortho);

	Matrix4x4 R2 = ortho * R1;
	
	Vec4 pFullTransform = R2 * p;

	VALIDATE(Approx(pFullTransform.x, -12.0f));
	VALIDATE(Approx(pFullTransform.y, 4.0f));
	VALIDATE(Approx(pFullTransform.z, 0.21f));

	auto invR2 = InvertMatrix(R2);
	pPrimedInv = invR2 * pFullTransform;
	VALIDATE(Approx(pPrimedInv.x, 5.0f));
	VALIDATE(Approx(pPrimedInv.y, 8.0f));
	VALIDATE(Approx(pPrimedInv.z, 10.0f));
}

void test()
{
	wprintf(L"rococo.maths.test running...\n");

	ValidatePolynomialLib();
	ValidateCollisionLib();
	ValidateMatrixLib();

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

