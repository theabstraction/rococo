#include <rococo.api.h>
#include <rococo.maths.h>

#include <rococo.allocators.h>
#include <new>

namespace
{
	// GuiRectf tree, child components laid out as below:
	//
	//                   y-axis
	//                   ^
	//                   |
	// -------------------------------------
	// | 2               | 3            NE |
	// |                 |                 |
	// |                 |                 |
	// |         <-- loose radius->        |
	// |                 |                 |
	// |                 |                 |
	// |                 | <--- width ---> |
	// ------------------O----------------------> x-axis
	// | 0               | 1               |
	// |                 |                 |
	// |                 |                 |
	// |                 |                 |
	// |                 |                 |
	// |                 |                 |
	// |SW               |                 |
	// -------------------------------------
	// In our loose quadtree each quadrant may include items 100% of the quadrant width from the quadrant centre
	// If an item is included in multiple loose regions of the same size, it may be recorded in each of them.
	// An item with radiius R will not be included in any region of width less than R or width more than 2R.
	// Thus no quadrant will pile lots of small objects into its centre lines

	using namespace Rococo;

	enum QUAD_INDEX
	{
		QUAD_INDEX_SW = 0,
		QUAD_INDEX_SE,
		QUAD_INDEX_NW,
		QUAD_INDEX_NE,
	};

	struct EntityNode
	{
		uint64 entityId;
		EntityNode* next;
		EntityNode* previous;
	};

	struct Quadrant
	{
		Quadrant* quadrant[4];
		Vec2 SW;
		Vec2 NE;
		Vec2 Centre;
		EntityNode* firstEntity;

		float Span() const { return NE.x - SW.x; }
	};

	struct Entry
	{
		mutable Memory::HomogenousAllocator<Quadrant>* quadAllocator;
		mutable Memory::HomogenousAllocator<EntityNode>* nodeAllocator;
		Sphere boundingSphere;
		uint64 id;
		Quadrant* AllocateQuad() const
		{
			return quadAllocator->Allocate();
		}
	};

	struct Search
	{
		IObjectEnumerator* qeCallback;
		const Sphere sphere;
	};

	EntityNode* CreateNode(const Entry& entry)
	{
		EntityNode* en = entry.nodeAllocator->Allocate();
		en->previous = nullptr;
		en->next = nullptr;
		en->entityId = entry.id;
		return en;
	}

	// This function does not recurse
	void InsertItem(Quadrant& q, const Entry& entry)
	{
		if (q.firstEntity == nullptr)
		{
			q.firstEntity = CreateNode(entry);
		}
		else
		{
			for (auto* current = q.firstEntity; true; current = current->next)
			{
				if (current->entityId == entry.id)
				{
					return; // item already in list
				}

				if (current->next == nullptr)
				{
					current->next = CreateNode(entry);
					current->next->previous = current;
					return;
				}
			}
		}
	}

	// This function does not recurse
	void DeleteItem(Quadrant& q, const Entry& entry)
	{
		if (q.firstEntity == nullptr) return;
		if (q.firstEntity->entityId == entry.id)
		{
			auto next = q.firstEntity->next;
			entry.nodeAllocator->Free(q.firstEntity);
			q.firstEntity = next;
			return;
		}

		bool isDeleted = false;

		for (auto* current = q.firstEntity; current != nullptr; )
		{
			auto* next = current->next;
			auto* previous = current->previous;

			if (current->entityId == entry.id)
			{
				if (previous != nullptr)
				{
					previous->next = next;
				}

				if (next != nullptr)
				{
					next->previous = current->previous;
				}

				entry.nodeAllocator->Free(current);	

				isDeleted = true;
			}
			
			current = next;
		}

		if (!isDeleted)
		{
			Throw(0, "Item not found in quadtree: %I64u", entry.id);
		}
	}

	// This function does not recurse
	void DeleteLocalNodes(Quadrant& q, const Entry& entry)
	{
		for (auto* current = q.firstEntity; current != nullptr; )
		{
			auto* next = current->next;
			entry.nodeAllocator->Free(current);
			current = next;
		}

		q.firstEntity = nullptr;
	}

	bool DoSpheresOverlap(Vec2 centre, float maxRangeSq, const Entry& entry)
	{
		float ds2 = Square(entry.boundingSphere.centre.x - centre.x) + Square(entry.boundingSphere.centre.y - centre.y);
		return ds2 < maxRangeSq;
	}

	Quadrant* CreateQuadrant(Vec2 sw, Vec2 ne, Vec2 centre, const Entry& entry)
	{
		Quadrant* q = entry.AllocateQuad();
		new (q) Quadrant{ { nullptr, nullptr, nullptr, nullptr }, sw, ne, centre, nullptr };
		return q;
	}

	void DeleteChild(Quadrant& q, int index, const Entry& entry);
	void DeleteChildren(Quadrant& q, const Entry& entry);

	void AddEntityToTree(Quadrant& quadrant, const Entry& entry)
	{
		float span = quadrant.Span();
		float halfSpan = 0.5f * span;
		if (halfSpan <= entry.boundingSphere.radius)
		{
			InsertItem(quadrant, entry);
			return;
		}
		
		Vec2 centres[4];
		centres[0] = 0.5f * (quadrant.SW + quadrant.Centre);
		centres[1] = centres[0] + Vec2{ halfSpan, 0.0f };
		centres[2] = centres[0] + Vec2{ 0.0f, halfSpan };
		centres[3] = centres[0] + Vec2{ halfSpan, halfSpan };

		int bestCell = -1;
		float bestRadius2 = 1.0e20f;

		for (int i = 0; i < 4; ++i)
		{
			float range2 = Square(entry.boundingSphere.centre.x - centres[i].x) + Square(entry.boundingSphere.centre.y - centres[i].y);
			if (range2 < bestRadius2)
			{
				bestRadius2 = range2;
				bestCell = i;
			}
		}

		if (bestCell == -1) return;
		
		if (!quadrant.quadrant[bestCell])
		{
			float quarterSpan = halfSpan * 0.5f;
			Vec2 SW = centres[bestCell] - Vec2{ quarterSpan, quarterSpan };
			Vec2 NE = centres[bestCell] + Vec2{ quarterSpan, quarterSpan };
			quadrant.quadrant[bestCell] = CreateQuadrant(SW, NE, centres[bestCell], entry);
		}

		AddEntityToTree(*quadrant.quadrant[bestCell], entry);
	}

	inline bool HasDescendants(const Quadrant& q)
	{
		size_t ptrBits = (size_t)q.quadrant[0] | (size_t)q.quadrant[1] | (size_t)q.quadrant[2] | (size_t)q.quadrant[3];
		return ptrBits != 0;
	}

	void DeleteEntityFromTree(Quadrant& quadrant, const Entry& entry)
	{
		float span = quadrant.Span();
		float halfSpan = 0.5f * span;
		if (halfSpan <= entry.boundingSphere.radius)
		{
			DeleteItem(quadrant, entry);
			return;
		}

		Vec2 centres[4];
		centres[0] = 0.5f * (quadrant.SW + quadrant.Centre);
		centres[1] = centres[0] + Vec2{ halfSpan, 0.0f };
		centres[2] = centres[0] + Vec2{ 0.0f, halfSpan };
		centres[3] = centres[0] + Vec2{ halfSpan, halfSpan };

		int bestCell = -1;
		float bestRadius2 = 1.0e20f;

		for (int i = 0; i < 4; ++i)
		{
			float range2 = Square(entry.boundingSphere.centre.x - centres[i].x) + Square(entry.boundingSphere.centre.y - centres[i].y);
			if (range2 < bestRadius2)
			{
				bestRadius2 = range2;
				bestCell = i;
			}
		}

		if (bestCell == -1) return;
		
		if (quadrant.quadrant[bestCell])
		{
			DeleteEntityFromTree(*quadrant.quadrant[bestCell], entry);

			if (quadrant.quadrant[bestCell]->firstEntity == nullptr && !HasDescendants(*quadrant.quadrant[bestCell]))
			{
				DeleteChild(quadrant, bestCell, entry);
			}
		}
	}

	void FindEntitiesInTree(Quadrant& q, Search& criteria)
	{
		for (auto* current = q.firstEntity; current != nullptr; current = current->next)
		{
			criteria.qeCallback->OnId(current->entityId);
		}

		float span = q.Span();
		float halfSpan = 0.5f * span;

		Vec2 centres[4];
		centres[0] = 0.5f * (q.SW + q.Centre);
		centres[1] = centres[0] + Vec2{ halfSpan, 0.0f };
		centres[2] = centres[0] + Vec2{ 0.0f, halfSpan };
		centres[3] = centres[0] + Vec2{ halfSpan, halfSpan };

		float sphereSeparation2 = Square(criteria.sphere.radius + span);

		for (int i = 0; i < 4; ++i)
		{
			auto child = q.quadrant[i];
			if (child)
			{
				float range2 = Square(centres[i].x - criteria.sphere.centre.x) + Square(centres[i].y - criteria.sphere.centre.y);
				if (range2 < sphereSeparation2)
				{
					FindEntitiesInTree(*child, criteria);
				}
			}
		}
	}

	void DeleteChild(Quadrant& q, int index, const Entry& entry)
	{
		auto* child = q.quadrant[index];
		if (child)
		{
			DeleteLocalNodes(*child, entry);
			DeleteChildren(*child, entry);
			child->~Quadrant();
			entry.quadAllocator->Free(child);
			q.quadrant[index] = nullptr;
		}
	}

	void DeleteChildren(Quadrant& q, const Entry& entry)
	{
		for (uint32 i = 0; i < 4; ++i)
		{
			DeleteChild(q, i, entry);
		}
	}

	class LooseQuadTree : public IQuadTreeSupervisor
	{
	private:
		Quadrant world;
		float minBoundingRadius;
		Memory::HomogenousAllocator<Quadrant> quad_allocator;
		Memory::HomogenousAllocator<EntityNode> node_allocator;
	public:
		LooseQuadTree(float _width, float _minBoundingRadius):
			world{ 
				{nullptr, nullptr, nullptr, nullptr}, 
				Vec2 { -0.5f * _width, -0.5f * _width   }, 
				Vec2 {  0.5f * _width,  0.5f * _width   },
				Vec2 {0,0},
				nullptr
			},
			minBoundingRadius(_minBoundingRadius)
		{
		}

		~LooseQuadTree()
		{
			Clear();
		}

		virtual void AddEntity(const Sphere& boundingSphere, uint64 id)
		{
			Entry entry{ &quad_allocator, &node_allocator, boundingSphere, id };
			
			entry.boundingSphere.radius = max(minBoundingRadius, entry.boundingSphere.radius);

			if (boundingSphere.radius >= world.Span())
			{
				InsertItem(world, entry);
				return;
			}

			// We now know that  boundingSphere.radius < world.Span() and can assume as much in AddEntityToTree(...)
			AddEntityToTree(world, entry);
		}

		virtual void Clear()
		{
			Entry entry{ &quad_allocator, &node_allocator, Sphere{ Vec3{ 0,0,0 }, 0.0f }, 0 };
			DeleteChildren(world, entry);
		}

		virtual void DeleteEntity(const Sphere& boundingSphere, uint64 id)
		{
			Entry entry{ &quad_allocator, &node_allocator, boundingSphere, id };
			entry.boundingSphere.radius = max(minBoundingRadius, entry.boundingSphere.radius);

			if (boundingSphere.radius >= world.Span())
			{
				DeleteItem(world, entry);
				return;
			}
			
			// We now know that  boundingSphere.radius < world.Span() and can assume as much in DeleteEntityFromTree(...)
			DeleteEntityFromTree(world, entry);
		}

		virtual void EnumerateItems(const Sphere& boundingSphere, IObjectEnumerator& cb)
		{
			Sphere sphere{ boundingSphere.centre, max(minBoundingRadius, boundingSphere.radius) };
			Search criteria{ &cb, sphere };
			FindEntitiesInTree(world, criteria);
		}

		virtual void GetStats(QuadStats& stats)
		{
			stats.nodesAllocated = node_allocator.NumberOfAllocations();
			stats.nodesFree = node_allocator.NumberOfFreeItems();
			stats.quadsAllocated = quad_allocator.NumberOfAllocations();
			stats.quadsFree = quad_allocator.NumberOfFreeItems();
			stats.nodeAllocSize = sizeof(EntityNode);
			stats.quadAllocSize = sizeof(Quadrant);
		}

		virtual void Free()
		{
			delete this;
		}
	};
}

namespace Rococo
{
	IQuadTreeSupervisor* CreateLooseQuadTree(float width, float minBoundingRadius)
	{
		if (minBoundingRadius <= 0.01f)
		{
			Throw(0, "CreateLooseQuadTree(...): minimum bounding radius was too smal");
		}

		if (width <= 2.0f * minBoundingRadius)
		{
			Throw(0, "The width of the quad tree must be greater than twice the minimum bounding radius for tree entitites");
		}

		if (width > 1.0e9f)
		{
			Throw(0, "Quadtree not tested to function for this width");
		}

		return new LooseQuadTree(width, minBoundingRadius);
	}
}