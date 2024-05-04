#pragma once
#include <rococo.types.h>
#include <rococo.cfgs.h>

namespace Rococo::SexyStudio
{
	struct ISexyDatabase;
	struct ISXYInterface;
	struct ISxyNamespace;
}

namespace Rococo::CFGS
{
	struct ColourTypeBinding
	{
		cstr typeName;
		cstr fqTypeName;
		Colours colours;
	};

	// Returns true if the anchor is allowed to connect to the target
	bool IsConnectionPermitted(const CableConnection& anchor, const ICFGSSocket& target, ICFGSDatabase& cfgs, SexyStudio::ISexyDatabase& db);

	ROCOCO_INTERFACE ICFGSCosmetics
	{
		// Assign cosmetic choices to a particular socket
		virtual void ConfigSocketCosmetics(ICFGSSocket& socket) = 0;

		// Enumerates all functions in the cfgs system and assigns cosmetic choices
		virtual void ConfigCFGSCosmetics(ICFGSDatabase& cfgs) = 0;

		// For a given typename find the appropriate colours for decorating cables and sockets.
		virtual Colours GetColoursForType(cstr typeName) = 0;
	};

	ROCOCO_INTERFACE ICFGSCosmeticsSupervisor : ICFGSCosmetics
	{
		virtual void Free() = 0;
	};

	ICFGSCosmeticsSupervisor* CreateCosmetics(SexyStudio::ISexyDatabase& db);

	ROCOCO_INTERFACE ICFGSVariableEnumerator
	{
		virtual void ForEachVariable(Function<void(cstr name, cstr type)> callback) = 0;
	};

	ROCOCO_INTERFACE ICFGSDesignerSpacePopupPopulator
	{
		virtual ICFGSVariableEnumerator& Variables() = 0;
	};

	ROCOCO_INTERFACE ICFGSSexyPopup : ICFGSDesignerSpacePopupSupervisor
	{
		virtual void ShowInterface(Vec2i desktopPosition, Rococo::Editors::DesignerVec2 designPosition, const SexyStudio::ISXYInterface& refInterface, const SexyStudio::ISxyNamespace& ns, const CableDropped& dropInfo) = 0;
	};

	void Compile(Rococo::SexyStudio::ISexyDatabase& db, CFGS::ICFGSDatabase& cfgs);

	ROCOCO_INTERFACE ICFGSSexyCLI
	{
		virtual void Compile() = 0;
		virtual void Free() = 0;
	};
}