#pragma once
#include <rococo.types.h>
#include <sexy.types.h>
#include <sexy.script.h>

class UClass;
class UObject;
class UMethod;
typedef int UnknownType;

namespace Rococo::UE::Native
{
	int64 ConstructUObject(Rococo::Script::NativeCallEnvironment& e);
	UMethod* GetNCEUMethod(Rococo::Script::NativeCallEnvironment& e);
	UMethod& GetMethod(UClass& classRef, crwstr methodName);
	UObject* GetNCEUObject(Rococo::Script::NativeCallEnvironment& e, int64 objectHandle);
	void ValidateArgs(UMethod* methodRef, void* args, size_t argSize);
	void ProcessEvent(UObject* object, UMethod* methodRef, void* args);

#pragma pack(push, 1)
	struct R_FString
	{
		crwstr buffer;
		int32 length;
		int32 alignmentPadding;
	};

	struct R_FName
	{
		uint32 comparisonIndex;
		uint32 number;
		uint32 displayIndex;
	};

	template<class T>
	struct TEnumAsByte
	{
		TEnumAsByte(): value(0)
		{

		}

		TEnumAsByte(T _value) : value(static_cast<uint8>(_value)) 
		{
		}
		
		T operator()() const 
		{
			return static_cast<T>((value)); 
		}

		uint8 value;
	};
#pragma pack(pop)
}
