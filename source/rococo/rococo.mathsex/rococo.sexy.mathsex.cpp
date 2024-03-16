// rococo.sexy.vectorlib.cpp : Defines the exported functions for the DLL application.

#include <rococo.types.h>
#include <rococo.maths.h>

#include <DirectXMath.h>

#include <sexy.script.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>

#include <rococo.api.h>
#include <rococo.os.h>
#include <rococo.time.h>

#include <random>
#include <vector>

namespace
{
   std::default_random_engine rng;
}

namespace Rococo
{
   namespace Random
   {
      void Seed(int64 value)
      {
         if (value == 0)
         {
            value = Time::TickCount();
         }

         uint32 a = (uint32)(0x00000000FFFFFFFF & value);
         uint32 b = (uint32)(value >> 32);
         rng.seed(a ^ b);
      }

	  int32 Rand(int32 modulus)
	  {
		  if (modulus <= 2) Throw(0, "Rococo::Random::Rand(): Bad modulus - %d", modulus);
		  return rng() % modulus;
	  }

	  int32 AnyInt(int32 minValue, int32 maxValue)
	  {
		  int32 range = (maxValue - minValue);
		  if (range == 0)
		  {
			  return minValue;
		  }

		  int32 delta = Rand(range + 1);
		  return delta + minValue;
	  }

	  RGBAb AnyColour(int32 rMin, int32 rMax, int32 gMin, int32 gMax, int32 bMin, int32 bMax, int32 aMin, int32 aMax)
	  {
		  int32 r = AnyInt(rMin, rMax);
		  int32 g = AnyInt(gMin, gMax);
		  int32 b = AnyInt(bMin, bMax);	
		  int32 a = AnyInt(aMin, aMax);

		  return RGBAb((uint8)r, (uint8)g, (uint8)b, (uint8)a);
	  }

      int32 RollDie(int32 sides)
      {
         return (rng() % sides) + 1;
      }

      int32 RollDice(int32 count, int32 sides)
      {
         int sum = 0;
         for (int i = 0; i < count; ++i)
         {
            sum += RollDie(sides);
         }
         return sum;
      }

      float AnyFloat(float minValue, float maxValue)
      {
         float32 range = maxValue - minValue;
         float f = rng() / (float) rng.max();
         return range * f + minValue;
      }
   }

   RGBAb MakeColour(int32 r, int32 g, int32 b, int32 a)
   {
	   return RGBAb( (uint8)r, (uint8)g, (uint8)b, (uint8)a);
   }

   RGBAb ToRGBA(float red, float green, float blue, float alpha)
   {
	   float a = clamp(255.0f * alpha, 0.0f, 255.0f);
	   float r = clamp(255.0f * red, 0.0f, 255.0f);
	   float g = clamp(255.0f * green, 0.0f, 255.0f);
	   float b = clamp(255.0f * blue, 0.0f, 255.0f);

	   int A = (int)a;
	   int R = (int)r;
	   int G = (int)g;
	   int B = (int)b;

	   return MakeColour(R, G, B, A);
   }

   namespace Maths
   {
      // N.B when compiled in release mode, these will generally inline within the vectorlib.inl function code
      using namespace DirectX;

	  void AddVec2ToVec2(const Vec2& a, const Vec2& b, OUT Vec2& sum)
	  {
		  sum = a + b;
	  }

      void AddVec3ToVec3(const Vec3& a, const Vec3& b, OUT Vec3& sum)
      {
		  sum = a + b;
      }

      void SubtractVec3fromVec3(const Vec3& a, const Vec3& b, OUT Vec3& c)
      {
         c = a - b;
      }

	  void ScaleVec3(float f, const Vec3& a, OUT  Vec3& fa)
	  {
		  fa = a * f;
	  }

	  void ScaleVec3(const Vec3& a, float f, OUT Vec3& fa)
	  {
		  fa = a * f;
	  }

	  void ScaleVec2(const Vec2& a, float f, OUT Vec2& fa)
	  {
		  fa = a * f;
	  }

	  void ScaleVec2(float f, const Vec2& a, OUT Vec2& fa)
	  {
		  fa = a * f;
	  }

	  void SubtractVec2fromVec2(const Vec2& a, const Vec2& b, OUT Vec2& c)
	  {
		  c = a - b;
	  }

	  void DivideVec3(const Vec3& v, float denominator, OUT Vec3& result)
	  {
		  float f = 1.0f / denominator;
		  result = f * v;
	  }

	  void DivideVec2(const Vec2& v, float denominator, OUT Vec2& result)
	  {
		  float f = 1.0f / denominator;
		  result = f * v;
	  }

	  void GetTriSpan(cr_vec3 c, cr_vec3 a, cr_vec3 b, OUT Vec2& span)
	  {
		  Vec3 vertical = a - c;
		  Vec3 tangental = b - a;

		  span = Vec2 { Length(tangental), Length(vertical) };
	  }

      void MultiplyMatrixByRef(const Matrix4x4& a, const Matrix4x4& b, OUT Matrix4x4& c)
      {
         Multiply(c, a, b);
      }

      void CrossByRef(const Vec3& a, const Vec3& b, OUT Vec3& c)
      {
         XMVECTOR xa = XMLoadFloat3((const XMFLOAT3*) &a.x);
         XMVECTOR xb = XMLoadFloat3((const XMFLOAT3*) &b.x);
         XMVECTOR xc = XMVector3Cross(xa, xb);
         XMStoreFloat3((XMFLOAT3*)&c, xc);
      }

	  float CrossVec2(const Vec2& a, const Vec2& b)
	  {
		  XMVECTOR xa = XMLoadFloat2((const XMFLOAT2*)&a.x);
		  XMVECTOR xb = XMLoadFloat2((const XMFLOAT2*)&b.x);
		  XMVECTOR xc = XMVector2Cross(xa, xb);

		  float result;
		  XMStoreFloat(&result, xc);
		  return result;
	  }

      void NormalizeInPlace(Vec3& a)
      {
         float ds = Length(a);
         if (ds == 0) Throw(0, "Cannot normalize null vector");
         float scale = 1.0f / ds;

         a.x *= scale;
         a.y *= scale;
         a.z *= scale;
      }

	  void NormalizeInPlace(Vec2& a)
	  {
		  float ds = Length(a);
		  if (ds == 0) Throw(0, "Cannot normalize null vector");
		  float scale = 1.0f / ds;

		  a.x *= scale;
		  a.y *= scale;
	  }

	  void NormalizeInPlace(Vec4& a)
	  {
		  float ds = Length(reinterpret_cast<Vec3&>(a));
		  if (ds == 0) Throw(0, "Cannot normalize null vector");
		  float scale = 1.0f / ds;

		  a.x *= scale;
		  a.y *= scale;
		  a.z *= scale;
	  }

	  void SafeNormalize(Vec3& a)
	  {
		  float ds = LengthSq(a);

		  if (ds == 0) return;

		  float scale = 1.0f / sqrtf(ds);

		  a.x *= scale;
		  a.y *= scale;
		  a.z *= scale;
	  }

	  void SafeNormalize(Vec2& a)
	  {
		  float ds = LengthSq(a);

		  if (ds == 0) return;

		  float scale = 1.0f / sqrtf(ds);

		  a.x *= scale;
		  a.y *= scale;
	  }

	  void SafeNormalize(Vec4& a)
	  {
		  float ds = LengthSq(reinterpret_cast<Vec3&>(a));

		  if (ds == 0) return;

		  float scale = 1.0f / sqrtf(ds);

		  a.x *= scale;
		  a.y *= scale;
		  a.z *= scale;
	  }

	  void GetNormal(const Triangle& t, OUT Vec3& normal)
	  {
		  Vec3 ba = t.A - t.B;
		  Vec3 bc = t.C - t.B;
		  Vec3 product = Cross(ba, bc);
		  if (!TryNormalize(product, normal))
		  {
			  normal = { 0,0,0 };
		  }
	  }

	  void LerpVec3(const Vec3& a, const Vec3& b, float t, OUT Vec3& mixed)
	  {
		  mixed = a * (1.0f - t) + b * t;
	  }

	  void TransformVector(const Matrix4x4& m, const Vec4& v, OUT Vec4& mv)
	  {
		  mv = m * v;
	  }

	  void TransformVector(const Matrix4x4& m, const Vec3& v3, float w, OUT Vec4& mv)
	  {
		  Vec4 v4 = Vec4::FromVec3(v3, w);
		  mv = m * v4;
	  }

	  void GetRotationQuat(Degrees theta, float i, float j, float k, OUT Quat& q)
	  {
		  Degrees halfTheta{ 0.5f * theta };

		  float c = Cos(halfTheta);
		  float s = Sin(halfTheta);

		  q.s = c;
		  q.v.x = i * s;
		  q.v.y = j * s;
		  q.v.z = k * s;
	  }

	  void MultiplyQuatByQuat(const Quat& p, const Quat& q, OUT Quat& pq)
	  {
		  XMVECTOR P = XMLoadFloat4((const XMFLOAT4*)&p);
		  XMVECTOR Q = XMLoadFloat4((const XMFLOAT4*)&q);
		  XMVECTOR PQ = XMQuaternionMultiply(P, Q);
		  XMStoreFloat4((XMFLOAT4*) &pq, PQ);
	  }

	  void UnitQuatToMatrix(const Quat& q, OUT Matrix4x4& rotation)
	  {
		  XMVECTOR Q = XMLoadFloat4((const XMFLOAT4*)&q);
		  XMMATRIX R = XMMatrixRotationQuaternion(Q);
		  XMStoreFloat4x4((XMFLOAT4X4*)&rotation, R);
	  }

      void RotateAboutZThetaDegrees(Degrees theta, OUT Matrix4x4& Rz)
      {
          Rz = Matrix4x4::RotateRHAnticlockwiseZ(theta);
      }

	  void RotateAboutYThetaDegrees(Degrees theta, OUT Matrix4x4& Rz)
	  {
		  Rz = Matrix4x4::RotateRHAnticlockwiseY(theta);
	  }

	  void RotateAboutXThetaDegrees(Degrees theta, OUT Matrix4x4& Rz)
	  {
		  Rz = Matrix4x4::RotateRHAnticlockwiseX(theta);
	  }

	  void MakeTranslateMatrix(cr_vec3 ds, OUT Matrix4x4& T)
	  {
		  T = Matrix4x4::Translate(ds);
	  }

	  boolean32 TryGetCommonSegment(OUT Vec3& a, OUT Vec3& b, const Quad& P, const Quad& Q)
	  {
		  const Vec3* p = &P.a;
		  const Vec3* q = &Q.a;

		  int matchCount = 0;

		  for (int i = 0; i < 4; i++)
		  {
			  for (int j = 0; j < 4; j++)
			  {
				  if (p[i] == q[j])
				  {
					  Vec3& target = matchCount == 0 ? a : b;
					  target = p[i];
					  matchCount++;
					  if (matchCount == 2)
					  {
						  return true;
					  }
					  break;
				  }
			  }
		  }

		  return false;
	  }
   } // Maths
}// Rococo

using namespace Rococo;
using namespace Rococo::Script;
using namespace Rococo::Compiler;

#include "code-gen\Mathsex.sxh.inl"
#include "mathsex.random.inl"
#include "mathsex.base.inl"

extern "C"
{
   __declspec(dllexport) INativeLib* CreateLib(Rococo::Script::IScriptSystem& ss)
   {
      class GeomsNativeLib : public INativeLib
      {
      private:
         IScriptSystem& ss;

      public:
         GeomsNativeLib(IScriptSystem& _ss) : ss(_ss)
         {
         }

      private:
         void AddNativeCalls() override
         {
            Sys::Geometry::F32::AddNativeCalls_SysGeometryF32(ss);
			Sys::Geometry::M4x4::AddNativeCalls_SysGeometryM4x4(ss);
			Sys::Geometry::Q::AddNativeCalls_SysGeometryQ(ss);
            Sys::Random::AddNativeCalls_SysRandom(ss);
			Sys::Type::AddNativeCalls_SysType(ss);
         }

		 void ClearResources() override
		 {

		 }

         void Release() override
         {
            delete this;
         }
      };
      return new GeomsNativeLib(ss);
   }
}