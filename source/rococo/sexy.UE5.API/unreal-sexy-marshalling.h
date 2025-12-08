#pragma once
#include <rococo.types.h>
#include <sexy.types.h>
#include <sexy.script.h>

class UClass;
class UObject;
class UMethod;
typedef int UnknownType;

#pragma pack(push, 1)

namespace Rococo::UE::Native
{
	namespace Delegate
	{

	}

	namespace Enum
	{
		template<class T>
		struct R_TEnumAsByte
		{
			R_TEnumAsByte() : value(0)
			{

			}

			R_TEnumAsByte(T _value) : value(static_cast<uint8>(_value))
			{
			}

			T operator()() const
			{
				return static_cast<T>((value));
			}

			uint8 value;
		};
	}

	namespace Struct
	{
		template<class T>
		struct R_TArray
		{
			T* allocatorInstance;
			int32 arrayNum;
			int32 arrayMax;
		};

		template<class T>
		struct R_TSet
		{
			int unknown;
		};

		template<class KEY, class VALUE>
		struct R_TMap
		{
			int unknown;
		};

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
		struct R_TObjectPtr
		{
			union
			{
				int64 hObject;
				T* pObject;
			} u;
		};

		struct R_FText
		{
			int64 id;
		};

		template<class T>
		struct R_TDelegate
		{
			UObject* objectHandler;
			R_FName methodName;
		};

		template<class T>
		struct R_TSoftObjectPtr
		{
			T* pObject;
		};

		template<class T>
		struct R_TSubclassOf
		{
			int dummy;
		};

		template<class T>
		struct R_TScriptInterface
		{
			int dummy;
		};
		
	}

	int64 ConstructUObject(Rococo::Script::NativeCallEnvironment& e);
	UMethod* GetNCEUMethod(Rococo::Script::NativeCallEnvironment& e);
	UMethod& GetMethod(UClass& classRef, crwstr methodName);
	UObject* GetNCEUObject(Rococo::Script::NativeCallEnvironment& e, int64 objectHandle);
	void ValidateArgs(UMethod* methodRef, void* args, size_t argSize);
	void ProcessEvent(UObject* object, UMethod* methodRef, void* args);
}

#pragma pack(pop)
