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

	bool IsConnectionPermitted(const CableConnection& anchor, const ICFGSSocket& target, ICFGSDatabase& cfgs, const SexyStudio::ISexyDatabase& db);
	void ConfigSocket(const ICFGSSocket& socket, SexyStudio::ISexyDatabase& db);
	void ConfigCFGS(ICFGSDatabase & cfgs, SexyStudio::ISexyDatabase & db);

	Colours GetColoursForType(cstr typeName, SexyStudio::ISexyDatabase& db);
}