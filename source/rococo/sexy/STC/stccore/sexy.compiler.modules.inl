/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	https://github.com/theabstraction/rococo

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	

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
	class Module: public IModuleBuilder
	{
	private:
		HString name;
		HString nsText;
		IPackage* package = nullptr;
		IProgramObject& object;
		FunctionRegistry functions;
		StructRegistry structures;
		typedef TSexyVector<Anon::Function*> TClosures;
		TClosures closures;

		typedef TSexyVector<INamespaceBuilder*> TPrefixes;
		TPrefixes prefixes;

		bool isSystem = false;

		mutable const INamespace* defaultNamespace = nullptr;

		void ClearClosures()
		{
			for(auto i = closures.begin(); i != closures.end(); ++i)
			{
				Anon::Function* f = *i;
				delete f;
			}

			closures.clear();
		}

		// Intrinsics constructor
		Module(IProgramObject& _object, cstr _name, void* /* hack */) :
			object(_object),
			name(_name),
			functions(true),
			structures(true)
		{
		}
	public:
		Module(IProgramObject& _object, cstr _name):
			object(_object),
			name(_name),
			functions(true),
			structures(true),
			byteCodeVersion(0)
		{
		}

		~Module()
		{
			ClearClosures();
		}

		bool IsSystem() const override
		{
			return isSystem;
		}

		void MakeSystem() override
		{
			isSystem = true;
		}

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS;

		const INamespace* DefaultNamespace() const override
		{
			if (defaultNamespace == nullptr)
			{
				if (nsText.length() > 0)
				{
					defaultNamespace = &object.GetRootNamespace().AddNamespace(nsText, Rococo::Compiler::ADDNAMESPACEFLAGS_CREATE_ROOTS);
				}
			}

			return defaultNamespace;
		}

		void SetDefaultNamespace(const INamespace* ns) override
		{
			if (ns == nullptr)
			{
				Rococo::Throw(0, "Module %s: SetDefaultNamespace() failed with null argument.", name.c_str());
			}
			else if (defaultNamespace == nullptr)
			{
				defaultNamespace = ns;
			}
			else if (ns != defaultNamespace)
			{
				Rococo::Throw(0, "Module %s: SetDefaultNamespace(%s) failed\nDefault %s already exists:",
					name.c_str(), ns->FullName(), defaultNamespace->FullName());
			}
		}

		void SetPackage(IPackage* package, cstr nsText) override
		{
			this->package = package;
			this->nsText = nsText;
		}

		void Clear() override
		{
			functions.Clear();
			structures.Clear();
			prefixes.clear();
			ClearClosures();
		}

		const IStructure& GetStructure(int index) const override
		{
			const IStructAlias& alias = structures.GetStruct(index);
			return alias.GetStructure();
		}

		IStructureBuilder& GetStructure(int index) override
		{
			IStructAliasBuilder& alias = structures.GetStruct(index);
			return alias.GetStructure();
		}

		IFunctionBuilder& DeclareClosure(IFunctionBuilder& parent, bool mayUseParentSF, const Sex::ISExpression* definition) override
		{
			char name[32];
			SafeFormat(name, 32, ("_Closure%s%u"), parent.Name(), closures.size());
			Anon::Function* f = new Anon::Function(FunctionPrototype(name, false), *this, &parent, mayUseParentSF, definition, 0);
			closures.push_back(f);
			return *f;
		}

		IFunctionBuilder& DeclareFunction(const FunctionPrototype& prototype, const Sex::ISExpression* definition, int popBytes) override
		{
			Anon::Function* f = (Anon::Function*) functions.Get(prototype.Name);
			if (f != NULL)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, ("Function %s already defined in %s"), prototype.Name, name.c_str());
			}

			f = new Anon::Function(prototype, *this, NULL, false, definition, popBytes);

			functions.Register(f->Name(), *f);

			return *f;
		}

		int FunctionCount() const override
		{
			return (int32) (functions.size() + closures.size());
		}

		int StructCount() const override
		{
			return structures.StructCount();
		}

		const IFunction& GetFunction(int index) const override
		{
			if ( index < functions.size())
			{
				return functions[index].GetFunction();
			}

			int closureIndex = index - functions.size();
			return *closures[closureIndex];
		}

		IFunctionBuilder& GetFunction(int index) override
		{
			if ( index < functions.size())
			{
				return functions[index].GetFunction();
			}

			int closureIndex = index - functions.size();
			return *closures[closureIndex];
		}

		int byteCodeVersion;

		int GetVersion() const override
		{
			return byteCodeVersion;
		}

		void IncVersion() override
		{
			byteCodeVersion++;
		}

		IStructure& DeclareClass(cstr name, const StructurePrototype& prototype, const Sex::ISExpression* definition) override
		{
			Structure* s = new Structure(name, prototype, *this, prototype.archetype != NULL ? VARTYPE_Closure : VARTYPE_Derivative, definition);
			structures.Register(s->Name(), *s);
			return *s;
		}

		IStructureBuilder& DeclareStrongType(cstr name, VARTYPE underlyingType) override
		{
			StructurePrototype prototype(MEMBERALIGN_1, INSTANCEALIGN_1, true, NULL, false);

			Structure* s = new Structure(name, prototype, *this, underlyingType, NULL);

			cstr varName = nullptr;
			switch (underlyingType)
			{
			case VARTYPE_Int32:
				varName = "Int32";
				break;
			case VARTYPE_Float32:
				varName = "Float32";
				break;
			case VARTYPE_Bool:	
				varName = "Bool";
				break;
			case VARTYPE_Float64:
				varName = "Float64";
				break;
			case VARTYPE_Int64:
				varName = "Int64";
					break;
			default:
				Throw(ERRORCODE_BAD_ARGUMENT, this->Name(), "%s: %s - bad vartype", __FUNCTION__, name);
			}

			s->AddMember(NameString::From("Value"), TypeString::From(varName));
			s->MakeStrong();

			StructureMember& m = s->GetMemberRef(0);

			switch (underlyingType)
			{
			case VARTYPE_Int32:
			case VARTYPE_Float32:
			case VARTYPE_Bool:
				m.SetSize(4);
				break;
			case VARTYPE_Float64:
			case VARTYPE_Int64:
				m.SetSize(8);
				break;
			default:
				Throw(ERRORCODE_BAD_ARGUMENT, this->Name(), "%s: %s - bad vartype", __FUNCTION__, name);
			}

			s->Seal();

			try
			{
				structures.Register(s->Name(), *s);
			}
			catch (...)
			{
				delete s;
				throw;
			}
			return *s;
		}

		IStructureBuilder& DeclareStructure(cstr name, const StructurePrototype& prototype, const Sex::ISExpression* definition) override
		{
			VARTYPE type;
			if (Eq(name, "_Array"))
			{
				type = VARTYPE_Array;
			}
			else if (Eq(name, "_List"))
			{
				type = VARTYPE_List;
			}
			else if (Eq(name, "_Map"))
			{
				type = VARTYPE_Map;
			}
			else if (prototype.archetype != NULL)
			{
				type = VARTYPE_Closure;
			}
			else
			{
				type = VARTYPE_Derivative;
			}

			Structure* s = new Structure(name, prototype, *this, type, definition);
			try
			{
				structures.Register(s->Name(), *s);
			}
			catch (...)
			{
				delete s;
				throw;
			}
			return *s;
		}

		IStructureBuilder* FindStructure(cstr name) override
		{
			return structures.TryGet(name);
		}

		const IStructure* FindStructure(cstr name) const override
		{
			const IStructure* s = ((Module *)this)->structures.TryGet(name);
			return s;
		}

		const IFunction* FindFunction(cstr name) const override
		{
			const IFunction* f = ((Module *)this)->GetFunctionCore(name);
			return f;
		}

		IFunctionBuilder* GetFunctionCore(cstr name)
		{
			IFunctionBuilder *f = functions.Get(name);
			if (f != NULL)
			{
				return f;
			}
			
			if (name[0] == '_')
			{
				for(auto i = closures.begin(); i != closures.end(); ++i)
				{
					f = *i;
					if (AreEqual(f->Name(), name))
					{
						return f;
					}
				}
			}

			return NULL;
		}

		IFunctionBuilder* FindFunction(cstr name) override
		{
			return GetFunctionCore(name);
		}

		void UnresolveStructures(OUT TStructureList& items)
		{
			for(int i = 0; i < structures.StructCount(); ++i)
			{
				IStructure& s = structures.GetStruct(i).GetStructure();
				Structure* S = (Structure*) &s;
				items.push_back(S);
			}
		}

		bool ResolveDefinitions()
		{
			bool success = true;
			for(int i = 0; i < functions.FunctionCount(); i++)
			{
				Anon::Function& f = (Anon::Function&) functions[i].GetFunction();
				if (!f.TryResolveArguments())
				{
					success = false;
				}				
			}

			return success;
		}
			
		cstr Name() const override {	return name.c_str(); }
		IProgramObject& Object() override  { return object; }
		IPublicProgramObject& Object() const override { return object; }

		FunctionRegistry& Functions() { return functions; }
		StructRegistry& Structures() { return structures; }
		const FunctionRegistry& Functions() const { return functions; }
		const StructRegistry& Structures() const { return structures; }

		void UsePrefix(cstr name) override
		{
			for(auto i = prefixes.begin(); i != prefixes.end(); ++i)
			{
				INamespace* ns = *i;
				if (AreEqual(ns->FullName(), name))
				{
					Throw(ERRORCODE_BAD_ARGUMENT, name, "Duplicate using directive for: %s", name);
				}
			}
				
			INamespaceBuilder* ns = object.GetRootNamespace().FindSubspace(name);
			if (ns == NULL)
			{
				Throw(ERRORCODE_BAD_ARGUMENT, Name(), "Could not resolve prefix directive's namespace: '%s' in '%s'", name, Name());
			}
			prefixes.push_back(ns);
		}

		int PrefixCount() const override { return (uint32) prefixes.size(); }
		INamespaceBuilder& GetPrefix(int index) override { return *prefixes[index]; }
		const INamespace& GetPrefix(int index) const override { return *prefixes[index]; }
		void ClearPrefixes() override { prefixes.clear(); }

		static Module* CreateIntrinsics(IProgramObject& object, cstr name)
		{
			return new Module(object, name, NULL);
		}
	};
}}} // Rococo::Compiler::Impl