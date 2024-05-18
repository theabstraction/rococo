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

static void AppendFunctionCall(StringBuilder& sb, cstr fqFunctionName, const ISXYFunction& f, ICFGSNode& calleeNode, ICFGSFunction& graph)
{
	for (int i = 0; i < f.InputCount(); i++)
	{
		cstr argType = f.InputType(i);
		cstr argName = f.InputName(i);
		AppendTabs(sb, 1);
		sb.AppendFormat("(%s %s)\n", argType, argName);

		auto* inputForThisArg = FindSocketInput(calleeNode, argType, argName);
		if (!inputForThisArg)
		{
			continue;
		}

		CableConnection outputConnection = FindConnectionToInput(graph, *inputForThisArg);
		ICFGSSocket* outputSocket = FindConnectionToOutput(graph, outputConnection);
		if (!Eq(outputSocket->Type().Value, inputForThisArg->Type().Value))
		{
			Throw(0, "Type mismatch between %s and %s for %s argument (%s %s)", outputSocket->Type().Value, inputForThisArg->Type().Value, fqFunctionName, argType, argName);
		}

		AppendTabs(sb, 1);
		sb.AppendFormat("(%s = %s)\n", argName, outputSocket->Name());
	}

	for (int i = 0; i < f.OutputCount(); i++)
	{
		cstr argType = f.OutputType(i);
		cstr argName = f.OutputName(i);

		AppendTabs(sb, 1);
		sb.AppendFormat("(%s %s)\n", argType, argName);
	}

	AppendTabs(sb, 1);
	sb.AppendFormat("(%s", fqFunctionName);

	for (int i = 0; i < f.InputCount(); i++)
	{
		cstr argName = f.InputName(i);
		sb.AppendFormat(" %s ", argName);
	}

	if (f.OutputCount() > 0)
	{
		sb << " -> ";
	}

	for (int i = 0; i < f.OutputCount(); i++)
	{
		cstr argName = f.OutputName(i);
		sb.AppendFormat(" %s", argName);
	}

	sb << ")\n\n";
}

static void AppendNodeImplementation(StringBuilder& sb, cstr fqType, ISexyDatabase& db, ICFGSFunction& graph, ICFGSNode& calleeNode)
{
	auto* aliasedFunction = db.FindFunction(fqType);
	if (aliasedFunction)
	{
		auto* localFunction = aliasedFunction->LocalFunction();
		if (localFunction)
		{
			AppendFunctionCall(sb, fqType, *localFunction, calleeNode, graph);
		}
		else
		{
			Throw(0, "Could not find local function aliased to %s", fqType);
		}
	}
	else
	{
		Throw(0, "Could not find public function aliased as %s", fqType);
	}
}

static void CompileFunctionBody(ICFGSFunction& f, StringBuilder& sb, ISexyDatabase& db, ICFGSFunction& graph)
{
	ICFGSNode* callerNode = FindBeginNode(f);

	if (callerNode == nullptr)
	{
		return;
	}

	std::unordered_map<ICFGSNode*, HString> nodeLabels;

	int nodeIndex = 1;

	while (true)
	{
		ICFGSCable* outgoingCable = FindOutgoingCable(*callerNode, f);
		if (!outgoingCable)
		{
			return;
		}

		auto* calleeNode = f.Nodes().FindNode(outgoingCable->EntryPoint().node);
		if (!calleeNode)
		{
			return;
		}

		if (!nodeLabels.empty())
		{
			sb << "\n";
		}

		auto i = nodeLabels.find(calleeNode);
		if (i == nodeLabels.end())
		{
			// Unhandled node, so we add implementation for it
			char label[64];
			SafeFormat(label, "node%d", nodeIndex++);

			if (GetNumberOfCallers(f, *calleeNode) > 1)
			{
				AppendTabs(sb, 1);
				sb.AppendFormat("(label %s)\n", label);
			}

			AppendNodeImplementation(sb, calleeNode->Type().Value, db, graph, *calleeNode);

			nodeLabels.insert(std::make_pair(calleeNode, HString(label)));

			callerNode = calleeNode;
		}
		else
		{
			AppendTabs(sb, 1);
			sb.AppendFormat("(goto %s)\n", i->second.c_str());
			return;
		}
	}
}

static void CompileToStringProtected(StringBuilder& sb, ISexyDatabase& db, ICFGSDatabase& cfgs, ICompileExportsSpec& exportSpec, ICFGSVariableEnumerator& variables)
{
	sb.AppendFormat("(interface %s\n", exportSpec.InterfaceName());

	cfgs.ForEachFunction(
		[&sb](ICFGSFunction& f)
		{
			cstr name = f.Name();
			if (name[0] != '_')
			{
				// Public method
				sb.AppendFormat("  (%s -> )\n", name);
			}
		}
	);

	sb << ")\n\n";

	sb.AppendFormat("(class %s (implements %s)\n", exportSpec.ClassName(), exportSpec.InterfaceName());

	variables.ForEachVariable(
		[&sb, &db](cstr name, cstr type, cstr defaultValue)
		{
			const ISXYType* sxyType = db.FindPrimitiveOrFQType(type);
			cstr correctedDefaultValue = GetCorrectedDefaultValue(sxyType, defaultValue);
			if (correctedDefaultValue)
			{
				sb.AppendFormat("  (%s %s = %s)\n", type, name, correctedDefaultValue);
			}
			else
			{
				sb.AppendFormat("  (%s %s)\n", type, name);
			}
		}
	);


	sb << ")\n";


	cfgs.ForEachFunction(
		[&sb,&db](ICFGSFunction& f)
		{
			cstr name = f.Name();
			if (name[0] == '_') name++;

			// Public method
			sb.AppendFormat("\n(method MyObject.%s", name);

			auto& beginNode = f.BeginNode();
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

			sb << " -> ";

			auto& returnNode = f.ReturnNode();
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

			sb.AppendFormat(":\n");

			CompileFunctionBody(f, sb, db, f);

			sb.AppendFormat("\n)\n");
		}
	);

	UNUSED(db);
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