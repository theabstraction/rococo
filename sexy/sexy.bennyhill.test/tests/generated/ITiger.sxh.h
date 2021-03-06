namespace Sys { namespace Animals { 
	enum AnimalType: int32
	{
		AnimalType_Cat = 0, 	// 0x0
		AnimalType_Dog = 1, 	// 0x1
		AnimalType_Tiger = 2, 	// 0x2
	};
	bool TryParse(const Rococo::fstring& s, AnimalType& value);
	bool TryShortParse(const Rococo::fstring& s, AnimalType& value); 
}}

