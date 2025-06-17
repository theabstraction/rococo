#include <rococo.api.h>
#include <rococo.maths.h>
#include <rococo.quadtree.h>
#include <vector>
#include <rococo.memory.freelist.h>

using namespace Rococo;
using namespace Rococo::Memory;

namespace
{
	struct QuadtreeNode;
	struct LooseQuadtree;
	class NodeAllocator;
	class PocketAllocator;

	typedef float QUADREAL;
	typedef Vec2 QUADVEC2;

	constexpr QUADREAL HALF = (QUADREAL)0.5;

	struct ObjectMetrics
	{
		QUADVEC2 centre;
		QUADREAL span;
		QUADVEC2 bottomLeft;
		QUADVEC2 topRight;
	};

	bool IsPointInMetricArea(const ObjectMetrics& metrics, const QUADVEC2& p)
	{
		return
			p.x >= metrics.bottomLeft.x &&
			p.x <= metrics.topRight.x &&
			p.y >= metrics.bottomLeft.y &&
			p.y <= metrics.topRight.y;
	}

	bool IsObjectSmallerThanContainer(const ObjectMetrics& container, const QuadtreeObject& object)
	{
		return object.span < container.span;
	}

	enum QuadIndex
	{
		QuadIndex_bottomLeft,
		QuadIndex_bottomRight,
		QuadIndex_topLeft,
		QuadIndex_topRight
	};

	constexpr QUADVEC2 IndexToChildOffset[4]
	{
		{0,0},
		{1,0},
		{0,1},
		{1,1}
	};

	void FreeNode(LooseQuadtree& tree, QuadtreeNode* node);
	NodeAllocator& Allocator(LooseQuadtree& tree);

	struct QuadtreePocketSupervisor
	{
		QuadtreePocketSupervisor* next;
		QuadtreePocket data;
		uint64 enumData = 0;

		QuadtreePocketSupervisor(const QuadtreeObject& object) :
			next(nullptr),
			data{ 0, 0, object }
		{

		}

		void FreeChain(PocketAllocator& allocator);
	};

	PocketAllocator& GetPocketAllocator(LooseQuadtree& tree);

	struct QuadtreeNode
	{
		const ObjectMetrics metrics;
		QuadtreeNode* parent;
		QuadtreeNode* children[4] = { 0 };
		LooseQuadtree& tree;
		QuadtreePocketSupervisor* firstPocket = nullptr;

		QuadtreeNode(LooseQuadtree& reftree, QuadtreeNode* pParent, const ObjectMetrics& refMetrics) :
			tree(reftree), parent(pParent), metrics(refMetrics)
		{
		}

		~QuadtreeNode()
		{
			for (int i = 0; i < 4; ++i)
			{
				FreeNode(tree, children[i]);
				children[i] = nullptr;
			}

			if (firstPocket)
			{
				firstPocket->FreeChain(GetPocketAllocator(tree));
				firstPocket = nullptr;
			}
		}

		QuadtreePocket& InsertAt(const QuadtreeObject& object);
		QuadtreeNode* CreateOrGetChild(int index);
	};

	class NodeAllocator : public Rococo::Memory::FreeListAllocator<QuadtreeNode, 8>
	{
	public:
		QuadtreeNode* Create(LooseQuadtree& reftree, QuadtreeNode* pParent, const ObjectMetrics& metrics)
		{
			auto* node = CreateWith(
				[&](void* pMemory)
				{
					return new (pMemory) QuadtreeNode(reftree, pParent, metrics);
				}
			);

			return node;
		}
	};

	class PocketAllocator : public Rococo::Memory::FreeListAllocator<QuadtreePocketSupervisor, 8>
	{
	public:
		QuadtreePocketSupervisor* Create(const QuadtreeObject& object)
		{
			auto* pocket = CreateWith(
				[&](void* pMemory)
				{
					return new (pMemory) QuadtreePocketSupervisor(object);
				}
			);

			return pocket;
		}
	};

	void QuadtreePocketSupervisor::FreeChain(PocketAllocator& allocator)
	{
		if (next) next->FreeChain(allocator);
		next = nullptr;
		allocator.FreeObject(this);
	}

	QuadtreePocket& QuadtreeNode::InsertAt(const QuadtreeObject& object)
	{
		auto& allocator = GetPocketAllocator(tree);
		auto* pocket = allocator.Create(object);

		if (!firstPocket)
		{
			firstPocket = pocket;
		}
		else
		{
			QuadtreePocketSupervisor* p = firstPocket;
			for (p = firstPocket; p->next != nullptr; p = p->next)
			{

			}

			p->next = pocket;
		}

		return pocket->data;
	}

	int GetSectorIndex(Vec2 sectorCentre, Vec2 p)
	{
		int index;

		if (p.x <= sectorCentre.x)
		{
			index = 0;
		}
		else if (p.x > sectorCentre.x)
		{
			index = 1;
		}
		else
		{
			return -1;
		}

		if (p.y <= sectorCentre.y)
		{
		}
		else if (p.y > sectorCentre.y)
		{
			index += 2;
		}
		else
		{
			return -1;
		}

		return index;
	}

	QuadtreePocket* RecursiveInsert(QuadtreeNode& branch, const QuadtreeObject& object)
	{
		QUADREAL childSpan = HALF * branch.metrics.span;
		if (object.span <= childSpan)
		{
			int index = GetSectorIndex(branch.metrics.centre, object.centre);
			if (index >= 0)
			{
				return RecursiveInsert(*branch.CreateOrGetChild(index), object);
			}
		}

		return &branch.InsertAt(object);
	}

	QuadtreeNode* QuadtreeNode::CreateOrGetChild(int index)
	{
		if (children[index] == nullptr)
		{
			const auto& offset = IndexToChildOffset[index];

			float ds = HALF * metrics.span;

			ObjectMetrics childMetrics;
			childMetrics.span = ds;
			childMetrics.bottomLeft = metrics.bottomLeft + offset * ds;
			childMetrics.topRight = childMetrics.bottomLeft + QUADVEC2{ ds, ds };
			childMetrics.centre = HALF * (childMetrics.bottomLeft + childMetrics.topRight);
			auto& allocator = Allocator(tree);
			children[index] = allocator.Create(tree, this, childMetrics);
		}

		return children[index];
	}

	void EnumerateDescendantsRecursive(QuadtreeNode& node, const Vec2& point, IEventCallback<QuadtreePocket>& cb)
	{
		for (auto* pocket = node.firstPocket; pocket != nullptr; pocket = pocket->next)
		{
			cb.OnEvent(pocket->data);
		}

		for (int i = 0; i < 4; ++i)
		{
			auto* child = node.children[i];
			if (child)
			{
				if (IsPointInMetricArea(child->metrics, point))
				{
					EnumerateDescendantsRecursive(*child, point, cb);
				}
			}
		}
	}

	struct LooseQuadtree : public IQuadtreeSupervisor
	{
		QuadtreeCreateContext occ;
		QuadtreeNode* root = nullptr;

		NodeAllocator nodeAllocator;
		PocketAllocator pocketAllocator;

		LooseQuadtree(QuadtreeCreateContext& refocc) : occ(refocc)
		{
			CreateRoot();
		}

		void CreateRoot()
		{
			ObjectMetrics rootMetrics;
			rootMetrics.centre = occ.centre;
			rootMetrics.span = occ.span;
			auto DS = HALF * QUADVEC2{ occ.span, occ.span };
			rootMetrics.bottomLeft = -DS;
			rootMetrics.topRight = DS;
			rootMetrics.span = occ.span;

			if (occ.minSpan <= 0.001f || occ.minSpan > 1000.0f)
			{
				Throw(0, "%s: Bad occ.minSpan. Range is 0.001 to 1000.0", __ROCOCO_FUNCTION__);
			}

			root = nodeAllocator.Create(*this, nullptr, rootMetrics);
		}

		virtual ~LooseQuadtree()
		{
			nodeAllocator.FreeObject(root);
		}

		void Clear() override
		{
			nodeAllocator.FreeObject(root);
			CreateRoot();
		}

		void EnumerateDescendants(Vec2 point, IEventCallback<QuadtreePocket>& cb) override
		{
			EnumerateDescendantsRecursive(*root, point, cb);
		}

		QuadtreePocket& Insert(const QuadtreeObject& object) override
		{
			if (object.span < 0) Throw(0, "%s: bad span", __ROCOCO_FUNCTION__);
			if (object.span < occ.minSpan) Throw(0, "%s: bad span. MinSpan is %f", __ROCOCO_FUNCTION__, (float)occ.minSpan);

			if (!IsObjectSmallerThanContainer(root->metrics, object))
			{
				return root->InsertAt(object);
			}

			return *RecursiveInsert(*root, object);
		}

		void Free() override
		{
			delete this;
		}

		void GetApproxMemoryUse(size_t& pocketBytes, size_t& nodeBytes) const
		{
			pocketBytes = pocketAllocator.MemoryUse();
			nodeBytes = nodeAllocator.MemoryUse();
		}
	};

	void FreeNode(LooseQuadtree& tree, QuadtreeNode* node)
	{
		tree.nodeAllocator.FreeObject(node);
	}

	NodeAllocator& Allocator(LooseQuadtree& tree)
	{
		return tree.nodeAllocator;
	}

	PocketAllocator& GetPocketAllocator(LooseQuadtree& tree)
	{
		return tree.pocketAllocator;
	}
}

namespace Rococo
{
	IQuadtreeSupervisor* CreateLooseQuadtree(QuadtreeCreateContext& occ)
	{
		return new LooseQuadtree(occ);
	}
}