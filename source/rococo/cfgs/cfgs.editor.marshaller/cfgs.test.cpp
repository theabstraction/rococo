#include <rococo.cfgs.h>
#include <vector>
#include <rococo.strings.h>
#include <rococo.time.h>
#include <rococo.os.h>
#include <unordered_map>

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
		SocketId id;
		std::vector<CableId> cables;
	public:
		TestSocket(TestNode* _parent, SocketPlacement _placement, CFGSSocketType _type, SocketClass _classification, cstr _name, SocketId _id) :
			parent(_parent),
			placement(_placement),
			socketType(_type.Value),
			classification(_classification),
			designRect{ 0, 0, 150, 100 },
			name(_name)
		{
			id = _id ? _id : SocketId{ Rococo::MakeNewUniqueId() };
		}

		void AddCable(CableId id) override
		{
			cables.push_back(id);
		}

		void Clear()
		{
			cables.clear();
		}

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

		SocketId Id() const override
		{
			return id;
		}

		int CableCount() const override
		{
			return (int32) cables.size();
		}

		CableId GetCable(int32 index) const override
		{
			if (index < 0 || index >= cables.size())
			{
				Throw(0, "%s: index (%d) out of bounds. Size is %llu", __FUNCTION__, index, cables.size());
			}

			return cables[index];
		}

		mutable GuiRect lastCircleRect{ -1, -1, -1, -1 };
		mutable Vec2i lastEdgePoint{ -1, -1 };

		void GetLastGeometry(OUT GuiRect& lastCircleRect, OUT Vec2i& lastEdgePoint) const
		{
			lastCircleRect = this->lastCircleRect;
			lastEdgePoint = this->lastEdgePoint;
		}

		void SetLastGeometry(const GuiRect& circleRect, Vec2i edgePoint) const override
		{
			lastCircleRect = circleRect;
			lastEdgePoint = edgePoint;
		}
	};

	class TestNode : public ICFGSNode, public IRenderScheme, public ICFGSNodeBuilder
	{
		HString typeName;
		DesignerRect designRect;

		std::vector<TestSocket*> sockets;

		NodeId uniqueId;

		int inputCount = 0;
		int outputCount = 0;
	public:
		TestNode(cstr _typeName, DesignerVec2 pos, NodeId id) : typeName(_typeName)
		{
			designRect = { pos.x, pos.y, pos.x + 150, pos.y + 100 };
			uniqueId = id ? id : NodeId{ Rococo::MakeNewUniqueId() };
		}

		void AddTestSockets(cstr result)
		{
			sockets.push_back(new TestSocket(this, SocketPlacement::Left, CFGSSocketType{ "Flow" }, SocketClass::Trigger, "Start", SocketId()));
			sockets.push_back(new TestSocket(this, SocketPlacement::Left, CFGSSocketType{ "Int32" }, SocketClass::InputVar, "A", SocketId()));
			sockets.push_back(new TestSocket(this, SocketPlacement::Left, CFGSSocketType{ "Int32" }, SocketClass::InputVar, "B", SocketId()));
			sockets.push_back(new TestSocket(this, SocketPlacement::Right, CFGSSocketType{ "Flow" }, SocketClass::Exit, "End", SocketId()));
			sockets.push_back(new TestSocket(this, SocketPlacement::Right, CFGSSocketType{ "Int32" }, SocketClass::OutputValue, result, SocketId()));
		}

		void AddSocket(cstr type, SocketClass socketClass, cstr label, SocketId id) override
		{
			sockets.push_back(new TestSocket(this, SocketPlacement::Left, CFGSSocketType{ type }, socketClass, label, id));
			
			switch(socketClass)
			{
			case SocketClass::None:
				break;
			case SocketClass::Trigger:
			case SocketClass::InputVar:
			case SocketClass::InputRef:
			case SocketClass::ConstInputRef:
				inputCount++;
				break;
			case SocketClass::OutputRef:
			case SocketClass::ConstOutputRef:
			case SocketClass::OutputValue:
			case SocketClass::Exit:
				outputCount++;
				break;
			};

			int nSlotHorizontals = 2 + max(inputCount, outputCount);

			int height = nSlotHorizontals * 20;

			designRect.bottom = designRect.top + height;
		}

		void ClearSockets()
		{
			for (auto* s : sockets)
			{
				s->Clear();
			}
		}

		~TestNode()
		{
			for (auto* s : sockets)
			{
				delete s;
			}
		}

		ICFGSSocket* FindSocket(SocketId id) override
		{
			for (auto* s : sockets)
			{
				if (s->Id() == id)
				{
					return s;
				}
			}

			return nullptr;
		}

		ICFGSSocket* FindSocket(cstr name) override
		{
			for (auto* s : sockets)
			{
				if (Strings::Eq(s->Name(), name))
				{
					return s;
				}
			}

			return nullptr;
		}

		CableConnection EntryPoint(int index) const
		{
			int i = 0;

			for (auto* socket : sockets)
			{
				if (socket->SocketClassification() == SocketClass::Trigger)
				{
					if (i == index)
					{
						return CableConnection{ uniqueId, socket->Id() };
					}

					i++;
				}
			}

			return CableConnection();
		}

		CableConnection ExitPoint(int index) const
		{
			int i = 0;

			for (auto* socket : sockets)
			{
				if (socket->SocketClassification() == SocketClass::Exit)
				{
					if (i == index)
					{
						return CableConnection{ uniqueId, socket->Id() };
					}

					i++;
				}
			}

			return CableConnection();
		}

		CFGSNodeType Type() const override
		{
			return { typeName };
		}

		NodeId UniqueId() const override
		{
			return uniqueId;
		}

		const ICFGSSocket& operator[](int32 index) const override
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

		// Gives the delta offset of the reported design rectangle. This enables dragging the node and reverting to the original position
		DesignerVec2 currentOffset{ 0,0 };

		DesignerRect GetDesignRectangle() const override
		{
			return
			{ 
				designRect.left + currentOffset.x,
				designRect.top + currentOffset.y,
				designRect.right + currentOffset.x,
				designRect.bottom + currentOffset.y
			};
		}

		void SetDesignOffset(const DesignerVec2& offset, bool makePermanent) override
		{
			if (makePermanent)
			{
				designRect.left += currentOffset.x;
				designRect.top += currentOffset.y;
				designRect.right += currentOffset.x;
				designRect.bottom += currentOffset.y;

				currentOffset = { 0, 0 };
			}
			else
			{
				currentOffset = offset;
			}
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

	class CableImpl: public ICFGSCable
	{
		CableConnection exitPoint;
		CableConnection entryPoint;
		CableId id;
		bool isSelected = false;
	public:
		CableImpl(const CableConnection& _exitPoint):
			exitPoint( _exitPoint)
		{
			id.id = Rococo::MakeNewUniqueId();
		}

		void SetEntryPoint(const CableConnection& entryPoint)
		{
			this->entryPoint = entryPoint;
		}

		CableConnection ExitPoint() const override
		{
			return exitPoint;
		}

		CableConnection EntryPoint() const override
		{
			return entryPoint;
		}

		CableId Id() const override
		{
			return id;
		}

		bool IsSelected() const
		{
			return isSelected;
		}

		void Select(bool value)
		{
			isSelected = value;
		}
	};

	class CableSet: public ICFGSCableEnumerator
	{
		std::vector<CableImpl*> cables;

	public:
		CableImpl& AddNew(const CableConnection& exitPoint)
		{
			auto* newCable = new CableImpl(exitPoint);
			cables.push_back(newCable);
			return *newCable;
		}

		void Add(NodeId startNode, SocketId startSocket, NodeId endNode, SocketId endSocket) override
		{
			CableConnection start{ startNode, startSocket };
			auto& cable = AddNew(start);

			CableConnection end{ endNode, endSocket };
			cable.SetEntryPoint(end);
		}

		~CableSet()
		{
			Clear();
		}

		void Clear()
		{
			for (auto* cable : cables)
			{
				delete cable;
			}

			cables.clear();
		}

		int32 Count() const override
		{
			return (int32) cables.size();
		}

		void Delete(int32 index)
		{
			if (index < 0 || index >+ (int32)cables.size())
			{
				return;
			}

			delete cables[index];
			cables[index] = nullptr;

			for (int32 i = index; i < cables.size() - 1; i++)
			{
				cables[i] = cables[i + 1];
			}

			cables.pop_back();
		}

		const ICFGSCable& operator[](int32 index) const override
		{
			if (index < 0 || index >= cables.size())
			{
				Throw(0, "%s: bad index %d. Size is %llu", __FUNCTION__, index, cables.size());
			}

			return *cables[index];
		}

		void VisuallySelect(int32 index, OUT bool& changed) override
		{
			if (index >= 0 && index < (int32)cables.size())
			{
				if (cables[index]->IsSelected())
				{
					changed = false;
					return;
				}
			}

			bool atLeastOnePreviousSelection = false;

			for (auto* c : cables)
			{
				if (c->IsSelected())
				{
					atLeastOnePreviousSelection = true;
				}

				c->Select(false);
			}

			if (index >= 0 && index < (int32)cables.size())
			{
				cables[index]->Select(true);
				changed = true;
				return;
			}

			if (atLeastOnePreviousSelection)
			{
				changed = true;
			}
			else
			{
				changed = false;
			}
		}
	};

	class TestNodes: public ICFGSDatabaseSupervisor, ICFGSNodeEnumerator, ICFGSNodeSetBuilder
	{
	private:
		std::vector<TestNode*> nodes;
		std::unordered_map<NodeId, TestNode*, NodeId::Hasher> mapIdToNode;
		std::vector<TestNode*> zOrderDescending;
		CableSet cables;

		typedef std::unordered_map<FunctionId, int, FunctionId::Hasher> TFunctions;
		TFunctions mapIdToFunction;
	public:
		TestNodes()
		{
		}

		~TestNodes()
		{
			DeleteAllNodes();
		}

		ICFGSNodeSetBuilder& Builder() override
		{
			return *this;
		}

		FunctionId CreateFunction() override
		{
			TFunctions::iterator i;

			for(;;)
			{
				FunctionId id{ MakeNewUniqueId() };
				auto insertion = mapIdToFunction.insert(std::make_pair(id, 0));
				if (insertion.second)
				{
					// Insertion took place, thus id must be unique to the collection
					i = insertion.first;
					break;
				}
			}

			return i->first;
		}

		void DeleteFunction(FunctionId id) override
		{
			auto i = mapIdToFunction.find(id);
			if (i != mapIdToFunction.end())
			{
				mapIdToFunction.erase(i);
			}
		}

		void BuildFunction(FunctionId id) override
		{
			auto i = mapIdToFunction.find(id);
			if (i == mapIdToFunction.end())
			{
				Throw(0, "%s: No such function id {%X, %X}", __FUNCTION__, id.id.iValues[0], id.id.iValues[1]);
			}
		}

		void ConnectCablesToSockets() override
		{
			for (size_t i = 0; i < nodes.size(); i++)
			{
				nodes[i]->ClearSockets();
			}

			for (int32 i = 0; i < cables.Count(); ++i)
			{
				auto& cable = cables[i];
				auto start = cable.ExitPoint();
				auto end = cable.EntryPoint();

				auto* pStartNode = FindNode(start.node);
				if (!pStartNode)
				{
					Throw(0, "Cannot find the start node for cable %d", i);
				}

				auto* pEndNode = FindNode(end.node);
				if (!pEndNode)
				{
					Throw(0, "Cannot find the end node for cable %d", i);
				}

				auto* pStartSocket = pStartNode->FindSocket(start.socket);
				if (!pStartSocket)
				{
					Throw(0, "Cannot find the start socket for cable %d", i);
				}

				auto* pEndSocket = pEndNode->FindSocket(end.socket);
				if (!pEndSocket)
				{
					Throw(0, "Cannot find the start socket for cable %d", i);
				}

				pStartSocket->AddCable(cable.Id());
				pEndSocket->AddCable(cable.Id());
			}
		}

		void DeleteCable(int32 cableIndex)
		{
			cables.Delete(cableIndex);
			ConnectCablesToSockets();
		}

		void DeleteAllNodes() override
		{
			for (auto* n : nodes)
			{
				delete n;
			}

			nodes.clear();
			cables.Clear();
			mapIdToNode.clear();
			zOrderDescending.clear();
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


					for (auto j = zOrderDescending.begin(); j != zOrderDescending.end(); j++)
					{
						if (*j != *i)
						{
							reorderedNodes.push_back(*j);
						}
					}

					zOrderDescending.swap(reorderedNodes);
					return;
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

		ICFGSNodeBuilder& AddNode(cstr typeString, const Rococo::Editors::DesignerVec2& topLeft, NodeId id) override
		{
			auto* node = new TestNode(typeString, topLeft, id);
			nodes.push_back(node);
			zOrderDescending.push_back(node);

			if (!mapIdToNode.insert(std::make_pair(node->UniqueId(), node)).second)
			{
				Throw(0, "A very rare event occured, a 128-bit duplicate hash value was generated!");
			}

			return *node;
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

		ICFGSNode* FindNode(NodeId id) override
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

		ICFGSCableEnumerator& Cables() override
		{
			return cables;
		}
	};

	ICFGSNode& TestSocket::ParentNode() const
	{
		return *parent;
	}
}

namespace Rococo::CFGS
{
	CFGS_MARSHALLER_API ICFGSDatabaseSupervisor* CreateCFGSTestSystem()
	{
		return new Internal::TestNodes();
	}
}