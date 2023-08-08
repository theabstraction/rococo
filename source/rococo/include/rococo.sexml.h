#pragma once
#include <sexy.types.h>
#include <Sexy.S-Parser.h>

#ifndef ROCOCO_SEXML_API
# define ROCOCO_SEXML_API ROCOCO_API_IMPORT
#endif

namespace Rococo::Sex::SEXML
{
	enum class SEXMLValueType
	{
		Atomic,					// Allows us to static_cast<ISexyXMLAttributeStringValue&>(base) where base is a ISexyXMLAttributeValue&
		StringLiteral,			// Allows us to static_cast<ISexyXMLAttributeStringValue&>(base) where base is a ISexyXMLAttributeValue&
		AtomicList,				// Allows us to static_cast<ISexyXMLAttributeStringListValue&>(base) where base is a ISexyXMLAttributeValue&. Every value element is guaranteed to be an atomic expression
		StringLiteralList,		// Allows us to static_cast<ISexyXMLAttributeStringListValue&>(base) where base is a ISexyXMLAttributeValue&. Every value element is guaranteed to be a string literal expression
		MixedStringList,		// Allows us to static_cast<ISexyXMLAttributeStringListValue&>(base) where base is a ISexyXMLAttributeValue&. Every value element is guaranteed to be an atomic expression or a string literal expression
		SmallVector,			// Allows us to static_cast<ISexyMLAttributeSmallVectorValue&>(base) where base is a ISexyXMLAttributeValue&. Every value element is guaranteed to be parsed as a 64-bit floating point number
		SmallVectorI,			// Allows us to static_cast<ISexyMLAttributeSmallVectorValue&>(base) where base is a ISexyXMLAttributeValue&. Every value element is guaranteed to be parsed as a 32-bit signed integer.
		Raw						// The S expression of ISexyXMLAttributeValue gives the raw expression, there is nothing to cast
	};

	ROCOCO_INTERFACE ISexyXMLAttributeValue
	{
		virtual SEXMLValueType Type() const = 0;

		// Source expression
		virtual cr_sex S() const = 0;
	};

	ROCOCO_INTERFACE ISexyMLAttributeSmallVectorValue : ISexyXMLAttributeValue
	{
		// Returns 2, 3 or 4 -> (x y), (x y z) or (x y z w) respectively.
		// For quaternions we have the convention that w is always the scalar
		// For spans we have 2 -> (width height)=(dx dy) and 3 = (dx dy dz) respectively
		virtual [[nodiscard]] int NumberOfDimensions() const = 0;

		// Returns the pointer to the first double in the compact vector array. We use the Xi notation here for vector components, i is 0 to NumberOfDimensions-1, so X[0] is x, X[1] is y etc
		virtual [[nodiscard]] const double* const X() const = 0;
	};

	ROCOCO_INTERFACE ISexyMLAttributeSmallVectorIValue : ISexyXMLAttributeValue
	{
		// Returns 2, 3 or 4 -> (x y), (x y z) or (x y z w) respectively.
		// For rects we have (left top right bottom)
		// For spans we have 2 -> (width height)=(dx dy) and 3 = (dx dy dz) respectively
		virtual [[nodiscard]] int NumberOfDimensions() const = 0;

		// Returns the pointer to the first double in the compact vector array. We use the Xi notation here for vector components, i is 0 to NumberOfDimensions-1, so X[0] is x, X[1] is y etc
		virtual [[nodiscard]] const int32* X() const = 0;
	};

	ROCOCO_INTERFACE ISexyXMLAttributeStringListValue : ISexyXMLAttributeValue
	{
		virtual [[nodiscard]] size_t NumberOfElements() const = 0;

		// Gives the element using the 1 based index. [0] gives the attribute name string. Maximum string length is 32767 bytes for atomics
		virtual [[nodiscard]] fstring operator[](size_t index) const = 0;
	};

	ROCOCO_INTERFACE ISexyXMLAttributeStringValue : ISexyXMLAttributeValue
	{
		// Maximum string length is 0x7FFFFFFF bytes, or 1 byte under 2GB
		virtual [[nodiscard]] cstr c_str() const = 0;
		virtual [[nodiscard]] fstring ToFString() const = 0;
	};

	ROCOCO_INTERFACE ISEXYMLAttribute
	{
		enum { MAX_ATTRIBUTE_NAME_LENGTH = 128 };

		// The attribute name, consists of [A-Z][a-z] followed by any of  [A-Z] | [a-z] | [0-9] | '-' | '_' | '.' 
		virtual [[nodiscard]] cstr Name() const = 0;
		virtual [[nodiscard]] cr_sex S() const = 0;
		virtual [[nodiscard]] const ISexyXMLAttributeValue& Value() const = 0;
	};

	using cr_sattr = const ISEXYMLAttribute&;

	ROCOCO_INTERFACE ISEXMLDirective
	{
		// Maximum subspace and namespace lengths
		enum
		{
			// Maximum subspace length, including terminaing null character in C
			MAX_SUBSPACE_LENGTH = 64,

			// Maximum total namespace length, including terminaing null character in C
			MAX_FQNAME_LENGTH = 128
		};

		// A fully qualified name. It consists of a dot separated namespace, with each subspace beginning with a capital letter A-Z, and followed by any alphanumeric character [A-Z] | [a-z] | [0-9]. 
		// There is a maximum of 63 characters per subspace and a maximum of 127 characters in the total subspace. Example: Rococo.Sex.SEXML.A72
		virtual [[nodiscard]] cstr FQName() const = 0;

		// The number of name-value pairs in the directive
		virtual [[nodiscard]] size_t NumberOfAttributes() const = 0;

		// get the name-value pair by index as it appears in the SEXML file
		virtual [[nodiscard]] const ISEXYMLAttribute& GetAttributeByIndex(size_t index) const = 0;

		// get the name-value pair by name, if it exists, else returns nullptr
		virtual const ISEXYMLAttribute* FindAttributeByName(cstr name) const = 0;

		// Source expression
		virtual [[nodiscard]] cr_sex S() const = 0;

		virtual size_t NumberOfChildren() const = 0;

		virtual const ISEXMLDirective& GetChild(size_t index) const = 0;

		inline const ISEXMLDirective& operator[](size_t index) const
		{
			return GetChild(index);
		}
	};

	ROCOCO_INTERFACE ISEXMLDirectiveList
	{
		virtual [[nodiscard]] size_t NumberOfDirectives() const = 0;
		virtual [[nodiscard]] const ISEXMLDirective& operator[](size_t index) const = 0;
	};

	// cr_sdir is a shorthand for a const ISEXMLDirective
	using cr_sdir = const ISEXMLDirective&;

	// The supervisor gives the Free() method and also the list of top level directives
	ROCOCO_INTERFACE ISEXMLRootSupervisor : ISEXMLDirectiveList
	{
		// Root expression
		virtual [[nodiscard]] cr_sex S() const = 0;

		// The internal allocator
		virtual [[nodiscard]] IAllocator& Allocator() = 0;

		// Memory release
		virtual [[nodiscard]] void Free() = 0;
	};

	ROCOCO_SEXML_API [[nodiscard]] ISEXMLRootSupervisor* CreateSEXMLParser(IAllocator& allocator, cr_sex sRoot);
}