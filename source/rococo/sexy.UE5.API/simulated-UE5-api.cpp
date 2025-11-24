#include <rococo.types.h>
#include <sexy.script.h>

typedef int UnknownType;
class UClass;
class UObject;
class UMethod;
class UIKRigDefinition;

namespace Rococo::UE5::Marshal
{
	UMethod* GetNCEUMethod(Rococo::Script::NativeCallEnvironment& e)
	{
		UNUSED(e);
		return nullptr;
	}

	UObject* GetNCEUObject(Rococo::Script::NativeCallEnvironment& e)
	{
		UNUSED(e);
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