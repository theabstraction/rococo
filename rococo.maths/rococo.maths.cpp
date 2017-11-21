#include <rococo.api.h>
#include <rococo.maths.h>
#include <rococo.strings.h>

#include <stdlib.h>

#ifdef _WIN32
# include <DirectXMath.h>

namespace Rococo
{
   // OSX not currently supported, as first target for OSX is VST plugins, not game engines
   float Determinant(const Matrix4x4& m)
   {
      using namespace DirectX;

      XMMATRIX xm = XMLoadFloat4x4(m);
      XMVECTOR xdet = XMMatrixDeterminant(xm);
      return xdet.m128_f32[0];
   }

   Matrix4x4 RotateDirectionToNegZ(cr_vec3 direction)
   {
	   float Dy = direction.y;
	   float Dx = direction.x;

	   float DS = sqrtf(Sq(Dx) + Sq(Dy));

	   Matrix4x4 rotZ;

	   if (DS < 0.0003f)
	   {
		   rotZ = Matrix4x4::Identity();
	   }
	   else
	   {
		   float sT = Dy / DS;
		   float cT = Dx / DS;
		   // Rotate direction onto y axis
		   rotZ =
		   {
			   { sT, -cT,     0,       0 },
			   { cT,  sT,     0,       0 },
			   { 0,    0,     1,       0 },
			   { 0,    0,     0,       1 }
		   };
	   }

	   float cP = direction.z;
	   float sP = sqrtf(1 - Sq(cP));

	   Matrix4x4 rotX
	   {
		   {  1,   0,   0,  0 },
		   {  0,  -cP,  sP,  0 },
		   {  0,  -sP,  -cP,  0 },
		   {  0,   0,   0,  1 }
	   };

	   return rotX * rotZ;
   }

   void XMVectorToVec4(DirectX::XMVECTOR xv, Vec4& v)
   {
      v.x = xv.m128_f32[0];
      v.y = xv.m128_f32[1];
      v.z = xv.m128_f32[2];
      v.w = xv.m128_f32[3];
   }

   void XMMatrixToM4x4(DirectX::XMMATRIX xm, Matrix4x4& m)
   {
      XMVectorToVec4(xm.r[0], m.row0);
      XMVectorToVec4(xm.r[1], m.row1);
      XMVectorToVec4(xm.r[2], m.row2);
      XMVectorToVec4(xm.r[3], m.row3);
   }

   void Matrix4x4::FromQuat(const Quat& quat, Matrix4x4& m)
   {
      using namespace DirectX;
      XMVECTOR q = XMLoadFloat4(quat);
      XMMATRIX xm = XMMatrixRotationQuaternion(q);
      XMMatrixToM4x4(xm, m);
   }

   bool GetTriangleHeight(const Triangle& t, cr_vec2 P, float& result)
   {
	   // Triangle is in a plane (P - A).N = 0 where A is a point in plane, N is normal and P is any other point in plane
	   // Expands to P.N = A.N: 
	   // Px.Nx + Py.Ny + Pz.Nz = A.N
	   // Pz = [A.N - (Px.Nx + Py.Ny)] / Nz

	   Vec3 N = Cross(t.B - t.A, t.C - t.B);

	   if (fabs(N.z) <= 0.001f) return false;

	   result = (Dot(t.A, N) - (P.x * N.x + P.y * N.y)) / N.z;
	   return true;
   }

   void Matrix4x4::GetRotationQuat(const Matrix4x4& m, Quat& quat)
   {
      using namespace DirectX;
      XMMATRIX xm = XMLoadFloat4x4(m);
      XMVECTOR q = XMQuaternionRotationMatrix(xm);
      XMStoreFloat4(quat, q);
   }

   void Matrix4x4::FromQuatAndThenTranspose(const Quat& quat, Matrix4x4& m)
   {
      using namespace DirectX;

      XMVECTOR q;
      q = XMLoadFloat4(quat);
      XMMATRIX xm = XMMatrixRotationQuaternion(q);
      XMMATRIX xmt = XMMatrixTranspose(xm);
      XMMatrixToM4x4(xmt, m);
   }

   void GetIsometricTransforms(Matrix4x4& worldMatrix, Matrix4x4& inverseWorldMatrixProj, Matrix4x4& worldMatrixAndProj, float scale, float aspectRatio, cr_vec3 centre, Degrees phi, Degrees viewTheta, Metres cameraHeight)
   {
      using namespace DirectX;

      Matrix4x4 Rx = Matrix4x4::RotateRHAnticlockwiseX(phi);
      Matrix4x4 Rz = Matrix4x4::RotateRHAnticlockwiseZ(viewTheta);
      Matrix4x4 centreToOrigin = Matrix4x4::Translate(-1.0f * centre);
      Matrix4x4 Sxyz = Matrix4x4::Scale(scale * aspectRatio, scale, -scale);

      Matrix4x4 verticalShift = Matrix4x4::Translate(Vec3{ 0, 0, cameraHeight });

      worldMatrix = verticalShift * Sxyz * Rx * Rz * centreToOrigin;

      XMMATRIX xortho = XMMatrixOrthographicLH(2.0f, 2.0f, 0.0f, 1000.0f);
      XMMatrixTranspose(xortho);

      Matrix4x4 ortho;
      XMStoreFloat4x4(ortho, xortho);

      worldMatrixAndProj = ortho * worldMatrix;

      inverseWorldMatrixProj = InvertMatrix(worldMatrixAndProj);
   }

   LineClassify ClassifyPtAgainstPlane(Vec2 a, Vec2 b, Vec2 p)
   {
	   Vec2 ab = b - a;
	   Vec2 ap = p - a;

	   float f = Cross(ab, ap);
	   if (f > 0)
	   {
		   return LineClassify_Left;
	   }
	   else if (f < 0)
	   {
		   return LineClassify_Right;
	   }
	   else
	   {
		   return LineClassify_OnLine;
	   }
   }

   void TransposeMatrix(const Matrix4x4& matrix, Matrix4x4& transposeOfMatrix)
   {
      using namespace DirectX;
      XMMATRIX t = XMLoadFloat4x4((DirectX::XMFLOAT4X4*) &matrix);
      XMStoreFloat4x4((DirectX::XMFLOAT4X4*) &transposeOfMatrix, XMMatrixTranspose(t));
   }

   AABB2d::AABB2d():
	   left(1.0e37f), bottom(1.0e37f), top(-1.0e37f), right(-1.0e37f)
   {
   }

   void AABB2d::Empty()
   {
	   *this = AABB2d();
   }

   bool AABB2d::HoldsPoint(cr_vec2 p) const
   {
	   return p.x >= left && p.x <= right && p.y >= bottom && p.y <= top;
   }

   Vec2 AABB2d::Centre() const
   {
	   return 0.5f * Vec2{ right + left, top + bottom };
   }

   Vec2 AABB2d::Span() const
   {
	   return { right - left, top - bottom };
   }

   AABB2d& AABB2d::operator << (cr_vec2 p)
   {
	   left = min(left, p.x);
	   right = max(right, p.x);
	   bottom = min(bottom, p.y);
	   top = max(top, p.y);
	   return *this;
   }

   AABB::AABB():
	   minXYZ{ 1.0e37f, 1.0e37f, 1.0e37f },
	   maxXYZ{ -1.0e37f, -1.0e37f,-1.0e37f }
   {

   }
  
   void AABB::Empty()
   {
	   *this = AABB();
   }

   AABB& AABB::operator << (cr_vec3 p)
   {
	   minXYZ.x = min(p.x, minXYZ.x);
	   minXYZ.y = min(p.y, minXYZ.y);
	   minXYZ.z = min(p.z, minXYZ.z);

	   maxXYZ.x = max(p.x, maxXYZ.x);
	   maxXYZ.y = max(p.y, maxXYZ.y);
	   maxXYZ.z = max(p.z, maxXYZ.z);

	   return *this;
   }

   bool AABB::HoldsPoint(cr_vec3 p) const
   {
	   if (p.x >= minXYZ.x && p.x <= maxXYZ.x && p.y >= minXYZ.y && p.y <= maxXYZ.y && p.z > minXYZ.z && p.z < maxXYZ.z)
	   {
		   return true;
	   }

	   return false;
   }

   bool AABB::Intersects(const AABB& other) const
   {
	   if (other.maxXYZ.z > maxXYZ.z && other.minXYZ.z > maxXYZ.z)
	   {
		   return false;
	   }

	   if (other.maxXYZ.z < minXYZ.z && other.minXYZ.z < minXYZ.z)
	   {
		   return false;
	   }

	   if (other.maxXYZ.x > maxXYZ.x && other.minXYZ.x > maxXYZ.x)
	   {
		   return false;
	   }

	   if (other.maxXYZ.x < minXYZ.x && other.minXYZ.x < minXYZ.x)
	   {
		   return false;
	   }

	   if (other.maxXYZ.y > maxXYZ.y && other.minXYZ.y > maxXYZ.y)
	   {
		   return false;
	   }

	   if (other.maxXYZ.y < minXYZ.y && other.minXYZ.y < minXYZ.y)
	   {
		   return false;
	   }

	   return true;
   }

   Vec3 AABB::Centre() const
   {
	   return 0.5f * (maxXYZ + maxXYZ);
   }

   void AABB::GetBox(BoundingBox& box) const
   {
	   box.bottom.nw = { minXYZ.x, maxXYZ.y, minXYZ.z };
	   box.bottom.ne = { maxXYZ.x, maxXYZ.y, minXYZ.z };
	   box.bottom.se = { maxXYZ.x, minXYZ.y, minXYZ.z };
	   box.bottom.sw = { minXYZ.x, minXYZ.y, minXYZ.z };
	   box.top.nw = { minXYZ.x, maxXYZ.y, maxXYZ.z };
	   box.top.ne = { maxXYZ.x, maxXYZ.y, maxXYZ.z };
	   box.top.se = { maxXYZ.x, minXYZ.y, maxXYZ.z };
	   box.top.sw = { minXYZ.x, minXYZ.y, maxXYZ.z };
   }

   Vec3 AABB::Span() const
   {
	   return maxXYZ - minXYZ;
   }

   AABB AABB::RotateBounds(const Matrix4x4& Rz) const
   {
		BoundingBox box;
		GetBox(box);

		Vec3 corners[8];
		TransformPositions(box.First(), 8, Rz, corners);

		AABB newBounds;
		for (auto& v : corners)
		{
			newBounds << v;
		}

		return newBounds;
   }

   void InvertMatrix(const Matrix4x4& matrix, Matrix4x4& inverseMatrix)
   {
      using namespace DirectX;
      XMMATRIX m = XMLoadFloat4x4((DirectX::XMFLOAT4X4*) &matrix);
      XMVECTOR det;
      XMMATRIX im = XMMatrixInverse(&det, m);
      XMStoreFloat4x4((DirectX::XMFLOAT4X4*) &inverseMatrix, im);
   }

   void Multiply(Matrix4x4& product, const Matrix4x4& Ra, const Matrix4x4& Rb)
   {
      using namespace DirectX;
      auto XRa = XMLoadFloat4x4(Ra);
      auto XRb = XMLoadFloat4x4(Rb);
      auto XRaRb = XMMatrixMultiply(XRa, XRb);

      XMStoreFloat4x4(product, XRaRb);
   }

   Quat InterpolateRotations(cr_quat a, cr_quat b, float t)
   {
      using namespace DirectX;

      XMVECTOR A = XMLoadFloat4(a);
      XMVECTOR B = XMLoadFloat4(b);

      auto p = DirectX::XMQuaternionSlerp(A, B, t);

      return Quat(Vec3{ p.m128_f32[0],p.m128_f32[1], p.m128_f32[2] }, p.m128_f32[3]);
   }

   Vec4 operator * (const Vec4& v, const Matrix4x4& R)
   {
      using namespace DirectX;
      auto XR = XMLoadFloat4x4(R);
      auto XV = XMLoadFloat4(v);

      auto vprimed = XMVector4Transform(XV, XR);

      Vec4 u;
      XMStoreFloat4(u, vprimed);
      return u;
   }
}

#endif

namespace // globals
{
	using namespace Rococo;

	const Matrix4x4 const_identity4x4
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};

	const Matrix4x4 const_null4x4
	{
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 }
	};
}

namespace Rococo
{
	Matrix4x4 Matrix4x4::GetRHProjectionMatrix(Degrees fov, float32 aspectRatio, float near, float far)
	{
		if (fov < 0.001f || fov > 179.0f)                Throw(0, "Field-of-view out of range. Must be 0.001 <= fov <= 179.");
		if (aspectRatio < 0.01f || aspectRatio > 100.0f) Throw(0, "Aspect ratio outside of sane range. Must be 0.01 <= AR <= 100.");
		if (near <= 0 || near > 100000.0f)               Throw(0, "Near plane outside of sane range. Range: 0 < near < 100000");
		if (far <= near || far > 100000.0f)              Throw(0, "Far plane outside of sane range. Range: near < far <= 100000");

		float32 tanhalffov = tanf(fov.ToRadians() / 2.0f);

		/*
		The camera sits at the origin looking down, therefore only  -far < z < -near is visible
		Thus z is negative to be visible.
		To stop x and y flipping directions in screen space, their quotient has to be positive
		So the w divide has to be positive. To make divide positive with -ve z, we need to multiply z by -1
		So we have matrix
		(A  0  0  0)
		(0  B  0  0)
		(0  0  C  D)
		(0  0 -1  0)
		When z - -far, proj(z) = proj(-far) = 1, thus (C.-far + D) / (-1)(-far) = 1, or D - C.far = far
		When z = -near, proj(z) = proj(-near) = 0, thus D = C.near
		// Thus C.near - C.far = far, and thus C = far / (near-far)
		With D = (far.near) / (near -far)
		// Note that near < far, thus C and D are negative

		*/

		float32 B = 1.0f / tanhalffov;
		float32 A = B / aspectRatio;
		float32 C = far / (near - far);
		float32 D = (far * near) / (near - far);

		return Matrix4x4
		{
		   { A,	  0,  0,   0 },
		   { 0,   B,  0,   0 },
		   { 0,   0,  C,   D },
		   { 0,   0, -1,   0 }
		};
	}

	void ExpandZoneToContain(GuiRect& rect, const Vec2i& p)
	{
		if (p.x < rect.left) rect.left = p.x;
		if (p.x > rect.right) rect.right = p.x;
		if (p.y < rect.top) rect.top = p.y;
		if (p.y > rect.bottom) rect.bottom = p.y;
	}

	void ExpandZoneToContain(GuiRectf& rect, const Vec2& p)
	{
		if (p.x < rect.left) rect.left = p.x;
		if (p.x > rect.right) rect.right = p.x;
		if (p.y < rect.top) rect.top = p.y;
		if (p.y > rect.bottom) rect.bottom = p.y;
	}

	Vec2  GetIntersect(Vec2 A, Vec2 D, Vec2 B, Vec2 E)
	{
		// Given line P(t) = A + Dt and line Q(u) = B + Eu
		// Find point common to both lines

		// Px = Ax + Dx.t
		// Py = Ay + Dy.y

		// Px = Bx + Ex.u
		// Py = By + Ey.u

		// Ax + Dxt = Bx + Exu
		// Ay + Dyt = By + Eyu

		// AxDy + DxDyt = BxDy + ExuDy
		// AyDx + DxDyt = DxBy + EyuDx

		// AxDy - AyDx = BxDy - DxBy + (ExDy - EyDx)u

		// (AxDy - AyDx -  BxDy + DxBy) / (ExDy - EyDx) = u
		// (Ax - Bx).Dy + (By - Ay).Dx /  (ExDy - EyDx) = u

		float denominator = E.x*D.y - E.y*D.x;

		if (denominator == 0.0f)
		{
			Throw(0, "Degenerate args in GetIntersect(...)");
		}

		float numerator = (A.x - B.x) * D.y + (B.y - A.y)*D.x;

		float u = numerator / denominator;

		return B + E * u;
	}

	const Matrix4x4& Matrix4x4::Identity()
	{
		return const_identity4x4;
	}

	const Matrix4x4& Matrix4x4::Null()
	{
		return const_null4x4;
	}

	Matrix4x4 Matrix4x4::Translate(cr_vec3 v)
	{
		return Matrix4x4{
			{1.0f, 0,      0,   v.x},
			{0,    1.0f,   0,   v.y },
			{0,    0,   1.0f,   v.z },
			{0,    0,      0,  1.0f }
		};
	}

	Matrix4x4 Matrix4x4::Scale(float Sx, float Sy, float Sz)
	{
		return Matrix4x4{
			{ Sx,    0,    0,     0},
			{ 0,    Sy,    0,     0},
			{ 0,     0,   Sz,     0},
			{ 0,     0,    0,  1.0f}
		};
	}

	Matrix4x4 Matrix4x4::RotateRHAnticlockwiseX(Radians phi)
	{
		float s = Sin(phi);
		float c = Cos(phi);

		return Matrix4x4{
			{ 1.0f, 0,     0,    0 },
			{ 0,    c,    -s,    0 },
			{ 0,    s,     c,    0 },
			{ 0,    0,     0,  1.0f }
		};
	}

	Matrix4x4 Matrix4x4::RotateRHAnticlockwiseZ(Radians theta)
	{
		float s = Sin(theta);
		float c = Cos(theta);

		return Matrix4x4{
			{ c,   -s,     0,        0 },
			{ s,    c,     0,        0 },
			{ 0,    0,     1.0f,     0 },
			{ 0,    0,     0,     1.0f }
		};
	}

	Vec3 Matrix4x4::GetForwardDirection() const
	{
		// M.north gives direction of target

		/*
		( row0 ) ( 0 )     ( row0.y )
		( row1 ) ( 1 ) =   ( row1.y )
		( row2 ) ( 0 )     ( row2.y )
		( row3 ) ( 0 )	   (   0    )
		*/

		return{ row0.y, row1.y, row2.y };
	}

	Vec3 Matrix4x4::GetRightDirection() const
	{
		// M.north gives direction of target

		/*
		( row0 ) ( 1 )     ( row0.x )
		( row1 ) ( 0 ) =   ( row1.x )
		( row2 ) ( 0 )     ( row2.x )
		( row3 ) ( 0 )	   (   0    )
		*/

		return{ row0.x, row1.x, row2.x };
	}

	Vec3 Matrix4x4::GetUpDirection() const
	{
		// M.north gives direction of target

		/*
		( row0 ) ( 0 )     ( row0.z )
		( row1 ) ( 0 ) =   ( row1.z )
		( row2 ) ( 1 )     ( row2.z )
		( row3 ) ( 0 )	   (   0    )
		*/

		return{ row0.z, row1.z, row2.z };
	}

	Radians GetHeadingOfVector(float DX, float DY)
	{
		if (DX == 0 && DY == 0) return 0.0_degrees;
		float f = DY / (Square(DX) + Square(DY));
		f = min(1.0f, max(-1.0f, f));

		float theta = acosf(f);

		if (DX < 0)
		{
			return Radians{ theta };
		}
		else
		{
			return Radians{ (360.0_degrees).ToRadians() - theta };
		}
	}

	void TransformPositions(const Vec3* vertices, size_t nElements, cr_m4x4 transform, Vec3* transformedVertices)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			cr_vec3 v = vertices[i];
			Vec3& tv = transformedVertices[i];
			tv.x = transform.row0.x * v.x + transform.row0.y * v.y + transform.row0.z * v.z + transform.row0.w;
			tv.y = transform.row1.x * v.x + transform.row1.y * v.y + transform.row1.z * v.z + transform.row1.w;
			tv.z = transform.row2.x * v.x + transform.row2.y * v.y + transform.row2.z * v.z + transform.row2.w;
		}
	}

	void TransformDirections(const Vec3* vertices, size_t nElements, cr_m4x4 transform, Vec3* transformedVertices)
	{
		for (size_t i = 0; i < nElements; ++i)
		{
			cr_vec3 v = vertices[i];
			Vec3& tv = transformedVertices[i];
			tv.x = transform.row0.x * v.x + transform.row0.y * v.y + transform.row0.z * v.z;
			tv.y = transform.row1.x * v.x + transform.row1.y * v.y + transform.row1.z * v.z;
			tv.z = transform.row2.x * v.x + transform.row2.y * v.y + transform.row2.z * v.z;
		}
	}

	void TransformDirection(cr_m4x4 m, cr_vec3 v, Vec3& np)
	{
		np.x = m.row0.x * v.x + m.row0.y * v.y + m.row0.z * v.z;
		np.y = m.row1.x * v.x + m.row1.y * v.y + m.row1.z * v.z;
		np.z = m.row2.x * v.x + m.row2.y * v.y + m.row2.z * v.z;
	}

#ifdef _WIN32

	Matrix4x4 InvertMatrix(const Matrix4x4& matrix)
	{
		Matrix4x4 invMatrix;
		InvertMatrix(matrix, invMatrix);
		return invMatrix;
	}

	Matrix4x4 TransposeMatrix(const Matrix4x4& matrix)
	{
		Matrix4x4 tMatrix;
		TransposeMatrix(matrix, tMatrix);
		return tMatrix;
	}

#endif

	float Dot(const Vec4& a, const Vec4& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}


#ifdef _WIN32
	Matrix4x4 operator * (const Matrix4x4& a, const Matrix4x4& b)
	{
		Matrix4x4 result;
		Multiply(result, a, b);
		return result;
	}
#endif

	Vec4 operator * (const Matrix4x4& R, const Vec4& v)
	{
		return Vec4{
			Dot(R.row0, v),
			Dot(R.row1, v),
			Dot(R.row2, v),
			Dot(R.row3, v),
		};
	}

	bool TryGetRealRoots(float& x0, float& x1, float a /* x^2 */, float b /* x */, float c)
	{
		// Solve quadratic equation ax^2 + bx + c = 0 using formula
		// x =  -b/2a (+-) sqrt( b^2 - 4ac)/2a

		if (a == 0.0f)
		{
			if (b == 0.0f)
			{
				x0 = x1 = 0.0f;
				return false;
			}
			// Degenerate case -> One root
			x0 = x1 = -c / b;
			return true;
		}
		else
		{
			float d = b*b - 4.0f*a*c;
			if (d < 0)
			{
				// Complex roots
				x0 = x1 = 0.0f;
				return false;
			}

			float e = -0.5f * b;
			float f = sqrtf(d);

			x0 = e + 0.5f * f;
			x1 = e - 0.5f * f;

			float ooa = 1.0f / a;
			x0 *= ooa;
			x1 *= ooa;

			return true;
		}
	}

	void swap(float& a, float &b)
	{
		float c = b;
		b = a;
		a = c;
	}

	bool TryGetIntersectionLineAndSphere(float& t0, float& t1, cr_vec3 start, cr_vec3 end, const Sphere& sphere)
	{
		// P(t) = start + (end - start).t for every point on the line containing start and end = a + (b-a).t
		// if P is on sphere, then (P - centre).(P - centre) = R^2
		// Thus at intersect
		// ((a-centre) + (b-a).t).(a-centre + (b-a).t) = R^2
		// (b-a).(b-a).t^2 + 2(a-centre)*(b-a)t + (a-centre)*(a-centre) - R^2 = 0
		// Yielding two intersect points, the roots of the quadratic

		Vec3 atoc = start - sphere.centre;

		if (TryGetRealRoots(t0, t1, LengthSq(end - start), 2.0f * atoc * (end - start), LengthSq(atoc) - sphere.radius*sphere.radius))
		{
			if (t0 > t1)
			{
				// Swap to 
				swap(t0, t1);
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	void ThrowNumericException(cstr format)
	{
		struct : IException
		{
			rchar msg[256];

			virtual cstr Message() const { return msg; }
			virtual int32 ErrorCode() const { return 0; }
		} ex;

		SafeFormat(ex.msg, sizeof(ex.msg), "%s", format);

		OS::TripDebugger();

		throw ex;
	}

	// Return normalized vector. In the event of a null vector, an exception is thrown
	Vec3 Normalize(cr_vec3 v)
	{
		const float epsilon = 10e-18f;
		float l = LengthSq(v);
		if (l <= epsilon)
		{
			ThrowNumericException("Vec3 Normalize(cr_vec3 v) failed: the argument was a null vecctor");
		}

		float f = 1.0f / sqrtf(l);
		return v * f;
	}

	bool TryNormalize(cr_vec3 v, Vec3& nv)
	{
		const float epsilon = 0.0000001f;
		float l = LengthSq(v);
		if (l <= epsilon)
		{
			return false;
		}

		float f = 1.0f / sqrtf(l);
		nv = v * f;
		return true;
	}

	float Length(cr_vec3 v)
	{
		float l2 = LengthSq(v);
		return sqrtf(l2);
	}

	// Returns elevation at which to fire projectile to hit a target. Returns negative if no sensible value
	// If the target cannot be made accurate to factor less than than the largestError value, then the query is not sensible
	Radians ComputeWeaponElevation(cr_vec3 origin, cr_vec3 target, float projectileSpeed, Degrees maxElevation, Gravity g, Metres largestError)
	{
		if (projectileSpeed <= 0 || maxElevation <= 5.0f)
		{
			return Radians{ -1.0f };
		}

		// Cap to something sensible
		if (maxElevation > 85.0f)
		{
			maxElevation = 85.0_degrees;
		}

		float S = Length(target - origin);

		const float pointBlank = 0.5f;

		if (S < pointBlank)
		{
			// At point blank, assume we hit target whatever elevation
			return Radians{ 0.0f };
		}

		float Z = target.z - origin.z;

		// Vz vertical component of projectile velocity is speed.sin(theta), where theta is angle of elevation
		// Vs horizontal component of projectile velocity is speed.cos(theta)

		// flight time t = S / Vs
		// In t change of height of projectile is Dh = Vz.t + 0.5gt^2
		// Target Dh is Z

		// 0.5g t ^ 2 + Vz.t - Z = 0..................(1)
		// t = S/Vs...................................(2)

		// 0.5gS^2/(speed^2.cos^2(theta)) + speed.sin(theta).S / (speed.cos(theta)) - DZ = 0....(3)

		// Let a = 0.5gS^2/speed^2

		// a / cos^2(theta) + S.tan(theta) = Z

		float maxTheta{ maxElevation * 3.14159f / 180.0f };
		float minTheta{ 0.0f };
		float deltaTheta = (maxTheta - minTheta) * 0.05f; // This gives 20 estimates

		float a = 0.5f * g * Square(S) / Square(projectileSpeed);

		float bestTheta = minTheta;
		float bestError = 1.0e30f;

		for (float theta = minTheta; theta < maxTheta; theta += deltaTheta)
		{
			float z = a / Square(cosf(theta)) + S * tanf(theta);

			float error = fabsf(z - Z);

			if (error < bestError)
			{
				bestTheta = theta;
				bestError = error;
			}
		}

		if (largestError > bestError)
		{
			return Radians{ bestTheta };
		}
		else
		{
			return Radians{ -1.0f };
		}
	}

	Vec2i TopCentre(const GuiRect& rect)
	{
		return Vec2i{ (rect.left + rect.right) >> 1, rect.top };
	}

	bool IsPointInRect(const Vec2i& p, const GuiRect& rect)
	{
		return (p.x >= rect.left && p.x <= rect.right && p.y >= rect.top && p.y <= rect.bottom);
	}

	bool Triangle2d::IsInternal(Vec2 p) const
	{
		float Fa = Cross(B - A, p - B);
		float Fb = Cross(C - B, p - C);
		float Fc = Cross(A - C, p - A);

		if (Fa < 0 && Fb < 0 && Fc < 0)
		{
			return true;
		}

		return false;
	}

	bool Triangle2d::IsInternalOrOnEdge(Vec2 p) const
	{
		float Fa = Cross(B - A, p - B);
		float Fb = Cross(C - B, p - C);
		float Fc = Cross(A - C, p - A);

		if (Fa <= 0 && Fb <= 0 && Fc <= 0)
		{
			return true;
		}

		return false;
	}

	size_t Triangle2d::CountInternalPoints(const Vec2* points, size_t nPoints)
	{
		size_t count = 0;
		for (size_t i = 0; i < nPoints; ++i)
		{
			auto p = points[i];
			if (IsInternal(p))
			{
				count++;
			}
		}

		return count;
	}

	bool IsOdd(int32 i)
	{
		return (i % 2) == 1;
	}

	bool DoesLineIntersectRing(Vec2 origin, Vec2 normal, IRingManipulator<Vec2>& ring)
	{
		IntersectCounts counts = { 0 };
		for (size_t i = 0; i < ring.ElementCount(); ++i)
		{
			Vec2 q0 = ring[i];
			Vec2 q1 = ring[i + 1];

			if (q0 != origin)
			{
				auto count = CountLineIntersects(origin, normal, q0, q1);
				counts.forwardCount += count.forwardCount;
				counts.backwardCount += count.backwardCount;
				counts.edgeCases += count.edgeCases;
				counts.coincidence += count.coincidence;
			}
		}

		return counts.forwardCount + counts.coincidence > 0;
	}

	bool IsClockwiseSequential(IRing<Vec2>& ring)
	{
		Vec2 p0 = ring[0];
		Vec2 p1 = ring[1];

		Vec2 centre = (p0 + p1) * 0.5f;
		Vec2 dp = p1 - p0;

		Vec2 normal = { dp.y, -dp.x };
			
		// Count the number of times that normal crosses the perimeter
		int32 forwardCount = 0;
		for (size_t i = 1; i < ring.ElementCount(); ++i)
		{
			Vec2 q0 = ring[i];
			Vec2 q1 = ring[i + 1];

			Vec2 other_normal = { q1.y - q0.y, q0.x - q1.x };

			float dot = Dot(normal, other_normal);

			float t, u;
			if (GetLineIntersect(q0, q1, centre, centre + normal, t, u))
			{
				if (u > 0) 
				{
					if (dot > 0)
					{
						// entering edge
						if (t >= 0 && t < 1)
						{
							forwardCount--;
						}
					}
					else if (dot == 0)
					{
						// Normal runs along edge
					}
					else
					{
						// leaving edge
						if (t > 0 && t <= 1)
						{
							forwardCount++;
						}
					}
				}
			}
		}

		return forwardCount > 0;
	}

	void TesselateByEarClip(I2dMeshBuilder& builder, IRingManipulator<Vec2>& ring)
	{
		if (ring.ElementCount() < 3)
		{
			Throw(0, "Cannot tesselate ring with fewer than 3 elements");
		}

		// Algorithm assumes ring is enumerated clockwise
		// If it is not clockwise the algorithm will throw the exception at the end of the function

		// Enumerate vertices looking for an angle < 180 degrees (i.e convex angles). Cross product positive

		size_t i = 0;
		while (i < ring.ElementCount() && ring.ElementCount() >= 3)
		{
			Triangle2d t{ ring[i], ring[i + 1], ring[i + 2] };

			Vec2 ab = t.B - t.A;
			Vec2 bc = t.C - t.B;

			float k = Cross(ab, bc);
			if (k == 0)
			{
				// abc is a straight line, hence b is redundant
				ring.Erase(i + 1);
				i = 0;
				continue;
			}
			else if (k < 0) // clockwise, we have an ear
			{
				// If we have an ear, we can eliminate B providing the triangle does not intersect any internal points

				size_t firstTestVertex = i + 3;
				size_t lastTestVertex = firstTestVertex + ring.ElementCount() - 3;

				bool hasInternalPoint = false;
				for (size_t j = firstTestVertex; j < lastTestVertex; ++j)
				{
					Vec2 p = ring[j];

					if (t.IsInternalOrOnEdge(p))
					{
						// Skip ear, there was an internal point
						hasInternalPoint = true;
						break;
					}
				}

				if (!hasInternalPoint)
				{
					builder.Append(t);
					ring.Erase(i + 1);
					i = 0;
					continue;
				}
			}


			i++;
		}
	}
}