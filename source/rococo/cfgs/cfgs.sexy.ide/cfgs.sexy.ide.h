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
		virtual void ForEachVariable(Function<void(cstr name, cstr type, cstr defaultValue)> callback) = 0;
	};

	ROCOCO_INTERFACE ICFGSDesignerSpacePopupPopulator
	{
		virtual ICFGSVariableEnumerator& Variables() = 0;
	};

	ROCOCO_INTERFACE ICFGSSexyPopup : ICFGSDesignerSpacePopupSupervisor
	{
		virtual void ShowInterface(Vec2i desktopPosition, Rococo::Editors::DesignerVec2 designPosition, const SexyStudio::ISXYInterface& refInterface, const SexyStudio::ISxyNamespace& ns, const CableDropped& dropInfo) = 0;
	};

	ROCOCO_INTERFACE ICompileExportsSpec
	{
		// The public full qualified interface name. Standard protocol is to take a noun that captures the essence of the method set and prefix with an I.
		virtual cstr InterfaceName() const = 0;

		// The public full qualified factory name. Standard protocol is to take the short interface name and replace the leading 'I' with 'New', so that IDog would have factory NewDog, IPrinter would have factory NewPrinter
		virtual cstr FactoryName() const = 0;

		// the private implementation class name. Standard protocol is to take the short interface name and strip the leading 'I', so that IDog would have class Dog, IPrinter would have class Printer
		virtual cstr ClassName() const = 0;
	};

	void Compile(Rococo::SexyStudio::ISexyDatabase& db, CFGS::ICFGSDatabase& cfgs, Strings::IStringPopulator& populator, ICompileExportsSpec& exportSpec, ICFGSVariableEnumerator& variables);

	ROCOCO_INTERFACE ICFGSSexyCLI
	{
		virtual void Compile(cstr targetFile) = 0;
		virtual void Free() = 0;
	};
}