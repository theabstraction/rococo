#include "cfgs.sexy.ide.h"
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.events.h>
#include <rococo.sexml.h>
#include <rococo.functional.h>
#include <..\sexystudio\sexystudio.api.h>
#include <stdio.h>
#include <rococo.hashtable.h>
#include <sexy.types.h>
#include <rococo.strings.ex.h>
#include <Sexy.S-Parser.h>
#include <unordered_map>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::SexyStudio;
using namespace Rococo::Strings;
using namespace Rococo::Sex::SEXML;

static Rococo::stringmap<int> knownPrimitives;

bool HasNoInputsNorTriggers(ICFGSNode& node)
{
	for (int i = 0; i < node.SocketCount(); i++)
	{
		switch (node[i].SocketClassification())
		{
		case SocketClass::Trigger:
		case SocketClass::InputRef:
		case SocketClass::InputVar:
		case SocketClass::ConstInputRef:
			return false;
		}
	}

	return true;
}

bool IsChildOf(cstr parent, cstr child)
{
	if (!EndsWith(parent, child))
	{
		return false;
	}

	// Ensure that our parent has form <root>.<child>, so that child is not merely a trailing string of subpace or function name such as Quit to RageQuit.
	// We want Game.App.Quit to have a child Quit or child App.Quit, but we do not want Game.App.RageQuit to have a child Quit

	auto parentLen = strlen(parent);
	auto childLen = strlen(child);

	if (parentLen == childLen)
	{
		return true;
	}

	char leadChar = parent[parentLen - childLen - 1];
	return leadChar == '.';
}

bool DoTypesMatch(cstr a, cstr b)
{
	if (Eq(a, b))
	{
		return true;
	}

	return IsChildOf(a, b) || IsChildOf(b, a);
}

static CableConnection FindConnectionToInput(ICFGSFunction& graph, ICFGSSocket& inputSocket)
{
	auto& cables = graph.Cables();

	for (int i = 0; i < cables.Count(); i++)
	{
		auto& cable = cables[i];
		if (cable.EntryPoint().socket == inputSocket.Id())
		{
			return cable.ExitPoint();
		}
	}

	return CableConnection{};
}

static ICFGSSocket* FindConnectionToOutput(ICFGSFunction& graph, CableConnection outputConnection)
{
	auto* node = graph.Nodes().FindNode(outputConnection.node);
	if (!node)
	{
		return nullptr;
	}

	return node->FindSocket(outputConnection.socket);
}

// A bundle of interfaces and containers for compiling a graph into Sexy. Every graph has its own builder.
class GraphBuilder
{
private:
	struct NodeData
	{
		HString label;
		int index;
	};
	std::unordered_map<ICFGSNode*, NodeData> nodeLabels;

	stringmap<int> argNames;

	int nextNodeIndex = 1;

	StringBuilder& sb;
	ISexyDatabase& db;
	ICFGSFunction& graph;
	ICompileExportsSpec& exportSpec;

	std::vector<ICFGSNode*> callstack;

	void AppendInvokeFunction(cstr fqFunctionName, const ISXYFunction& sexyFunction, ICFGSNode& calleeNode);
	void AppendInputDeclarations();
	void AppendOutputDeclarations();
	void CompileFunctionBody();
	void CompileNodeInsideFunctionBodyRecursive(ICFGSNode& callerNode, int branchDepth, int chainLinkCount);
	void CompileCalleeNodeInsideFunctionBodyRecursive(ICFGSNode& calleeNode, int branchDepth, int chainLinkCount);
	void AppendNodeImplementation(cstr fqType, ICFGSNode& calleeNode, int branchDepth, int chainLinkCount);
	void AppendBranchOnCondition(ICFGSNode& calleeNode, int branchDepth, int chainLinkCount);
	void AppendGetVariable(ICFGSNode& calleeNode, int branchDepth, int chainLinkCount);
	void AppendSetVariable(ICFGSNode& calleeNode, int branchDepth, int chainLinkCount);
	void AppendFunctionCall(cstr fqFunctionName, const ISXYFunction& f, ICFGSNode& calleeNode, int branchDepth, int chainLinkCount);
	void AppendDeclareAndAssignArgumentToFunctionCall(cstr argType, cstr argName, cstr fqFunctionName, ICFGSNode& calleeNode, int branchDepth, int chainLinkCount);
	void AssignDefaultValueToVariable(cstr argType, cstr argName, cstr fqFunctionName, ICFGSSocket& inputSocket, ICFGSNode& calleeNode);

	int GetNodeIndex(ICFGSNode& node);

	cstr GetArgForNode(cstr baseArgName, ICFGSNode& node);

public:
	GraphBuilder(StringBuilder& _sb, ISexyDatabase& _db, ICFGSFunction& _graph, ICompileExportsSpec& _exportSpec) :
		sb(_sb), db(_db), graph(_graph), exportSpec(_exportSpec)
	{

	}
	void AppendMethodImplementingFunction();
};

cstr GetCorrectedDefaultValue(const ISXYType* type, cstr defaultValue)
{
	if (!type)
	{
		return nullptr;
	}

	if (knownPrimitives.empty())
	{
		knownPrimitives["Float32"] = 0;
		knownPrimitives["Float64"] = 0;
		knownPrimitives["Int32"] = 0;
		knownPrimitives["Int64"] = 0;
	}

	auto* localType = type->LocalType();

	if (localType && localType->FieldCount() == 1)
	{
		if (knownPrimitives.find(localType->GetField(0).type) != knownPrimitives.end())
		{
			return defaultValue;
		}
		else if (Eq(localType->GetField(0).type, "Bool"))
		{
			if (Eq(defaultValue, "0"))
			{
				return "false";
			}

			if (Eq(defaultValue, "1"))
			{
				return "true";
			}

			if (Eq(defaultValue, "true") || Eq(defaultValue, "false"))
			{
				return defaultValue;
			}
		}
	}

	return nullptr;
}

static ICFGSSocket* FindCallee(ICFGSNode& caller)
{
	for (int i = 0; i < caller.SocketCount(); i++)
	{
		auto& socket = caller[i];
		if (socket.SocketClassification() == SocketClass::Exit)
		{
			return &socket;
		}
	}

	return nullptr;
}

static ICFGSSocket* FindNamedCallee(cstr name, ICFGSNode& caller)
{
	for (int i = 0; i < caller.SocketCount(); i++)
	{
		auto& socket = caller[i];
		if (socket.SocketClassification() == SocketClass::Exit && Eq(socket.Name(), name))
		{
			return &socket;
		}
	}

	return nullptr;
}

static ICFGSCable* FindOutgoingCable(ICFGSNode& node, ICFGSFunction& f)
{
	ICFGSSocket* s = FindCallee(node);
	if (!s)
	{
		return nullptr;
	}

	auto& cables = f.Cables();

	for (int i = 0; i < cables.Count(); i++)
	{
		auto& cable = cables[i];
		if (cable.ExitPoint().socket == s->Id())
		{
			return &cable;
		}
	}

	return nullptr;
}

static ICFGSCable* FindNamedOutgoingCable(cstr name, ICFGSNode& node, ICFGSFunction& f)
{
	ICFGSSocket* s = FindNamedCallee(name, node);
	if (!s)
	{
		return nullptr;
	}

	auto& cables = f.Cables();

	for (int i = 0; i < cables.Count(); i++)
	{
		auto& cable = cables[i];
		if (cable.ExitPoint().socket == s->Id())
		{
			return &cable;
		}
	}

	return nullptr;
}

static void AppendTabs(StringBuilder& sb, int count)
{
	int spacesPerTab = 4;
	for (int i = 0; i < count * spacesPerTab; i++)
	{
		sb << " ";
	}
}

static ICFGSNode* FindBeginNode(ICFGSFunction& f)
{
	auto& nodes = f.Nodes();
	for (int i = 0; i < nodes.Count(); i++)
	{
		auto& node = nodes[i];
		if (Eq(node.Type().Value, "<Begin>"))
		{
			return &node;
		}
	}

	return nullptr;
}

static int GetNumberOfCallers(ICFGSFunction& f, ICFGSNode& node)
{
	int count = 0;

	auto& cables = f.Cables();
	for (int i = 0; i < cables.Count(); i++)
	{
		auto& cable = cables[i];
		if (cable.EntryPoint().node == node.Id())
		{
			count++;
		}
	}

	return count;
}

static ICFGSSocket* FindSocketInput(ICFGSNode& node, cstr argType, cstr argName)
{
	int matchingTypeCount = 0;
	int firstMatchingTypeIndex = -1;

	for (int i = 0; i < node.SocketCount(); i++)
	{
		auto& s = node[i];
		if (Eq(s.Type().Value, argType))
		{
			matchingTypeCount++;
			if (firstMatchingTypeIndex == -1)
			{
				firstMatchingTypeIndex = i;
			}

			if (Eq(s.Name(), argName))
			{
				return &s;
			}
		}
	}
	
	if (matchingTypeCount == 1)
	{
		// We didn't match the arg name, but the type was unique, so assume the argument has undergone a cosmetic name change
		return &node[firstMatchingTypeIndex];
	}

	return nullptr;
}

static void ValidateDefaultCharacterAsNonString(cstr text, cstr argName, cstr fqFunctionName, cstr graphName)
{
	for (cstr p = text; *p != 0; p++)
	{
		if (*p <= 32)
		{
			Throw(0, "Bad blank space %d in default value string for %s of %s for graph %s", (int)*p, argName, fqFunctionName, graphName);
		}

		switch (*p)
		{
		case '&':
		case '(':
		case ')':
		case '\'':
		case '"':
		case '/':
		case '\\':
			Throw(0, "Bad character %d in default value string for %s of %s for graph %s", (int)*p, argName, fqFunctionName, graphName);
		}
	}
}

void GraphBuilder::AssignDefaultValueToVariable(cstr argType, cstr argName, cstr fqFunctionName, ICFGSSocket& inputSocket, ICFGSNode& calleeNode)
{
	HString data;
	HStringPopulator populator(data);
	
	if (!inputSocket.TryGetField(argName, populator))
	{
		return;
	}

	AppendTabs(sb, 1);

	sb.AppendFormat("(%s = ", GetArgForNode(argName, calleeNode));

	if (Eq(argType, "Sys.Type.IString") || (Eq(argType, "IString")))
	{
		sb.AppendChar('"');
		Rococo::Strings::AppendEscapedSexyString(sb, data);
		sb.AppendChar('"');
	}
	else
	{
		ValidateDefaultCharacterAsNonString(data, argName, fqFunctionName, graph.Name());
		sb << data.c_str();
	}

	sb << ")\n";
}

static int CountNumberOfArgsWithNameInGraph(ICFGSFunction& graph, cstr argName)
{
	int count = 0;

	for (int i = 0; i < graph.Nodes().Count(); i++)
	{
		auto& node = graph.Nodes()[i];

		for (int j = 0; j < node.SocketCount(); j++)
		{
			auto& socket = node[j];
			if (Eq(socket.Name(), argName))
			{
				count++;
			}
		}
	}

	return count;
}

cstr GraphBuilder::GetArgForNode(cstr baseArgName, ICFGSNode& node)
{
	int index = GetNodeIndex(node);

	char fullArg[512];
	SafeFormat(fullArg, "%sForNode%d", baseArgName, index);

	auto i = argNames.find(fullArg);
	if (i == argNames.end())
	{
		int nConflicts = CountNumberOfArgsWithNameInGraph(graph, baseArgName) - 1;
		if (nConflicts == 0)
		{
			return baseArgName;
		}
		else
		{
			i = argNames.insert(fullArg, 0).first;
		}
	}

	return i->first;
}

int GraphBuilder::GetNodeIndex(ICFGSNode& node)
{
	auto i = nodeLabels.find(&node);
	if (i == nodeLabels.end())
	{
		i = nodeLabels.insert(std::make_pair(&node, NodeData{ "", nextNodeIndex++ })).first;	
	}

	return i->second.index;
}

void GraphBuilder::AppendDeclareAndAssignArgumentToFunctionCall(cstr argType, cstr argName, cstr fqFunctionName, ICFGSNode& calleeNode, int branchDepth, int chainLinkCount)
{
	AppendTabs(sb, 1);
	sb.AppendFormat("(%s %s)\n", argType, GetArgForNode(argName, calleeNode));

	auto* inputForThisArg = FindSocketInput(calleeNode, argType, argName);
	if (!inputForThisArg)
	{
		return;
	}

	CableConnection outputConnection = FindConnectionToInput(graph, *inputForThisArg);
	ICFGSSocket* outputSocket = FindConnectionToOutput(graph, outputConnection);
	if (!outputSocket)
	{
		AssignDefaultValueToVariable(argType, argName, fqFunctionName, *inputForThisArg, calleeNode);
		return;
	}

	if (!DoTypesMatch(outputSocket->Type().Value, inputForThisArg->Type().Value))
	{
		Throw(0, "Type mismatch between %s and %s for %s argument (%s %s)", outputSocket->Type().Value, inputForThisArg->Type().Value, fqFunctionName, argType, argName);
	}

	auto* outputNode = graph.Nodes().FindNode(outputConnection.node);

	if (HasNoInputsNorTriggers(*outputNode))
	{
		// If a function has no inputs nor triggers we can evaluate it Just-in-Time before the point of application
		AppendNodeImplementation(outputNode->Type().Value, *outputNode, branchDepth, chainLinkCount + 1);
	}
	else
	{
		if (std::find(callstack.begin(), callstack.end(), outputNode) == callstack.end())
		{
			cstr requiredArg = GetArgForNode(outputSocket->Name(), *outputNode);
			Throw(0, "Cannot find the supplier of the argument %s in the callstack, which means the output value of %s was never computed ahead of its required access", requiredArg, outputNode->Type().Value);
		}
	}

	AppendTabs(sb, 1);
	sb.AppendFormat("(%s = %s)\n", GetArgForNode(argName, calleeNode), GetArgForNode(outputSocket->Name(), *outputNode));
}

// Appends the expression prefix (<fqFunctionName <arg1> ... <argN> to the builder
void GraphBuilder::AppendInvokeFunction(cstr fqFunctionName, const ISXYFunction& sexyFunction, ICFGSNode& calleeNode)
{
	sb.AppendFormat("(%s", fqFunctionName);

	for (int i = 0; i < sexyFunction.InputCount(); i++)
	{
		cstr argName = sexyFunction.InputName(i);
		sb << " " << GetArgForNode(argName, calleeNode);
	}
}

void GraphBuilder::AppendFunctionCall(cstr fqFunctionName, const ISXYFunction& sexyFunction, ICFGSNode& calleeNode, int branchDepth, int chainLinkCount)
{
	// TODO - optimize this so that for the case of functions of 1 output we put the function call inline with the output declaration, e.g (Float32 x = (Sys.Maths.SinDegrees 45))
	// First we declare and assign inputs
	for (int i = 0; i < sexyFunction.InputCount(); i++)
	{
		cstr argType = sexyFunction.InputType(i);
		cstr argName = sexyFunction.InputName(i);
		AppendDeclareAndAssignArgumentToFunctionCall(argType, argName, fqFunctionName, calleeNode, branchDepth, chainLinkCount);
	}

	// Next we declare outputs
	for (int i = 0; i < sexyFunction.OutputCount(); i++)
	{
		cstr argType = sexyFunction.OutputType(i);
		cstr argName = sexyFunction.OutputName(i);

		AppendTabs(sb, 1);
		sb.AppendFormat("(%s %s)\n", argType, GetArgForNode(argName, calleeNode));
	}

	AppendTabs(sb, 1);

	switch (sexyFunction.OutputCount())
	{
		case 0:
		{
		// Function has no output so we invoke it like this (<fn-name>)
			AppendInvokeFunction(fqFunctionName, sexyFunction, calleeNode);
			break;
		}
		case 1:
		{
		// Function has one output so invoke it inline like this: (variableName = (<fn-name> <inputs...>))
			cstr outputArgName = sexyFunction.OutputName(0);
			sb.AppendFormat("(%s = ", GetArgForNode(outputArgName, calleeNode));
			AppendInvokeFunction(fqFunctionName, sexyFunction, calleeNode);
			sb << ")";
			break;
		}
		// Function has multiple outputs, so invoke it like this: (<fn-name <inputs ...> -> <outputs ...>)
		default:
		{
			// Finally invoke the function, mapping inputs to ouputs
			AppendInvokeFunction(fqFunctionName, sexyFunction, calleeNode);

			if (sexyFunction.OutputCount() > 0)
			{
				sb << " -> ";
			}

			for (int i = 0; i < sexyFunction.OutputCount(); i++)
			{
				cstr argName = sexyFunction.OutputName(i);
				sb << " " << GetArgForNode(argName, calleeNode);
			}

			break;
		}
	}

	sb << ")\n\n";
}

void GraphBuilder::AppendBranchOnCondition(ICFGSNode& branchNode, int branchDepth, int chainLinkCount)
{
	UNUSED(chainLinkCount);

	// TODO - optimize this to elimanate the condition variable and place the boolean value directly in the (if <condition > ... ) statement

	AppendDeclareAndAssignArgumentToFunctionCall("Bool", "condition", "if...else", branchNode, branchDepth, chainLinkCount);

	sb << "\n";

	AppendTabs(sb, 1);
	sb << "(if " << GetArgForNode("condition", branchNode) << "\n";

	ICFGSNode* truthCallee = nullptr;
	ICFGSNode* falseCallee = nullptr;

	ICFGSCable* truthCable = FindNamedOutgoingCable("True", branchNode, graph);
	if (!truthCable)
	{
		AppendTabs(sb, 1);
		sb << "(return)\n";
	}

	truthCallee = truthCable ? graph.Nodes().FindNode(truthCable->EntryPoint().node) : nullptr;
	if (!truthCallee)
	{
		AppendTabs(sb, 1);
		sb << "(return)\n";
	}
	else
	{
		int truthIndex = GetNodeIndex(*truthCallee);

		AppendTabs(sb, 2);
		sb.AppendFormat("(goto node%d)\n", truthIndex);

		AppendTabs(sb, 1);
	}

	sb << "else\n";

	ICFGSCable* falseCable = FindNamedOutgoingCable("False", branchNode, graph);
	if (!falseCable)
	{
		AppendTabs(sb, 2);
		sb << "(return)\n";
		AppendTabs(sb, 1);
		sb << ")\n";
	}

	falseCallee = falseCable ? graph.Nodes().FindNode(falseCable->EntryPoint().node) : nullptr;
	if (!falseCallee)
	{
		AppendTabs(sb, 2);
		sb << "(return)\n";
		AppendTabs(sb, 1);
		sb << ")\n";
	}
	else
	{
		int falseIndex = GetNodeIndex(*falseCallee);

		AppendTabs(sb, 2);
		sb.AppendFormat("(goto node%d)\n", falseIndex);

		AppendTabs(sb, 1);
		sb << ")\n\n";
	}

	if (truthCallee) CompileCalleeNodeInsideFunctionBodyRecursive(*truthCallee, branchDepth + 1, 0);
	if (falseCallee) CompileCalleeNodeInsideFunctionBodyRecursive(*falseCallee, branchDepth + 1, 0);
}

ICFGSSocket* GetAssignValueSocket(ICFGSNode& setNode)
{
	for (int i = 0; i < setNode.SocketCount(); i++)
	{
		auto& s = setNode[i];
		switch (s.SocketClassification())
		{
		case SocketClass::InputVar:
		case SocketClass::InputRef:
		case SocketClass::ConstInputRef:
			return &s;
		}
	}

	return nullptr;
}

ICFGSSocket* GetRetrieveValueSocket(ICFGSNode& setNode)
{
	for (int i = 0; i < setNode.SocketCount(); i++)
	{
		auto& s = setNode[i];
		switch (s.SocketClassification())
		{
		case SocketClass::OutputRef:
		case SocketClass::OutputValue:
		case SocketClass::ConstOutputRef:
			return &s;
		}
	}

	return nullptr;
}

void GraphBuilder::AppendGetVariable(ICFGSNode& getNode, int branchDepth, int chainLinkCount)
{
	UNUSED(chainLinkCount);
	UNUSED(branchDepth);

	cstr getString = getNode.Type().Value;

	char bracketSet[8];
	char variableName[256];
	char dash[2];
	char typeName[256];

	uint32 sizeofBracketSet = sizeof bracketSet;
	uint32 sizeofVariableName = sizeof variableName;
	uint32 sizeofTypeName = sizeof typeName;

	// [<Get>] <variable> - <type>
	if (4 != sscanf_s(getString, "%s %s %s %s", bracketSet, sizeofBracketSet, variableName, sizeofVariableName, dash, 2, typeName, sizeofTypeName))
	{
		Throw(0, "Expecting 4 arguments in setNode: %s", getString);
	}

	if (!Eq(bracketSet, "<Get>"))
	{
		Throw(0, "Expectign <Get> as first argument in GetNode: %s", getString);
	}

	ICFGSSocket* s = GetRetrieveValueSocket(getNode);
	if (!s)
	{
		Throw(0, "No retrieval socket for node: %s", getString);
	}

	if (!DoTypesMatch(typeName, s->Type().Value))
	{
		Throw(0, "Type mismatch for %s vs %s for %s", typeName, s->Type().Value, getString);
	}

	AppendTabs(sb, 1);

	sb.AppendFormat("(%s = this.%s)\n", GetArgForNode(s->Name(), getNode),  variableName);
}

// TODO - optimize this to set variables inline if the assignation comes from a <GET> node
void GraphBuilder::AppendSetVariable(ICFGSNode& setNode, int branchDepth, int chainLinkCount)
{
	UNUSED(chainLinkCount);
	UNUSED(branchDepth);

	cstr setString = setNode.Type().Value;

	char bracketSet[8];
	char variableName[256];
	char dash[2];
	char typeName[256];

	uint32 sizeofBracketSet = sizeof bracketSet;
	uint32 sizeofVariableName = sizeof variableName;
	uint32 sizeofTypeName = sizeof typeName;

	// [<Set>] <variable> - <type>
	if (4 != sscanf_s(setString, "%s %s %s %s", bracketSet, sizeofBracketSet, variableName, sizeofVariableName, dash, 2, typeName, sizeofTypeName))
	{
		Throw(0, "Expecting 4 arguments in setNode: %s", setString);
	}

	if (!Eq(bracketSet, "<Set>"))
	{
		Throw(0, "Expectign <Set> as first argument in SetNode: %s", setString);
	}

	ICFGSSocket* s = GetAssignValueSocket(setNode);
	if (!s)
	{
		Throw(0, "No assignment socket for node: %s", setString);
	}

	if (!DoTypesMatch(typeName, s->Type().Value))
	{
		Throw(0, "Type mismatch for %s vs %s for %s", typeName, s->Type().Value, setString);
	}

	CableConnection outputConnection = FindConnectionToInput(graph, *s);	
	auto* outputNode = graph.Nodes().FindNode(outputConnection.node);
	if (outputNode)
	{
		auto* outputSocket = outputNode->FindSocket(outputConnection.socket);
		if (!outputSocket)
		{
			Throw(0, "Expecting output connection socket, but none found for %s", outputNode->Type());
		}

		if (!DoTypesMatch(typeName, outputSocket->Type().Value))
		{
			Throw(0, "Type mismatch for [%s of %s] with [%s of %s]", typeName, setString, outputSocket->Type().Value, outputNode->Type().Value);
		}

		if (HasNoInputsNorTriggers(*outputNode))
		{
			AppendTabs(sb, 1);
			sb.AppendFormat("(%s %s)\n", outputSocket->Type().Value, outputSocket->Name());
			// If a function has no inputs nor triggers we can evaluate it Just-in-Time before the point of application
			AppendNodeImplementation(outputNode->Type().Value, *outputNode, branchDepth, chainLinkCount + 1);
		}

		AppendTabs(sb, 1);
		sb.AppendFormat("(this.%s = %s)\n", variableName, GetArgForNode(outputSocket->Name(), *outputNode));
	}
	else
	{
		HString fieldValue;
		HStringPopulator fieldValuePopulator(fieldValue);
		if (!s->TryGetField(s->Name(), fieldValuePopulator))
		{
			Throw(0, "No attribute %s for (socket %s %s) of %s", s->Name(), s->Type().Value, s->Name(), setNode.Type().Value);
		}

		if (DoTypesMatch(typeName, "Sys.Type.String"))
		{
			AppendTabs(sb, 1);
			sb.AppendFormat("(this.%s = \"", variableName);
			AppendEscapedSexyString(sb, fieldValue);
			sb << "\")\n";
		}
		else
		{
			AppendTabs(sb, 1);
			sb.AppendFormat("(this.%s = %s)\n", variableName, fieldValue.c_str());
		}
	}
}

void GraphBuilder::AppendNodeImplementation(cstr fqType, ICFGSNode& calleeNode, int branchDepth, int chainLinkCount)
{
	auto* aliasedFunction = db.FindFunction(fqType);
	if (aliasedFunction)
	{
		auto* localFunction = aliasedFunction->LocalFunction();
		if (localFunction)
		{
			AppendFunctionCall(fqType, *localFunction, calleeNode, branchDepth, chainLinkCount);
			CompileNodeInsideFunctionBodyRecursive(calleeNode, branchDepth, chainLinkCount + 1);
		}
		else
		{
			Throw(0, "Could not find local function aliased to %s", fqType);
		}
	}
	else
	{
		if (Eq(fqType, "<IfElse>"))
		{
			AppendBranchOnCondition(calleeNode, branchDepth, chainLinkCount + 1);
		}
		else if (StartsWith(fqType, "<Set>"))
		{
			AppendSetVariable(calleeNode, branchDepth, chainLinkCount + 1);
			CompileNodeInsideFunctionBodyRecursive(calleeNode, branchDepth, chainLinkCount + 2);
		}
		else if (StartsWith(fqType, "<Get>"))
		{
			AppendGetVariable(calleeNode, branchDepth, chainLinkCount + 1);
		}
		else
		{
			Throw(0, "%s: not implemented", fqType);
		}
	}
}

void GraphBuilder::CompileCalleeNodeInsideFunctionBodyRecursive(ICFGSNode& calleeNode, int branchDepth, int chainLinkCount)
{
	if (!nodeLabels.empty())
	{
		sb << "\n";
	}

	int index = GetNodeIndex(calleeNode);

	auto i = nodeLabels.find(&calleeNode);
	if (i->second.label.length() == 0)
	{
		// Unhandled node, so we add implementation for it
		char label[64];
		SafeFormat(label, "node%d", index);

		if (index > 1 && GetNumberOfCallers(graph, calleeNode) > 1 || branchDepth > 0 && chainLinkCount == 0)
		{
			AppendTabs(sb, 1);
			sb.AppendFormat("(label %s)\n", label);
		}

		i->second.label = label;

		AppendNodeImplementation(calleeNode.Type().Value, calleeNode, branchDepth, chainLinkCount);
	}
	else
	{
		AppendTabs(sb, 1);
		sb.AppendFormat("(goto %s)\n", i->second.label.c_str());
		return;
	}
}

void GraphBuilder::CompileNodeInsideFunctionBodyRecursive(ICFGSNode& callerNode, int branchDepth, int chainLinkCount)
{
	ICFGSCable* outgoingCable = FindOutgoingCable(callerNode, graph);
	if (!outgoingCable)
	{
		AppendTabs(sb, 1);
		sb << "(return)\n";
		return;
	}

	auto* calleeNode = graph.Nodes().FindNode(outgoingCable->EntryPoint().node);
	if (!calleeNode)
	{
		return;
	}

	callstack.push_back(calleeNode);

	CompileCalleeNodeInsideFunctionBodyRecursive(*calleeNode, branchDepth, chainLinkCount);

	callstack.pop_back();
}

void GraphBuilder::CompileFunctionBody()
{
	ICFGSNode* callerNode = FindBeginNode(graph);

	if (callerNode == nullptr)
	{
		return;
	}

	callstack.push_back(callerNode);

	CompileNodeInsideFunctionBodyRecursive(*callerNode, 0, 0);

	if (callstack.size() != 1)
	{
		Throw(0, "Incorrect callstack length: %llu", callstack.size());
	}

	callstack.pop_back();
}

void GraphBuilder::AppendInputDeclarations()
{
	auto& beginNode = graph.BeginNode();
	for (int i = 0; i < beginNode.SocketCount(); i++)
	{
		auto& input = beginNode[i];
		switch (input.SocketClassification())
		{
		case SocketClass::ConstInputRef:
		case SocketClass::InputRef:
		case SocketClass::InputVar:
			sb.AppendFormat(" (%s %s)", input.Type(), input.Name());
		}
	}
}

void GraphBuilder::AppendOutputDeclarations()
{
	auto& returnNode = graph.ReturnNode();
	for (int i = 0; i < returnNode.SocketCount(); i++)
	{
		auto& output = returnNode[i];
		switch (output.SocketClassification())
		{
		case SocketClass::ConstOutputRef:
		case SocketClass::OutputRef:
		case SocketClass::OutputValue:
			sb.AppendFormat("(%s %s) ", output.Type(), output.Name());
		}
	}
}

void GraphBuilder::AppendMethodImplementingFunction()
{
	// Methods that begin with underscore are deemed local methods private to an implementation and not exposed by an interface.
	// Since the Sexy language does not permit underscores in any local identifier, we skip leading underscores
	cstr name = graph.Name();
	if (name[0] == '_') name++;

	/* Methods look like this
	 
	(method Dog.Bark (Decibals dB)(Hz hz)(Vec3 location) -> (Bool barkSucceeded): 
		(this.system.emit dB hz location "Bark.mp3" -> barkSucceeded)
	)
	 
	*/

	sb.AppendFormat("\n(method %s.%s", exportSpec.ClassName(), name);

	AppendInputDeclarations();

	sb << " -> ";

	AppendOutputDeclarations();

	sb.AppendFormat(":\n");

	CompileFunctionBody();

	sb.AppendFormat("\n)\n");
}

static void CompileToStringProtected(StringBuilder& sb, ISexyDatabase& db, ICFGSDatabase& cfgs, ICompileExportsSpec& exportSpec, ICFGSVariableEnumerator& variables)
{
	// Build the interface definition
	sb.AppendFormat("(interface %s\n", exportSpec.InterfaceName());
	cfgs.ForEachFunction(
		[&sb](ICFGSFunction& f)
		{
			// Methods that begin with underscore are deemed local methods private to an implementation and not exposed by an interface.
			cstr name = f.Name();
			if (name[0] != '_')
			{
				// Public method
				AppendTabs(sb, 1);
				sb.AppendFormat("(%s -> )\n", name);
			}
		}
	);
	sb << ")\n\n";

	// Build the class definition that specifies the implementation interface and member variables
	sb.AppendFormat("(class %s (implements %s)\n", exportSpec.ClassName(), exportSpec.InterfaceName());
	variables.ForEachVariable(
		[&sb, &db](cstr name, cstr type, cstr defaultValue)
		{
			UNUSED(defaultValue);
			const ISXYType* sxyType = db.FindPrimitiveOrFQType(type);
			UNUSED(sxyType);
			AppendTabs(sb, 1);
			sb.AppendFormat("(%s %s)\n", type, name);
		}
	);
	sb << ")\n";

	// Turn the graphs (functions) into methods for our new class.
	cfgs.ForEachFunction(
		[&sb, &db, &exportSpec](ICFGSFunction& f)
		{ 
			GraphBuilder bundle(sb, db, f, exportSpec);
			bundle.AppendMethodImplementingFunction();
		}
	);
}

static bool CompileToString(StringBuilder& sb, ISexyDatabase& db, ICFGSDatabase& cfgs, ICompileExportsSpec& exportSpec, ICFGSVariableEnumerator& variables) noexcept
{
	try
	{
		CompileToStringProtected(sb, db, cfgs, exportSpec, variables);
		return true;
	}
	catch (IException& ex)
	{
		char buffer[2048];
		OS::BuildExceptionString(buffer, sizeof buffer, ex, true);
		sb << "Exception thrown compiling CFGS graphs:" << buffer;
	}
	catch (...)
	{
		sb << "Unspecified exception thrown compiling CFGS graphs:";
	}

	return false;
}

namespace Rococo::CFGS
{
	void Compile(ISexyDatabase& db, ICFGSDatabase& cfgs, Strings::IStringPopulator& populator, ICompileExportsSpec& exportSpec, ICFGSVariableEnumerator& variables)
	{
		AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(32768);
		auto& sb = dsb->Builder();

		char timestamp[128];
		Rococo::Time::FormatTime(Rococo::Time::UTCTime(), timestamp, sizeof timestamp);

		sb << "// Code generated by cfgs.sexy.cli at " << timestamp << " UTC\n";
		sb << "// CFGS-Origin: " << cfgs.Origin() << "\n\n";

		bool success = CompileToString(sb, db, cfgs, exportSpec, variables);

		populator.Populate(*sb);

		if (!success)
		{
			Throw(0, "Error compiling cfgs into sexy. Check file for more information");
		}
	}
}

#include <rococo.os.win32.h>
#include <rococo.sexystudio.api.h>

struct SexyIDEWindow : ISexyStudioEventHandler
{
	AutoFree<Rococo::SexyStudio::ISexyStudioInstance1> ideInstance;

	SexyIDEWindow()
	{

	}

	void Create(ISexyStudioFactory1& factory)
	{
		ideInstance = factory.CreateSexyIDE(Windows::NoParent(), *this);
	}

	bool TryOpenEditor(cstr filePath, int lineNumber) override
	{
		Rococo::OS::ShellOpenDocument(ideInstance->Gui().GetIDEFrame(), "CFGS SexyStudio IDE. Open file... ", filePath, lineNumber);
		return true;
	}

	EIDECloseResponse OnIDEClose(IWindow&) override
	{
		return EIDECloseResponse::Shutdown;
	}
};

namespace Rococo::CFGS
{
	ROCOCO_API_IMPORT void LoadDatabase(ICFGSDatabase& db, cstr filename, ICFGSLoader& loader);
}

struct CLI_Compiler : ICFGSSexyCLI, ICFGSLoader, ICFGSVariableEnumerator
{
	AutoFree<Rococo::Events::IPublisherSupervisor> publisher;
	AutoFree<ICFGSDatabaseSupervisor> cfgs;
	AutoFree<ISexyStudioFactory1> ssFactory;
	SexyIDEWindow ideWindow;

	CLI_Compiler()
	{
		publisher = Events::CreatePublisher();

		HMODULE hSexyStudio = LoadLibraryA("sexystudio.dll");
		if (!hSexyStudio)
		{
			Throw(GetLastError(), "%s: failed to load sexystudio.dll", __FUNCTION__);
		}

		auto CreateSexyStudioFactory = (FN_CreateSexyStudioFactory)GetProcAddress(hSexyStudio, "CreateSexyStudioFactory");
		if (!CreateSexyStudioFactory)
		{
			Throw(GetLastError(), "%s: failed to find proc CreateSexyStudioFactory in sexystudio.dll", __FUNCTION__);
		}

		cstr interfaceURL = "Rococo.SexyStudio.ISexyStudioFactory1";

		int nErr = CreateSexyStudioFactory((void**)&ssFactory, interfaceURL);
		if FAILED(nErr)
		{
			Throw(nErr, "CreateSexyStudioFactory with URL %s failed", interfaceURL);
		}

		ideWindow.Create(*ssFactory);

		cfgs = CreateCFGSDatabase(*publisher);
	}

	ISexyDatabase& DB()
	{
		return ideWindow.ideInstance->GetDatabase();
	}

	void ForEachVariable(Rococo::Function<void(Rococo::cstr name, Rococo::cstr type, Rococo::cstr defaultValue)> callback)
	{
		for (auto& v : variables)
		{
			callback.Invoke(v.first, v.second.type, v.second.defaultValue);
		}
	}

	void Compile(cstr targetFile) override
	{
		if (!targetFile)
		{
			Throw(0, "No target file. Specify -cfgs:<target_file> on command line");
		}

		struct : Strings::IStringPopulator
		{
			void Populate(cstr text) override
			{
				puts(text);
			}
		} onCompile;

		struct spec : ICompileExportsSpec
		{
			cstr InterfaceName() const override
			{
				return "Sys.CFGS.ITest";
			}

			cstr FactoryName() const override
			{
				return "Sys.CFGS.NewTest";
			}

			cstr ClassName() const override
			{
				return "Test";
			}
		} exportSpec;

		try
		{
			LoadDatabase(*cfgs, targetFile, *this);
			cfgs->SetOrigin(targetFile);
			Rococo::CFGS::Compile(DB(), *cfgs, onCompile, exportSpec, *this);
		}
		catch (ParseException& ex)
		{
			Throw(ex.ErrorCode(), "%s: parse error at line %d pos %d.\n\t%s", targetFile, ex.Start().y, ex.Start().x, ex.Message());
		}
	}

	void Free() override
	{
		delete this;
	}

	struct VariableDef
	{
		HString type;
		HString defaultValue;
	};

	stringmap<VariableDef> variables;

	void Loader_OnLoadNavigation(const ISEXMLDirective& navDirective) override
	{
		size_t startIndex = 0;
		const auto* varDirective = navDirective.FindFirstChild(REF startIndex, "Variables");
		if (varDirective)
		{
			auto& variableList = varDirective->Children();

			for (size_t i = 0; i < variableList.NumberOfDirectives(); i++)
			{
				auto& variable = variableList[i];

				cstr name = SEXML::AsAtomic(variable["Name"]).c_str();
				cstr type = SEXML::AsAtomic(variable["Type"]).c_str();
				cstr defaultValue = SEXML::AsAtomic(variable["Default"]).c_str();

				variables.insert(name, VariableDef{  type, defaultValue });
			}
		}
	}
};

extern "C" __declspec(dllexport) ICFGSSexyCLI* Create_CFGS_Win32_CLI()
{
	return new CLI_Compiler();
}