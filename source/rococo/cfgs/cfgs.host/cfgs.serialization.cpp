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

	void LoadGraphNode(ICFGSDatabase& db, const ISEXMLDirective& node)
	{
		auto& type = AsString(node["Type"]);
		double xPos = AsAtomicDouble(node["XPos"]);
		double yPos = AsAtomicDouble(node["YPos"]);
		auto nodeId = AsId<CFGS::NodeId>(node["Id"]);

		auto& nb = db.Nodes().Builder().AddNode(type.c_str(), { xPos, yPos }, nodeId);

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
		}
	}

	void LoadGraphCable(ICFGSDatabase& db, const ISEXMLDirective& cable)
	{
		auto startNodeId = AsId<CFGS::NodeId>(cable["StartNode"]);
		auto startSocketId = AsId<CFGS::SocketId>(cable["StartSocket"]);
		auto endNodeId = AsId<CFGS::NodeId>(cable["EndNode"]);
		auto endSocketId = AsId<CFGS::SocketId>(cable["EndSocket"]);

		db.Cables().Add(startNodeId, startSocketId, endNodeId, endSocketId);
	}

	void LoadGraphSXML(IUI2DGridSlateSupervisor& gridSlate, ICFGSDatabase& db, const ISEXMLDirectiveList& topLevelDirectives)
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

		startIndex = 0;
		auto& nodes = GetDirective(topLevelDirectives, "Nodes", IN OUT startIndex);

		size_t nChildren = nodes.NumberOfChildren();
		for (size_t i = 0; i < nChildren; i++)
		{
			auto& node = nodes[i];
			if (!Strings::Eq(node.FQName(), "Node"))
			{
				Throw(node.S(), "Expecting (Node ...)");
			}

			LoadGraphNode(db, node);
		}

		startIndex = 0;
		auto& cables = GetDirective(topLevelDirectives, "Cables", IN OUT startIndex);
		size_t nCables = cables.NumberOfChildren();
		for (size_t i = 0; i < nCables; i++)
		{
			auto& cable = cables[i];
			if (!Strings::Eq(cable.FQName(), "Cable"))
			{
				Throw(cable.S(), "Expecting (Cable ...)");
			}

			LoadGraphCable(db, cable);
		}

		db.ConnectCablesToSockets();

		gridSlate.QueueRedraw();
	}

	void LoadGraph(IUI2DGridSlateSupervisor& gridSlate, ICFGSDatabase& db, const wchar_t* filename)
	{
		auto lambda = [&gridSlate, &db](const ISEXMLDirectiveList& directives)
			{
				db.Nodes().Builder().DeleteAllNodes();
				LoadGraphSXML(gridSlate, db, directives);
			};

		Rococo::OS::LoadSXMLBySysPath(filename, lambda);
	}

	void AddId(cstr key, UniqueIdHolder id, Rococo::Sex::SEXML::ISEXMLBuilder& sb)
	{
		char buf[128];
		Strings::SafeFormat(buf, "%X %X", id.iValues[0], id.iValues[1]);
		sb.AddStringLiteral(key, buf);
	}

	void SaveCurrentGraph(ICFGSDatabase& db, Rococo::Sex::SEXML::ISEXMLBuilder& sb)
	{
		sb.AddDirective("ControlFlowGraphSystem");
		sb.AddAtomicAttribute("FileFormat", "SXML");
		sb.AddAtomicAttribute("Version", "1.0");
		sb.CloseDirective();

		sb.AddDirective("Nodes");
		auto& nodes = db.Nodes();
		for (int i = 0; i < nodes.Count(); i++)
		{
			auto& node = nodes[i];

			sb.AddDirective("Node");
			sb.AddStringLiteral("Type", node.Type().Value);
			sb.AddAtomicAttribute("XPos", node.GetDesignRectangle().left);
			sb.AddAtomicAttribute("YPos", node.GetDesignRectangle().top);
			AddId("Id", node.UniqueId().id, sb);

			for (int j = 0; j < node.SocketCount(); j++)
			{
				auto& socket = node[j];
				sb.AddDirective("Socket");

				sb.AddStringLiteral("Label", socket.Name());
				sb.AddStringLiteral("Type", socket.Type().Value);
				AddId("Id", socket.Id().id, sb);
				sb.AddAtomicAttribute("Class", CFGS::ToString(socket.SocketClassification()));

				sb.CloseDirective();
			}

			sb.CloseDirective();
		}

		sb.CloseDirective();

		sb.AddDirective("Cables");
		auto& cables = db.Cables();

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
}