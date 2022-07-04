namespace HV
{
	bool TryParse(const Rococo::fstring& s, TriggerType& value)
	{
		if (s ==  "TriggerType_None"_fstring)
		{
			value = TriggerType::None;
		}
		else if (s ==  "TriggerType_Depressed"_fstring)
		{
			value = TriggerType::Depressed;
		}
		else if (s ==  "TriggerType_Pressed"_fstring)
		{
			value = TriggerType::Pressed;
		}
		else if (s ==  "TriggerType_LevelLoad"_fstring)
		{
			value = TriggerType::LevelLoad;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, TriggerType& value)
	{
		if (s ==  "None"_fstring)
		{
			value = TriggerType::None;
		}
		else if (s ==  "Depressed"_fstring)
		{
			value = TriggerType::Depressed;
		}
		else if (s ==  "Pressed"_fstring)
		{
			value = TriggerType::Pressed;
		}
		else if (s ==  "LevelLoad"_fstring)
		{
			value = TriggerType::LevelLoad;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(TriggerType value)
	{
		switch(value)
		{
			case TriggerType::None:
				return "None"_fstring;
			case TriggerType::Depressed:
				return "Depressed"_fstring;
			case TriggerType::Pressed:
				return "Pressed"_fstring;
			case TriggerType::LevelLoad:
				return "LevelLoad"_fstring;
			default:
				return {"",0};
		}
	}
}// HV.TriggerType

namespace HV::Chemicals
{
	bool TryParse(const Rococo::fstring& s, Element& value)
	{
		if (s ==  "Element_None"_fstring)
		{
			value = Element::None;
		}
		else if (s ==  "Element_Hydrogen"_fstring)
		{
			value = Element::Hydrogen;
		}
		else if (s ==  "Element_Helium"_fstring)
		{
			value = Element::Helium;
		}
		else if (s ==  "Element_Lithium"_fstring)
		{
			value = Element::Lithium;
		}
		else if (s ==  "Element_Beryllium"_fstring)
		{
			value = Element::Beryllium;
		}
		else if (s ==  "Element_Boron"_fstring)
		{
			value = Element::Boron;
		}
		else if (s ==  "Element_Carbon"_fstring)
		{
			value = Element::Carbon;
		}
		else if (s ==  "Element_Nitrogen"_fstring)
		{
			value = Element::Nitrogen;
		}
		else if (s ==  "Element_Oxygen"_fstring)
		{
			value = Element::Oxygen;
		}
		else if (s ==  "Element_Flourine"_fstring)
		{
			value = Element::Flourine;
		}
		else if (s ==  "Element_Neon"_fstring)
		{
			value = Element::Neon;
		}
		else if (s ==  "Element_Sodium"_fstring)
		{
			value = Element::Sodium;
		}
		else if (s ==  "Element_Magnesium"_fstring)
		{
			value = Element::Magnesium;
		}
		else if (s ==  "Element_Aluminium"_fstring)
		{
			value = Element::Aluminium;
		}
		else if (s ==  "Element_Silicon"_fstring)
		{
			value = Element::Silicon;
		}
		else if (s ==  "Element_Phosphorous"_fstring)
		{
			value = Element::Phosphorous;
		}
		else if (s ==  "Element_Sulphor"_fstring)
		{
			value = Element::Sulphor;
		}
		else if (s ==  "Element_Chlorine"_fstring)
		{
			value = Element::Chlorine;
		}
		else if (s ==  "Element_Argon"_fstring)
		{
			value = Element::Argon;
		}
		else if (s ==  "Element_Potassium"_fstring)
		{
			value = Element::Potassium;
		}
		else if (s ==  "Element_Calcium"_fstring)
		{
			value = Element::Calcium;
		}
		else if (s ==  "Element_Scandium"_fstring)
		{
			value = Element::Scandium;
		}
		else if (s ==  "Element_Titanium"_fstring)
		{
			value = Element::Titanium;
		}
		else if (s ==  "Element_Vanadium"_fstring)
		{
			value = Element::Vanadium;
		}
		else if (s ==  "Element_Chromium"_fstring)
		{
			value = Element::Chromium;
		}
		else if (s ==  "Element_Manganese"_fstring)
		{
			value = Element::Manganese;
		}
		else if (s ==  "Element_Iron"_fstring)
		{
			value = Element::Iron;
		}
		else if (s ==  "Element_Cobalt"_fstring)
		{
			value = Element::Cobalt;
		}
		else if (s ==  "Element_Nickel"_fstring)
		{
			value = Element::Nickel;
		}
		else if (s ==  "Element_Copper"_fstring)
		{
			value = Element::Copper;
		}
		else if (s ==  "Element_Zinc"_fstring)
		{
			value = Element::Zinc;
		}
		else if (s ==  "Element_Gallium"_fstring)
		{
			value = Element::Gallium;
		}
		else if (s ==  "Element_Germanium"_fstring)
		{
			value = Element::Germanium;
		}
		else if (s ==  "Element_Arsenic"_fstring)
		{
			value = Element::Arsenic;
		}
		else if (s ==  "Element_Selenium"_fstring)
		{
			value = Element::Selenium;
		}
		else if (s ==  "Element_Bromine"_fstring)
		{
			value = Element::Bromine;
		}
		else if (s ==  "Element_Krypton"_fstring)
		{
			value = Element::Krypton;
		}
		else if (s ==  "Element_Rubidium"_fstring)
		{
			value = Element::Rubidium;
		}
		else if (s ==  "Element_Stontium"_fstring)
		{
			value = Element::Stontium;
		}
		else if (s ==  "Element_Yttrium"_fstring)
		{
			value = Element::Yttrium;
		}
		else if (s ==  "Element_Zirconium"_fstring)
		{
			value = Element::Zirconium;
		}
		else if (s ==  "Element_Niobium"_fstring)
		{
			value = Element::Niobium;
		}
		else if (s ==  "Element_Molybdenum"_fstring)
		{
			value = Element::Molybdenum;
		}
		else if (s ==  "Element_Technetium"_fstring)
		{
			value = Element::Technetium;
		}
		else if (s ==  "Element_Ruthenium"_fstring)
		{
			value = Element::Ruthenium;
		}
		else if (s ==  "Element_Rhodium"_fstring)
		{
			value = Element::Rhodium;
		}
		else if (s ==  "Element_Palladium"_fstring)
		{
			value = Element::Palladium;
		}
		else if (s ==  "Element_Silver"_fstring)
		{
			value = Element::Silver;
		}
		else if (s ==  "Element_Cadmium"_fstring)
		{
			value = Element::Cadmium;
		}
		else if (s ==  "Element_Indium"_fstring)
		{
			value = Element::Indium;
		}
		else if (s ==  "Element_Tin"_fstring)
		{
			value = Element::Tin;
		}
		else if (s ==  "Element_Antimony"_fstring)
		{
			value = Element::Antimony;
		}
		else if (s ==  "Element_Tellurium"_fstring)
		{
			value = Element::Tellurium;
		}
		else if (s ==  "Element_Iodine"_fstring)
		{
			value = Element::Iodine;
		}
		else if (s ==  "Element_Xenon"_fstring)
		{
			value = Element::Xenon;
		}
		else if (s ==  "Element_Caesium"_fstring)
		{
			value = Element::Caesium;
		}
		else if (s ==  "Element_Barium"_fstring)
		{
			value = Element::Barium;
		}
		else if (s ==  "Element_Lanthanum"_fstring)
		{
			value = Element::Lanthanum;
		}
		else if (s ==  "Element_Cerium"_fstring)
		{
			value = Element::Cerium;
		}
		else if (s ==  "Element_Praseodymium"_fstring)
		{
			value = Element::Praseodymium;
		}
		else if (s ==  "Element_Neodymium"_fstring)
		{
			value = Element::Neodymium;
		}
		else if (s ==  "Element_Promethium"_fstring)
		{
			value = Element::Promethium;
		}
		else if (s ==  "Element_Samarium"_fstring)
		{
			value = Element::Samarium;
		}
		else if (s ==  "Element_Europium"_fstring)
		{
			value = Element::Europium;
		}
		else if (s ==  "Element_Gadolinium"_fstring)
		{
			value = Element::Gadolinium;
		}
		else if (s ==  "Element_Terbium"_fstring)
		{
			value = Element::Terbium;
		}
		else if (s ==  "Element_Dysprosium"_fstring)
		{
			value = Element::Dysprosium;
		}
		else if (s ==  "Element_Holmium"_fstring)
		{
			value = Element::Holmium;
		}
		else if (s ==  "Element_Erbium"_fstring)
		{
			value = Element::Erbium;
		}
		else if (s ==  "Element_Thulium"_fstring)
		{
			value = Element::Thulium;
		}
		else if (s ==  "Element_Ytterbium"_fstring)
		{
			value = Element::Ytterbium;
		}
		else if (s ==  "Element_Lutetium"_fstring)
		{
			value = Element::Lutetium;
		}
		else if (s ==  "Element_Hafnium"_fstring)
		{
			value = Element::Hafnium;
		}
		else if (s ==  "Element_Tantalum"_fstring)
		{
			value = Element::Tantalum;
		}
		else if (s ==  "Element_Tungsten"_fstring)
		{
			value = Element::Tungsten;
		}
		else if (s ==  "Element_Rhenium"_fstring)
		{
			value = Element::Rhenium;
		}
		else if (s ==  "Element_Osmium"_fstring)
		{
			value = Element::Osmium;
		}
		else if (s ==  "Element_Iridium"_fstring)
		{
			value = Element::Iridium;
		}
		else if (s ==  "Element_Platinum"_fstring)
		{
			value = Element::Platinum;
		}
		else if (s ==  "Element_Gold"_fstring)
		{
			value = Element::Gold;
		}
		else if (s ==  "Element_Mercury"_fstring)
		{
			value = Element::Mercury;
		}
		else if (s ==  "Element_Thallium"_fstring)
		{
			value = Element::Thallium;
		}
		else if (s ==  "Element_Lead"_fstring)
		{
			value = Element::Lead;
		}
		else if (s ==  "Element_Bismuth"_fstring)
		{
			value = Element::Bismuth;
		}
		else if (s ==  "Element_Polonium"_fstring)
		{
			value = Element::Polonium;
		}
		else if (s ==  "Element_Astatine"_fstring)
		{
			value = Element::Astatine;
		}
		else if (s ==  "Element_Radon"_fstring)
		{
			value = Element::Radon;
		}
		else if (s ==  "Element_Francium"_fstring)
		{
			value = Element::Francium;
		}
		else if (s ==  "Element_Radium"_fstring)
		{
			value = Element::Radium;
		}
		else if (s ==  "Element_Actinium"_fstring)
		{
			value = Element::Actinium;
		}
		else if (s ==  "Element_Thorium"_fstring)
		{
			value = Element::Thorium;
		}
		else if (s ==  "Element_Protactinium"_fstring)
		{
			value = Element::Protactinium;
		}
		else if (s ==  "Element_Uranium"_fstring)
		{
			value = Element::Uranium;
		}
		else if (s ==  "Element_Neptunium"_fstring)
		{
			value = Element::Neptunium;
		}
		else if (s ==  "Element_Plutonium"_fstring)
		{
			value = Element::Plutonium;
		}
		else if (s ==  "Element_Americium"_fstring)
		{
			value = Element::Americium;
		}
		else if (s ==  "Element_Curium"_fstring)
		{
			value = Element::Curium;
		}
		else if (s ==  "Element_Berkelium"_fstring)
		{
			value = Element::Berkelium;
		}
		else if (s ==  "Element_Californium"_fstring)
		{
			value = Element::Californium;
		}
		else if (s ==  "Element_Einsteinium"_fstring)
		{
			value = Element::Einsteinium;
		}
		else if (s ==  "Element_Fermium"_fstring)
		{
			value = Element::Fermium;
		}
		else if (s ==  "Element_Mendelevium"_fstring)
		{
			value = Element::Mendelevium;
		}
		else if (s ==  "Element_Nobelium"_fstring)
		{
			value = Element::Nobelium;
		}
		else if (s ==  "Element_Lawrencium"_fstring)
		{
			value = Element::Lawrencium;
		}
		else if (s ==  "Element_Rutherfordium"_fstring)
		{
			value = Element::Rutherfordium;
		}
		else if (s ==  "Element_Dubnium"_fstring)
		{
			value = Element::Dubnium;
		}
		else if (s ==  "Element_Seaborgium"_fstring)
		{
			value = Element::Seaborgium;
		}
		else if (s ==  "Element_Bohrium"_fstring)
		{
			value = Element::Bohrium;
		}
		else if (s ==  "Element_Hassium"_fstring)
		{
			value = Element::Hassium;
		}
		else if (s ==  "Element_Meitnerium"_fstring)
		{
			value = Element::Meitnerium;
		}
		else if (s ==  "Element_Darmstadtium"_fstring)
		{
			value = Element::Darmstadtium;
		}
		else if (s ==  "Element_Roentgenium"_fstring)
		{
			value = Element::Roentgenium;
		}
		else if (s ==  "Element_Copernicium"_fstring)
		{
			value = Element::Copernicium;
		}
		else if (s ==  "Element_Unutrium"_fstring)
		{
			value = Element::Unutrium;
		}
		else if (s ==  "Element_Flerovium"_fstring)
		{
			value = Element::Flerovium;
		}
		else if (s ==  "Element_Unupentium"_fstring)
		{
			value = Element::Unupentium;
		}
		else if (s ==  "Element_Livermorium"_fstring)
		{
			value = Element::Livermorium;
		}
		else if (s ==  "Element_Unuseptium"_fstring)
		{
			value = Element::Unuseptium;
		}
		else if (s ==  "Element_Ununoctium"_fstring)
		{
			value = Element::Ununoctium;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, Element& value)
	{
		if (s ==  "None"_fstring)
		{
			value = Element::None;
		}
		else if (s ==  "Hydrogen"_fstring)
		{
			value = Element::Hydrogen;
		}
		else if (s ==  "Helium"_fstring)
		{
			value = Element::Helium;
		}
		else if (s ==  "Lithium"_fstring)
		{
			value = Element::Lithium;
		}
		else if (s ==  "Beryllium"_fstring)
		{
			value = Element::Beryllium;
		}
		else if (s ==  "Boron"_fstring)
		{
			value = Element::Boron;
		}
		else if (s ==  "Carbon"_fstring)
		{
			value = Element::Carbon;
		}
		else if (s ==  "Nitrogen"_fstring)
		{
			value = Element::Nitrogen;
		}
		else if (s ==  "Oxygen"_fstring)
		{
			value = Element::Oxygen;
		}
		else if (s ==  "Flourine"_fstring)
		{
			value = Element::Flourine;
		}
		else if (s ==  "Neon"_fstring)
		{
			value = Element::Neon;
		}
		else if (s ==  "Sodium"_fstring)
		{
			value = Element::Sodium;
		}
		else if (s ==  "Magnesium"_fstring)
		{
			value = Element::Magnesium;
		}
		else if (s ==  "Aluminium"_fstring)
		{
			value = Element::Aluminium;
		}
		else if (s ==  "Silicon"_fstring)
		{
			value = Element::Silicon;
		}
		else if (s ==  "Phosphorous"_fstring)
		{
			value = Element::Phosphorous;
		}
		else if (s ==  "Sulphor"_fstring)
		{
			value = Element::Sulphor;
		}
		else if (s ==  "Chlorine"_fstring)
		{
			value = Element::Chlorine;
		}
		else if (s ==  "Argon"_fstring)
		{
			value = Element::Argon;
		}
		else if (s ==  "Potassium"_fstring)
		{
			value = Element::Potassium;
		}
		else if (s ==  "Calcium"_fstring)
		{
			value = Element::Calcium;
		}
		else if (s ==  "Scandium"_fstring)
		{
			value = Element::Scandium;
		}
		else if (s ==  "Titanium"_fstring)
		{
			value = Element::Titanium;
		}
		else if (s ==  "Vanadium"_fstring)
		{
			value = Element::Vanadium;
		}
		else if (s ==  "Chromium"_fstring)
		{
			value = Element::Chromium;
		}
		else if (s ==  "Manganese"_fstring)
		{
			value = Element::Manganese;
		}
		else if (s ==  "Iron"_fstring)
		{
			value = Element::Iron;
		}
		else if (s ==  "Cobalt"_fstring)
		{
			value = Element::Cobalt;
		}
		else if (s ==  "Nickel"_fstring)
		{
			value = Element::Nickel;
		}
		else if (s ==  "Copper"_fstring)
		{
			value = Element::Copper;
		}
		else if (s ==  "Zinc"_fstring)
		{
			value = Element::Zinc;
		}
		else if (s ==  "Gallium"_fstring)
		{
			value = Element::Gallium;
		}
		else if (s ==  "Germanium"_fstring)
		{
			value = Element::Germanium;
		}
		else if (s ==  "Arsenic"_fstring)
		{
			value = Element::Arsenic;
		}
		else if (s ==  "Selenium"_fstring)
		{
			value = Element::Selenium;
		}
		else if (s ==  "Bromine"_fstring)
		{
			value = Element::Bromine;
		}
		else if (s ==  "Krypton"_fstring)
		{
			value = Element::Krypton;
		}
		else if (s ==  "Rubidium"_fstring)
		{
			value = Element::Rubidium;
		}
		else if (s ==  "Stontium"_fstring)
		{
			value = Element::Stontium;
		}
		else if (s ==  "Yttrium"_fstring)
		{
			value = Element::Yttrium;
		}
		else if (s ==  "Zirconium"_fstring)
		{
			value = Element::Zirconium;
		}
		else if (s ==  "Niobium"_fstring)
		{
			value = Element::Niobium;
		}
		else if (s ==  "Molybdenum"_fstring)
		{
			value = Element::Molybdenum;
		}
		else if (s ==  "Technetium"_fstring)
		{
			value = Element::Technetium;
		}
		else if (s ==  "Ruthenium"_fstring)
		{
			value = Element::Ruthenium;
		}
		else if (s ==  "Rhodium"_fstring)
		{
			value = Element::Rhodium;
		}
		else if (s ==  "Palladium"_fstring)
		{
			value = Element::Palladium;
		}
		else if (s ==  "Silver"_fstring)
		{
			value = Element::Silver;
		}
		else if (s ==  "Cadmium"_fstring)
		{
			value = Element::Cadmium;
		}
		else if (s ==  "Indium"_fstring)
		{
			value = Element::Indium;
		}
		else if (s ==  "Tin"_fstring)
		{
			value = Element::Tin;
		}
		else if (s ==  "Antimony"_fstring)
		{
			value = Element::Antimony;
		}
		else if (s ==  "Tellurium"_fstring)
		{
			value = Element::Tellurium;
		}
		else if (s ==  "Iodine"_fstring)
		{
			value = Element::Iodine;
		}
		else if (s ==  "Xenon"_fstring)
		{
			value = Element::Xenon;
		}
		else if (s ==  "Caesium"_fstring)
		{
			value = Element::Caesium;
		}
		else if (s ==  "Barium"_fstring)
		{
			value = Element::Barium;
		}
		else if (s ==  "Lanthanum"_fstring)
		{
			value = Element::Lanthanum;
		}
		else if (s ==  "Cerium"_fstring)
		{
			value = Element::Cerium;
		}
		else if (s ==  "Praseodymium"_fstring)
		{
			value = Element::Praseodymium;
		}
		else if (s ==  "Neodymium"_fstring)
		{
			value = Element::Neodymium;
		}
		else if (s ==  "Promethium"_fstring)
		{
			value = Element::Promethium;
		}
		else if (s ==  "Samarium"_fstring)
		{
			value = Element::Samarium;
		}
		else if (s ==  "Europium"_fstring)
		{
			value = Element::Europium;
		}
		else if (s ==  "Gadolinium"_fstring)
		{
			value = Element::Gadolinium;
		}
		else if (s ==  "Terbium"_fstring)
		{
			value = Element::Terbium;
		}
		else if (s ==  "Dysprosium"_fstring)
		{
			value = Element::Dysprosium;
		}
		else if (s ==  "Holmium"_fstring)
		{
			value = Element::Holmium;
		}
		else if (s ==  "Erbium"_fstring)
		{
			value = Element::Erbium;
		}
		else if (s ==  "Thulium"_fstring)
		{
			value = Element::Thulium;
		}
		else if (s ==  "Ytterbium"_fstring)
		{
			value = Element::Ytterbium;
		}
		else if (s ==  "Lutetium"_fstring)
		{
			value = Element::Lutetium;
		}
		else if (s ==  "Hafnium"_fstring)
		{
			value = Element::Hafnium;
		}
		else if (s ==  "Tantalum"_fstring)
		{
			value = Element::Tantalum;
		}
		else if (s ==  "Tungsten"_fstring)
		{
			value = Element::Tungsten;
		}
		else if (s ==  "Rhenium"_fstring)
		{
			value = Element::Rhenium;
		}
		else if (s ==  "Osmium"_fstring)
		{
			value = Element::Osmium;
		}
		else if (s ==  "Iridium"_fstring)
		{
			value = Element::Iridium;
		}
		else if (s ==  "Platinum"_fstring)
		{
			value = Element::Platinum;
		}
		else if (s ==  "Gold"_fstring)
		{
			value = Element::Gold;
		}
		else if (s ==  "Mercury"_fstring)
		{
			value = Element::Mercury;
		}
		else if (s ==  "Thallium"_fstring)
		{
			value = Element::Thallium;
		}
		else if (s ==  "Lead"_fstring)
		{
			value = Element::Lead;
		}
		else if (s ==  "Bismuth"_fstring)
		{
			value = Element::Bismuth;
		}
		else if (s ==  "Polonium"_fstring)
		{
			value = Element::Polonium;
		}
		else if (s ==  "Astatine"_fstring)
		{
			value = Element::Astatine;
		}
		else if (s ==  "Radon"_fstring)
		{
			value = Element::Radon;
		}
		else if (s ==  "Francium"_fstring)
		{
			value = Element::Francium;
		}
		else if (s ==  "Radium"_fstring)
		{
			value = Element::Radium;
		}
		else if (s ==  "Actinium"_fstring)
		{
			value = Element::Actinium;
		}
		else if (s ==  "Thorium"_fstring)
		{
			value = Element::Thorium;
		}
		else if (s ==  "Protactinium"_fstring)
		{
			value = Element::Protactinium;
		}
		else if (s ==  "Uranium"_fstring)
		{
			value = Element::Uranium;
		}
		else if (s ==  "Neptunium"_fstring)
		{
			value = Element::Neptunium;
		}
		else if (s ==  "Plutonium"_fstring)
		{
			value = Element::Plutonium;
		}
		else if (s ==  "Americium"_fstring)
		{
			value = Element::Americium;
		}
		else if (s ==  "Curium"_fstring)
		{
			value = Element::Curium;
		}
		else if (s ==  "Berkelium"_fstring)
		{
			value = Element::Berkelium;
		}
		else if (s ==  "Californium"_fstring)
		{
			value = Element::Californium;
		}
		else if (s ==  "Einsteinium"_fstring)
		{
			value = Element::Einsteinium;
		}
		else if (s ==  "Fermium"_fstring)
		{
			value = Element::Fermium;
		}
		else if (s ==  "Mendelevium"_fstring)
		{
			value = Element::Mendelevium;
		}
		else if (s ==  "Nobelium"_fstring)
		{
			value = Element::Nobelium;
		}
		else if (s ==  "Lawrencium"_fstring)
		{
			value = Element::Lawrencium;
		}
		else if (s ==  "Rutherfordium"_fstring)
		{
			value = Element::Rutherfordium;
		}
		else if (s ==  "Dubnium"_fstring)
		{
			value = Element::Dubnium;
		}
		else if (s ==  "Seaborgium"_fstring)
		{
			value = Element::Seaborgium;
		}
		else if (s ==  "Bohrium"_fstring)
		{
			value = Element::Bohrium;
		}
		else if (s ==  "Hassium"_fstring)
		{
			value = Element::Hassium;
		}
		else if (s ==  "Meitnerium"_fstring)
		{
			value = Element::Meitnerium;
		}
		else if (s ==  "Darmstadtium"_fstring)
		{
			value = Element::Darmstadtium;
		}
		else if (s ==  "Roentgenium"_fstring)
		{
			value = Element::Roentgenium;
		}
		else if (s ==  "Copernicium"_fstring)
		{
			value = Element::Copernicium;
		}
		else if (s ==  "Unutrium"_fstring)
		{
			value = Element::Unutrium;
		}
		else if (s ==  "Flerovium"_fstring)
		{
			value = Element::Flerovium;
		}
		else if (s ==  "Unupentium"_fstring)
		{
			value = Element::Unupentium;
		}
		else if (s ==  "Livermorium"_fstring)
		{
			value = Element::Livermorium;
		}
		else if (s ==  "Unuseptium"_fstring)
		{
			value = Element::Unuseptium;
		}
		else if (s ==  "Ununoctium"_fstring)
		{
			value = Element::Ununoctium;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(Element value)
	{
		switch(value)
		{
			case Element::None:
				return "None"_fstring;
			case Element::Hydrogen:
				return "Hydrogen"_fstring;
			case Element::Helium:
				return "Helium"_fstring;
			case Element::Lithium:
				return "Lithium"_fstring;
			case Element::Beryllium:
				return "Beryllium"_fstring;
			case Element::Boron:
				return "Boron"_fstring;
			case Element::Carbon:
				return "Carbon"_fstring;
			case Element::Nitrogen:
				return "Nitrogen"_fstring;
			case Element::Oxygen:
				return "Oxygen"_fstring;
			case Element::Flourine:
				return "Flourine"_fstring;
			case Element::Neon:
				return "Neon"_fstring;
			case Element::Sodium:
				return "Sodium"_fstring;
			case Element::Magnesium:
				return "Magnesium"_fstring;
			case Element::Aluminium:
				return "Aluminium"_fstring;
			case Element::Silicon:
				return "Silicon"_fstring;
			case Element::Phosphorous:
				return "Phosphorous"_fstring;
			case Element::Sulphor:
				return "Sulphor"_fstring;
			case Element::Chlorine:
				return "Chlorine"_fstring;
			case Element::Argon:
				return "Argon"_fstring;
			case Element::Potassium:
				return "Potassium"_fstring;
			case Element::Calcium:
				return "Calcium"_fstring;
			case Element::Scandium:
				return "Scandium"_fstring;
			case Element::Titanium:
				return "Titanium"_fstring;
			case Element::Vanadium:
				return "Vanadium"_fstring;
			case Element::Chromium:
				return "Chromium"_fstring;
			case Element::Manganese:
				return "Manganese"_fstring;
			case Element::Iron:
				return "Iron"_fstring;
			case Element::Cobalt:
				return "Cobalt"_fstring;
			case Element::Nickel:
				return "Nickel"_fstring;
			case Element::Copper:
				return "Copper"_fstring;
			case Element::Zinc:
				return "Zinc"_fstring;
			case Element::Gallium:
				return "Gallium"_fstring;
			case Element::Germanium:
				return "Germanium"_fstring;
			case Element::Arsenic:
				return "Arsenic"_fstring;
			case Element::Selenium:
				return "Selenium"_fstring;
			case Element::Bromine:
				return "Bromine"_fstring;
			case Element::Krypton:
				return "Krypton"_fstring;
			case Element::Rubidium:
				return "Rubidium"_fstring;
			case Element::Stontium:
				return "Stontium"_fstring;
			case Element::Yttrium:
				return "Yttrium"_fstring;
			case Element::Zirconium:
				return "Zirconium"_fstring;
			case Element::Niobium:
				return "Niobium"_fstring;
			case Element::Molybdenum:
				return "Molybdenum"_fstring;
			case Element::Technetium:
				return "Technetium"_fstring;
			case Element::Ruthenium:
				return "Ruthenium"_fstring;
			case Element::Rhodium:
				return "Rhodium"_fstring;
			case Element::Palladium:
				return "Palladium"_fstring;
			case Element::Silver:
				return "Silver"_fstring;
			case Element::Cadmium:
				return "Cadmium"_fstring;
			case Element::Indium:
				return "Indium"_fstring;
			case Element::Tin:
				return "Tin"_fstring;
			case Element::Antimony:
				return "Antimony"_fstring;
			case Element::Tellurium:
				return "Tellurium"_fstring;
			case Element::Iodine:
				return "Iodine"_fstring;
			case Element::Xenon:
				return "Xenon"_fstring;
			case Element::Caesium:
				return "Caesium"_fstring;
			case Element::Barium:
				return "Barium"_fstring;
			case Element::Lanthanum:
				return "Lanthanum"_fstring;
			case Element::Cerium:
				return "Cerium"_fstring;
			case Element::Praseodymium:
				return "Praseodymium"_fstring;
			case Element::Neodymium:
				return "Neodymium"_fstring;
			case Element::Promethium:
				return "Promethium"_fstring;
			case Element::Samarium:
				return "Samarium"_fstring;
			case Element::Europium:
				return "Europium"_fstring;
			case Element::Gadolinium:
				return "Gadolinium"_fstring;
			case Element::Terbium:
				return "Terbium"_fstring;
			case Element::Dysprosium:
				return "Dysprosium"_fstring;
			case Element::Holmium:
				return "Holmium"_fstring;
			case Element::Erbium:
				return "Erbium"_fstring;
			case Element::Thulium:
				return "Thulium"_fstring;
			case Element::Ytterbium:
				return "Ytterbium"_fstring;
			case Element::Lutetium:
				return "Lutetium"_fstring;
			case Element::Hafnium:
				return "Hafnium"_fstring;
			case Element::Tantalum:
				return "Tantalum"_fstring;
			case Element::Tungsten:
				return "Tungsten"_fstring;
			case Element::Rhenium:
				return "Rhenium"_fstring;
			case Element::Osmium:
				return "Osmium"_fstring;
			case Element::Iridium:
				return "Iridium"_fstring;
			case Element::Platinum:
				return "Platinum"_fstring;
			case Element::Gold:
				return "Gold"_fstring;
			case Element::Mercury:
				return "Mercury"_fstring;
			case Element::Thallium:
				return "Thallium"_fstring;
			case Element::Lead:
				return "Lead"_fstring;
			case Element::Bismuth:
				return "Bismuth"_fstring;
			case Element::Polonium:
				return "Polonium"_fstring;
			case Element::Astatine:
				return "Astatine"_fstring;
			case Element::Radon:
				return "Radon"_fstring;
			case Element::Francium:
				return "Francium"_fstring;
			case Element::Radium:
				return "Radium"_fstring;
			case Element::Actinium:
				return "Actinium"_fstring;
			case Element::Thorium:
				return "Thorium"_fstring;
			case Element::Protactinium:
				return "Protactinium"_fstring;
			case Element::Uranium:
				return "Uranium"_fstring;
			case Element::Neptunium:
				return "Neptunium"_fstring;
			case Element::Plutonium:
				return "Plutonium"_fstring;
			case Element::Americium:
				return "Americium"_fstring;
			case Element::Curium:
				return "Curium"_fstring;
			case Element::Berkelium:
				return "Berkelium"_fstring;
			case Element::Californium:
				return "Californium"_fstring;
			case Element::Einsteinium:
				return "Einsteinium"_fstring;
			case Element::Fermium:
				return "Fermium"_fstring;
			case Element::Mendelevium:
				return "Mendelevium"_fstring;
			case Element::Nobelium:
				return "Nobelium"_fstring;
			case Element::Lawrencium:
				return "Lawrencium"_fstring;
			case Element::Rutherfordium:
				return "Rutherfordium"_fstring;
			case Element::Dubnium:
				return "Dubnium"_fstring;
			case Element::Seaborgium:
				return "Seaborgium"_fstring;
			case Element::Bohrium:
				return "Bohrium"_fstring;
			case Element::Hassium:
				return "Hassium"_fstring;
			case Element::Meitnerium:
				return "Meitnerium"_fstring;
			case Element::Darmstadtium:
				return "Darmstadtium"_fstring;
			case Element::Roentgenium:
				return "Roentgenium"_fstring;
			case Element::Copernicium:
				return "Copernicium"_fstring;
			case Element::Unutrium:
				return "Unutrium"_fstring;
			case Element::Flerovium:
				return "Flerovium"_fstring;
			case Element::Unupentium:
				return "Unupentium"_fstring;
			case Element::Livermorium:
				return "Livermorium"_fstring;
			case Element::Unuseptium:
				return "Unuseptium"_fstring;
			case Element::Ununoctium:
				return "Ununoctium"_fstring;
			default:
				return {"",0};
		}
	}
}// HV.Chemicals.Element

namespace HV::Chemicals
{
	bool TryParse(const Rococo::fstring& s, Compounds& value)
	{
		if (s ==  "Compounds_Oak"_fstring)
		{
			value = Compounds::Oak;
		}
		else if (s ==  "Compounds_Pine"_fstring)
		{
			value = Compounds::Pine;
		}
		else if (s ==  "Compounds_Walnut"_fstring)
		{
			value = Compounds::Walnut;
		}
		else if (s ==  "Compounds_Teak"_fstring)
		{
			value = Compounds::Teak;
		}
		else if (s ==  "Compounds_Rosewood"_fstring)
		{
			value = Compounds::Rosewood;
		}
		else if (s ==  "Compounds_Elm"_fstring)
		{
			value = Compounds::Elm;
		}
		else if (s ==  "Compounds_Polythene"_fstring)
		{
			value = Compounds::Polythene;
		}
		else if (s ==  "Compounds_Perspex"_fstring)
		{
			value = Compounds::Perspex;
		}
		else if (s ==  "Compounds_Glass"_fstring)
		{
			value = Compounds::Glass;
		}
		else if (s ==  "Compounds_BorosilicateGlass"_fstring)
		{
			value = Compounds::BorosilicateGlass;
		}
		else if (s ==  "Compounds_Ceramic"_fstring)
		{
			value = Compounds::Ceramic;
		}
		else if (s ==  "Compounds_CermaicFibre"_fstring)
		{
			value = Compounds::CermaicFibre;
		}
		else if (s ==  "Compounds_StainlessSteel"_fstring)
		{
			value = Compounds::StainlessSteel;
		}
		else if (s ==  "Compounds_CobaltSteel"_fstring)
		{
			value = Compounds::CobaltSteel;
		}
		else if (s ==  "Compounds_ChromeSteel"_fstring)
		{
			value = Compounds::ChromeSteel;
		}
		else if (s ==  "Compounds_Bronze"_fstring)
		{
			value = Compounds::Bronze;
		}
		else if (s ==  "Compounds_Brass"_fstring)
		{
			value = Compounds::Brass;
		}
		else if (s ==  "Compounds_Gunmetal"_fstring)
		{
			value = Compounds::Gunmetal;
		}
		else if (s ==  "Compounds_MedievalSteel"_fstring)
		{
			value = Compounds::MedievalSteel;
		}
		else if (s ==  "Compounds_SpringSteel"_fstring)
		{
			value = Compounds::SpringSteel;
		}
		else if (s ==  "Compounds_Cotton"_fstring)
		{
			value = Compounds::Cotton;
		}
		else if (s ==  "Compounds_Wool"_fstring)
		{
			value = Compounds::Wool;
		}
		else if (s ==  "Compounds_Silk"_fstring)
		{
			value = Compounds::Silk;
		}
		else if (s ==  "Compounds_SpiderSilk"_fstring)
		{
			value = Compounds::SpiderSilk;
		}
		else if (s ==  "Compounds_Polyester"_fstring)
		{
			value = Compounds::Polyester;
		}
		else if (s ==  "Compounds_Spandex"_fstring)
		{
			value = Compounds::Spandex;
		}
		else if (s ==  "Compounds_Leather"_fstring)
		{
			value = Compounds::Leather;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, Compounds& value)
	{
		if (s ==  "Oak"_fstring)
		{
			value = Compounds::Oak;
		}
		else if (s ==  "Pine"_fstring)
		{
			value = Compounds::Pine;
		}
		else if (s ==  "Walnut"_fstring)
		{
			value = Compounds::Walnut;
		}
		else if (s ==  "Teak"_fstring)
		{
			value = Compounds::Teak;
		}
		else if (s ==  "Rosewood"_fstring)
		{
			value = Compounds::Rosewood;
		}
		else if (s ==  "Elm"_fstring)
		{
			value = Compounds::Elm;
		}
		else if (s ==  "Polythene"_fstring)
		{
			value = Compounds::Polythene;
		}
		else if (s ==  "Perspex"_fstring)
		{
			value = Compounds::Perspex;
		}
		else if (s ==  "Glass"_fstring)
		{
			value = Compounds::Glass;
		}
		else if (s ==  "BorosilicateGlass"_fstring)
		{
			value = Compounds::BorosilicateGlass;
		}
		else if (s ==  "Ceramic"_fstring)
		{
			value = Compounds::Ceramic;
		}
		else if (s ==  "CermaicFibre"_fstring)
		{
			value = Compounds::CermaicFibre;
		}
		else if (s ==  "StainlessSteel"_fstring)
		{
			value = Compounds::StainlessSteel;
		}
		else if (s ==  "CobaltSteel"_fstring)
		{
			value = Compounds::CobaltSteel;
		}
		else if (s ==  "ChromeSteel"_fstring)
		{
			value = Compounds::ChromeSteel;
		}
		else if (s ==  "Bronze"_fstring)
		{
			value = Compounds::Bronze;
		}
		else if (s ==  "Brass"_fstring)
		{
			value = Compounds::Brass;
		}
		else if (s ==  "Gunmetal"_fstring)
		{
			value = Compounds::Gunmetal;
		}
		else if (s ==  "MedievalSteel"_fstring)
		{
			value = Compounds::MedievalSteel;
		}
		else if (s ==  "SpringSteel"_fstring)
		{
			value = Compounds::SpringSteel;
		}
		else if (s ==  "Cotton"_fstring)
		{
			value = Compounds::Cotton;
		}
		else if (s ==  "Wool"_fstring)
		{
			value = Compounds::Wool;
		}
		else if (s ==  "Silk"_fstring)
		{
			value = Compounds::Silk;
		}
		else if (s ==  "SpiderSilk"_fstring)
		{
			value = Compounds::SpiderSilk;
		}
		else if (s ==  "Polyester"_fstring)
		{
			value = Compounds::Polyester;
		}
		else if (s ==  "Spandex"_fstring)
		{
			value = Compounds::Spandex;
		}
		else if (s ==  "Leather"_fstring)
		{
			value = Compounds::Leather;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(Compounds value)
	{
		switch(value)
		{
			case Compounds::Oak:
				return "Oak"_fstring;
			case Compounds::Pine:
				return "Pine"_fstring;
			case Compounds::Walnut:
				return "Walnut"_fstring;
			case Compounds::Teak:
				return "Teak"_fstring;
			case Compounds::Rosewood:
				return "Rosewood"_fstring;
			case Compounds::Elm:
				return "Elm"_fstring;
			case Compounds::Polythene:
				return "Polythene"_fstring;
			case Compounds::Perspex:
				return "Perspex"_fstring;
			case Compounds::Glass:
				return "Glass"_fstring;
			case Compounds::BorosilicateGlass:
				return "BorosilicateGlass"_fstring;
			case Compounds::Ceramic:
				return "Ceramic"_fstring;
			case Compounds::CermaicFibre:
				return "CermaicFibre"_fstring;
			case Compounds::StainlessSteel:
				return "StainlessSteel"_fstring;
			case Compounds::CobaltSteel:
				return "CobaltSteel"_fstring;
			case Compounds::ChromeSteel:
				return "ChromeSteel"_fstring;
			case Compounds::Bronze:
				return "Bronze"_fstring;
			case Compounds::Brass:
				return "Brass"_fstring;
			case Compounds::Gunmetal:
				return "Gunmetal"_fstring;
			case Compounds::MedievalSteel:
				return "MedievalSteel"_fstring;
			case Compounds::SpringSteel:
				return "SpringSteel"_fstring;
			case Compounds::Cotton:
				return "Cotton"_fstring;
			case Compounds::Wool:
				return "Wool"_fstring;
			case Compounds::Silk:
				return "Silk"_fstring;
			case Compounds::SpiderSilk:
				return "SpiderSilk"_fstring;
			case Compounds::Polyester:
				return "Polyester"_fstring;
			case Compounds::Spandex:
				return "Spandex"_fstring;
			case Compounds::Leather:
				return "Leather"_fstring;
			default:
				return {"",0};
		}
	}
}// HV.Chemicals.Compounds

namespace HV
{
	bool TryParse(const Rococo::fstring& s, EquipmentSlot& value)
	{
		if (s ==  "EquipmentSlot_None"_fstring)
		{
			value = EquipmentSlot::None;
		}
		else if (s ==  "EquipmentSlot_Head"_fstring)
		{
			value = EquipmentSlot::Head;
		}
		else if (s ==  "EquipmentSlot_Neck"_fstring)
		{
			value = EquipmentSlot::Neck;
		}
		else if (s ==  "EquipmentSlot_Body"_fstring)
		{
			value = EquipmentSlot::Body;
		}
		else if (s ==  "EquipmentSlot_Waist"_fstring)
		{
			value = EquipmentSlot::Waist;
		}
		else if (s ==  "EquipmentSlot_Arm"_fstring)
		{
			value = EquipmentSlot::Arm;
		}
		else if (s ==  "EquipmentSlot_Hand"_fstring)
		{
			value = EquipmentSlot::Hand;
		}
		else if (s ==  "EquipmentSlot_Finger"_fstring)
		{
			value = EquipmentSlot::Finger;
		}
		else if (s ==  "EquipmentSlot_Shoulder"_fstring)
		{
			value = EquipmentSlot::Shoulder;
		}
		else if (s ==  "EquipmentSlot_Leg"_fstring)
		{
			value = EquipmentSlot::Leg;
		}
		else if (s ==  "EquipmentSlot_Feet"_fstring)
		{
			value = EquipmentSlot::Feet;
		}
		else if (s ==  "EquipmentSlot_Outerwear"_fstring)
		{
			value = EquipmentSlot::Outerwear;
		}
		else if (s ==  "EquipmentSlot_Underwear"_fstring)
		{
			value = EquipmentSlot::Underwear;
		}
		else if (s ==  "EquipmentSlot_Jewelery"_fstring)
		{
			value = EquipmentSlot::Jewelery;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, EquipmentSlot& value)
	{
		if (s ==  "None"_fstring)
		{
			value = EquipmentSlot::None;
		}
		else if (s ==  "Head"_fstring)
		{
			value = EquipmentSlot::Head;
		}
		else if (s ==  "Neck"_fstring)
		{
			value = EquipmentSlot::Neck;
		}
		else if (s ==  "Body"_fstring)
		{
			value = EquipmentSlot::Body;
		}
		else if (s ==  "Waist"_fstring)
		{
			value = EquipmentSlot::Waist;
		}
		else if (s ==  "Arm"_fstring)
		{
			value = EquipmentSlot::Arm;
		}
		else if (s ==  "Hand"_fstring)
		{
			value = EquipmentSlot::Hand;
		}
		else if (s ==  "Finger"_fstring)
		{
			value = EquipmentSlot::Finger;
		}
		else if (s ==  "Shoulder"_fstring)
		{
			value = EquipmentSlot::Shoulder;
		}
		else if (s ==  "Leg"_fstring)
		{
			value = EquipmentSlot::Leg;
		}
		else if (s ==  "Feet"_fstring)
		{
			value = EquipmentSlot::Feet;
		}
		else if (s ==  "Outerwear"_fstring)
		{
			value = EquipmentSlot::Outerwear;
		}
		else if (s ==  "Underwear"_fstring)
		{
			value = EquipmentSlot::Underwear;
		}
		else if (s ==  "Jewelery"_fstring)
		{
			value = EquipmentSlot::Jewelery;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(EquipmentSlot value)
	{
		switch(value)
		{
			case EquipmentSlot::None:
				return "None"_fstring;
			case EquipmentSlot::Head:
				return "Head"_fstring;
			case EquipmentSlot::Neck:
				return "Neck"_fstring;
			case EquipmentSlot::Body:
				return "Body"_fstring;
			case EquipmentSlot::Waist:
				return "Waist"_fstring;
			case EquipmentSlot::Arm:
				return "Arm"_fstring;
			case EquipmentSlot::Hand:
				return "Hand"_fstring;
			case EquipmentSlot::Finger:
				return "Finger"_fstring;
			case EquipmentSlot::Shoulder:
				return "Shoulder"_fstring;
			case EquipmentSlot::Leg:
				return "Leg"_fstring;
			case EquipmentSlot::Feet:
				return "Feet"_fstring;
			case EquipmentSlot::Outerwear:
				return "Outerwear"_fstring;
			case EquipmentSlot::Underwear:
				return "Underwear"_fstring;
			case EquipmentSlot::Jewelery:
				return "Jewelery"_fstring;
			default:
				return {"",0};
		}
	}
}// HV.EquipmentSlot

namespace HV
{
	bool TryParse(const Rococo::fstring& s, AddItemFlags& value)
	{
		if (s ==  "AddItemFlags_None"_fstring)
		{
			value = AddItemFlags::None;
		}
		else if (s ==  "AddItemFlags_AlignEdge"_fstring)
		{
			value = AddItemFlags::AlignEdge;
		}
		else if (s ==  "AddItemFlags_RandomHeading"_fstring)
		{
			value = AddItemFlags::RandomHeading;
		}
		else if (s ==  "AddItemFlags_RandomPosition"_fstring)
		{
			value = AddItemFlags::RandomPosition;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, AddItemFlags& value)
	{
		if (s ==  "None"_fstring)
		{
			value = AddItemFlags::None;
		}
		else if (s ==  "AlignEdge"_fstring)
		{
			value = AddItemFlags::AlignEdge;
		}
		else if (s ==  "RandomHeading"_fstring)
		{
			value = AddItemFlags::RandomHeading;
		}
		else if (s ==  "RandomPosition"_fstring)
		{
			value = AddItemFlags::RandomPosition;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(AddItemFlags value)
	{
		switch(value)
		{
			case AddItemFlags::None:
				return "None"_fstring;
			case AddItemFlags::AlignEdge:
				return "AlignEdge"_fstring;
			case AddItemFlags::RandomHeading:
				return "RandomHeading"_fstring;
			case AddItemFlags::RandomPosition:
				return "RandomPosition"_fstring;
			default:
				return {"",0};
		}
	}
}// HV.AddItemFlags

// BennyHill generated Sexy native functions for HV::IObjectPrototypeBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVIObjectPrototypeBuilderAddDynamics(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ObjectDynamics* dynamics;
		_offset += sizeof(dynamics);
		ReadInput(dynamics, _sf, -_offset);

		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddDynamics(*dynamics);
	}
	void NativeHVIObjectPrototypeBuilderAddMeleeData(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::MeleeData* melee;
		_offset += sizeof(melee);
		ReadInput(melee, _sf, -_offset);

		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddMeleeData(*melee);
	}
	void NativeHVIObjectPrototypeBuilderAddArmourData(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ArmourData* armour;
		_offset += sizeof(armour);
		ReadInput(armour, _sf, -_offset);

		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddArmourData(*armour);
	}
	void NativeHVIObjectPrototypeBuilderAddSlot(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::EquipmentSlot slot;
		_offset += sizeof(slot);
		ReadInput(slot, _sf, -_offset);

		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddSlot(slot);
	}
	void NativeHVIObjectPrototypeBuilderAddDesc(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _desc;
		ReadInput(_desc, _sf, -_offset);
		fstring desc { _desc->buffer, _desc->length };


		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddDesc(desc);
	}
	void NativeHVIObjectPrototypeBuilderAddShortName(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _shortname;
		ReadInput(_shortname, _sf, -_offset);
		fstring shortname { _shortname->buffer, _shortname->length };


		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddShortName(shortname);
	}
	void NativeHVIObjectPrototypeBuilderAddMaterial(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::MaterialData* material;
		_offset += sizeof(material);
		ReadInput(material, _sf, -_offset);

		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddMaterial(*material);
	}
	void NativeHVIObjectPrototypeBuilderAddIcon(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddIcon(pingPath);
	}
	void NativeHVIObjectPrototypeBuilderCloneFrom(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _source;
		ReadInput(_source, _sf, -_offset);
		fstring source { _source->buffer, _source->length };


		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CloneFrom(source);
	}
	void NativeHVIObjectPrototypeBuilderRemoveSlot(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::EquipmentSlot slot;
		_offset += sizeof(slot);
		ReadInput(slot, _sf, -_offset);

		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->RemoveSlot(slot);
	}
	void NativeHVIObjectPrototypeBuilderHasSlot(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::EquipmentSlot slot;
		_offset += sizeof(slot);
		ReadInput(slot, _sf, -_offset);

		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 hasSlot = _pObject->HasSlot(slot);
		_offset += sizeof(hasSlot);
		WriteOutput(hasSlot, _sf, -_offset);
	}
	void NativeHVIObjectPrototypeBuilderMakeStackable(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 maxStackSize;
		_offset += sizeof(maxStackSize);
		ReadInput(maxStackSize, _sf, -_offset);

		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MakeStackable(maxStackSize);
	}
	void NativeHVIObjectPrototypeBuilderClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeHVIObjectPrototypeBuilderCommit(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _uniqueName;
		ReadInput(_uniqueName, _sf, -_offset);
		fstring uniqueName { _uniqueName->buffer, _uniqueName->length };


		HV::IObjectPrototypeBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Commit(uniqueName);
	}

	void NativeGetHandleForHVObjectPrototypeBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IObjectPrototypeBuilder* nceContext = reinterpret_cast<HV::IObjectPrototypeBuilder*>(_nce.context);
		// Uses: HV::IObjectPrototypeBuilder* FactoryConstructHVObjectPrototypeBuilder(HV::IObjectPrototypeBuilder* _context);
		HV::IObjectPrototypeBuilder* pObject = FactoryConstructHVObjectPrototypeBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV
{
	void AddNativeCalls_HVIObjectPrototypeBuilder(Rococo::Script::IPublicScriptSystem& ss, HV::IObjectPrototypeBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeGetHandleForHVObjectPrototypeBuilder, _nceContext, ("GetHandleForIObjectPrototypeBuilder0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderAddDynamics, nullptr, ("IObjectPrototypeBuilderAddDynamics (Pointer hObject)(HV.ObjectDynamics dynamics) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderAddMeleeData, nullptr, ("IObjectPrototypeBuilderAddMeleeData (Pointer hObject)(HV.MeleeData melee) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderAddArmourData, nullptr, ("IObjectPrototypeBuilderAddArmourData (Pointer hObject)(HV.ArmourData armour) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderAddSlot, nullptr, ("IObjectPrototypeBuilderAddSlot (Pointer hObject)(Int32 slot) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderAddDesc, nullptr, ("IObjectPrototypeBuilderAddDesc (Pointer hObject)(Sys.Type.IString desc) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderAddShortName, nullptr, ("IObjectPrototypeBuilderAddShortName (Pointer hObject)(Sys.Type.IString shortname) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderAddMaterial, nullptr, ("IObjectPrototypeBuilderAddMaterial (Pointer hObject)(HV.MaterialData material) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderAddIcon, nullptr, ("IObjectPrototypeBuilderAddIcon (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderCloneFrom, nullptr, ("IObjectPrototypeBuilderCloneFrom (Pointer hObject)(Sys.Type.IString source) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderRemoveSlot, nullptr, ("IObjectPrototypeBuilderRemoveSlot (Pointer hObject)(Int32 slot) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderHasSlot, nullptr, ("IObjectPrototypeBuilderHasSlot (Pointer hObject)(Int32 slot) -> (Bool hasSlot)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderMakeStackable, nullptr, ("IObjectPrototypeBuilderMakeStackable (Pointer hObject)(Int32 maxStackSize) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderClear, nullptr, ("IObjectPrototypeBuilderClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBuilderCommit, nullptr, ("IObjectPrototypeBuilderCommit (Pointer hObject)(Sys.Type.IString uniqueName) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::IObjectPrototypeBase 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVIObjectPrototypeBaseAppendName(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** sb;
		ReadInput(sb, _sf, -_offset);
		Rococo::Helpers::StringPopulator _sbPopulator(_nce, sb);
		HV::IObjectPrototypeBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AppendName(_sbPopulator);
	}

}

namespace HV
{
	void AddNativeCalls_HVIObjectPrototypeBase(Rococo::Script::IPublicScriptSystem& ss, HV::IObjectPrototypeBase* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeHVIObjectPrototypeBaseAppendName, nullptr, ("IObjectPrototypeBaseAppendName (Pointer hObject)(Sys.Type.IStringBuilder sb) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::IPlayer 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVIPlayerSetPlayerEntity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::IPlayer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPlayerEntity(id);
	}
	void NativeHVIPlayerGetPlayerEntity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IPlayer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY id = _pObject->GetPlayerEntity();
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeHVIPlayerGetInventory(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IPlayer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IInventoryArray* inventory = _pObject->GetInventory();
		_offset += sizeof(CReflectedClass*);
		auto& _inventoryStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IInventoryArray"), ("ProxyIInventoryArray"), _nce.ss);
		CReflectedClass* _sxyinventory = _nce.ss.Represent(_inventoryStruct, inventory);
		WriteOutput(&_sxyinventory->header.pVTables[0], _sf, -_offset);
	}

	void NativeGetHandleForHVPlayer(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		HV::IPlayerSupervisor* nceContext = reinterpret_cast<HV::IPlayerSupervisor*>(_nce.context);
		// Uses: HV::IPlayer* FactoryConstructHVPlayer(HV::IPlayerSupervisor* _context, int32 _index);
		HV::IPlayer* pObject = FactoryConstructHVPlayer(nceContext, index);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV
{
	void AddNativeCalls_HVIPlayer(Rococo::Script::IPublicScriptSystem& ss, HV::IPlayerSupervisor* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeGetHandleForHVPlayer, _nceContext, ("GetHandleForIPlayer0 (Int32 index) -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIPlayerSetPlayerEntity, nullptr, ("IPlayerSetPlayerEntity (Pointer hObject)(Int64 id) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIPlayerGetPlayerEntity, nullptr, ("IPlayerGetPlayerEntity (Pointer hObject) -> (Int64 id)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIPlayerGetInventory, nullptr, ("IPlayerGetInventory (Pointer hObject) -> (Rococo.IInventoryArray inventory)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::IScriptConfig 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVIScriptConfigGetFloat(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxValue;
		_offset += sizeof(maxValue);
		ReadInput(maxValue, _sf, -_offset);

		float minValue;
		_offset += sizeof(minValue);
		ReadInput(minValue, _sf, -_offset);

		float default;
		_offset += sizeof(default);
		ReadInput(default, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _variableName;
		ReadInput(_variableName, _sf, -_offset);
		fstring variableName { _variableName->buffer, _variableName->length };


		HV::IScriptConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		float value = _pObject->GetFloat(variableName, default, minValue, maxValue);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeHVIScriptConfigGetFloatRange(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxValue;
		_offset += sizeof(maxValue);
		ReadInput(maxValue, _sf, -_offset);

		float minValue;
		_offset += sizeof(minValue);
		ReadInput(minValue, _sf, -_offset);

		float defaultRight;
		_offset += sizeof(defaultRight);
		ReadInput(defaultRight, _sf, -_offset);

		float defaultLeft;
		_offset += sizeof(defaultLeft);
		ReadInput(defaultLeft, _sf, -_offset);

		Vec2* values;
		_offset += sizeof(values);
		ReadInput(values, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _variableName;
		ReadInput(_variableName, _sf, -_offset);
		fstring variableName { _variableName->buffer, _variableName->length };


		HV::IScriptConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetFloatRange(variableName, *values, defaultLeft, defaultRight, minValue, maxValue);
	}

	void NativeGetHandleForHVScriptConfig(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IScriptConfig* nceContext = reinterpret_cast<HV::IScriptConfig*>(_nce.context);
		// Uses: HV::IScriptConfig* FactoryConstructHVScriptConfig(HV::IScriptConfig* _context);
		HV::IScriptConfig* pObject = FactoryConstructHVScriptConfig(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV
{
	void AddNativeCalls_HVIScriptConfig(Rococo::Script::IPublicScriptSystem& ss, HV::IScriptConfig* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeGetHandleForHVScriptConfig, _nceContext, ("GetHandleForIScriptConfig0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIScriptConfigGetFloat, nullptr, ("IScriptConfigGetFloat (Pointer hObject)(Sys.Type.IString variableName)(Float32 default)(Float32 minValue)(Float32 maxValue) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVIScriptConfigGetFloatRange, nullptr, ("IScriptConfigGetFloatRange (Pointer hObject)(Sys.Type.IString variableName)(Sys.Maths.Vec2 values)(Float32 defaultLeft)(Float32 defaultRight)(Float32 minValue)(Float32 maxValue) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::ICorridor 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVICorridorGetSpan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		HV::ICorridor* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSpan(*span);
	}
	void NativeHVICorridorIsSloped(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ICorridor* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isSloped = _pObject->IsSloped();
		_offset += sizeof(isSloped);
		WriteOutput(isSloped, _sf, -_offset);
	}

	void NativeGetHandleForHVCorridor(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ICorridor* nceContext = reinterpret_cast<HV::ICorridor*>(_nce.context);
		// Uses: HV::ICorridor* FactoryConstructHVCorridor(HV::ICorridor* _context);
		HV::ICorridor* pObject = FactoryConstructHVCorridor(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV
{
	void AddNativeCalls_HVICorridor(Rococo::Script::IPublicScriptSystem& ss, HV::ICorridor* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeGetHandleForHVCorridor, _nceContext, ("GetHandleForICorridor0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVICorridorGetSpan, nullptr, ("ICorridorGetSpan (Pointer hObject)(Sys.Maths.Vec3 span) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVICorridorIsSloped, nullptr, ("ICorridorIsSloped (Pointer hObject) -> (Bool isSloped)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::ISectorAIBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorAIBuilderClearTriggers(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorAIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearTriggers();
	}
	void NativeHVISectorAIBuilderAddTrigger(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		HV::ISectorAIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTrigger(name);
	}
	void NativeHVISectorAIBuilderAddAction(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _factoryName;
		ReadInput(_factoryName, _sf, -_offset);
		fstring factoryName { _factoryName->buffer, _factoryName->length };


		HV::ISectorAIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddAction(factoryName);
	}
	void NativeHVISectorAIBuilderAddActionArgumentI32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _argName;
		ReadInput(_argName, _sf, -_offset);
		fstring argName { _argName->buffer, _argName->length };


		HV::ISectorAIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddActionArgumentI32(argName, value);
	}
	void NativeHVISectorAIBuilderAddActionArgumentF32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _argName;
		ReadInput(_argName, _sf, -_offset);
		fstring argName { _argName->buffer, _argName->length };


		HV::ISectorAIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddActionArgumentF32(argName, value);
	}
	void NativeHVISectorAIBuilderAddActionArgumentString(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _value;
		ReadInput(_value, _sf, -_offset);
		fstring value { _value->buffer, _value->length };


		_offset += sizeof(IString*);
		IString* _argName;
		ReadInput(_argName, _sf, -_offset);
		fstring argName { _argName->buffer, _argName->length };


		HV::ISectorAIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddActionArgumentString(argName, value);
	}
	void NativeHVISectorAIBuilderAddTag(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		HV::ISectorAIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTag(text);
	}

	void NativeGetHandleForHVSectorAIBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 sectorId;
		_offset += sizeof(sectorId);
		ReadInput(sectorId, _sf, -_offset);

		Cosmos* nceContext = reinterpret_cast<Cosmos*>(_nce.context);
		// Uses: HV::ISectorAIBuilder* FactoryConstructHVSectorAIBuilder(Cosmos* _context, int32 _sectorId);
		HV::ISectorAIBuilder* pObject = FactoryConstructHVSectorAIBuilder(nceContext, sectorId);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV
{
	void AddNativeCalls_HVISectorAIBuilder(Rococo::Script::IPublicScriptSystem& ss, Cosmos* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorAIBuilder, _nceContext, ("GetHandleForISectorAIBuilder0 (Int32 sectorId) -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorAIBuilderClearTriggers, nullptr, ("ISectorAIBuilderClearTriggers (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorAIBuilderAddTrigger, nullptr, ("ISectorAIBuilderAddTrigger (Pointer hObject)(Sys.Type.IString name) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorAIBuilderAddAction, nullptr, ("ISectorAIBuilderAddAction (Pointer hObject)(Sys.Type.IString factoryName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorAIBuilderAddActionArgumentI32, nullptr, ("ISectorAIBuilderAddActionArgumentI32 (Pointer hObject)(Sys.Type.IString argName)(Int32 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorAIBuilderAddActionArgumentF32, nullptr, ("ISectorAIBuilderAddActionArgumentF32 (Pointer hObject)(Sys.Type.IString argName)(Float32 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorAIBuilderAddActionArgumentString, nullptr, ("ISectorAIBuilderAddActionArgumentString (Pointer hObject)(Sys.Type.IString argName)(Sys.Type.IString value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorAIBuilderAddTag, nullptr, ("ISectorAIBuilderAddTag (Pointer hObject)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::ISectorBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorBuilderAddVertex(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float y;
		_offset += sizeof(y);
		ReadInput(y, _sf, -_offset);

		float x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddVertex(x, y);
	}
	void NativeHVISectorBuilderClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeHVISectorBuilderDisableMeshGeneration(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DisableMeshGeneration();
	}
	void NativeHVISectorBuilderEnableMeshGeneration(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->EnableMeshGeneration();
	}
	void NativeHVISectorBuilderGenerateMeshes(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GenerateMeshes();
	}
	void NativeHVISectorBuilderCreateFromTemplate(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 height;
		_offset += sizeof(height);
		ReadInput(height, _sf, -_offset);

		int32 altitude;
		_offset += sizeof(altitude);
		ReadInput(altitude, _sf, -_offset);

		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 id = _pObject->CreateFromTemplate(altitude, height);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeHVISectorBuilderSetTemplateWallScript(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _scriptName;
		ReadInput(_scriptName, _sf, -_offset);
		fstring scriptName { _scriptName->buffer, _scriptName->length };


		boolean32 useScript;
		_offset += sizeof(useScript);
		ReadInput(useScript, _sf, -_offset);

		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetTemplateWallScript(useScript, scriptName);
	}
	void NativeHVISectorBuilderSetTemplateDoorScript(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _scriptName;
		ReadInput(_scriptName, _sf, -_offset);
		fstring scriptName { _scriptName->buffer, _scriptName->length };


		boolean32 hasDoor;
		_offset += sizeof(hasDoor);
		ReadInput(hasDoor, _sf, -_offset);

		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetTemplateDoorScript(hasDoor, scriptName);
	}
	void NativeHVISectorBuilderSetTemplateFloorScript(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _scriptName;
		ReadInput(_scriptName, _sf, -_offset);
		fstring scriptName { _scriptName->buffer, _scriptName->length };


		boolean32 useScript;
		_offset += sizeof(useScript);
		ReadInput(useScript, _sf, -_offset);

		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetTemplateFloorScript(useScript, scriptName);
	}
	void NativeHVISectorBuilderSetTemplateMaterial(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _persistentId;
		ReadInput(_persistentId, _sf, -_offset);
		fstring persistentId { _persistentId->buffer, _persistentId->length };


		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		Rococo::Graphics::MaterialCategory cat;
		_offset += sizeof(cat);
		ReadInput(cat, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _bodyClass;
		ReadInput(_bodyClass, _sf, -_offset);
		fstring bodyClass { _bodyClass->buffer, _bodyClass->length };


		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetTemplateMaterial(bodyClass, cat, colour, persistentId);
	}
	void NativeHVISectorBuilderSetWallScriptF32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetWallScriptF32(name, value);
	}
	void NativeHVISectorBuilderSetFloorScriptF32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetFloorScriptF32(name, value);
	}
	void NativeHVISectorBuilderSetCorridorScriptF32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCorridorScriptF32(name, value);
	}

	void NativeGetHandleForHVSectorBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectors* nceContext = reinterpret_cast<HV::ISectors*>(_nce.context);
		// Uses: HV::ISectorBuilder* FactoryConstructHVSectorBuilder(HV::ISectors* _context);
		HV::ISectorBuilder* pObject = FactoryConstructHVSectorBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV
{
	void AddNativeCalls_HVISectorBuilder(Rococo::Script::IPublicScriptSystem& ss, HV::ISectors* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorBuilder, _nceContext, ("GetHandleForISectors0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderAddVertex, nullptr, ("ISectorsAddVertex (Pointer hObject)(Float32 x)(Float32 y) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderClear, nullptr, ("ISectorsClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderDisableMeshGeneration, nullptr, ("ISectorsDisableMeshGeneration (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderEnableMeshGeneration, nullptr, ("ISectorsEnableMeshGeneration (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderGenerateMeshes, nullptr, ("ISectorsGenerateMeshes (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderCreateFromTemplate, nullptr, ("ISectorsCreateFromTemplate (Pointer hObject)(Int32 altitude)(Int32 height) -> (Int32 id)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetTemplateWallScript, nullptr, ("ISectorsSetTemplateWallScript (Pointer hObject)(Bool useScript)(Sys.Type.IString scriptName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetTemplateDoorScript, nullptr, ("ISectorsSetTemplateDoorScript (Pointer hObject)(Bool hasDoor)(Sys.Type.IString scriptName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetTemplateFloorScript, nullptr, ("ISectorsSetTemplateFloorScript (Pointer hObject)(Bool useScript)(Sys.Type.IString scriptName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetTemplateMaterial, nullptr, ("ISectorsSetTemplateMaterial (Pointer hObject)(Sys.Type.IString bodyClass)(Int32 cat)(Int32 colour)(Sys.Type.IString persistentId) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetWallScriptF32, nullptr, ("ISectorsSetWallScriptF32 (Pointer hObject)(Sys.Type.IString name)(Float32 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetFloorScriptF32, nullptr, ("ISectorsSetFloorScriptF32 (Pointer hObject)(Sys.Type.IString name)(Float32 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetCorridorScriptF32, nullptr, ("ISectorsSetCorridorScriptF32 (Pointer hObject)(Sys.Type.IString name)(Float32 value) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::ISectorLayout 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorLayoutExists(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 exists = _pObject->Exists();
		_offset += sizeof(exists);
		WriteOutput(exists, _sf, -_offset);
	}
	void NativeHVISectorLayoutCountSquares(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 sqCount = _pObject->CountSquares();
		_offset += sizeof(sqCount);
		WriteOutput(sqCount, _sf, -_offset);
	}
	void NativeHVISectorLayoutGetSquare(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		AABB2d* sq;
		_offset += sizeof(sq);
		ReadInput(sq, _sf, -_offset);

		int32 sqIndex;
		_offset += sizeof(sqIndex);
		ReadInput(sqIndex, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSquare(sqIndex, *sq);
	}
	void NativeHVISectorLayoutCeilingQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 sqIndex;
		_offset += sizeof(sqIndex);
		ReadInput(sqIndex, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CeilingQuad(sqIndex, *q);
	}
	void NativeHVISectorLayoutFloorQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 sqIndex;
		_offset += sizeof(sqIndex);
		ReadInput(sqIndex, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->FloorQuad(sqIndex, *q);
	}
	void NativeHVISectorLayoutTryGetAsRectangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRectf* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 success = _pObject->TryGetAsRectangle(*rect);
		_offset += sizeof(success);
		WriteOutput(success, _sf, -_offset);
	}
	void NativeHVISectorLayoutAltitude(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* altitudes;
		_offset += sizeof(altitudes);
		ReadInput(altitudes, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Altitude(*altitudes);
	}
	void NativeHVISectorLayoutNumberOfSegments(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 segCount = _pObject->NumberOfSegments();
		_offset += sizeof(segCount);
		WriteOutput(segCount, _sf, -_offset);
	}
	void NativeHVISectorLayoutNumberOfGaps(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 gapCount = _pObject->NumberOfGaps();
		_offset += sizeof(gapCount);
		WriteOutput(gapCount, _sf, -_offset);
	}
	void NativeHVISectorLayoutGetSegment(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::WallSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 segIndex;
		_offset += sizeof(segIndex);
		ReadInput(segIndex, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSegment(segIndex, *segment);
	}
	void NativeHVISectorLayoutGetGap(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::GapSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 gapIndex;
		_offset += sizeof(gapIndex);
		ReadInput(gapIndex, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetGap(gapIndex, *segment);
	}
	void NativeHVISectorLayoutAddSceneryAroundObject(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ObjectCreationSpec* ocs;
		_offset += sizeof(ocs);
		ReadInput(ocs, _sf, -_offset);

		HV::InsertItemSpec* iis;
		_offset += sizeof(iis);
		ReadInput(iis, _sf, -_offset);

		ID_ENTITY centrePieceId;
		_offset += sizeof(centrePieceId);
		ReadInput(centrePieceId, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _mesh;
		ReadInput(_mesh, _sf, -_offset);
		fstring mesh { _mesh->buffer, _mesh->length };


		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY id = _pObject->AddSceneryAroundObject(mesh, centrePieceId, *iis, *ocs);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeHVISectorLayoutAddItemToLargestSquare(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ObjectCreationSpec* ocs;
		_offset += sizeof(ocs);
		ReadInput(ocs, _sf, -_offset);

		int32 addItemFlags;
		_offset += sizeof(addItemFlags);
		ReadInput(addItemFlags, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _mesh;
		ReadInput(_mesh, _sf, -_offset);
		fstring mesh { _mesh->buffer, _mesh->length };


		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY id = _pObject->AddItemToLargestSquare(mesh, addItemFlags, *ocs);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeHVISectorLayoutPlaceItemOnUpFacingQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 wasMoved = _pObject->PlaceItemOnUpFacingQuad(id);
		_offset += sizeof(wasMoved);
		WriteOutput(wasMoved, _sf, -_offset);
	}
	void NativeHVISectorLayoutDeleteScenery(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DeleteScenery();
	}
	void NativeHVISectorLayoutDeleteItemsWithMesh(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _prefix;
		ReadInput(_prefix, _sf, -_offset);
		fstring prefix { _prefix->buffer, _prefix->length };


		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DeleteItemsWithMesh(prefix);
	}
	void NativeHVISectorLayoutClearManagedEntities(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearManagedEntities();
	}
	void NativeHVISectorLayoutManageEntity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ManageEntity(id);
	}
	void NativeHVISectorLayoutUseUpFacingQuads(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->UseUpFacingQuads(id);
	}

}

namespace HV
{
	void AddNativeCalls_HVISectorLayout(Rococo::Script::IPublicScriptSystem& ss, HV::ISectorLayout* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeHVISectorLayoutExists, nullptr, ("ISectorLayoutExists (Pointer hObject) -> (Bool exists)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutCountSquares, nullptr, ("ISectorLayoutCountSquares (Pointer hObject) -> (Int32 sqCount)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutGetSquare, nullptr, ("ISectorLayoutGetSquare (Pointer hObject)(Int32 sqIndex)(Rococo.AAB2d sq) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutCeilingQuad, nullptr, ("ISectorLayoutCeilingQuad (Pointer hObject)(Int32 sqIndex)(Rococo.QuadVertices q) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutFloorQuad, nullptr, ("ISectorLayoutFloorQuad (Pointer hObject)(Int32 sqIndex)(Rococo.QuadVertices q) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutTryGetAsRectangle, nullptr, ("ISectorLayoutTryGetAsRectangle (Pointer hObject)(Sys.Maths.Rectf rect) -> (Bool success)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutAltitude, nullptr, ("ISectorLayoutAltitude (Pointer hObject)(Sys.Maths.Vec2 altitudes) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutNumberOfSegments, nullptr, ("ISectorLayoutNumberOfSegments (Pointer hObject) -> (Int32 segCount)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutNumberOfGaps, nullptr, ("ISectorLayoutNumberOfGaps (Pointer hObject) -> (Int32 gapCount)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutGetSegment, nullptr, ("ISectorLayoutGetSegment (Pointer hObject)(Int32 segIndex)(HV.WallSegment segment) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutGetGap, nullptr, ("ISectorLayoutGetGap (Pointer hObject)(Int32 gapIndex)(HV.GapSegment segment) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutAddSceneryAroundObject, nullptr, ("ISectorLayoutAddSceneryAroundObject (Pointer hObject)(Sys.Type.IString mesh)(Int64 centrePieceId)(HV.InsertItemSpec iis)(HV.ObjectCreationSpec ocs) -> (Int64 id)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutAddItemToLargestSquare, nullptr, ("ISectorLayoutAddItemToLargestSquare (Pointer hObject)(Sys.Type.IString mesh)(Int32 addItemFlags)(HV.ObjectCreationSpec ocs) -> (Int64 id)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutPlaceItemOnUpFacingQuad, nullptr, ("ISectorLayoutPlaceItemOnUpFacingQuad (Pointer hObject)(Int64 id) -> (Bool wasMoved)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutDeleteScenery, nullptr, ("ISectorLayoutDeleteScenery (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutDeleteItemsWithMesh, nullptr, ("ISectorLayoutDeleteItemsWithMesh (Pointer hObject)(Sys.Type.IString prefix) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutClearManagedEntities, nullptr, ("ISectorLayoutClearManagedEntities (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutManageEntity, nullptr, ("ISectorLayoutManageEntity (Pointer hObject)(Int64 id) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorLayoutUseUpFacingQuads, nullptr, ("ISectorLayoutUseUpFacingQuads (Pointer hObject)(Int64 id) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::ISectorEnumerator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorEnumeratorCount(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorEnumerator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 nCount = _pObject->Count();
		_offset += sizeof(nCount);
		WriteOutput(nCount, _sf, -_offset);
	}
	void NativeHVISectorEnumeratorGetSector(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		HV::ISectorEnumerator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		HV::ISectorLayout* layout = _pObject->GetSector(index);
		_offset += sizeof(CReflectedClass*);
		auto& _layoutStruct = Rococo::Helpers::GetDefaultProxy(("HV"),("ISectorLayout"), ("ProxyISectorLayout"), _nce.ss);
		CReflectedClass* _sxylayout = _nce.ss.Represent(_layoutStruct, layout);
		WriteOutput(&_sxylayout->header.pVTables[0], _sf, -_offset);
	}
	void NativeHVISectorEnumeratorGetSectorById(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::ISectorEnumerator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		HV::ISectorLayout* layout = _pObject->GetSectorById(id);
		_offset += sizeof(CReflectedClass*);
		auto& _layoutStruct = Rococo::Helpers::GetDefaultProxy(("HV"),("ISectorLayout"), ("ProxyISectorLayout"), _nce.ss);
		CReflectedClass* _sxylayout = _nce.ss.Represent(_layoutStruct, layout);
		WriteOutput(&_sxylayout->header.pVTables[0], _sf, -_offset);
	}
	void NativeHVISectorEnumeratorGetSelectedSector(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorEnumerator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		HV::ISectorLayout* layout = _pObject->GetSelectedSector();
		_offset += sizeof(CReflectedClass*);
		auto& _layoutStruct = Rococo::Helpers::GetDefaultProxy(("HV"),("ISectorLayout"), ("ProxyISectorLayout"), _nce.ss);
		CReflectedClass* _sxylayout = _nce.ss.Represent(_layoutStruct, layout);
		WriteOutput(&_sxylayout->header.pVTables[0], _sf, -_offset);
	}

	void NativeGetHandleForHVSectorEnumerator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorEnumerator* nceContext = reinterpret_cast<HV::ISectorEnumerator*>(_nce.context);
		// Uses: HV::ISectorEnumerator* FactoryConstructHVSectorEnumerator(HV::ISectorEnumerator* _context);
		HV::ISectorEnumerator* pObject = FactoryConstructHVSectorEnumerator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV
{
	void AddNativeCalls_HVISectorEnumerator(Rococo::Script::IPublicScriptSystem& ss, HV::ISectorEnumerator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorEnumerator, _nceContext, ("GetHandleForISectorEnumerator0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorEnumeratorCount, nullptr, ("ISectorEnumeratorCount (Pointer hObject) -> (Int32 nCount)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorEnumeratorGetSector, nullptr, ("ISectorEnumeratorGetSector (Pointer hObject)(Int32 index) -> (HV.ISectorLayout layout)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorEnumeratorGetSectorById, nullptr, ("ISectorEnumeratorGetSectorById (Pointer hObject)(Int32 id) -> (HV.ISectorLayout layout)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorEnumeratorGetSelectedSector, nullptr, ("ISectorEnumeratorGetSelectedSector (Pointer hObject) -> (HV.ISectorLayout layout)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::ISectorComponents 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorComponentsAddTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		VertexTriangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		HV::ISectorComponents* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTriangle(*t);
	}
	void NativeHVISectorComponentsAddPhysicsHull(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Triangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		HV::ISectorComponents* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddPhysicsHull(*t);
	}
	void NativeHVISectorComponentsBuildComponent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _componentName;
		ReadInput(_componentName, _sf, -_offset);
		fstring componentName { _componentName->buffer, _componentName->length };


		HV::ISectorComponents* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->BuildComponent(componentName);
	}
	void NativeHVISectorComponentsClearComponents(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _componentName;
		ReadInput(_componentName, _sf, -_offset);
		fstring componentName { _componentName->buffer, _componentName->length };


		HV::ISectorComponents* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearComponents(componentName);
	}
	void NativeHVISectorComponentsCompleteComponent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveMesh;
		_offset += sizeof(preserveMesh);
		ReadInput(preserveMesh, _sf, -_offset);

		HV::ISectorComponents* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CompleteComponent(preserveMesh);
	}
	void NativeHVISectorComponentsGetMaterial(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _componentClass;
		ReadInput(_componentClass, _sf, -_offset);
		fstring componentClass { _componentClass->buffer, _componentClass->length };


		MaterialVertexData* mat;
		_offset += sizeof(mat);
		ReadInput(mat, _sf, -_offset);

		HV::ISectorComponents* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetMaterial(*mat, componentClass);
	}

	void NativeGetHandleForHVSectorComponents(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorComponents* nceContext = reinterpret_cast<HV::ISectorComponents*>(_nce.context);
		// Uses: HV::ISectorComponents* FactoryConstructHVSectorComponents(HV::ISectorComponents* _context);
		HV::ISectorComponents* pObject = FactoryConstructHVSectorComponents(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV
{
	void AddNativeCalls_HVISectorComponents(Rococo::Script::IPublicScriptSystem& ss, HV::ISectorComponents* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorComponents, _nceContext, ("GetHandleForISectorComponents0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorComponentsAddTriangle, nullptr, ("ISectorComponentsAddTriangle (Pointer hObject)(Rococo.VertexTriangle t) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorComponentsAddPhysicsHull, nullptr, ("ISectorComponentsAddPhysicsHull (Pointer hObject)(Sys.Maths.Triangle t) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorComponentsBuildComponent, nullptr, ("ISectorComponentsBuildComponent (Pointer hObject)(Sys.Type.IString componentName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorComponentsClearComponents, nullptr, ("ISectorComponentsClearComponents (Pointer hObject)(Sys.Type.IString componentName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorComponentsCompleteComponent, nullptr, ("ISectorComponentsCompleteComponent (Pointer hObject)(Bool preserveMesh) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorComponentsGetMaterial, nullptr, ("ISectorComponentsGetMaterial (Pointer hObject)(Rococo.MaterialVertexData mat)(Sys.Type.IString componentClass) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::ITriangleList 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVITriangleListAddTriangleByVertices(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ObjectVertex* c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		ObjectVertex* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		ObjectVertex* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		HV::ITriangleList* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTriangleByVertices(*a, *b, *c);
	}
	void NativeHVITriangleListAddTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		VertexTriangle* abc;
		_offset += sizeof(abc);
		ReadInput(abc, _sf, -_offset);

		HV::ITriangleList* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTriangle(*abc);
	}
	void NativeHVITriangleListAddQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ObjectVertex* d;
		_offset += sizeof(d);
		ReadInput(d, _sf, -_offset);

		ObjectVertex* c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		ObjectVertex* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		ObjectVertex* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		HV::ITriangleList* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddQuad(*a, *b, *c, *d);
	}
	void NativeHVITriangleListCountVertices(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ITriangleList* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 vertices = _pObject->CountVertices();
		_offset += sizeof(vertices);
		WriteOutput(vertices, _sf, -_offset);
	}
	void NativeHVITriangleListCountTriangles(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ITriangleList* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 triangles = _pObject->CountTriangles();
		_offset += sizeof(triangles);
		WriteOutput(triangles, _sf, -_offset);
	}

}

namespace HV
{
	void AddNativeCalls_HVITriangleList(Rococo::Script::IPublicScriptSystem& ss, HV::ITriangleList* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeHVITriangleListAddTriangleByVertices, nullptr, ("ITriangleListAddTriangleByVertices (Pointer hObject)(Rococo.ObjectVertex a)(Rococo.ObjectVertex b)(Rococo.ObjectVertex c) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVITriangleListAddTriangle, nullptr, ("ITriangleListAddTriangle (Pointer hObject)(Rococo.VertexTriangle abc) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVITriangleListAddQuad, nullptr, ("ITriangleListAddQuad (Pointer hObject)(Rococo.ObjectVertex a)(Rococo.ObjectVertex b)(Rococo.ObjectVertex c)(Rococo.ObjectVertex d) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVITriangleListCountVertices, nullptr, ("ITriangleListCountVertices (Pointer hObject) -> (Int32 vertices)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVITriangleListCountTriangles, nullptr, ("ITriangleListCountTriangles (Pointer hObject) -> (Int32 triangles)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::ISectorWallTesselator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorWallTesselatorNumberOfSegments(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 count = _pObject->NumberOfSegments();
		_offset += sizeof(count);
		WriteOutput(count, _sf, -_offset);
	}
	void NativeHVISectorWallTesselatorNumberOfGaps(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 count = _pObject->NumberOfGaps();
		_offset += sizeof(count);
		WriteOutput(count, _sf, -_offset);
	}
	void NativeHVISectorWallTesselatorGetSegment(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::WallSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 ringIndex;
		_offset += sizeof(ringIndex);
		ReadInput(ringIndex, _sf, -_offset);

		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSegment(ringIndex, *segment);
	}
	void NativeHVISectorWallTesselatorGetGap(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::GapSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 ringIndex;
		_offset += sizeof(ringIndex);
		ReadInput(ringIndex, _sf, -_offset);

		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetGap(ringIndex, *segment);
	}
	void NativeHVISectorWallTesselatorWallTriangles(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		HV::ITriangleList* triangles = _pObject->WallTriangles();
		_offset += sizeof(CReflectedClass*);
		auto& _trianglesStruct = Rococo::Helpers::GetDefaultProxy(("HV"),("ITriangleList"), ("ProxyITriangleList"), _nce.ss);
		CReflectedClass* _sxytriangles = _nce.ss.Represent(_trianglesStruct, triangles);
		WriteOutput(&_sxytriangles->header.pVTables[0], _sf, -_offset);
	}
	void NativeHVISectorWallTesselatorGetMaterial(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _componentClass;
		ReadInput(_componentClass, _sf, -_offset);
		fstring componentClass { _componentClass->buffer, _componentClass->length };


		MaterialVertexData* mat;
		_offset += sizeof(mat);
		ReadInput(mat, _sf, -_offset);

		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetMaterial(*mat, componentClass);
	}

	void NativeGetHandleForHVSectorWallTesselator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorWallTesselator* nceContext = reinterpret_cast<HV::ISectorWallTesselator*>(_nce.context);
		// Uses: HV::ISectorWallTesselator* FactoryConstructHVSectorWallTesselator(HV::ISectorWallTesselator* _context);
		HV::ISectorWallTesselator* pObject = FactoryConstructHVSectorWallTesselator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV
{
	void AddNativeCalls_HVISectorWallTesselator(Rococo::Script::IPublicScriptSystem& ss, HV::ISectorWallTesselator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorWallTesselator, _nceContext, ("GetHandleForISectorWallTesselator0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorNumberOfSegments, nullptr, ("ISectorWallTesselatorNumberOfSegments (Pointer hObject) -> (Int32 count)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorNumberOfGaps, nullptr, ("ISectorWallTesselatorNumberOfGaps (Pointer hObject) -> (Int32 count)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorGetSegment, nullptr, ("ISectorWallTesselatorGetSegment (Pointer hObject)(Int32 ringIndex)(HV.WallSegment segment) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorGetGap, nullptr, ("ISectorWallTesselatorGetGap (Pointer hObject)(Int32 ringIndex)(HV.GapSegment segment) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorWallTriangles, nullptr, ("ISectorWallTesselatorWallTriangles (Pointer hObject) -> (HV.ITriangleList triangles)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorGetMaterial, nullptr, ("ISectorWallTesselatorGetMaterial (Pointer hObject)(Rococo.MaterialVertexData mat)(Sys.Type.IString componentClass) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for HV::ISectorFloorTesselator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorFloorTesselatorNumberOfSquares(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 count = _pObject->NumberOfSquares();
		_offset += sizeof(count);
		WriteOutput(count, _sf, -_offset);
	}
	void NativeHVISectorFloorTesselatorFoundationsExist(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 exists = _pObject->FoundationsExist();
		_offset += sizeof(exists);
		WriteOutput(exists, _sf, -_offset);
	}
	void NativeHVISectorFloorTesselatorGetSquare(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		AABB2d* sq;
		_offset += sizeof(sq);
		ReadInput(sq, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSquare(index, *sq);
	}
	void NativeHVISectorFloorTesselatorCeilingQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CeilingQuad(index, *q);
	}
	void NativeHVISectorFloorTesselatorFloorQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->FloorQuad(index, *q);
	}
	void NativeHVISectorFloorTesselatorAddCeilingTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		VertexTriangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddCeilingTriangle(*t);
	}
	void NativeHVISectorFloorTesselatorAddFloorTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		VertexTriangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddFloorTriangle(*t);
	}
	void NativeHVISectorFloorTesselatorGetMaterial(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _componentClass;
		ReadInput(_componentClass, _sf, -_offset);
		fstring componentClass { _componentClass->buffer, _componentClass->length };


		MaterialVertexData* mat;
		_offset += sizeof(mat);
		ReadInput(mat, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetMaterial(*mat, componentClass);
	}
	void NativeHVISectorFloorTesselatorSetUVScale(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float scale;
		_offset += sizeof(scale);
		ReadInput(scale, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetUVScale(scale);
	}
	void NativeHVISectorFloorTesselatorNumberOfSegments(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 segCount = _pObject->NumberOfSegments();
		_offset += sizeof(segCount);
		WriteOutput(segCount, _sf, -_offset);
	}
	void NativeHVISectorFloorTesselatorNumberOfGaps(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 gapCount = _pObject->NumberOfGaps();
		_offset += sizeof(gapCount);
		WriteOutput(gapCount, _sf, -_offset);
	}
	void NativeHVISectorFloorTesselatorGetSegment(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::WallSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 segIndex;
		_offset += sizeof(segIndex);
		ReadInput(segIndex, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSegment(segIndex, *segment);
	}
	void NativeHVISectorFloorTesselatorGetGap(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::GapSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 gapIndex;
		_offset += sizeof(gapIndex);
		ReadInput(gapIndex, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetGap(gapIndex, *segment);
	}

	void NativeGetHandleForHVSectorFloorTesselator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorFloorTesselator* nceContext = reinterpret_cast<HV::ISectorFloorTesselator*>(_nce.context);
		// Uses: HV::ISectorFloorTesselator* FactoryConstructHVSectorFloorTesselator(HV::ISectorFloorTesselator* _context);
		HV::ISectorFloorTesselator* pObject = FactoryConstructHVSectorFloorTesselator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV
{
	void AddNativeCalls_HVISectorFloorTesselator(Rococo::Script::IPublicScriptSystem& ss, HV::ISectorFloorTesselator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("HV.Native");
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorFloorTesselator, _nceContext, ("GetHandleForISectorFloorTesselator0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorNumberOfSquares, nullptr, ("ISectorFloorTesselatorNumberOfSquares (Pointer hObject) -> (Int32 count)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorFoundationsExist, nullptr, ("ISectorFloorTesselatorFoundationsExist (Pointer hObject) -> (Bool exists)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorGetSquare, nullptr, ("ISectorFloorTesselatorGetSquare (Pointer hObject)(Int32 index)(Rococo.AAB2d sq) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorCeilingQuad, nullptr, ("ISectorFloorTesselatorCeilingQuad (Pointer hObject)(Int32 index)(Rococo.QuadVertices q) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorFloorQuad, nullptr, ("ISectorFloorTesselatorFloorQuad (Pointer hObject)(Int32 index)(Rococo.QuadVertices q) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorAddCeilingTriangle, nullptr, ("ISectorFloorTesselatorAddCeilingTriangle (Pointer hObject)(Rococo.VertexTriangle t) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorAddFloorTriangle, nullptr, ("ISectorFloorTesselatorAddFloorTriangle (Pointer hObject)(Rococo.VertexTriangle t) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorGetMaterial, nullptr, ("ISectorFloorTesselatorGetMaterial (Pointer hObject)(Rococo.MaterialVertexData mat)(Sys.Type.IString componentClass) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorSetUVScale, nullptr, ("ISectorFloorTesselatorSetUVScale (Pointer hObject)(Float32 scale) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorNumberOfSegments, nullptr, ("ISectorFloorTesselatorNumberOfSegments (Pointer hObject) -> (Int32 segCount)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorNumberOfGaps, nullptr, ("ISectorFloorTesselatorNumberOfGaps (Pointer hObject) -> (Int32 gapCount)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorGetSegment, nullptr, ("ISectorFloorTesselatorGetSegment (Pointer hObject)(Int32 segIndex)(HV.WallSegment segment) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorGetGap, nullptr, ("ISectorFloorTesselatorGetGap (Pointer hObject)(Int32 gapIndex)(HV.GapSegment segment) -> "), __FILE__, __LINE__);
	}
}
