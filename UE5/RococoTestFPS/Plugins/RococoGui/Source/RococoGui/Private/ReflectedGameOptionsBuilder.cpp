#include <CoreMinimal.h>
#include <CoreUObject.h>

namespace Rococo::GreatSex
{
	struct IGreatSexGenerator;
}

namespace RococoTestFPS
{
	ROCOCOGUI_API void ReflectIntoGenerator(UObject& object, Rococo::GreatSex::IGreatSexGenerator& generator)
	{
		UClass* objectClass = object.GetClass();
	}
}