#include "unreal-sexy-marshalling.h"

#include <rococo.types.h>
#include <sexy.script.h>
#include <rococo.strings.h>

#include "unreal-sexy-marshalling.resolvers.h"

typedef int UnknownType;
class UClass;
class UObject;
class UIKRigDefinition;

#include <stdio.h>

namespace Rococo::UE5::Marshal
{
	struct MarshalException : IException
	{
		char msg[256] = { 0 };

		const char* Message() const override
		{
			return msg;
		}

		int ErrorCode() const override
		{
			return 0;
		}

		Debugging::IStackFrameEnumerator* StackFrames() override
		{
			return nullptr;
		}
	};

	[[noreturn]] void ThrowException(cstr format, ...)
	{
		MarshalException ex;
		va_list args;
		va_start(args, format);
		vsprintf_s(ex.msg, sizeof ex.msg, format, args);
		va_end(args);

		throw ex;
	}
}

namespace Rococo::UE5::Marshal::Resolver
{
	[[noreturn]] void ThrowResolver(cstr filename, int lineNumber, cstr functionName)
	{
		ThrowException("Rococo::UE5::Marshal::Resolver::SetReflectionResolver must be invoked first in call to %s from %s line %d", functionName, filename, lineNumber);
	}

	struct UndefinedResolver : IResolver
	{
		UObject* ConstructObject(UObject* outer, UClass* classRef) override
		{
			UNUSED(outer);
			UNUSED(classRef);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		UClass* FindStaticClassRef(const TCHAR* fullPath)
		{
			UNUSED(fullPath);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		UFunction* FindMethod(UClass& classRef, const TCHAR* methodName)
		{
			UNUSED(classRef);
			UNUSED(methodName);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		void InvokeMethod(UObject* object, UFunction* methodRef, void* args)
		{
			UNUSED(object);
			UNUSED(methodRef);
			UNUSED(args);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		void LogMessage(EMessageLevel level, cstr msg) override
		{
			UNUSED(level);
			UNUSED(msg);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		UObject* GetObjectFromHandle(ObjectHandle hObject) override
		{
			UNUSED(hObject);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}

		ObjectHandle GetHandleFromObject(UObject* object) override
		{
			UNUSED(object);
			ThrowResolver(__FILE__, __LINE__, __FUNCTION__);
		}
	} s_undefinedResolver;

	IResolver* s_resolver = &s_undefinedResolver;

	SEXY_MARSHALLING_API void SetReflectionResolver(IResolver* staticResolver)
	{
		s_resolver = staticResolver;
	}

	inline UClass* FindClassByPath(crwstr fullPath)
	{
		return s_resolver->FindStaticClassRef(fullPath);
	}

	inline UFunction* FindMethod(UClass& classRef, crwstr methodName)
	{
		return s_resolver->FindMethod(classRef, methodName);
	}

	inline void InvokeMethod(UObject* object, UFunction* method, void* args)
	{
		s_resolver->InvokeMethod(object, method, args);
	}

	inline void LogMarshallingErrorDirect(cstr msg)
	{
		s_resolver->LogMessage(EMessageLevel::Error, msg);
	}

	UObject* GetObjectFromHandle(ObjectHandle hObject)
	{
		auto* object = s_resolver->GetObjectFromHandle(hObject);
		if (object == nullptr && hObject.handleIndex != 0)
		{
			ThrowException(__FUNCTION__ ": object handle was 0x%llx, but object pointer returned was not zero. Bad API", hObject.handleIndex);
		}
		return object;
	}

	inline UObject* ConstructObject(UObject* outer, UClass* classRef)
	{
		return s_resolver->ConstructObject(outer, classRef);
	}

	ObjectHandle GetHandleFromObject(UObject* object)
	{
		auto handle = s_resolver->GetHandleFromObject(object);
		if (handle.handleIndex == 0 && object != nullptr)
		{
			ThrowException(__FUNCTION__ ": <object> was not null, but method returned 0");
		}
		return handle;
	}
}

namespace Rococo::UE5::Marshal
{
	int64 ConstructUObject(Rococo::Script::NativeCallEnvironment& e)
	{
		int64 outerObjectHandle = e.ss.GetScriptContext();
		UObject* outerObject = Resolver::GetObjectFromHandle(Resolver::ObjectHandle{ outerObjectHandle });

		UClass* classRef = reinterpret_cast<UClass*>(e.context);

		UObject* newObject = Resolver::ConstructObject(outerObject, classRef);

		// The classRef in Unreal Engine can be used to construct an object. We must return a 64-bit handle to the object, ideally by unwrapping the API from TObjectPtr<UObject>
		return Resolver::GetHandleFromObject(newObject).handleIndex;
	}

	UFunction* GetNCEUMethod(Rococo::Script::NativeCallEnvironment& e)
	{
		UFunction* method = reinterpret_cast<UFunction*>(e.context);
		return method;
	}

	UObject* GetNCEUObject(Rococo::Script::NativeCallEnvironment& e, int64 objectHandle)
	{
		UNUSED(e);
		auto* object = Resolver::GetObjectFromHandle(Resolver::ObjectHandle{ objectHandle });
		return object;
	}

	void ValidateArgs(UFunction* methodRef, void* args, size_t argSize)
	{
		UNUSED(methodRef);
		UNUSED(args);
		UNUSED(argSize);
	}

	void ProcessEvent(UObject* object, UFunction* methodRef, void* args)
	{
		Resolver::InvokeMethod(object, methodRef, args);
	}

	void LogMarshallingError(cstr format, ...)
	{
		va_list args;
		va_start(args, format);
		char msg[1024];
		vsprintf_s(msg, sizeof msg, format, args);
		va_end(args);

		Resolver::LogMarshallingErrorDirect(msg);
	}

	void ScriptUFunction(Rococo::Script::IPublicScriptSystem& ss, const Rococo::Compiler::INamespace& ns, cstr implementationName, int lineNumber, UClass& classRef, Rococo::Script::FN_NATIVE_CALL nativeCall, crwstr methodName, cstr scriptSignature)
	{
		UFunction* method = Resolver::FindMethod(classRef, methodName);
		if (method == nullptr)
		{
			LogMarshallingError("No method found named '%ls' in class marshalling code defined at %s line %d", methodName, implementationName, lineNumber);
			return;
		}
		ss.AddNativeCall(ns, nativeCall, method, scriptSignature, implementationName, lineNumber);
	}

	UClass& GetStaticClassRef(crwstr fullPath)
	{
		auto* classPtr = Resolver::FindClassByPath(fullPath);
		if (!classPtr)
		{
			ThrowException("Could not resolve path into class: %ls", fullPath);
		}
		return *classPtr;
	}
}
