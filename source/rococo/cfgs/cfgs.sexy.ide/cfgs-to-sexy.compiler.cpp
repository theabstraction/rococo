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

struct BuildBundle
{
	struct NodeData
	{
		HString label;
		int index;
	};
	std::unordered_map<ICFGSNode*, NodeData> nodeLabels;

	int nextNodeIndex = 1;

	StringBuilder& sb;
	ISexyDatabase& db;
	ICFGSFunction& graph;
	ICompileExportsSpec& exportSpec;

	BuildBundle(StringBuilder& _sb, ISexyDatabase& _db, ICFGSFunction& _graph, ICompileExportsSpec& _exportSpec):
		sb(_sb), db(_db), graph(_graph), exportSpec(_exportSpec)
	{

	}

	void AppendMethodImplementingFunction();
	void AppendInputDeclarations();
	void AppendOutputDeclarations();
	void CompileFunctionBody();
	void AppendNodeImplementation(cstr fqType, ICFGSNode& calleeNode);
	void AppendBranchOnCondition(ICFGSNode& calleeNode);
	void AppendFunctionCall(cstr fqFunctionName, const ISXYFunction& f, ICFGSNode& calleeNode);
	void AppendDeclareAndAssignArgumentToFunctionCall(cstr argType, cstr argName, cstr fqFunctionName, ICFGSNode& calleeNode);
	void AssignDefaultValueToVariable(cstr argType, cstr argName, cstr fqFunctionName, ICFGSSocket& inputSocket, ICFGSNode& calleeNode);

	int GetNodeIndex(ICFGSNode& node);

	void AppendArgForNode(cstr baseArgName, ICFGSNode& node);
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

void BuildBundle::AssignDefaultValueToVariable(cstr argType, cstr argName, cstr fqFunctionName, ICFGSSocket& inputSocket, ICFGSNode& calleeNode)
{
	HString data;
	HStringPopulator populator(data);
	
	if (!inputSocket.TryGetField(argName, populator))
	{
		return;
	}

	AppendTabs(sb, 1);

	sb << "(";

	AppendArgForNode(argName, calleeNode);

	sb << " = ";

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

void BuildBundle::AppendArgForNode(cstr baseArgName, ICFGSNode& node)
{
	int index = GetNodeIndex(node);
	sb.AppendFormat("%sForNode%d", baseArgName, index);
}

int BuildBundle::GetNodeIndex(ICFGSNode& node)
{
	auto i = nodeLabels.find(&node);
	if (i == nodeLabels.end())
	{
		i = nodeLabels.insert(std::make_pair(&node, NodeData{ "", nextNodeIndex++ })).first;	
	}

	return i->second.index;
}

void BuildBundle::AppendDeclareAndAssignArgumentToFunctionCall(cstr argType, cstr argName, cstr fqFunctionName, ICFGSNode& calleeNode)
{
	AppendTabs(sb, 1);
	sb.AppendFormat("(%s ", argType);
	AppendArgForNode(argName, calleeNode);
	sb << ")\n";

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

	if (!Eq(outputSocket->Type().Value, inputForThisArg->Type().Value))
	{
		Throw(0, "Type mismatch between %s and %s for %s argument (%s %s)", outputSocket->Type().Value, inputForThisArg->Type().Value, fqFunctionName, argType, argName);
	}

	auto* callerNode = graph.Nodes().FindNode(outputConnection.node);

	AppendTabs(sb, 1);
	sb << "(";
	AppendArgForNode(argName, calleeNode);
	sb << " = ";
	AppendArgForNode(outputSocket->Name(), *callerNode);
	sb << ")\n";
}

void BuildBundle::AppendFunctionCall(cstr fqFunctionName, const ISXYFunction& sexyFunction, ICFGSNode& calleeNode)
{
	// First we declare and assign inputs
	for (int i = 0; i < sexyFunction.InputCount(); i++)
	{
		cstr argType = sexyFunction.InputType(i);
		cstr argName = sexyFunction.InputName(i);
		AppendDeclareAndAssignArgumentToFunctionCall(argType, argName, fqFunctionName, calleeNode);
	}

	// Next we declare outputs
	for (int i = 0; i < sexyFunction.OutputCount(); i++)
	{
		cstr argType = sexyFunction.OutputType(i);
		cstr argName = sexyFunction.OutputName(i);

		AppendTabs(sb, 1);
		sb.AppendFormat("(%s ", argType);
		AppendArgForNode(argName, calleeNode);
		sb << ")\n";
	}

	// Finally invoke the function, mapping inputs to ouputs
	AppendTabs(sb, 1);
	sb.AppendFormat("(%s", fqFunctionName);

	for (int i = 0; i < sexyFunction.InputCount(); i++)
	{
		cstr argName = sexyFunction.InputName(i);
		sb << " ";
		AppendArgForNode(argName, calleeNode);
	}

	if (sexyFunction.OutputCount() > 0)
	{
		sb << " -> ";
	}

	for (int i = 0; i < sexyFunction.OutputCount(); i++)
	{
		cstr argName = sexyFunction.OutputName(i);
		sb << " ";
		AppendArgForNode(argName, calleeNode);
	}

	sb << ")\n\n";
}

void BuildBundle::AppendBranchOnCondition(ICFGSNode& branchNode)
{
	AppendDeclareAndAssignArgumentToFunctionCall("Bool", "condition", "if...else", branchNode);
	AppendTabs(sb, 1);
	sb << "(if ";
	AppendArgForNode("condition", branchNode);
	sb << "\n";

	ICFGSCable* truthCable = FindNamedOutgoingCable("True", branchNode, graph);
	if (!truthCable)
	{
		AppendTabs(sb, 1);
		sb << "(return)\n";
		return;
	}

	auto* truthCallee = graph.Nodes().FindNode(truthCable->EntryPoint().node);
	if (!truthCallee)
	{
		AppendTabs(sb, 1);
		sb << "(return)\n";
		return;
	}

	int truthIndex = GetNodeIndex(*truthCallee);

	AppendTabs(sb, 2);
	sb.AppendFormat("(goto node%d)\n", truthIndex);
	
	AppendTabs(sb, 1);
	sb << "else\n";

	ICFGSCable* falseCable = FindNamedOutgoingCable("False", branchNode, graph);
	if (!falseCable)
	{
		AppendTabs(sb, 2);
		sb << "(return)\n";
		AppendTabs(sb, 1);
		sb << ")\n";
		return;
	}

	auto* falseCallee = graph.Nodes().FindNode(falseCable->EntryPoint().node);
	if (!falseCallee)
	{
		AppendTabs(sb, 2);
		sb << "(return)\n";
		AppendTabs(sb, 1);
		sb << ")\n";
		return;
	}

	int falseIndex = GetNodeIndex(*truthCallee);

	AppendTabs(sb, 2);
	sb.AppendFormat("(goto node%d)\n", falseIndex);

	AppendTabs(sb, 1);
	sb << ")\n\n";
}

void BuildBundle::AppendNodeImplementation(cstr fqType, ICFGSNode& calleeNode)
{
	auto* aliasedFunction = db.FindFunction(fqType);
	if (aliasedFunction)
	{
		auto* localFunction = aliasedFunction->LocalFunction();
		if (localFunction)
		{
			AppendFunctionCall(fqType, *localFunction, calleeNode);
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
			AppendBranchOnCondition(calleeNode);
		}
		else
		{
			Throw(0, "Could not find public function aliased as %s", fqType);
		}
	}
}

void BuildBundle::CompileFunctionBody()
{
	nodeLabels.clear();

	ICFGSNode* callerNode = FindBeginNode(graph);

	if (callerNode == nullptr)
	{
		return;
	}

	while (true)
	{
		ICFGSCable* outgoingCable = FindOutgoingCable(*callerNode, graph);
		if (!outgoingCable)
		{
			return;
		}

		auto* calleeNode = graph.Nodes().FindNode(outgoingCable->EntryPoint().node);
		if (!calleeNode)
		{
			return;
		}

		if (!nodeLabels.empty())
		{
			sb << "\n";
		}

		int index = GetNodeIndex(*calleeNode);

		auto i = nodeLabels.find(calleeNode);
		if (i->second.label.length() == 0)
		{
			// Unhandled node, so we add implementation for it
			char label[64];
			SafeFormat(label, "node%d", index);

			if (GetNumberOfCallers(graph, *calleeNode) > 1)
			{
				AppendTabs(sb, 1);
				sb.AppendFormat("(label %s)\n", label);
			}

			AppendNodeImplementation(calleeNode->Type().Value, *calleeNode);

			i->second.label = label;

			callerNode = calleeNode;
		}
		else
		{
			AppendTabs(sb, 1);
			sb.AppendFormat("(goto %s)\n", i->second.label.c_str());
			return;
		}
	}
}

void BuildBundle::AppendInputDeclarations()
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

void BuildBundle::AppendOutputDeclarations()
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

void BuildBundle::AppendMethodImplementingFunction()
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
			BuildBundle bundle(sb, db, f, exportSpec);
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
		Rococo::OS::ShellOpenDocument(ideInstance->GetIDEFrame(), "CFGS SexyStudio IDE. Open file... ", filePath, lineNumber);
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