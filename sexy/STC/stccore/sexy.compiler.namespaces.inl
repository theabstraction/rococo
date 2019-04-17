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

namespace Rococo { namespace Compiler { namespace Impl
{
	void CreateNullInstance(ObjectStub* obj, const IStructure& nullStruct)
	{
		obj->Desc = (ObjectDesc*) nullStruct.GetVirtualTable(0);
		obj->refCount = ObjectStub::NO_REF_COUNT;
		obj->pVTables[0] = (VirtualTable*) nullStruct.GetVirtualTable(1);
	}

	Interface::Interface(cstr _name, const int _methodCount, IStructureBuilder& _nullObjectType, Interface* _base):
		name(_name), methodCount(_methodCount), nullObjectType(_nullObjectType), universalNullInstance(NULL), base(_base)
	{
		archetypes = new Archetype*[methodCount];
		for(int i = 0; i < _methodCount; ++i)
		{
			archetypes[i] = NULL;
		}		
	}

	Interface::~Interface()
	{
		for(int i = 0; i < methodCount; ++i)
		{
			delete archetypes[i];
		}

		delete[] archetypes;
		delete universalNullInstance;
	}

	void Interface::SetMethod(size_t index, cstr name, size_t argCount, cstr _argNames[], const IStructure* types[], const IArchetype* archetypes[], const IStructure* genericArgs1[], const bool isOut[], const void* definition)
	{
		int nOutputs = 0;

		for(size_t i = 0; i < argCount; ++i)
		{
			if (isOut[i]) nOutputs++;
		}

		delete this->archetypes[index];
		this->archetypes[index] = new Archetype(name, _argNames, types, archetypes, genericArgs1, nOutputs, (int32)(argCount) - nOutputs, true, definition);
	}

	void Interface::PostCompile()
	{
		CreateNullInstance(UniversalNullInstance(), nullObjectType);
	}

	Namespace::Namespace(IProgramObject& _object): object(_object), parent(NULL), functions(false),	structures(false)
	{
		name = CreateSexString((""));
		fullname = CreateSexString((""));
	}

	Namespace::Namespace(IProgramObject& _object, cstr _name, Namespace* _parent):	object(_object),	parent(_parent), functions(false),	structures(false)
	{
		int32 capacity = _parent->FullName()->Length + 2 + StringLength(_name); 
		char* fullNamebuffer = (char*) alloca(capacity * sizeof(char));
		if (_parent->FullName()->Length > 0)
		{
			SafeFormat(fullNamebuffer, capacity, ("%s.%s"), _parent->FullName()->Buffer, _name);
		}
		else
		{
         SafeFormat(fullNamebuffer, capacity, ("%s"), _name);
		}

		fullname = CreateSexString(fullNamebuffer);
		name = CreateSexString(_name);
	}

	Namespace::~Namespace()
	{
		FreeSexString(name);
		FreeSexString(fullname);
		Clear();
	}

	void Namespace::EnumerateFactories(ICallback<const IFactory, cstr>& onFactory) const
	{
		for (auto& i : factories)
		{
			if (onFactory(*i.second, i.first.c_str()) == CALLBACK_CONTROL_BREAK) break;
		}
	}

	void Namespace::EnumerateStrutures(ICallback<const IStructure, cstr>& onStructure) const
	{
		for(int i = 0; i < structures.StructCount(); ++i)
		{
			auto& alias = structures.GetStruct(i);
			if (onStructure(alias.GetStructure(), alias.GetPublicName()) == CALLBACK_CONTROL_BREAK) break;
		}
	}

	void Namespace::EnumerateFunctions(ICallback<const IFunction, cstr>& onFunction) const
	{
		for (int i = 0; i < functions.FunctionCount(); ++i)
		{
			auto& alias = functions[i];
			if (onFunction(alias.GetFunction(), alias.GetPublicName()) == CALLBACK_CONTROL_BREAK) break;
		}
	}

	void Namespace::EnumerateArchetypes(ICallback<const IArchetype>& onArchetype) const
	{
		archetypes.EnumerateAll(onArchetype);
	}

	const IMacro* Namespace::FindMacro(cstr name) const
	{
		auto i = macros.find(CStringKey(name));
		return i == macros.end() ? NULL : i->second;
	}

	IMacroBuilder* Namespace::FindMacro(cstr name)
	{
		auto i = macros.find(CStringKey(name));
		return i == macros.end() ? NULL : i->second;
	}

	IMacroBuilder* Namespace::AddMacro(cstr name, void* expression, IFunctionBuilder& f)
	{
		auto i = macros.find(CStringKey(name));
		if (i != macros.end()) return NULL;
		auto m = new Macro(name, expression, *this, f);
		macros.insert(std::make_pair(CStringKey(name), m));
		return m;
	}

	FunctionRegistry& Namespace::Functions() { return functions; }
	StructRegistry& Namespace::Structures() { return structures; }
	const FunctionRegistry& Namespace::Functions() const { return functions; }
	const StructRegistry& Namespace::Structures() const { return structures; }
	IProgramObject& Namespace::Object() { return object; }
	IPublicProgramObject& Namespace::Object() const  { return object; }

	void Namespace::Clear()
	{
		for(auto i = children.begin(); i != children.end(); ++i)
		{
			Namespace* child = *i;
			child->Clear();
			delete child;
		}

		children.clear();
		nameToChildren.clear();

		for(auto i = interfaces.begin(); i != interfaces.end(); ++i)
		{
			auto* interface = i->second;
			interface->Free();
		}

		interfaces.clear();

		for (auto i = factories.begin(); i != factories.end(); ++i)
		{
			Factory* f = i->second;
			delete f;
		}

		factories.clear();

		for (auto i = macros.begin(); i != macros.end(); ++i)
		{
			Macro* m = i->second;
			delete m;
		}

		macros.clear();
	}

	void Namespace::Alias(IFunctionBuilder& f)
	{
		functions.Register(f.Name(), f);
	}

	void Namespace::Alias(cstr name, IFunctionBuilder& f)
	{
		functions.Register(name, f);
	}

	void Namespace::Alias(cstr publicName, IStructureBuilder& s)
	{
		structures.Register(publicName, s);
	}

	const IArchetype& Namespace::AddArchetype(cstr name, cstr argNames[], const IStructure* stArray[], const IArchetype* archArray[], const IStructure* genericArg1s[], int numberOfOutputs, int numberOfInputs, const void* definition)
	{
		return archetypes.Register(name, argNames, stArray, archArray, genericArg1s, numberOfOutputs, numberOfInputs, definition);
	}

	IInterfaceBuilder* Namespace::DeclareInterface(cstr name, int methodCount, IStructureBuilder& nullObject, IInterfaceBuilder* base)
	{
		Interface* inter = new Interface(name, methodCount, nullObject, base == NULL ? NULL : (Interface*) base);
		interfaces.insert(std::make_pair(inter->Name(), inter));
		interfaceEnum.push_back(inter);
		return inter;
	}

	IInterfaceBuilder* Namespace::FindInterface(cstr publicName)
	{
		auto i = interfaces.find(publicName);
		if (i != interfaces.end())
		{
			return i->second;
		}

		return NULL;
	}

	const IInterface* Namespace::FindInterface(cstr publicName) const
	{
		auto i = interfaces.find(publicName);
		if (i != interfaces.end())
		{
			return i->second;
		}

		return NULL;
	}

	IFunctionBuilder* Namespace::FindFunction(cstr name)
	{
		return FindByName(functions, name);
	}

	const IFunction* Namespace::FindFunction(cstr name) const
	{
		return FindByName(functions, name);
	}

	IStructureBuilder* Namespace::FindStructure(cstr name)
	{
		return structures.TryGet(name);
	}

	const IStructure* Namespace::FindStructure(cstr name) const
	{
		return structures.TryGet(name);
	}

	const IFactory* Namespace::FindFactory(cstr name) const
	{
		auto i = factories.find(name);
		return i == factories.end() ? NULL : i->second;
	}

	IFactoryBuilder* Namespace::FindFactory(cstr name)
	{
		auto i = factories.find(name);
		return i == factories.end() ? NULL : i->second;
	}

	IFactory& Namespace::RegisterFactory(cstr name, IFunctionBuilder& constructor, IInterfaceBuilder& interf, sexstring interfType)
	{
		Factory* f = new Factory(name, constructor, interf, interfType);
		factories.insert(std::make_pair(f->Name(), f));
		return *f;
	}

	INamespaceBuilder& Namespace::AddNamespace(cstr childName, ADDNAMESPACEFLAGS flags)
	{
		REQUIRE_NAMESPACE_STRING(childName, ("Namespace::AddNamespace"));

		cstr dotPos = GetSubString(childName, ("."));
		if (dotPos == NULL)
		{
			CStringKey key(childName);
			auto i = nameToChildren.find(key);
			if (i == nameToChildren.end())
			{
				Namespace* child = new Namespace(object, childName, this);
				children.push_back(child);
				CStringKey childKey(child->Name()->Buffer);
				nameToChildren.insert(std::make_pair(childKey, child));
				return *child;
			}
			else
			{
				return *i->second;
			}
		}
		else
		{
			int32 len = (int32)(dotPos - childName) + 1;
			char* branchName = (char*) alloca(sizeof(char) * len);
			memcpy_s(branchName, sizeof(char) * len, childName, sizeof(char) * (len-1));
			branchName[len-1] = 0;

			if (IsFlagged(flags, ADDNAMESPACEFLAGS_CREATE_ROOTS))
			{
				INamespaceBuilder& branch = AddNamespace(branchName, flags);
				return branch.AddNamespace(dotPos+1, flags);
			}
			else
			{
				INamespaceBuilder* branch = FindSubspace(branchName);
				if (branch != NULL)
				{
					return branch->AddNamespace(dotPos+1, flags);
				}
				else
				{
					sexstringstream<1024> streamer;
					cstr sep = fullname->Length == 0 ? ("") : (".");
					streamer.sb << ("Cannot create namespace '") << fullname->Buffer << sep << childName << ("' as one of its roots '") << fullname->Buffer << sep << branchName << ("' was undefined");
					throw STCException(ERRORCODE_BAD_ARGUMENT, ("Namespace"), streamer);
				}
			}
		}
	}

	size_t Namespace::ChildCount() const { return children.size(); }
	INamespaceBuilder& Namespace::GetChild(size_t index) { return *children[index]; }
	const INamespace& Namespace::GetChild(size_t index) const { return *children[index]; }
	const INamespace* Namespace::FindSubspace(cstr childName) const
	{
		return (const_cast<Namespace*>(this)->GetSubspaceCore(childName));
	}

	INamespaceBuilder* Namespace::GetSubspaceCore(cstr childName) 
	{
		cstr dotPos = GetSubString(childName, ("."));
			
		if (dotPos == NULL)
		{
			// The subspace is not qualified, so search children
			CStringKey childKey(childName);
			auto i = nameToChildren.find(childKey);
			return (i != nameToChildren.end()) ? i->second : NULL;
		}
		else
		{
			int length = (int32)(dotPos - childName);
			int capacity = sizeof(char) * (length + 1);
			char* branch = (char*) alloca(capacity);
			memcpy_s(branch, capacity, childName, sizeof(char) * length);
			branch[length] = 0;

			auto i = nameToChildren.find(branch);
			if (i != nameToChildren.end())
			{
				Namespace* ns = i->second;
				return ns->FindSubspace(dotPos+1);
			}
			else
			{
				return NULL;
			}
		}
	}

	INamespaceBuilder* Namespace::FindSubspace(cstr childName) 
	{
		return GetSubspaceCore(childName);
	}

	const sexstring Namespace::FullName() const {	return fullname; }
	const sexstring Namespace::Name() const { return name; }
	INamespaceBuilder* Namespace::Parent() { return parent; }
	const INamespace* Namespace::Parent() const { return parent; }

	const IArchetype* Namespace::FindArchetype(cstr name) const
	{
		return archetypes.Find(name);
	}

	IArchetype* Namespace::FindArchetype(cstr name)
	{
		return archetypes.Find(name);
	}
}}} // Rococo::Compiler::Impl