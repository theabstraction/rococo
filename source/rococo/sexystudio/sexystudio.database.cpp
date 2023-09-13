#include "sexystudio.api.h"
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.strings.h>

#include <string>
#include <vector>
#include <rococo.hashtable.h>
#include <memory>
#include <algorithm>

#include <rococo.auto-release.h>

#include <rococo.io.h>
#include <rococo.os.h>

#include "rococo.sexystudio.api.h"

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::SexyStudio;
using namespace Rococo::Strings;

fstring pckPrefix = "[package]:"_fstring;

cstr AlwaysGetAtomic(cr_sex s)
{
	return IsAtomic(s) ? s.c_str() : "<expected atomic argument>";
}

cstr AlwaysGetAtomic(cr_sex s, int index)
{
	if (index < 0 || index >= s.NumberOfElements()) return "<invalid subexpression index>";
	return IsAtomic(s[index]) ? s[index].c_str() : "<expected atomic argument>";
}

int CountDots(cr_substring prefix)
{
	int dots = 0;

	for (auto p = prefix.start; p < prefix.finish; ++p)
	{
		char c = *p;

		switch (c)
		{
		case '(':
		case ')':
		case '\r':
		case '\n':
		case '\t':
			return dots;
		case '.':
			dots++;
			break;
		}
	}

	return dots;
}

namespace Rococo::SexyStudio
{
	ParseKeyword keywordNamespace("namespace");
	ParseKeyword keywordInterface("interface");
	ParseKeyword keywordStruct("struct");
	ParseKeyword keywordStrong("strong");
	ParseKeyword keywordFunction("function");
	ParseKeyword keywordMacro("macro");
	ParseKeyword keywordAlias("alias");
	ParseKeyword keywordFactory("factory");
	ParseKeyword keywordArchetype("archetype");
	ParseKeyword keywordClass("class");
	AtomicArg ParseAtomic;

	cstr FindDot(cstr s)
	{
		while (*s != '.' && *s != 0)
		{
			s++;
		}

		return s;
	}

	cr_sex GetElement(cr_sex s, int index)
	{
		return s[index];
	}

	int Len(cr_sex s)
	{
		return s.NumberOfElements();
	}

	bool AtomicArg::Matches(cr_sex sParent, int index) const
	{
		cr_sex s = sParent[index];
		return IsAtomic(s);
	}

	fstring AtomicArg::operator()(cr_sex sParent, int index) const
	{
		cr_sex s = sParent[index];
		return fstring{ s.c_str(), s.String()->Length };
	}

	ParseKeyword::ParseKeyword(cstr _keyword) : keyword(to_fstring(_keyword))
	{
	}

	bool ParseKeyword::Matches(cr_sex sParent, int index) const
	{
		cr_sex s = sParent[index];
		if (!IsAtomic(s)) return false;
		return Eq(s.c_str(), keyword);
	}

	fstring ParseKeyword::operator()(cr_sex s, int index) const
	{
		UNUSED(s);
		UNUSED(index);
		return keyword;
	}
}

namespace ANON
{
	struct SXYAttribute
	{
		const ISExpression* sAttribute;
		cstr GetName() const
		{
			return AlwaysGetAtomic(*sAttribute, 1);
		}
	};

	struct SXYMethod: ISXYFunction
	{
		const ISExpression* psMethod;
		cstr name = nullptr;
		int mapIndex = 0;
		int classOffset;
		int finalArg;
		sexstring cppSource = nullptr;
		int lineNumber = 0;

		int InputCount() const override
		{
			return mapIndex <= 0 ? 0 : mapIndex - 1 - classOffset;
		}

		int OutputCount() const override
		{
			return mapIndex <= 0 ? 0 : finalArg - mapIndex;
		}

		cstr SourcePath() const override
		{
			return cppSource ? cppSource->Buffer : "<no path>";
		}

		int LineNumber() const override
		{
			return lineNumber;
		}

		cstr InputType(int index) const override
		{
			cr_sex sArg = psMethod->GetElement(index + 1 + classOffset);
			return AlwaysGetAtomic(sArg, 0);
		}

		cstr OutputType(int index) const override
		{
			cr_sex sArg = psMethod->GetElement(index + mapIndex + 1);
			return AlwaysGetAtomic(sArg, 0);
		}

		cstr InputName(int index) const override
		{
			cr_sex sArg = psMethod->GetElement(index + 1 + classOffset);
			return AlwaysGetAtomic(sArg, 1);
		}

		cstr OutputName(int index) const override
		{
			cr_sex sArg = psMethod->GetElement(index + mapIndex + 1);
			return AlwaysGetAtomic(sArg, 1);
		}

		cstr PublicName() const
		{
			return name;
		}
		
		SXYMethod(cr_sex _sMethod, bool isClassMethod = false) : psMethod(&_sMethod), classOffset(isClassMethod ? 1 : 0),
			finalArg(0)
		{
			// Assumes s_Method.NumberOfElements >= 2 and s_Method[0] is atomic
			// Example: (AppendPrefixAndGetName (IString prefix)->(IString name))

			name = AlwaysGetAtomic(_sMethod, classOffset);

			if (isClassMethod)
			{
				name = FindDot(name) + 1;
			}

			for (int i = 1 + classOffset; i < _sMethod.NumberOfElements(); ++i)
			{
				if (IsAtomic(_sMethod[i]))
				{
					cstr arg = _sMethod[i].c_str();
					if (Eq(arg, "->"))
					{
						mapIndex = i;
						break;
					}
				}
			}

			finalArg = _sMethod.NumberOfElements() - 1;

			if (mapIndex)
			{
				if (isClassMethod)
				{
					for (int i = mapIndex + 1; i < _sMethod.NumberOfElements(); ++i)
					{
						if (IsAtomic(_sMethod[i]))
						{
							cstr arg = _sMethod[i].c_str();
							if (Eq(arg, ":"))
							{
								finalArg = i - 1;
								break;
							}
						}
					}
				}
				else
				{
					finalArg = _sMethod.NumberOfElements() - 1;
				}
			}

			if (mapIndex)
			{
				return;
			}

			// Check to see if there is an expression (source "<path>" <line-number) in the last element of the function

			cr_sex sFinalExpression = _sMethod[_sMethod.NumberOfElements() - 1];
			if (sFinalExpression.NumberOfElements() != 3)
			{
				return;
			}

			cr_sex sFinalDirective = sFinalExpression[0];
			if (!IsAtomic(sFinalDirective))
			{
				return;
			}

			if (!Eq(sFinalDirective.c_str(), "source"))
			{
				return;
			}

			cr_sex sSourceFile = sFinalExpression[1];
			if (!IsStringLiteral(sSourceFile))
			{
				return;
			}

			cppSource = sSourceFile.String();

			cr_sex sLineNumber = sFinalExpression[2];
			if (!IsAtomic(sLineNumber))
			{
				return;
			}

			lineNumber = atoi(sLineNumber.c_str());
		}
	};

	void ResolvePackageName(cstr fqName, char* buf, size_t capacity, cr_sex s)
	{
		// fqName in this context has form $.<name>, where $ indicated the default namespace generated by the resource path.

		cstr packagePath = s.Tree().Source().Name();
		if (Compare(packagePath, pckPrefix, pckPrefix.length) == 0)
		{
			// Source directory has the form [package]:/MHost/Mhost/Graphics/Renderer.sxy
			cstr appendix = packagePath + pckPrefix.length;
			char* p = buf;
			char* end = buf + capacity;

			char* finalSlash = nullptr;

			for (cstr c = appendix; *c != 0 && p < end; ++c, p++)
			{
				if (*c == '/')
				{
					finalSlash = p;
					*p = '.';
				}
				else
				{
					*p = *c;
				}
			}

			if (finalSlash)
			{
				*finalSlash = 0; // That strips the file name
				// Now if we have space, append the .<name>
				SafeFormat(finalSlash, end - finalSlash, "%s", fqName + 1);
			}
			else
			{
				SafeFormat(buf, capacity, "%s defined by %s", fqName, packagePath);
			}
		}
		else
		{
			SafeFormat(buf, capacity, "%s defined by %s", fqName, packagePath);
		}
	}

	struct SxyFactory : ISXYFactory
	{
		SxyFactory(cstr name, ISXYFile& _source_file, cr_sex _sDef) :
			shortName(name), source_file(&_source_file), sDef(&_sDef), bodyIndicator(0)
		{
			// Factory expressions have the form (factory <name> <defined-interface> (arg1type arg1name) ...(argNtype argNname): ...)
			for (int i = 3; i < _sDef.NumberOfElements(); ++i)
			{
				if (Eq(AlwaysGetAtomic(_sDef, i), ":"))
				{
					bodyIndicator = i;
					break;
				}
			}

			if (!bodyIndicator) bodyIndicator = 3; // This is an error, but it is not currently the job of sexystudio to provide correction suggestions
		}

		cstr SourcePath() const override
		{
			return sDef->Tree().Source().Name();
		}

		int LineNumber() const override
		{
			return sDef->Start().y;
		}

		cstr PublicName() const override
		{
			return shortName.c_str();
		}

		int InputCount() const override
		{
			return bodyIndicator - 3;
		}

		void GetDefinedInterface(char* buf, size_t capacity) const override
		{
			cstr fqInterfaceName = AlwaysGetAtomic(*sDef, 2);
			if (Compare(fqInterfaceName, "$.", 2) == 0)
			{
				ResolvePackageName(fqInterfaceName, buf, capacity, *sDef);
			}
			else
			{
				SafeFormat(buf, capacity, "%s", fqInterfaceName);
			}
		}

		cstr InputType(int index) const override
		{
			int i = 3 + index;
			cr_sex sInput = sDef->GetElement(i);
			return AlwaysGetAtomic(sInput, 0);
		}

		cstr InputName(int index) const  override
		{
			int i = 3 + index;
			cr_sex sInput = sDef->GetElement(i);
			return AlwaysGetAtomic(sInput, 1);
		}

		std::string shortName;
		ISXYFile* source_file;
		int bodyIndicator;
		const ISExpression* sDef;
	};

	struct SxyInterface: ISXYInterface
	{
		SxyInterface(cstr name, ISXYFile& _source_file, cr_sex _sInterfaceDef):
			shortName(name), source_file(&_source_file), sInterfaceDef(&_sInterfaceDef)
		{

		}

		cstr SourcePath() const override
		{
			return sInterfaceDef->Tree().Source().Name();
		}

		int AttributeCount() const override
		{
			return (int)attributes.size();
		}

		cstr GetAttribute(int index) const
		{
			return attributes[index].GetName();
		}

		cr_sex GetDefinition() const
		{
			return *sInterfaceDef;
		}

		int MethodCount() const override
		{
			return (int) methods.size();
		}

		ISXYFunction& GetMethod(int index) override
		{
			return methods[index];
		}

		cstr PublicName() const override
		{
			return shortName.c_str();
		}

		std::string shortName;
		ISXYFile* source_file;
		const ISExpression* sInterfaceDef;
		std::vector<SXYAttribute> attributes;
		const ISExpression* base = nullptr;
		cstr Base() const override
		{
			return base ? AlwaysGetAtomic(*base, 1) : nullptr;
		}
		std::vector<SXYMethod> methods;
	};

	struct SXYMacro
	{
		std::string shortName;
		ISXYFile& source_file;
		cr_sex sMacroDef;
		
		cstr GetMacroValue() const
		{
			// An enum looks like this (macro #Sys.IO.MaxFilePathLen in out (out.AddAtomic "0x102"))

			if (sMacroDef.NumberOfElements() == 5)
			{
				cr_sex sDirective = sMacroDef[4]; // (out.AddAtomic "0x102"))
				if (sDirective.NumberOfElements() == 2)
				{
					if (IsAtomic(sDirective[0]) && IsStringLiteral(sDirective[1]))
					{
						cstr s0 = sDirective[0].c_str();
						cstr s1 = sDirective[1].c_str();

						if (Eq(s0, "out.AddAtomic") && Compare(s1, "0x", 2) == 0)
						{
							return s1;
						}
					}
				}
			}

			return nullptr;
		}

	};

	struct SXYPublicFunction: ISXYPublicFunction
	{
		SXYPublicFunction(cstr _publicName, cstr _localName, ISXYFile& _file) :
			publicName(_publicName), localName(_localName), file(&_file)
		{

		}

		cstr PublicName() const override
		{
			return publicName.c_str();
		}

		std::string publicName;
		std::string localName;
		ISXYFile* file;

		ISXYFunction* localFunction = nullptr;

		ISXYFunction* LocalFunction() override
		{
			return localFunction;
		}
	};

	struct SXYStruct: ISXYLocalType
	{
		cr_sex sStructDef;
		int numberOfFields = 0;

		int FieldCount() const override { return numberOfFields; }

		SXYStruct(cr_sex _sStructDef) : sStructDef(_sStructDef)
		{
			// Assumes _StructDef.NumberOfElements > 0
			for (int i = 2; i < _sStructDef.NumberOfElements(); ++i)
			{
				cr_sex sFieldDef = _sStructDef[i];
				if (sFieldDef.NumberOfElements() > 1)
				{
					numberOfFields += sFieldDef.NumberOfElements() - 1;
				}
			}
		}

		bool IsStrong() const override
		{
			return Eq(sStructDef[0].c_str(), "strong");
		}

		cstr LocalName() const override
		{
			return sStructDef[1].c_str();
		}

		cstr SourcePath() const override
		{
			return sStructDef.Tree().Source().Name();
		}

		int LineNumber() const override
		{
			return sStructDef.Start().y;
		}

		/* Example:
		(struct Quaternion
			(Float32 scalar)
			(Float32 vi vj vk)
		)
		*/
		SXYField GetField(int index) const override
		{
			if (Eq(sStructDef[0].c_str(), "strong"))
			{
				cr_sex innerValue = sStructDef[2];
				if (innerValue.NumberOfElements() == 1)
				{
					cr_sex innerAtomic = innerValue[0];
					if (IsAtomic(innerAtomic))
					{
						return { innerAtomic.c_str(), "value" };
					}
				}

				return { nullptr,nullptr };
			}

			if (index < 0 || index >= numberOfFields)
			{
				return SXYField{ nullptr, nullptr };
			}

			int fieldCount = 0;

			for (int i = 2; i < sStructDef.NumberOfElements(); ++i)
			{
				cr_sex sFieldDef = sStructDef[i];
				int nFieldsInExpression = sFieldDef.NumberOfElements() - 1;
				if (index < fieldCount + nFieldsInExpression)
				{
					cstr type = AlwaysGetAtomic(sFieldDef, 0);
					cstr name = AlwaysGetAtomic(sFieldDef, index - fieldCount + 1);
					return SXYField{ type,name };
				}
				else
				{
					fieldCount += nFieldsInExpression;
				}
			}

			return { nullptr,nullptr };
		}
	};

	struct SXYPublicStruct: ISXYType
	{
		std::string publicName;
		std::string localName;
		ISXYFile& file;
		ISXYLocalType* localType = nullptr;

		SXYPublicStruct(cstr _publicName, cstr _localName, ISXYFile& _file):
			publicName(_publicName), localName(_localName), file(_file)
		{

		}

		cstr PublicName() const override
		{
			return publicName.c_str();
		}

		ISXYLocalType* LocalType() override
		{
			return localType;
		}
	};

	struct SXYNSAlias
	{
		std::string publicName;
		sexstring fqName;
		cr_sex sAliasDef;
		ISXYFile& file;
	};

	// archetype expressions have the format (archetype <FQ_NAME> (arg-type1 arg-name1)...(arg-typeN arg-nameN)->(out-type1 out-name1)...(out-typeN out-nameN))
	struct SXYArchetype : ISXYArchetype
	{
		int mappingIndex = 0;

		SXYArchetype(cstr _publicName, ISXYFile& _file, cr_sex _sDef) :
			publicName(_publicName), sDef(_sDef), file(_file)
		{
			for (int i = 2; i < sDef.NumberOfElements(); ++i)
			{
				if (Eq(AlwaysGetAtomic(sDef, i), "->"))
				{
					mappingIndex = i;
					break;
				}
			}
		}

		cstr PublicName() const override
		{
			return publicName.c_str();
		}

		int InputCount() const override
		{
			return mappingIndex > 0 ? mappingIndex - 2 : 0;
		}

		int OutputCount() const override
		{
			return mappingIndex > 0 ? sDef.NumberOfElements() - mappingIndex - 1 : 0;
		}

		cstr InputType(int index) const override
		{
			auto& sArg = sDef[index + 2];
			return AlwaysGetAtomic(sArg, 0);
		}

		cstr OutputType(int index) const override
		{
			auto& sArg = sDef[mappingIndex + index + 1];
			return AlwaysGetAtomic(sArg, 0);
		}

		cstr InputName(int index) const override
		{
			auto& sArg = sDef[index + 2];
			return AlwaysGetAtomic(sArg, 1);
		}

		cstr OutputName(int index) const override
		{
			auto& sArg = sDef[mappingIndex + index + 1];
			return AlwaysGetAtomic(sArg, 1);
		}

		cstr SourcePath() const override
		{
			return sDef.Tree().Source().Name();
		}

		int LineNumber() const override
		{
			return sDef.Start().y;
		}

		cr_sex sDef;
		std::string publicName;
		ISXYFile& file;
	};


	struct SxyNamespace : ISxyNamespace
	{
		std::string name;
		std::vector<std::unique_ptr<SxyNamespace>> subspaces;
		std::vector<SxyFactory> factories;
		std::vector<SxyInterface> interfaces;
		std::vector<SXYMacro> macros;
		std::vector<SXYMacro> enums;
		std::vector<SXYPublicFunction> functions;
		std::vector<SXYPublicStruct> structures;
		std::vector<SXYNSAlias> nsAlias;
		std::vector<SXYArchetype> archetypes;
		SxyNamespace* parent;

		SxyNamespace(cstr _name, SxyNamespace* _parent) : name(_name), parent(_parent) {}

		int AliasCount() const override
		{
			return (int) nsAlias.size();
		}

		cstr GetNSAliasFrom(int index) const override
		{
			return AlwaysGetAtomic(nsAlias[index].sAliasDef, 1);
		}

		cstr GetNSAliasTo(int index) const override
		{
			return nsAlias[index].publicName.c_str();
		}

		ISxyNamespace* GetParent() override
		{
			return parent;
		}

		cstr GetAliasSourcePath(int index) const override
		{
			return nsAlias[index].sAliasDef.Tree().Source().Name();
		}

		ISXYFactory& GetFactory(int index) override
		{
			return factories[index];
		}

		int FactoryCount() const override
		{
			return (int) factories.size();
		}

		int EnumCount() const override
		{
			return (int)enums.size();
		}

		cstr GetEnumName(int index) const override
		{
			return enums[index].shortName.c_str();
		}

		cstr GetEnumValue(int index) const override
		{
			return enums[index].GetMacroValue();
		}

		cstr GetEnumSourcePath(int index) const override
		{
			return enums[index].sMacroDef.Tree().Source().Name();
		}

		int SubspaceCount() const override
		{
			return (int) subspaces.size();
		}

		int FunctionCount() const override
		{
			return (int)functions.size();
		}

		ISXYArchetype& GetArchetype(int index) override
		{
			return archetypes[index];
		}

		int ArchetypeCount() const override
		{
			return (int) archetypes.size();
		}

		ISXYPublicFunction& GetFunction(int index) override
		{
			return functions[index];
		}

		int InterfaceCount() const override
		{
			return (int) interfaces.size();
		}

		int TypeCount() const override
		{
			return (int)structures.size();
		}

		ISXYType& GetType(int index) override
		{
			return structures[index];
		}

		ISXYInterface& GetInterface(int index) override
		{
			return interfaces[index];
		}

		ISxyNamespace& operator[] (int index) override
		{
			if (index < 0 || index >= (int)subspaces.size())
			{
				Throw(0, "Bad namespace index");
			}

			return *subspaces[index];
		}

		cstr Name() override
		{
			return name.c_str();
		}

		// Creates or updates namespace tree by breaking a path such as MHost/Widgets into MHost & Widgets
		ISxyNamespace& MapPathToNamespace(cstr path)
		{
			cstr s = path;
			while (*s != '/' && *s != 0)
			{
				s++;
			}

			if (*s == 0)
			{
				if (EndsWith(path, ".sxy"))
				{
					return *this;
				}

				for (auto& sub : subspaces)
				{
					if (Eq(sub->Name(), path))
					{
						return *sub;
					}
				}

				subspaces.push_back(std::make_unique<SxyNamespace>(path, this));
				return *subspaces[subspaces.size() - 1];
			}
			else
			{
				char* prefix = (char*)alloca(s - path + 1);
				memcpy(prefix, path, s - path);
				prefix[s - path] = 0;

				for (auto& sub : subspaces)
				{
					if (Eq(sub->Name(), prefix))
					{
						return sub->MapPathToNamespace(s + 1);
					}
				}

				subspaces.push_back(std::make_unique<SxyNamespace>(prefix, this));
				return subspaces.back()->MapPathToNamespace(s + 1);
			}
		}

		ISxyNamespace& Update(cstr subspace, cr_sex src) override
		{
			if (Eq(subspace, "$"))
			{
				// The dollar indicates we generate the namespace from the file path
				cstr filename = src.Tree().Source().Name();

				if (Compare(filename, pckPrefix, pckPrefix.length) == 0)
				{
					filename += pckPrefix.length;
				}

				return MapPathToNamespace(filename);
			}

			for (auto& s : subspaces)
			{
				if (Eq(s->Name(), subspace))
				{
					return *s;
				}
			}

			subspaces.push_back(std::make_unique<SxyNamespace>(subspace, this));
			return *subspaces[subspaces.size() - 1];
		}

		void UpdateArchetype(cstr name, cr_sex sArchetypeDef, ISXYFile& file) override
		{
			archetypes.push_back({ name, file, sArchetypeDef });
		}

		void UpdateFactory(cstr name, cr_sex sFactoryDef, ISXYFile& file) override
		{
			factories.push_back({ name, file, sFactoryDef });
		}

		void UpdateInterface(cstr name, cr_sex sInterfaceDef, ISXYFile& file) override
		{
			interfaces.push_back( { name, file, sInterfaceDef } );
			auto& interf = interfaces.back();
			for (int i = 2; i < sInterfaceDef.NumberOfElements(); ++i)
			{
				auto& sChild = sInterfaceDef[i];
				if (sInterfaceDef.NumberOfElements() >= 2)
				{
					cstr child = AlwaysGetAtomic(sChild, 0);
					if (IsCapital(child[0]))
					{
						// methods have capital letters
						// example (AppendPrefixAndGetName (IString prefix)->(IString name))
						interf.methods.push_back(sChild);
					}
					else if (Eq(child, "attribute"))
					{
						interf.attributes.push_back( { &sChild  } );
					}
					else if (Eq(child, "extends"))
					{
						// base interface
						interf.base = &sChild;
					}
				}
			}

			std::sort(interf.methods.begin(), interf.methods.end(), 
				[](const SXYMethod& a, const SXYMethod& b)
				{
					return Compare(a.name, b.name) < 0;
				}
			);
		}

		void UpdateInterfaceViaDefinition(cstr interfacePublicName, cr_sex sClassDef, ISXYFile& file) override
		{
			interfaces.push_back({ interfacePublicName, file, sClassDef });

			sexstring className = sClassDef[1].String();

			auto& interf = interfaces.back();
			for (int i = 2; i < sClassDef.NumberOfElements(); ++i)
			{
				auto& sChild = sClassDef[i];
				if (sClassDef.NumberOfElements() >= 2)
				{
					cstr child = AlwaysGetAtomic(sChild, 0);
					if (Eq(child, "attribute"))
					{
						interf.attributes.push_back({ &sChild });
					}
					else if (Eq(child, "extends"))
					{
						// base interface
						interf.base = &sChild;
					}
				}
			}

			cr_sex sRoot = sClassDef.Tree().Root();
			for (int i = 0; i < sRoot.NumberOfElements(); ++i)
			{
				cr_sex sMethod = sRoot[i];
				if (sMethod.NumberOfElements() >= 4)
				{
					// methods are expressed thus: (method <class-name>.<method-name> [inputs] -> [outputs]: [body])
					if (Eq(AlwaysGetAtomic(sMethod, 0), "method"))
					{
						if (IsAtomic(sMethod[1]))
						{
							sexstring classAndMethod = sMethod[1].String();

							auto* dot = FindDot(classAndMethod->Buffer);
							if (*dot == '.')
							{
								if (dot - classAndMethod->Buffer == className->Length)
								{
									if (Compare(classAndMethod->Buffer, className->Buffer, className->Length) == 0)
									{
										// We have a class match for <class-name>.<method-name>
										cstr methodName = dot + 1;
										if (*methodName && !Eq(methodName, "Construct") && !Eq(methodName, "Destruct"))
										{
											interf.methods.push_back({ sMethod, true });
										}
									}
								}
							}
						}
					}
				}
			}

			std::sort(interf.methods.begin(), interf.methods.end(),
				[](const SXYMethod& a, const SXYMethod& b)
				{
					return Compare(a.name, b.name) < 0;
				}
			);
		}

		void UpdateMacro(cstr name, cr_sex sMacroDef, ISXYFile& file) override
		{
			SXYMacro macro { name, file, sMacroDef };
			if (macro.GetMacroValue() == nullptr)
			{
				macros.push_back(macro);
			}
			else
			{
				enums.push_back(macro);
			}
		}

		void SortRecursive() override
		{
			std::sort(subspaces.begin(), subspaces.end(),
				[](const std::unique_ptr<SxyNamespace>& a, const std::unique_ptr<SxyNamespace>& b)->bool
				{
					return a->name < b->name;
				}
			);

			std::sort(functions.begin(), functions.end(),
				[](const SXYPublicFunction& a, const SXYPublicFunction& b)->bool
				{
					return Compare(a.PublicName(), b.PublicName()) < 0;
				}
			);
			
			std::sort(factories.begin(), factories.end(),
				[](const SxyFactory& a, const SxyFactory& b)->bool
				{
					return Compare(a.PublicName(), b.PublicName()) < 0;
				}
			);

			std::sort(interfaces.begin(), interfaces.end(),
				[](const SxyInterface& a, const SxyInterface& b)->bool
				{
					return Compare(a.PublicName(), b.PublicName()) < 0;
				}
			);

			for (auto& subspace : subspaces)
			{
				subspace->SortRecursive();
			}
		}

		void AliasFunction(cstr localName, ISXYFile& file, cstr publicName) override
		{
			functions.push_back({publicName, localName, file});
		}

		void AliasStruct(cstr localName, ISXYFile& file, cstr publicName) override
		{
			structures.push_back({ publicName, localName, file });
		}

		void AliasNSREf(cstr publicName, cr_sex sAliasDef, ISXYFile& file) override
		{
			nsAlias.push_back({ publicName, sAliasDef[2].String(), sAliasDef, file});
		}
	};

	struct SXYFunction: ISXYFunction
	{
		cr_sex sFunction;
		cstr name = nullptr;
		int mapIndex = -1;
		int bodyIndex = -1;
		sexstring cpp_source = nullptr;
		int lineNumber = 0;

		cstr SourcePath() const override
		{
			return cpp_source ? cpp_source->Buffer : sFunction.Tree().Source().Name();
		}

		int LineNumber() const override
		{
			return lineNumber;
		}

		cstr PublicName() const override
		{
			return name;
		}

		int InputCount() const override
		{
			return mapIndex < 0 ? 0 : mapIndex - 2;
		}

		int OutputCount() const override
		{
			return bodyIndex - mapIndex - 1;
		}

		cstr InputType(int index) const override
		{
			cr_sex sArg = sFunction.GetElement(index + 2);
			return AlwaysGetAtomic(sArg, 0);
		}

		cstr OutputType(int index) const override
		{
			cr_sex sArg = sFunction.GetElement(index + mapIndex + 1);
			return AlwaysGetAtomic(sArg, 0);
		}

		cstr InputName(int index) const override
		{
			cr_sex sArg = sFunction.GetElement(index + 2);
			return AlwaysGetAtomic(sArg, 1);
		}

		cstr OutputName(int index) const override
		{
			cr_sex sArg = sFunction.GetElement(index + mapIndex + 1);
			return AlwaysGetAtomic(sArg, 1);
		}

		SXYFunction(cr_sex _sFunction) : sFunction(_sFunction)
		{
			name = AlwaysGetAtomic(_sFunction, 1);

			lineNumber = _sFunction.Start().y;

			for (int i = 1; i < _sFunction.NumberOfElements(); ++i)
			{
				if (IsAtomic(_sFunction[i]))
				{
					cstr arg = _sFunction[i].c_str();
					if (mapIndex < 0)
					{
						if (Eq(arg, "->"))
						{
							mapIndex = i;
						}
					}
					else // map index defined and the next legal atomic argument is ':' the body indicator
					{
						if (Eq(arg, ":"))
						{
							bodyIndex = i;
							break;
						}
					}
				}
			}

			if (bodyIndex == -1)
			{
				return;
			}

			// Check to see if there is an expression (source "<path>" <line-number) in the last element of the function

			cr_sex sFinalExpression = _sFunction[_sFunction.NumberOfElements() - 1];
			if (sFinalExpression.NumberOfElements() != 3)
			{
				return;
			}

			cr_sex sFinalDirective = sFinalExpression[0];
			if (!IsAtomic(sFinalDirective))
			{
				return;
			}

			if (!Eq(sFinalDirective.c_str(), "source"))
			{
				return;
			}

			cr_sex sSourceFile = sFinalExpression[1];
			if (!IsStringLiteral(sSourceFile))
			{
				return;
			}

			cpp_source = sSourceFile.String();

			cr_sex sLineNumber = sFinalExpression[2];
			if (!IsAtomic(sLineNumber))
			{
				return;
			}

			lineNumber = atoi(sLineNumber.c_str());
		}

		int GetBeginInputIndex() const
		{
			return 2;
		}

		int GetEndIndexIndex() const
		{
			return mapIndex < 0 ? 1 : mapIndex;
		}

		int GetBeginOutputIndex() const
		{
			return mapIndex + 1;
		}

		int GetEndOutputIndex() const
		{
			return mapIndex < 0 ? 0 : bodyIndex;
		}

		SXYMethodArgument GetArg(int index) const
		{
			auto& sArg = sFunction[index];
			if (sArg.NumberOfElements() == 2)
			{
				if (IsAtomic(sArg[0]) && IsAtomic(sArg[1]))
				{
					return { sArg[0].c_str(), sArg[1].c_str() };
				}
			}

			return { nullptr, nullptr };
		}
	};

	struct File_SXY: ISXYFile
	{
		File_SXY(const std::string& _filename): filename(_filename)
		{

		}

		stringmap<SXYStruct> structures;
		stringmap<SXYFunction> functions;

		int errorCode = 0;
		std::string errorMessage;
		uint64 fileLength = 0;
		std::string filename;
		AutoRelease<ISParserTree> s_tree;
	};

	void CopyFinalNameToBuffer(char* buffer, size_t capacity, cstr fqName)
	{
		cstr s = fqName;
		while (*s != 0) s++;

		while (s > fqName)
		{
			if (*s == '.')
			{
				strcpy_s(buffer, capacity, s + 1);
				return;
			}

			s--;
		}

		if (buffer && capacity) *buffer = 0;
	}

	struct DeclarationAssociation
	{
		HString key;
		HString value;
		bool isImport;
	};

	struct SolutionFile: ISolution
	{
		std::vector<DeclarationAssociation> declarationAssocations;
		stringmap<HString> packages;
		stringmap<HString> mapPrefixToPackageSource;
		U8FilePath contentPath;
		U8FilePath packageRoot;
		U8FilePath scriptPath;

		cstr GetPackageRoot() const override
		{
			return packageRoot;
		}

		cstr GetContentFolder() const override
		{
			return contentPath;
		}

		cstr GetScriptFolder() const override
		{
			return scriptPath;
		}

		void SetContentFolder(cstr path) override
		{
			Format(contentPath, "%s", path);
			Format(packageRoot, "%s", path);
			Format(scriptPath, "%sscripts\\", path);
			Rococo::IO::MakeContainerDirectory(packageRoot.buf);
			StringCat(packageRoot.buf, "packages\\", U8FilePath::CAPACITY);
		}

		void ParseSolution(cr_sex sRoot)
		{
			declarationAssocations.clear();
			mapPrefixToPackageSource.clear();
			packages.clear();

			for (int i = 0; i < sRoot.NumberOfElements(); ++i)
			{
				cr_sex sDirective = sRoot[i];
				if (sDirective.NumberOfElements() == 4)
				{
					cr_sex sCommand = GetAtomicArg(sDirective, 0);
					cstr cmd = sCommand.c_str();
					if (Eq(cmd, "included"))
					{
						cr_sex sKey = sDirective[1];
						if (!IsStringLiteral(sKey))
						{
							Throw(sKey, "Expecting string literal - <include-path>");
						}

						cr_sex sMap = sDirective[2];
						if (!IsAtomic(sMap))
						{
							Throw(sMap, "Expecting map operator ->");
						}

						cr_sex sValue = sDirective[3];
						if (!IsStringLiteral(sValue))
						{
							Throw(sKey, "Expecting string literal - <declaration-path>");
						}

						declarationAssocations.push_back({ sKey.c_str(), sValue.c_str(), false });
						continue;
					}
					else if (Eq(cmd, "imported"))
					{
						cr_sex sKey = sDirective[1];
						if (!IsAtomic(sKey))
						{
							Throw(sKey, "Expecting atomic - <import-package-name>");
						}

						cr_sex sMap = sDirective[2];
						if (!IsAtomic(sMap))
						{
							Throw(sMap, "Expecting map operator ->");
						}

						cr_sex sValue = sDirective[3];
						if (!IsStringLiteral(sValue))
						{
							Throw(sKey, "Expecting string literal - <declaration-path>");
						}

						declarationAssocations.push_back({ sKey.c_str(), sValue.c_str(), true });
						continue;
					}
					else if (Eq(cmd, "package"))
					{
						cr_sex sKey = sDirective[1];
						if (!IsAtomic(sKey))
						{
							Throw(sKey, "Expecting atomic - <import-package-name>");
						}

						cr_sex sMap = sDirective[2];
						if (!IsAtomic(sMap))
						{
							Throw(sMap, "Expecting map operator ->");
						}

						cr_sex sValue = sDirective[3];
						if (!IsStringLiteral(sValue))
						{
							Throw(sKey, "Expecting string literal - <package-ping-path>");
						}

						packages.insert(sKey.c_str(), sValue.c_str());
						continue;
					}
					else if (Eq(cmd, "map-prefix-to-source"))
					{
						cr_sex sKey = sDirective[1];
						if (!IsStringLiteral(sKey))
						{
							Throw(sKey, "Expecting string literal - <file-name-prefix>");
						}

						cr_sex sMap = sDirective[2];
						if (!IsAtomic(sMap))
						{
							Throw(sMap, "Expecting map operator ->");
						}

						cr_sex sValue = sDirective[3];
						if (!IsStringLiteral(sValue))
						{
							Throw(sValue, "Expecting string literal - <source-folder>");
						}

						if (!EndsWith(sValue.c_str(), "\\"))
						{
							Throw(sValue, "Expecting <source-folder> to end with backslash '\\'");
						}

						mapPrefixToPackageSource.insert(sKey.c_str(), sValue.c_str());
						continue;
					}
					else if (!Eq(cmd, "'"))
					{
						// Raw literal
						continue;
					}

					Throw(sDirective, "Unknown directive");
				}
			}
		}

		cstr GetDeclarationPathForInclude(cstr includeName, int& priority) const override
		{
			priority = 0;
			for (auto& a : declarationAssocations)
			{
				if (!a.isImport && Eq(includeName, a.key.c_str()))
				{
					return a.value.c_str();
				}

				priority++;
			}

			return nullptr;
		}

		cstr GetDeclarationPathForImport(cstr packageName, int& priority) const override
		{
			priority = 0;
			for (auto& a : declarationAssocations)
			{
				if (a.isImport && Eq(packageName, a.key.c_str()))
				{
					return a.value.c_str();
				}

				priority++;
			}

			return nullptr;
		}

		cstr GetPackagePingPath(cstr packageName) const override
		{
			auto i = packages.find(packageName);
			return i != packages.end() ? i->second.c_str() : nullptr;
		}

		cstr GetPackageSourceFolder(cstr packagePath) const override
		{
			for (auto i : mapPrefixToPackageSource)
			{
				if (StartsWith(packagePath, i.first))
				{
					return i.second;
				}
			}

			return nullptr;
		}
	};

	struct SexyDatabase : ISexyDatabaseSupervisor
	{
		IFactoryConfig& config;
		AutoRelease<ISParser> sparser;
		ANON::SxyNamespace rootNS;
		std::unordered_map<std::string, std::unique_ptr<File_SXY>> filenameToFile;

		AutoFree<IO::IOSSupervisor> os = IO::GetIOS();
		AutoFree<IO::IInstallationSupervisor> installation;

		SolutionFile solutionFile;

		stringmap<int> mapCppCodeToUsage;
		stringmap<int> resourceCount;

		SexyDatabase(IFactoryConfig& _config): 
			config(_config),
			sparser(Rococo::Sex::CreateSexParser_2_0(Rococo::Memory::CheckedAllocator())),
			rootNS("", nullptr)
		{

		}

		~SexyDatabase()
		{
			
		}

		bool HasResource(cstr id) const override
		{
			return resourceCount.find(id) != resourceCount.end();
		}

		void MarkResource(cstr id) override
		{
			auto i = resourceCount.insert(id, 0);
			i.first->second++;
		}

		IFactoryConfig& Config() override
		{
			return config;
		}

		ISolution& Solution()
		{
			return solutionFile;
		}

		void SetContentPath(cstr contentFolder) override
		{
			solutionFile.SetContentFolder(contentFolder);

			WideFilePath wContentFolder;
			Assign(wContentFolder, contentFolder);
		
			installation = CreateInstallationDirect(wContentFolder, *os);

			WideFilePath associationPath;
			installation->ConvertPingPathToSysPath("!scripts/declarations/sys/sexystudio.solution.sxyq", associationPath);

			AutoRelease<ISourceCode> src = sparser->LoadSource(associationPath, { 1,1 });
			AutoRelease<ISParserTree> tree;

			try
			{
				tree = sparser->CreateTree(*src);
				solutionFile.ParseSolution(tree->Root());
			}
			catch (ParseException& ex)
			{
				Throw(ex.ErrorCode(), "%ls error %s line column %d line %d", associationPath.buf, ex.Message(), ex.Start().x, ex.Start().y);
			}
		}

		void PingPathToSysPath(cstr pingPath, U8FilePath& sysPath) override
		{
			// Ping paths have the format !directory/filename.

			if (!installation)
			{
				Throw(0, "%s: no installation. Call SetScriptPath first", __FUNCTION__);
			}
			
			WideFilePath wideSysPath;
			installation->ConvertPingPathToSysPath(pingPath, wideSysPath);

			Format(sysPath, "%ls", wideSysPath.buf);
		}

		void SysPathToPingPath(cstr sysPath, U8FilePath& pingPath) override
		{
			// Ping paths have the format !directory/filename.

			if (!installation)
			{
				Throw(0, "%s: no installation. Call SetScriptPath first", __FUNCTION__);
			}

			WideFilePath wideSysPath;
			Assign(wideSysPath, sysPath);
			installation->ConvertSysPathToPingPath(wideSysPath, pingPath);
		}

		ISxyNamespace& GetRootNamespace()
		{
			return rootNS;
		}

		void SearchContainersAndAddSexyVisFilesToDatabase(cstr projectFilePath)
		{
			WideFilePath wSexyVisDirectory;
			Assign(wSexyVisDirectory, projectFilePath);
			while (IO::MakeContainerDirectory(wSexyVisDirectory.buf))
			{
				WideFilePath wSexyVisPath;
				Format(wSexyVisPath, L"%simplicits.vis.sxy", wSexyVisDirectory.buf);
				if (IO::IsFileExistant(wSexyVisPath))
				{
					AutoRelease<ISourceCode> visSrc = sparser->LoadSource(wSexyVisPath, { 1,1 });
					AutoRelease<ISParserTree>visTree = sparser->CreateTree(*visSrc);
					BuildDatabaseFromProject(*this, visTree->Root(), projectFilePath, false);
					break;
				}
			}
		}

		void FocusProject(cstr projectFilePath) override
		{
			this->Clear();

			WideFilePath wPath;
			Assign(wPath, projectFilePath);

			SearchContainersAndAddSexyVisFilesToDatabase(projectFilePath);

			AutoRelease<ISourceCode> src = sparser->LoadSource(wPath, { 1,1 });
			AutoRelease<ISParserTree> tree = sparser->CreateTree(*src);
			BuildDatabaseFromProject(*this, tree->Root(), projectFilePath, true);
		}

		void ResolveRecursive(SxyNamespace& ns)
		{
			for (auto& s : ns.structures)
			{
				File_SXY& source = static_cast<File_SXY&>(s.file);
				auto i = source.structures.find(s.localName.c_str());
				if (i != source.structures.end())
				{
					s.localType = &i->second;
				}
			}

			for (auto& f : ns.functions)
			{
				File_SXY& source = static_cast<File_SXY&>(*f.file);
				auto i = source.functions.find(f.localName.c_str());
				if (i != source.functions.end())
				{
					f.localFunction = &i->second;
				}
			}

			for (auto& subspace: ns.subspaces)
			{
				ResolveRecursive(*subspace);
			}
		}

		void Sort() override
		{
			rootNS.SortRecursive();
			ResolveRecursive(rootNS);
		}

		ISParserTree* TryGetTree(int& errorCode, char* errorBuffer, size_t errorCapacity, cstr filename)
		{
			auto i = filenameToFile.find(filename);
			if (i == filenameToFile.end())
			{
				errorCode = 0;
				if (errorBuffer) SafeFormat(errorBuffer, errorCapacity, "No such file: %s", filename);
				return nullptr;
			}
			else
			{
				auto& file = *i->second;
				if (!file.s_tree)
				{
					errorCode = file.errorCode;
					if (errorBuffer) SafeFormat(errorBuffer, errorCapacity, "%s", file.errorMessage.c_str());
					return nullptr;
				}
				else
				{
					return file.s_tree;
				}
			}
		}

		void UpdateFile_SXY_PackedItem(cstr data, int32 length, cstr path) override
		{
			std::string filename = path;
			auto i = filenameToFile.find(filename);
			if (i == filenameToFile.end())
			{
				i = filenameToFile.insert(std::make_pair(filename, new File_SXY(filename))).first;
				auto& file = *i->second;

				U8FilePath name;
				SafeFormat(name.buf, "[package]:%hs", path);

				AutoRelease<ISourceCode> src;

				try
				{
					src = sparser->DuplicateSourceBuffer(data, length, { 1,1 }, name);
				}
				catch (IException& ex)
				{
					file.errorCode = ex.ErrorCode();
					file.errorMessage = ex.Message();
					return;
				}

				try
				{
					file.s_tree = sparser->CreateTree(*src);
					file.fileLength = file.s_tree->Source().SourceLength();
				}
				catch (IException& ex)
				{
					file.errorCode = ex.ErrorCode();
					file.errorMessage = ex.Message();
				}
			}

			if (i->second->s_tree)
			{
				ParseTree(*i->second->s_tree, *i->second);
			}
		}

		void UpdateFile_SXY(cstr fullpathToSxy) override
		{
			std::string filename = fullpathToSxy;
			auto i = filenameToFile.find(filename);
			if (i == filenameToFile.end())
			{
				i = filenameToFile.insert(std::make_pair(filename, new File_SXY(filename))).first;
				auto& file = *i->second;

				uint64 len = GetFileLength(fullpathToSxy);

				if (len > 32_megabytes)
				{
					file.errorCode = 0;
					file.errorMessage = "File too large";
				}
				else
				{
					WideFilePath wPath;
					Assign(wPath, fullpathToSxy);

					AutoRelease<ISourceCode> src;

					try
					{
						src = sparser->LoadSource(wPath, { 1,1 });
					}
					catch (IException& ex)
					{
						file.errorCode = ex.ErrorCode();
						file.errorMessage = ex.Message();
						return;
					}

					try
					{
						file.s_tree = sparser->CreateTree(*src);
						file.fileLength = file.s_tree->Source().SourceLength();
					}
					catch (ParseException& pex)
					{
						file.errorCode = pex.ErrorCode();
						file.errorMessage = pex.Message();
					}
					catch (IException& ex)
					{
						file.errorCode = ex.ErrorCode();
						file.errorMessage = ex.Message();
					}
				}

				if (i->second->s_tree)
				{
					ParseTree(*i->second->s_tree, *i->second);
				}
			}
		}

		void Clear() override
		{
			mapCppCodeToUsage.clear();
			resourceCount.clear();
			filenameToFile.clear();
			rootNS.subspaces.clear();
			rootNS.enums.clear();
			rootNS.functions.clear();
			rootNS.structures.clear();
			rootNS.interfaces.clear();
		}

		void Free() override
		{
			delete this;
		}

		void AppendAllChildrenFromRoot(cr_substring prefix, std::unordered_map<std::string, int>& exportList, ISxyNamespace& ns, int depth)
		{
			int dots = CountDots(prefix);
			if (depth > dots)
			{
				return;
			}

			char name[256];
			StackStringBuilder nameBuilder(name, sizeof name);
			AppendFullName(ns, nameBuilder);

			if (StartsWith(name, prefix))
			{
				exportList.insert(std::make_pair(std::string(name), 0));
			}

			if (StartsWith(prefix.start, name))
			{
				for (int i = 0; i < ns.FunctionCount(); ++i)
				{
					nameBuilder.Clear();
					AppendFullName(ns, nameBuilder);
					nameBuilder << ".";
					nameBuilder << ns.GetFunction(i).PublicName();

					if (ns.GetFunction(i).PublicName()[0] != '_' && StartsWith(name, prefix) && nameBuilder.Length() > Length(prefix))
					{
						exportList.insert(std::make_pair(std::string(name), 0));
					}
				}

				for (int i = 0; i < ns.InterfaceCount(); ++i)
				{
					auto& interf = ns.GetInterface(i);

					nameBuilder.Clear();
					AppendFullName(ns, nameBuilder);
					nameBuilder << ".";
					nameBuilder << interf.PublicName();

					if (StartsWith(name, prefix) && nameBuilder.Length() > Length(prefix))
					{
						exportList.insert(std::make_pair(std::string(name), 0));
					}
				}

				for (int i = 0; i < ns.TypeCount(); ++i)
				{
					auto& type = ns.GetType(i);

					nameBuilder.Clear();
					AppendFullName(ns, nameBuilder);
					nameBuilder << ".";
					nameBuilder << type.PublicName();

					if (StartsWith(name, prefix) && nameBuilder.Length() > Length(prefix))
					{
						exportList.insert(std::make_pair(std::string(name), 0));
					}
				}

				for (int i = 0; i < ns.FactoryCount(); ++i)
				{
					auto& factory = ns.GetFactory(i);

					nameBuilder.Clear();
					AppendFullName(ns, nameBuilder);
					nameBuilder << ".";
					nameBuilder << factory.PublicName();

					if (StartsWith(name, prefix) && nameBuilder.Length() > Length(prefix))
					{
						exportList.insert(std::make_pair(std::string(name), 0));
					}
				}
			}

			for (int i = 0; i < ns.SubspaceCount(); ++i)
			{
				if (!Eq(ns[i].Name(), "Native"))
				{
					AppendAllChildrenFromRoot(prefix, exportList, ns[i], depth + 1);
				}
			}
		}

		void AppendAllMacroChildrenFromRoot(cr_substring prefix, std::unordered_map<std::string, int>& exportList, ISxyNamespace& ns)
		{
			for (int i = 0; i < ns.EnumCount(); ++i)
			{
				cstr enumName = ns.GetEnumName(i);
				if (StartsWith(enumName, prefix))
				{
					char enumNameForExport[128];
					SafeFormat(enumNameForExport, "#%s", enumName);
					exportList.insert(std::make_pair(enumNameForExport, 0));
				}
			}

			for (int i = 0; i < ns.SubspaceCount(); ++i)
			{
				auto& subspace = ns[i];
				AppendAllMacroChildrenFromRoot(prefix, exportList, subspace);
			}
		}


		ISXYInterface* FindInterface(cstr typeString, ISxyNamespace** ppNamespace = nullptr) override
		{
			auto* direct = FindInterfaceDirect(GetRootNamespace(), typeString, ppNamespace);
			if (direct)
			{
				return direct;
			}

			return RecursivelySearchForInterface(GetRootNamespace(), typeString, ppNamespace);
		}

		ISXYInterface* FindInterfaceDirect(ISxyNamespace& ns, cstr typeString, ISxyNamespace** ppNamespace)
		{
			for (int i = 0; i < ns.SubspaceCount(); ++i)
			{
				auto& subspace = ns[i];
				if (Strings::StartsWith(typeString, subspace.Name()))
				{
					cstr subTypeString = typeString + strlen(subspace.Name());
					if (*subTypeString == '.')
					{
						subTypeString++;
						cstr nextDot = Strings::FindChar(subTypeString, '.');
						if (nextDot == nullptr)
						{
							for (int j = 0; j < subspace.InterfaceCount(); j++)
							{
								auto& candidate = subspace.GetInterface(j);
								if (Eq(candidate.PublicName(), subTypeString))
								{
									if (ppNamespace)
									{
										*ppNamespace = &subspace;
									}
									return &candidate;
								}
							}
						}

						return FindInterfaceDirect(subspace, subTypeString, ppNamespace);
					}
				}
			}

			return nullptr;
		}

		ISXYInterface* RecursivelySearchForInterface(ISxyNamespace& ns, cstr typeString, ISxyNamespace** ppNamespace)
		{
			for (int i = 0; i < ns.SubspaceCount(); ++i)
			{
				int interfaceCount = ns[i].InterfaceCount();
				for (int j = 0; j < interfaceCount; ++j)
				{
					auto& refInterface = ns[i].GetInterface(j);
					if (Eq(refInterface.PublicName(), typeString))
					{
						if (ppNamespace)
						{
							*ppNamespace = &ns[i];
						}
						return &refInterface;
					}
				}

				auto* pInterface = RecursivelySearchForInterface(ns[i], typeString, ppNamespace);
				if (pInterface)
				{
					return pInterface;
				}
			}

			return nullptr;
		}

		ISXYType* SearchForTypeWithoutRecursion(ISxyNamespace& ns, cr_substring typeString)
		{
			int typeCount = ns.TypeCount();
			for (int j = 0; j < typeCount; ++j)
			{
				auto& type = ns.GetType(j);
				if (Eq(typeString, type.PublicName()))
				{
					return &type;
				}
			}

			return nullptr;
		}

		ISXYType* SearchForLocalTypeWithoutRecursion(ISxyNamespace& ns, cr_substring typeString)
		{
			int typeCount = ns.TypeCount();
			for (int j = 0; j < typeCount; ++j)
			{
				auto& type = ns.GetType(j);
				if (Eq(type.LocalType()->LocalName(), typeString))
				{
					return &type;
				}
			}

			return nullptr;
		}

		ISxyNamespace* FindSubspaceWithoutRecursion(ISxyNamespace& ns, cr_substring subName)
		{
			for (int i = 0; i < ns.SubspaceCount(); ++i)
			{
				if (Eq(ns[i].Name(), subName))
				{
					return &ns[i];
				}
			}
			return nullptr;
		}

		ISXYType* RecursivelySearchForType(ISxyNamespace& ns, cr_substring typeString)
		{
			Substring subsearch = typeString;

			if (*ns.Name())
			{	
				cstr firstDot = Strings::ForwardFind('.', typeString);
				if (firstDot)
				{
					subsearch.start = firstDot + 1;

					if (IsCapital(*subsearch.start))
					{
						Substring rootSpace{ typeString.start, firstDot };

						if (!Eq(rootSpace, ns.Name()))
						{
							return nullptr;
						}
					}
				}
			}

			auto* type = SearchForTypeWithoutRecursion(ns, subsearch);
			if (type)
			{
				return type;
			}
				
			for (int i = 0; i < ns.SubspaceCount(); ++i)
			{
				auto& subspace = ns[i];
				auto* localType = RecursivelySearchForType(subspace, subsearch);
				if (localType)
				{
					return localType;
				}
			}

			// We did not find a public type that matched the typestring, but perhaps there is a local type
			return SearchForLocalTypeWithoutRecursion(ns, subsearch);
		}

		void AppendFieldsFromTypeRef(ISXYType& type, cstr prependString, ISexyFieldEnumerator& fieldEnumerator)
		{
			auto* localType = type.LocalType();
			if (localType)
			{
				for (int k = 0; k < localType->FieldCount(); ++k)
				{
					auto field = localType->GetField(k);
					char buffer[128];
					SafeFormat(buffer, "%s%s", prependString, field.name);
					fieldEnumerator.OnField(buffer, Substring::Null());
				}
			}
		}

		bool AppendFieldsFromTypeRefWithPrefix(cr_substring memberPrefix, ISXYType& type, ISexyFieldEnumerator& fieldEnumerator)
		{
			size_t len = memberPrefix.Length();

			int outputCount = 0;

			auto* localType = type.LocalType();
			if (localType)
			{
				for (int k = 0; k < localType->FieldCount(); ++k)
				{
					auto field = localType->GetField(k);
					if (StartsWith(field.name, memberPrefix))
					{
						fieldEnumerator.OnField(field.name + len, Substring::Null());
						outputCount++;
					}
				}
			}

			return outputCount > 0;
		}

		cstr FindFieldTypeByName(ISXYLocalType& localType, cr_substring qualifiedVariableName)
		{
			Substring child = RightOfFirstChar('.', qualifiedVariableName);
			Substring parent = (child) ? Substring { qualifiedVariableName.start, child.start - 1 } : qualifiedVariableName;
			if (parent.finish[-1] == '.')
			{
				parent.finish--;
			}

			for (int k = 0; k < localType.FieldCount(); ++k)
			{
				auto field = localType.GetField(k);
				if (Eq(parent, to_fstring(field.name)))
				{
					return field.type;
				}
			}

			return nullptr;
		}

		bool AppendFieldsFromType(cr_substring variableName, ISxyNamespace& ns, cr_substring typeString, ISexyFieldEnumerator& fieldEnumerator)
		{
			// Variable name may be qualified, e.g: rect.left. In this case the typeString refers to the root of the namespace.
			// So we need to find the type, then advance the namespace to the child, i.e left, 
			ISXYType* type = RecursivelySearchForType(ns, typeString);

			if (!type)
			{
				return false;
			}

			fieldEnumerator.OnHintFound(typeString);
			
			Substring parent = variableName;
			
			Substring childVariable = RightOfFirstChar('.', parent);
			if (!childVariable)
			{
				if (type)
				{
					cstr prependString = variableName.finish[-1] == '.' ? "" : ".";
					AppendFieldsFromTypeRef(*type, prependString, fieldEnumerator);
					return true;
				}
				else
				{
					return false;
				}
			}

			auto* localType = type->LocalType();
			if (!localType)
			{
				return false;
			}
#
			cstr fieldType = FindFieldTypeByName(*localType, childVariable);
			if (fieldType)
			{
				fieldEnumerator.OnHintFound(ToSubstring(fieldType));
				return AppendFieldsFromType(childVariable, ns, ToSubstring(fieldType), fieldEnumerator);
			}
			else
			{
				// If we cannot find a field with the given name, perhaps we can find fields that match the 'childVariable' as a prefix
				return AppendFieldsFromTypeRefWithPrefix(childVariable, *type, fieldEnumerator);
			}
		}

		bool AppendMethodsFromType(cr_substring variableName, cstr candidateFinish, ISxyNamespace& ns, cr_substring type, ISexyFieldEnumerator& fieldEnumerator, int depth = 0)
		{
			// Variable name may be qualified, e.g: rect.left. In this case the typeString refers to the root of the namespace.
			// So we need to find the type, then advance the namespace to the child, i.e left, 

			char typeString[128];
			CopyWithTruncate(type, typeString, sizeof typeString);

			ISXYInterface* pInterfaceType = FindInterface(typeString);

			enum { MAX_INTERFACE_TREE_DEPTH = 16 };
			if (depth > MAX_INTERFACE_TREE_DEPTH)
			{
				// Most likely scenario is that we have a recursive interface definition in which some A derives from B and B derives from A, which is illegal, but parseable.
				return true;
			}

			if (!pInterfaceType)
			{
				return false;
			}

			cstr lastDotPos = Strings::ReverseFind('.', variableName);
			Substring methodPrefix{ lastDotPos + 1, candidateFinish };

			char prefix[64];
			SubstringToString(prefix, sizeof prefix, methodPrefix);

			if (depth == 0) fieldEnumerator.OnHintFound(ToSubstring(typeString));

			for (int k = 0; k < pInterfaceType->MethodCount(); ++k)
			{
				auto& method = pInterfaceType->GetMethod(k);

				cstr publicName = method.PublicName();

				if (StartsWith(publicName, prefix))
				{
					fieldEnumerator.OnField(publicName + methodPrefix.Length(), Substring::Null());
				}
			}

			cstr base = pInterfaceType->Base();
			if (base)
			{
				// Commented out for now. Notepad++ does not have a way to flag an item as a category string
				// char baseIndicator[128];
				// SafeFormat(baseIndicator, 128, "/@*//...%s-Methods...//", base);
				// fieldEnumerator.OnField(baseIndicator, Substring::Null());
				AppendMethodsFromType(variableName, candidateFinish, ns, ToSubstring(base), fieldEnumerator, depth + 1);
				return true;
			}

			return true;
		}

		bool EnumerateVariableAndFieldList(cr_substring candidate, cr_substring typeString, ISexyFieldEnumerator& fieldEnumerator) override
		{
			Substring variableName = candidate;
	/*		if (variableName && variableName.finish[-1] == '.')
			{
				variableName.finish--;
			}
			*/
			auto& root = GetRootNamespace();
			if (AppendFieldsFromType(variableName, root, typeString, fieldEnumerator))
			{
				return true;
			}
			else
			{
				if (!candidate) return false;

				/*
				if (candidate.finish[-1] != '.')
				{
					// We only append methods if the last character is a dot
					return false;
				}
				*/

				Substring objectName = candidate;

				// We show methods if either the caret lands on a dot, or lands on capitals after the dot
				if (objectName.finish[-1] != '.')
				{
					for (cstr i = candidate.finish - 2; i >= candidate.start; --i)
					{
						if (*i == '.')
						{
							if (!isupper(i[1]))
							{
								// Not a method character
								return false;
							}

							objectName.finish = i + 1;
							break;
						}
					}
				}

				return AppendMethodsFromType(objectName, candidate.finish, root, typeString, fieldEnumerator);
			}
		}

		void ForEachAutoCompleteCandidate(cr_substring prefix, ISexyFieldEnumerator& fieldEnumerator) override
		{
			std::unordered_map<std::string, int> exportList;

			auto& root = GetRootNamespace();
			for (int i = 0; i < root.SubspaceCount(); ++i)
			{
				AppendAllChildrenFromRoot(prefix, exportList, root[i], 0);
			}

			std::vector<std::string> sortedList;
			for (auto& i : exportList)
			{
				sortedList.push_back(i.first);
			}

			std::stable_sort(sortedList.begin(), sortedList.end());
	
			for (auto i : sortedList)
			{
				fieldEnumerator.OnField(i.c_str(), Substring::Null());
			}
		}

		void ForEachAutoCompleteMacroCandidate(cr_substring prefix, ISexyFieldEnumerator& fieldEnumerator) override
		{
			std::unordered_map<std::string, int> exportList;

			auto& root = GetRootNamespace();
			for (int i = 0; i < root.SubspaceCount(); ++i)
			{
				AppendAllMacroChildrenFromRoot(prefix, exportList, root[i]);
			}

			std::vector<std::string> sortedList;
			for (auto& i : exportList)
			{
				sortedList.push_back(i.first);
			}

			std::stable_sort(sortedList.begin(), sortedList.end());

			for (auto i : sortedList)
			{
				fieldEnumerator.OnField(i.c_str(), Substring::Null());
			}
		}

		bool GetHintForCandidateByNS(ISxyNamespace& ns, cr_substring prefix, char args[1024])
		{
			char name[256];
			StackStringBuilder nameBuilder(name, sizeof name);
			AppendFullName(ns, nameBuilder);

			if (!StartsWith(prefix, *nameBuilder))
			{
				return false;
			}

			if (nameBuilder.Length() == Length(prefix))
			{
				SafeFormat(args, 1024, "(namespace %s)", name);
				return true;
			}

			for (int i = 0; i < ns.FunctionCount(); ++i)
			{
				nameBuilder.Clear();
				AppendFullName(ns, nameBuilder);
				nameBuilder << ".";

				auto& f = ns.GetFunction(i);
				nameBuilder << f.PublicName();

				auto* l = f.LocalFunction();

				if (l && Eq(*nameBuilder, prefix))
				{
					StackStringBuilder argBuilder(args, 1024);

					for (int j = 0; j < l->InputCount(); ++j)
					{
						cstr inputName = l->InputName(j);
						cstr inputType = l->InputType(j);
						argBuilder.AppendFormat("(%s %s)", inputType, inputName);
					}

					argBuilder << " -> ";

					for (int j = 0; j < l->OutputCount(); ++j)
					{
						cstr outputName = l->OutputName(j);
						cstr outputType = l->OutputType(j);
						argBuilder.AppendFormat("(%s %s)", outputType, outputName);
					}

					argBuilder << ")";

					return true;
				}
			}
			
			for (int i = 0; i < ns.SubspaceCount(); ++i)
			{
				if (GetHintForCandidateByNS(ns[i], prefix, args))
				{
					return true;
				}
			}

			return false;
		}

		void GetHintForCandidate(cr_substring prefix, char args[1024]) override
		{
			auto& root = GetRootNamespace();
			if (!GetHintForCandidateByNS(root, prefix, args))
			{
				args[0] = 0;
			}
		}

		ISxyNamespace& InsertNamespaceRecursive(cstr ns, ISxyNamespace& parent, cr_sex src)
		{
			cstr nsIndex = FindDot(ns);
			
			char* subspace = (char*) _alloca(nsIndex - ns + 1);
			memcpy(subspace, ns, nsIndex - ns);
			subspace[nsIndex - ns] = 0;

			auto& branch = parent.Update(subspace, src);

			if (*nsIndex == 0)
			{
				// No more work to do
				return branch;
			}
			else
			{
				// We still have subspaces after the dot
				return InsertNamespaceRecursive(nsIndex+1, branch, src);
			}
		}

		ISxyNamespace& InsertNamespaceRecursiveSANSEnd(cstr ns, ISxyNamespace& parent, cr_sex src)
		{
			cstr nsIndex = FindDot(ns);

			if (*nsIndex == 0)
			{
				return parent;
			}

			char subspace[256];
			if (nsIndex - ns >= 255)
			{
				// Namespace too long
				return parent;
			}

			memcpy(subspace, ns, nsIndex - ns);
			subspace[nsIndex - ns] = 0;
			auto& branch = parent.Update(subspace, src);
			return InsertNamespaceRecursiveSANSEnd(nsIndex + 1, branch, src);
		}

		void InsertNamespaceUnique(cr_sex s, cstr ns, File_SXY&)
		{
			InsertNamespaceRecursive(ns, rootNS, s);
		}

		void InsertInterfaceRecursive(cstr ns, ISxyNamespace& parent, File_SXY& file, cr_sex sInterfaceDef)
		{
			cstr nsIndex = FindDot(ns);

			if (*nsIndex == '.')
			{
				char* subspace = (char*)_alloca(nsIndex - ns + 1);
				memcpy(subspace, ns, nsIndex - ns);
				subspace[nsIndex - ns] = 0;

				auto& branch = parent.Update(subspace, sInterfaceDef);
				InsertInterfaceRecursive(nsIndex + 1, branch, file, sInterfaceDef);
			}
			else
			{
				parent.UpdateInterface(ns, sInterfaceDef, file);
			}
		}

		void InsertArchetype(cr_sex s, const fstring& archetypeName, File_SXY& file)
		{
			char* publicName = (char*)_alloca(archetypeName.length + 1);
			CopyFinalNameToBuffer(publicName, archetypeName.length + 1, archetypeName);
			ISxyNamespace& ns = InsertNamespaceRecursiveSANSEnd(archetypeName, rootNS, s);
			ns.UpdateArchetype(publicName, s, file);
		}

		void InsertClass(cr_sex s, const fstring& className, File_SXY& file)
		{
			UNUSED(className);
			for (int i = 2; i < s.NumberOfElements(); ++i)
			{
				cr_sex sArg = s[i];
				if (sArg.NumberOfElements() == 2)
				{
					if (Eq(AlwaysGetAtomic(sArg, 0), "defines"))
					{
						if (IsAtomic(sArg[1]))
						{
							auto strFqName = sArg[1].String();
							cstr fqName = strFqName->Buffer;
							char publicName[128];
							CopyFinalNameToBuffer(publicName, sizeof publicName, fqName);
							ISxyNamespace& ns = InsertNamespaceRecursiveSANSEnd(fqName, rootNS, s);
							ns.UpdateInterfaceViaDefinition(publicName, s, file);
						}
					}
				}
			}
		}

		void InsertFactory(cr_sex s, const fstring& factoryName, const fstring& factoryInterface, File_SXY& file)
		{
			UNUSED(factoryInterface);
			char* publicName = (char*)_alloca(factoryName.length + 1);
			CopyFinalNameToBuffer(publicName, factoryName.length + 1, factoryName);
			ISxyNamespace& ns = InsertNamespaceRecursiveSANSEnd(factoryName, rootNS, s);
			ns.UpdateFactory(publicName, s, file);
		}

		void InsertInterface(cr_sex s, const fstring& fqName, File_SXY& file)
		{
			char publicName[128];
			CopyFinalNameToBuffer(publicName, sizeof publicName, fqName);
			ISxyNamespace& ns = InsertNamespaceRecursiveSANSEnd(fqName, rootNS, s);
			ns.UpdateInterface(publicName, s, file);
		}

		void InsertStruct(cr_sex s, const fstring& structName, File_SXY& file)
		{
			file.structures.insert(structName, SXYStruct(s));
		}

		void InsertStrong(cr_sex s, const fstring& strongName, File_SXY& file)
		{
			file.structures.insert(strongName, SXYStruct(s));
		}

		void InsertFunction(cr_sex s, const fstring& fnName, File_SXY& file)
		{
			file.functions.insert(fnName, SXYFunction(s));
		}

		void InsertMacroRecursive(cstr ns, ISxyNamespace& parent, File_SXY& file, cr_sex sMacroDef)
		{
			cstr nsIndex = FindDot(ns);

			if (*nsIndex == '.')
			{
				char* subspace = (char*)_alloca(nsIndex - ns + 1);
				memcpy(subspace, ns, nsIndex - ns);
				subspace[nsIndex - ns] = 0;

				auto& branch = parent.Update(subspace, sMacroDef);
				InsertMacroRecursive(nsIndex + 1, branch, file, sMacroDef);
			}
			else
			{
				parent.UpdateMacro(ns, sMacroDef, file);
			}
		}

		void InsertMacro(cr_sex s, const fstring& fqMacroName, File_SXY& file)
		{
			InsertMacroRecursive(fqMacroName, rootNS, file, s);
		}

		void InsertAlias(cr_sex s, const fstring& aliasFrom, const fstring& const_aliasTo, File_SXY& file)
		{
			// We have two sorts of alias: 
			//     1) from one namespace to another (alias Sys.Type.Float32 Sys.OpenGL.GLfloat)
			//     2) a local object to a namespace (alias Main EntryPoint.Main)

			char publicName[Rococo::NAMESPACE_MAX_LENGTH];
			char implicitName[Rococo::NAMESPACE_MAX_LENGTH];

			fstring aliasTo;

			if (Eq(const_aliasTo, "$"_fstring))
			{
				CopyString(publicName, Rococo::NAMESPACE_MAX_LENGTH, aliasFrom);

				CopyString(implicitName, Rococo::NAMESPACE_MAX_LENGTH, file.filename.c_str());
				cstr lastSlash = Rococo::ReverseFind('/', Substring { implicitName, implicitName + strlen(implicitName) } );
				if (lastSlash)
				{
					*const_cast<char*>(lastSlash) = 0;
				}

				ReplaceChar(implicitName, Rococo::NAMESPACE_MAX_LENGTH, '/', '.');

				StringCat(implicitName, ".", sizeof implicitName);
				StringCat(implicitName, aliasFrom, sizeof implicitName);

				aliasTo = to_fstring(implicitName);
			}
			else
			{
				CopyFinalNameToBuffer(publicName, sizeof publicName, const_aliasTo);
				aliasTo = const_aliasTo;
			}

			if (*publicName == 0)
			{
				// Error, the alias was not fully qualified - X.Y.PublicName
				return;
			}

			cstr dot = FindDot(aliasFrom);

			if (*dot == '.')
			{
				// A namespace mapping
				ISxyNamespace& ns = InsertNamespaceRecursiveSANSEnd(aliasTo, rootNS, s);
				ns.AliasNSREf(publicName, s, file);
			}
			else
			{
				// A local object published in the target namespace
				auto i = file.functions.find(aliasFrom);
				if (i != file.functions.end())
				{
					ISxyNamespace& ns = InsertNamespaceRecursiveSANSEnd(aliasTo, rootNS, s);
					ns.AliasFunction(aliasFrom, file, publicName);
				}

				auto j = file.structures.find(aliasFrom);
				if (j != file.structures.end())
				{
					ISxyNamespace& ns = InsertNamespaceRecursiveSANSEnd(aliasTo, rootNS, s);
					ns.AliasStruct(aliasFrom, file, publicName);
				}
			}
		}

		bool CanInsertCppRef(cr_sex sCppDeclaration)
		{
			if (sCppDeclaration.NumberOfElements() < 3)
			{
				return false;
			}

			cr_sex finalExpression = sCppDeclaration[sCppDeclaration.NumberOfElements() - 1];
			if (finalExpression.NumberOfElements() != 3)
			{
				return false;
			}

			// (source <cpp-file> <line-number>)
			cstr source = finalExpression[0].c_str();
			cstr fileName = finalExpression[1].c_str();
			cstr lineNumber = finalExpression[2].c_str();

			if (!Eq(source, "source"))
			{
				return false;
			}

			U8FilePath path;
			Format(path, "%s#%s", fileName, lineNumber);

			auto insertStatus = mapCppCodeToUsage.insert(path, 0);
			if (insertStatus.second)
			{
				// Avoid duplication for C++ stuff, as declarations overlap in most areas
				return true;
			}
			else
			{
				insertStatus.first->second++;
				return false;
			}
		}

		void ParseTree(ISParserTree& tree, File_SXY& file)
		{
			cr_sex sRoot = tree.Root();

			cr_sex sLine0 = sRoot[0];
			cr_sex sDirective0 = sLine0[0];

			bool isDeclarations = Eq(sDirective0.c_str(), "SexyDeclarations");

			// First build up a list of objects in the sxy file
			for (int i = 0; i < sRoot.NumberOfElements(); ++i)
			{
				if (match_compound(sRoot[i], 2, keywordNamespace, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& nsText)
					{
						UNUSED(sKeyword);
						InsertNamespaceUnique(s, nsText, file);
					}
				)) continue;

				enum { MAX_METHODS_AND_ATTRIBUTES_PER_INTERFACE = 32766 };
				if (match_compound(sRoot[i], MAX_METHODS_AND_ATTRIBUTES_PER_INTERFACE, keywordInterface, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& fqName)
					{
						UNUSED(sKeyword);
						InsertInterface(s, fqName, file);
					}
				)) continue;

				enum { MAX_FIELDS_PER_STRUCT = 32766 };
				if (match_compound(sRoot[i], MAX_FIELDS_PER_STRUCT, keywordStruct, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& structName)
					{
						UNUSED(sKeyword);
						InsertStruct(s, structName, file);
					}
				)) continue;

				enum {MAX_FIELDS_PER_STRONG = 3};
				if (match_compound(sRoot[i], MAX_FIELDS_PER_STRONG, keywordStrong, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& strongName)
					{
						UNUSED(sKeyword);
						InsertStrong(s, strongName, file);
					}
				)) continue;

				enum { MAX_ARGS_PER_FUNCTION = 128 };
				if (match_compound(sRoot[i], MAX_ARGS_PER_FUNCTION, keywordFunction, ParseAtomic,
					[this, isDeclarations, &file](cr_sex s, const fstring& sKeyword, const fstring& fnName)
					{
						UNUSED(sKeyword);
						if ((isDeclarations && CanInsertCppRef(s)) || !isDeclarations)
						{
							InsertFunction(s, fnName, file);
						}
					}
				)) continue;

				enum { MAX_ARGS_PER_MACRO = 128 };
				if (match_compound(sRoot[i], MAX_ARGS_PER_MACRO, keywordMacro, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& macroName)
					{
						UNUSED(sKeyword);
						InsertMacro(s, macroName, file);
					}
				)) continue;


				// Example (factory $.NewUIStack $.IUIStack : (construct UIStack))
				enum { MAX_ARGS_PER_FACTORY = 128 };
				if (match_compound(sRoot[i], MAX_ARGS_PER_FACTORY, keywordFactory, ParseAtomic, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& factoryName, const fstring& factoryInterface)
					{
						UNUSED(sKeyword);
						InsertFactory(s, factoryName, factoryInterface, file);
					}
				)) continue;

				// Example (archetype $.name (arg1 <name1>)...(argN <nameN>)->(out1 <name1>)...(outN <nameN>))
				enum { MAX_ARGS_PER_ARCHETYPE = 128 };
				if (match_compound(sRoot[i], MAX_ARGS_PER_ARCHETYPE, keywordArchetype, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& archetypeName)
					{
						UNUSED(sKeyword);
						InsertArchetype(s, archetypeName, file);
					}
				)) continue;

				// Example (class Dog (defines Sys.IDog) ...))
				enum { MAX_ARGS_PER_CLASS_DEFINITION = 1024 };
				if (match_compound(sRoot[i], MAX_ARGS_PER_CLASS_DEFINITION, keywordClass, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& localClassName)
					{
						UNUSED(sKeyword);
						InsertClass(s, localClassName, file);
					}
				)) continue;
			}

			// Then compute the aliases
			for (int i = 0; i < sRoot.NumberOfElements(); ++i)
			{
				enum { MAX_ALIAS_LEN = 3 };
				match_compound(sRoot[i], MAX_ALIAS_LEN, keywordAlias, ParseAtomic, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& aliasFrom, const fstring& aliasTo)
					{
						UNUSED(sKeyword);
						InsertAlias(s, aliasFrom, aliasTo, file);
					}
				);
			}
		}
	};
}

namespace Rococo::SexyStudio
{
	ISexyDatabaseSupervisor* CreateSexyDatabase(IFactoryConfig& config)
	{
		return new ANON::SexyDatabase(config);
	}

	void AppendFullName(ISxyNamespace& ns, StringBuilder& sb)
	{
		std::vector<cstr> genealogy;

		for (ISxyNamespace* i = &ns; i != nullptr; i = i->GetParent())
		{
			genealogy.push_back(i->Name());
		}

		// The final namespace is the root which has no name
		genealogy.pop_back();

		int depth = 0;

		for (auto i = genealogy.rbegin(); i != genealogy.rend(); i++, depth++)
		{
			if (depth > 0)
			{
				sb << ".";
			}

			sb << *i;
		}
	}
}