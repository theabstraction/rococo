#include <rococo.cfgs.h>
#include <rococo.strings.h>
#include <rococo.time.h>
#include <rococo.os.h>
#include <rococo.hashtable.h>
#include <rococo.functional.h>
#include <rococo.properties.h>
#include <rococo.validators.h>
#include <rococo.events.h>
#include <vector>
#include <algorithm>

using namespace Rococo::Editors;
using namespace Rococo::Reflection;
using namespace Rococo::Strings;

namespace Rococo::CFGS::Internal
{
	struct CFGSNode;

	struct CFGSSocket : public ICFGSSocket
	{
		CFGSNode* parent;
		SocketPlacement placement;
		DesignerRect designRect;
		HString socketType;
		HString name;
		SocketClass classification;
		SocketId id;
		std::vector<CableId> cables;
		mutable RGBAb colour;
		mutable RGBAb hilightColour;

		CFGSSocket(CFGSNode* _parent, SocketPlacement _placement, CFGSSocketType _type, SocketClass _classification, cstr _name, SocketId _id) :
			parent(_parent),
			placement(_placement),
			socketType(_type.Value),
			classification(_classification),
			designRect{ 0, 0, 150, 100 },
			name(_name)
		{
			id = _id ? _id : SocketId{ Rococo::Ids::MakeNewUniqueId() };

			switch (SocketClassification())
			{
			case SocketClass::Trigger:
			case SocketClass::Exit:
				hilightColour = RGBAb(0, 255, 0);
				colour = RGBAb(0, 192, 0);
				break;
			case SocketClass::InputRef:
			case SocketClass::InputVar:
			case SocketClass::ConstInputRef:
				hilightColour = RGBAb(0, 255, 255);
				colour = RGBAb(0, 192, 192);
				break;
			default:
				hilightColour = RGBAb(128, 128, 255);
				colour = RGBAb(96, 96, 192);
				break;
			}
		}

		void AcceptVisitor(IPropertyVisitor& visitor, uint64, IPropertyUIEvents&)
		{
			visitor.VisitHeader("class", "Classification", ToString(classification));
			visitor.VisitHeader("type", "SocketType", socketType);
			visitor.VisitHeader("name", "Name", name);
			visitor.VisitHeader("placement", "Placement", ToString(placement));
		}

		void AddCable(CableId id) override
		{
			cables.push_back(id);
		}

		RGBAb GetSocketColour(bool isLit) const override
		{
			return isLit ? hilightColour : colour;
		}

		void SetColours(RGBAb normalColour, RGBAb litColour) const override
		{
			this->colour = normalColour;
			this->hilightColour = litColour;
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

		void SetName(cstr name)
		{
			this->name = name;
		}

		void SetType(CFGSSocketType type)
		{
			this->socketType = type.Value;
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

		mutable stringmap<HString> fields;

		void SetField(cstr fieldName, cstr fieldValue) const override
		{
			fields[fieldName] = fieldValue;
		}

		bool TryGetField(cstr fieldName, IStringPopulator& populator) const override
		{
			auto i = fields.find(fieldName);
			if (i != fields.end())
			{
				populator.Populate(i->second);
				return true;
			}

			return false;
		}

		size_t EnumerateFields(Rococo::Function<void(cstr fieldName, cstr fieldValue, size_t index)> callback) const override
		{
			size_t index = 0;

			for (auto i : fields)
			{
				callback.Invoke(i.first, i.second, index++);
			}

			return fields.size();
		}
	};

	struct CFGSNode : public ICFGSNode, public IRenderScheme, public ICFGSNodeBuilder
	{
		HString typeName;
		DesignerRect designRect;

		std::vector<CFGSSocket*> sockets;

		NodeId uniqueId;

		int inputCount = 0;
		int outputCount = 0;

		CFGSNode(cstr _typeName, DesignerVec2 pos, NodeId id) : typeName(_typeName)
		{
			designRect = { pos.x, pos.y, pos.x + 150, pos.y + 100 };
			uniqueId = id ? id : NodeId{ Rococo::Ids::MakeNewUniqueId() };
		}

		void AcceptVisitor(IPropertyVisitor& visitor, IPropertyUIEvents& events)
		{
			ArrayHeaderControl socketHeader;
			socketHeader.isElementDeleteSupported = false;
			socketHeader.isExpandable = false;
			socketHeader.isOrderImmutable = true;
			socketHeader.minElements = 0;
			socketHeader.maxElements = 32'768;

			PropertyMarshallingStub socketStub{ "sockets", "Sockets", events };
			visitor.BeginArray(socketStub, socketHeader);

			VisitPointerArray(visitor, events, sockets);

			visitor.EndArray();
		}

		void AddField(cstr name, cstr value, SocketId socketId) override
		{
			auto* s = FindSocket(socketId);
			if (s)
			{
				s->SetField(name, value);
			}
		}

		ICFGSSocket& AddSocket(cstr type, SocketClass socketClass, cstr label, SocketId id) override
		{
			auto* s = new CFGSSocket(this, SocketPlacement::Left, CFGSSocketType{ type }, socketClass, label, id);
			sockets.push_back(s);
			
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

			return *s;
		}

		void DeleteSocket(SocketId id)
		{
			for (size_t i = 0; i < sockets.size(); i++)
			{
				auto* s = sockets[i];
				if (s->Id() == id)
				{
					delete s;

					for (size_t j = i + 1; j < sockets.size(); j++)
					{
						sockets[j-1] = sockets[j];
					}

					sockets.pop_back();
					return;
				}
			}			
		}

		void ClearSockets()
		{
			for (auto* s : sockets)
			{
				s->Clear();
			}
		}

		~CFGSNode()
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

		NodeId Id() const override
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
			id.id = Rococo::Ids::MakeNewUniqueId();
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

		void Delete(NodeId id)
		{
			auto end = std::remove_if(cables.begin(), cables.end(),
				[&id](const CableImpl* cable)
				{
					return cable->EntryPoint().node == id || cable->ExitPoint().node == id;
				}
			);

			cables.erase(end, cables.end());
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

	struct CFGSArgumentEventHandler : IPropertyUIEvents
	{
		void OnBooleanButtonChanged(IPropertyEditor& property) override
		{
			UNUSED(property);
		}

		void OnPropertyEditorLostFocus(IPropertyEditor& property) override
		{
			UNUSED(property);
		}

		void OnDependentVariableChanged(cstr propertyId, IEstateAgent& agent) override
		{
			UNUSED(propertyId);
			UNUSED(agent);
		}
	};

	ROCOCO_INTERFACE ICFGSDatabaseSupervisorInternal : ICFGSDatabaseSupervisor
	{
		virtual IPropertyUIEvents& InputArgumentHandler() = 0;
		virtual IPropertyUIEvents& OutputArgumentHandler() = 0;
	};

	class CFGSFunction: public ICFGSFunction, ICFGSNodeEnumerator, ICFGSNodeSetBuilder, IPropertyVenue
	{
		std::vector<CFGSNode*> nodes;
		std::unordered_map<NodeId, CFGSNode*, NodeId::Hasher> mapIdToNode;
		std::vector<CFGSNode*> zOrderDescending;
		CableSet cables;
		mutable HString name; // we lazily evaluate name with [cstr Name() const] when it has not been defined elsewhere
		FunctionId id;

		CFGSNode beginNode;
		CFGSNode returnNode;

		ICFGSDatabaseSupervisorInternal& db;

		IEnumDescriptor* inputTypeOptions = nullptr;
		IEnumDescriptor* outputTypeOptions = nullptr;
	public:
		CFGSFunction(ICFGSDatabaseSupervisorInternal& _db, FunctionId _id) :
			id(_id),
			beginNode("_Begin",DesignerVec2{ 0, 0 }, NodeId{ Ids::MakeNewUniqueId() }),
			returnNode("_Return", DesignerVec2 { 200 , 200}, NodeId { Ids::MakeNewUniqueId() }),
			db(_db)
		{
			
		}

		~CFGSFunction()
		{
			DeleteAllNodes();
		}

		void SetInputTypeOptions(Rococo::Reflection::IEnumDescriptor* inputTypes)
		{
			this->inputTypeOptions = inputTypes;
		}

		void SetOutputTypeOptions(Rococo::Reflection::IEnumDescriptor* outputTypes)
		{
			this->outputTypeOptions = outputTypes;
		}

		CFGSNode& BeginNode()
		{
			return beginNode;
		}

		const ICFGSNode& BeginNode() const override
		{
			return beginNode;
		}

		CFGSNode& ReturnNode()
		{
			return returnNode;
		}

		const ICFGSNode& ReturnNode() const override
		{
			return beginNode;
		}

		FunctionId Id() const override
		{
			return id;
		}

		ICFGSNodeSetBuilder& Builder() override
		{
			return *this;
		}

		cstr Name() const override
		{
			if (name.length() == 0)
			{
				Strings::Format(name, "Fn%llXx%llX", id.id.iValues[0], id.id.iValues[1]);
			}

			return name;
		}

		void SetName(cstr name) override
		{
			this->name = name;
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

		void DeleteCable(int32 cableIndex) override
		{
			cables.Delete(cableIndex);
			ConnectCablesToSockets();
		}

		void DeleteNode(NodeId id) override
		{
			CFGSNode* node = nullptr;
			auto j = mapIdToNode.find(id);
			if (j != mapIdToNode.end())
			{
				node = j->second;
				mapIdToNode.erase(j);
			}

			delete node;
			auto end = std::remove(nodes.begin(), nodes.end(), node);
			nodes.erase(end, nodes.end());

			auto zend = std::remove(zOrderDescending.begin(), zOrderDescending.end(), node);
			zOrderDescending.erase(zend, zOrderDescending.end());

			cables.Delete(id);
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
					std::vector<CFGSNode*> reorderedNodes;
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
			auto* node = new CFGSNode(typeString, topLeft, id);
			nodes.push_back(node);
			zOrderDescending.push_back(node);

			if (!mapIdToNode.insert(std::make_pair(node->Id(), node)).second)
			{
				Throw(0, "A very rare event occured, a 128-bit duplicate hash value was generated!");
			}

			return *node;
		}

		int32 Count() const override
		{
			return (int32)nodes.size();
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

		ICFGSNodeEnumerator& Nodes() override
		{
			return *this;
		}

		ICFGSCableEnumerator& Cables() override
		{
			return cables;
		}

		IPropertyVenue& PropertyVenue() override
		{
			return *this;
		}

		void GetInputArrayId(char inputId[256]) const
		{
			SafeFormat(inputId, 256, "Fn%llx-%llx@inputs", id.id.iValues[0], id.id.iValues[1]);
		}

		void GetOutputArrayId(char inputId[256]) const
		{
			SafeFormat(inputId, 256, "Fn%llx-%llx@outputs", id.id.iValues[0], id.id.iValues[1]);
		}

		bool IsInputArrayId(cstr id) const
		{
			char inputId[256];
			GetInputArrayId(inputId);
			return Strings::Eq(inputId, id);
		}

		bool IsOutputArrayId(cstr id) const
		{
			char inputId[256];
			GetOutputArrayId(inputId);
			return Strings::Eq(inputId, id);
		}

		void VisitVenue(IPropertyVisitor& visitor) override
		{
			ArrayHeaderControl socketHeader;
			socketHeader.isElementDeleteSupported = true;
			socketHeader.isExpandable = true;
			socketHeader.isOrderImmutable = false;
			socketHeader.minElements = 0;
			socketHeader.maxElements = 32'768;

			char inputId[256];
			GetInputArrayId(inputId);
			PropertyMarshallingStub inputStub{ inputId, "Inputs", db.InputArgumentHandler()};
			visitor.BeginArray(inputStub, socketHeader);
		
			int index = 0;
			char argId[256];
			for (auto* s : beginNode.sockets)
			{
				SafeFormat(argId, "Fn%llx %llx, Sck %llx %llx_InBeginIndex", id.id.iValues[0], id.id.iValues[1], s->Id().id.iValues[0], s->Id().id.iValues[1]);

				visitor.BeginIndex(argId, index, true, db.InputArgumentHandler());

				if (inputTypeOptions)
				{
					SafeFormat(argId, "Fn%llx %llx, Sck %llx %llx_Type", id.id.iValues[0], id.id.iValues[1], s->Id().id.iValues[0], s->Id().id.iValues[1]);

					PropertyMarshallingStub optionStub{ argId, "Type", db.InputArgumentHandler() };
					HString socketType = s->Type().Value;
					OptionRef optCurrentType{ socketType };
					visitor.VisitOption(optionStub, REF optCurrentType, 256, *inputTypeOptions);

					auto& S = *static_cast<CFGSSocket*>(s);
					S.SetType(CFGSSocketType{ socketType });
				}

				HString socketName = s->Name();
				SafeFormat(argId, "Fn%llx %llx, Sck %llx %llx_Name", id.id.iValues[0], id.id.iValues[1], s->Id().id.iValues[0], s->Id().id.iValues[1]);
				MARSHAL_STRING(visitor, argId, "Name", db.InputArgumentHandler(), socketName, 256);

				s->SetName(socketName);

				visitor.EndIndex();

				index++;
			}

			visitor.EndArray();

			SafeFormat(argId, "Fn%llx %llx_InputBlackSpace", id.id.iValues[0], id.id.iValues[1]);
			visitor.VisitHeader(argId, "", "");

			char outputId[256];
			GetOutputArrayId(outputId);

			PropertyMarshallingStub outputStub{ outputId, "Outputs", db.OutputArgumentHandler() };
			visitor.BeginArray(outputStub, socketHeader);

			index = 0;
			for (auto* s : returnNode.sockets)
			{
				SafeFormat(argId, "Fn%llx %llx, Sck %llx %llx_OutBeginIndex", id.id.iValues[0], id.id.iValues[1], s->Id().id.iValues[0], s->Id().id.iValues[1]);
				visitor.BeginIndex(argId, index, true, db.OutputArgumentHandler());

				HString socketName = s->Name();

				if (outputTypeOptions)
				{
					SafeFormat(argId, "Fn%llx %llx, Sck %llx %llx_OutType", id.id.iValues[0], id.id.iValues[1], s->Id().id.iValues[0], s->Id().id.iValues[1]);

					PropertyMarshallingStub optionStub{ argId, "Type", db.OutputArgumentHandler() };
					HString socketType = s->Type().Value;
					OptionRef optCurrentType{ socketType };
					visitor.VisitOption(optionStub, REF optCurrentType, 256, *outputTypeOptions);
					auto& S = *static_cast<CFGSSocket*>(s);
					S.SetType(CFGSSocketType{ socketType });
				}

				SafeFormat(argId, "Fn%llx %llx, Sck %llx %llx_OutName", id.id.iValues[0], id.id.iValues[1], s->Id().id.iValues[0], s->Id().id.iValues[1]);
				MARSHAL_STRING(visitor, argId, "Name", db.OutputArgumentHandler(), socketName, 256);
				s->SetName(socketName);
				visitor.EndIndex();

				index++;
			}

			visitor.EndArray();

			SafeFormat(argId, "Fn%llx %llx_OutputBlackSpace", id.id.iValues[0], id.id.iValues[1]);
			visitor.VisitHeader(argId, "", "");
		}
	};

	struct SocketArrayBuilder: IArrayProperty
	{
		FunctionId functionId;
		std::vector<CFGSSocket*>* target;
		CFGSNode* node;
		SocketPlacement placement;
		CFGSSocketType type;
		SocketClass classification;
		Events::IPublisher& publisher;

		SocketArrayBuilder(Events::IPublisher& _publisher): publisher(_publisher)
		{

		}

		void Append() override
		{
			target->push_back(new CFGSSocket(node, placement, type, classification, "undefined-name", SocketId()));
			publisher.PostOneArg(functionId, "FunctionChanged"_event);
		}
	};		

	static auto evPropertyChanged = "PropertyChanged"_event;

	struct ArgumentEventHandler : IPropertyUIEvents
	{
		ICFGSDatabaseSupervisorInternal& db;
		Events::IPublisher& publisher;

		ArgumentEventHandler(ICFGSDatabaseSupervisorInternal& _db, Events::IPublisher& _publisher): db(_db), publisher(_publisher)
		{
		}

		void OnBooleanButtonChanged(IPropertyEditor& property) override
		{
			Events::TEventArgs<IPropertyEditor*> args;
			args.value = &property;
			publisher.Publish(args, evPropertyChanged);
		}

		void OnPropertyEditorLostFocus(Reflection::IPropertyEditor& property) override
		{
			Events::TEventArgs<IPropertyEditor*> args;
			args.value = &property;
			publisher.Publish(args, evPropertyChanged);
		}

		void OnDeleteSection(cstr sectionId) override
		{
			if (!EndsWith(sectionId, "BeginIndex"))
			{
				return;
			}

			Substring s = Substring::ToSubstring(sectionId);

			cstr finalChar = ReverseFind('_', s);
			if (!finalChar)
			{
				return;
			}
			
			bool isInput = strstr(finalChar, "In") != nullptr;

			Substring itemId = { sectionId, finalChar };

			db.ForEachFunction([isInput, &itemId, this](ICFGSFunction& f)
				{
					auto& F = static_cast<CFGSFunction&>(f);					
					auto& node = isInput ? F.BeginNode() : F.ReturnNode();

					SocketId targetId;

					for (int i = 0; i < node.SocketCount(); i++)
					{
						auto& s = node[i];
						
						char argId[256];
						SafeFormat(argId, "Fn%llx %llx, Sck %llx %llx", F.Id().id.iValues[0], F.Id().id.iValues[1], s.Id().id.iValues[0], s.Id().id.iValues[1]);

						if (Eq(itemId, argId))
						{
							targetId = s.Id();
							break;
						}
					}

					if (targetId)
					{
						node.DeleteSocket(targetId);
					}
				}
			);
		}

		void OnDependentVariableChanged(cstr propertyId, IEstateAgent& agent) override
		{
			UNUSED(propertyId);
			UNUSED(agent);
		}

		// Search for the function with an input or output array that is identified with the unique id, and provide an interface to it
		void CallArrayMethod(cstr fullArrayId, Function<void(IArrayProperty&)> callback) override
		{
			auto header = "_header"_fstring;

			if (!EndsWith(fullArrayId, header))
			{
				return;
			}

			char arrayId[256];
			CopyString(arrayId, sizeof arrayId, fullArrayId);

			size_t len = strlen(arrayId);

			arrayId[len - header.length] = 0;

			db.ForEachFunction([arrayId, this, &callback](ICFGSFunction& f)
				{
					auto& F = static_cast<CFGSFunction&>(f);
					if (F.IsInputArrayId(arrayId))
					{
						SocketArrayBuilder builder(publisher);
						builder.functionId = F.Id();
						builder.node = &F.BeginNode();
						builder.classification = SocketClass::InputVar;
						builder.placement = SocketPlacement::Left;
						builder.target = &F.BeginNode().sockets;
						builder.type = CFGSSocketType{ "Int32" };
						callback(builder);
					}
					else if (F.IsOutputArrayId(arrayId))
					{
						SocketArrayBuilder builder(publisher);
						builder.functionId = F.Id();
						builder.node = &F.BeginNode();
						builder.classification = SocketClass::OutputValue;
						builder.placement = SocketPlacement::Right;
						builder.target = &F.ReturnNode().sockets;
						builder.type = CFGSSocketType{ "Int32" };
						callback(builder);
					}
				}
			);
		}
	};

	class CFGSDatabase: public ICFGSDatabaseSupervisorInternal
	{
	private:
		typedef std::unordered_map<FunctionId, CFGSFunction*, FunctionId::Hasher> TFunctions;
		TFunctions mapIdToFunction;
		FunctionId currentFunctionId;

		ArgumentEventHandler inputEventHandler;
		ArgumentEventHandler outputEventHandler;

		Rococo::Events::IPublisher& publisher;
	public:
		CFGSDatabase(Rococo::Events::IPublisher& _publisher): 
			inputEventHandler(*this, _publisher), 
			outputEventHandler(*this, _publisher),
			publisher(_publisher)
		{

		}

		~CFGSDatabase()
		{
			Clear();
		}

		IPropertyUIEvents& InputArgumentHandler() override
		{
			return inputEventHandler;
		}

		IPropertyUIEvents& OutputArgumentHandler() override
		{
			return outputEventHandler;
		}
		
		void BuildFunction(FunctionId id) override
		{
			currentFunctionId = id;
		}

		void Clear() override
		{
			for (auto& i : mapIdToFunction)
			{
				delete i.second;
			}

			mapIdToFunction.clear();
		}

		FunctionId CreateFunction() override
		{
			TFunctions::iterator i;

			for(;;)
			{
				FunctionId id{ Ids::MakeNewUniqueId() };
				auto insertion = mapIdToFunction.insert(std::make_pair(id, (CFGSFunction*) nullptr));
				if (insertion.second)
				{
					// Insertion took place, thus id must be unique to the collection
					i = insertion.first;
					break;
				}
			}

			i->second = new CFGSFunction(*this, i->first);

			return i->first;
		}

		ICFGSFunction* FindFunction(FunctionId id) override
		{
			auto i = mapIdToFunction.find(id);
			return i != mapIdToFunction.end() ? i->second : nullptr;
		}

		void ForEachFunction(Rococo::Function<void(ICFGSFunction& f)> callback)
		{
			for (auto& i : mapIdToFunction)
			{
				callback(*i.second);
			}
		}

		ICFGSFunction* CurrentFunction() override
		{
			auto i = mapIdToFunction.find(currentFunctionId);
			return i != mapIdToFunction.end() ? i->second : nullptr;
		}

		void DeleteFunction(FunctionId id) override
		{
			auto i = mapIdToFunction.find(id);
			if (i != mapIdToFunction.end())
			{
				delete i->second;
				mapIdToFunction.erase(i);
			}
		}

		void Free() override
		{
			delete this;
		}
	};

	ICFGSNode& CFGSSocket::ParentNode() const
	{
		return *parent;
	}
}

namespace Rococo::CFGS
{
	CFGS_MARSHALLER_API ICFGSDatabaseSupervisor* CreateCFGSDatabase(Rococo::Events::IPublisher& publisher)
	{
		return new Internal::CFGSDatabase(publisher);
	}
}