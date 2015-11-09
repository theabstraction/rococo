namespace Sys::Animals {
	bool TryParse(const fstring& s, AnimalType& value)
	{
		if (s == L"AnimalType_Cat"_fstring)
		{
			value = AnimalType_Cat;
		}
		else if (s == L"AnimalType_Dog"_fstring)
		{
			value = AnimalType_Dog;
		}
		else if (s == L"AnimalType_Tiger"_fstring)
		{
			value = AnimalType_Tiger;
		}
		else
		{
			return false;
		}

		return true;
	}
}

