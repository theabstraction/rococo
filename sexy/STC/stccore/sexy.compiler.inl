/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	http://www.sexiestengine.com

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	and 'Society of Demons.'

	1. This software is open-source. It can be freely copied, distributed, compiled and the compilation executed.
	
	1.a Modification of the software is not allowed where such modifcations fail to include this exact comment header in each source file, or alter, reduce or extend the Sexy language.
	The purpose of this limitation is to prevent different incompatible versions of the Sexy language emerging. 

	1.b You are allowed to fix bugs and implement performance improvements providing you inform Sexy's copyright owner via email. Such changes may then be incorporated in
	later versions of Sexy without compromising Sexy's copyright license.
	
	2. You are not permitted to copyright derivative versions of the source code. You are free to compile the code into binary libraries and include the binaries in a commercial application. 

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM “AS IS” WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

#pragma once

namespace Rococo { namespace Compiler { namespace Impl
{
	class FunctionAlias: public IFunctionAliasBuilder
	{
	private:
		stdstring publicName;
		IFunctionBuilder* fn;

	public:
		FunctionAlias(): fn(NULL) {}
		FunctionAlias(cstr _publicName, IFunctionBuilder& f): publicName(_publicName), fn(&f) {}

		virtual IFunctionBuilder& GetFunction() { return *fn; }
		virtual const IFunction& GetFunction() const { return *fn; }
		virtual cstr GetPublicName() const { return publicName.c_str(); }
	};

	class FunctionRegistry: public IFunctionEnumeratorBuilder
	{
	private:
		typedef std::vector<FunctionAlias> TFunctions;
		typedef std::unordered_map<CStringKey,IFunctionBuilder*, hashCStringKey> TFunctionsByName;
		TFunctions functions;
		TFunctionsByName functionsByName;
		bool managesLifetime;

	public:
		FunctionRegistry(bool _managesLifetime): managesLifetime(_managesLifetime) {}
		
		~FunctionRegistry()
		{
			Clear();
		}

		virtual void Clear()
		{		
			if (managesLifetime)
			{
				for(auto i = functionsByName.begin(); i != functionsByName.end(); ++i)
				{
					 i->second->Free();
				}
			}
			
			functionsByName.clear();
			functions.clear();
		}

		virtual int size() const
		{
			return (int) functions.size();
		}

		virtual void Register(cstr publicName, IFunctionBuilder& f)
		{
			functions.push_back(FunctionAlias(publicName, f));
			functionsByName.insert(std::make_pair(CStringKey(f.Name()), &f));
		}

		virtual int FunctionCount() const
		{
			return (int32) functions.size();
		}		

		virtual IFunctionAliasBuilder& operator[](int index)
		{
			return functions[index];
		}

		virtual const IFunctionAlias& operator[](int index) const
		{
			return functions[index];
		}

		virtual IFunctionBuilder* Get(cstr name)
		{
			auto i = functionsByName.find(CStringKey(name));
			return i != functionsByName.end() ? i->second : NULL;
		}

		virtual const IFunction* Get(cstr name) const
		{
			auto i = functionsByName.find(CStringKey(name));
			return i != functionsByName.end() ? i->second : NULL;
		}
	};

	class Archetype: public IArchetype
	{
	private:
		stdstring publicName;
		const IStructure** structureArray;
		const IArchetype** archetypeArray;
		const IStructure** genericArg1Array;
		cstr* argNameArray;
		int32 numberOfOutputs;
		int32 numberOfInputs;
		const bool isVirtualMethod;
		const void* definition;

	public:
		Archetype(cstr name, cstr* argNameArray, const IStructure** stArray, const IArchetype** archArray, const IStructure** _genericArg1Array, int32 numberOfOutputs, int32 numberOfInputs, bool isVirtual, const void* _def):
				publicName(name), structureArray(NULL), archetypeArray(NULL), argNameArray(NULL), isVirtualMethod(isVirtual), genericArg1Array(NULL), definition(_def)
		{
			SetStructs(argNameArray, stArray, archArray, _genericArg1Array, numberOfOutputs, numberOfInputs);
		}

		virtual const void* Definition() const { return definition; }

		virtual const bool IsVirtualMethod() const
		{
			return isVirtualMethod;
		}

		void SetStructs(const cstr* _argNameArray, const IStructure** stArray, const IArchetype** archArray, const IStructure** _genericArg1Array, int32 numberOfOutputs, int32 numberOfInputs)
		{
			delete[] structureArray;
			delete[] archetypeArray;
			delete[] argNameArray;
			delete[] genericArg1Array;

			this->numberOfOutputs = numberOfOutputs;
			this->numberOfInputs = numberOfInputs;

			if (numberOfOutputs + numberOfInputs == 0)
			{
				structureArray = NULL;
				archArray = NULL;
				argNameArray = NULL;
				genericArg1Array = NULL;
			}
			else
			{
				structureArray = new const IStructure*[numberOfOutputs+numberOfInputs];
				archetypeArray = new const IArchetype*[numberOfOutputs+numberOfInputs];
				argNameArray = new cstr[numberOfOutputs+numberOfInputs];
				genericArg1Array = new const IStructure*[numberOfOutputs+numberOfInputs];
				for(int32 i = 0; i < numberOfOutputs + numberOfInputs; ++i)
				{
					structureArray[i] = stArray[i];
					archetypeArray[i] = archArray[i];
					argNameArray[i] = _argNameArray[i];
					genericArg1Array[i] = _genericArg1Array[i];
				}				
			}
		}

		~Archetype()
		{
			delete[] structureArray;
			delete[] archetypeArray;
			delete[] argNameArray;
			delete[] genericArg1Array;
		}

		virtual cstr Name() const								{ return publicName.c_str(); }
		virtual const int32 NumberOfOutputs() const					{ return numberOfOutputs; }
		virtual const int32 NumberOfInputs() const					{ return numberOfInputs; }
		virtual const IStructure& GetArgument(int32 index) const	{ return *structureArray[index]; }
		virtual cstr GetArgName(int index) const					{ return argNameArray[index]; }
		virtual const IStructure* GetGenericArg1(int32 index) const { return genericArg1Array[index]; }
	};

	class ArchetypeRegistry
	{
	private:
		typedef std::vector<Archetype*> TDeclarations;
		TDeclarations declarations;

	public:
		// This rubbish needs thinking about
		const IArchetype& Register(cstr name, cstr* argNameArray, const IStructure** stArray, const IArchetype** archArray, const IStructure** genericArg1Array, uint32 numberOfOutputs, uint32 numberOfInputs, const void* definition)
		{
			Archetype* a = Find(name);
			if (a != NULL)
			{
				a->SetStructs(argNameArray, stArray, archArray, genericArg1Array, numberOfOutputs, numberOfInputs);
			}
			else
			{
				a = new Archetype(name, argNameArray, stArray, archArray, genericArg1Array, numberOfOutputs, numberOfInputs, false, definition);
				declarations.push_back(a);				
			}			

			return *a;
		}

		Archetype* Find(cstr name)
		{
			for(auto i = declarations.begin(); i != declarations.end(); ++i)
			{
				const Archetype* decl = *i;
				if (AreEqual(decl->Name(), name))
				{
					return *i;
				}
			}

			return NULL;
		}

		const Archetype* Find(cstr name) const
		{
			for(auto i = declarations.begin(); i != declarations.end(); ++i)
			{
				const Archetype* decl = *i;
				if (AreEqual(decl->Name(), name))
				{
					return *i;
				}
			}

			return NULL;
		}

		~ArchetypeRegistry()
		{
			for(auto i = declarations.begin(); i != declarations.end(); ++i)
			{
				delete *i;
			}
		}

		void EnumerateAll(ICallback<const IArchetype>& onArchetype) const
		{
			for (auto i = declarations.begin(); i != declarations.end(); ++i)
			{
				if (onArchetype(**i) == CALLBACK_CONTROL_BREAK) break;
			}
		}
	};

	class StructAlias: public IStructAliasBuilder
	{
	private:
		stdstring publicName;
		IStructureBuilder* s;

	public:
		StructAlias(): s(NULL) {}
		StructAlias(cstr _publicName, IStructureBuilder& _s): publicName(_publicName), s(&_s) {}

		virtual IStructureBuilder& GetStructure(){ return *s; }
		virtual const IStructure& GetStructure() const{ return *s; }
		virtual cstr GetPublicName() const { return publicName.c_str(); }
	};

	class StructRegistry
	{
	private:
		typedef std::unordered_map<stdstring, size_t> TMapNameToIndex;
		typedef std::vector<StructAlias> TStructures;
		TStructures structures;
		TMapNameToIndex structureMap;
		bool managesLifetime;

	public:
		StructRegistry(bool _managesLifetime): managesLifetime(_managesLifetime)
		{
	
		}

		~StructRegistry()
		{
			Clear();
		}

		virtual void Clear()
		{
			if (managesLifetime)
			{
				for(auto i = structures.begin(); i != structures.end(); ++i)
				{
					auto& sb = i->GetStructure();
               sb.Free();
				}
			}
			structures.clear();
		}

		virtual void Register(cstr publicName, IStructureBuilder& s)
		{
			TMapNameToIndex::const_iterator i = structureMap.find(publicName);
			if (i != structureMap.end())
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Duplicate structure definition for %s"), publicName);
			}

			structureMap.insert(std::make_pair(publicName, structures.size()));
			structures.push_back(StructAlias(publicName,s));
		}

		virtual int StructCount() const {  return (int32)structures.size(); }		
		virtual IStructAliasBuilder& GetStruct(int index) { return structures[index]; }
		virtual const IStructAlias& GetStruct(int index) const { return structures[index]; }
		virtual IStructureBuilder* TryGet(cstr name)
		{
			if (name == nullptr || *name == 0) Rococo::Throw(0, "%s: name was blank", __func__);
			for(auto i = structures.begin(); i != structures.end(); ++i)
			{
				IStructAliasBuilder& alias = *i;
				if (AreEqual(alias.GetPublicName(), name))
				{
					return &alias.GetStructure();
				}
			}

			return NULL;
		}

		virtual const IStructure* TryGet(cstr name) const
		{
			if (name == nullptr || *name == 0) Rococo::Throw(0, "%s: name was blank", __func__);
			for(auto i = structures.begin(); i != structures.end(); ++i)
			{
				const IStructAlias& alias = *i;
				if (AreEqual(alias.GetPublicName(), name))
				{
					return &alias.GetStructure();
				}
			}

			return NULL;
		}
	};

	class StructureMember: public IMemberBuilder
	{
	private:
		stdstring name;
		stdstring typeDesc;
		stdstring genericArg1Type;
		stdstring genericArg2Type;
		int sizeOfMember;
		IStructureBuilder* underlyingType;
		IStructureBuilder* underlyingGenericArg1Type;
		IStructureBuilder* underlyingGenericArg2Type;
		bool isInterfaceRef;
	public:
		void SetUnderlyingType(IStructureBuilder* _underlyingType, IStructureBuilder* _genericArg1Type, IStructureBuilder* _genericArg2Type)
		{
			underlyingType = _underlyingType;
			underlyingGenericArg1Type = _genericArg1Type;
			underlyingGenericArg2Type = _genericArg2Type;
			sizeOfMember = (underlyingType == NULL) ? 0 : (isInterfaceRef ? sizeof(void*) : underlyingType->SizeOfStruct());
		}

		StructureMember(cstr _name, cstr _type, cstr _genericArg1Type, cstr _genericArg2Type, bool _isInterfaceRef = false):
			name(_name), typeDesc(_type), underlyingType(NULL), genericArg1Type(_genericArg1Type),  genericArg2Type(_genericArg2Type), sizeOfMember(0),
			underlyingGenericArg1Type(NULL), underlyingGenericArg2Type(NULL), isInterfaceRef(_isInterfaceRef) {}

		StructureMember(): underlyingType(NULL), sizeOfMember(0), underlyingGenericArg1Type(NULL), underlyingGenericArg2Type(NULL), isInterfaceRef(false)  {}

		StructureMember(cstr _name, IStructureBuilder& type, IStructureBuilder* _genericArg1Type, IStructureBuilder* _genericArg2Type):
			name(_name), typeDesc(type.Name()), sizeOfMember(type.SizeOfStruct()), isInterfaceRef(false),
				genericArg1Type(_genericArg1Type == NULL ? ("") : _genericArg1Type->Name()),
				genericArg2Type(_genericArg2Type == NULL ? ("") : _genericArg2Type->Name())
		{
			underlyingType = &type;
			underlyingGenericArg1Type = _genericArg1Type;
			underlyingGenericArg2Type = _genericArg2Type;
		}

		const bool IsResolved() const override { return sizeOfMember != 0; }
		cstr Name() const	override { return name.c_str(); }
		const int SizeOfMember() const	override {	return sizeOfMember; }
		const IStructure* UnderlyingType() const override { return underlyingType; }
		IStructureBuilder* UnderlyingType() override { return underlyingType; }
		const IStructure* UnderlyingGenericArg1Type() const override { return underlyingGenericArg1Type; }
		IStructureBuilder* UnderlyingGenericArg1Type() override { return underlyingGenericArg1Type; }
		const IStructure* UnderlyingGenericArg2Type() const override { return underlyingGenericArg2Type; }
		IStructureBuilder* UnderlyingGenericArg2Type() override { return underlyingGenericArg2Type; }
		bool IsInterfaceVariable() const override { return isInterfaceRef; }

		cstr Type() const { return typeDesc.c_str(); }
		cstr GenericArg1Type() const { return genericArg1Type.c_str(); }
		cstr GenericArg2Type() const { return genericArg2Type.c_str(); }
		void SetSize(size_t size) { sizeOfMember = (int32) size; }
	};

	typedef std::vector<StructureMember> TStructureMembers;

	class Structure: public IStructureBuilder
	{
	private:
		stdstring name;
		StructurePrototype prototype;
		IModuleBuilder& module;
		TStructureMembers members;
		bool isSealed;
		mutable int sizeOfStruct;
		VARTYPE type;
		typedef std::vector<stdstring> TInterfaceNames;
		typedef std::vector<IInterfaceBuilder*> TInterfaces;
		TInterfaceNames interfaceNames;
		TInterfaces interfaces;
		mutable ID_BYTECODE destructorId;
		mutable ID_BYTECODE** virtualTables;
		const void* definition;
		const IFunction* constructor;
		
	public:
		Structure(cstr _name, const StructurePrototype& _prototype, IModuleBuilder& _module, VARTYPE type, const void* _definition);
		~Structure();

      virtual void Free() { delete this; }

		virtual void AddInterface(cstr interfaceFullName);
		virtual int InterfaceCount() const;
		virtual const IInterface& GetInterface(int index) const;
		virtual IInterfaceBuilder& GetInterface(int index);

		virtual void AddMember(const NameString& _name, const TypeString& _type, cstr _genericArgType1 = NULL, cstr _genericArgType2 = NULL);
		virtual void AddInterfaceMember(const NameString& _name, const TypeString& _type);
		virtual void Seal();
		virtual int MemberCount() const;
		virtual IProgramObject& Object();
		virtual IPublicProgramObject& Object() const;
		virtual cstr Name() const;
		virtual const VARTYPE VarType() const;
		virtual const StructurePrototype& Prototype() const;
		virtual IModuleBuilder& Module() { return module; }
		virtual const IModule& Module() const { return module; }
		virtual int SizeOfStruct() const;
		virtual bool IsResolved() const { return sizeOfStruct > 0; }
		virtual const ID_BYTECODE* GetVirtualTable(int interfaceIndex) const;
		virtual void ExpandAllocSize(int minimumByteCount);
		virtual const void* Definition() const { return definition; }
		virtual const IFunction* Constructor() const { return constructor; }
		virtual void SetConstructor(const IFunction* _cons) { constructor = _cons;}
	
		void ExpandToNullObjects();
		void AddMemberRaw(const NameString& _memberName, const TypeString& _type);

		StructureMember& GetMemberRef(int index);
		IMemberBuilder& GetMember(int index);
		const IMember& GetMember(int index) const;
		const IArchetype* Archetype() const { return prototype.archetype; }
		void Update();
		bool ResolveInterfaces(ILog& log, bool reportErrors);
		virtual ID_BYTECODE GetDestructorId() const;	
	};

	typedef std::list<Structure*> TStructureList;

	class CStructIdentityAlias: public IStructAliasBuilder
	{
	private:
		Structure* s;

	public:
		CStructIdentityAlias(Structure* _s): s(_s) {}

		virtual cstr GetPublicName() const	{	return s->Name();	}
		virtual IStructureBuilder& GetStructure() { return *s; }
		virtual const IStructure& GetStructure() const	{	return *s;	}
	};

	class AttributeContainer: public IAttributes
	{		
	private:
		typedef std::unordered_map<CStringKey, const void*, hashCStringKey> TMapKeyToAttr;
		TMapKeyToAttr attributeMap;

		typedef std::vector<std::pair<cstr,const void*>> TAttrVector;
		TAttrVector attributes;
	public:
		virtual bool AddAttribute(cstr name, const void* value);
		virtual const int AttributeCount() const;		
		virtual const bool FindAttribute(cstr name, OUT const void*& value) const;
		virtual cstr GetAttribute(int index, OUT const void*& value) const;

		void Clear() { attributes.clear(); attributeMap.clear(); }
	};

	class Interface: public IInterfaceBuilder
	{
	private:
		Archetype** archetypes;
		const int methodCount;
		stdstring name;
		IStructureBuilder& nullObjectType;
		mutable ObjectStub* universalNullInstance;
		AttributeContainer attributes;
		Interface* base;

	public:
		Interface(cstr _name, const int _methodCount, IStructureBuilder& _nullObjectType, Interface* _base);
		~Interface();
		
      virtual void Free()
      {
         delete this;
      }
		//IInterface
		virtual const IInterface* Base() const  { return base; }
		virtual const IAttributes& Attributes() const { return attributes; }
		virtual const IArchetype& GetMethod(int index) const {	return *archetypes[index];	}
		virtual const int MethodCount() const 	{ return methodCount;	}
		virtual cstr Name() const { return name.c_str(); }		
		virtual const IStructure& NullObjectType() const { return nullObjectType; }
		virtual ObjectStub* UniversalNullInstance() const 
		{
			if (!universalNullInstance)
			{
				int nBytes = nullObjectType.SizeOfStruct();
				universalNullInstance = (ObjectStub*) new uint8[nBytes];
				memset(universalNullInstance, 0, nBytes);
			}
			return universalNullInstance;
		}
		
		// IInterfaceBuilder
		virtual IAttributes& Attributes() { return attributes; }
		virtual void ExpandNullObjectAllocSize(int minimumByteCount) { nullObjectType.ExpandAllocSize(minimumByteCount); }
		virtual void SetMethod(size_t index, cstr name, size_t argCount, cstr argNames[], const IStructure* types[], const IArchetype* archetypes[], const IStructure* genericArg1s[], const bool isOut[], const void* definition);
		virtual void PostCompile();
		virtual IStructureBuilder& NullObjectType() { return nullObjectType; }
	};

	class Factory: public IFactoryBuilder
	{
	private:
		stdstring name;
		IFunctionBuilder& constructor;
		IInterfaceBuilder& interf;
		sexstring interfType;
		IFunctionBuilder* inlineConstructor;
		IStructureBuilder* inlineClass;

	public:
		Factory(cstr _name, IFunctionBuilder& _constructor, IInterfaceBuilder& _interf, sexstring _intType):
				name(_name), constructor(_constructor), interf(_interf), interfType(_intType), 
				inlineConstructor(NULL), inlineClass(NULL)
		{}

		virtual cstr Name() const {	return name.c_str();	}
		virtual const IFunction& Constructor() const	{	return constructor;	}
		virtual IFunctionBuilder& Constructor()			{	return constructor;	}
		virtual const IInterface& ThisInterface() const { return interf; }
		virtual sexstring InterfaceType() const { return interfType; }
		virtual const IFunction* InlineConstructor() const { return inlineConstructor; }
		virtual const IStructure* InlineClass() const { return inlineClass; }
		virtual IStructureBuilder* InlineClass() { return inlineClass; }
		virtual void SetInline(IFunctionBuilder* f, IStructureBuilder* s) { inlineConstructor = f; inlineClass = s; }
	};

	typedef std::unordered_map<CStringKey,Structure*,hashCStringKey> TResolvedStructures;
	typedef std::vector<CStructIdentityAlias> TStructures;
	typedef std::unordered_map<CStringKey,Interface*,hashCStringKey> TInterfaces;
	typedef std::vector<IInterfaceBuilder*> TInterfaceEnum;
	typedef std::unordered_map<CStringKey,Factory*,hashCStringKey> TFactories;

	class Macro: public IMacroBuilder
	{
	public:
		Macro(cstr _name, void* _expression, INamespace& _ns, IFunctionBuilder& _f): name(_name), expression(_expression), ns(_ns), f(_f) {}

		virtual cstr Name() const { return name; }
		virtual const void* Expression() const { return expression; }
		virtual const INamespace& NS() const { return ns; }
		virtual const IFunction& Implementation() const { return f; }
		virtual IFunctionBuilder& Implementation() { return f; }
	private:
		cstr name;
		void* expression;
		INamespace& ns;
		IFunctionBuilder& f;
	};

	class Namespace: public INamespaceBuilder
	{
	private:
		sexstring name;
		sexstring fullname;

		typedef std::vector<Namespace*> TChildren;
		typedef std::unordered_map<CStringKey,Namespace*,hashCStringKey> TMapNameToNS;
		typedef std::unordered_map<CStringKey,Macro*,hashCStringKey> TMapNameToMacro;

		TChildren children;
		TMapNameToNS nameToChildren;
		Namespace* parent;
		IProgramObject& object;

		FunctionRegistry functions;
		StructRegistry structures;
		ArchetypeRegistry archetypes;
		TInterfaces interfaces;
		TInterfaceEnum interfaceEnum;
		TFactories factories;

		TMapNameToMacro macros;

		INamespaceBuilder* GetSubspaceCore(cstr childName);
	public:
		Namespace(IProgramObject& _object);
		Namespace(IProgramObject& _object, cstr _name, Namespace* _parent);
		~Namespace();

		FunctionRegistry& Functions();
		StructRegistry& Structures();
		const FunctionRegistry& Functions() const;
		const StructRegistry& Structures() const;

		virtual IProgramObject& Object();
		virtual IPublicProgramObject& Object() const;

		virtual void Clear();

		virtual void Alias(IFunctionBuilder& f);
		virtual void Alias(cstr name, IFunctionBuilder& f);
		virtual void Alias(cstr publicName, IStructureBuilder& s);

		virtual IFunctionBuilder* FindFunction(cstr name);
		virtual IStructureBuilder* FindStructure(cstr name);
		virtual const IFunction* FindFunction(cstr name) const;
		virtual const IStructure* FindStructure(cstr name) const;

		virtual INamespaceBuilder& AddNamespace(cstr childName, ADDNAMESPACEFLAGS flags);
		virtual size_t ChildCount() const;
		virtual const INamespace& GetChild(size_t index) const;
		virtual INamespaceBuilder& GetChild(size_t index);
		virtual const INamespace* FindSubspace(cstr childName) const;
		virtual INamespaceBuilder* FindSubspace(cstr childName);
		virtual const sexstring FullName() const;
		virtual const sexstring Name() const;
		virtual const INamespace* Parent() const;
		virtual INamespaceBuilder* Parent();

		virtual const IArchetype& AddArchetype(cstr name, cstr argNames[], const IStructure* stArray[], const IArchetype* archArray[], const IStructure* genericArg1s[], int numberOfOutputs, int numberOfInputs, const void* definition);
		virtual const IArchetype* FindArchetype(cstr name) const;
		virtual IArchetype* FindArchetype(cstr name);

		virtual IInterfaceBuilder* DeclareInterface(cstr name, int methodCount, IStructureBuilder& nullObject, IInterfaceBuilder* base);
		virtual IInterfaceBuilder* FindInterface(cstr name);
		virtual const IInterface* FindInterface(cstr name) const;

		virtual int InterfaceCount() const { return (int32) interfaceEnum.size(); }
		virtual IInterfaceBuilder& GetInterface(int index) { return *interfaceEnum[index]; }
		virtual const IInterface& GetInterface(int index) const { return *interfaceEnum[index]; }

		virtual IMacroBuilder* FindMacro(cstr name);
		virtual const IFactory* FindFactory(cstr name) const;
		virtual IFactory& RegisterFactory(cstr name, IFunctionBuilder& constructor, IInterfaceBuilder& interf, sexstring interfType);

		virtual IMacroBuilder* AddMacro(cstr name, void* expression, IFunctionBuilder& f);

		virtual IFactoryBuilder* FindFactory(cstr name);
		virtual const IMacro* FindMacro(cstr name) const;

		virtual void EnumerateFactories(ICallback<const IFactory, cstr>& onFactory) const;
		virtual void EnumerateStrutures(ICallback<const IStructure, cstr>& onStructure) const;
		virtual void EnumerateFunctions(ICallback<const IFunction, cstr>& onFunction) const;
		virtual void EnumerateArchetypes(ICallback<const IArchetype>& onArchetype) const;
	};	
}}} // Rococo::Compiler::Impl