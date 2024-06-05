#pragma once
#include <rococo.types.h>

#ifndef ROCOCO_SEXML_API
# define ROCOCO_SEXML_API ROCOCO_API_IMPORT
#endif

namespace Rococo::Sex
{
	struct ISExpression;
	typedef const ISExpression& cr_sex;
}

namespace Rococo::Strings
{
	struct StringBuilder;
}

namespace Rococo::Sex::SEXML
{
	enum class SEXMLValueType
	{
		Atomic,					// Allows us to static_cast<ISexyXMLAttributeStringValue&>(base) where base is a ISEXMLAttributeValue&
		StringLiteral,			// Allows us to static_cast<ISexyXMLAttributeStringValue&>(base) where base is a ISEXMLAttributeValue&
		AtomicList,				// Allows us to static_cast<ISexyXMLAttributeStringListValue&>(base) where base is a ISEXMLAttributeValue&. Every value element is guaranteed to be an atomic expression
		StringLiteralList,		// Allows us to static_cast<ISexyXMLAttributeStringListValue&>(base) where base is a ISEXMLAttributeValue&. Every value element is guaranteed to be a string literal expression
		MixedStringList,		// Allows us to static_cast<ISexyXMLAttributeStringListValue&>(base) where base is a ISEXMLAttributeValue&. Every value element is guaranteed to be an atomic expression or a string literal expression
		SmallVector,			// Allows us to static_cast<ISEXMLAttributeSmallVectorValue&>(base) where base is a ISEXMLAttributeValue&. Every value element is guaranteed to be parsed as a 64-bit floating point number
		SmallVectorI,			// Allows us to static_cast<ISEXMLAttributeSmallVectorValue&>(base) where base is a ISEXMLAttributeValue&. Every value element is guaranteed to be parsed as a 32-bit signed integer.
		Raw						// The S expression of ISEXMLAttributeValue gives the raw expression, there is nothing to cast
	};

	ROCOCO_INTERFACE ISEXMLAttributeValue
	{
		virtual [[nodiscard]] SEXMLValueType Type() const = 0;

		// Source expression
		virtual [[nodiscard]] cr_sex S() const = 0;

		virtual ~ISEXMLAttributeValue() {}
	};

	ROCOCO_INTERFACE ISEXMLAttributeSmallVectorValue : ISEXMLAttributeValue
	{
		// Returns 2, 3 or 4 -> (x y), (x y z) or (x y z w) respectively.
		// For quaternions we have the convention that w is always the scalar
		// For spans we have 2 -> (width height)=(dx dy) and 3 = (dx dy dz) respectively
		virtual [[nodiscard]] int NumberOfDimensions() const = 0;

		// Returns the pointer to the first double in the compact vector array. We use the Xi notation here for vector components, i is 0 to NumberOfDimensions-1, so X[0] is x, X[1] is y etc
		virtual [[nodiscard]] const double* const X() const = 0;
	};

	ROCOCO_INTERFACE ISEXMLAttributeSmallVectorIValue : ISEXMLAttributeValue
	{
		// Returns 2, 3 or 4 -> (x y), (x y z) or (x y z w) respectively.
		// For rects we have (left top right bottom)
		// For spans we have 2 -> (width height)=(dx dy) and 3 = (dx dy dz) respectively
		virtual [[nodiscard]] int NumberOfDimensions() const = 0;

		// Returns the pointer to the first double in the compact vector array. We use the Xi notation here for vector components, i is 0 to NumberOfDimensions-1, so X[0] is x, X[1] is y etc
		virtual [[nodiscard]] const int32* X() const = 0;
	};

	ROCOCO_INTERFACE ISEXMLAttributeStringListValue : ISEXMLAttributeValue
	{
		virtual [[nodiscard]] size_t NumberOfElements() const = 0;

		// Gives the element using the 1 based index. [0] gives the attribute name string. Maximum string length is 32767 bytes for atomics
		virtual [[nodiscard]] fstring operator[](size_t index) const = 0;
	};

	ROCOCO_INTERFACE ISEXMLAttributeStringValue : ISEXMLAttributeValue
	{
		// Maximum string length is 0x7FFFFFFF bytes, or 1 byte under 2GB
		virtual [[nodiscard]] cstr c_str() const = 0;
		virtual [[nodiscard]] fstring ToFString() const = 0;
	};

	ROCOCO_INTERFACE ISEXMLAttribute
	{
		enum { MAX_ATTRIBUTE_NAME_LENGTH = 128 };

		// The attribute name, consists of [A-Z][a-z] followed by any of  [A-Z] | [a-z] | [0-9] | '-' | '_' | '.' 
		virtual [[nodiscard]] cstr Name() const = 0;
		virtual [[nodiscard]] cr_sex S() const = 0;
		virtual [[nodiscard]] const ISEXMLAttributeValue& Value() const = 0;
	};

	ROCOCO_SEXML_API [[nodiscard]] const ISEXMLAttributeStringValue& AsAtomic(const ISEXMLAttributeValue& value);
	ROCOCO_SEXML_API [[nodiscard]] int32 AsAtomicInt32(const ISEXMLAttributeValue& value);
	ROCOCO_SEXML_API [[nodiscard]] double AsAtomicDouble(const ISEXMLAttributeValue& value);
	ROCOCO_SEXML_API [[nodiscard]] const ISEXMLAttributeStringListValue& AsStringList(const ISEXMLAttributeValue& value);
	ROCOCO_SEXML_API [[nodiscard]] uint64 AsFlags(const ISEXMLAttributeValue& value, Function<uint64(const fstring& token)> mapNameToFlag);
	ROCOCO_SEXML_API [[nodiscard]] const ISEXMLAttributeStringValue& AsString(const ISEXMLAttributeValue& value);
	ROCOCO_SEXML_API [[nodiscard]] bool AsBool(const ISEXMLAttributeValue& value);
	ROCOCO_SEXML_API [[nodiscard]] const ISEXMLAttributeSmallVectorIValue& AsSmallIVector(const ISEXMLAttributeValue& value);
	ROCOCO_SEXML_API [[nodiscard]] const ISEXMLAttributeSmallVectorValue& AsSmallVector(const ISEXMLAttributeValue& value);

	using cr_sattr = const ISEXMLAttribute&;

	struct ISEXMLDirectiveList;

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

		// If the fqName does not match the argument the method throws an exception citing the fqName, the expected fqName and the expression associated with the directive
		virtual void Assert(cstr expectedFqName) const = 0;

		// A fully qualified name. It consists of a dot separated namespace, with each subspace beginning with a capital letter A-Z, and followed by any alphanumeric character [A-Z] | [a-z] | [0-9]. 
		// There is a maximum of 63 characters per subspace and a maximum of 127 characters in the total subspace. Example: Rococo.Sex.SEXML.HAL9000
		virtual [[nodiscard]] cstr FQName() const = 0;

		// The number of name-value pairs in the directive
		virtual [[nodiscard]] size_t NumberOfAttributes() const = 0;

		// get the name-value pair by index as it appears in the SEXML file
		virtual [[nodiscard]] const ISEXMLAttribute& GetAttributeByIndex(size_t index) const = 0;

		// get the name-value pair by name, if it exists, else returns nullptr
		virtual [[nodiscard]] const ISEXMLAttribute* FindAttributeByName(cstr name) const = 0;

		// get the name-value pair by name, if it exists, else throws an exception
		virtual [[nodiscard]] const ISEXMLAttribute& GetAttributeByName(cstr name) const = 0;

		inline const ISEXMLAttributeValue& operator[](cstr name) const
		{
			return GetAttributeByName(name).Value();
		}

		// Source expression
		virtual [[nodiscard]] cr_sex S() const = 0;

		virtual [[nodiscard]] size_t NumberOfChildren() const = 0;

		virtual [[nodiscard]] const ISEXMLDirective& GetChild(size_t index) const = 0;

		// Finds the first directive that matches the fqName starting with [startIndex].
		// [startIndex] is updated to the matching directive's index.
		// if [fqName] is null then returns the child at [startIndex]
		// If no child satisfies the critera an exception is thrown citing the parent expression and the fqName
		virtual [[nodiscard]] const ISEXMLDirective& GetDirectivesFirstChild(IN OUT size_t& startIndex, cstr fqName) const = 0;

		// Finds the first directive that matches the fqName starting with [startIndex].
		// [startIndex] is updated to the matching directive's index.
		// if [fqName] is null then returns the child at [startIndex]
		// If no child satisfies the criteria returns nullptr
		virtual [[nodiscard]] const ISEXMLDirective* FindFirstChild(IN OUT size_t& startIndex, cstr fqName) const = 0;

		inline [[nodiscard]] const ISEXMLDirective& operator[](size_t index) const
		{
			return GetChild(index);
		}

		virtual [[nodiscard]] const ISEXMLDirectiveList& Children() const = 0;
	};

	ROCOCO_SEXML_API [[nodiscard]] Vec2i GetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, Vec2i defaultValues);
	ROCOCO_SEXML_API [[nodiscard]] GuiRect GetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, GuiRect defaultValues);
	ROCOCO_SEXML_API [[nodiscard]] bool TryGetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, OUT Vec2i& resultValues);
	ROCOCO_SEXML_API [[nodiscard]] bool TryGetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, OUT Vec2F64& resultValues);
	ROCOCO_SEXML_API [[nodiscard]] bool GetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, bool defaultValue);
	ROCOCO_SEXML_API [[nodiscard]] const fstring GetOptionalAttribute(const ISEXMLDirective& directive, cstr attributeName, const fstring defaultValue);

	ROCOCO_INTERFACE ISEXMLDirectiveList
	{
		virtual [[nodiscard]] size_t NumberOfDirectives() const = 0;
		virtual [[nodiscard]] const ISEXMLDirective& operator[](size_t index) const = 0;
		virtual [[nodiscard]] cr_sex S() const = 0;
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

	// Searches the directive list for the next directive with specified name. If none are found null is returned
	// Start index should generally be initialized with 0. With each function call it is moved to the directive index just beyond the returned result
	ROCOCO_SEXML_API [[nodiscard]] const ISEXMLDirective* FindDirective(const ISEXMLDirectiveList& items, cstr directiveName, IN OUT size_t& startIndex);

	// Searches the directive list for the next directive with specified name. If none are found an exception is thrown
	// Start index should generally be initialized with 0. With each function call it is moved to the directive index just beyond the returned result
	ROCOCO_SEXML_API [[nodiscard]] const ISEXMLDirective& GetDirective(const ISEXMLDirectiveList& items, cstr directiveName, IN OUT size_t& startIndex);

	ROCOCO_INTERFACE ISEXMLBuilder
	{
		// Adds a child to the current directive, or a top level to the root if no current directive exists. 
		// Throws an exception if the name does not pass ISEXMLDirective::FQName() parsering rules.
		virtual ISEXMLBuilder& AddDirective(cstr name) = 0;

		// Closes the current directive definition, or throws an exception if one does not exist
		virtual ISEXMLBuilder& CloseDirective() = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. 
		// The value must have no special S-Expression characters: blankspace, nor any of ()":
		virtual ISEXMLBuilder& AddAtomicAttribute(cstr name, cstr value) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. 
		// The value must have no special S-Expression characters: blankspace, nor any of ()":
		virtual ISEXMLBuilder& AddAtomicAttribute(cstr name, int32 value) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. 
		// The value must have no special S-Expression characters: blankspace, nor any of ()":
		virtual ISEXMLBuilder& AddAtomicAttribute(cstr name, int64 value) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. 
		// The value must have no special S-Expression characters: blankspace, nor any of ()":
		// The data is stored in hexadecimal
		virtual ISEXMLBuilder& AddAtomicAttribute(cstr name, uint64 value) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. 
		// The value must have no special S-Expression characters: blankspace, nor any of ()":
		// The data is stored in hexadecimal
		virtual ISEXMLBuilder& AddAtomicAttribute(cstr name, uint32 value) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. 
		// The value must have no special S-Expression characters: blankspace, nor any of ()":
		// The data is stored as either 'true' or 'false'
		virtual ISEXMLBuilder& AddAtomicAttribute(cstr name, bool value) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. 
		// The value must have no special S-Expression characters: blankspace, nor any of ()":
		// The data is stored as either 'true' or 'false'
		virtual ISEXMLBuilder& AddAtomicAttribute(cstr name, float value) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. 
		// The value must have no special S-Expression characters: blankspace, nor any of ()":
		virtual ISEXMLBuilder& AddAtomicAttribute(cstr name, double value) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules.
		// Escapes a string literal expression which can contain any sequence of characters
		virtual ISEXMLBuilder& AddStringLiteral(cstr  name, cstr value) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. Adds two value components
		virtual ISEXMLBuilder& AddVec2(cstr name, double x, double y) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. Adds three value components
		virtual ISEXMLBuilder& AddVec3(cstr name, double x, double y, double z) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. Adds four value components
		virtual ISEXMLBuilder& AddVec4(cstr name, double x, double y, double z, double w) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. Adds four value components
		virtual ISEXMLBuilder& AddQuat(cstr name, double Vx, double Vy, double Vz, double scalar, bool addLayoutComment = true) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. Adds two value components
		virtual ISEXMLBuilder& AddVec2i(cstr name, int32 x, int32 y) = 0;

		// The name has to follow ISEXMLAttribute::Name() rules. Adds four value components
		// data as hexadecimal if asHex is true, else as a signed decimal integer
		virtual ISEXMLBuilder& AddRecti(cstr name, int32 left, int32 top, int32 right, int32 bottom, bool addLayoutComment = true) = 0;

		// The builder adds a list attribute
		virtual ISEXMLBuilder& OpenListAttribute(cstr name) = 0;

		// Retrieves a string builder for appending items to a list attribute
		virtual Rococo::Strings::StringBuilder& GetListBuilder() = 0;

		// Adds a string literal to the currently open list, and escapes where necessary
		virtual ISEXMLBuilder& AddEscapedStringToList(cstr s) = 0;

		// Closes the current list attribute, or throws an exception if none are open
		virtual ISEXMLBuilder& CloseListAttribute() = 0;

		// Deallocate resource to ISEXMLBuilder and invalidates any existing references
		virtual void Free() = 0;

		virtual void ValidateClosed() = 0;
	};

	// Piggy-backs onto a string builder to format a SEXML file. If [compact] is set to true then the format is dense with a minimum of tabs and newlines
	ROCOCO_SEXML_API [[nodiscard]] ISEXMLBuilder* CreateSEXMLBuilder(Rococo::Strings::StringBuilder& sb, bool compact);
}

namespace Rococo::OS
{
	// Gets the full path for the user's XML and populates the supplied buffer with the result 
	// If organization is not provided the default is chosen.
	ROCOCO_SEXML_API [[nodiscard]] void GetUserSEXMLFullPath(U8FilePath& fullpath, cstr organization, cstr section);

	// Attempts to load $USER-DOCS/organization/section.sexml and provides a parser to decode the data in a callback
	// If organization is not provided the default is chosen.
	ROCOCO_SEXML_API void LoadUserSEXML(cstr organization, cstr section, Function<void(const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)> onLoad);

	// Tests to see if the UserSEXML file exists, and returns a value true if and only if it does exist
	// If organization is not provided the default is chosen.
	ROCOCO_SEXML_API [[nodiscard]] bool IsUserSEXMLExistant(cstr organization, cstr section);

	// Attempts to build an SEXML using the callback provided builder and then, if successful, saves the result to $USER-DOCS/organization/section.sexml
	// If organization is not provided the default is chosen.
	ROCOCO_SEXML_API void SaveUserSEXML(cstr organization, cstr section, Function<void(Rococo::Sex::SEXML::ISEXMLBuilder& builder)> onBuild);

	// Sets the default organization string. Must be definite
	ROCOCO_SEXML_API void SetDefaultOrganization(cstr defaultOrganization);

	ROCOCO_SEXML_API void LoadSXMLBySysPath(const wchar_t* filename, Function<void(const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)> onLoad);
	
	ROCOCO_SEXML_API void LoadSXMLBySysPath(cstr filename, Function<void(const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)> onLoad);

	ROCOCO_SEXML_API void SaveSXMLBySysPath(cstr filename, Function<void(Rococo::Sex::SEXML::ISEXMLBuilder& builder)> onBuild);

	ROCOCO_SEXML_API void SaveSXMLBySysPath(const wchar_t* filename, Function<void(Rococo::Sex::SEXML::ISEXMLBuilder& builder)> onBuild);
}