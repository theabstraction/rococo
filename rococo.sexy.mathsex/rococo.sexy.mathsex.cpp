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

      float AnyOf(float minValue, float maxValue)
      {
         float32 range = maxValue - minValue;
         float f = rng() / (float) rng.max();
         return range * f + minValue;
      }
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

	  void ScaleVector3(const Vec3& a, float f, Vec3& fa)
	  {
		  fa = a * f;
	  }

	  void ScaleVector3(float f, const Vec3& a, Vec3& fa)
	  {
		  fa = a * f;
	  }

      void MultiplyMatrixByRef(const Matrix4x4& a, const Matrix4x4& b, Matrix4x4& c)
      {
         Multiply(c, a, b);
      }

      float Dot(const Vec3& a, const Vec3& b)
      {
         return a.x * b.x + a.y * b.y + a.z * b.z;
      }

      float Length(const Vec3& a)
      {
         float ds2 = Rococo::Maths::Dot(a, a);
         float ds = sqrtf(ds2);
         return ds;
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
         float ds = Rococo::Maths::Length(a);
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
   }
}

using namespace Rococo;
using namespace Rococo::Script;
using namespace Rococo::Compiler;

#include "mathsex.vectors.inl"
#include "mathsex.random.inl"
#include "mathsex.time.inl"

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
            Sys::Time::AddNativeCalls_SysTime(ss, nullptr);
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