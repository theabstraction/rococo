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

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::SexyStudio;

cstr AlwaysGetAtomic(cr_sex s)
{
	return IsAtomic(s) ? s.String()->Buffer : "<expected atomic argument>";
}

cstr AlwaysGetAtomic(cr_sex s, int index)
{
	if (index < 0 || index >= s.NumberOfElements()) return "<invalid subexpression index>";
	return IsAtomic(s[index]) ? s[index].String()->Buffer : "<expected atomic argument>";
}

namespace Rococo::SexyStudio
{
	ParseKeyword keywordNamespace("namespace");
	ParseKeyword keywordInterface("interface");
	ParseKeyword keywordStruct("struct");
	ParseKeyword keywordFunction("function");
	ParseKeyword keywordMacro("macro");
	ParseKeyword keywordAlias("alias");
	AtomicArg ParseAtomic;

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
		return fstring{ s.String()->Buffer, s.String()->Length };
	}

	ParseKeyword::ParseKeyword(cstr _keyword) : keyword(to_fstring(_keyword))
	{
	}

	bool ParseKeyword::Matches(cr_sex sParent, int index) const
	{
		cr_sex s = sParent[index];
		if (!IsAtomic(s)) return false;
		return Eq(s.String()->Buffer, keyword);
	}

	fstring ParseKeyword::operator()(cr_sex s, int index) const
	{
		return keyword;
	}
}

namespace ANON
{
	struct SXYAttribute
	{
		cr_sex sAttribute;
		cstr GetName() const
		{
			return AlwaysGetAtomic(sAttribute, 1);
		}
	};

	struct SXYMethod: ISXYFunction
	{
		const ISExpression* psMethod;
		cstr name = nullptr;
		int mapIndex = -1;

		int InputCount() const override
		{
			return mapIndex < 0 ? 0 : mapIndex - 1;
		}

		int OutputCount() const override
		{
			return mapIndex < 0 ? 0 : psMethod->NumberOfElements() - mapIndex - 1;
		}

		cstr SourcePath() const override
		{
			return "<no path>";
		}

		cstr InputType(int index) const override
		{
			cr_sex sArg = psMethod->GetElement(index + 1);
			return AlwaysGetAtomic(sArg, 0);
		}

		cstr OutputType(int index) const override
		{
			cr_sex sArg = psMethod->GetElement(index + mapIndex + 1);
			return AlwaysGetAtomic(sArg, 0);
		}

		cstr InputName(int index) const override
		{
			cr_sex sArg = psMethod->GetElement(index + 1);
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
		
		SXYMethod(cr_sex _sMethod) : psMethod(&_sMethod)
		{
			// Assumes s_Method.NumberOfElements >= 2 and s_Method[0] is atomic
			// Example: (AppendPrefixAndGetName (IString prefix)->(IString name))

			name = AlwaysGetAtomic(_sMethod, 0);

			for (int i = 1; i < _sMethod.NumberOfElements(); ++i)
			{
				if (IsAtomic(_sMethod[i]))
				{
					cstr arg = _sMethod[i].String()->Buffer;
					if (Eq(arg, "->"))
					{
						mapIndex = i;
						break;
					}
				}
			}
		}

		int GetBeginInputIndex() const
		{
			return 1;
		}

		int GetEndIndexIndex() const
		{
			return mapIndex < 0 ? 1 :  mapIndex;
		}

		int GetBeginOutputIndex() const
		{
			return mapIndex + 1;
		}

		int GetEndOutputIndex() const
		{
			return mapIndex < 0 ? 0 : psMethod->NumberOfElements();
		}

		SXYMethodArgument GetArg(int index) const
		{
			auto& sArg = (*psMethod)[index];
			if (sArg.NumberOfElements() == 2)
			{
				if (IsAtomic(sArg[0]) && IsAtomic(sArg[1]))
				{
					return { sArg[0].String()->Buffer, sArg[1].String()->Buffer };
				}
			}

			return { nullptr, nullptr };
		}
	};

	struct SxyInterface: ISXYInterface
	{
		SxyInterface(cstr name, ISXYFile& _source_file, cr_sex _sInterfaceDef):
			shortName(name), source_file(_source_file), sInterfaceDef(_sInterfaceDef)
		{

		}

		cstr SourcePath() const override
		{
			return sInterfaceDef.Tree().Source().Name();
		}

		int AttributeCount() const override
		{
			return (int)attributes.size();
		}

		cstr GetAttribute(int index) const
		{
			return attributes[index].GetName();
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
		ISXYFile& source_file;
		cr_sex sInterfaceDef;
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
						cstr s0 = sDirective[0].String()->Buffer;
						cstr s1 = sDirective[1].String()->Buffer;

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
			publicName(_publicName), localName(_localName), file(_file)
		{

		}

		cstr PublicName() const override
		{
			return publicName.c_str();
		}

		std::string publicName;
		std::string localName;
		ISXYFile& file;

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

		cstr SourcePath() const
		{
			return sStructDef.Tree().Source().Name();
		}

		/* Example:
		(struct Quaternion
			(Float32 scalar)
			(Float32 vi vj vk)
		)
		*/
		SXYField GetField(int index) const override
		{
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

	struct SxyNamespace : ISxyNamespace
	{
		std::string name;
		std::vector<std::unique_ptr<SxyNamespace>> subspaces;
		std::vector<SxyInterface> interfaces;
		std::vector<SXYMacro> macros;
		std::vector<SXYMacro> enums;
		std::vector<SXYPublicFunction> functions;
		std::vector<SXYPublicStruct> structures;
		std::vector<SXYNSAlias> nsAlias;

		SxyNamespace(cstr _name) : name(_name) {}

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

		cstr GetAliasSourcePath(int index) const override
		{
			return nsAlias[index].sAliasDef.Tree().Source().Name();
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

		int Length() const override
		{
			return (int) subspaces.size();
		}

		int FunctionCount() const override
		{
			return (int)functions.size();
		}

		ISXYPublicFunction& GetFunction(int index)
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

		ISxyNamespace& Update(cstr subspace) override
		{
			for (auto& s : subspaces)
			{
				if (Eq(s->Name(), subspace))
				{
					return *s;
				}
			}

			subspaces.push_back(std::make_unique<SxyNamespace>(subspace));
			return *subspaces[subspaces.size() - 1];
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
						interf.attributes.push_back( { sChild  } );
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

		cstr SourcePath() const override
		{
			return sFunction.Tree().Source().Name();
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

			for (int i = 1; i < _sFunction.NumberOfElements(); ++i)
			{
				if (IsAtomic(_sFunction[i]))
				{
					cstr arg = _sFunction[i].String()->Buffer;
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
					return { sArg[0].String()->Buffer, sArg[1].String()->Buffer };
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

	cstr FindDot(cstr s)
	{
		while (*s != '.' && *s != 0)
		{
			s++;
		}
		
		return s;
	}

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

	struct SexyDatabase : ISexyDatabaseSupervisor
	{
		AutoRelease<ISParser> sparser;
		ANON::SxyNamespace rootNS;

		std::unordered_map<std::string, std::unique_ptr<File_SXY>> filenameToFile;

		SexyDatabase(): 
			sparser(Sexy_CreateSexParser_2_0(Rococo::Memory::CheckedAllocator())),
			rootNS("")
		{

		}

		~SexyDatabase()
		{
			
		}

		ISxyNamespace& GetRootNamespace()
		{
			return rootNS;
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
				File_SXY& source = static_cast<File_SXY&>(f.file);
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

				if (len > 1_megabytes)
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
					catch (IException& ex)
					{
						file.errorCode = ex.ErrorCode();
						file.errorMessage = ex.Message();
					}
				}
			}

			if (i->second->s_tree)
			{
				ParseTree(*i->second->s_tree, *i->second);
			}
		}

		void Clear() override
		{
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

		ISxyNamespace& InsertNamespaceRecursive(cstr ns, ISxyNamespace& parent)
		{
			cstr nsIndex = FindDot(ns);
			
			char* subspace = (char*) _alloca(nsIndex - ns + 1);
			memcpy(subspace, ns, nsIndex - ns);
			subspace[nsIndex - ns] = 0;

			auto& branch = parent.Update(subspace);

			if (*nsIndex == 0)
			{
				// No more work to do
				return branch;
			}
			else
			{
				// We still have subspaces after the dot
				return InsertNamespaceRecursive(nsIndex+1, branch);
			}
		}

		ISxyNamespace& InsertNamespaceRecursiveSANSEnd(cstr ns, ISxyNamespace& parent)
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
			auto& branch = parent.Update(subspace);
			return InsertNamespaceRecursiveSANSEnd(nsIndex + 1, branch);
		}

		void InsertNamespaceUnique(cr_sex s, cstr ns, File_SXY& file)
		{
			InsertNamespaceRecursive(ns, rootNS);
		}

		void InsertInterfaceRecursive(cstr ns, ISxyNamespace& parent, File_SXY& file, cr_sex sInterfaceDef)
		{
			cstr nsIndex = FindDot(ns);

			if (*nsIndex == '.')
			{
				char* subspace = (char*)_alloca(nsIndex - ns + 1);
				memcpy(subspace, ns, nsIndex - ns);
				subspace[nsIndex - ns] = 0;

				auto& branch = parent.Update(subspace);
				InsertInterfaceRecursive(nsIndex + 1, branch, file, sInterfaceDef);
			}
			else
			{
				parent.UpdateInterface(ns, sInterfaceDef, file);
			}
		}

		void InsertInterface(cr_sex s, const fstring& fqName, File_SXY& file)
		{
			char* publicName = (char*)_alloca(fqName.length + 1);
			CopyFinalNameToBuffer(publicName, fqName.length + 1, fqName);
			ISxyNamespace& ns = InsertNamespaceRecursiveSANSEnd(fqName, rootNS);
			ns.UpdateInterface(publicName, s, file);
		}

		void InsertStruct(cr_sex s, const fstring& structName, File_SXY& file)
		{
			file.structures.insert(structName, SXYStruct(s));
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

				auto& branch = parent.Update(subspace);
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

		void InsertAlias(cr_sex s, const fstring& aliasFrom, const fstring& aliasTo, File_SXY& file)
		{
			// We have two sorts of alias: 
			//     1) from one namespace to another (alias Sys.Type.Float32 Sys.OpenGL.GLfloat)
			//     2) a local object to a namespace (alias Main EntryPoint.Main)

			char* publicName = (char*)_alloca(aliasTo.length + 1);
			CopyFinalNameToBuffer(publicName, aliasTo.length + 1, aliasTo);

			if (*publicName == 0)
			{
				// Error, the alias was not fully qualified - X.Y.PublicName
				return;
			}

			cstr dot = FindDot(aliasFrom);

			if (*dot == '.')
			{
				// A namespace mapping
				ISxyNamespace& ns = InsertNamespaceRecursiveSANSEnd(aliasTo, rootNS);
				ns.AliasNSREf(publicName, s, file);
			}
			else
			{
				// A local object published in the target namespace
				auto i = file.functions.find(aliasFrom);
				if (i != file.functions.end())
				{
					ISxyNamespace& ns = InsertNamespaceRecursiveSANSEnd(aliasTo, rootNS);
					ns.AliasFunction(aliasFrom, file, i->second.name);
				}

				auto j = file.structures.find(aliasFrom);
				if (j != file.structures.end())
				{
					ISxyNamespace& ns = InsertNamespaceRecursiveSANSEnd(aliasTo, rootNS);
					ns.AliasStruct(aliasFrom, file, j->second.sStructDef[1].String()->Buffer);
				}
			}
		}

		void ParseTree(ISParserTree& tree, File_SXY& file)
		{
			cr_sex sRoot = tree.Root();

			// First build up a list of objects in the sxy file
			for (int i = 0; i < sRoot.NumberOfElements(); ++i)
			{
				if (match_compound(sRoot[i], 2, keywordNamespace, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& nsText)
					{
						InsertNamespaceUnique(s, nsText, file);
					}
				)) continue;

				enum { MAX_METHODS_AND_ATTRIBUTES_PER_INTERFACE = 32766 };
				if (match_compound(sRoot[i], MAX_METHODS_AND_ATTRIBUTES_PER_INTERFACE, keywordInterface, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& fqName)
					{
						InsertInterface(s, fqName, file);
					}
				)) continue;

				enum { MAX_FIELDS_PER_STRUCT = 32766 };
				if (match_compound(sRoot[i], MAX_FIELDS_PER_STRUCT + 2, keywordStruct, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& structName)
					{
						InsertStruct(s, structName, file);
					}
				)) continue;

				enum { MAX_ARGS_PER_FUNCTION = 128 };
				if (match_compound(sRoot[i], MAX_ARGS_PER_FUNCTION + 2, keywordFunction, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& fnName)
					{
						InsertFunction(s, fnName, file);
					}
				)) continue;

				enum { MAX_ARGS_PER_MACRO = 128 };
				if (match_compound(sRoot[i], MAX_ARGS_PER_MACRO + 2, keywordMacro, ParseAtomic,
					[this, &file](cr_sex s, const fstring& sKeyword, const fstring& macroName)
					{
						InsertMacro(s, macroName, file);
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
						InsertAlias(s, aliasFrom, aliasTo, file);
					}
				);
			}
		}
	};
}

namespace Rococo::SexyStudio
{
	ISexyDatabaseSupervisor* CreateSexyDatabase()
	{
		return new ANON::SexyDatabase();
	}
}