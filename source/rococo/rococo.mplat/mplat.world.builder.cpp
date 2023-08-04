#include <rococo.mplat.h>
#include <rococo.quadtree.h>
#include <rococo.maths.h>
#include <vector>
#include "rococo.script.types.h"
#include "mplat.components.h"

using namespace Rococo;
using namespace Rococo::Components;
using namespace Rococo::Entities;
using namespace Rococo::Graphics;

namespace
{
	struct World : IWorldSupervisor
	{
		IMeshBuilderSupervisor& meshes;
		IInstancesSupervisor& instances;
		IECS& ecs;
		AutoFree<IQuadtreeSupervisor> quadtree;

		std::vector<TriangleScan> scanlist;

		union Flags
		{
			enum EFlags
			{
				TYPE_ZERO = 0,
				TYPE_TRIANGLE = 0x1,
			};

			uint64 u64Data;
			struct u32x2Data
			{
				uint32 index;
				uint32 u32Flags;
			} u32Data;

			bool IsFlagged(EFlags flags) const
			{
				return (u32Data.u32Flags & flags) != 0;
			}

			uint32 Index() const
			{
				return u32Data.index;
			}
		};

		uint64 MakeFlags(uint32 index, uint32 flags)
		{
			Flags f;
			f.u32Data.index = index;
			f.u32Data.u32Flags = flags;
			return f.u64Data;
		}

		void AddTriangleToQuadtree(const Triangle& t, uint32 index, ID_ENTITY id, Flags::EFlags extraFlags)
		{
			AABB2d box;
			box << Vec2{t.A.x, t.A.y}  << Vec2{ t.B.x, t.B.y } << Vec2{ t.C.x, t.C.y };

			Vec2 fullSpan = box.Span();

			QuadtreeObject obj;
			obj.centre = box.Centre();
			obj.span = max(fullSpan.x, fullSpan.y);
			auto& pocket = quadtree->Insert(obj);
			pocket.context = id.Value();
			pocket.flags = MakeFlags(index, Flags::TYPE_TRIANGLE | extraFlags);
		}

		World(Graphics::IMeshBuilderSupervisor& refMeshes, IECS& _ecs, IInstancesSupervisor& refInstances):
			meshes(refMeshes), instances(refInstances), ecs(_ecs)
		{

		}

		float GetHeightAt(float x, float y, TriangleScan& scan) override
		{
			if (!quadtree) return 0;

			GetTriangleAt(Vec2{ x,y }, scan);
			
			float height = 0;
			GetTriangleHeight(scan.t, Vec2{ x,y }, height);
			return height;
		}

		void GetTriangleAt(const Vec2& position, TriangleScan& t) override
		{
			if (!quadtree) Throw(0, "%s: No quadtree, world not defined.", __FUNCTION__);

			struct ANON : IEventCallback<QuadtreePocket>
			{
				World* world;
				void OnEvent(QuadtreePocket& pocket) override
				{
					world->AppendTriangleForPocket(pocket);
				}
			} cb;
			cb.world = this;

			scanlist.clear();
			quadtree->EnumerateDescendants(position, cb);

			for (auto i = scanlist.rbegin(); i != scanlist.rend(); ++i)
			{
				Triangle2d t2d;
				t2d.A = { i->t.A.x, i->t.A.y };
				t2d.B = { i->t.B.x, i->t.B.y };
				t2d.C = { i->t.C.x, i->t.C.y };
				
				if (t2d.IsInternalOrOnEdge(position))
				{
					t = *i;
					return;
				}
			}

			t.id = ID_ENTITY::Invalid();
			t.idMesh = ID_SYS_MESH::Invalid();
			t.t = { 0 };
		}

		void AppendTriangleForPocket(const QuadtreePocket& pocket)
		{
			Flags f;
			f.u64Data = pocket.flags;
			if (f.IsFlagged(Flags::TYPE_TRIANGLE))
			{
				auto triangleIndex = f.Index();

				ID_ENTITY id{ pocket.context };
				auto body = API::ForIBodyComponent::Get(id);
				if (body)
				{
					auto meshId = body->Mesh();
					size_t nTriangles;
					auto* triangles = meshes.GetTriangles(meshId, nTriangles);
					if (triangleIndex < nTriangles)
					{
						TriangleScan scan;
						const auto& vt = triangles[triangleIndex];
						scan.t.A = vt.a.position;
						scan.t.B = vt.b.position;
						scan.t.C = vt.c.position;
						scan.id = id;
						scan.idMesh = meshId;
						scanlist.push_back(scan);
					}
				}
			}
		}

		void AddMeshToQuadtree(ID_ENTITY id) override
		{
			if (!quadtree) Throw(0, "%s: No quadtree, world not defined.", __FUNCTION__);

			auto body = API::ForIBodyComponent::Get(id);
			if (!body)
			{
				Throw(0, "%s: no entity found with id 0x%llX", __FUNCTION__, id.Value());
			}
				
			auto meshId = body->Mesh();
			if (!meshId)
			{
				Throw(0, "%s: Entity with id 0x%llX had no mesh", __FUNCTION__, id.Value());
			}

			size_t nTriangles;
			auto* triangles = meshes.GetTriangles(meshId, nTriangles);
			if (nTriangles == 0)
			{
				Throw(0, "%s: Entity with id 0x%llX with mesh id 0x%llX has no accessible triangles", __FUNCTION__, id.Value(), meshId.value);
			}

			uint32 index = 0;
			auto* end = triangles + nTriangles;
			for (auto* t = triangles; t < end; ++t)
			{
				Triangle T;
				T.A = t->a.position;
				T.B = t->b.position;
				T.C = t->c.position;
				AddTriangleToQuadtree(T, index++, id, Flags::TYPE_ZERO);
			}
		}

		void New(Metres span, Metres quantum) override
		{
			const float minQuantum = 0.001f;
			const float maxQuantum = 1000.0f;
			const float minAcceptableSpan = 100.0f;
			const float maxAcceptableSpan = 10000000.0f;
			if (span < minAcceptableSpan || span > maxAcceptableSpan)
			{
				Throw(0, "%s: %f <= span <= %f", __FUNCTION__, minAcceptableSpan, maxAcceptableSpan);
			}

			if (quantum < minQuantum || quantum > maxQuantum)
			{
				Throw(0, "%s: %f <= quantum <= %f", __FUNCTION__, minQuantum, maxQuantum);
			}

			if (span < 2.0f * quantum)
			{
				Throw(0, "%s: span must exceeded twice the quantum", __FUNCTION__);
			}

			QuadtreeCreateContext occ;
			occ.centre = { 0,0 };
			occ.minSpan = quantum;
			occ.span = span;
			quadtree = CreateLooseQuadtree(occ);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo
{
	IWorldSupervisor* CreateWorld(IMeshBuilderSupervisor& meshes, IECS& ecs, IInstancesSupervisor& instances)
	{
		return new World(meshes, ecs, instances);
	}
}