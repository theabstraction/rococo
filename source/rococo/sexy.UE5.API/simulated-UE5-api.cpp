#include <rococo.types.h>
#include <sexy.script.h>

typedef int UnknownType;
class UClass;
class UObject;
class UMethod {};
class UIKRigDefinition;

namespace Rococo::UE5::Marshal
{
	int64 ConstructUObject(Rococo::Script::NativeCallEnvironment& e)
	{
		int64 outerObjectHandle = e.ss.GetScriptContext();
		UNUSED(outerObjectHandle);
		// UObject* outerObject = GetObjectFromHandle(outerObjectHandle);

		UClass* classRef = reinterpret_cast<UClass*>(e.context);
		UNUSED(e);
		UNUSED(classRef);
		// The classRef in Unreal Engine can be used to construct an object. We must return a 64-bit handle to the object, ideally by unwrapping the API from TObjectPtr<UObject>
		return 0;
	}

	UMethod* GetNCEUMethod(Rococo::Script::NativeCallEnvironment& e)
	{
		UMethod* method = reinterpret_cast<UMethod*>(e.context);
		UNUSED(method);
		UNUSED(e);
		return nullptr;
	}

	UMethod& GetMethod(UClass& classRef, crwstr methodName)
	{
		UNUSED(classRef);
		UNUSED(methodName);
		// In the Unreal Engine build we use classRef to lookup the method name and throw an exception is there is no result
		static UMethod dummy;
		return dummy;
	}

	UObject* GetNCEUObject(Rococo::Script::NativeCallEnvironment& e, int64 objectHandle)
	{
		UNUSED(e);
		UNUSED(objectHandle);
		return nullptr;
	}

	void ValidateArgs(UMethod* methodRef, void* args, size_t argSize)
	{
		UNUSED(methodRef);
		UNUSED(args);
		UNUSED(argSize);
	}

	void ProcessEvent(UObject* object, UMethod* methodRef, void* args)
	{
		UNUSED(object);
		UNUSED(methodRef);
		UNUSED(args);
	}
}