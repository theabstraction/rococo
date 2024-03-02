#include <rococo.cfgs.h>
#include <vector>
#include <rococo.strings.h>

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

	class TestNode: public ICFGSNode
	{
		HString typeName;
		DesignerRect designRect;

		std::vector<TestSocket*> sockets;

	public:
		TestNode(cstr _typeName, cstr result, DesignerVec2 pos): typeName(_typeName)
		{
			designRect = { pos.x, pos.y, pos.x + 150, pos.y + 100 };

			sockets.push_back(new TestSocket(this, SocketPlacement::Left, CFGSSocketType{ "Flow" }, SocketClass::Trigger, ""));
			sockets.push_back(new TestSocket(this, SocketPlacement::Left, CFGSSocketType{ "Int32" }, SocketClass::InputVar, "A"));
			sockets.push_back(new TestSocket(this, SocketPlacement::Left, CFGSSocketType{ "Int32" }, SocketClass::InputVar, "B"));
			sockets.push_back(new TestSocket(this, SocketPlacement::Right, CFGSSocketType{ "Int32" }, SocketClass::OutputValue, result));
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
	};

	class TestNodes: public ICFGSSupervisor, ICFGSNodeEnumerator
	{
	private:
		std::vector<TestNode*> nodes;

	public:
		TestNodes()
		{
			nodes.push_back(new TestNode("Add", "A + B", { 0, 0 }));
			nodes.push_back(new TestNode("Subtract", "A - B", { 200, 0 }));
			nodes.push_back(new TestNode("Multiply", "A x B", { 400, 0 }));
			nodes.push_back(new TestNode("Divide", "A / B", { 600, 0 }));
		}

		~TestNodes()
		{
			for (auto* n : nodes)
			{
				delete n;
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