#pragma once
#include <rococo.types.h>

namespace Rococo::SexyStudio
{
	struct ISexyDatabase;	
}

namespace Rococo::CFGS
{
	struct CableConnection;
	struct ICFGSDatabase;
	struct ICFGSSocket;

	struct Colours
	{
		RGBAb normal;
		RGBAb hilight;
	};

	struct ColourTypeBinding
	{
		cstr typeName;
		cstr fqTypeName;
		Colours colours;
	};

	// Returns true if the anchor is allowed to connect to the target
	bool IsConnectionPermitted(const CableConnection& anchor, const ICFGSSocket& target, ICFGSDatabase& cfgs, const SexyStudio::ISexyDatabase& db);

	ROCOCO_INTERFACE ICFGSCosmetics
	{
		// Assign cosmetic choices to a particular socket
		virtual void ConfigSocketCosmetics(const ICFGSSocket& socket);

		// Enumerates all functions in the cfgs system and assigns cosmetic choices
		virtual void ConfigCFGSCosmetics(ICFGSDatabase& cfgs);

		// For a given typename find the appropriate colours for decorating cables and sockets.
		virtual Colours GetColoursForType(cstr typeName);
	};

	ROCOCO_INTERFACE ICFGSCosmeticsSupervisor : ICFGSCosmetics
	{
		virtual void Free() = 0;
	};

	ICFGSCosmeticsSupervisor* CreateCosmetics(SexyStudio::ISexyDatabase& db);
}