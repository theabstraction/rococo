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
	Structure::Structure(cstr _name, const StructurePrototype& _prototype, IModuleBuilder& _module, VARTYPE _type, const void* _definition):
		name(_name),
		prototype(_prototype),
		module(_module),
		isSealed(false),
		sizeOfStruct(0),
		type(_type),
		destructorId(0),
		virtualTables(NULL),
		definition(_definition),
		constructor(NULL)
	{
	}

	Structure::~Structure()
	{
		if (virtualTables != NULL)
		{
			for(int i = 0; i <= (int) interfaces.size(); ++i)
			{
				delete[] virtualTables[i];
			}

			delete[] virtualTables;
		}
	}

	void Structure::AddInterface(cstr interfaceFullName)
	{
		interfaceNames.push_back(interfaceFullName);
		interfaces.push_back(NULL);
	}

	int Structure::InterfaceCount() const
	{
		return (int) interfaceNames.size();
	}

	const IInterface& Structure::GetInterface(int index) const
	{
		return *interfaces[index];
	}

	IInterfaceBuilder& Structure::GetInterface(int index)
	{
		return *interfaces[index];
	}

	void Structure::AddMemberRaw(const NameString& _memberName, const TypeString& _type)
	{
		StructureMember m(_memberName.c_str(), _type.c_str(), (""), (""));
		members.push_back(m);
	}

	void Structure::AddMember(const NameString& _memberName, const TypeString& _type, cstr _genericArgType1, cstr _genericArgType2)
	{
		if (isSealed)
		{
			Throw(ERRORCODE_SEALED, ("Structure %s is sealed"), name.c_str());
		}

		for(uint32 i = 0; i < members.size(); ++i)
		{
			if (AreEqual(members[i].Name(), _memberName.c_str()))
			{
				Throw(ERRORCODE_BAD_ARGUMENT, __SEXFUNCTION__, ("Member %s of %s is duplicated"), _memberName.c_str(), name.c_str());
			}
		}

		StructureMember m(_memberName.c_str(), _type.c_str(), _genericArgType1 == NULL ? ("") : _genericArgType1, _genericArgType2 == NULL ? ("") : _genericArgType2);
		members.push_back(m);
	}

	void Structure::AddPseudoMember(const NameString& _name, const TypeString& _type)
	{
		if (isSealed)
		{
			Throw(ERRORCODE_SEALED, ("Structure %s is sealed"), name.c_str());
		}

		for(uint32 i = 0; i < members.size(); ++i)
		{
			if (AreEqual(members[i].Name(), _name.c_str()))
			{
				Throw(ERRORCODE_BAD_ARGUMENT, __SEXFUNCTION__, ("Member %s of %s is duplicated"), _name.c_str(), name.c_str());
			}
		}

		StructureMember m(_name.c_str(), _type.c_str(), (""), (""), true);
		members.push_back(m);
	}

	void Structure::Seal() { isSealed = true; }
	int Structure::MemberCount() const { return (int) members.size(); }

	IProgramObject& Structure::Object()
	{ 
		return module.Object();
	}

	IPublicProgramObject& Structure::Object() const
	{ 
		return module.Object();
	}

	cstr Structure::Name() const
	{
		return name.c_str();
	}

	const VARTYPE Structure::VarType() const
	{
		return type;
	}

	const StructurePrototype& Structure::Prototype() const
	{
		return prototype;
	}

	int Structure::SizeOfStruct() const
	{
		if (sizeOfStruct == 0)
		{
			int totalSize = 0;
			for(uint32 i = 0; i < members.size(); ++i)
			{
				size_t memberSize = members[i].SizeOfMember();
				if (memberSize > 0)
				{
					totalSize += (int32) memberSize;
				}
				else
				{
					if (!members[i].IsPseudoVariable())
					{
						return 0;
					}
				}
			}
			sizeOfStruct = totalSize;
		}

		return sizeOfStruct;
	}

	void Structure::ExpandAllocSize(int minimumByteCount)
	{
		sizeOfStruct = std::max(sizeOfStruct, minimumByteCount);
	}

	StructureMember& Structure::GetMemberRef(int index)
	{
		return members[index];
	}

	const IMember& Structure::GetMember(int index) const
	{
		return members[index];
	}

	IMemberBuilder& Structure::GetMember(int index)
	{
		return members[index];
	}

	void Structure::Update()
	{
		sizeOfStruct = 0;
	}

	IInterfaceBuilder* TryResolveInterfaceUsingPrefix(ILog& log, Structure& s, cstr name, bool reportErrors)
	{
		IInterfaceBuilder* finalInterface = NULL;
		INamespaceBuilder* finalNS = NULL;

		for(int i = 0; i < s.Module().PrefixCount(); ++i)
		{
			INamespaceBuilder& prefix = s.Module().GetPrefix(i);
			IInterfaceBuilder* interf = prefix.FindInterface(name);
			if (interf != NULL)
			{
				if (finalInterface != NULL)
				{
					if (reportErrors) LogError(log, ("Cannot resolve interface %s of %s from %s. Could be from %s or %s"), name, s.Name(), s.Module().Name(), prefix.FullName(), finalNS->FullName());
					return NULL;
				}

				finalInterface = interf;
				finalNS = &prefix;
			}
		}

		return finalInterface;	
	}

	void Structure::ExpandToNullObjects()
	{
		for(size_t i = 0; i < interfaceNames.size(); ++i)
		{
			interfaces[i]->ExpandNullObjectAllocSize(SizeOfStruct());
		}
	}

	bool Structure::ResolveInterfaces(ILog& log, bool reportErrors)
	{
		for(size_t i = 0; i < interfaceNames.size(); ++i)
		{
			cstr name = interfaceNames[i].c_str();

			NamespaceSplitter splitter(name);

			cstr body, publicName;
			if (!splitter.SplitTail(OUT body, OUT publicName))
			{
				IInterface* interf = TryResolveInterfaceUsingPrefix(log, *this, name, reportErrors);
				if (interf == NULL)
				{
					if (reportErrors) LogError(log, ("Cannot resolve interface %s of %s from %s. It is not fully qualified A.B.C.D, nor found in any namespace known to the module in which it is defined"), name, this->name.c_str(), this->module.Name());
					return false;
				}

				interfaces[i] = (IInterfaceBuilder*) interf;
			}
			else
			{
				INamespace* ns = module.Object().GetRootNamespace().FindSubspace(body);
				if (ns == NULL)
				{
					if (reportErrors) LogError(log, ("Cannot resolve interface %s of %s from %s. The namespace %s was not found"), name, this->name.c_str(), body, body);
					return false;
				}

				interfaces[i] =  (IInterfaceBuilder*) ns->FindInterface(publicName);
				if (interfaces[i] == NULL)
				{
					if (reportErrors) LogError(log, ("Cannot resolve interface %s of %s from %s. The interface name %s was not found in the namespace"), name, this->name.c_str(), (cstr) ns->FullName()->Buffer, publicName);
					return false;
				}
			}
		}

		// ExpandToNullObjects();

		return true;
	}

	bool TryResolveMember(ILog& log, Structure& s, StructureMember& member, bool reportErrors)
	{
		IStructureBuilder* underlyingType = Compiler::MatchStructure(log, member.Type(), s.Module());
		if (underlyingType == NULL) 
		{
			if (reportErrors)			
			{
				LogError(log, ("Could not resolve type from %s %s of %s from %s"), member.Type(), member.Name(), s.Name(), s.Module().Name());
			}
			return false;
		}

		if (member.GenericArg1Type()[0] == 0)
		{
			member.SetUnderlyingType(underlyingType, NULL, NULL);
		}
		else
		{
			IStructureBuilder* underlyingGenericArg1Type = Compiler::MatchStructure(log, member.GenericArg1Type(), s.Module());
			if (underlyingGenericArg1Type == NULL)
			{
				if (reportErrors)			
				{
					LogError(log, ("Could not resolving generic type (%s) from %s of %s from %s"), member.GenericArg1Type(), member.Name(), s.Name(), s.Module().Name());
				}
				return false;
			}

			if (member.GenericArg2Type()[0] == 0)
			{
				member.SetUnderlyingType(underlyingType, underlyingGenericArg1Type, NULL);
			}
			else
			{
				IStructureBuilder* underlyingGenericArg2Type = Compiler::MatchStructure(log, member.GenericArg2Type(), s.Module());
				if (underlyingGenericArg2Type == NULL)
				{
					if (reportErrors)			
					{
						LogError(log, ("Could not resolving generic type (%s) from %s of %s from %s"), member.GenericArg2Type(), member.Name(), s.Name(), s.Module().Name());
					}
					return false;
				}

				member.SetUnderlyingType(underlyingType, underlyingGenericArg1Type, underlyingGenericArg2Type);
			}
		}
		
		return member.SizeOfMember() != 0 || member.IsPseudoVariable();
	}

	bool TryResolve(ILog& log, Structure& s, bool reportErrors)
	{
		s.Seal();
		s.Update();

		uint32 nMembers = s.MemberCount();
		for(uint32 i = 0; i < nMembers; ++i)
		{
			StructureMember& member = s.GetMemberRef(i);
			member.SetUnderlyingType(NULL, NULL, NULL);

			if (!TryResolveMember(log, s, member, reportErrors))
			{
				return false;
			}
		}

		return s.ResolveInterfaces(log, reportErrors);
	}

	void ResolveTypesInIntrinsics(IProgramObject& object)
	{
		TStructureList unresolvedStructures;
		TStructureList resolvedStructures;

		int firstUnresolvedIndex = -1;

		Module& intrinsics = (Module&) object.IntrinsicModule();
		for(int i = 0; i < intrinsics.StructCount(); i++)
		{
			Structure& s = (Structure&) intrinsics.GetStructure(i);
			if (s.SizeOfStruct() == 0)
			{
				firstUnresolvedIndex = i;
				break;
			}
		}

		if (firstUnresolvedIndex < 0) return;

		for(int i = firstUnresolvedIndex; i < intrinsics.StructCount(); i++)
		{
			Structure& toResolve = (Structure&) intrinsics.GetStructure(i);
			toResolve.Seal();
			toResolve.Update();

			for(int k = 0; k < toResolve.MemberCount(); ++k)
			{
				StructureMember& member = toResolve.GetMemberRef(k);
					
				for(int j = 0; j < firstUnresolvedIndex; ++j)
				{
					Structure& nativeType = (Structure&) intrinsics.GetStructure(j);
					if (AreEqual(member.Type(),nativeType.Name()))
					{
						member.SetUnderlyingType(&nativeType, NULL, NULL);
						if (!member.IsPseudoVariable()) member.SetSize(nativeType.SizeOfStruct());
						break;
					}
				}				
			}
				
			toResolve.ResolveInterfaces(object.Log(), true);
		}
	}

	bool ResolveStructures(IProgramObject& object)
	{
		TStructureList unresolvedStructures;

		for(int i = 0; i < object.ModuleCount(); ++i)
		{
			Module& module = (Module&) object.GetModule(i);
			module.UnresolveStructures(OUT unresolvedStructures);
		}

		size_t toCheck = unresolvedStructures.size();
		while(!unresolvedStructures.empty())
		{
			Structure* s = unresolvedStructures.front();
			unresolvedStructures.pop_front();

			if (TryResolve(object.Log(), *s, false))
			{
				toCheck = unresolvedStructures.size();
			}
			else
			{
				unresolvedStructures.push_back(s);
				toCheck--;
				if (toCheck == 0)
				{
					// The resolve attempts that fail will trigger a report to the log
					for(auto i = unresolvedStructures.begin(); i != unresolvedStructures.end(); ++i)
					{
						Structure* s = *i;
						TryResolve(object.Log(), *s, true);
					}

					return false;
				}
			}				
		}

		return true;
	}

	typedef std::unordered_map<IStructureBuilder*,int> TStructureSet;

	void AddDependency(TStructureSet& x, IStructureBuilder& y)
	{
		auto i = x.find(&y);
		if (i == x.end())
		{
			x.insert(std::make_pair(&y, 1));
		}
		else
		{
			i->second++;
		}
	}	

	void ZeroDependencies(TStructureSet& x)
	{
		for(auto i = x.begin(); i != x.end(); ++i)
		{
			i->second = 0;
		}
	}

	void RemoveIndependents(TStructureSet& x)
	{
		auto i = x.begin();
		while (i != x.end())
		{
			if (i->second == 0) 
			{
				i = x.erase(i);
			}
			else
			{
				++i;
			}
		}
	}

	void UpdateAssociatedNullObjects(IStructureBuilder& s, TStructureSet& expandedStructures)
	{
		for(int m = 0; m < s.InterfaceCount(); ++m)
		{
			IInterfaceBuilder& interf = s.GetInterface(m);
			IStructureBuilder& nullType = interf.NullObjectType();

			if (nullType.SizeOfStruct() < s.SizeOfStruct())
			{
				nullType.ExpandAllocSize(s.SizeOfStruct());
				AddDependency(expandedStructures, nullType);
			}
		}
	}

	bool IsTypeInExpandList(IStructureBuilder& y, const TStructureSet& x)
	{
		auto i = x.find(&y);
		return i != x.end();
	}

	bool TryExpandMembers(IProgramObject& object, TStructureSet& expandedStructures)
	{
		int updateCount = 0;

		ZeroDependencies(expandedStructures);

		for(int i = 0; i < object.ModuleCount(); ++i)
		{
			Module& module = (Module&) object.GetModule(i);
			for (int j = 0; j < module.StructCount(); ++j)
			{
				IStructureBuilder& s = module.GetStructure(j);

				for(int k = 0; k < s.MemberCount(); ++k)
				{
					IMemberBuilder& member = s.GetMember(k);
					IStructureBuilder& type = *member.UnderlyingType();

					cstr name = member.Name();					

					if (IsTypeInExpandList(type, expandedStructures))
					{
						int diff = type.SizeOfStruct() - member.SizeOfMember();
						if (diff != 0)
						{
							((StructureMember&)(member)).SetSize(type.SizeOfStruct());
							updateCount++;

							s.Update();

							if (s.InterfaceCount() != 0)
							{
								UpdateAssociatedNullObjects(s, expandedStructures);
							}

							AddDependency(expandedStructures, s);
						}
					}					
				}
			}
		}

		RemoveIndependents(expandedStructures);

		return updateCount > 0;
	}

	bool TryUpdateNullObjects(IProgramObject& object, TStructureSet& expandedStructures)
	{
		int updateCount = 0;

		for(int i = 0; i < object.ModuleCount(); ++i)
		{
			Module& module = (Module&) object.GetModule(i);
			for (int j = 0; j < module.StructCount(); ++j)
			{
				const IStructure& s = module.GetStructure(j);
				
				int interfCount = s.InterfaceCount();
				if (interfCount > 0 && !IsNullType(s))
				{
					for(int k = 0; k < interfCount; ++k)				
					{
						IInterfaceBuilder& interf = (IInterfaceBuilder&) s.GetInterface(k);
						int concreteSize = s.SizeOfStruct();
						if (interf.NullObjectType().SizeOfStruct() < concreteSize)
						{
							interf.ExpandNullObjectAllocSize(concreteSize);

							AddDependency(expandedStructures, interf.NullObjectType());

							updateCount++;
						}
					}
				}
			}
		}

		return updateCount > 0;
	}

	bool ExpandMembersAndValidate(IProgramObject& object)
	{
		TStructureSet expandedStructures;	// Do not move inside the loop	

		enum { EXPAND_LIMIT = 10 };
		for(int i = 0; i < EXPAND_LIMIT; ++i)
		{
			TryUpdateNullObjects(object, REF expandedStructures);

			if (expandedStructures.empty())
			{
				return true;
			}

			if (!TryExpandMembers(object, REF expandedStructures))
			{
				return true;
			}
		}

		return false;
	}

	ID_BYTECODE Structure::GetDestructorId() const
	{
		if (destructorId == 0)
		{
			cstr className = name.c_str();

			TokenBuffer destrName;
         SafeFormat(destrName.Text, TokenBuffer::MAX_TOKEN_CHARS, ("%s.Destruct"), className);
			const IFunction* destructor = module.FindFunction(destrName);

			IProgramObject& object = (IProgramObject&) module.Object();

			if (destructor == NULL)
			{
				destructor = object.IntrinsicModule().FindFunction(("_nothing"));
			}

			CodeSection destSection;
			destructor->Code().GetCodeSection(OUT destSection);

			destructorId = destSection.Id;
		}
		return destructorId;
	}

	ID_BYTECODE* CreateIntrinsicVirtualTable(const IStructure& s, ID_BYTECODE destructorId)
	{
		ID_BYTECODE* intrinsicTable = new ID_BYTECODE[3];
		intrinsicTable[0] = 0; 
		intrinsicTable[1] = destructorId;
		intrinsicTable[2] = (ID_BYTECODE) &s;

		return intrinsicTable;
	}

	ID_BYTECODE GetByteCodeId(const IFunction& f)
	{
		CodeSection codeSection;
		f.Code().GetCodeSection(OUT codeSection);
		return codeSection.Id;
	}
	
	const ID_BYTECODE* Structure::GetVirtualTable(int interfaceIndex) const
	{
		if (virtualTables == NULL)
		{
			virtualTables = new ID_BYTECODE*[InterfaceCount()+1];
			for(int i = 0; i <= InterfaceCount(); ++i) virtualTables[i] = NULL;
		}

		if (virtualTables[interfaceIndex] == NULL)
		{
			if (interfaceIndex == 0)
			{
				virtualTables[0] = CreateIntrinsicVirtualTable(*this, GetDestructorId());
			}
			else
			{
				const IInterface& interf = GetInterface(interfaceIndex-1);
				virtualTables[interfaceIndex] = new ID_BYTECODE[interf.MethodCount()+1];

				virtualTables[interfaceIndex][0] = (ID_BYTECODE) -((int)(sizeof(size_t) * interfaceIndex + sizeof(int32)));

				for(int k = 0; k < interf.MethodCount(); k++)
				{
					const IArchetype& method = interf.GetMethod(k);

					TokenBuffer qualifiedMethodName;
               SafeFormat(qualifiedMethodName.Text, TokenBuffer::MAX_TOKEN_CHARS, ("%s.%s"), name.c_str(), method.Name());
					IFunction* implementation = module.FindFunction(qualifiedMethodName);
					if (implementation == NULL)
					{
						Throw(ERRORCODE_COMPILE_ERRORS, qualifiedMethodName, ("Compiler error getting implementation for interface method"));
					}

					virtualTables[interfaceIndex][k+1] = GetByteCodeId(*implementation);
				}
			}
		}

		return (const ID_BYTECODE*) virtualTables[interfaceIndex];
	}
}}} //Rococo::Compiler::Impl