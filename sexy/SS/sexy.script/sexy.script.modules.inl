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

namespace
{
	class CScript;
	class CScripts;

	struct CBindNSExpressionToModule
	{
		const ISExpression* E;
		CScript* Module;
		INamespace* NS;
	};

	inline bool operator < (const CBindNSExpressionToModule& a, const CBindNSExpressionToModule& b)
	{
		return a.E->String()->Length < b.E->String()->Length;
	}

	typedef std::vector<CBindNSExpressionToModule> TNamespaceDefinitions;

	bool IsIStringInlined(CScripts& scripts);

	class PredHasModule
	{
	private:
		const CScript* module;

	public:
		PredHasModule(const CScript* _module): module(_module)	{}
		const bool operator() (const CBindNSExpressionToModule& A) const
		{
			return module == A.Module;
		}
	};

	struct CBindFnDefToExpression
	{
		const ISExpression* FnDef;
		IFunctionBuilder* Fn;
	};
	typedef std::tr1::unordered_map<CStringKey,CBindFnDefToExpression,hashCStringKey> TFunctionDefinitions;

	struct CBindStructDefToExpression
	{
		const ISExpression* StructDef;
		IStructureBuilder* Struct;
	};
	typedef std::vector<CBindStructDefToExpression> TStructureDefinitions;

	void Resolve(IArgumentBuilder& arg, csexstr type, csexstr id, csexstr source, cr_sex e, csexstr genericArgType = NULL)
	{
		if (!arg.TryResolveArgument())
		{
			sexstringstream streamer;

			if (genericArgType != NULL)
			{
				streamer << SEXTEXT("Unknown type in argument (***") << type << SEXTEXT("*** ") << genericArgType << SEXTEXT(" ") << id << SEXTEXT(") of function ") << source << SEXTEXT("\r\n");
			}
			else
			{
				streamer << SEXTEXT("Unknown type in argument (***") << type << SEXTEXT("*** ") << id << SEXTEXT(") of function ") << source << SEXTEXT("\r\n");
			}
			
			Throw(e, streamer);
		}
	}

	IStructureBuilder& DeclareStructure(IModuleBuilder& module, csexstr structName, const StructurePrototype& prototype, cr_sex src)
	{
		try
		{
			IStructureBuilder& s = module.DeclareStructure(structName, prototype, &src);
			return s;
		}
		catch (IException& e)
		{
			sexstringstream streamer;
			streamer << e.Message();
			Throw(src, streamer);

			IStructureBuilder *s = NULL;
			return *s;
		}
	}

	INamespaceBuilder& ValidateSplitTail(REF NamespaceSplitter& splitter, OUT csexstr& body, OUT csexstr& publicName, IN cr_sex s, IN IProgramObject& po)
	{
		if (!splitter.SplitTail(OUT body, OUT publicName))
		{
			Throw(s, SEXTEXT("Expected fully qualified name 'A.B.C.D'"));
		}

		INamespaceBuilder* ns = po.GetRootNamespace().FindSubspace(body);
		if (ns == NULL)
		{
			sexstringstream streamer;
			streamer << "Cannot find namespace '" << body << "' that prefixes the public name: " << publicName;
			Throw(s, streamer);
		}		

		return *ns;
	}

	void AppendAlias(IModuleBuilder& module, cr_sex nsName, cr_sex nameExpr)
	{
		NamespaceSplitter splitter(nsName.String()->Buffer);

		csexstr body, tail;
		INamespaceBuilder& ns = ValidateSplitTail(splitter, body, tail, nsName, module.Object());

		csexstr publicFunctionName = tail;

		csexstr name = nameExpr.String()->Buffer;

		IFunctionBuilder* f = module.FindFunction(name);
		if (f == NULL)
		{
			NamespaceSplitter splitter(name);

			csexstr nsBody, shortName;
			if (splitter.SplitTail(nsBody, shortName))
			{
				INamespaceBuilder* nsSrc = MatchNamespace(module, nsBody);
				if (nsSrc == NULL)
				{
					sexstringstream streamer;
					streamer << SEXTEXT("Cannot resolve alias. Source name '") << nsBody << SEXTEXT("' was not a reconigzed namespace");
					Throw(nameExpr, streamer);
				}

				IStructureBuilder* s = nsSrc->FindStructure(shortName);
				if (s == NULL)
				{
					sexstringstream streamer;
					streamer << SEXTEXT("Cannot find '") << shortName << SEXTEXT("' in ") << nsBody;
					Throw(nameExpr, streamer);
				}

				ns.Alias(tail, *s);

				return;
			}

			IStructureBuilder* s = module.FindStructure(name);
			if (s == NULL)
			{
				sexstringstream streamer;
				streamer << SEXTEXT("Cannot resolve alias. Local name '") << name << SEXTEXT("' was neither a structure or a function");
				Throw(nameExpr, streamer);
			}
			else
			{
				if (s->Prototype().IsClass)
				{
					sexstringstream streamer;
					streamer << SEXTEXT("Aliasing a class is not allowed: '") << name << SEXTEXT("'");
					Throw(nameExpr, streamer);
				}
				else
				{
					ns.Alias(tail, *s);
				}
			}
		}
		else
		{
			ns.Alias(tail, *f);
		}
	}

	void AppendAliases(IModuleBuilder& module, IN const ISParserTree& tree)
	{
		cr_sex root = tree.Root();			
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = root.GetElement(i);
			cr_sex elementNameExpr = GetAtomicArg(topLevelItem, 0);
			csexstr elementName = elementNameExpr.String()->Buffer;
			if (AreEqual(elementName, SEXTEXT("alias")))
			{
				// (alias <qualified-name> <name>)
				AssertNotTooFewElements(topLevelItem, 3);
				AssertNotTooManyElements(topLevelItem, 3);

				cr_sex localName = GetAtomicArg(topLevelItem, 1);		
				cr_sex nsName = GetAtomicArg(topLevelItem, 2);					

				AppendAlias(REF module, IN nsName, IN localName);
			}
		}
	}

	int GetIndexOf(int start, cr_sex s, csexstr text)
	{
		for(int i = start; i < s.NumberOfElements(); ++i)
		{
			cr_sex child = s.GetElement(i);
			if (child.Type() == EXPRESSION_TYPE_ATOMIC)
			{
				if (AreEqual(child.String(), text))
				{
					return i;
				}
			}
		}

		return -1;
	}

	void AddGenericInput(IFunctionBuilder& f, csexstr type, cr_sex id, cr_sex elementType, cr_sex src, CScript& script)
	{
		AssertQualifiedIdentifier(elementType);
		AssertLocalIdentifier(id);

		const sexstring idString = id.String();
		const sexstring firstTypeString = elementType.String();

		if (!AreEqual(type, SEXTEXT("_Array")) && ! AreEqual(type, SEXTEXT("_List")))
		{
			Throw(*id.Parent(), SEXTEXT("Unexpected type in generic input definition"));
		}
	
		IArgumentBuilder& arg = f.AddInput(NameString::From(idString), TypeString::From(type), TypeString::From(firstTypeString), (void*) &src);			
		Resolve(arg, type, idString->Buffer, f.Name(), *id.Parent(), firstTypeString->Buffer);

		if (AreEqual(type, SEXTEXT("_Array")))
		{
			AddArrayDef(script, f.Builder(), id.String()->Buffer, *arg.GenericTypeArg1(), src);
		}
		else if (AreEqual(type, SEXTEXT("_List")))
		{
			AddListDef(script, f.Builder(), id.String()->Buffer, *arg.GenericTypeArg1(), src);
		}
	}

	void AddGenericInput(IFunctionBuilder& f, csexstr type, cr_sex id, cr_sex SType, cr_sex TType, cr_sex src, CScript& script)
	{
		AssertQualifiedIdentifier(SType);
		AssertQualifiedIdentifier(TType);
		AssertLocalIdentifier(id);

		const sexstring idString = id.String();
		const sexstring firstTypeString = SType.String();
		const sexstring secondTypeString = TType.String();

		if (!AreEqual(type, SEXTEXT("_Map")))
		{
			Throw(*id.Parent(), SEXTEXT("Unexpected type in generic input definition"));
		}
	
		IArgumentBuilder& arg = f.AddInput(NameString::From(idString), TypeString::From(type), TypeString::From(firstTypeString), TypeString::From(secondTypeString), (void*) &src);			
		Resolve(arg, type, idString->Buffer, f.Name(), *id.Parent(), firstTypeString->Buffer);

		if (AreEqual(type, SEXTEXT("_Map")))
		{
			AddMapDef(script, f.Builder(), id.String()->Buffer, *arg.GenericTypeArg1(), *arg.GenericTypeArg2(), src);
		}
	}

	void AddClosureInput(IFunctionBuilder& f, cr_sex type, cr_sex id, cr_sex src)
	{
		AssertQualifiedIdentifier(type);
		AssertLocalIdentifier(id);

		const sexstring typeString = type.String();
		const sexstring idString = id.String();

		IArgumentBuilder& arg = f.AddClosureInput(NameString::From(idString), TypeString::From(typeString), (void*)&src);
		Resolve(arg, typeString->Buffer, idString->Buffer, f.Name(), type);

		if (arg.ResolvedType()->VarType() != VARTYPE_Closure)
		{
			Throw(type, SEXTEXT("The type must be an archetype in a closure input"));
		}
	}

	void AddInput(IFunctionBuilder& f, cr_sex type, cr_sex id, cr_sex src)
	{
		AssertQualifiedIdentifier(type);
		AssertLocalIdentifier(id);

		const sexstring typeString = type.String();
		const sexstring idString = id.String();

		IArgumentBuilder& arg = f.AddInput(NameString::From(idString), TypeString::From(typeString), (void*) &src);			
		Resolve(arg, typeString->Buffer, idString->Buffer, f.Name(), type);
	}

	void AddOutput(IFunctionBuilder& f, cr_sex type, cr_sex id, cr_sex src)
	{
		AssertQualifiedIdentifier(type);
		AssertLocalIdentifier(id);

		const sexstring typeString = type.String();
		const sexstring idString = id.String();

		IArgumentBuilder& arg = f.AddOutput(NameString::From(idString), TypeString::From(typeString), (void*) &src);
		Resolve(arg, typeString->Buffer, idString->Buffer, f.Name(), type);
	}

	IStructure* GetLocalStructure(CScript& script, csexstr className);

	void AddChildConstructor(cr_sex childConstruct)
	{
		// bang();
	}

	void AddThisPointer(REF IFunctionBuilder& method, cr_sex methodDef, CScript& script)
	{
		csexstr name = method.Name();

		NamespaceSplitter splitter(name);

		cr_sex nameExpr = GetAtomicArg(methodDef, 1);

		csexstr className, methodName;
		if (!splitter.SplitTail(OUT className, OUT methodName))
		{
			Throw(nameExpr, SEXTEXT("Expecting qualified method name using syntax: '<class>.<method-name>'"));
		}

		IStructure* s = GetLocalStructure(script, className);
		if (s == NULL)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Could not find a matching class definition inside the module for ") << className;
			Throw(nameExpr, streamer);
		}

		/*if (!s->Prototype().IsClass) //TODO -> delete
		{
			sexstringstream streamer;
			streamer << SEXTEXT("The object matching the name was a struct, not a class: ") << className;
			Throw(nameExpr, streamer);
		}*/

		IArgumentBuilder& arg = method.AddInput(NameString::From(THIS_POINTER_TOKEN), TypeString::From(className), (void*) &nameExpr);			
		Resolve(arg, className, THIS_POINTER_TOKEN, method.Name(), nameExpr);	
	}

	void AddInputs(REF IFunctionBuilder& f, cr_sex fdef, int inputStart, int inputEnd, CScript& script)
	{
		for(int i = inputStart; i <= inputEnd; ++i)
		{
			cr_sex inputItem = fdef.GetElement(i);
			AssertNotTooFewElements(inputItem, 2);
			AssertNotTooManyElements(inputItem, 4);

			cr_sex sexType = GetAtomicArg(inputItem, 0);

			if (AreEqual(sexType.String(), SEXTEXT("array")))
			{
				AssertNotTooFewElements(inputItem, 3);
				AssertNotTooManyElements(inputItem, 3);

				cr_sex elementType = GetAtomicArg(inputItem, 1);
				cr_sex sexIdentifier = GetAtomicArg(inputItem, 2);

				AddGenericInput(f, SEXTEXT("_Array"), sexIdentifier, elementType, inputItem, script);				
			}
			else if (AreEqual(sexType.String(), SEXTEXT("list")))
			{
				AssertNotTooFewElements(inputItem, 3);
				AssertNotTooManyElements(inputItem, 3);

				cr_sex elementType = GetAtomicArg(inputItem, 1);
				cr_sex sexIdentifier = GetAtomicArg(inputItem, 2);
				
				AddGenericInput(f, SEXTEXT("_List"), sexIdentifier, elementType, inputItem, script);				
			}
			else if (AreEqual(sexType.String(), SEXTEXT("map")))
			{
				AssertNotTooFewElements(inputItem, 4);
				cr_sex keyType = GetAtomicArg(inputItem, 1);
				cr_sex valueType = GetAtomicArg(inputItem, 2);
				cr_sex sexIdentifier = GetAtomicArg(inputItem, 3);

				AddGenericInput(f, SEXTEXT("_Map"), sexIdentifier, keyType, valueType, inputItem, script);
			}
			else if (AreEqual(sexType.String(), SEXTEXT("closure")))
			{
				AssertNotTooFewElements(inputItem, 3);
				AssertNotTooManyElements(inputItem, 3);

				cr_sex sArchetype = GetAtomicArg(inputItem, 1);
				cr_sex sId = GetAtomicArg(inputItem, 2);
				AddClosureInput(f, sArchetype, sId, inputItem);
			}
			else
			{
				AssertNotTooManyElements(inputItem, 2);
				cr_sex sexIdentifier = GetAtomicArg(inputItem, 1);
				AddInput(f, sexType, sexIdentifier, inputItem);
			}
		}	
	}

	void ValidateChildConstructorExists(int startPos, int endPos, cr_sex constructorDef, const IMember& m)
	{
		int constructCount = 0;

		if (startPos > endPos)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expected a child constructor indicator '->' after the constructor input arguments,\r\n followed by (construct ") << m.Name() << SEXTEXT(" arg1... argN) amongst the child constructors."); 
			Throw(constructorDef, streamer);
		}

		// We need a constructor for generic types
		for(int i = startPos; i <= endPos; ++i)
		{
			cr_sex childConstruct = constructorDef.GetElement(i);
			cr_sex childNameExpr = GetAtomicArg(childConstruct, 1);
			if (AreEqual(childNameExpr.String(), m.Name()))
			{
				constructCount++;
			}
		}

		if (constructCount == 0)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Could not find child constructor for ") << m.Name() << SEXTEXT(".\r\nExpected (construct ") << m.Name() << SEXTEXT(" arg1... argN) amongst the child constructors."); 
			Throw(constructorDef, streamer);
		}
		else if (constructCount > 1)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Conflicting child constructors for ") << m.Name();
			Throw(constructorDef, streamer);
		}
	}

	void ValidateChildConstructors(const IFunction& constructor, cr_sex constructorDef, int startPos, int endPos)
	{
		csexstr name = constructor.Name();

		NamespaceSplitter splitter(name);

		csexstr classType, constructMethod;
		splitter.SplitTail(classType, constructMethod);

		const IStructure* type = constructor.Module().FindStructure(classType);
		for(int i = 0; i < type->MemberCount(); ++i)
		{
			const IMember& m = type->GetMember(i);
			if (m.UnderlyingGenericArg1Type() != NULL) 
			{
				// Generic containers need to be constructed
				ValidateChildConstructorExists(startPos, endPos, constructorDef, m);
			}
			else
			{
				// If a member has a constructor then we should use that constructor
				const IFunction* memberConstructor = m.UnderlyingType()->Constructor();
				if (memberConstructor != NULL)
				{
					ValidateChildConstructorExists(startPos, endPos, constructorDef, m);
				}
			}
		}
	}

	void CompileConstructor(REF IFunctionBuilder& constructor, IN cr_sex constructorDef, CScript& script)
	{
		// (method <Classname>.Construct (inputType1 inputVar1) ... (inputTypeN inputVarN) -> (construct <child1> child1Args) ... (construct <child2> child1Args): body )

		// Arg0 = method
		// Arg1 = <Classname>.Construct
		// Args2...N = input list
		// ArgN+1 = ->
		// ArgN+2 = member construct list
		// ArgN+3= :
		// ArgN+4...N' = constructor body
		
		csexstr name = constructor.Name();

		IStructureBuilder* type = (IStructureBuilder*) constructor.GetType();
		type->SetConstructor(&constructor);

		AssertNotTooFewElements(constructorDef, 3); // -> Minimist constructor: (method Eros.Construct :)
		
		int bodyIndex = GetIndexOf(2, constructorDef, SEXTEXT(":"));					
		if (bodyIndex < 0)
		{			
			Throw(constructorDef, SEXTEXT("Expecting body indication token ':' inside the constructor definition"));
		}

		int childConstructIndex = GetIndexOf(2, constructorDef, SEXTEXT("->"));
		if (childConstructIndex < 0)
		{
			childConstructIndex = bodyIndex;
		}	
		else if (childConstructIndex > bodyIndex)
		{
			Throw(constructorDef, SEXTEXT("Expecting child construction token '->' to occur before the body indication token ':' inside the constructor definition"));
		}

		int childConstructorStart = childConstructIndex+1;
		int childConstructorEnd = bodyIndex-1;

		AddInputs(constructor, constructorDef, 2, childConstructIndex-1, script);
		AddThisPointer(constructor, constructorDef, script);
	}

	void AddOutputs(REF IFunctionBuilder& f, int outputStart, int outputEnd, cr_sex fdef)
	{
		for(int i = outputStart; i <= outputEnd; ++i)
		{
			cr_sex outputItem = fdef.GetElement(i);
			AssertCompound(outputItem);
			AssertNotTooFewElements(outputItem, 2);
			AssertNotTooManyElements(outputItem, 2);

			cr_sex sexType = GetAtomicArg(outputItem, 0);
			cr_sex sexIdentifier = GetAtomicArg(outputItem, 1);

			AddOutput(f, sexType, sexIdentifier, outputItem);
		}
	}

	bool IsConstructor(const IFunction& f)
	{		
		if (f.IsVirtualMethod())
		{
			csexstr name = f.Name();

			NamespaceSplitter splitter(name);
			csexstr path,publicName;
			if (splitter.SplitTail(path, publicName))
			{
				if (AreEqual(publicName, SEXTEXT("Construct")))
				{
					return true;
				}
			}
		}

		return false;
	}

	void CompileDeclaration(REF IFunctionBuilder& f, IN cr_sex fdef, CScript& script)
	{
		// function <name> (inputType1 inputVar1) ... (inputTypeN inputVarN) -> (outputType1 outputVar1) ... (outputTypeN outputVarN): body )

		// Arg0 = function
		// Arg1 = <name>
		// Args2...N = input list
		// ArgN+1 = ->
		// ArgN+2 = output list
		// ArgN+3= :
		// ArgN+4...N' = function body
		
		// First stack the outputs, then the inputs

		// method <class>.<method-name> (inputType1 inputVar1) ... (inputTypeN inputVarN) -> (outputType1 outputVar1) ... (outputTypeN outputVarN): body )

		NamespaceSplitter splitter(f.Name());

		csexstr classTypeName, methodName;
		if (splitter.SplitTail(classTypeName, methodName))
		{
			const IStructure* s = f.Module().FindStructure(classTypeName);
			if (s == NULL)
			{
				sexstringstream streamer;
				streamer <<  SEXTEXT("Unknown structure: ") << classTypeName;
				Throw(fdef, streamer);
			}

			f.SetType(s);
		}

		if (IsConstructor(f))
		{
			CompileConstructor(f, fdef, script);
			return;
		}

		csexstr name = f.Name();

		AssertNotTooFewElements(fdef, 4);
	
		int mapIndex = GetIndexOf(2, fdef, SEXTEXT("->"));

		if (mapIndex < 0)
		{			
			Throw(fdef, SEXTEXT("Expecting mapping token '->' inside the function definition"));
		}

		int bodyIndex = GetIndexOf(mapIndex, fdef, SEXTEXT(":"));
		if (bodyIndex == -1) Throw(fdef, SEXTEXT("Expecting body indicator token ':' after the mapping token and inside the function definition"));

		AddOutputs(f, mapIndex+1, bodyIndex-1, fdef);
		AddInputs(f, fdef, 2, mapIndex-1, script);

		if (f.IsVirtualMethod())
		{
			AddThisPointer(REF f, fdef, script);
		}		
	}

	void AppendInvokeCallDestructor(CCompileEnvironment& ce, const IStructure& s, csexstr name, int SFoffset)
	{
		TokenBuffer destructorName;
		StringPrint(destructorName, SEXTEXT("%s.Destruct"), s.Name());
		
		if (!IsNullType(s))
		{
			if (s == ce.StructList())
			{
				ce.Builder.Assembler().Append_GetStackFrameAddress(VM::REGISTER_D7, SFoffset);
				AppendInvoke(ce, GetListCallbacks(ce).ListClear, *(const ISExpression*) s.Definition());
				return;
			}

			if (s == ce.StructMap())
			{
				ce.Builder.Assembler().Append_GetStackFrameAddress(VM::REGISTER_D7, SFoffset);
				AppendInvoke(ce, GetMapCallbacks(ce).MapClear,  *(const ISExpression*) s.Definition());
				return;
			}

			// Regular class			
			const IFunction* f = s.Module().FindFunction(destructorName);
			if (f == NULL)
			{
				// concrete class without destructor
				return;
			}
		}
		
		TokenBuffer destructorSymbol;
		StringPrint(destructorSymbol, SEXTEXT("%s %s.Destruct"), GetFriendlyName(s), name);
		ce.Builder.AddSymbol(destructorSymbol);	
		ce.Builder.Assembler().Append_CallVirtualFunctionByAddress(SFoffset, sizeof(ID_BYTECODE));
		ce.Builder.Assembler().Append_Pop(sizeof(size_t));
	}

	void AppendInvokeDestructorChildren(CCompileEnvironment& ce, const IStructure& s, int SFoffset)
	{
		int subOffset = 0;

		for(int i = 0; i < s.MemberCount(); ++i)
		{
			const IMember& m = s.GetMember(i);			
			const IStructure& memberType = *m.UnderlyingType();

			if (IsPrimitiveType(memberType.VarType()) || m.IsPseudoVariable())
			{
				// primitive types are skipped, as are pseudo variables, as the pointers point to something that are destructed elsewhere
			}
			else	
			{
				AppendInvokeDestructorChildren(ce, memberType, SFoffset + subOffset);
				AppendInvokeCallDestructor(ce, memberType, m.Name(), SFoffset + subOffset);	
			}

			subOffset += m.SizeOfMember();
		}
	}

	void AppendInvokeDestructor(CCompileEnvironment& ce, csexstr instanceName, cr_sex sequence, const MemberDef& instanceDef)
	{
		if (instanceDef.AllocSize == 0) return; // Pseudo values describe things that are destructed by the compile system, not at run-time.

		const IStructure& s = *instanceDef.ResolvedType;

		if (s == ce.StructArray())
		{
			CompileArrayDestruct(ce, s, instanceName);		
			return;
		}
		else if (s == ce.StructList())
		{
			ce.Builder.AssignVariableRefToTemp(instanceName, Sexy::ROOT_TEMPDEPTH);
			AppendInvoke(ce, GetListCallbacks(ce).ListClear, sequence);
			return;
		}
		else if (s == ce.StructMap())
		{
			ce.Builder.AssignVariableRefToTemp(instanceName, Sexy::ROOT_TEMPDEPTH);
			AppendInvoke(ce, GetMapCallbacks(ce).MapClear, sequence);
			return;
		}

		AppendInvokeDestructorChildren(ce, s, instanceDef.SFOffset);
		AppendInvokeCallDestructor(ce, s, instanceName, instanceDef.SFOffset);
	}

	void AppendDeconstructAll(CCompileEnvironment& ce, cr_sex sequence)
	{
		int backRef = ce.Builder.GetVariableCount() - 1;

		int numberOfVariables = ce.Builder.GetVariableCount() - ce.Builder.Owner().NumberOfInputs() - ce.Builder.Owner().NumberOfOutputs();

		for(int i = numberOfVariables; i > 0; i--)
		{
			if (backRef < 0) Throw(sequence, SEXTEXT("Algorithmic error #1 in deconstruction logic"));

			MemberDef def;
			csexstr instanceName;
			ce.Builder.GetVariableByIndex(OUT def, OUT instanceName, backRef);

			if (def.SFOffset < 0) Throw(sequence, SEXTEXT("Algorithmic error #2 in deconstruction logic"));

			if (def.Usage == ARGUMENTUSAGE_BYVALUE && def.ResolvedType->VarType() == VARTYPE_Derivative)
			{
				AppendInvokeDestructor(ce, instanceName, sequence, def);
			}

			backRef--;
		}

		ce.Builder.PopLastVariables(numberOfVariables);
	}

	void AppendDeconstruct(CCompileEnvironment& ce, cr_sex sequence)
	{
		int backRef = ce.Builder.GetVariableCount() - 1;

		int numberOfVariables = ce.Builder.GetVariableCount() - ce.Builder.SectionArgCount();

		for(int i = numberOfVariables; i > 0; i--)
		{
			if (backRef < 0) Throw(sequence, SEXTEXT("Algorithmic error #1 in deconstruction logic"));

			MemberDef def;
			csexstr instanceName;
			ce.Builder.GetVariableByIndex(OUT def, OUT instanceName, backRef);
			
			if (def.SFOffset < 0) Throw(sequence, SEXTEXT("Algorithmic error #2 in deconstruction logic"));

			if (def.Usage == ARGUMENTUSAGE_BYVALUE && def.ResolvedType->VarType() == VARTYPE_Derivative)
			{
				AppendInvokeDestructor(ce, instanceName, sequence, def);
			}
				
			if (*def.ResolvedType == ce.Object.Common().TypeNode())
			{
				// Expecting node to be by ARGUMENTUSAGE_BYVALUE and allocsize to be zero, as the only such here are pseudo variables
				// After the pseudo variable we can expect the actual reference, a pointer type by value
				
				ce.Builder.AssignVariableRefToTemp(instanceName, Sexy::ROOT_TEMPDEPTH);
				AppendInvoke(ce, GetListCallbacks(ce).NodeReleaseRef, sequence); // release the ref to the node
			}
			else if (*def.ResolvedType == ce.Object.Common().TypeMapNode())
			{
				// Expecting node to be by ARGUMENTUSAGE_BYVALUE and allocsize to be zero, as the only such here are pseudo variables
				// After the pseudo variable we can expect the actual reference, a pointer type by value
				
				ce.Builder.AssignVariableRefToTemp(instanceName, Sexy::ROOT_TEMPDEPTH);
				AppendInvoke(ce, GetMapCallbacks(ce).MapNodeReleaseRef, sequence); // release the ref to the node
			}
			backRef--;			
		}

		ce.Builder.PopLastVariables(numberOfVariables);
	}

	void CompileExpressionSequenceProtected(CCompileEnvironment& ce, int start, int end, cr_sex sequence)
	{
		if (sequence.Type() == EXPRESSION_TYPE_NULL)
		{
			ce.Builder.Assembler().Append_NoOperation();
			return;
		}

		AssertCompound(sequence);
		if (start == 0 && end == sequence.NumberOfElements()-1 && sequence.NumberOfElements() > 0)
		{
			cr_sex firstArg = sequence.GetElement(0);
			if (firstArg.Type() == EXPRESSION_TYPE_ATOMIC)
			{
				ce.Builder.EnterSection();
				CompileExpression(ce, sequence); 
				AppendDeconstruct(ce, sequence);
				ce.Builder.LeaveSection();
				return;
			}
		}

		ce.Builder.EnterSection();

		// Compound expression ( (..) (...) ... )
		for(int i = start; i <= end; i++)
		{
			cr_sex s = sequence.GetElement(i);
			CompileExpression(ce, s); 
		}

		AppendDeconstruct(ce, sequence);

		ce.Builder.LeaveSection();
	}

	void CompileExpressionSequence(CCompileEnvironment& ce, int start, int end, cr_sex sequence)
	{
		try
		{
			CompileExpressionSequenceProtected(ce, start, end, sequence);
		}
		catch(STCException& ex)
		{
			sexstringstream streamer;
			StreamSTCEX(streamer, ex);
			Throw(sequence, streamer.str().c_str());
		}
	}

	int GetInterfaceImplementingMethod(const IStructure& s, csexstr methodName)
	{
		for(int j = 0; j < s.InterfaceCount(); ++j)
		{
			const IInterface& interf = s.GetInterface(j);
			for(int i = 0; i < interf.MethodCount(); ++i)
			{
				const IArchetype& a = interf.GetMethod(i);
				if (AreEqual(a.Name(), methodName))
				{
					return j;
				}
			}
		}

		return -1;
	}

	int ComputeThisOffset(const IFunction& f, cr_sex fdef)
	{
		if (!f.IsVirtualMethod())
		{
			return 0;
		}

		csexstr className, methodName;

		NamespaceSplitter splitter(f.Name());
		if (!splitter.SplitTail(OUT className, OUT methodName))
			Throw(fdef, SEXTEXT("Internal compiler error. Expecting function name to be splittable"));

		if (f.NumberOfInputs() <= 0) Throw(fdef, SEXTEXT("No inputs found in method!"));

		const IStructure& s = f.GetArgument(ArgCount(f)-1);

		if (&s == NULL)
			Throw(fdef, SEXTEXT("Expecting class to be defined in the same module in which the class-method is defined"));

		if (AreEqual(methodName, SEXTEXT("Destruct")))
		{
			// Destructors are from the vTable aligned to the instance, ergo no correction
			return 0;
		}

		if (AreEqual(methodName, SEXTEXT("Construct")))
		{
			// Concrete constructors always take the instance in the this pointer, never an interface, ergo no correction
			return 0;
		}

		int interfIndex = GetInterfaceImplementingMethod(s, methodName);
		if (interfIndex < 0)
			Throw(fdef, SEXTEXT("Expecting method to be found amongst the interfaces of the class for which it is defined"));

		int correction =  sizeof(int32) + (interfIndex+1) * sizeof(void*); //TODO -> Use class structure to calculate offsets.
		return correction;
	}

	void* GetInterfacePtrFromNullInstancePtr(void* instancePtr)
	{
		CClassHeader* header = (CClassHeader*) instancePtr;
		return &header->_vTables[0];
	}

	void CompileSetOutputRefToUniversalNullObjects(REF IFunctionBuilder& f)
	{
		// Initialize output refs to null objects
		for(int i = 0; i < f.NumberOfOutputs(); ++i)
		{
			const IArgument& arg = f.Arg(i);
			if (arg.Direction() == ARGDIRECTION_OUTPUT)
			{
				const IStructure& argType = *arg.ResolvedType();
				if (IsNullType(argType))
				{
					MemberDef def;
					f.Builder().TryGetVariableByName(OUT def, arg.Name());

					VariantValue nullPtr;
					nullPtr.vPtrValue =  GetInterfacePtrFromNullInstancePtr(argType.GetInterface(0).UniversalNullInstance());

					SEXCHAR symbol[128];
					StringPrint(symbol, 128, SEXTEXT("%s = null object"), arg.Name());
					f.Builder().AddSymbol(symbol);
					f.Builder().Assembler().Append_SetStackFrameImmediate(def.SFOffset, nullPtr, BITCOUNT_POINTER);
				}
			}
			else
			{
				OS::BreakOnThrow();
			}
		}
	}

	void ConstructMemberByRef(CCompileEnvironment& ce, cr_sex args, int tempDepth, const IStructure& type, int offset)
	{
		if (args.NumberOfElements() != type.MemberCount())
		{
			sexstringstream streamer;
			streamer << SEXTEXT("The number of arguments supplied in the memberwise constructor is ") << args.NumberOfElements()
					 << SEXTEXT(", while the number of members in ") << GetFriendlyName(type) << SEXTEXT(" is ") << type.MemberCount();
			Throw(args, streamer);
		}

		for(int i = 0; i < type.MemberCount(); ++i)
		{
			cr_sex arg = args.GetElement(i);
			const IMember& m = type.GetMember(i);
			const IStructure& mtype = *m.UnderlyingType();
			if (IsPrimitiveType(mtype.VarType()))
			{
				// ToDo -> remove archiving when the assembly does not overwrite any registers, save D7
				AddArchiveRegister(ce, VM::REGISTER_D4 + tempDepth, VM::REGISTER_D4 + tempDepth, BITCOUNT_POINTER);
				CompileNumericExpression(ce, arg, mtype.VarType()); // Numeric value in D7
				ce.Builder.PopLastVariables(1); // VM::REGISTER_D4 + tempDepth contains the value pointer
				ce.Builder.Assembler().Append_Poke(VM::REGISTER_D7, GetBitCount(mtype.VarType()), VM::REGISTER_D4 + tempDepth, offset); // write the argument to the member
				
			}
			else if (mtype.VarType() == VARTYPE_Derivative)
			{
				if (mtype.Prototype().IsClass)
				{
					Throw(arg, SEXTEXT("Internal Compiler Error. The member type was a class, which cannot be memberwise constructed"));
				}

				ConstructMemberByRef(ce, arg, tempDepth, mtype, offset);
			}
			else
			{
				Throw(arg, SEXTEXT("Internal Compiler Error. ConstructMemberByRef found an unusual member type"));
			}

			offset += mtype.SizeOfStruct();
		}
	}

	void CompileListConstruct(CCompileEnvironment& ce, cr_sex def, const IMember& member, csexstr fullName)
	{
		ce.Builder.AssignVariableRefToTemp(fullName, 0); // List goes to D4

		VariantValue v;
		v.vPtrValue = (void*) member.UnderlyingGenericArg1Type();
		ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, v, BITCOUNT_POINTER); // Element type to D5
				
		AddListDef(ce.Script, ce.Builder, fullName, *member.UnderlyingGenericArg1Type(), def);

		ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).ListInit);		
	}

	void CompileMapConstruct(CCompileEnvironment& ce, cr_sex def, const IMember& member, csexstr fullName)
	{
		ce.Builder.AssignVariableRefToTemp(fullName, Sexy::ROOT_TEMPDEPTH); // Map goes to D7

		VariantValue key;
		key.vPtrValue = (void*) member.UnderlyingGenericArg1Type();
		ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, key, BITCOUNT_POINTER); // Key type to D5

		VariantValue val;
		val.vPtrValue = (void*) member.UnderlyingGenericArg2Type();
		ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, val, BITCOUNT_POINTER); // Value type to D4
				
		ce.Builder.Assembler().Append_Invoke(GetMapCallbacks(ce).MapInit);		
	}

	void CompileArrayConstruct(CCompileEnvironment& ce, cr_sex conDef, const IMember& member, csexstr fullName)
	{
		AssertNotTooManyElements(conDef, 3);
		cr_sex value = conDef.GetElement(2);
		CompileNumericExpression(ce, value, VARTYPE_Int32); // The capacity is now in D7

		ce.Builder.AssignVariableRefToTemp(fullName, 1); // Array goes to D5

		VariantValue v;
		v.vPtrValue = (void*) member.UnderlyingGenericArg1Type();
		ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_POINTER); // Element type to D4
				
		AddArrayDef(ce.Script, ce.Builder, fullName, *member.UnderlyingGenericArg1Type(), conDef);

		ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayInit);		
	}

	void CompileInvokeChildConstructor(CCompileEnvironment& ce, cr_sex conDef, const IMember& member, csexstr instance)
	{		
		// (construct member <arg1>...<argN>)
		const IFunction* childConstructorFn = member.UnderlyingType()->Constructor();
		if (childConstructorFn == NULL) ThrowTokenNotFound(conDef, member.UnderlyingType()->Name(), member.UnderlyingType()->Module().Name(), SEXTEXT("Constructor"));

		int inputCount = childConstructorFn->NumberOfInputs();

		if (inputCount < conDef.NumberOfElements() - 1)
		{
			Throw(conDef, SEXTEXT("Too many arguments supplied to the child constructor"));
		}
		else if (inputCount > conDef.NumberOfElements() - 1)
		{
			Throw(conDef, SEXTEXT("Insufficient arguments supplied to the child constructor"));
		}


		int inputStackAllocCount = PushInputs(ce, conDef, *childConstructorFn, true, 2);		
		inputStackAllocCount += CompileInstancePointerArg(ce, instance);

		AppendFunctionCallAssembly(ce, *childConstructorFn); 

		ce.Builder.MarkExpression(&conDef);

		RepairStack(ce, *conDef.Parent(), *childConstructorFn);
		ce.Builder.AssignClosureParentSF();
	}

	void CompileConstructMember(CCompileEnvironment& ce, cr_sex conDef, const IMember& member, csexstr instance)
	{
		if (*member.UnderlyingType() == ce.StructArray())
		{
			CompileArrayConstruct(ce, conDef, member, instance);
		}
		else if (*member.UnderlyingType() == ce.StructList())
		{
			CompileListConstruct(ce, conDef, member, instance);
		}
		else if (*member.UnderlyingType() == ce.StructMap())
		{
			CompileMapConstruct(ce, conDef, member, instance);
		}
		else
		{
			CompileInvokeChildConstructor(ce, conDef, member, instance);
		}
	}

	void CompileConstructChild(CCompileEnvironment& ce, cr_sex conDef, const IStructure& parentType, csexstr parentInstance)
	{
		AssertNotTooFewElements(conDef, 3);
		
		cr_sex constructDirective = GetAtomicArg(conDef, 0);
		if (!AreEqual(constructDirective.String(), SEXTEXT("construct")))
		{
			Throw(constructDirective, SEXTEXT("Expecting 'construct' keyword in this directive"));
		}

		cr_sex fieldNameExpr = GetAtomicArg(conDef, 1);
		csexstr fieldName = fieldNameExpr.String()->Buffer;

		TokenBuffer fullFieldName;
		StringPrint(fullFieldName, SEXTEXT("%s.%s"), parentInstance, fieldName);

		int offset = 0;
		const IMember* member = FindMember(parentType, fieldName, OUT offset);
		if (member == NULL) ThrowTokenNotFound(fieldNameExpr, fieldName, parentType.Name(), SEXTEXT("member"));
		
		CompileConstructMember(ce, conDef, *member, fullFieldName);
	}

	void CompileConstructChildren(CCompileEnvironment& ce, int firstChildConstructIndex, int endChildConstructIndex, cr_sex constructorDef, const IStructure& parentType)
	{
		for(int i = firstChildConstructIndex; i <= endChildConstructIndex; ++i)
		{
			cr_sex childConstructDef = constructorDef.GetElement(i);
			CompileConstructChild(ce, childConstructDef, parentType, SEXTEXT("this"));
		}
	}

	void CompileConstructorFromExpression(REF IFunctionBuilder& constructor, IN cr_sex constructorDef, CScript& script)
	{
		csexstr name = constructor.Name();

		// (method <type-name>.Construct  (inputType1 inputVar1) ... (inputTypeN inputVarN) -> (construct <memberName1> arg1 arg2 arg3 ) ... (construct <memberName2> arg1 arg2 arg3): body )

		int bodyIndex = GetIndexOf(2, constructorDef, SEXTEXT(":"));
		if (bodyIndex == -1) Throw(constructorDef, SEXTEXT("Expecting body indicator token ':' after the optional child construct token '->' and just before the constructor definition"));

		int mapIndex = GetIndexOf(2, constructorDef, SEXTEXT("->"));
		if (mapIndex > bodyIndex) Throw(constructorDef, SEXTEXT("Expecting construct token '->' before the body indicator token inside the constructor definition"));
		else if (mapIndex == -1) mapIndex = bodyIndex;

		ICodeBuilder& builder = constructor.Builder();
		
		// The final input is an interface
		int thisOffset = ComputeThisOffset(builder.Owner(), constructorDef);
		builder.SetThisOffset(thisOffset);

		NamespaceSplitter splitter(name);
		csexstr typeName, shortName;
		splitter.SplitTail(typeName, shortName);

		const IStructure* type = constructor.Module().FindStructure(typeName);

		ValidateChildConstructors(constructor, constructorDef, mapIndex+1, bodyIndex-1);
		
		builder.Begin();

			CCompileEnvironment ce(script, builder);

			builder.Append_InitializeVirtualTable(SEXTEXT("this"), *type);
			InitClassMembers(ce, SEXTEXT("this"));

			CompileConstructChildren(ce, mapIndex+1, bodyIndex-1, constructorDef, *type);

			CompileExpressionSequence(ce, bodyIndex+1, constructorDef.NumberOfElements()-1, constructorDef);

		builder.End();
		builder.Assembler().Clear();
	}

	void CompileFunctionFromExpression(REF IFunctionBuilder& f, IN cr_sex fdef, CScript& script)
	{
		// function <name> (inputType1 inputVar1) ... (inputTypeN inputVarN) -> (outputType1 outputVar1) ... (outputTypeN outputVarN): body )

		// Arg0 = function
		// Arg1 = <name>
		// Args2...N = input list
		// ArgN+1 = ->
		// ArgN+2 = output list
		// ArgN+3= :
		// ArgN+4...N' = function body

		// First stack the outputs, then the inputs

		csexstr fname = f.Name();

		if (IsConstructor(f))
		{
			 CompileConstructorFromExpression(REF f, fdef, script);
			 return;
		}

		int mapIndex = GetIndexOf(2, fdef, SEXTEXT("->"));
		if (mapIndex == -1) Throw(fdef, SEXTEXT("Expecting mapping token '->' inside the function definition"));

		int bodyIndex = GetIndexOf(mapIndex, fdef, SEXTEXT(":"));
		if (bodyIndex == -1) Throw(fdef, SEXTEXT("Expecting body indicator token ':' after the mapping token and inside the function definition"));

		ICodeBuilder& builder = f.Builder();
		
		if (f.IsVirtualMethod())
		{
			// The final input is an interface
			int thisOffset = ComputeThisOffset(builder.Owner(), fdef);
			builder.SetThisOffset(thisOffset);
		}
		
		builder.Begin();

			CCompileEnvironment ce(script, builder);
			CompileSetOutputRefToUniversalNullObjects(REF f);
			CompileExpressionSequence(ce, bodyIndex+1, fdef.NumberOfElements()-1, fdef);

		builder.End();

		builder.Assembler().Clear();
	}

	struct CClosureDef
	{
		const ISExpression* ClosureExpr;
		IFunctionBuilder* Closure;
		CClosureDef(cr_sex closureExpr, IFunctionBuilder& closure): ClosureExpr(&closureExpr), Closure(&closure) {}
	};

	typedef std::vector<CClosureDef> TClosures;

	struct CNullDef
	{
		IInterfaceBuilder* Interface;
		const ISExpression* Source;
		IStructureBuilder* NullObject;
		INamespaceBuilder* NS;
	};

	typedef std::vector<CNullDef> TNullObjectDefs;

	IFunctionBuilder& DeclareFunction(CScript& script, cr_sex source, FunctionPrototype& prototype);

	void CompileNullMethod(const IArchetype& nullMethod, IInterface& interf, IStructureBuilder& nullObject, cr_sex source, INamespace& ns)
	{
		TokenBuffer qualifiedMethodName;
		StringPrint(qualifiedMethodName, SEXTEXT("%s.%s"), nullObject.Name(), nullMethod.Name());

		IFunctionBuilder& f = DeclareFunction(nullObject.Module(), source, FunctionPrototype(qualifiedMethodName, true));
		
		for(int i = 0; i < nullMethod.NumberOfOutputs(); ++i)
		{
			const IStructure& argStruct = nullMethod.GetArgument(i);
			csexstr argType = GetFriendlyName(argStruct);
			csexstr argName = nullMethod.GetArgName(i);
			f.AddOutput(NameString::From(argName), argStruct, (void*)&source);
		}

		for(int i = 0; i < nullMethod.NumberOfInputs()-1; ++i)
		{
			int index = nullMethod.NumberOfOutputs() + i;
			const IStructure& argStruct = nullMethod.GetArgument(index);
			f.AddInput(NameString::From(nullMethod.GetArgName(index)), argStruct, (void*)&source);
		}

		TokenBuffer qualifiedInterfaceName;
		StringPrint(qualifiedInterfaceName, SEXTEXT("%s.%s"), ns.FullName()->Buffer, interf.Name());

		f.AddInput(NameString::From(THIS_POINTER_TOKEN), TypeString::From(qualifiedInterfaceName), (void*) &source);

		if (!f.TryResolveArguments())
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Error resolving arguments in null method: ") << qualifiedMethodName;
			Throw(source, streamer);
		}

		f.Builder().Begin();

		// Set all outputs to zero
		for(int i = 0; i < f.NumberOfOutputs(); i++)
		{
			const IArgument& arg = f.Arg(i);
			if (!IsNullType(*arg.ResolvedType()))
			{
				f.Builder().AssignLiteral(NameString::From(arg.Name()), SEXTEXT("0"));
			}
		}

		CompileSetOutputRefToUniversalNullObjects(f);

		f.Builder().End();
		f.Builder().Assembler().Clear();
	}

	void CompileNullObject(IInterface& interf, IStructureBuilder& nullObject, cr_sex source, INamespace& ns)
	{
		for(int i = 0; i < interf.MethodCount(); ++i)
		{
			const IArchetype& nullMethod = interf.GetMethod(i);
			CompileNullMethod(nullMethod, interf, nullObject, source, ns);
		}
	}

	class CScript;
	void RegisterContext(CScript& script, csexstr name);
	csexstr emptyString = SEXTEXT("");

	class CCompileContext
	{
	private:
		csexstr name;
		CScript& script;

	public:
		CCompileContext(CScript& _script, csexstr _name): script(_script), name(_name) { RegisterContext(script,name); }
		~CCompileContext() { RegisterContext(script,emptyString); }	
	};

	typedef std::vector<CStringConstant*> TStringConstants;

	typedef std::vector<IMacroBuilder*> TMacros;

	struct BuilderAndNameKey
	{
		ICodeBuilder* Builder;
		CStringKey Name;
	};

	struct hashBuilderAndNameKey
	{
		size_t operator()(const BuilderAndNameKey& s) const
		{
			return s.Name.Hash() ^ (size_t) s.Builder;
		}
	};

	bool operator == (const BuilderAndNameKey& a, const BuilderAndNameKey& b)
	{
		return a.Builder == b.Builder && a.Name == b.Name;
	}

	class CScript
	{
	private:
		ISParserTree& tree;
		IProgramObject& programObject;
		IModuleBuilder& module;
		bool isDirty;
		CScripts& scripts;
		TFunctionDefinitions localFunctions;
		TStructureDefinitions localStructures;
		TClosures closureJobs;
		TNullObjectDefs nullDefs;
		TStringConstants stringConstants;
		TMacros macros;

		typedef std::unordered_map<CStringKey, GlobalValue, hashCStringKey> TGlobalVariables;
		TGlobalVariables globalVariables;

		typedef std::vector<const ISExpression*> TExceptionBlocks;
		TExceptionBlocks exceptionBlocks;	
		csexstr contextName;

		typedef std::unordered_map<BuilderAndNameKey,ArrayDef,hashBuilderAndNameKey> TMapNameToArrayDef;
		typedef std::unordered_map<BuilderAndNameKey,ListDef,hashBuilderAndNameKey> TMapNameToListDef;
		typedef std::unordered_map<BuilderAndNameKey,NodeDef,hashBuilderAndNameKey> TMapNameToNodeDef;
		typedef std::unordered_map<BuilderAndNameKey,MapDef,hashBuilderAndNameKey> TMapNameToMapDef;
		typedef std::unordered_map<BuilderAndNameKey,MapNodeDef,hashBuilderAndNameKey> TMapNameToMapNodeDef;

		TMapNameToArrayDef mapNameToArrayDef;
		TMapNameToListDef mapNameToListDef;
		TMapNameToNodeDef mapNameToNodeDef;
		TMapNameToMapDef mapNameToMapDef;
		TMapNameToMapNodeDef mapNameToMapNodeDef;
	public:
		CScript(ISParserTree& _tree, IProgramObject& _programObject, CScripts& _scripts):
			tree(_tree),
			programObject(_programObject),
			isDirty(true),
			scripts(_scripts),
			module(_programObject.AddModule(tree.Source().Name())),
			contextName(emptyString)
		{
			tree.AddRef();
		}

		GlobalValue* GetGlobalValue(csexstr name)
		{
			CStringKey key(name);
			auto i = globalVariables.find(key);
			return i == globalVariables.end() ? nullptr : &i->second;
		}

		void EnumerateGlobals(IGlobalEnumerator& cb)
		{
			for (auto i : globalVariables)
			{
				cb(i.first.c_str(), i.second);
			}
		}

		const ArrayDef* GetArrayDef(ICodeBuilder& builder, csexstr arrayName)
		{
			BuilderAndNameKey key;
			key.Builder = &builder;
			key.Name = CStringKey(arrayName);

			TMapNameToArrayDef::const_iterator i = mapNameToArrayDef.find(key);
			return i == mapNameToArrayDef.end() ? NULL : &i->second;
		}

		const ListDef* GetListDef(ICodeBuilder& builder, csexstr listName)
		{
			BuilderAndNameKey key;
			key.Builder = &builder;
			key.Name = CStringKey(listName);

			TMapNameToListDef::const_iterator i = mapNameToListDef.find(key);
			return i == mapNameToListDef.end() ? NULL : &i->second;
		}

		const NodeDef* GetNodeDef(ICodeBuilder& builder, csexstr listName)
		{
			BuilderAndNameKey key;
			key.Builder = &builder;
			key.Name = CStringKey(listName);

			TMapNameToNodeDef::const_iterator i = mapNameToNodeDef.find(key);
			return i == mapNameToNodeDef.end() ? NULL : &i->second;
		}

		const MapDef* GetMapDef(ICodeBuilder& builder, csexstr listName)
		{
			BuilderAndNameKey key;
			key.Builder = &builder;
			key.Name = CStringKey(listName);

			TMapNameToMapDef::const_iterator i = mapNameToMapDef.find(key);
			return i == mapNameToMapDef.end() ? NULL : &i->second;
		}

		const MapNodeDef* GetMapNodeDef(ICodeBuilder& builder, csexstr name)
		{
			BuilderAndNameKey key;
			key.Builder = &builder;
			key.Name = CStringKey(name);

			TMapNameToMapNodeDef::const_iterator i = mapNameToMapNodeDef.find(key);
			return i == mapNameToMapNodeDef.end() ? NULL : &i->second;
		}

		void AddArrayDef(ICodeBuilder& builder, csexstr arrayName, const IStructure& elementType, cr_sex s)
		{
			BuilderAndNameKey key;
			key.Builder = &builder;
			key.Name = CStringKey(arrayName);

			ArrayDef def(s, elementType);
			mapNameToArrayDef.insert(std::make_pair(key,def));
		}

		void AddListDef(ICodeBuilder& builder, csexstr name, const IStructure& elementType, cr_sex s)
		{
			BuilderAndNameKey key;
			key.Builder = &builder;
			key.Name = CStringKey(name);

			ListDef def(s, elementType);
			mapNameToListDef.insert(std::make_pair(key,def));
		}

		void AddMapDef(ICodeBuilder& builder, csexstr name, const IStructure& keyType, const IStructure& valueType, cr_sex s)
		{
			BuilderAndNameKey key;
			key.Builder = &builder;
			key.Name = CStringKey(name);

			MapDef def(s, keyType, valueType);
			mapNameToMapDef.insert(std::make_pair(key,def));
		}

		void AddMapNodeDef(ICodeBuilder& builder, const MapDef& mapDef, csexstr mapName, csexstr nodeName, cr_sex s)
		{
			BuilderAndNameKey key;
			key.Builder = &builder;
			key.Name = CStringKey(nodeName);

			MapNodeDef def(s, mapDef, mapName);
			mapNameToMapNodeDef.insert(std::make_pair(key,def));
		}

		void AddNodeDef(ICodeBuilder& builder, csexstr nodeName, const IStructure& elementType, cr_sex s)
		{
			BuilderAndNameKey key;
			key.Builder = &builder;
			key.Name = CStringKey(nodeName);

			NodeDef def(s, elementType);
			mapNameToNodeDef.insert(std::make_pair(key,def));
		}

		const bool IsIStringInlined() const { return ::IsIStringInlined(scripts); }

		IProgramObject& Object() { return programObject; }
		IScriptSystem& System() { return GetSystem(scripts); }

		CStringConstant* CreateStringConstant(int length, csexstr pointer, const ISExpression* srcExpression)
		{
			const IStructure& scStruct = Object().Common().TypeStringLiteral();

			CStringConstant* sc = new CStringConstant;
			sc->header._allocSize = scStruct.SizeOfStruct();
			sc->header._vTables[0].Root = scStruct.GetVirtualTable(1);
			sc->header._typeInfo = (CClassDesc*) scStruct.GetVirtualTable(0);
			sc->length = length;
			sc->pointer = pointer;
			sc->srcExpression = (void*) srcExpression;

			stringConstants.push_back(sc);
			return sc;
		}

		void RegisterContext(csexstr _name) { contextName = _name; }
		csexstr GetContext() const { return contextName; }

		const ISExpression* GetTryCatchExpression() const
		{
			return exceptionBlocks.empty() ? NULL : exceptionBlocks.back();
		}

		void PushTryCatchBlock(cr_sex s)
		{
			exceptionBlocks.push_back(&s);
		}

		void PopTryCatchBlock()
		{
			exceptionBlocks.pop_back();
		}

		void AddCatchHandler(CScript& script, ID_BYTECODE id, size_t start, size_t end, size_t handlerOffset);

		~CScript()
		{
			tree.Release();

			for(auto i = stringConstants.begin(); i != stringConstants.end(); ++i)
			{
				CStringConstant* sc = *i;
				delete sc;
			}

			stringConstants.clear();
		}

		IModuleBuilder& ProgramModule() { return module; }
		TFunctionDefinitions& LocalFunctions() { return localFunctions; }
		TNullObjectDefs& NullObjects() { return nullDefs; }
		const ISParserTree& Tree() const { return tree; }
		ISParserTree& Tree() { return tree; }

		void Clear()
		{
			macros.clear();
			module.Clear();
			closureJobs.clear();
			stringConstants.clear();
			globalVariables.clear();
		}

		void CompileNullObjects()
		{
			for(auto n = nullDefs.begin(); n != nullDefs.end(); ++n)
			{
				CompileNullObject(*n->Interface, *n->NullObject, *n->Source, *n->NS);
				n->Interface->PostCompile();
			}
		}

		void CompileNextClosures()
		{
			// Compiling a local function can enqueue a closure for further compilation, so handle that next
			while(!closureJobs.empty())
			{
				CClosureDef cdef = closureJobs.back();
				closureJobs.pop_back();
				CompileClosureBody(*cdef.ClosureExpr, *cdef.Closure, *this);
			}
		}

		void CompileLocalFunctions()
		{
			for(auto i = localFunctions.begin(); i != localFunctions.end(); ++i)
			{
				CompileFunctionFromExpression(*i->second.Fn, *i->second.FnDef, *this);
			}
		}

		void CompileJITStubs()
		{
			for(auto i = localFunctions.begin(); i != localFunctions.end(); ++i)
			{
				csexstr fname = i->second.Fn->Name();
				CompileJITStub(*i->second.Fn, *i->second.FnDef, *this, GetSystem(*this));
			}
		}

		void AppendCompiledNamespaces(TNamespaceDefinitions& nsDefs);
		void AddEnumeratedVirtualMethod(IN cr_sex virtualMethodExpr, IN csexstr methodName, REF IInterfaceBuilder& inter, IN size_t vmIndex);
		void AddVirtualMethod(IN cr_sex virtualMethodExpr, REF IInterfaceBuilder& inter, IN size_t vmIndex);
		void AddVirtualMethod(IN const IArchetype& archetype, REF IInterfaceBuilder& inter, IN size_t vmIndex, cr_sex def);		
		void ComputeGlobal(cr_sex globalDef, int& globalBaseIndex);
		void ComputeGlobals(int& globalBaseIndex);
		void ComputeInterfaces();
		void ComputeInterfacePrototypes();
		void ComputePrefixes();
		void ComputeStructureNames();
		void ComputeFunctionNames();
		void ComputeStructureFields();
		void ComputeFunctionArgs();
		void CompileFactoryJITStubs();
		void CompileMacroJITStubs();
		void ValidateTopLevel();
		void ComputeArchetypes();
		void ComputeArchetypeNames();
		void AddArchetype(cr_sex e);
		void AddArchetypeName(cr_sex e);
		void PostClosure(cr_sex s, IFunctionBuilder& closure);
		void AddInterfacePrototype(cr_sex s, bool isInterfaceDefinedFromClassMethods);
		void ValidateConcreteClasses();
		void ValidateConstructors();
		void DeclareFactory(cr_sex factoryDef);
		void DeclareMacros();
		IFunctionBuilder& GetNullFunction(const IArchetype& archetype);
	};

	CStringConstant* CreateStringConstant(CScript& script, int length, csexstr pointer, const ISExpression* srcExpression)
	{
		return script.CreateStringConstant(length, pointer, srcExpression);
	}

	void RegisterContext(CScript& script, csexstr name) { script.RegisterContext(name); }
	csexstr GetContext(const CScript& script) { return script.GetContext(); }

	IModuleBuilder& GetModule(CScript& script) { return script.ProgramModule(); }

	void CompileNextClosures(CScript& script) { script.CompileNextClosures(); }

	const ISExpression* GetTryCatchExpression(CScript& script) { return script.GetTryCatchExpression(); }

	IStructure* GetLocalStructure(CScript& script, csexstr className)
	{
		return script.ProgramModule().FindStructure(className);
	}

	class CScripts
	{
		friend CScript;
	private:
		typedef std::tr1::unordered_map<ISParserTree*,CScript*> TMapTreeToScript;
		TMapTreeToScript scriptMap;
		typedef std::vector<std::pair<ISParserTree*,CScript*>> TScriptVector;
		TScriptVector scripts;

		IProgramObject& programObject;
		TNamespaceDefinitions namespaceDefinitions;
		TStructureDefinitions structDefinitions;
		CDefaultExceptionLogic exceptionLogic;
		IScriptSystem& system;
		int globalBaseIndex;
		std::unordered_map<const IArchetype*, IFunctionBuilder*> nullArchetypeFunctions;

		bool canInlineString;		
	public:
		CScripts(IProgramObject& _programObject, IScriptSystem& _system) : programObject(_programObject), system(_system), exceptionLogic(_system), canInlineString(false), globalBaseIndex(0)
		{			
		}

		~CScripts()
		{
			for(auto i = scripts.begin(); i != scripts.end(); ++i)
			{
				CScript* script = i->second;
				delete script;
			}
		}

		void SetGlobalVariablesToDefaults(IVirtualMachine& vm)
		{
			if (globalBaseIndex == 0) return;

			size_t nGlobalDataBytes = globalBaseIndex;

			for (auto i = scripts.begin(); i != scripts.end(); ++i)
			{
				CScript* script = i->second;

				struct : IGlobalEnumerator
				{
					uint8* global;
					virtual void operator()(csexstr name, const GlobalValue& variable)
					{
						BITCOUNT variableLen = GetBitCount(variable.type);
						size_t offset = variable.offset;

						if (variableLen == BITCOUNT_32)
						{
							memcpy(global + offset, &variable.initialValue.int32Value, 4);
						}
						else
						{
							memcpy(global + offset, &variable.initialValue.int64Value, 8);
						}
					}
				} cb;

				cb.global = (uint8*)alloca(nGlobalDataBytes);
				script->EnumerateGlobals(cb);
				vm.SetGlobalData(cb.global, nGlobalDataBytes);
			}
		}

		CScript* CreateModule(ISParserTree& tree)
		{
			CScript* module = new CScript(tree, programObject, *this);
			scriptMap.insert(std::make_pair(&tree, module));
			scripts.push_back(std::make_pair(&tree, module));
			return module;
		}

		virtual Sex::ISParserTree* GetSourceCode(const IModule& module)
		{
			for(auto i = scripts.begin(); i != scripts.end(); ++i)
			{
				if (&i->second->ProgramModule() == &module)
				{
					return i->first;
				}
			}

			return NULL;
		}

		const bool InlineStrings() const { return canInlineString; }

		CDefaultExceptionLogic& ExceptionLogic() { return exceptionLogic; }

		CScript* FindDefiningModule(INamespace* ns)
		{
			for(auto i = namespaceDefinitions.begin(); i != namespaceDefinitions.end(); ++i)
			{
				if (i->NS == ns)
				{
					return i->Module;
				}
			}

			return NULL;
		}

		IScriptSystem& System() { return system; }

		void ResolveNamespaces()
		{
			namespaceDefinitions.clear();
			for(auto i = scripts.begin(); i != scripts.end(); ++i)
			{
				CScript* module = i->second;
				module->AppendCompiledNamespaces(namespaceDefinitions);
			}

			// Sort the namespace defs by name length ascending. Note Length('A.B.C.D') > Length('A.B.C') > Length('A.B') > Length('A')
			// Sorting gives 'A', 'A.B', 'A.B.C', 'A.B.C.D', which is the same order we need to construct 'A.B.C.D' from its ancestors
			std::sort(namespaceDefinitions.begin(), namespaceDefinitions.end()); 

			for(auto j = namespaceDefinitions.begin(); j != namespaceDefinitions.end(); ++j)
			{
				csexstr nsSymbol = j->E->String()->Buffer;
				INamespace* ns = programObject.GetRootNamespace().FindSubspace(nsSymbol);
				if (ns != NULL)
				{
					CScript* origin = FindDefiningModule(ns);
					csexstr source = (origin == NULL) ? SEXTEXT("an intrinsic module") : origin->Tree().Source().Name();	

					sexstringstream streamer;
					streamer << "The namespace " << nsSymbol << SEXTEXT(" is defined in ") << source << SEXTEXT(". Only one definition is permitted");				
					Throw(*(j->E), streamer);
				}
				else
				{
					try
					{
						j->NS = &programObject.GetRootNamespace().AddNamespace(nsSymbol, ADDNAMESPACEFLAGS_NORMAL);
					}
					catch (IException& e)
					{
						Throw(*(j->E), e.Message());
					}
					catch (std::exception& e)
					{
						Throw(*(j->E), e.what());
					}				
				}
			}
		}

		template<class T> void ForEachScript(T& t)
		{
			for(auto i = scripts.begin(); i != scripts.end(); ++i)
			{
				CScript* script = i->second;
				csexstr name = script->ProgramModule().Name();	

				cr_sex root = script->Tree().Root();
				
				try
				{
					t.Process(*script, name);
				}
				catch(STCException& ex)
				{
					sexstringstream streamer;
					streamer << ex.Source() << SEXTEXT(": ") << ex.Message();
					Throw(root, streamer);
				}			
				catch (std::exception& )
				{
					Throw(root, SEXTEXT("std::exception thrown" /* TODO: e.what() */));
				}	
			}
		}

		void CompileNamespaces()
		{
			struct FnctorCompileNamespaces
			{
				void Process(CScript& script, csexstr name)
				{
					script.Clear();
					script.ValidateTopLevel();
				}
			} fnctorCompileNamespaces;

			ForEachScript(fnctorCompileNamespaces);
			ResolveNamespaces();
		}

		void AddSpecialStructures()
		{
			IModuleBuilder& module = programObject.GetModule(0);
			auto ns =  Compiler::MatchNamespace(module, SEXTEXT("Sys.Native"));

			{
				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure(SEXTEXT("_Array"), prototype, NULL);

				s.AddMember(NameString::From(SEXTEXT("_start")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_length")), TypeString::From(SEXTEXT("Int32")));
				s.AddMember(NameString::From(SEXTEXT("_elementCapacity")), TypeString::From(SEXTEXT("Int32")));
				s.AddMember(NameString::From(SEXTEXT("_elementType")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_elementSize")), TypeString::From(SEXTEXT("Int32")));
				s.AddMember(NameString::From(SEXTEXT("_lock")), TypeString::From(SEXTEXT("Int32")));

				ns->Alias(SEXTEXT("_Array"), s);
			}
			{
				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure(SEXTEXT("_List"), prototype, NULL);

				s.AddMember(NameString::From(SEXTEXT("_length")), TypeString::From(SEXTEXT("Int32")));
				s.AddMember(NameString::From(SEXTEXT("_lock")), TypeString::From(SEXTEXT("Int32")));
				s.AddMember(NameString::From(SEXTEXT("_elementType")), TypeString::From(SEXTEXT("Pointer")));												
				s.AddMember(NameString::From(SEXTEXT("_head")), TypeString::From(SEXTEXT("Pointer")));			
				s.AddMember(NameString::From(SEXTEXT("_tail")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_elementSize")), TypeString::From(SEXTEXT("Int32")));
			
				ns->Alias(SEXTEXT("_List"), s);
			}
			{
				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure(SEXTEXT("_Node"), prototype, NULL);

				s.AddMember(NameString::From(SEXTEXT("_list")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_elementType")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_previous")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_next")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_refCount")), TypeString::From(SEXTEXT("Int32")));
							
				ns->Alias(SEXTEXT("_Node"), s);
			}
			{
				csexstr lockName = SEXTEXT("_Lock");

				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure(lockName, prototype, NULL);

				s.AddMember(NameString::From(SEXTEXT("_lockSource")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_lockMemberOffset")), TypeString::From(SEXTEXT("Int32")));

				ns->Alias(lockName, s);
			}
			{
				csexstr mapName = SEXTEXT("_Map");

				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure(mapName, prototype, NULL);

				s.AddMember(NameString::From(SEXTEXT("_length")), TypeString::From(SEXTEXT("Int32")));
				s.AddMember(NameString::From(SEXTEXT("_keyType")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_valueType")), TypeString::From(SEXTEXT("Pointer")));				
				s.AddMember(NameString::From(SEXTEXT("_nullNode")), TypeString::From(SEXTEXT("Pointer")));		
				s.AddMember(NameString::From(SEXTEXT("_head")), TypeString::From(SEXTEXT("Pointer")));	
				s.AddMember(NameString::From(SEXTEXT("_tail")), TypeString::From(SEXTEXT("Pointer")));	
				s.AddMember(NameString::From(SEXTEXT("_stdVec1")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_stdVec2")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_stdVec3")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_stdVec4")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_stdVec5")), TypeString::From(SEXTEXT("Pointer")));
				s.AddMember(NameString::From(SEXTEXT("_keyResolver")), TypeString::From(SEXTEXT("Pointer")));

				ns->Alias(mapName, s);
			}
			{
				csexstr nodeName = SEXTEXT("_MapNode");

				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure(nodeName, prototype, NULL);

				s.AddMember(NameString::From(SEXTEXT("_container")), TypeString::From(SEXTEXT("Pointer")));		
				s.AddMember(NameString::From(SEXTEXT("_previous")), TypeString::From(SEXTEXT("Pointer")));		
				s.AddMember(NameString::From(SEXTEXT("_next")), TypeString::From(SEXTEXT("Pointer")));		
				s.AddMember(NameString::From(SEXTEXT("_exists")), TypeString::From(SEXTEXT("Bool")));	
				s.AddMember(NameString::From(SEXTEXT("_refCount")), TypeString::From(SEXTEXT("Int32")));	
				s.AddMember(NameString::From(SEXTEXT("_hashcode")), TypeString::From(SEXTEXT("Int32")));	

				ns->Alias(nodeName, s);
			}
		}

		void CompileDeclarations()
		{
			exceptionLogic.Clear();

			struct FnctorComputeArchetypeNames
			{
				void Process(CScript& script, csexstr name)
				{
					script.ComputeArchetypeNames();
					script.ComputeInterfacePrototypes();
				}
			} fnctorComputeArchetypeNames;
			ForEachScript(fnctorComputeArchetypeNames);


			struct FnctorComputeStructNames
			{
				void Process(CScript& script, csexstr name)
				{
					script.ComputePrefixes();
					script.ComputeStructureNames();	
				}
			} fnctorComputeStructNames;
			ForEachScript(fnctorComputeStructNames);

			AddSpecialStructures();

			struct FnctorComputeFunctionNames
			{
				void Process(CScript& script, csexstr name)
				{
					script.ComputeFunctionNames();	
				}
			} fnctorComputeFunctionNames;
			ForEachScript(fnctorComputeFunctionNames);

			struct FnctorAppendAliases
			{
				void Process(CScript& script, csexstr name)
				{
					AppendAliases(script.ProgramModule(), script.Tree());
				}
			} fnctorAppendAliases;
			ForEachScript(fnctorAppendAliases);

			struct FnctorStructureFields
			{
				void Process(CScript& script, csexstr name)
				{
					script.ComputeStructureFields();
				}
			} fnctorStructureFields;
			ForEachScript(fnctorStructureFields);

			struct FnctorFunctionArgs
			{
				void Process(CScript& script, csexstr name)
				{
					script.ComputeFunctionArgs();
				}
			} fnctorFunctionArgs;
			ForEachScript(fnctorFunctionArgs);

			struct FnctorComputeArchetypes
			{
				void Process(CScript& script, csexstr name)
				{
					script.ComputeArchetypes();
					script.ComputeInterfaces();	
				}
			} fnctorComputeArchetypes;
			ForEachScript(fnctorComputeArchetypes);

			if (!programObject.ResolveDefinitions())
			{
				SourcePos start, end;
				start.X = end.X = 0;
				start.Y = end.Y = 0;

				ParseException ex(start, end, SEXTEXT("Sexy Script System"), SEXTEXT("Failed to resolve definitions"), SEXTEXT(""), NULL);
				throw ex;
			}

			const IStructure* mapNode = programObject.GetModule(0).FindStructure(SEXTEXT("_Map"));
			if (mapNode->SizeOfStruct() < sizeof(MapImage))
			{
				SourcePos start, end;
				start.X = end.X = 0;
				start.Y = end.Y = 0;
				ParseException ex(start, end, SEXTEXT("Sexy Script System"), SEXTEXT("_Map was too small to represent a MapImage. Add a few fake pointers in the _Map definition."), SEXTEXT(""), NULL);
				throw ex;
			}

			programObject.InitCommon();

			struct FnctorValidateConcreteClasses
			{
				void Process(CScript& script, csexstr name)
				{
					script.ValidateConcreteClasses();
				}
			} fnctorValidateConcreteClasses;
			ForEachScript(fnctorValidateConcreteClasses);

			struct FnctorValidateConstructors
			{
				void Process(CScript& script, csexstr name)
				{
					script.ValidateConstructors();
				}
			} fnctorValidateConstructors;
			ForEachScript(fnctorValidateConstructors);

			struct FnctorDeclareMacros
			{
				void Process(CScript& script, csexstr name)
				{
					script.DeclareMacros();
				}
			} fnctorDeclareMacros;
			ForEachScript(fnctorDeclareMacros);

			struct FnctorComputeGlobals
			{
				int globalBaseIndex;
				void Process(CScript& script, csexstr name)
				{
					script.ComputeGlobals(REF globalBaseIndex);
				}
			} fnctorComputeGlobals;
			fnctorComputeGlobals.globalBaseIndex = 0;
			ForEachScript(fnctorComputeGlobals);

			globalBaseIndex = fnctorComputeGlobals.globalBaseIndex;
		}

		bool TryInlineIString()
		{		
			const IInterface& istring = programObject.Common().SysTypeIString();

			for(auto k = scripts.begin(); k != scripts.end(); ++k)
			{
				const IModule& mod = k->second->ProgramModule();
				for(int j = 0; j < mod.StructCount(); ++j)
				{
					const IStructure& s = mod.GetStructure(j);

					if (DoesClassImplementInterface(s, istring))
					{
						if (s.InterfaceCount() != 1)
						{
							sexstringstream streamer;
							streamer << SEXTEXT("Cannot inline IString, as ") << mod.Name() << SEXTEXT("/") << s.Name() << SEXTEXT(" implements multiple interfaces.") << std::ends;
							programObject.Log().Write(streamer.str().c_str());
							return false;
						}

						if (s.MemberCount() < 5)
						{
							sexstringstream streamer;
							streamer << SEXTEXT("Cannot inline IString, as ") << mod.Name() << SEXTEXT("/") << s.Name() << SEXTEXT(" does not implement both the buffer and length members.") << std::ends;
							programObject.Log().Write(streamer.str().c_str());
							return false;
						}

						const IMember& bufferMember = s.GetMember(4);
						if (bufferMember.UnderlyingType()->VarType() != VARTYPE_Pointer || !AreEqual(SEXTEXT("buffer"), bufferMember.Name()))
						{
							sexstringstream streamer;
							streamer << SEXTEXT("Cannot inline IString, as ") << mod.Name() << SEXTEXT("/") << s.Name() << SEXTEXT(" 5th member is not (Pointer Buffer).") << std::ends;
							programObject.Log().Write(streamer.str().c_str());
							return false;
						}

						const IMember& lenMember = s.GetMember(3);
						if (lenMember.UnderlyingType()->VarType() != VARTYPE_Int32 || !AreEqual(SEXTEXT("length"), lenMember.Name()))
						{
							sexstringstream streamer;
							streamer << SEXTEXT("Cannot inline IString, as ") << mod.Name() << SEXTEXT("/") << s.Name() << SEXTEXT(" 4th member is not (Int32 length).") << std::ends;
							programObject.Log().Write(streamer.str().c_str());
							return false;
						}
					}
				}
			}

			return true;
		}

		void CompileBytecode()
		{
			struct FnctorCompileNullObjects
			{
				void Process(CScript& script, csexstr name)
				{
					script.CompileNullObjects();
				}
			} fnctorCompileNullObjects;
			ForEachScript(fnctorCompileNullObjects);

			canInlineString = TryInlineIString();			
					
			struct FnctorJIT
			{
				void Process(CScript& script, csexstr name)
				{
					script.CompileJITStubs();
					script.CompileFactoryJITStubs();		
					script.CompileMacroJITStubs();
				}
			} fnctorJIT;
			ForEachScript(fnctorJIT);

			/*
			for(auto i = scripts.begin(); i != scripts.end(); ++i)
			{
				CScript* script = i->second;
				csexstr name = script->ProgramModule().Name();	
				script->CompileLocalFunctions();
			}
			*/
		}

		void ReleaseAll()
		{
			for(auto i = scripts.begin(); i != scripts.end(); ++i)
			{				
				delete i->second;
			}

			scriptMap.clear();
			scripts.clear();
		}

		void ReleaseModule(ISParserTree& tree)
		{
			auto i = scriptMap.find(&tree);
			if (i != scriptMap.end())
			{
				// if we find a namespace definition has been lost, invalidate the entire namespace tree
				PredHasModule hasModule(i->second);
				auto j = std::find_if(namespaceDefinitions.begin(), namespaceDefinitions.end(), hasModule);
				if (j != namespaceDefinitions.end())
				{
					programObject.GetRootNamespace().Clear();
					namespaceDefinitions.clear();
				}		
				
				delete i->second;
				scriptMap.erase(i);

				auto k = scripts.begin();
				while (k != scripts.end())
				{
					if (k->first == &tree)
					{
						k = scripts.erase(k);
					}
					else
					{
						++k;
					}
				}
			}
		}
	};

	IFunctionBuilder& CScript::GetNullFunction(const IArchetype& archetype)
	{
		auto i = scripts.nullArchetypeFunctions.find(&archetype);
		if (i != scripts.nullArchetypeFunctions.end())
		{
			return *i->second;
		}

		TokenBuffer nullFunctionName;
		StringPrint(nullFunctionName, SEXTEXT("_null%s"), archetype.Name());

		cr_sex source = *(const Sex::ISExpression*) archetype.Definition();

		IFunctionBuilder& f = DeclareFunction(module, source, FunctionPrototype(nullFunctionName, true));

		scripts.nullArchetypeFunctions.insert(std::make_pair(&archetype, &f));

		for (int i = 0; i < archetype.NumberOfOutputs(); ++i)
		{
			const IStructure& argStruct = archetype.GetArgument(i);
			csexstr argType = GetFriendlyName(argStruct);
			csexstr argName = archetype.GetArgName(i);
			f.AddOutput(NameString::From(argName), argStruct, (void*) archetype.Definition());
		}

		for (int i = 0; i < archetype.NumberOfInputs() - 1; ++i)
		{
			int index = archetype.NumberOfOutputs() + i;
			const IStructure& argStruct = archetype.GetArgument(index);
			f.AddInput(NameString::From(archetype.GetArgName(index)), argStruct, (void*) archetype.Definition());
		}

		if (!f.TryResolveArguments())
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Error resolving arguments in: ") << nullFunctionName;
			Throw(source, streamer);
		}

		f.Builder().Begin();

		// Set all outputs to zero
		for (int i = 0; i < f.NumberOfOutputs(); i++)
		{
			const IArgument& arg = f.Arg(i);
			f.Builder().AssignLiteral(NameString::From(arg.Name()), SEXTEXT("0"));
		}

		f.Builder().End();
		f.Builder().Assembler().Clear();
		return f;
	}

	IFunctionBuilder& GetNullFunction(CScript& script, const IArchetype& archetype)
	{
		return script.GetNullFunction(archetype);
	}

	void AddCatchHandler(CScript& script, ID_BYTECODE id, size_t start, size_t end, size_t handlerOffset)
	{
		script.AddCatchHandler(script, id, start, end, handlerOffset);
	}

	void CScript::AddCatchHandler(CScript& script, ID_BYTECODE id, size_t start, size_t end, size_t handlerOffset)
	{
		scripts.ExceptionLogic().AddCatchHandler(id, start, end, handlerOffset);
	}

	void CScript::PostClosure(cr_sex s, IFunctionBuilder& closure)
	{
		closureJobs.push_back(CClosureDef(s,closure));
		CompileNextClosures();
	}

	void PostClosure(cr_sex s, IFunctionBuilder& closure, CScript& script)
	{
		script.PostClosure(s, closure);
	}

	void CScript::AppendCompiledNamespaces(TNamespaceDefinitions& nsDefs)
	{
		cr_sex root = tree.Root();			
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex e = root.GetElement(i);
			cr_sex elementName = GetAtomicArg(e, 0);
			if (AreEqual(elementName.String(), SEXTEXT("namespace")))
			{
				AssertNotTooFewElements(e, 2);

				CBindNSExpressionToModule def;
				def.E = &e.GetElement(1);
				def.Module = this;
				def.NS = NULL;

				AssertValidNamespaceDef(*def.E);

				nsDefs.push_back(def);
			}
		}
	}

	IStructure* LookupArg(cr_sex s, IModuleBuilder& module, OUT csexstr& argName)
	{
		AssertCompound(s);
		AssertNotTooFewElements(s, 2);
		AssertNotTooManyElements(s, 2);

		cr_sex type = GetAtomicArg(s, 0);
		cr_sex name = GetAtomicArg(s, 1);

		AssertTypeIdentifier(type);
		AssertLocalIdentifier(name);

		IStructure* st = MatchStructure(type, module);
		if (st == NULL)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Cannot find match to ") << type.String()->Buffer;
			Throw(s, streamer);
		}

		argName = name.String()->Buffer;

		return st;
	}

	IArchetype* LookupArchetype(cr_sex s, IModuleBuilder& module)
	{
		AssertCompound(s);
		AssertNotTooFewElements(s, 2);
		AssertNotTooManyElements(s, 2);

		cr_sex type = GetAtomicArg(s, 0);
		cr_sex name = GetAtomicArg(s, 1);

		AssertTypeIdentifier(type);
		AssertLocalIdentifier(name);

		return MatchArchetype(type, module);
	}
	
	enum { MAX_ARGS_PER_CLOSURE = 40};

	void AddArchetypeToNamespace(INamespaceBuilder& ns, csexstr archetypeName, cr_sex s, CScript& script)
	{
		// (archetype fully-qualified-name (input1)...(inputN) -> (output1) ... (outputN))
		int mapIndex = GetIndexOf(2, s, SEXTEXT("->"));
		if (mapIndex < 0)
		{
			Throw(s, SEXTEXT("Could not find mapping token '->' in the archetype definition"));
		}

		int totalArgs = s.NumberOfElements() - 3;
			
		if (totalArgs > MAX_ARGS_PER_CLOSURE)
		{
			Throw(s, SEXTEXT("Too many arguments supplied to the archetype"));
		}

		const IStructure** st = (const IStructure**) alloca(sizeof(IStructure*) * totalArgs);
		const IArchetype** ar = (const IArchetype**) alloca(sizeof(IArchetype*) * totalArgs);
		const IStructure** genericArg1s = (const IStructure**) alloca(sizeof(IStructure*) * totalArgs);
		csexstr* names = (csexstr*) alloca(sizeof(csexstr) * totalArgs);

		for(int i = mapIndex+1; i < s.NumberOfElements(); ++i)
		{
			cr_sex child = s.GetElement(i);
			int inputIndex = i-mapIndex-1;
			st[inputIndex] = LookupArg(child, script.ProgramModule(), names[inputIndex]);
			ar[inputIndex] = LookupArchetype(child, script.ProgramModule());
			genericArg1s[inputIndex] = NULL;
		}

		int nOutputs = s.NumberOfElements() - mapIndex - 1;

		for(int j = 0; j < mapIndex-2; ++j)
		{
			cr_sex child = s.GetElement(j+2);
			int outputIndex = j + nOutputs;
			st[outputIndex] = LookupArg(child, script.ProgramModule(), names[outputIndex]);
			ar[outputIndex] = LookupArchetype(child, script.ProgramModule());
			genericArg1s[outputIndex] = NULL;
		}

		if (!ns.FindArchetype(archetypeName))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Could not find archetype: '") << archetypeName << SEXTEXT("'");
			Throw(s, streamer);
		}

		ns.AddArchetype(archetypeName, names, st, ar, genericArg1s, nOutputs, s.NumberOfElements() - nOutputs - 3, &s);
	}

	const IArchetype& AddArchetypeNameToNamespace(INamespaceBuilder& ns, csexstr archetypeName, cr_sex s)
	{
		if (ns.FindArchetype(archetypeName))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Archetype declaration conflict. Multiple declarations using the same name: '") << archetypeName << SEXTEXT("'");
			Throw(s, streamer);
		}

		return ns.AddArchetype(archetypeName, NULL, NULL, NULL, NULL, 0, 0, &s);
	}

	void CScript::AddArchetype(cr_sex s)
	{
		// (archetype fully-qualified-name (input_args) -> (output_args) )

		AssertNotTooFewElements(s, 3);
		AssertNotTooManyElements(s, 3 + MAX_ARGS_PER_CLOSURE);

		cr_sex fullyQualifiedNameExpr = GetAtomicArg(s, 1);
		
		int mapIndex = GetIndexOf(2, s, SEXTEXT("->"));

		if (mapIndex < 0)
		{
			Throw(s, SEXTEXT("Expected mapping token '->' in archetype expression"));
		}
		
		AssertQualifiedIdentifier(fullyQualifiedNameExpr);

		NamespaceSplitter splitter(fullyQualifiedNameExpr.String()->Buffer);
		csexstr body, tail;
		INamespaceBuilder& ns = ValidateSplitTail(splitter, OUT body, OUT tail, fullyQualifiedNameExpr, programObject);
		AddArchetypeToNamespace(ns, tail, s, *this);
	}

	void CScript::AddArchetypeName(cr_sex s)
	{
		// (archetype fully-qualified-name (input_args1)...(input_argsN) -> (output_args)...(output_argsN) )

		AssertNotTooFewElements(s, 3);
		AssertNotTooManyElements(s, MAX_ARGS_PER_CLOSURE + 3);

		cr_sex fullyQualifiedNameExpr = GetAtomicArg(s, 1);
		
		int mapIndex = GetIndexOf(2, s, SEXTEXT("->"));
		if (mapIndex < 0)
		{
			Throw(s, SEXTEXT("Could not find mapping token '->' in the archetype"));
		}

		AssertQualifiedIdentifier(fullyQualifiedNameExpr);

		NamespaceSplitter splitter(fullyQualifiedNameExpr.String()->Buffer);

		csexstr body, tail;
		INamespaceBuilder& ns = ValidateSplitTail(REF splitter, OUT body, OUT tail, IN fullyQualifiedNameExpr, IN programObject);	

		const IArchetype* existing = ns.FindArchetype(tail);
		if (existing != NULL) Throw(fullyQualifiedNameExpr, SEXTEXT("Archetype already defined"));

		AssertValidArchetypeName(fullyQualifiedNameExpr, tail);

		const IArchetype& a = AddArchetypeNameToNamespace(ns, tail, s);
		IStructureBuilder& st = DeclareStructure(module, tail, StructurePrototype(MEMBERALIGN_1, INSTANCEALIGN_1, true, &a, false), fullyQualifiedNameExpr);
		st.AddMember(NameString::From(SEXTEXT("bytecodeId")), TypeString::From(SEXTEXT("Pointer")));
		st.AddMember(NameString::From(SEXTEXT("parentSF")), TypeString::From(SEXTEXT("Pointer")));
		ns.Alias(tail, st);
	}

	void SubstituteChars(csexstr src, SEXCHAR* dest, int len, SEXCHAR from, SEXCHAR to)
	{
		for(int i = 0; i < len; ++i)
		{
			SEXCHAR c = src[i];
			dest[i] = c == from ? to : c;
			if (c == 0) break;
		}

		dest[len-1] = 0;
	}

	struct InterfaceProtoDesc
	{
		int methodCount;
		IInterfaceBuilder* base;
	};

	InterfaceProtoDesc ComputeMethodCount(cr_sex s, CScript& script, csexstr interfaceShortName)
	{
		InterfaceProtoDesc desc = { 0 };

		for (int i = 2; i < s.NumberOfElements(); ++i)
		{
			cr_sex child = s.GetElement(i);
			cr_sex childDirective = GetAtomicArg(child, 0);
			sexstring childType = childDirective.String();

			if (AreEqual(childType, SEXTEXT("extends")))
			{
				cr_sex baseExpr = GetAtomicArg(child, 1);
				sexstring baseName = baseExpr.String();

				if (desc.base != NULL) ThrowTokenAlreadyDefined(child, SEXTEXT("extends"), interfaceShortName, SEXTEXT("attribute"));

				desc.base = GetInterfaceFQN(baseExpr, script);
				if (desc.base == NULL)
				{
					Throw(baseExpr, SEXTEXT("The base interface has not been defined prior to this point."));
				}
			}
			else if (AreEqual(childType, SEXTEXT("attribute")))
			{
			}
			else
			{
				desc.methodCount++;
			}
		}

		if (desc.base != NULL) desc.methodCount += desc.base->MethodCount();
		return desc;
	}

	bool IsMethod(cr_sex s)
	{
		return s.NumberOfElements() > 1 && IsAtomic(s[0]) && AreEqual(s[0].String(), SEXTEXT("method"));
	}

	struct IMethodEnumerator
	{
		virtual void operator()(cr_sex smethodDef, csexstr className, csexstr methodName) = 0;
	};

	void EnumerateMethodsInClass(cr_sex root, IMethodEnumerator& callback)
	{
		for (int i = 0; i < root.NumberOfElements(); ++i)
		{
			cr_sex smethodDef = root[i];
			if (IsMethod(smethodDef))
			{
				sexstring dottedMethodName = GetAtomicArg(smethodDef, 1).String();
				NamespaceSplitter splitter(dottedMethodName->Buffer);

				csexstr methodClassName, methodName;
				if (splitter.SplitTail(OUT methodClassName, OUT methodName) )
				{
					if (!AreEqual(methodName, SEXTEXT("Construct")))
					{
						callback(smethodDef, methodClassName, methodName);
					}
				}
				else
				{
					Throw(smethodDef[1], SEXTEXT("Could not split full method name into class and short method name"));
				}
			}
		}
	}

	InterfaceProtoDesc CountMethodsDefinedForClass(cr_sex sInterfaceFromClassDef, IModuleBuilder& module)
	{
		IInterfaceBuilder* base = nullptr;

		struct : IMethodEnumerator
		{
			int count;
			sexstring className;
			virtual void operator()(cr_sex smethodDef, csexstr className, csexstr methodName)
			{
				if (AreEqual(this->className, className))
				{
					count++;
				}
			}
		} cb;
		cb.className = GetAtomicArg(*sInterfaceFromClassDef.Parent(), 1).String();
		cb.count = 0;

		EnumerateMethodsInClass(sInterfaceFromClassDef.Tree().Root(), cb);

		if (sInterfaceFromClassDef.NumberOfElements() == 4)
		{
			cr_sex sbaseName = sInterfaceFromClassDef[3];
			if (!IsAtomic(sbaseName))
			{
				Throw(sbaseName, SEXTEXT("Expecting base interface name"));
			}

			base = MatchInterface(sbaseName, module);
			if (base == nullptr)
			{
				Throw(sbaseName, SEXTEXT("Could not resolve base interface name"));
			}
		}

		return { cb.count, base };	
	}

	void CScript::AddInterfacePrototype(cr_sex s, bool isInterfaceDefinedFromClassMethods)
	{
		cr_sex nameExpr = GetAtomicArg(s, 1);
		csexstr name = nameExpr.String()->Buffer;

		NamespaceSplitter splitter(name);

		csexstr body, tail;
		INamespaceBuilder& ns = ValidateSplitTail(REF splitter, OUT body, OUT tail, IN s, IN programObject);	

		AssertValidInterfaceName(nameExpr, tail);

		IInterfaceBuilder* current_interf = ns.FindInterface(tail);
		if (current_interf != NULL)	Throw(nameExpr, SEXTEXT("Duplicate interface definition"));

		TokenBuffer flatName;
		SubstituteChars(name, flatName.Text, 256, SEXCHAR('.'), SEXCHAR('_'));
		
		TokenBuffer nullName;
		StringPrint(nullName, SEXTEXT("_Null_%s"), (csexstr) flatName);

		StructurePrototype prototype(MEMBERALIGN_1, INSTANCEALIGN_1, true, NULL, true);

		IStructureBuilder& nullObject = DeclareStructure(module, nullName, prototype, nameExpr);

		InterfaceProtoDesc desc = isInterfaceDefinedFromClassMethods ?  CountMethodsDefinedForClass(s, module) : ComputeMethodCount(s, *this, tail);
		IInterfaceBuilder* interf = ns.DeclareInterface(tail, desc.methodCount, nullObject, desc.base);

		// We also need to declare a null object that implements the interface			
		nullObject.AddMember(NameString::From(SEXTEXT("_typeInfo")), TypeString::From(SEXTEXT("Pointer")));
		nullObject.AddMember(NameString::From(SEXTEXT("_allocSize")), TypeString::From(SEXTEXT("Int32")));
		nullObject.AddMember(NameString::From(SEXTEXT("_vTable1")), TypeString::From(SEXTEXT("Pointer")));

		for (const IInterface* z = interf; z != NULL; z = z->Base())
		{
			if (AreEqual(z->NullObjectType().Name(), SEXTEXT("_Null_Sys_Type_IString")))
			{
				nullObject.AddMember(NameString::From(SEXTEXT("length")), TypeString::From(SEXTEXT("Int32")));
				nullObject.AddMember(NameString::From(SEXTEXT("buffer")), TypeString::From(SEXTEXT("Pointer")));
				break;
			}
		}
			
		nullObject.AddInterface(name);

		ns.Alias(tail, nullObject);

		CNullDef def;
		def.Interface = interf;
		def.NullObject = &nullObject;
		def.Source = &s;
		def.NS = &ns;

		nullDefs.push_back(def);
	}

	void CScript::ComputeArchetypes()
	{
		cr_sex root = tree.Root();			
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex e = root.GetElement(i);
			cr_sex elementName = GetAtomicArg(e, 0);
			if (AreEqual(elementName.String(), SEXTEXT("archetype")))
			{
				AddArchetype(e);
			}
		}
	}

	void CScript::ComputeArchetypeNames()
	{
		cr_sex root = tree.Root();			
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex e = root.GetElement(i);
			cr_sex elementName = GetAtomicArg(e, 0);
			if (AreEqual(elementName.String(), SEXTEXT("archetype")))
			{
				AddArchetypeName(e);
			}
		}
	}

	void CScript::AddVirtualMethod(IN const IArchetype& archetype, REF IInterfaceBuilder& inter, IN size_t vmIndex, cr_sex def)
	{
		int nArgs = archetype.NumberOfInputs() + archetype.NumberOfOutputs();

		csexstr* names = (csexstr*) alloca(nArgs * sizeof(csexstr));
		const IStructure** resolvedTypes = (const IStructure**) alloca(nArgs * sizeof(const IStructure*));
		const IArchetype** archetypes = (const IArchetype**) alloca(nArgs * sizeof(IArchetype*));
		const IStructure** genericArg1s = (const IStructure**) alloca(nArgs * sizeof(IArchetype*));
		bool* isOut = (bool*) alloca(nArgs * sizeof(bool));

		for(int i = 0; i < archetype.NumberOfOutputs(); ++i)
		{
			const IStructure& type = archetype.GetArgument(i);
			names[i] = archetype.GetArgName(i);
			resolvedTypes[i] = &type;
			archetypes[i] = type.Archetype();
			genericArg1s[i] = NULL;
			isOut[i] = true;
		}

		int count = archetype.NumberOfOutputs();

		for(int i = 0; i < archetype.NumberOfInputs()-1; ++i)
		{
			const IStructure& type = archetype.GetArgument(count);
			names[count] = archetype.GetArgName(count);
			resolvedTypes[count] = &type;
			archetypes[count] = type.Archetype();
			genericArg1s[count] = archetype.GetGenericArg1(count);
			isOut[count] = false;
			count++;;
		}

		names[count] = SEXTEXT("_vTable");
		resolvedTypes[count] = module.Object().IntrinsicModule().FindStructure(SEXTEXT("Pointer"));
		archetypes[count] = NULL;
		isOut[count] = false;
		genericArg1s[count] = NULL;

		inter.SetMethod(vmIndex, archetype.Name(), nArgs, names, resolvedTypes, archetypes, genericArg1s, isOut, &def);
	}

	void CScript::AddEnumeratedVirtualMethod(IN cr_sex methodExpr, IN csexstr methodName, REF IInterfaceBuilder& inter, IN size_t vmIndex)
	{
		AssertCompound(methodExpr);
		AssertNotTooFewElements(methodExpr, 4); // (method Dog.Bark -> :)

		int bodyIndex = GetIndexOf(2, methodExpr, SEXTEXT(":"));
		if (bodyIndex < 0)
		{
			Throw(methodExpr, SEXTEXT("Expecting body indicator token ':' inside method definition"));
		}

		enum { MAX_METHOD_ARGS = 40 };
		
		if (bodyIndex > MAX_METHOD_ARGS + 3)
		{
			Throw(methodExpr, SEXTEXT("Too many arguments in method"));
		}
		
		int mapIndex = GetIndexOf(2, methodExpr, SEXTEXT("->"));
		if (mapIndex < 0)
		{
			Throw(methodExpr, SEXTEXT("Could not find the mapping token '->' in the method definition"));
		}

		int nArgs = bodyIndex - 2;
		csexstr* names = (csexstr*)alloca(nArgs * sizeof(csexstr));
		const IStructure** resolvedTypes = (const IStructure**)alloca(nArgs * sizeof(const IStructure*));
		const IArchetype** archetypes = (const IArchetype**)alloca(nArgs * sizeof(IArchetype*));
		const IStructure** genericArg1s = (const IStructure**)alloca(nArgs * sizeof(const IStructure*));
		bool* isOut = (bool*)alloca(nArgs * sizeof(bool));

		int count = 0;

		for (int i = mapIndex + 1; i < bodyIndex; ++i)
		{
			cr_sex typeNameExptr = methodExpr.GetElement(i);
			AssertCompound(typeNameExptr);
			AssertNotTooFewElements(typeNameExptr, 2);
			AssertNotTooManyElements(typeNameExptr, 2);

			cr_sex type = GetAtomicArg(typeNameExptr, 0);
			cr_sex name = GetAtomicArg(typeNameExptr, 1);

			names[count] = name.String()->Buffer;
			resolvedTypes[count] = MatchStructure(type, module);
			archetypes[count] = MatchArchetype(type, module);
			genericArg1s[count] = NULL;
			isOut[count] = true;
			if (resolvedTypes[count] == NULL) Throw(typeNameExptr, SEXTEXT("Cannot resolve type. Check the spelling and/or use a fully qualified name."));
			count++;
		}

		for (int i = 2; i < mapIndex; ++i)
		{
			cr_sex typeNameExptr = methodExpr.GetElement(i);
			AssertCompound(typeNameExptr);
			AssertNotTooFewElements(typeNameExptr, 2);

			cr_sex type = GetAtomicArg(typeNameExptr, 0);

			if (AreEqual(type.String(), SEXTEXT("array")))
			{
				AssertNotTooManyElements(typeNameExptr, 3);
				cr_sex name = GetAtomicArg(typeNameExptr, 2);
				cr_sex elementType = GetAtomicArg(typeNameExptr, 1);
				names[count] = name.String()->Buffer;
				resolvedTypes[count] = MatchStructure(type, module);
				if (resolvedTypes[count] == NULL) Throw(typeNameExptr, SEXTEXT("Cannot resolve type. Check the spelling and/or use a fully qualified name."));
				archetypes[count] = NULL;
				genericArg1s[count] = MatchStructure(elementType, module);
			}
			else
			{
				AssertNotTooManyElements(typeNameExptr, 2);
				cr_sex name = GetAtomicArg(typeNameExptr, 1);
				names[count] = name.String()->Buffer;
				resolvedTypes[count] = MatchStructure(type, module);
				if (resolvedTypes[count] == NULL) Throw(typeNameExptr, SEXTEXT("Cannot resolve type. Check the spelling and/or use a fully qualified name."));
				archetypes[count] = MatchArchetype(type, module);
				genericArg1s[count] = NULL;
			}

			isOut[count] = false;
			count++;;
		}

		names[count] = SEXTEXT("_vTable");
		resolvedTypes[count] = module.Object().IntrinsicModule().FindStructure(SEXTEXT("Pointer"));
		archetypes[count] = NULL;
		genericArg1s[count] = NULL;
		isOut[count] = false;

		count++;

		inter.SetMethod(vmIndex, methodName, nArgs, names, resolvedTypes, archetypes, genericArg1s, isOut, &methodExpr);
	}

	void CScript::AddVirtualMethod(IN cr_sex virtualMethodExpr, REF IInterfaceBuilder& inter, IN size_t vmIndex)
	{
		AssertCompound(virtualMethodExpr);
		AssertNotTooFewElements(virtualMethodExpr, 2);

		enum {MAX_METHOD_ARGS = 40};
		AssertNotTooManyElements(virtualMethodExpr, MAX_METHOD_ARGS+2);

		cr_sex methodNameExpr = GetAtomicArg(virtualMethodExpr,0);
		csexstr methodName = methodNameExpr.String()->Buffer;

		int mapIndex = GetIndexOf(1, virtualMethodExpr, SEXTEXT("->"));
		if (mapIndex < 0)
		{
			Throw(virtualMethodExpr, SEXTEXT("Could not find the mapping token '->' in the virtual method definition"));
		}

		int nArgs = virtualMethodExpr.NumberOfElements() - 2 + 1;
		csexstr* names = (csexstr*) alloca(nArgs * sizeof(csexstr));
		const IStructure** resolvedTypes = (const IStructure**) alloca(nArgs * sizeof(const IStructure*));
		const IArchetype** archetypes = (const IArchetype**) alloca(nArgs * sizeof(IArchetype*));
		const IStructure** genericArg1s = (const IStructure**) alloca(nArgs * sizeof(const IStructure*));
		bool* isOut = (bool*) alloca(nArgs * sizeof(bool));

		int count = 0;

		for(int i = mapIndex+1; i < virtualMethodExpr.NumberOfElements(); ++i)
		{
			cr_sex typeNameExptr = virtualMethodExpr.GetElement(i);
			AssertCompound(typeNameExptr);
			AssertNotTooFewElements(typeNameExptr, 2);
			AssertNotTooManyElements(typeNameExptr, 2);

			cr_sex type = GetAtomicArg(typeNameExptr,0);
			cr_sex name = GetAtomicArg(typeNameExptr,1);

			names[count] = name.String()->Buffer;
			resolvedTypes[count] = MatchStructure(type, module);
 			archetypes[count] = MatchArchetype(type, module);
			genericArg1s[count] = NULL;
			isOut[count] = true;
			if (resolvedTypes[count] == NULL) Throw(typeNameExptr, SEXTEXT("Cannot resolve type. Check the spelling and/or use a fully qualified name."));		
			count++;
		}

		for(int i = 1; i < mapIndex; ++i)
		{
			cr_sex typeNameExptr = virtualMethodExpr.GetElement(i);
			AssertCompound(typeNameExptr);
			AssertNotTooFewElements(typeNameExptr, 2);

			cr_sex type = GetAtomicArg(typeNameExptr,0);

			if (AreEqual(type.String(), SEXTEXT("array")))
			{				
				AssertNotTooManyElements(typeNameExptr, 3);
				cr_sex name = GetAtomicArg(typeNameExptr,2);
				cr_sex elementType =  GetAtomicArg(typeNameExptr,1);
				names[count] = name.String()->Buffer;
				resolvedTypes[count] = MatchStructure(type, module);
				if (resolvedTypes[count] == NULL) Throw(typeNameExptr, SEXTEXT("Cannot resolve type. Check the spelling and/or use a fully qualified name."));	
				archetypes[count] = NULL;
				genericArg1s[count] = MatchStructure(elementType, module);
			}
			else
			{
				AssertNotTooManyElements(typeNameExptr, 2);
				cr_sex name = GetAtomicArg(typeNameExptr,1);
				names[count] = name.String()->Buffer;
				resolvedTypes[count] = MatchStructure(type, module);
				if (resolvedTypes[count] == NULL) Throw(typeNameExptr, SEXTEXT("Cannot resolve type. Check the spelling and/or use a fully qualified name."));	
				archetypes[count] = MatchArchetype(type, module);
				genericArg1s[count] = NULL;
			}					
			
			isOut[count] = false;
			count++;;
		}

		names[count] = SEXTEXT("_vTable");
		resolvedTypes[count] = module.Object().IntrinsicModule().FindStructure(SEXTEXT("Pointer"));
		archetypes[count] = NULL;
		genericArg1s[count] = NULL;
		isOut[count] = false;

		count++;

		inter.SetMethod(vmIndex, methodName, nArgs, names, resolvedTypes, archetypes, genericArg1s, isOut, &virtualMethodExpr);
	}

	void ValidateDefineAttribute(IAttributes& a, cr_sex attributeExpr)
	{
		cr_sex nameExpr = GetAtomicArg(attributeExpr, 1);
		csexstr name = nameExpr.String()->Buffer;
		if (!a.AddAttribute(name, &attributeExpr))
		{
			Throw(nameExpr, SEXTEXT("Duplicate attribute name in attribute collections are not allowed"));
		}
	}

	void AssertExpectedParse(cr_sex source, VariantValue& value, VARTYPE type, csexstr buffer)
	{
		auto result = Parse::TryParse(value, type, buffer);
		switch (result)
		{
		case Parse::PARSERESULT_GOOD:
			return;
		case Parse::PARSERESULT_BAD_DECIMAL_DIGIT:
			Throw(source, SEXTEXT("Bad decimal digit"));
			break;
		case Parse::PARSERESULT_HEXADECIMAL_BAD_CHARACTER:
			Throw(source, SEXTEXT("Bad hexadecimal digit"));
			break;
		case Parse::PARSERESULT_HEXADECIMAL_INCORRECT_NUMBER_OF_DIGITS:
			Throw(source, SEXTEXT("Incorrect number of digits"));
			break;
		case Parse::PARSERESULT_HEX_FOR_FLOAT:
			Throw(source, SEXTEXT("Hexadecimal may not be used to initialize a floating point value"));
			break;
		case Parse::PARSERESULT_OVERFLOW:
			Throw(source, SEXTEXT("Overflow"));
			break;
		case Parse::PARSERESULT_UNDERFLOW:
			Throw(source, SEXTEXT("Underflow"));
			break;
		case Parse::PARSERESULT_UNHANDLED_TYPE:
		default:
			Throw(source, SEXTEXT("Unhandled type/literal"));
			break;
		}
	}

	void CScript::ComputeGlobal(cr_sex globalDef, int& globalBaseIndex)
	{
		if (globalDef.NumberOfElements() != 5)
		{
			Throw(globalDef, SEXTEXT("(global <vartype> <varname> = <initvalue>)"));
		}

		if (!IsAtomic(globalDef[3]) || !AreEqual(globalDef[3].String(), SEXTEXT("=")))
		{
			Throw(globalDef[3], SEXTEXT("Expecting variable assignment character '='"));
		}

		cr_sex stype = GetAtomicArg(globalDef, 1);
		cr_sex sname = globalDef[2];
		cr_sex svalue = GetAtomicArg(globalDef, 4);

		AssertLocalIdentifier(sname);
		sexstring name = sname.String();
		sexstring type = stype.String();
		sexstring value = svalue.String();

		CStringKey key(name->Buffer);
		auto i = globalVariables.find(key);
		if (i != globalVariables.end())
		{
			Throw(sname, SEXTEXT("Duplicate global variable name."));
		}

		auto structure = MatchStructure(stype, module);
		if (structure == nullptr)
		{
			Throw(stype, SEXTEXT("Cannot resolve global variable type"));
		}

		if (!IsPrimitiveType(structure->VarType()))
		{
			Throw(stype, SEXTEXT("Only primitive types can serve as global variables"));
		}

		GlobalValue g;
		g.type = structure->VarType();
		g.offset = globalBaseIndex;
		globalBaseIndex += structure->SizeOfStruct();
		AssertExpectedParse(stype, g.initialValue, g.type, value->Buffer);
		globalVariables.insert(std::make_pair(key, g));
	}

	void CScript::ComputeGlobals(int& globalBaseIndex)
	{
		cr_sex root = tree.Root();
		for (int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = root[i];
			cr_sex elementName = GetAtomicArg(topLevelItem, 0);

			// (global <vartype> <varname> = <initvalue>)

			if (AreEqual(elementName.String(), SEXTEXT("global")))
			{
				ComputeGlobal(topLevelItem, REF globalBaseIndex);
			}
		}
	}

	void CScript::ComputeInterfaces()
	{
		cr_sex root = tree.Root();			
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex e = root.GetElement(i);
			cr_sex elementName = GetAtomicArg(e, 0);

			if (AreEqual(elementName.String(), SEXTEXT("interface")))
			{
				cr_sex name = GetAtomicArg(e, 1);

				NamespaceSplitter splitter(name.String()->Buffer);

				csexstr body, publicName;
				INamespaceBuilder& ns = ValidateSplitTail(REF splitter, OUT body, OUT publicName, IN e, IN programObject);		
				
				int methodIndex = 0;

				IInterfaceBuilder* interf = ns.FindInterface(publicName);

				if (interf->Base() != NULL)
				{
					for(int j = 0; j < interf->Base()->MethodCount(); j++)
					{
						const IArchetype& baseMethod = interf->Base()->GetMethod(j);
						AddVirtualMethod(baseMethod, *interf, methodIndex++, *(const ISExpression*) baseMethod.Definition());
					}
				}

				for(int j = 2; j < e.NumberOfElements(); ++j)
				{
					cr_sex virtualMethodExpr = e.GetElement(j);
					cr_sex methodNameExpr = GetAtomicArg(virtualMethodExpr, 0);
					sexstring methodName = methodNameExpr.String();

					if (AreEqual(methodName, SEXTEXT("attribute")))
					{
						ValidateDefineAttribute(interf->Attributes(), virtualMethodExpr);
					}
					else if (AreEqual(methodName, SEXTEXT("extends")))
					{						
					}
					else
					{
						AddVirtualMethod(virtualMethodExpr, *interf, methodIndex++);
					}
				}
			}
			else if (AreEqual(elementName.String(), SEXTEXT("class")))
			{
				for (int i = 1; i < e.NumberOfElements(); ++i)
				{
					cr_sex classDirective = e[i];
					if (classDirective.NumberOfElements() > 1 && IsAtomic(classDirective[0]) && AreEqual(classDirective[0].String(), SEXTEXT("defines")))
					{	
						auto classStruct = module.FindStructure(e[1].String()->Buffer);
						if (classStruct->InterfaceCount() != 1)
						{
							Throw(e, SEXTEXT("Classes that define an interface may not implement more than one interface"));
						}

						cr_sex name = GetAtomicArg(classDirective, 1);

						NamespaceSplitter splitter(name.String()->Buffer);

						csexstr body, publicName;
						INamespaceBuilder& ns = ValidateSplitTail(REF splitter, OUT body, OUT publicName, IN e, IN programObject);

						int methodIndex = 0;

						struct : IMethodEnumerator
						{
							sexstring className;
							CScript* This;
							IInterfaceBuilder* interf;
							int methodIndex;

							virtual void operator()(cr_sex smethodDef, csexstr className, csexstr methodName)
							{
								if (AreEqual(this->className, className))
								{
									This->AddEnumeratedVirtualMethod(smethodDef, methodName, *interf, methodIndex++);
								}
							}
						} cb;

						cb.className = GetAtomicArg(e, 1).String();
						cb.This = this;
						cb.interf = ns.FindInterface(publicName);
						cb.methodIndex = 0;

						EnumerateMethodsInClass(classDirective.Tree().Root(), cb);
					}
				}
			}
		}
	}

	void ValidateClassImplementsMethod(const IArchetype& archetype, const IStructure& classType, cr_sex src, csexstr source)
	{
		TokenBuffer qualifiedName;
		StringPrint(qualifiedName, SEXTEXT("%s.%s"), classType.Name(), archetype.Name());

		const IFunction* f = classType.Module().FindFunction(qualifiedName);
		if (f == NULL)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Expecting method named '") << (csexstr) qualifiedName << SEXTEXT("' inside source module");
			Throw(classType.Definition() != NULL ? *(const ISExpression*) classType.Definition() : src, streamer);
		}

		TokenBuffer dottedName;
		StringPrint(dottedName, SEXTEXT("%s."), source);

		ValidateArchetypeMatchesArchetype(src, *f, archetype, dottedName);
	}

	void ValidateClassImplementsInterface(const IInterface& interf, const IStructure& classType, cr_sex src)
	{
		for(int i = 0; i < interf.MethodCount(); ++i)
		{
			const IArchetype& method = interf.GetMethod(i);
			ValidateClassImplementsMethod(method, classType, src, interf.Name());
		}
	}

	void CScript::ValidateConcreteClasses()
	{
		csexstr name = ProgramModule().Name();

		for(int i = 0; i < module.StructCount();  ++i)
		{
			const IStructure& s = module.GetStructure(i);
			if (s.Prototype().IsClass && !IsNullType(s))
			{
				for(int j = 0; j < s.InterfaceCount(); j++)
				{			
					ValidateClassImplementsInterface(s.GetInterface(j), s, *(const ISExpression*) s.Definition());
				}
			}
		}
	}

	void ValidateConstructorExistsForType(const IStructure& specimen, const IStructure& type)
	{
		const IFunction* constructor = type.Constructor();
		if (constructor == NULL)
		{
			const ISExpression* exp = (const ISExpression*) specimen.Definition();
			if (exp != NULL)
			{
				sexstringstream streamer;
				streamer << SEXTEXT("Type requires an explicit constructor definition inside the module ") << type.Module().Name() << SEXTEXT(". Expecting ") << type.Name() << SEXTEXT(".Construct");
				Throw(*exp, streamer);
			}
		}
	}

	void ValidateMembersForConstructors(IProgramObject& obj, const IStructure& specimen, const IStructure& child)
	{
		for(int i = 0; i < child.MemberCount(); ++i)
		{
			const IMember& m = child.GetMember(i);

			if (!IsPrimitiveType(m.UnderlyingType()->VarType()))
			{
				if (m.UnderlyingGenericArg1Type())
				{
					ValidateConstructorExistsForType(specimen, child);
				}

				ValidateMembersForConstructors(obj, specimen, *m.UnderlyingType());
			}			
		}
	}

	void CScript::ValidateConstructors()
	{
		for(int i = 0; i < module.StructCount(); ++i)
		{
			const IStructure& s = module.GetStructure(i);

			ValidateMembersForConstructors(Object(), s, s);
		}
	}

	void CScript::ComputeInterfacePrototypes()
	{
		cr_sex root = tree.Root();			
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex e = root.GetElement(i);
			cr_sex elementName = GetAtomicArg(e, 0);
			if (AreEqual(elementName.String(), SEXTEXT("interface")))
			{
				AddInterfacePrototype(e, false);
			}
		}
	}

	csexstr topLevelItems[] =
	{
		SEXTEXT("namespace"),
		SEXTEXT("struct"),
		SEXTEXT("function"),
		SEXTEXT("alias"),
		SEXTEXT("archetype"),
		SEXTEXT("using"),
		SEXTEXT("interface"),
		SEXTEXT("class"),
		SEXTEXT("method"),
		SEXTEXT("factory"),
		SEXTEXT("macro"),
		SEXTEXT("global"),
		NULL
	};

	bool IsOneOf(csexstr item, csexstr items[])
	{
		for(csexstr* i = items; *i != NULL; i++)
		{
			if (AreEqual(item, *i)) return true;
		}

		return false;
	}

	void CScript::ValidateTopLevel()
	{
		cr_sex root = tree.Root();			
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex e = root.GetElement(i);
			cr_sex elementName = GetAtomicArg(e, 0);

			sexstring directive = elementName.String();
			if(directive->Buffer[0] != '\'' && !IsOneOf(directive->Buffer, topLevelItems))
			{
				Throw(elementName, SEXTEXT("Unknown top level item, expecting keyword or data item"));
			}
		}
	}

	void CScript::ComputePrefixes()
	{
		module.ClearPrefixes();

		cr_sex root = tree.Root();			
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = root.GetElement(i);
			cr_sex elementName = GetAtomicArg(topLevelItem, 0);

			if (AreEqual(elementName.String(), SEXTEXT("using")))
			{
				AssertNotTooFewElements(topLevelItem, 2);
				AssertNotTooManyElements(topLevelItem, 2);

				cr_sex prefixExpr = topLevelItem.GetElement(1);
				AssertQualifiedIdentifier(prefixExpr);

				sexstring prefix = prefixExpr.String();

				INamespace* ns = module.Object().GetRootNamespace().FindSubspace(prefix->Buffer);
				if (ns == NULL)
				{
					Throw(prefixExpr, SEXTEXT("Could not find namespace specified in the using directive"));
				}

				module.UsePrefix(prefix->Buffer);
			}
		}
	}

	void CompileFactoryBody(IFactoryBuilder& factory, cr_sex factoryDef, int bodyStart,  CScript& script)
	{
		CCompileContext cc(script, SEXTEXT("factory"));

		IFunctionBuilder& f = factory.Constructor();
		csexstr fname = f.Name();
		ICodeBuilder& builder = f.Builder();
		builder.Begin();

		CCompileEnvironment ce(script, builder);

		CompileExpressionSequence(ce, bodyStart, factoryDef.NumberOfElements()-1, factoryDef);

		builder.End();
		builder.Assembler().Clear();
	}

	IFunctionBuilder& GetConcreteMethod(cr_sex src, sexstring className, csexstr methodName, IModuleBuilder& module)
	{
		TokenBuffer fullName;
		StringPrint(fullName, SEXTEXT("%s.%s"), (csexstr) className->Buffer, methodName);

		IFunctionBuilder* cons = module.FindFunction(fullName);

		if (cons == NULL)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Cannot find ") << fullName << SEXTEXT(" in the module");
			Throw(src, streamer);
		}

		return *cons;
	}

	const IStructure& GetStructure(cr_sex typeExpr, const sexstring typeName, CScript& script)
	{
		const IStructure* s = Compiler::MatchStructure(script.Object().Log(), typeExpr.String()->Buffer, script.ProgramModule());
		if (s == NULL)
		{
			Throw(typeExpr, SEXTEXT("Could not resolve type"));
		}
		return *s;
	}

	bool HasElementEqualTo(cr_sex s, int index, csexstr value)
	{
		if (index >= s.NumberOfElements() || index < 0)
			return false;

		cr_sex subExpr = s.GetElement(index);
		if (!IsAtomic(subExpr))
			return false;

		return AreEqual(subExpr.String(), value);
	}

	IFunctionBuilder* TryCompileInline(IStructureBuilder** ppInlineClass, IFactory& factory, cr_sex factoryDef, int bodyIndex, CScript& script)
	{
		*ppInlineClass = NULL;

		if (factoryDef.NumberOfElements() != bodyIndex+1)
		{
			return NULL;
		}

		cr_sex body = factoryDef.GetElement(bodyIndex);

		// a factory is inlined if the only thing in the expression is a constructor with arguments that exactly match those of the factory
		// TODO - use general function inlining, rather than this special case
		if (!IsCompound(body))
			return NULL;


		if (!HasElementEqualTo(body, 0, SEXTEXT("construct")))
		{
			return NULL;
		}

		// Now we know/hope the body expression is of form (construct class-name arg1 arg2 ... argN)
		cr_sex classNameExpr = GetAtomicArg(body, 1);
		sexstring className = classNameExpr.String();

		IStructureBuilder* s = script.ProgramModule().FindStructure(className->Buffer);
		if (s == NULL)
		{
			Throw(classNameExpr, SEXTEXT("Cannot find the specified class in the module"));
		}

		IFunctionBuilder& cons = GetConcreteMethod(classNameExpr, className, SEXTEXT("Construct"), script.ProgramModule());
		if (cons.NumberOfOutputs() > 0)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Unexpected output ") << (csexstr) className->Buffer << SEXTEXT(".Construct in the module");
			Throw(classNameExpr, streamer);
		}

		if (cons.NumberOfInputs() != body.NumberOfElements() - 1)
		{
			return NULL;
		}

		if (cons.NumberOfInputs() != factoryDef.NumberOfElements() - 4)
		{
			return NULL;
		}

		for(int i = 0; i < cons.NumberOfInputs() - 1; ++i) // N.B the final input of the constructor is the implicit 'this' ref
		{
			const IArgument& input = cons.Arg(i);
			cr_sex exprArg = body.GetElement(i + 2);
			if (!IsAtomic(exprArg)) return NULL;

			cr_sex inputDef = factoryDef.GetElement(i + 3);
			cr_sex inputTypeExpr = GetAtomicArg(inputDef, 0);
			sexstring inputTypeName = inputTypeExpr.String();
			sexstring inputName = GetAtomicArg(inputDef, 1).String();

			const IStructure& inputType = GetStructure(inputTypeExpr, inputTypeName, script);
			if (inputType != *input.ResolvedType())
			{
				return NULL;
			}

			if (!AreEqual(exprArg.String(), inputName))
			{
				return NULL;
			}
		}

		*ppInlineClass = s;
		return &cons;
	}

	void CScript::CompileFactoryJITStubs()
	{
		cr_sex root = tree.Root();
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = root.GetElement(i);
			AssertNotTooFewElements(topLevelItem, 1);
			cr_sex elementName = GetAtomicArg(topLevelItem, 0);

			if (AreEqual(elementName.String(), SEXTEXT("factory")))
			{
				cr_sex factoryNameExpr = GetAtomicArg(topLevelItem, 1);
				NamespaceSplitter splitter(factoryNameExpr.String()->Buffer);

				csexstr nsRoot, publicName;
				splitter.SplitTail(OUT nsRoot, OUT publicName);

				INamespaceBuilder* ns = programObject.GetRootNamespace().FindSubspace(nsRoot);
				IFactoryBuilder* f = ns->FindFactory(publicName);

				int bodyIndex = GetIndexOf(1, topLevelItem, SEXTEXT(":"));
				if (bodyIndex < 0) Throw(topLevelItem, SEXTEXT("Could not find body indicator ':' in factory definition"));
				if (bodyIndex >= topLevelItem.NumberOfElements()) Throw(topLevelItem, SEXTEXT("Body indicator ':' was at the end of the expression. Expecting body to follow it"));

				CompileJITStub(f, topLevelItem, *this, GetSystem(*this));

				IStructureBuilder* inlineClass = NULL;
				IFunctionBuilder* inlineCons = TryCompileInline(OUT &inlineClass, *f, topLevelItem, bodyIndex+1, *this);

				f->SetInline(inlineCons, inlineClass);
			}
		}
	}

	void CScript::CompileMacroJITStubs()
	{
		for(auto i = macros.begin(); i != macros.end(); ++i)
		{
			auto macro = *i;
			CompileJITStub(macro, *this, GetSystem(*this));
		}
	}

	int CountClassElements(cr_sex classDef, csexstr elementName)
	{
		int count = 0;
		for(int j = 2; j < classDef.NumberOfElements(); j++)
		{
			cr_sex field = classDef.GetElement(j);
			if (field.NumberOfElements() > 1)
			{
				cr_sex classElement = field.GetElement(0);
				if (IsAtomic(classElement))
				{
					if (AreEqual(classElement.String(), elementName))
					{
						count++;
					}
				}
			}
		}

		return count;
	}

	void CScript::DeclareMacros()
	{
		cr_sex root = tree.Root();
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = root.GetElement(i);
			cr_sex elementNameExpr = GetAtomicArg(topLevelItem, 0);
			sexstring elementName = elementNameExpr.String();
			
			if (AreEqual(elementName, SEXTEXT("macro")))
			{
				IMacroBuilder* macro = DeclareMacro(topLevelItem, module);
				macros.push_back(macro);		
			}
		}
	}

	void CScript::DeclareFactory(cr_sex factoryDef)
	{
		// (factory <factory> <interface> (arg1) (arg2): <body>)
		AssertNotTooFewElements(factoryDef, 5);
		cr_sex factoryNameExpr = GetAtomicArg(factoryDef, 1);
		cr_sex factoryInterfaceExpr = GetAtomicArg(factoryDef, 2);
		sexstring factoryName = factoryNameExpr.String();
		sexstring interfaceName = factoryInterfaceExpr.String();

		AssertQualifiedIdentifier(factoryNameExpr);
		AssertQualifiedIdentifier(factoryInterfaceExpr);

		NamespaceSplitter splitter(factoryName->Buffer);
		
		csexstr ns, shortName;
		if (!splitter.SplitTail(OUT ns, OUT shortName))Throw(factoryNameExpr, SEXTEXT("Expecting fully qualified name"));

		int bodyIndex = GetIndexOf(3, factoryDef, SEXTEXT(":"));
		if (bodyIndex < 0)	Throw(factoryDef, SEXTEXT("Expecting a body start token ':' in the expression"));
		if (bodyIndex == factoryDef.NumberOfElements()-1)	Throw(factoryDef, SEXTEXT("Expecting a expressions to follow the body start token ':' in the expression"));

		INamespaceBuilder* factoryNS = module.Object().GetRootNamespace().FindSubspace(ns);
		if (factoryNS == NULL) Throw(factoryNameExpr, SEXTEXT("Cannot find the namespace containing the factory"));

		int inputCount = bodyIndex - 3;
		for(int i = 0; i < inputCount; ++i)
		{
			cr_sex inputExpression = factoryDef.GetElement(i + 3);
			AssertCompound(inputExpression);
			AssertNotTooFewElements(inputExpression, 2);
			AssertNotTooManyElements(inputExpression, 2);
			AssertAtomic(inputExpression.GetElement(0));
			AssertAtomic(inputExpression.GetElement(1));
			AssertLocalIdentifier(inputExpression.GetElement(1));
		}
		
		IInterfaceBuilder* interf = MatchInterface(factoryInterfaceExpr, module);
		if (interf == NULL)	Throw(factoryInterfaceExpr, SEXTEXT("Unknown interface"));

		IFactory* factory = factoryNS->FindFactory(shortName);
		if (factory != NULL) Throw(factoryNameExpr, SEXTEXT("A factory with the same name exists in the same namespace"));

		IFunctionBuilder& factoryFunction = module.DeclareFunction(FunctionPrototype(factoryName->Buffer, false), &factoryDef);

		for(int i = 0; i < inputCount; ++i)
		{
			cr_sex inputExpression = factoryDef.GetElement(i + 3);
			cr_sex typeExpr = GetAtomicArg(inputExpression, 0);
			cr_sex nameExpr = GetAtomicArg(inputExpression, 1);
			factoryFunction.AddInput(NameString::From(nameExpr.String()), TypeString::From(typeExpr.String()), NULL);
		}

		factoryFunction.AddInput(NameString::From(SEXTEXT("this")), TypeString::From(factoryInterfaceExpr.String()), NULL);

		factoryNS->RegisterFactory(shortName, factoryFunction, *interf, factoryInterfaceExpr.String());
	}

	void CScript::ComputeStructureNames()
	{
		localStructures.clear();

		cr_sex root = tree.Root();
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = root.GetElement(i);
			AssertNotTooFewElements(topLevelItem, 1);
			cr_sex elementNameExpr = GetAtomicArg(topLevelItem, 0);
			sexstring elementName = elementNameExpr.String();

			bool isStruct = false;
			bool isClass = false;

			if (AreEqual(elementName, SEXTEXT("struct")))
			{
				isStruct = true;
			}
			else if (AreEqual(elementName, SEXTEXT("class")))
			{
				isClass = true;
			}

			if (isStruct || isClass)
			{
				// (struct <name> (<type0> <varname0>) ... (<typeN>, <varnameN>) )
				AssertNotTooFewElements(topLevelItem, 2);

				cr_sex structNameExpr = GetAtomicArg(topLevelItem, 1);
				csexstr structName = structNameExpr.String()->Buffer;

				AssertValidStructureName(structNameExpr);

				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, isClass);

				IStructureBuilder& s = DeclareStructure(module, structName, prototype, structNameExpr);

				if (isClass)
				{
					s.AddMember(NameString::From(SEXTEXT("_typeInfo")), TypeString::From(SEXTEXT("Pointer")));
					s.AddMember(NameString::From(SEXTEXT("_allocSize")), TypeString::From(SEXTEXT("Int32")));

					int interfaceCount = 1; // The vtable0 interface, giving the destructor Id
					interfaceCount += CountClassElements(topLevelItem, SEXTEXT("implements"));
					interfaceCount += CountClassElements(topLevelItem, SEXTEXT("defines"));

					for (int i = 1; i < topLevelItem.NumberOfElements(); ++i)
					{
						cr_sex sdefineInterface = topLevelItem[i];
						if (IsCompound(sdefineInterface) && IsAtomic(sdefineInterface[0]) && AreEqual(sdefineInterface[0].String(), SEXTEXT("defines")))
						{
							AddInterfacePrototype(sdefineInterface, true);
						}
					}

					for(int i = 1; i < interfaceCount; i++)
					{
						SEXCHAR vtableName[32];
						StringPrint(vtableName, 32, SEXTEXT("_vTable%d"), i);
						s.AddMember(NameString::From(vtableName), TypeString::From(SEXTEXT("Pointer")));
					}					
				}

				CBindStructDefToExpression structDef;
				structDef.Struct = &s;
				structDef.StructDef = &topLevelItem;

				localStructures.push_back(structDef);
			}			
		}
	}

	void CScript::ComputeFunctionNames()
	{
		localFunctions.clear();
		cr_sex root = tree.Root();
		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = root.GetElement(i);
			AssertNotTooFewElements(topLevelItem, 1);
			cr_sex elementName = GetAtomicArg(topLevelItem, 0);

			bool isFunction = false;
			bool isMethod = false;

			if (AreEqual(elementName.String(), SEXTEXT("function")))
			{
				isFunction = true;
			}
			else if (AreEqual(elementName.String(), SEXTEXT("method")))
			{
				isMethod = true;
			}

			if (isFunction || isMethod)
			{
				// (function <name> (args) -> (results): body)
				// (method <local-class>.<method-name> (args) -> (results): body)
				cr_sex functionName = GetAtomicArg(topLevelItem, 1);
				AssertValidFunctionName(functionName);

				FunctionPrototype prototype(functionName.String()->Buffer, isMethod);
				
				IFunctionBuilder& f = DeclareFunction(*this, topLevelItem, prototype);			

				CBindFnDefToExpression binding;
				binding.Fn = &f;
				binding.FnDef = &topLevelItem;
				localFunctions.insert(std::make_pair(CStringKey(f.Name()), binding));
			}
		}

		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = root.GetElement(i);
			cr_sex elementName = GetAtomicArg(topLevelItem, 0);

			if (AreEqual(elementName.String(), SEXTEXT("factory")))
			{
				DeclareFactory(topLevelItem);
			}
		}
	}

	void CScript::ComputeStructureFields()
	{
		for(auto j = localStructures.begin(); j != localStructures.end(); ++j)
		{
			for(int i = 2; i < j->StructDef->NumberOfElements(); i++)
			{
				cr_sex field = j->StructDef->GetElement(i);
				AddMember(*j->Struct, field);
			}
		}
	}

	void CScript::ComputeFunctionArgs()
	{
		for(auto i = localFunctions.begin(); i != localFunctions.end(); i++)
		{
			CBindFnDefToExpression& def = i->second;
			CompileDeclaration(*def.Fn, *def.FnDef, *this);
		}
	}

	IFunctionBuilder& DeclareFunction(IModuleBuilder& module, cr_sex source, FunctionPrototype& prototype)
	{
		try
		{
			IFunctionBuilder& f = module.DeclareFunction(prototype, &source);
			return f;
		}
		catch (IException& e)
		{
			sexstringstream streamer;
			streamer << e.Message();
			Throw(source, streamer);

			IFunctionBuilder* f = NULL;
			return *f;
		}		
	}

	IFunctionBuilder& DeclareFunction(CScript& script, cr_sex source, FunctionPrototype& prototype)
	{
		return DeclareFunction(script.ProgramModule(), source, prototype);
	}

	IScriptSystem& GetSystem(CScripts& scripts)
	{
		return scripts.System();
	}

	IScriptSystem& GetSystem(CScript& script)
	{
		return script.System();
	}

	bool IsIStringInlined(CScript& script)
	{
		return script.IsIStringInlined();
	}

	bool IsIStringInlined(CScripts& scripts)
	{
		return scripts.InlineStrings();
	}

	const IStructure& GetArrayDef(CCompileEnvironment& ce, cr_sex src, csexstr arrayName)
	{
		NamespaceSplitter splitter(arrayName);

		csexstr instance, arrayMember;
		if (splitter.SplitTail(instance, arrayMember))
		{
			const IStructure* s = ce.Builder.GetVarStructure(instance);
			if (s == NULL)
			{
				ThrowTokenNotFound(src, instance, ce.Builder.Owner().Name(), SEXTEXT("variable"));
			}

			int offset = 0;
			const IMember *m = FindMember(*s, arrayMember, REF offset);
			if (m == NULL)
			{
				ThrowTokenNotFound(src, arrayMember, instance, SEXTEXT("member"));
			}

			return *m->UnderlyingGenericArg1Type();
		}

		const ArrayDef* variableDef = ce.Script.GetArrayDef(ce.Builder, arrayName);
		if (variableDef == NULL)
		{
			ThrowTokenNotFound(src, arrayName, ce.Builder.Owner().Name(), SEXTEXT("member"));
		}

		return variableDef->ElementType;
	}

	void AddArrayDef(CScript& script, ICodeBuilder& builder, csexstr arrayName, const IStructure& elementType, cr_sex s)
	{
		script.AddArrayDef(builder, arrayName, elementType, s);
	}

	const IStructure& GetListDef(CCompileEnvironment& ce, cr_sex src, csexstr listName)
	{
		NamespaceSplitter splitter(listName);

		csexstr instance, listMember;
		if (splitter.SplitTail(instance, listMember))
		{
			const IStructure* s = ce.Builder.GetVarStructure(instance);
			if (s == NULL)
			{
				ThrowTokenNotFound(src, instance, ce.Builder.Owner().Name(), SEXTEXT("variable"));
			}

			int offset = 0;
			const IMember *m = FindMember(*s, listMember, REF offset);
			if (m == NULL)
			{
				ThrowTokenNotFound(src, listMember, instance, SEXTEXT("member"));
			}

			return *m->UnderlyingGenericArg1Type();
		}

		const ListDef* variableDef = ce.Script.GetListDef(ce.Builder, listName);
		if (variableDef == NULL)
		{
			ThrowTokenNotFound(src, listName, ce.Builder.Owner().Name(), SEXTEXT("member"));
		}

		return variableDef->ElementType;
	}

	void AddListDef(CScript& script, ICodeBuilder& builder, csexstr name, const IStructure& elementType, cr_sex s)
	{
		script.AddListDef(builder, name, elementType, s);
	}

	void AddMapDef(CScript& script, ICodeBuilder& builder, csexstr name, const IStructure& keyType, const IStructure& valueType, cr_sex s)
	{
		script.AddMapDef(builder, name, keyType, valueType, s);
	}

	void AddMapNodeDef(CScript& script, ICodeBuilder& builder, const MapDef& def, csexstr nodeName, csexstr mapName, cr_sex s)
	{
		script.AddMapNodeDef(builder, def, mapName, nodeName, s);
	}

	const MapDef GetMapDef(CCompileEnvironment& ce, cr_sex src, csexstr name)
	{
		NamespaceSplitter splitter(name);

		csexstr instance, member;
		if (splitter.SplitTail(instance, member))
		{
			const IStructure* s = ce.Builder.GetVarStructure(instance);
			if (s == NULL)
			{
				ThrowTokenNotFound(src, instance, ce.Builder.Owner().Name(), SEXTEXT("variable"));
			}

			int offset = 0;
			const IMember *m = FindMember(*s, member, REF offset);
			if (m == NULL)
			{
				ThrowTokenNotFound(src, member, instance, SEXTEXT("member"));
			}

			return MapDef(*(const ISExpression*) s->Definition(), *m->UnderlyingGenericArg1Type(), *m->UnderlyingGenericArg2Type());
		}

		const MapDef* mapDef = ce.Script.GetMapDef(ce.Builder, name);
		if (mapDef == NULL)
		{
			ThrowTokenNotFound(src, name, ce.Builder.Owner().Name(), SEXTEXT("member"));
		}

		return *mapDef;
	}

	const MapNodeDef& GetMapNodeDef(CCompileEnvironment& ce, cr_sex src, csexstr name)
	{
		const MapNodeDef* def = ce.Script.GetMapNodeDef(ce.Builder, name);
		if (def == NULL)
		{
			Throw(src, SEXTEXT("Internal compiler error: No definition for this node was found. "));
		}

		return *def;
	}

	const IStructure& GetNodeDef(CCompileEnvironment& ce, cr_sex src, csexstr nodeName)
	{
		NamespaceSplitter splitter(nodeName);

		csexstr instance, listMember;
		if (splitter.SplitTail(instance, listMember))
		{
			const IStructure* s = ce.Builder.GetVarStructure(instance);
			if (s == NULL)
			{
				ThrowTokenNotFound(src, instance, ce.Builder.Owner().Name(), SEXTEXT("variable"));
			}

			int offset = 0;
			const IMember *m = FindMember(*s, listMember, REF offset);
			if (m == NULL)
			{
				ThrowTokenNotFound(src, listMember, instance, SEXTEXT("member"));
			}

			return *m->UnderlyingGenericArg1Type();
		}

		const NodeDef* variableDef = ce.Script.GetNodeDef(ce.Builder, nodeName);
		if (variableDef == NULL)
		{
			ThrowTokenNotFound(src, nodeName, ce.Builder.Owner().Name(), SEXTEXT("member"));
		}

		return variableDef->ElementType;
	}

	void AddNodeDef(CScript& script, ICodeBuilder& builder, csexstr name, const IStructure& elementType, cr_sex s)
	{
		script.AddNodeDef(builder, name, elementType, s);
	}
}