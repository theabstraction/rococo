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
