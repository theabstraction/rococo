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

namespace
{
	class FunctionArgument: public IArgumentBuilder
	{
	private:
		stdstring name;
		stdstring type;
		stdstring genericArg1Type;
		stdstring genericArg2Type;
		ARGUMENTUSAGE usage;
		ARGDIRECTION direction;
		const IStructureBuilder* resolvedType;
		const IStructureBuilder* resolvedGenericArg1Type;
		const IStructureBuilder* resolvedGenericArg2Type;
		IFunctionBuilder& parentFunction;
		void* userdata;
		bool isClosureInput;

		bool _TryResolveArgument()
		{
			IProgramObject& object = parentFunction.Object();
			ILog& log = object.Log();

			resolvedType = NULL;

			IModuleBuilder& localMod = parentFunction.Module();

			IStructureBuilder* s = Compiler::MatchStructure(log, type.c_str(), localMod);
			if (s != NULL)
			{
				resolvedType = s;
				
				if (!genericArg1Type.empty())
				{
					IStructureBuilder* arg = Compiler::MatchStructure(log, genericArg1Type.c_str(), localMod);
					if (arg != NULL)
					{
						resolvedGenericArg1Type = arg;
						if (!genericArg2Type.empty())
						{
							IStructureBuilder* arg = Compiler::MatchStructure(log, genericArg2Type.c_str(), localMod);
							if (arg != NULL)
							{
								resolvedGenericArg2Type = arg;
								return true;
							}
							else
							{
								LogError(log, SEXTEXT("Error resolving '%s' of '%s' in '%s'"), genericArg2Type.c_str(), parentFunction.Name(), localMod.Name());
								return false;
							}
						}
						
						return true;
					}
					else
					{
						LogError(log, SEXTEXT("Error resolving '%s' of '%s' in '%s'"), genericArg1Type.c_str(), parentFunction.Name(), localMod.Name());
						return false;
					}
				}
				else
				{
					return true;
				}
			}
			else
			{
				LogError(log, SEXTEXT("Error resolving '%s' of '%s' in '%s'"), type.c_str(), parentFunction.Name(), localMod.Name());
				return false;
			}
		}
	public:
		FunctionArgument(const Rococo::Compiler::NameString& _name, const Rococo::Compiler::TypeString& _type, ARGDIRECTION _direction, IFunctionBuilder& _parentFunction, void* _userdata):
			name(_name.c_str()), type(_type.c_str()), usage(ARGUMENTUSAGE_BYVALUE), isClosureInput(false),
			direction(_direction), resolvedType(NULL), parentFunction(_parentFunction), userdata(_userdata), resolvedGenericArg1Type(NULL), resolvedGenericArg2Type(NULL)
		{

		}

		FunctionArgument(const Rococo::Compiler::NameString& _name, const Rococo::Compiler::IStructure& _type, ARGDIRECTION _direction, IFunctionBuilder& _parentFunction, void* _userdata):
			name(_name.c_str()), type(GetFriendlyName(_type)), usage(ARGUMENTUSAGE_BYVALUE), isClosureInput(false),
			direction(_direction), resolvedType((IStructureBuilder*)&_type), parentFunction(_parentFunction), userdata(_userdata), resolvedGenericArg1Type(NULL), resolvedGenericArg2Type(NULL)
		{

		}

		FunctionArgument(const Rococo::Compiler::NameString& _name, const Rococo::Compiler::TypeString& _genericType, const Rococo::Compiler::TypeString& _genericArgtype, ARGDIRECTION _direction, IFunctionBuilder& _parentFunction, void* _userdata):
			name(_name.c_str()), type(_genericType.c_str()), usage(ARGUMENTUSAGE_BYVALUE), genericArg1Type(_genericArgtype.c_str()), isClosureInput(false),
			direction(_direction), resolvedType(NULL), parentFunction(_parentFunction), userdata(_userdata), resolvedGenericArg1Type(NULL), resolvedGenericArg2Type(NULL)
		{

		}

		FunctionArgument(const Rococo::Compiler::NameString& _name, const Rococo::Compiler::TypeString& _genericType, const Rococo::Compiler::TypeString& _genericArgtype, const Rococo::Compiler::TypeString& _genericArgtype2, ARGDIRECTION _direction, IFunctionBuilder& _parentFunction, void* _userdata):
			name(_name.c_str()), type(_genericType.c_str()), usage(ARGUMENTUSAGE_BYVALUE), genericArg1Type(_genericArgtype.c_str()), genericArg2Type(_genericArgtype2.c_str()), isClosureInput(false),
			direction(_direction), resolvedType(NULL), parentFunction(_parentFunction), userdata(_userdata), resolvedGenericArg1Type(NULL), resolvedGenericArg2Type(NULL)
		{

		}

		void MakeClosureInput()
		{
			isClosureInput = true;
		}

		virtual csexstr Name() const	{	return name.c_str();	}
		virtual csexstr TypeString() const {	return type.c_str();	}
		virtual const IStructure* GenericTypeArg1() const { return resolvedGenericArg1Type; }
		virtual const IStructure* GenericTypeArg2() const { return resolvedGenericArg2Type; }
		virtual const ARGUMENTUSAGE Usage() const { return usage; }
		virtual const ARGDIRECTION Direction() const { return direction; }
		virtual const IStructure* ResolvedType() const { return resolvedType; }
		virtual IFunctionBuilder& Parent() { return parentFunction; }
		virtual const IFunction& Parent() const { return parentFunction; }
		virtual void* Userdata() const { return userdata; }
		virtual bool IsClosureInput() const { return isClosureInput; }

		virtual bool TryResolveArgument()
		{
			if (resolvedType != NULL || _TryResolveArgument())
			{
				usage = (VARTYPE_Derivative == resolvedType->VarType()) ? ARGUMENTUSAGE_BYREFERENCE : ARGUMENTUSAGE_BYVALUE;
				return true;
			}

			return false;
		}
	};

	class Function: public IFunctionBuilder
	{
	private:
		stdstring name;
		IModuleBuilder& module;
		ICodeBuilder* builder;
		Function* parent;
		bool isMethod;
		int inputCount;
		int outputCount;
		const void *definition;
		const IStructure* type;

		typedef std::vector<FunctionArgument*> TFunctionArgs;
		TFunctionArgs args;
	public:
		Function(const FunctionPrototype& fp, IModuleBuilder& _module, IFunctionBuilder* _parent, bool _mayUseParentSF, const void* _definition):
			name(fp.Name), module(_module), inputCount(0), outputCount(0), definition(_definition), type(NULL)
		{
			parent = (Function*) _parent;
			builder = CreateBuilder(*this, _mayUseParentSF);
			isMethod = fp.IsMethod;
		}

		~Function()
		{
			for(auto i = args.begin(); i != args.end(); i++)
			{
				FunctionArgument* arg = *i;
				delete arg;
			}

		   builder->Free();
		}

      virtual void Free()
      {
         delete this;
      }

		virtual csexstr Name() const								{ return name.c_str(); }
		const IStructure& GetArgument(int index) const				{ return *args[index]->ResolvedType(); }
		virtual const IStructure* GetGenericArg1(int index) const	{ return args[index]->GenericTypeArg1(); }
		csexstr GetArgName(int index)	const						{ return args[index]->Name(); }
		virtual const int NumberOfInputs() const					{ return inputCount; }
		virtual const int NumberOfOutputs() const					{ return outputCount; }
		virtual const bool IsVirtualMethod() const					{ return isMethod; }
		virtual const void* Definition() const						{ return definition; }
		virtual const IStructure* GetType() const					{ return type; }
		virtual void SetType(const IStructure* _type)				{ type = _type; }

		virtual const IModule& Module()  const			{ return module; }
		virtual IProgramObject& Object()				{ return module.Object(); }
		virtual IModuleBuilder& Module()  				{ return module; }
		virtual IPublicProgramObject& Object()	const	{ return module.Object(); }
		virtual const IFunctionCode& Code() const		{ return *builder; }
		virtual const IFunction* Parent() const			{ return parent; }
		virtual IFunctionBuilder* Parent()				{ return parent; }
		virtual ICodeBuilder& Builder()					{ return *builder; }

		virtual const IArgument& Arg(int32 index) const	{ return *args[index]; }

		virtual IArgumentBuilder& AddInput(const NameString& name, const TypeString& type, void* userdata)
		{
			FunctionArgument* a = new FunctionArgument(name, type, ARGDIRECTION_INPUT, *this, userdata);
			args.push_back(a);			
			inputCount++;
			return *a;
		}

		virtual IArgumentBuilder& AddClosureInput(const NameString& name, const TypeString& type, void* userdata)
		{
			FunctionArgument* a = new FunctionArgument(name, type, ARGDIRECTION_INPUT, *this, userdata);
			a->MakeClosureInput();
			args.push_back(a);
			inputCount++;
			return *a;
		}

		virtual IArgumentBuilder& AddInput(const NameString& name, const IStructure& type, void* userdata)
		{
			FunctionArgument* a = new FunctionArgument(name, type, ARGDIRECTION_INPUT, *this, userdata);
			args.push_back(a);			
			inputCount++;
			return *a;
		}

		virtual IArgumentBuilder& AddInput(const NameString& name, const TypeString& genericType, const TypeString& genericArgType1, void* userdata)
		{
			FunctionArgument* a = new FunctionArgument(name, genericType, genericArgType1, ARGDIRECTION_INPUT, *this, userdata);
			args.push_back(a);			
			inputCount++;
			return *a;
		}

		virtual IArgumentBuilder& AddInput(const NameString& name, const TypeString& genericType, const TypeString& genericArgType1,  const TypeString& genericArgType2, void* userdata)
		{
			FunctionArgument* a = new FunctionArgument(name, genericType, genericArgType1, genericArgType2, ARGDIRECTION_INPUT, *this, userdata);
			args.push_back(a);			
			inputCount++;
			return *a;
		}

		virtual IArgumentBuilder& AddOutput(const NameString& name, const IStructure& type, void* userdata)
		{
			FunctionArgument* a = new FunctionArgument(name, type, ARGDIRECTION_OUTPUT, *this, userdata);
			args.push_back(a);
			outputCount++;
			return *a;
		}

		virtual IArgumentBuilder& AddOutput(const NameString& name, const TypeString& type, void* userdata)
		{
			FunctionArgument* a = new FunctionArgument(name, type, ARGDIRECTION_OUTPUT, *this, userdata);
			args.push_back(a);
			outputCount++;
			return *a;
		}

		virtual const IArgument* GetArgumentByName(csexstr name) const
		{
			for(auto i = args.begin(); i != args.end(); ++i)
			{
				const IArgument* a = *i;
				if (AreEqual(a->Name(), name))
				{
					return a;
				}
			}

			return NULL;
		}

		bool TryResolveArguments()
		{
			for(auto i = args.begin(); i != args.end(); i++)
			{
				FunctionArgument* arg = *i;
				if (!arg->TryResolveArgument())
				{
					return false;
				}
			}

			return true;
		}
	};
}