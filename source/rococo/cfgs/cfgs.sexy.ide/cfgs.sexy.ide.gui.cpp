#include "cfgs.sexy.ide.h"
#include "..\sexystudio\sexystudio.api.h"
#include <rococo.strings.h>
#include <rococo.cfgs.h>
#include <rococo.functional.h>
#include <rococo.sexml.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.os.h>
#include <rococo.hashtable.h>
#include <vector>
#include <algorithm>

using namespace Rococo::SexyStudio;
using namespace Rococo::Strings;
using namespace Rococo::Sex::SEXML;

namespace Rococo::CFGS
{
	bool IsConnectionPermitted(const CableConnection& anchor, const ICFGSSocket& target, ICFGSDatabase& cfgs, ISexyDatabase& db)
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

		if (db.AreTypesEquivalent(srcType, trgType))
		{
			return true;
		}

		auto* srcInterface = db.FindInterface(srcType);
		auto* trgInterface = db.FindInterface(trgType);

		if (srcInterface && srcInterface == trgInterface)
		{
			return true;
		}

		return false;
	}

	RGBAb GetSolidColour(const ISEXMLDirective& directive)
	{
		int32 r = AsAtomicInt32(directive["Red"]);
		int32 g = AsAtomicInt32(directive["Green"]);
		int32 b = AsAtomicInt32(directive["Blue"]);

		int32 R = clamp(r, 0, 255);
		int32 G = clamp(g, 0, 255);
		int32 B = clamp(b, 0, 255);

		return RGBAb((uint8) R, (uint8)G, (uint8)B, 255);
	}

	struct Cosmetics : ICFGSCosmeticsSupervisor
	{
		ISexyDatabase& db;

		stringmap<Colours> colourSchemeMap;

		Cosmetics(ISexyDatabase& _db) : db(_db)
		{
			colourSchemeMap["Float32"]          = { RGBAb(128,64,64,1),    RGBAb(255,128,128,1) };
			colourSchemeMap["Sys.Type.Float32"] = { RGBAb(128,64,64,1),    RGBAb(255,128,128,1) };
			colourSchemeMap["Float64"]			= { RGBAb(192,128,0,1),    RGBAb(255,192,0,1)   };
			colourSchemeMap["Sys.Type.Float64"] = { RGBAb(128,64,64,1),    RGBAb(255,128,128,1) };
			colourSchemeMap["Int32"]            = { RGBAb(128,128,128,1),  RGBAb(224,224,224,1) };
			colourSchemeMap["Sys.Type.Int32"]   = { RGBAb(128,128,128,1),  RGBAb(224,224,224,1) };
			colourSchemeMap["Int64"]            = { RGBAb(144,112,128,1),  RGBAb(255,160,224,1) };
			colourSchemeMap["Sys.Type.Int64"]   = { RGBAb(144,112,128,1),  RGBAb(255,160,224,1) };
			colourSchemeMap["Bool"]             = { RGBAb(192,128,192,1),  RGBAb(224,192,224,1) };
			colourSchemeMap["Sys.Type.Bool"]    = { RGBAb(192,128,192,1),  RGBAb(224,192,224,1) };
			colourSchemeMap["__Flow"]           = { RGBAb(0,224,0,1),      RGBAb(0,255,0,1)     };
			colourSchemeMap["__Interfaces"]     = { RGBAb(32,224,32,1),    RGBAb(64,255,64,1)   };
			colourSchemeMap["__Structs"]        = { RGBAb(32,64,224,1),    RGBAb(64,128,255,1)  };

			Reload();
		}

		void Free()
		{
			SaveCosmetics();

			delete this;
		}

		void SaveCosmetics()
		{
			OS::SaveUserSEXML(nullptr, "cfgs.sexy.cosmetics",
				[this](ISEXMLBuilder& builder)
				{
					builder.AddDirective("Header");
					builder.AddAtomicAttribute("FileFormat", "SXML.Cosmetics");
					builder.AddAtomicAttribute("Version", "1.0");
					builder.CloseDirective();

					builder.AddDirective("Colours");

					struct Binding
					{
						cstr key;
						Colours colours;
					};

					std::vector<Binding> sortedMap;
					sortedMap.reserve(colourSchemeMap.size());

					for (auto i : colourSchemeMap)
					{
						sortedMap.push_back({ i.first, i.second });
					}

					std::sort(sortedMap.begin(), sortedMap.end(), [](const Binding& a, const Binding& b)
						{
							return strcmp(a.key, b.key) < 0;
						}
					);

					for (auto i : sortedMap)
					{
						builder.AddDirective("Colour");

						builder.AddAtomicAttribute("Key", i.key);
						builder.AddDirective("Normal");
						builder.AddAtomicAttribute("Red", i.colours.normal.red);
						builder.AddAtomicAttribute("Green", i.colours.normal.green);
						builder.AddAtomicAttribute("Blue", i.colours.normal.blue);
						builder.CloseDirective();

						builder.AddDirective("Lit");
						builder.AddAtomicAttribute("Red", i.colours.hilight.red);
						builder.AddAtomicAttribute("Green", i.colours.hilight.green);
						builder.AddAtomicAttribute("Blue", i.colours.hilight.blue);
						builder.CloseDirective();

						builder.CloseDirective();
					}

					builder.CloseDirective();
				}
			);

		}

		void LoadCosmetics(const ISEXMLDirectiveList& directives)
		{
			size_t startIndex = 0;
			auto& header = GetDirective(directives, "Header", IN OUT startIndex);
			auto& format = SEXML::AsString(header["FileFormat"]);
			if (!Strings::Eq(format.c_str(), "SXML.Cosmetics"))
			{
				Throw(format.S(), "Expecting (FileFormat SXML.Cosmetics) in the Header directive");
			}

			auto& version = SEXML::AsString(header["Version"]);
			if (!Strings::Eq(version.c_str(), "1.0"))
			{
				Throw(version.S(), "Expecting (Version 1.0) in the Version directive. Value was '%s'", version.c_str());
			}

			startIndex = 0;
			auto& colours = GetDirective(directives, "Colours", IN OUT startIndex);

			for (int i = 0; i < colours.NumberOfChildren(); i++)
			{
				auto& colourDef = colours[i];
				colourDef.Assert("Colour");

				auto& key = AsAtomic(colourDef["Key"]);

				startIndex = 0;
				auto& dNormalColour = colourDef.GetDirectivesFirstChild(IN OUT startIndex, "Normal");
				RGBAb normalColour = GetSolidColour(dNormalColour);

				startIndex = 0;
				auto& dLitColour = colourDef.GetDirectivesFirstChild(IN OUT startIndex, "Lit");
				RGBAb litColour = GetSolidColour(dLitColour);

				colourSchemeMap[key.c_str()] = { normalColour, litColour };
			}
		}

		void Reload()
		{
			cstr section = "cfgs.sexy.cosmetics";
			
			if (Rococo::OS::IsUserSEXMLExistant(nullptr, section))
			{
				try
				{
					Rococo::OS::LoadUserSEXML(nullptr, section, 
						[this](const ISEXMLDirectiveList& directives)
						{
							LoadCosmetics(directives);
						}
					);
				}
				catch (ParseException& pex)
				{
					U8FilePath path;
					Rococo::OS::GetUserSEXMLFullPath(OUT path, nullptr, section);
					Throw(pex.ErrorCode(), "Error in %s: %s. Line %d pos %d", path.buf, pex.Message(), pex.Start().y, pex.Start().x);
				}
				catch (IException& ex)
				{
					U8FilePath path;
					Rococo::OS::GetUserSEXMLFullPath(OUT path, nullptr, section);
					Throw(ex.ErrorCode(), "Error in %s: %s", path.buf, ex.Message());
				}
			}
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
			Reload();

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
			auto i = colourSchemeMap.find(typeName);
			if (i != colourSchemeMap.end())
			{
				return i->second;
			}
			
			if (db.FindInterface(typeName) != nullptr)
			{
				return colourSchemeMap.find("__Interfaces")->second;
			}
			else // Other stuff, needs a bit of love
			{
				return colourSchemeMap.find("__Structs")->second;
			}
		}
	};

	ICFGSCosmeticsSupervisor* CreateCosmetics(ISexyDatabase& db)
	{
		return new Cosmetics(db);
	}
}