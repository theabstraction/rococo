#pragma once
#include <rococo.types.h>
#include <sexy.types.h>
#include <sexy.script.h>

class UClass;
class UObject;
class UFunction;
typedef int UnknownType;

#pragma pack(push, 1)

namespace Rococo::UE
{
	typedef void (*FN_AddSexyNatives_Unreal)(Rococo::Script::IPublicScriptSystem& ss);

	ROCOCO_INTERFACE ISexyNativeRegistry
	{
		virtual void AddNativeAPI(cstr package, cstr className, FN_AddSexyNatives_Unreal fnAddNatives) = 0;
	};

	ROCOCO_INTERFACE IClassMatch
	{
		virtual void OnMatch(cstr ns, cstr className) = 0;
	};

	ROCOCO_INTERFACE ISexyNativeRegistrySupervisor
	{
		virtual void RegisterPackagesByFilters(const Rococo::Sex::ISExpression* referenceSrc, int referenceStartIndex, cstr filters[], int numberOfFilters, Rococo::Script::IPublicScriptSystem &ss, IClassMatch& onMatch) = 0;
		virtual void Free();
	};

	SEXY_MARSHALLING_API ISexyNativeRegistrySupervisor* CreateRegistryForEverything();
}

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
			uint8 _opaqueData[80];
		};

		template<class KEY, class VALUE>
		struct R_TMap
		{
			R_TSet<KEY> _opaqueSet;
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

		struct R_ITextData;

		struct R_FText
		{
			R_ITextData* data;
			int32 flags;
			uint32 _padding;
		};

		template<class T>
		struct R_TDelegate
		{
			char _opaque_data[32];
		};

		struct R_FUtf8String
		{
			char* utf8Encoding;
			int len;
		};

		static_assert(sizeof(R_FUtf8String) == 12);

		template<class T>
		struct R_TSoftObjectPtr
		{
			T* pWeakObjectPtr;
			R_FName packageName;
			R_FName assetName;
			R_FUtf8String subPathString;
			uint32 padding;
		};

		template<class T>
		struct R_TSubclassOf
		{
			R_TObjectPtr<T> classRef;
		};

		template<class T>
		struct R_TScriptInterface
		{
			int dummy;
		};

		template<class T, class INNER>
		struct R_TEnum
		{
			INNER value;
		}; 
	}
}

namespace Rococo::UE::Marshal
{
	UClass& GetStaticClassRef(crwstr fullPath);
	int64 ConstructUObject(Rococo::Script::NativeCallEnvironment& e);
	UFunction* GetNCEUMethod(Rococo::Script::NativeCallEnvironment& e);
	UObject* GetNCEUObject(Rococo::Script::NativeCallEnvironment& e, int64 objectHandle);
	void ValidateArgs(UFunction* methodRef, void* args, size_t argSize);
	void ProcessEvent(UObject* object, UFunction* methodRef, void* args);
	void ScriptUFunction(Rococo::Script::IPublicScriptSystem& ss, const Rococo::Compiler::INamespace& ns, cstr implementationName, int lineNumber, UClass& classRef, Rococo::Script::FN_NATIVE_CALL nativeCall, crwstr methodName, cstr scriptSignature);

	template<class T> inline void CloneToOutputFromArg(T& cloneTarget, const T& origin)
	{
		cloneTarget = cloneSource;
	}
}

namespace Rococo::Script
{
	SCRIPTEXPORT_API void AddNativeCallSecurity(IPublicScriptSystem& ss, cstr nativeNamespace, cstr permittedPingPath);
}

#ifndef TEXT
#define TEXT(x) L ## x
#endif

#pragma pack(pop)
