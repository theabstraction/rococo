#include <rococo.cfgs.h>
#include <vector>
#include <rococo.strings.h>
#include <rococo.time.h>
#include <rococo.os.h>
#include <unordered_map>
#include <atomic>

using namespace Rococo::Editors;
using namespace Rococo::Strings;

namespace Rococo::CFGS::Internal
{
	class TestNode;

	class TestSocket : public ICFGSSocket
	{
	private:
		TestNode* parent;
		SocketPlacement placement;
		DesignerRect designRect;
		HString socketType;
		HString name;
		SocketClass classification;

		// Return the face or vertex where the socket is placed. Conceptually the cable to the socket extends outwards through the specified face
		SocketPlacement Placement() const override
		{
			return placement;
		}

		// Returns the designer bounds for the graph node socket, relative to the bottom left of the node rectangle.
		DesignerRect GetDesignRectangle() const override
		{
			return designRect;
		}

		// Returns the unique typename URL for the socket
		CFGSSocketType Type() const override
		{
			return { socketType };
		}

		SocketClass SocketClassification() const override
		{
			return classification;
		}

		ICFGSNode& ParentNode() const override;

		cstr Name() const override
		{
			return name;
		}
	public:
		TestSocket(TestNode* _parent, SocketPlacement _placement, CFGSSocketType _type, SocketClass _classification, cstr _name): 
			parent(_parent),
			placement(_placement),
			socketType(_type.Value),
			classification(_classification),
			designRect { 0, 0, 150, 100 },
			name(_name)
		{
		}
	};

	static std::atomic<int32> uniqueCounter = 0;

	class TestNode: public ICFGSNode, public IRenderScheme
	{
		HString typeName;
		DesignerRect designRect;

		std::vector<TestSocket*> sockets;

		NodeId uniqueId;

	public:
		TestNode(cstr _typeName, cstr result, DesignerVec2 pos): typeName(_typeName)
		{
			designRect = { pos.x, pos.y, pos.x + 150, pos.y + 100 };

			sockets.push_back(new TestSocket(this, SocketPlacement::Left, CFGSSocketType{ "Flow" }, SocketClass::Trigger, ""));
			sockets.push_back(new TestSocket(this, SocketPlacement::Left, CFGSSocketType{ "Int32" }, SocketClass::InputVar, "A"));
			sockets.push_back(new TestSocket(this, SocketPlacement::Left, CFGSSocketType{ "Int32" }, SocketClass::InputVar, "B"));
			sockets.push_back(new TestSocket(this, SocketPlacement::Right, CFGSSocketType{ "Int32" }, SocketClass::OutputValue, result));

			struct Username : IStringPopulator
			{
				uint64 hash = 0;

				void Populate(cstr text) override
				{
					hash = Strings::XXHash64Arg(text, strlen(text));
				}
			} username;

			OS::GetCurrentUserName(username);

			int32 next = uniqueCounter++;
			int64 next64 = next;

			SafeFormat(uniqueId.subValues.bufValue, "%llX", next64 * (Rococo::Time::UTCTime() ^ username.hash) );
		}

		~TestNode()
		{
			for (auto* s : sockets)
			{
				delete s;
			}
		}

		CFGSNodeType Type() const override
		{
			return { typeName };
		}

		NodeId UniqueId() const override
		{
			return uniqueId;
		}

		const ICFGSSocket& operator[](int32 index)  override
		{
			if (index < 0 || index >= (int32)sockets.size())
			{
				Throw(0, "%s: bad index %d. Socket count is %llu", __FUNCTION__, index, sockets.size());
			}

			return *sockets[index];
		}

		int32 SocketCount() const  override
		{
			return (int32) sockets.size();
		}

		DesignerRect GetDesignRectangle() const override
		{
			return designRect;
		}

		const IRenderScheme& Scheme() const override
		{
			return *this;
		}

		void GetFillColours(OUT ColourSchemeQuantum& q) const override
		{
			q.dullColour = RGBAb(4, 12, 4);
			q.litColour = RGBAb(6, 18, 6);
		}

		void GetTypeNameColours(OUT ColourSchemeQuantum& q) const override
		{
			q.dullColour = RGBAb(0, 0, 0);
			q.litColour = RGBAb(0, 0, 0);
		}

		void GetTypeNamePlateColours(OUT ColourSchemeQuantum& q) const override
		{
			q.dullColour = RGBAb(48, 144, 48);
			q.litColour = RGBAb(64, 192, 64);
		}
	};

	struct NodeHasher
	{
		size_t operator()(const NodeId& id) const noexcept
		{
			return id.subValues.iValues[0] ^ id.subValues.iValues[1];
		}
	};

	class TestNodes: public ICFGSSupervisor, ICFGSNodeEnumerator
	{
	private:
		std::vector<TestNode*> nodes;
		std::unordered_map<NodeId, TestNode*, NodeHasher> mapIdToNode;
		std::vector<TestNode*> zOrderDescending;
	public:
		TestNodes()
		{
			nodes.push_back(new TestNode("Add", "A + B", { 0, 0 }));
			nodes.push_back(new TestNode("Subtract", "A - B", { 200, 0 }));
			nodes.push_back(new TestNode("Multiply", "A x B", { 400, 0 }));
			nodes.push_back(new TestNode("Divide", "A / B", { 600, 0 }));

			mapIdToNode.reserve(nodes.size());
			for (auto* n : nodes)
			{
				zOrderDescending.push_back(n);

				if (!mapIdToNode.insert(std::make_pair(n->UniqueId(), n)).second)
				{
					Throw(0, "A very rare event occured, a 64-bit duplicate hash value was generated!");
				}
			}
		}

		~TestNodes()
		{
			for (auto* n : nodes)
			{
				delete n;
			}
		}

		const ICFGSNode& GetByZOrderAscending(int32 index) override
		{
			int32 count = (int32)nodes.size();

			if (index < 0 || index >= count)
			{
				Throw(0, "%s: bad index %d. Node count is %llu", __FUNCTION__, index, nodes.size());
			}

			return *zOrderDescending[count - index - 1];
		}

		const ICFGSNode& GetByZOrderDescending(int32 index) override
		{
			if (index < 0 || index >= (int32)nodes.size())
			{
				Throw(0, "%s: bad index %d. Node count is %llu", __FUNCTION__, index, nodes.size());
			}

			return *zOrderDescending[index];
		}

		void MakeTopMost(const ICFGSNode& node)
		{
			for (auto i = zOrderDescending.begin(); i != zOrderDescending.end(); i++)
			{
				if (&node == *i)
				{
					std::vector<TestNode*> reorderedNodes;
					reorderedNodes.reserve(zOrderDescending.size());

					reorderedNodes.push_back(*i);

					auto j = zOrderDescending.begin();
					j++;

					for (; j != zOrderDescending.end(); j++)
					{
						if (*j != *i)
						{
							reorderedNodes.push_back(*j);
						}
					}

					zOrderDescending.swap(reorderedNodes);
				}
			}
		}

		const ICFGSNode& operator[](int32 index) override
		{
			if (index < 0 || index >= (int32)nodes.size())
			{
				Throw(0, "%s: bad index %d. Node count is %llu", __FUNCTION__, index, nodes.size());
			}

			return *nodes[index];
		}

		int32 Count() const override
		{
			return (int32) nodes.size();
		}

		const ICFGSNode* FindNode(NodeId id) const override
		{
			auto i = mapIdToNode.find(id);
			return i != mapIdToNode.end() ? i->second : nullptr;
		}

		void Free() override
		{
			delete this;
		}

		ICFGSNodeEnumerator& Nodes() override
		{
			return *this;
		}
	};

	ICFGSNode& TestSocket::ParentNode() const
	{
		return *parent;
	}
}

namespace Rococo::CFGS
{
	CFGS_MARSHALLER_API ICFGSSupervisor* CreateCFGSTestSystem()
	{
		return new Internal::TestNodes();
	}
}