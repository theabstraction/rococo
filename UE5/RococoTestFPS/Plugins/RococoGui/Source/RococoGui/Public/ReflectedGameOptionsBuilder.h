#pragma once

#include <CoreMinimal.h>
#include <CoreUObject.h>

namespace Rococo::GreatSex
{
	struct IGreatSexGenerator;

	struct IReflectedGameOptionsBuilder
	{
		virtual void AddMethods(UObject& object) = 0;
		virtual void Free() = 0;
		virtual void ReflectIntoGenerator(UObject& object, IGreatSexGenerator& generator) = 0;
	};

	ROCOCOGUI_API IReflectedGameOptionsBuilder* CreateReflectedGameOptionsBuilder();
}