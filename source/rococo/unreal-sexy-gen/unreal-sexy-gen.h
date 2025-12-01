#pragma once
#include <rococo.types.h>

namespace Rococo::Strings
{
	DECLARE_ROCOCO_INTERFACE StringBuilder;
}

namespace Rococo::Unreal
{
	ROCOCO_INTERFACE IUnrealArg
	{
		virtual void AppendName(Strings::StringBuilder & sb, bool makeSexyVariableName = false) const = 0;
		virtual void AppendType(Strings::StringBuilder& sb, bool makeSexyVariableType = false) const = 0;
		virtual bool GetObjectPointerType(char* buffer, size_t capacity) const = 0;

		// Returns true if Sexy can marshal the argument
		virtual bool HasSexyCounterpart() const = 0;
		virtual bool IsConst() const = 0;
		virtual bool IsCPPOutput() const = 0;
		virtual bool IsSexyOutput() const = 0;
		virtual bool IsPtr() const = 0;
		virtual bool IsRef() const = 0;
	};

	ROCOCO_INTERFACE IUnrealFunction
	{
		// Returns true is Sexy can marshal the function call
		virtual bool HasSexyCounterpart() const = 0;
		virtual void AppendFunctionName(Strings::StringBuilder& sb) const = 0;
		virtual IUnrealArg* GetArg(size_t index) = 0;
	};

	ROCOCO_INTERFACE IUnrealClass
	{
		virtual cstr PackageName() const = 0;
		virtual cstr ShortName() const = 0;
		virtual size_t MethodCount() const = 0;
		virtual IUnrealFunction& GetFunction(size_t index) = 0;
	};

	ROCOCO_INTERFACE IUnrealStructElement
	{
		virtual int Offset() const = 0;
		virtual int SizeOf() const = 0;
		virtual cstr TypeName() const = 0;
		virtual cstr FieldName() const = 0;
	};

	ROCOCO_INTERFACE IUnrealStruct
	{
		virtual cstr TypeName() const = 0;
		virtual cstr Package() const = 0;

		virtual int Alignment() const = 0;
		virtual int SizeOf() const = 0;

		virtual size_t ElementCount() const = 0;
		virtual IUnrealStructElement& operator[](size_t index) = 0;
	};
}