#include <rococo.mvc.h>
#include <rococo.abstract.editor.h>
#include <rococo.strings.h>
#include <rococo.cfgs.h>
#include <rococo.sexml.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.functional.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <stdio.h>

using namespace Rococo::Editors;
using namespace Rococo::Sex;
using namespace Rococo::Sex::SEXML;

namespace Rococo::CFGS
{
	UniqueIdHolder AsUniqueId(const ISEXMLAttributeValue& a)
	{
		fstring id = AsString(a).ToFString();

		UniqueIdHolder uniqueId;
		if (sscanf_s(id, "%llx %llx", &uniqueId.iValues[0], &uniqueId.iValues[1]) != 2)
		{
			Throw(a.S(), "Expecting two 64-bit hexademical strings. Format is (Id \"<int64-hex> <int64-hex>\")");
		}

		return uniqueId;
	}

	template<class T>
	T AsId(const ISEXMLAttributeValue& a)
	{
		UniqueIdHolder id = AsUniqueId(a);
		return T{ id };
	}

	void LoadGraphNode(ICFGSFunction& f, const ISEXMLDirective& node)
	{
		auto& type = AsString(node["Type"]);
		double xPos = AsAtomicDouble(node["XPos"]);
		double yPos = AsAtomicDouble(node["YPos"]);
		auto nodeId = AsId<CFGS::NodeId>(node["Id"]);

		auto& nb = f.Nodes().Builder().AddNode(type.c_str(), { xPos, yPos }, nodeId);

		auto nChildren = node.NumberOfChildren();
		for (size_t i = 0; i < nChildren; i++)
		{
			auto& socket = node[i];
			if (!Strings::Eq(socket.FQName(), "Socket"))
			{
				Throw(node.S(), "Expecting (Socket ...)");
			}

			auto& socketType = SEXML::AsString(socket["Type"]);
			auto& socketClass = SEXML::AsString(socket["Class"]);
			auto& socketLabel = SEXML::AsString(socket["Label"]);
			auto socketId = AsId<CFGS::SocketId>(socket["Id"]);

			CFGS::SocketClass sclass;
			if (CFGS::TryParse(OUT sclass, socketClass.c_str()))
			{
				nb.AddSocket(socketType.c_str(), sclass, socketLabel.c_str(), socketId);
			}
			else
			{
				Throw(socketClass.S(), "Could not parse %s as a SocketClass enum.", socketClass.c_str());
			}

			size_t startIndex = 0;
			const ISEXMLDirective* optFields = socket.FindFirstChild(IN OUT startIndex, "Fields");
			if (optFields)
			{
				auto& fields = *optFields;
				for (size_t j = 0; j < fields.NumberOfChildren(); ++j)
				{
					auto& field = fields[j];
					auto& name = AsString(field["Name"]);
					auto& value = AsString(field["Value"]);

					nb.AddField(name.c_str(), value.c_str(), socketId);
				}
			}
		}
	}

	void LoadGraphCable(ICFGSFunction& f, const ISEXMLDirective& cable)
	{
		auto startNodeId = AsId<CFGS::NodeId>(cable["StartNode"]);
		auto startSocketId = AsId<CFGS::SocketId>(cable["StartSocket"]);
		auto endNodeId = AsId<CFGS::NodeId>(cable["EndNode"]);
		auto endSocketId = AsId<CFGS::SocketId>(cable["EndSocket"]);

		f.Cables().Add(startNodeId, startSocketId, endNodeId, endSocketId);
	}

	void LoadGraphFunction(ICFGSFunction& f, const ISEXMLDirective& fDirective)
	{
		if (fDirective.NumberOfChildren() != 2)
		{
			Throw(fDirective.S(), "Expecting one and only one [Nodes] element, and one and only one [Cables] element");
		}

		size_t startIndex = 0;
		const ISEXMLDirective& nodes = fDirective.GetDirectivesFirstChild(IN OUT startIndex, "Nodes");

		size_t nChildren = nodes.NumberOfChildren();
		for (size_t i = 0; i < nChildren; i++)
		{
			auto& node = nodes[i];
			node.Assert("Node");		
			LoadGraphNode(f, node);
		}

		startIndex = 1;
		const ISEXMLDirective& cables = fDirective.GetDirectivesFirstChild(IN OUT startIndex, "Cables");

		size_t nCables = cables.NumberOfChildren();
		for (size_t i = 0; i < nCables; i++)
		{
			auto& cable = cables[i];
			cable.Assert("Cable");
			LoadGraphCable(f, cable);
		}

		f.ConnectCablesToSockets();
	}

	void LoadDatabase(ICFGSDatabase& db, const ISEXMLDirectiveList& topLevelDirectives, ICFGSLoader& loader)
	{
		size_t startIndex = 0;
		auto& header = GetDirective(topLevelDirectives, "ControlFlowGraphSystem", IN OUT startIndex);
		auto& format = SEXML::AsString(header["FileFormat"]);
		if (!Strings::Eq(format.c_str(), "SXML"))
		{
			Throw(format.S(), "Expecting (FileFormat SEXML) in the ControlFlowGraphSystem directive");
		}

		auto& version = SEXML::AsString(header["Version"]);
		if (!Strings::Eq(version.c_str(), "1.0"))
		{
			Throw(version.S(), "Expecting (Version 1.0) in the ControlFlowGraphSystem directive. Value was '%s'", version.c_str());
		}

		startIndex = 1;
		auto& functions = GetDirective(topLevelDirectives, "Functions", IN OUT startIndex);

		size_t nChildren = functions.NumberOfChildren();
		for (size_t i = 0; i < nChildren; i++)
		{
			auto& function = functions[i];
			function.Assert("Function");
		
			FunctionId id = db.CreateFunction();
			auto* f = db.FindFunction(id);

			auto& name = SEXML::AsString(function["FQName"]);
			f->SetName(name.c_str());

			LoadGraphFunction(*f, function);
		}

		startIndex = 1;
		auto& navigation = GetDirective(topLevelDirectives, "Navigation", IN OUT startIndex);
		loader.Loader_OnLoadNavigation(navigation);
	}

	void AddId(cstr key, UniqueIdHolder id, ISEXMLBuilder& sb)
	{
		char buf[128];
		Strings::SafeFormat(buf, "%llX %llX", id.iValues[0], id.iValues[1]);
		sb.AddStringLiteral(key, buf);
	}

	void LoadDatabase(ICFGSDatabase& db, cstr filename, ICFGSLoader& loader)
	{
		Rococo::OS::LoadSXMLBySysPath(filename,
			[&db, &loader](const ISEXMLDirectiveList& directives)
			{
				db.Clear();
				LoadDatabase(db, directives, loader);
			}
		);
	}

	void SaveGraph(ICFGSFunction& f, ISEXMLBuilder& sb)
	{
		sb.AddDirective("Nodes");

		auto& nodes = f.Nodes();
		for (int i = 0; i < nodes.Count(); i++)
		{
			auto& node = nodes[i];

			sb.AddDirective("Node");
			sb.AddStringLiteral("Type", node.Type().Value);
			sb.AddAtomicAttribute("XPos", node.GetDesignRectangle().left);
			sb.AddAtomicAttribute("YPos", node.GetDesignRectangle().top);
			AddId("Id", node.Id().id, sb);

			for (int j = 0; j < node.SocketCount(); j++)
			{
				auto& socket = node[j];
				sb.AddDirective("Socket");

				sb.AddStringLiteral("Label", socket.Name());
				sb.AddStringLiteral("Type", socket.Type().Value);
				AddId("Id", socket.Id().id, sb);
				sb.AddAtomicAttribute("Class", CFGS::ToString(socket.SocketClassification()));

				size_t fieldCount = socket.EnumerateFields([](cstr, cstr, size_t) {});
				
				if (fieldCount > 0)
				{
					sb.AddDirective("Fields");

					socket.EnumerateFields(
						[&sb](cstr name, cstr value, size_t index) 
						{
							UNUSED(index);
							sb.AddDirective("Field");
							sb.AddAtomicAttribute("Name", name);
							sb.AddStringLiteral("Value", value);
							sb.CloseDirective();
						}
					);

					sb.CloseDirective();
				}

				sb.CloseDirective();
			}

			sb.CloseDirective();
		}

		sb.CloseDirective();

		sb.AddDirective("Cables");
		auto& cables = f.Cables();

		for (int i = 0; i < cables.Count(); ++i)
		{
			auto& cable = cables[i];
			sb.AddDirective("Cable");
			AddId("StartNode", cable.ExitPoint().node.id, sb);
			AddId("StartSocket", cable.ExitPoint().socket.id, sb);
			AddId("EndNode", cable.EntryPoint().node.id, sb);
			AddId("EndSocket", cable.EntryPoint().socket.id, sb);
			sb.CloseDirective();
		}

		sb.CloseDirective();
	}

	void SaveDatabase(ICFGSDatabase& db, ISEXMLBuilder& sb, ICFGArchiver& archiver)
	{
		sb.AddDirective("ControlFlowGraphSystem");
		sb.AddAtomicAttribute("FileFormat", "SXML");
		sb.AddAtomicAttribute("Version", "1.0");
		sb.CloseDirective();

		sb.AddDirective("Functions");

		db.ForEachFunction(
			[&sb](ICFGSFunction& f) 
			{
				sb.AddDirective("Function");
				sb.AddStringLiteral("FQName", f.Name());
				SaveGraph(f, sb);
				sb.CloseDirective();
			}
		);

		sb.CloseDirective();

		sb.AddDirective("Navigation");
		archiver.Archiver_OnSaveNavigation(sb);
		sb.CloseDirective();
	}
}