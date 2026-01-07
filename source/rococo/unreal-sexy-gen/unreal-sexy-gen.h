#pragma once
#include <rococo.types.h>

namespace Rococo::Strings
{
	DECLARE_ROCOCO_INTERFACE StringBuilder;
}

namespace Rococo::Unreal
{
	ROCOCO_INTERFACE IUnrealEnumDef
	{
		virtual cstr Name() const = 0;
		virtual cstr Package() const = 0;
		virtual int64 MaxValue() const = 0;
		virtual int32 NumberOfKeys() const = 0;
		virtual cstr GetKey(int32 index) const = 0;
		virtual int64 GetValue(int32 index) const = 0;
		virtual size_t GetUnderlyingSize() const = 0;
	};

	ROCOCO_INTERFACE IEnums
	{
		virtual const IUnrealEnumDef* FindEnum(cstr name) const = 0;
	};

	ROCOCO_INTERFACE IObjectSearcher
	{
		virtual bool HasSexyCounterpart(cstr argType, cstr elementType, cstr keyType) const = 0;
		virtual bool IsKnownElementType(cstr argType) const = 0;
	};

	ROCOCO_INTERFACE IUnrealArg
	{
		virtual void AppendName(Strings::StringBuilder & sb, bool makeSexyVariableName = false) const = 0;
		virtual cstr ArgType() const = 0;
		virtual cstr KeyType() const = 0;
		virtual cstr ElementType() const = 0;
		virtual bool GetObjectPointerType(char* buffer, size_t capacity) const = 0;

		// Returns true if Sexy can marshal the argument
		virtual bool HasSexyCounterpart() const = 0;
		virtual bool IsConst() const = 0;
		virtual bool IsCPPOutput() const = 0;
		virtual bool IsSexyOutput() const = 0;
		virtual bool IsPtr() const = 0;
		virtual bool IsRef() const = 0;
		virtual bool IsContainer() const = 0;
	};

	ROCOCO_INTERFACE IUnrealFunction
	{
		// Returns true is Sexy can marshal the function call
		virtual bool HasSexyCounterpart() const = 0;
		virtual void AppendFunctionName(Strings::StringBuilder& sb, bool makeSexyVariableName = false) const = 0;
		virtual IUnrealArg* GetArg(size_t index) = 0;
		virtual cstr Name() const = 0;
	};

	ROCOCO_INTERFACE IUnrealClass
	{
		virtual cstr PackageName() const = 0;
		virtual cstr ShortName() const = 0;
		virtual size_t MethodCount() const = 0;
		virtual IUnrealFunction& GetFunction(size_t index) = 0;
		virtual size_t ClassIndex() const = 0;
	};

	ROCOCO_INTERFACE IUnrealStructElement
	{
		virtual bool IsBitfield() const = 0;
		virtual int Offset() const = 0;
		virtual int SizeOf() const = 0;
		virtual cstr TypeName() const = 0;
		virtual cstr FieldName() const = 0;

		// Defined if TypeName is TMap
		virtual cstr InnerKeyType() const = 0;

		// Defined if TypeName is TArray or TSet or TMap
		virtual cstr InnerValueType() const = 0;

		virtual void Throw(cstr message) = 0;
	};

	ROCOCO_INTERFACE IUnrealStruct
	{
		virtual cstr TypeName() const = 0;
		virtual cstr Package() const = 0;

		virtual int Alignment() const = 0;
		virtual int SizeOf() const = 0;

		virtual size_t ElementCount() const = 0;

		// Returns the element at the given index
		virtual IUnrealStructElement& operator[](size_t index) = 0;
	};

	ROCOCO_INTERFACE IMarshalType
	{
		virtual cstr CPPName() const = 0;
		virtual cstr SXYName() const = 0;
	};

	ROCOCO_INTERFACE IClassSystem
	{
		virtual void AddClass(const IUnrealClass& classRef) = 0;
		virtual void Commit() = 0;
		virtual void Free() = 0;
	};

	IClassSystem* CreateClassSystem(crwstr outputDirectory, crwstr sxyOutputDirectory);
}