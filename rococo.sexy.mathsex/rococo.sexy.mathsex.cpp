// rococo.sexy.vectorlib.cpp : Defines the exported functions for the DLL application.

#include <rococo.types.h>
#include <rococo.maths.h>

#include <DirectXMath.h>

#include <sexy.script.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>

#include <rococo.api.h>

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
            value = OS::CpuTicks();
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

   namespace Maths
   {
      // N.B when compiled in release mode, these will generally inline within the vectorlib.inl function code
      using namespace DirectX;

      void AddVec3toVec3(const Vec3& a, const Vec3& b, Vec3& c)
      {
         c = a + b;
      }

      void SubtractVec3fromVec3(const Vec3& a, const Vec3& b, Vec3& c)
      {
         c = a - b;
      }

	  void ScaleVector3(float f, const Vec3& a, Vec3& fa)
	  {
		  fa = a * f;
	  }

	  void ScaleVector3(const Vec3& a, float f, Vec3& fa)
	  {
		  fa = a * f;
	  }

	  void AddVec2toVec2(const Vec2& a, const Vec2& b, Vec2& c)
	  {
		  c = a + b;
	  }

	  void SubtractVec2fromVec2(const Vec2& a, const Vec2& b, Vec2& c)
	  {
		  c = a - b;
	  }

	  void GetTriSpan(cr_vec3 d, cr_vec3 a, cr_vec3 b, Vec2& span)
	  {
		  Vec3 vertical = a - d;
		  Vec3 tangental = b - a;

		  span = Vec2 { Length(tangental), Length(vertical) };
	  }

	  void ScaleVector2(const Vec2& a, float f, Vec2& fa)
	  {
		  fa = a * f;
	  }

	  void ScaleVector2( float f, const Vec2& a, Vec2& fa)
	  {
		  fa = a * f;
	  }

      void MultiplyMatrixByRef(const Matrix4x4& a, const Matrix4x4& b, Matrix4x4& c)
      {
         Multiply(c, a, b);
      }

      void CrossByRef(const Vec3& a, const Vec3& b, Vec3& c)
      {
         XMVECTOR xa = XMLoadFloat3((const XMFLOAT3*) &a.x);
         XMVECTOR xb = XMLoadFloat3((const XMFLOAT3*) &b.x);
         XMVECTOR xc = XMVector3Cross(xa, xb);
         XMStoreFloat3((XMFLOAT3*)&c, xc);
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

	  void GetNormal(const Triangle& t, Vec3& normal)
	  {
		  Vec3 ba = t.A - t.B;
		  Vec3 bc = t.C - t.B;
		  Vec3 product = Cross(ba, bc);
		  if (!TryNormalize(product, normal))
		  {
			  normal = { 0,0,0 };
		  }
	  }

	  void LerpVec3(const Vec3& a, const Vec3& b, float t, Vec3& mixed)
	  {
		  mixed = a * (1.0f - t) + b * t;
	  }

	  void TransformVector(const Matrix4x4& m, const Vec4& v, Vec4& mv)
	  {
		  mv = m * v;
	  }

	  void GetRotationQuat(Degrees theta, float i, float j, float k, Quat& q)
	  {
		  Degrees halfTheta{ 0.5f * theta };

		  float c = Cos(halfTheta);
		  float s = Sin(halfTheta);

		  q.s = c;
		  q.v.x = i * s;
		  q.v.y = j * s;
		  q.v.z = k * s;
	  }

	  void MultiplyQuatByQuat(const Quat& p, const Quat& q, Quat& pq)
	  {
		  XMVECTOR P = XMLoadFloat4((const XMFLOAT4*)&p);
		  XMVECTOR Q = XMLoadFloat4((const XMFLOAT4*)&q);
		  XMVECTOR PQ = XMQuaternionMultiply(P, Q);
		  XMStoreFloat4((XMFLOAT4*) &pq, PQ);
	  }

      void RotateAboutZThetaDegrees(Degrees theta, Matrix4x4 Rz)
      {
          Rz = Matrix4x4::RotateRHAnticlockwiseZ(theta);
      }
   } // Maths
}// Rococo

using namespace Rococo;
using namespace Rococo::Script;
using namespace Rococo::Compiler;

#include "mathsex.vectors.inl"
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
         virtual void AddNativeCalls()
         {
            Sys::Geometry::F32::AddNativeCalls_SysGeometryF32(ss, nullptr);
            Sys::Random::AddNativeCalls_SysRandom(ss, nullptr);
         }

         virtual void ClearResources()
         {
         }

         virtual void Release()
         {
            delete this;
         }
      };
      return new GeomsNativeLib(ss);
   }
}