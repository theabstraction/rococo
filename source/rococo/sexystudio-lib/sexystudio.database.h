#pragma once

#include <rococo.types.h>

namespace Rococo
{
	enum class EQualifier;
}

namespace Rococo::Sex::Inference
{
	struct TypeInference;
}

namespace Rococo
{
	ROCOCO_INTERFACE ISexyFieldEnumerator;
}

namespace Rococo::SexyStudio
{
	DECLARE_ROCOCO_INTERFACE ISxyNamespace;
 
	SEXYSTUDIO_API void GetFullNamespaceName(char* fullName, size_t bufferCapacity, const ISxyNamespace& ns);

	struct SearchPathDescAtom
	{
		cstr pingPath;
		bool isActive;
	};

	ROCOCO_INTERFACE ISearchPaths
	{
		virtual SearchPathDescAtom GetSearchPath(size_t index) const = 0;
	};

	ROCOCO_INTERFACE IFactoryConfig : ISearchPaths
	{
		// The ping paths of the packages the project selects, e.g. !packages/mhost_1000.sxyz. Returns nullptr when index is beyond the last.
		// The database loads exactly these packages, see PopulateWithConfiguredPackages
		virtual cstr GetPackage(size_t index) const = 0;
		virtual cstr GetProjectPath() const = 0;
		virtual crwstr ConfigPath() const = 0;
		virtual void SetSearchPathActivity(size_t index, bool isActive) = 0;

		// Adds the package with the given ping path to the selection, or removes it. The IDE's package ticks edit the selection through this
		virtual void SetPackageSelected(cstr pingPath, bool isSelected) = 0;
	};

	ROCOCO_INTERFACE ISXYFile
	{

	};

	ROCOCO_INTERFACE ISXYArchetype
	{
		virtual cstr PublicName() const = 0;
		virtual int InputCount() const = 0;
		virtual int OutputCount() const = 0;
		virtual EQualifier InputQualifier(int index) const = 0;
		virtual cstr InputType(int index) const = 0;
		virtual cstr OutputType(int index) const = 0;
		virtual cstr InputName(int index) const = 0;
		virtual cstr OutputName(int index) const = 0;
		virtual cstr SourcePath() const = 0;
		virtual int LineNumber() const = 0;
	};

	ROCOCO_INTERFACE ISXYFunction : ISXYArchetype{};

	ROCOCO_INTERFACE ISXYFactory
	{
		virtual cstr PublicName() const = 0;
		virtual int InputCount() const = 0;
		virtual void GetDefinedInterface(char* buf, size_t capacity) const = 0;
		virtual cstr InputType(int index) const = 0;
		virtual cstr InputName(int index) const = 0;
		virtual int LineNumber() const = 0;
		virtual cstr SourcePath() const = 0;
	};

	ROCOCO_INTERFACE ISXYInterface
	{
		virtual cstr Base() const = 0;
		virtual cstr PublicName() const = 0;
		virtual int AttributeCount() const = 0;
		virtual int MethodCount() const = 0;
		virtual cstr GetAttribute(int index) const = 0;
		virtual ISXYFunction& GetMethod(int index) = 0;
		virtual const ISXYFunction& GetMethod(int index) const = 0;
		virtual cstr SourcePath() const = 0;
		virtual Sex::cr_sex GetDefinition() const = 0;
	};

	struct SXYMethodArgument
	{
		cstr type;
		cstr name;
	};

	struct SXYField
	{
		cstr type;
		cstr name;
	};

	ROCOCO_INTERFACE ISXYLocalType
	{
		virtual int FieldCount() const = 0;
		virtual SXYField GetField(int index) const = 0;
		virtual bool IsStrong() const = 0;
		virtual cstr LocalName() const = 0;
		virtual cstr SourcePath() const = 0;
		virtual int LineNumber() const = 0;
	};

	ROCOCO_INTERFACE ISXYEnum
	{
		virtual size_t NumberOfEntries() const = 0;
		virtual void GetEntry(size_t index, OUT cstr& name, OUT int64& value) const = 0;
	};

	ROCOCO_INTERFACE ISXYType
	{
		virtual cstr LocalName() const = 0;
		virtual cstr PublicName() const = 0;
		virtual ISXYLocalType* LocalType() = 0;
		virtual const ISXYLocalType* LocalType() const = 0;
	};

	ROCOCO_INTERFACE ISXYPublicFunction
	{
		virtual cstr PublicName() const = 0;
		virtual ISXYFunction* LocalFunction() = 0;
		virtual const ISXYFunction* LocalFunction() const = 0;
	};

	ROCOCO_INTERFACE IImplicitNamespaces
	{
		virtual int ImplicitCount() const = 0;
		virtual const ISxyNamespace& GetImplicitNamespace(int index) const = 0;

		// Try to match a fqName to a namespace and stores the implicit reference. If no match, returns false, else returns true.
		[[nodiscard]] virtual bool AddImplicitNamespace(cstr fqName) = 0;
		virtual void ClearImplicitNamespaces() = 0;
	};

	ROCOCO_INTERFACE ISxyNamespace
	{
		virtual int AliasCount() const = 0;
		virtual void AppendFullNameToStringBuilder(REF Strings::StringBuilder& sb) const = 0;
		virtual const ISXYPublicFunction* FindFunction(cstr shortName) const = 0;
		virtual const ISxyNamespace* FindSubspaceByShortName(cstr shortname) const = 0;
		virtual const ISxyNamespace* FindSubspace(cstr fqNamespace) const = 0;
		virtual cstr FindAliasFrom(cstr source) const = 0;
		virtual const ISXYType* FindType(Strings::cr_substring typeName) const = 0;
		virtual cstr GetNSAliasFrom(int index) const = 0;
		virtual	cstr GetNSAliasTo(int index) const = 0;
		virtual	cstr GetAliasSourcePath(int index) const = 0;
		virtual ISXYArchetype& GetArchetype(int index) = 0;
		virtual ISXYInterface& GetInterface(int index) = 0;
		virtual const ISXYInterface& GetInterface(int index) const = 0;
		virtual ISXYType& GetType(int index) = 0;
		virtual const ISXYType& GetType(int index) const = 0;
		virtual ISXYPublicFunction& GetFunction(int index) = 0;
		virtual const ISXYPublicFunction& GetFunction(int index) const = 0;
		virtual ISXYFactory& GetFactory(int index) = 0;
		virtual const ISXYFactory& GetFactory(int index) const = 0;
		virtual ISxyNamespace* GetParent() = 0;
		virtual const ISxyNamespace* GetParent() const = 0;
		virtual int ArchetypeCount() const = 0;
		virtual int FactoryCount() const = 0;
		virtual int FunctionCount() const = 0;
		virtual int InterfaceCount() const = 0;
		virtual int TypeCount() const = 0;
		virtual int SubspaceCount() const = 0;

		// The indexer retrieves the subspace at the position of the index
		virtual ISxyNamespace& operator[] (int index) = 0;

		// The indexer retrieves the subspace at the position of the index
		virtual const ISxyNamespace& operator[] (int index) const = 0;

		virtual cstr Name() const = 0;
		virtual ISxyNamespace& Update(cstr subspace, Sex::cr_sex src) = 0;
		virtual void UpdateArchetype(cstr name, Sex::cr_sex sDef, ISXYFile& file) = 0;
		virtual void UpdateFactory(cstr name, Sex::cr_sex sFactoryDef, ISXYFile& file) = 0;
		virtual void UpdateInterface(cstr name, Sex::cr_sex sInterfaceDef, ISXYFile& file) = 0;
		virtual void UpdateInterfaceViaDefinition(cstr name, Sex::cr_sex sInterfaceDef, ISXYFile& file) = 0;
		virtual void UpdateMacro(cstr name, Sex::cr_sex sMacroDef, ISXYFile& file) = 0;
		virtual void SortRecursive() = 0;
		virtual void AliasFunction(cstr localName, ISXYFile& file, cstr publicName) = 0;
		virtual void AliasStruct(cstr localName, ISXYFile& file, cstr publicName) = 0;
		virtual void AliasNSREf(cstr publicName, Sex::cr_sex sAliasDef, ISXYFile& file) = 0;
		virtual int NumberOfMacroEnums() const = 0;
		virtual cstr GetEnumMacroName(int index) const = 0;
		virtual cstr GetEnumMacroValue(int index) const = 0;
		virtual cstr GetEnumMacroSourcePath(int index) const = 0;
		virtual const Sex::ISExpression* FindMacroDefinition(cstr shortmacroName) const = 0;
		virtual int MacroCount() const = 0;
		virtual cstr GetMacroName(int index) const = 0;
		virtual cstr GetMacroSourcePath(int index) const = 0;
		virtual IImplicitNamespaces* ImplicitNamespaces() = 0;
		virtual const IImplicitNamespaces* ImplicitNamespaces() const = 0;
	};

	// Appends the fully qualified namespace of the [ns] argument to the string builder
	SEXYSTUDIO_API void AppendFullName(IN const ISxyNamespace& ns, REF struct Strings::StringBuilder& sb);


	ROCOCO_INTERFACE ISolution
	{
		virtual cstr GetContentFolder() const = 0;
		virtual cstr GetScriptFolder() const = 0;
		virtual cstr GetDeclarationPathForInclude(cstr includeName, int& priority) const = 0;
		virtual cstr GetDeclarationPathForImport(cstr packageName, int& priority) const = 0;
		virtual cstr GetPackagePingPath(cstr packageName) const = 0;
		virtual cstr GetPackageRoot() const = 0;
		virtual cstr GetPackageSourceFolder(cstr packagePath) const = 0;
		virtual void SetConfigDirectory(cstr sysConfigDirectory) = 0;
	};

	ROCOCO_INTERFACE ISexyDatabase
	{
		virtual IPingPathResolver& PingPathResolver() = 0;
		virtual bool AreTypesEquivalent(cstr a, cstr b) const = 0;
		virtual void Clear() = 0;
		virtual IFactoryConfig& Config() = 0;
		virtual bool EnumerateVariableAndFieldList(Strings::cr_substring variable, Strings::cr_substring type, ISexyFieldEnumerator& fieldEnumerator) = 0;
		virtual void EnumerateTemplateMethods(Strings::cr_substring variable, const Rococo::Sex::Inference::TypeInference& inference, ISexyFieldEnumerator& fieldEnumerator) = 0;
		virtual const ISXYEnum* FindEnum(cstr typeName) const = 0;
		virtual const ISXYType* FindFQType(cstr typeName) const = 0;
		virtual const ISXYType* FindPrimitiveOrFQType(cstr typeName) const = 0;
		virtual const ISXYInterface* FindInterface(cstr typeString, const ISxyNamespace** ppNamespace = nullptr) const = 0;
		virtual const ISXYPublicFunction* FindFunction(cstr fqFunctionName) const = 0;
		virtual const ISXYType* FindType(cstr typeName) const = 0;
		virtual void FocusProject(cstr projectFilePath) = 0;
		virtual void ForEachAutoCompleteCandidate(Strings::cr_substring prefix, ISexyFieldEnumerator& fieldEnumerator) = 0;
		virtual void ForEachAutoCompleteMacroCandidate(Strings::cr_substring prefix, ISexyFieldEnumerator& fieldEnumerator) = 0;
		virtual void GetHintForCandidate(Strings::cr_substring prefix, char args[1024]) = 0;
		virtual ISxyNamespace& GetRootNamespace() = 0;
		virtual bool HasResource(cstr id) const = 0;
		virtual void MarkResource(cstr id) = 0;
		virtual const ISXYLocalType* ResolveLocalType(cstr sourceFile, cstr localTypeName) const = 0;
		virtual void Sort() = 0;
		virtual void UpdateFile_SXY(cstr fullpathToSxy) = 0;
		virtual void UpdateFile_SXY_PackedItem(cstr data, int32 length, cstr path) = 0;
		// Clears the database, then loads every file under the active search paths and every package the config selects, then sorts. Calling it again reloads
		// the lot from disk rather than adding to what is there. A package that cannot be loaded is skipped and recorded, see GetPopulateError
		virtual void PopulateViaSearchPaths() = 0;
		virtual ISolution& Solution() = 0;

		// The number of errors the last PopulateViaSearchPaths recorded, such as a selected package that is missing or corrupt. Clear() resets it to zero
		[[nodiscard]] virtual size_t PopulateErrorCount() const = 0;

		// Returns the [index]th error recorded by the last PopulateViaSearchPaths, or nullptr if the index is out of range
		[[nodiscard]] virtual cstr GetPopulateError(size_t index) const = 0;

		// The number of source files, loose or packaged, this database has loaded since it was created. Clear() does not reset it.
		// It is a diagnostic, so tests can show that a file is parsed once and not again
		[[nodiscard]] virtual size_t LoadedFileCount() const = 0;
	};

	void BuildDatabaseFromProject(ISexyDatabase& database, Sex::cr_sex sProjectRoot, cstr projectPath, bool addDeclarations);

	SEXYSTUDIO_API void PopulateTreeWithPackage(cstr packageFolder, ISexyDatabase& database);

	ROCOCO_INTERFACE IPackageErrorHandler
	{
		virtual void OnPackageError(cstr pingPath, IException& ex) = 0;
	};

	// Loads every package that database.Config() selects (IFactoryConfig::GetPackage) into the database. A package already loaded since the last Clear() is
	// not loaded again. A package that cannot be loaded is skipped, and onError, if not null, is told why. Call Sort() afterwards to resolve what was loaded.
	// ISexyDatabase::PopulateViaSearchPaths calls this, recording the errors in the database, so a database populated without the IDE has the same packages the IDE would load
	SEXYSTUDIO_API void PopulateWithConfiguredPackages(ISexyDatabase& database, IPackageErrorHandler* onError);

	ROCOCO_INTERFACE ISexyDatabaseSupervisor : ISexyDatabase
	{
		virtual void Free() = 0;
		virtual void SetContentPath(cstr contentFolder) = 0;
	};

	SEXYSTUDIO_API ISexyDatabaseSupervisor* CreateSexyDatabase(IFactoryConfig& config);
}