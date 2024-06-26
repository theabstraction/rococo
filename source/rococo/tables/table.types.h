namespace Rococo { namespace Science { namespace Materials { 
	struct PeriodicTableRow
	{
		int32 atomicNumber;
		Rococo::Science::Materials::ElementName element;
		Rococo::Science::Materials::ElementSymbol symbol;
		float atomicMass;
		boolean32 metal;
		Rococo::Science::Materials::ElementType elementType;
		float electroNegativity;
		Rococo::SI::ElectronVolts firstIonization;
		float density;
		Rococo::SI::Kelvin meltingPoint;
		Rococo::SI::Kelvin boilingPoint;
	};
}}}
namespace Rococo { namespace Strings { 
	struct LocalizedTextRowSexy
	{
		Rococo::Strings::TextId id;
		InterfacePointer english;
		InterfacePointer german;
	};
}}
namespace Rococo { namespace Quotes { 
	struct QuotesRowSexy
	{
		Rococo::Quotes::QuoteId id;
		InterfacePointer text;
	};
}}
namespace Rococo { namespace Test { namespace UserDemo { 
	struct UsersRowSexy
	{
		InterfacePointer ownerId;
		int64 purchaseId;
	};
}}}
