#include "cfgs.sexy.ide.h"
#include "..\sexystudio\sexystudio.api.h"
#include <rococo.strings.h>
#include <rococo.cfgs.h>
#include <rococo.functional.h>
#include <vector>

using namespace Rococo::SexyStudio;
using namespace Rococo::Strings;

namespace Rococo::CFGS
{
	bool IsConnectionPermitted(const CableConnection& anchor, const ICFGSSocket& target, ICFGSDatabase& cfgs, const ISexyDatabase& db)
	{
		auto* f = cfgs.CurrentFunction();
		if (!f)
		{
			return false;
		}

		auto* srcNode = f->Nodes().FindNode(anchor.node);
		if (!srcNode)
		{
			return false;
		}

		auto* srcSocket = srcNode->FindSocket(anchor.socket);
		if (!srcSocket)
		{
			return false;
		}

		if (srcSocket->SocketClassification() == SocketClass::Exit && target.SocketClassification() == SocketClass::Trigger)
		{
			return srcSocket->CableCount() == 0;
		}

		cstr srcType = srcSocket->Type().Value;
		cstr trgType = target.Type().Value;

		if (!db.AreTypesEquivalent(srcType, trgType))
		{
			return false;
		}

		return true;
	}

	struct Cosmetics : ICFGSCosmeticsSupervisor
	{
		ISexyDatabase& db;

		Cosmetics(ISexyDatabase& _db) : db(_db)
		{
		}

		void Free()
		{
			delete this;
		}

		void ConfigSocketCosmetics(const ICFGSSocket& socket) override
		{
			switch (socket.SocketClassification())
			{
			case SocketClass::ConstOutputRef:
			case SocketClass::ConstInputRef:
			case SocketClass::InputVar:
			case SocketClass::InputRef:
			case SocketClass::OutputRef:
			case SocketClass::OutputValue:
			{
				auto colours = GetColoursForType(socket.Type().Value);
				socket.SetColours(colours.normal, colours.hilight);
				return;
			}
			case SocketClass::Exit:
			case SocketClass::Trigger:
				socket.SetColours(RGBAb(0, 224, 0, 255), RGBAb(0, 255, 0, 255));
				return;
			}
		}

		void ConfigCFGSCosmetics(ICFGSDatabase& cfgs) override
		{
			cfgs.ForEachFunction(
				[this](ICFGSFunction& f)
				{
					auto& nodes = f.Nodes();
					int32 nNodes = nodes.Count();
					for (int32 i = 0; i < nNodes; i++)
					{
						auto& node = nodes[i];
						int nSockets = node.SocketCount();
						for (int32 j = 0; j < nSockets; j++)
						{
							auto& socket = node[j];
							ConfigSocketCosmetics(socket);
						}
					}
				}
			);
		}

		Colours GetColoursForType(cstr typeName) override
		{
			static std::vector<ColourTypeBinding> bindings =
			{
				{ "Float32", "Sys.Type.Float32", { RGBAb(128,64,64,1),    RGBAb(255,128,128,1) }},
				{ "Float64", "Sys.Type.Float64", { RGBAb(192,128,0,1),    RGBAb(255,192,0,1)   }},
				{ "Int32",   "Sys.Type.Int32",   { RGBAb(128,128,128,1),  RGBAb(224,224,224,1) }},
				{ "Int64",   "Sys.Type.Int64",   { RGBAb(144,112,128,1),  RGBAb(255,160,224,1) }},
				{ "Bool",    "Sys.Type.Bool",    { RGBAb(192,128,192,1),  RGBAb(224,192,224,1) }},
			};

			for (auto b : bindings)
			{
				if (Eq(b.typeName, typeName))
				{
					return b.colours;
				}

				if (Eq(b.fqTypeName, typeName))
				{
					return b.colours;
				}
			}

			if (db.FindInterface(typeName) != nullptr)
			{
				return { RGBAb(128,128,224,1),  RGBAb(192,192,255,1) };
			}
			else // Other stuff, needs a bit of love
			{
				return { RGBAb(192,192,0,1),  RGBAb(255,255,0,1) };
			}
		}
	};

	ICFGSCosmeticsSupervisor* CreateCosmetics(ISexyDatabase& db)
	{
		return new Cosmetics(db);
	}
}