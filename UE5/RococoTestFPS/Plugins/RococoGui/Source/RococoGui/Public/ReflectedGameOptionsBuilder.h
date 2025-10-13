#pragma once

#include <CoreMinimal.h>

namespace Rococo::GreatSex
{
	struct IGreatSexGenerator;

	struct IReflectedGameOptionsBuilder
	{
		virtual void AddMethods(UObject& object) = 0;
		virtual void Free() = 0;
		virtual void ReflectIntoGenerator(UObject& object, const FString& optionCategory, IGreatSexGenerator& generator) = 0;
	};

	ROCOCO_GUI_API IReflectedGameOptionsBuilder* CreateReflectedGameOptionsBuilder();
}