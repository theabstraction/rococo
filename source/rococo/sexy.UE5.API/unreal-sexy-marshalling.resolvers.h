#pragma once

class UObject;
class UClass;
class UFunction;

namespace Rococo::UE::Marshal::Resolver
{
	enum class EMessageLevel
	{
		Info,
		Error
	};

	struct ObjectHandle
	{
		__int64 handleIndex;
	};

	struct IResolver
	{
		virtual UObject* ConstructObject(UObject* outer, UClass* classRef) = 0;
		virtual UClass* FindStaticClassRef(const TCHAR* fullPath) = 0;
		virtual UFunction* FindMethod(UClass& classRef, const TCHAR* methodName) = 0;
		virtual ObjectHandle GetHandleFromObject(UObject* object) = 0;
		virtual void InvokeMethod(UObject* object, UFunction* methodRef, void* args) = 0;
		virtual void LogMessage(EMessageLevel level, const char* msg) = 0;

		// Assuming hObject.handleIndex is equivalent to a TObjectPtr<UObject*>, we can convert the handle to a pointer
		// If hObject.handleIndex != 0 and the method returns null, then our API is bad and we cached a reference to a deleted object
		// An IException will be thrown.
		virtual UObject* GetObjectFromHandle(ObjectHandle hObject) = 0;
	};

	SEXY_MARSHALLING_API void SetReflectionResolver(IResolver* staticResolver);
}