#include <rococo.mplat.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace ANON
{
	void SetNormala(QuadVertices& qv, cr_vec3 n)
	{
		qv.normals.a = qv.normals.b = qv.normals.c = qv.normals.d = n;
	}

	struct ObjectQuad
	{
		ObjectVertex a;
		ObjectVertex b;
		ObjectVertex c;
		ObjectVertex d;
	};

	struct RodTesselator : public IRodTesselator
	{
		Platform& platform;

		bool smoothNormals = true;
		MaterialVertexData top { RGBAb(255, 255, 255), 0, 0 };
		MaterialVertexData middle { RGBAb(128, 128, 128), 0, 0.1f };
		MaterialVertexData bottom{ RGBAb(0, 0, 0), 0, 0.2f };
		float uvScale = 1.0f;

		std::vector<VertexTriangle>  triangles;

		Vec3 direction{ 0, 0, 1 };
		Vec3 bitangent{ 1, 0, 0 };
		Vec3 origin { 0, 0, 0 };

		void AddQuad(const ObjectQuad& q)
		{
			triangles.push_back({ q.a, q.b, q.c });
			triangles.push_back({ q.c, q.d, q.a });
		}

		RodTesselator(Platform& _platform):
			platform(_platform)
		{

		}

		void AddBox(Metres length, const Vec2& a, const Vec2& b, const Vec2& c, const Vec2& d) override
		{
			float z0 = origin.z;
			float z1 = z0 + length;
 
			Vec3 usw { d.x, d.y, z1 };
			Vec3 use { c.x, c.y, z1 };
			Vec3 unw { a.x, a.y, z1 };
			Vec3 une { b.x, b.y, z1 };
			Vec3 lse { c.x, c.y, z0 };
			Vec3 lsw { d.x, d.y, z0 };
			Vec3 lne { b.x, b.y, z0 };
			Vec3 lnw { a.x, a.y, z0 };

			float vB = 0;
			float vT = length * uvScale;
			float uSW = 0;
			float uSE = Length(lse - lsw) * uvScale;

			MaterialVertexData topMat = { top.colour, middle.materialId, top.gloss };
			MaterialVertexData botMat = { bottom.colour, middle.materialId, bottom.gloss };

			Vec3 Ns{ 0,-1, 0 };

			ObjectQuad south
			{
				{usw,Ns,{uSW, vT }, topMat },
				{use,Ns,{uSE, vT }, topMat },
				{lse,Ns,{uSE, vB }, botMat },
				{lsw,Ns,{uSW, vB }, botMat },
			};

			AddQuad(south);

			float uNE = uSE + Length(une - use) * uvScale;

			Vec3 Ne{ 1, 0, 0 };
			ObjectQuad east
			{
				{ use,Ne,{ uSE, vT }, topMat },
				{ une,Ne,{ uNE, vT }, topMat },
				{ lne,Ne,{ uNE, vB }, botMat },
				{ lse,Ne,{ uSE, vB }, botMat },
			};
			AddQuad(east);

			Vec3 Nn{ 0,1, 0 };

			float uNW = uNE + Length(une - unw) * uvScale;

			ObjectQuad north
			{
				{ une,Nn,{ uNE, vT }, topMat },
				{ unw,Nn,{ uNW, vT }, topMat },
				{ lnw,Nn,{ uNW, vB }, botMat },
				{ lne,Nn,{ uNE, vB }, botMat },
			};
			AddQuad(north);

			float uE = uNW + Length(une - use) * uvScale;

			Vec3 Nw{ -1, 0, 0 };

			ObjectQuad west
			{ 
				{ unw, Nw, {uNW, vT}, topMat },
				{ usw, Nw, {uE,  vT}, topMat },
				{ lsw, Nw, {uE,  vB}, botMat },
				{ lnw, Nw, {uNW, vB}, botMat }
			};
			AddQuad(west);

			Vec3 Nl{ 0,0,-1 };

			float du = Length(lnw - lne) * uvScale;
			float dv = Length(lnw - lsw) * uvScale;

			ObjectQuad lower
			{
				{ lsw, Nl,{ 0,  dv }, bottom },
				{ lse, Nl,{ du, dv }, bottom },
				{ lne, Nl,{ du,  0 }, bottom },
				{ lnw, Nl,{ 0,   0 }, bottom }
			};
			AddQuad(lower);

			Vec3 Nu{ 0,0,1 };

			ObjectQuad upper
			{ 
				{ unw,  Nu,{ 0,   0 }, top },
				{ une,  Nu,{ du,  0 }, top },
				{ use,  Nu,{ du,  dv }, top },
				{ usw,  Nu,{ 0,   dv }, top },
			};

			AddQuad(upper);
		}

		void AddPrism(Metres length, const Vec2& a, const Vec2& b, const Vec2& c) override
		{

		}

		void AddSphere(Metres radius, int32 nRings, int32 nDivs) override
		{

		}

		void AddTube(Metres length, Metres bottomRadius, Metres topRadius, int32 nDivs) override
		{

		}

		void Clear() override
		{
			triangles.clear();
			direction = { 0, 0, 1 };
			bitangent = { 1, 0, 0 };
			origin = { 0,0,0 };
		}

		void Destruct() override
		{
			delete this;
		}

		boolean32 PopNextTriangle(VertexTriangle& t) override
		{
			if (triangles.empty())
			{
				t.a = t.b = t.c = { 0 };
				return false;
			}

			t = triangles.back();
			triangles.pop_back();
			return true;
		}

		void CopyToMeshBuilder(const fstring& meshName)
		{
			platform.meshes.Begin(meshName);

			for (auto& t : triangles)
			{
				platform.meshes.AddTriangleEx(t);
			}

			platform.meshes.End();
		}

		void SetMaterialBottom(MaterialVertexData& bottom) override
		{
			this->bottom = bottom;
		}

		void SetMaterialMiddle(MaterialVertexData& middle) override
		{
			this->middle = middle;
		}

		void SetMaterialTop(MaterialVertexData& top) override
		{
			this->top = top;
		}

		void UseFaceNormals() override
		{
			smoothNormals = false;
		}

		void UseSmoothNormals() override
		{
			smoothNormals = true;
		}

		void SetUVScale(float uvScale) override
		{
			this->uvScale = uvScale;
		}
	};
}

namespace Rococo
{
	namespace Graphics
	{
		IRodTesselator* CreateRodTesselator(Platform& platform)
		{
			return new ANON::RodTesselator(platform);
		}
	}
}
