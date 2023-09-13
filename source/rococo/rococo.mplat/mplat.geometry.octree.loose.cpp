#include <rococo.api.h>
#include <rococo.maths.h>
#include <rococo.octree.h>
#include <vector>
#include <rococo.memory.freelist.h>

using namespace Rococo;
using namespace Rococo::Memory;

namespace
{
	struct OctreeNode;
	struct LooseOctree;
	class NodeAllocator;
	class PocketAllocator;

	typedef float OCTREAL;
	typedef Vec3 OCTVEC3;

	constexpr OCTREAL HALF = (OCTREAL) 0.5;

	struct ObjectMetrics
	{
		OCTVEC3 centre;
		OCTREAL span;
		OCTVEC3 lowerBottomLeft;
		OCTVEC3 upperTopRight;
	};

	bool IsPointInMetricVolume(const ObjectMetrics& metrics, const OCTVEC3& p)
	{
		return
			p.x >= metrics.lowerBottomLeft.x &&
			p.x <= metrics.upperTopRight.x &&
			p.y >= metrics.lowerBottomLeft.y &&
			p.y <= metrics.upperTopRight.y &&
			p.z >= metrics.lowerBottomLeft.z &&
			p.z <= metrics.upperTopRight.z;
	}

	bool IsObjectSmallerThanContainer(const ObjectMetrics& container, const OctreeObject& object)
	{
		return object.span < container.span;
	}

	enum BoxIndex
	{
		BoxIndex_LowerBottomLeft,
		BoxIndex_LowerBottomRight,
		BoxIndex_LowerTopLeft,
		BoxIndex_LowerTopRight,
		BoxIndex_UpperBottomLeft,
		BoxIndex_UpperBottomRight,
		BoxIndex_UpperTopLeft,
		BoxIndex_UpperTopRight,
	};

	constexpr OCTVEC3 IndexToChildOffset[8]
	{
		{0,0,0},
		{1,0,0},
		{0,1,0},
		{1,1,0},
		{0,0,1},
		{1,0,1},
		{0,1,1},
		{1,1,1}
	};

	void FreeNode(LooseOctree& tree, OctreeNode* node);
	NodeAllocator& Allocator(LooseOctree& tree);

	struct OctreePocketSupervisor
	{
		OctreePocketSupervisor* next;
		OctreePocket data;
		uint64 enumData = 0;

		OctreePocketSupervisor(const OctreeObject& object):
			next(nullptr),
			data { 0, 0, object }
		{

		}

		void FreeChain(PocketAllocator& allocator);
	};

	PocketAllocator& GetPocketAllocator(LooseOctree& tree);

	struct OctreeNode
	{
		const ObjectMetrics metrics;
		OctreeNode* parent;
		OctreeNode* children[8] = { 0 };
		LooseOctree& tree;
		OctreePocketSupervisor* firstPocket = nullptr;

		OctreeNode(LooseOctree& reftree, OctreeNode* pParent, const ObjectMetrics& refMetrics) :
			tree(reftree), parent(pParent), metrics(refMetrics)
		{
		}

		~OctreeNode()
		{
			for (int i = 0; i < 8; ++i)
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

		OctreePocket& InsertAt(const OctreeObject& object);
		OctreeNode* CreateOrGetChild(int index);
	};

	class NodeAllocator : public Rococo::Memory::FreeListAllocator<OctreeNode, 8>
	{
	public:
		OctreeNode* Create(LooseOctree& reftree, OctreeNode* pParent, const ObjectMetrics& metrics)
		{
			auto* node = CreateWith(
				[&](void* pMemory)
				{
					return new (pMemory) OctreeNode(reftree, pParent, metrics);
				}
			);

			return node;
		}
	};

	class PocketAllocator : public Rococo::Memory::FreeListAllocator<OctreePocketSupervisor, 8>
	{
	public:
		OctreePocketSupervisor* Create(const OctreeObject& object)
		{
			auto* pocket = CreateWith(
				[&](void* pMemory)
				{
					return new (pMemory) OctreePocketSupervisor(object);
				}
			);

			return pocket;
		}
	};

	void OctreePocketSupervisor::FreeChain(PocketAllocator& allocator)
	{
		if (next) next->FreeChain(allocator);
		next = nullptr;
		allocator.FreeObject(this);
	}

	OctreePocket& OctreeNode::InsertAt(const OctreeObject& object)
	{
		auto& allocator = GetPocketAllocator(tree);
		auto* pocket = allocator.Create(object);

		if (!firstPocket)
		{
			firstPocket = pocket;
		}
		else
		{
			OctreePocketSupervisor* p = firstPocket;
			for (p = firstPocket; p->next != nullptr; p = p->next)
			{

			}

			p->next = pocket;
		}

		return pocket->data;
	}

	int GetSectorIndex(cr_vec3 sectorCentre, cr_vec3 p)
	{
		int index;

		if (p.x < sectorCentre.x)
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

		if (p.y < sectorCentre.y)
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

		if (p.z < sectorCentre.z)
		{
		}
		else if (p.z > sectorCentre.z)
		{
			index += 4;
		}
		else
		{
			return -1;
		}

		return index;
	}

	OctreePocket* RecursiveInsert(OctreeNode& branch, const OctreeObject& object)
	{
		if (IsObjectSmallerThanContainer(branch.metrics, object))
		{
			if (IsPointInMetricVolume(branch.metrics, object.centre))
			{
				OCTREAL childSpan = HALF * branch.metrics.span;
				if (object.span < childSpan)
				{
					int index = GetSectorIndex(branch.metrics.centre, object.centre);
					if (index >= 0)
					{
						return RecursiveInsert(*branch.CreateOrGetChild(index), object);
					}
				}

				return &branch.InsertAt(object);
			}
		}
		
		return nullptr;
	}

	OctreeNode* OctreeNode::CreateOrGetChild(int index)
	{
		if (children[index] == nullptr)
		{
			const auto& offset = IndexToChildOffset[index];

			float ds = HALF * metrics.span;

			ObjectMetrics childMetrics;
			childMetrics.span = ds;
			childMetrics.lowerBottomLeft = metrics.lowerBottomLeft + offset * ds;
			childMetrics.upperTopRight = childMetrics.lowerBottomLeft + OCTVEC3 {ds, ds, ds};
			childMetrics.centre = HALF * (childMetrics.lowerBottomLeft + childMetrics.upperTopRight);
			auto& allocator = Allocator(tree);
			children[index] = allocator.Create(tree, this, childMetrics);
		}

		return children[index];
	}

	void EnumerateDescendantsRecursive(OctreeNode& node, const Vec3& point, IEventCallback<OctreePocket>& cb)
	{
		for (auto* pocket = node.firstPocket; pocket != nullptr; pocket = pocket->next)
		{
			cb.OnEvent(pocket->data);
		}

		for (int i = 0; i < 8; ++i)
		{
			auto* child = node.children[i];
			if (child)
			{
				if (IsPointInMetricVolume(child->metrics, point))
				{
					EnumerateDescendantsRecursive(*child, point, cb);
				}
			}
		}
	}

	struct LooseOctree : public IOctreeSupervisor
	{
		OctreeCreateContext occ;
		OctreeNode* root = nullptr;

		NodeAllocator nodeAllocator;
		PocketAllocator pocketAllocator;

		LooseOctree(OctreeCreateContext& refocc): occ(refocc)
		{		
			CreateRoot();
		}

		void CreateRoot()
		{
			ObjectMetrics rootMetrics;
			rootMetrics.centre = occ.centre;
			rootMetrics.span = occ.span;
			auto DS = HALF * OCTVEC3{ occ.span, occ.span, occ.span };
			rootMetrics.lowerBottomLeft = -DS;
			rootMetrics.upperTopRight = DS;
			rootMetrics.span = occ.span;

			if (occ.minSpan <= 0.001f || occ.minSpan > 1000.0f)
			{
				Throw(0, "%s: Bad occ.minSpan. Range is 0.001 to 1000.0", __FUNCTION__);
			}

			root = nodeAllocator.Create(*this, nullptr, rootMetrics);
		}

		virtual ~LooseOctree()
		{
			nodeAllocator.FreeObject(root);
		}

		void Clear() override
		{
			nodeAllocator.FreeObject(root);
			CreateRoot();
		}

		void EnumerateDescendants(const Vec3& point, IEventCallback<OctreePocket>& cb) override
		{
			EnumerateDescendantsRecursive(*root, point, cb);
		}

		OctreePocket& Insert(const OctreeObject& object) override
		{
			if (object.span < 0) Throw(0, "%s: bad span", __FUNCTION__);
			if (object.span < occ.minSpan) Throw(0, "%s: bad span. MinSpan is %f", __FUNCTION__, (float) occ.minSpan);

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

	void FreeNode(LooseOctree& tree, OctreeNode* node)
	{
		tree.nodeAllocator.FreeObject(node);
	}

	NodeAllocator& Allocator(LooseOctree& tree)
	{
		return tree.nodeAllocator;
	}

	PocketAllocator& GetPocketAllocator(LooseOctree& tree)
	{
		return tree.pocketAllocator;
	}
}

namespace Rococo
{
	IOctreeSupervisor* CreateLooseOctree(OctreeCreateContext& occ)
	{
		return new LooseOctree(occ);
	}
}