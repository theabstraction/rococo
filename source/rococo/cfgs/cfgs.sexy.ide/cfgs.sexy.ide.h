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

	// Assign cosmetic choices to a particular socket
	void ConfigSocket(const ICFGSSocket& socket, SexyStudio::ISexyDatabase& db);

	// Enumerates all functions in the cfgs system and assigns cosmetic choices
	void ConfigCFGS(ICFGSDatabase &cfgs, SexyStudio::ISexyDatabase & db);

	// For a given typename find the appropriate colours for decorating cables and sockets.
	Colours GetColoursForType(cstr typeName, SexyStudio::ISexyDatabase& db);
}