#pragma once

#include <rococo.types.h>

namespace Rococo::JSon
{
	DECLARE_ROCOCO_INTERFACE INameValueTreeSupervisor;

	ROCOCO_INTERFACE IJSonParser
	{
		virtual INameValueTreeSupervisor* Parse(cstr text) = 0;
	};

	ROCOCO_INTERFACE IJSonParserSupervisor : IJSonParser
	{
		virtual void Free() = 0;
	};

	ROCOCO_API IJSonParserSupervisor* CreateJSonParser();

	ROCOCO_INTERFACE INameValueTreeBuilder
	{
		virtual void AddValue(cstr name, cstr format, ...) = 0;
		virtual void BeginObject(cstr name) = 0;
		virtual void EndObject() = 0;
	};

	ROCOCO_INTERFACE INameValueBranch
	{
		// Returns the attribute string for the attribute with the given name, or nullptr if not matched. Lookup time is O(AttributeCount)

		virtual cstr At(cstr name) const = 0;
		
		inline cstr operator[](cstr name) const
		{
			return At(name);
		}

		virtual int32 AttributeCount() const = 0;
		virtual void GetAttribute(int32 index, cstr& a, cstr& b) const = 0;

		virtual INameValueBranch* Parent() = 0;
		virtual int32 ChildCount() const = 0;
		virtual INameValueBranch& Child(int32 index) = 0;

		// TryAs<...> : attempt to interpret the value for the given named attribute as a particular type.
		// If no match or conversion can occur the result is the default and wasFound is set to false.
		// On match and conversion the converted value is returned and wasFound is set to true
		virtual int32 TryGet(cstr name, bool& wasFound, int32 default) const = 0;
		virtual float32 TryGet(cstr name, bool& wasFound, float32 default) const = 0;
		virtual int64 TryGet(cstr name, bool& wasFound, int64 default) const = 0;
		virtual float64 TryGet(cstr name, bool& wasFound, float64 default) const = 0;
		virtual bool TryGet(cstr name, bool& wasFound, bool default) const = 0;
	};

	ROCOCO_INTERFACE INameValueTree
	{
		virtual INameValueBranch& Root() = 0;
	};

	ROCOCO_INTERFACE INameValueTreeSupervisor : INameValueTree
	{
		virtual void Free() = 0;
	};
}