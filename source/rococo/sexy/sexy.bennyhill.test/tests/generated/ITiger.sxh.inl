namespace Sys { namespace Animals { 
	bool TryParse(const Rococo::fstring& s, AnimalType& value)
	{
		if (s ==  "AnimalType_Cat"_fstring)
		{
			value = AnimalType_Cat;
		}
		else if (s ==  "AnimalType_Dog"_fstring)
		{
			value = AnimalType_Dog;
		}
		else if (s ==  "AnimalType_Tiger"_fstring)
		{
			value = AnimalType_Tiger;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, AnimalType& value)
	{
		if (s ==  "Cat"_fstring)
		{
			value = AnimalType_Cat;
		}
		else if (s ==  "Dog"_fstring)
		{
			value = AnimalType_Dog;
		}
		else if (s ==  "Tiger"_fstring)
		{
			value = AnimalType_Tiger;
		}
		else
		{
			return false;
		}

		return true;
	}
}}// Sys.Animals.AnimalType

