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

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM �AS IS� WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

namespace Sexy { namespace Compiler { namespace Impl
{
	class Module: public IModuleBuilder
	{
	private:
		stdstring name;
		IProgramObject& object;
		FunctionRegistry functions;
		StructRegistry structures;
		typedef std::vector<Function*> TClosures;
		TClosures closures;

		typedef std::vector<INamespaceBuilder*> TPrefixes;
		TPrefixes prefixes;

		void ClearClosures()
		{
			for(auto i = closures.begin(); i != closures.end(); ++i)
			{
				Function* f = *i;
				delete f;
			}

			closures.clear();
		}

		Module(IProgramObject& _object, csexstr _name, void* hack):
			object(_object),
			name(_name),
			functions(true),
			structures(true)
		{
		}
	public:
		Module(IProgramObject& _object, csexstr _name):
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

		virtual void Clear()
		{
			functions.Clear();
			structures.Clear();
			prefixes.clear();
			ClearClosures();
		}

		virtual const IStructure& GetStructure(int index) const
		{
			const IStructAlias& alias = structures.GetStruct(index);
			return alias.GetStructure();
		}

		virtual IStructureBuilder& GetStructure(int index)
		{
			IStructAliasBuilder& alias = structures.GetStruct(index);
			return alias.GetStructure();
		}

		virtual IFunctionBuilder& DeclareClosure(IFunctionBuilder& parent, bool mayUseParentSF, const void* definition)
		{
			SEXCHAR name[32];
			StringPrint(name, 32, SEXTEXT("_Closure%s%u"), parent.Name(), closures.size());
			Function* f = new Function(FunctionPrototype(name, false), *this, &parent, mayUseParentSF, definition);
			closures.push_back(f);
			return *f;
		}

		virtual IFunctionBuilder& DeclareFunction(const FunctionPrototype& prototype, const void* definition)
		{
			Function* f = (Function*) functions.Get(prototype.Name);
			if (f != NULL)
			{
				Throw(ERRORCODE_COMPILE_ERRORS, __SEXFUNCTION__, SEXTEXT("Function %s already defined in %s"), prototype.Name, name.c_str());
			}

			f = new Function(prototype, *this, NULL, false, definition);

			functions.Register(f->Name(), *f);

			return *f;
		}

		virtual int FunctionCount() const 
		{
			return (int32) (functions.size() + closures.size());
		}

		virtual int StructCount() const
		{
			return structures.StructCount();
		}

		const IFunction& GetFunction(int index) const
		{
			if ( index < functions.size())
			{
				return functions[index].GetFunction();
			}

			int closureIndex = index - functions.size();
			return *closures[closureIndex];
		}

		IFunctionBuilder& GetFunction(int index)
		{
			if ( index < functions.size())
			{
				return functions[index].GetFunction();
			}

			int closureIndex = index - functions.size();
			return *closures[closureIndex];
		}

		int byteCodeVersion;

		virtual int GetVersion() const
		{
			return byteCodeVersion;
		}

		virtual void IncVersion()
		{
			byteCodeVersion++;
		}

		virtual IStructure& DeclareClass(csexstr name, const StructurePrototype& prototype, const void* definition)
		{
			Structure* s = new Structure(name, prototype, *this, prototype.archetype != NULL ? VARTYPE_Closure : VARTYPE_Derivative, definition);
			structures.Register(s->Name(), *s);
			return *s;
		}

		virtual IStructureBuilder& DeclareStructure(csexstr name, const StructurePrototype& prototype, const void* definition)
		{
			Structure* s = new Structure(name, prototype, *this, prototype.archetype != NULL ? VARTYPE_Closure : VARTYPE_Derivative, definition);
			structures.Register(s->Name(), *s);
			return *s;
		}

		virtual IStructureBuilder* FindStructure(csexstr name)
		{
			return structures.TryGet(name);
		}

		virtual const IStructure* FindStructure(csexstr name) const
		{
			const IStructure* s = ((Module *)this)->structures.TryGet(name);
			return s;
		}

		virtual const IFunction* FindFunction(csexstr name) const
		{
			const IFunction* f = ((Module *)this)->GetFunctionCore(name);
			return f;
		}

		IFunctionBuilder* GetFunctionCore(csexstr name)
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

		virtual IFunctionBuilder* FindFunction(csexstr name)
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

		virtual bool ResolveDefinitions()
		{
			bool success = true;
			for(int i = 0; i < functions.FunctionCount(); i++)
			{
				Function& f = (Function&) functions[i].GetFunction();
				if (!f.TryResolveArguments())
				{
					success = false;
				}				
			}

			return success;
		}
			
		virtual csexstr Name() const	{	return name.c_str(); }
		virtual IProgramObject& Object() { return object; }
		virtual IPublicProgramObject& Object() const { return object; }

		FunctionRegistry& Functions() { return functions; }
		StructRegistry& Structures() { return structures; }
		const FunctionRegistry& Functions() const { return functions; }
		const StructRegistry& Structures() const { return structures; }

		virtual void UsePrefix(csexstr name)
		{
			for(auto i = prefixes.begin(); i != prefixes.end(); ++i)
			{
				INamespace* ns = *i;
				if (AreEqual(ns->FullName(), name))
				{
					Throw(ERRORCODE_BAD_ARGUMENT, name, SEXTEXT("Duplicate prefix directive"));
				}
			}
				
			INamespaceBuilder* ns = object.GetRootNamespace().FindSubspace(name);
			if (ns == NULL)
			{
				sexstringstream streamer;
				streamer << SEXTEXT("Could not resolve prefix directive's namespace: '") << name << SEXTEXT("' in '") << Name() << SEXTEXT("'") << std::ends;
				Throw(ERRORCODE_BAD_ARGUMENT, Name(), streamer.str().c_str());
			}
			prefixes.push_back(ns);
		}

		virtual int PrefixCount() const { return (uint32) prefixes.size(); }
		virtual INamespaceBuilder& GetPrefix(int index) { return *prefixes[index]; }
		virtual const INamespace& GetPrefix(int index) const { return *prefixes[index]; }
		virtual void ClearPrefixes() { prefixes.clear(); }

		static Module* CreateIntrinsics(IProgramObject& object, csexstr name)
		{
			return new Module(object, name, NULL);
		}
	};
}}} // Sexy::Compiler::Impl