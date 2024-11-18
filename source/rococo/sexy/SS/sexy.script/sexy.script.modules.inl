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

#include <rococo.time.h>

namespace Rococo::Script
{
	class CScript;
	class CScripts;

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

	void Resolve(IArgumentBuilder& arg, cstr type, cstr id, cstr source, cr_sex e, cstr genericArgType = NULL)
	{
		if (!arg.TryResolveArgument())
		{
			if (genericArgType != NULL)
			{
				Throw(e, "Unknown type in argument: *** %s *** %s %s of function %s", type, genericArgType, id, source);
			}
			else
			{
				Throw(e, "Unknown type in argument (*** %s *** %s of function %s", type, id, source);
			}
		}
	}

	IStructureBuilder& DeclareStructure(IModuleBuilder& module, cstr structName, const StructurePrototype& prototype, cr_sex src)
	{
		try
		{
			IStructureBuilder& s = module.DeclareStructure(structName, prototype, &src);
			
			return s;
		}
		catch (IException& e)
		{
			Throw(src, "%s", e.Message());
		}
	}

	IStructureBuilder& DeclareStrongType(IModuleBuilder& module, cstr strongName, cstr primitiveType, cr_sex src)
	{
		VARTYPE type;
		if (Eq(primitiveType, "Int32"))
		{
			type = VARTYPE_Int32;
		}
		else if (Eq(primitiveType, "Int64"))
		{
			type = VARTYPE_Int64;
		}
		else if (Eq(primitiveType, "Float32"))
		{
			type = VARTYPE_Float32;
		}
		else if (Eq(primitiveType, "Float64"))
		{
			type = VARTYPE_Float64;
		}
		else if (Eq(primitiveType, "Bool"))
		{
			type = VARTYPE_Bool;
		}
		else
		{
			Throw(src, "Expecting one of Int32|Int64|Float32|Float64|Bool");
		}

		try
		{
			IStructureBuilder& s = module.DeclareStrongType(strongName, type);
			return s;
		}
		catch (IException& e)
		{
			Throw(src, "%s", e.Message());
		}
	}

	INamespaceBuilder& ValidateSplitTail
		(
			REF NamespaceSplitter& splitter,
			OUT cstr& body,
			OUT cstr& publicName, 
			IN cr_sex s,
			IN IProgramObject& po,
			IN IModule& module
		)
	{
		if (!splitter.SplitTail(OUT body, OUT publicName))
		{
			Throw(s, ("Expected fully qualified name 'A.B.C.D'"));
		}

		INamespaceBuilder* ns = po.GetRootNamespace().FindSubspace(body);
		if (ns == NULL)
		{
			if (Eq(body, "$"))
			{
				ns = (INamespaceBuilder*)module.DefaultNamespace();
			}

			if (ns == nullptr)
			{
				Throw(s, "Cannot find namespace '%s' that prefixes the public name: '%s'", body, publicName);
			}
		}		

		return *ns;
	}

	bool IsLegalTypename(cstr __restrict begin, cstr __restrict end)
	{
		if (!IsCapital(*begin)) return false;

		for (auto* s = begin + 1; s < end; ++s)
		{
			auto c = *s;
			if (!IsCapital(c) && !IsLowerCase(c) && !IsNumeric(c))
			{
				return false;
			}
		}

		return true;
	}

	void MakeTypenameFromFilename(char* buf, size_t capacity, cstr filename, cr_sex src)
	{
		size_t len = strlen(filename);

		auto* end = filename + len;

		auto* rend = filename - 1;
		for (auto* s = end - 1; s != rend; s--)
		{
			if (*s == '.')
			{
				if (Eq(s, ".sxy"))
				{
					// Extension, which we skip
					end = s;
					break;
				}
			}
		}

		for (auto* s = end - 1; s != rend; s--)
		{
			if (*s == '.')
			{
				Throw(src, "The source filename has more than one dot, which prevents $ translating it to a Sexy type name");
			}
			else if (*s == '/')
			{
				if (!IsCapital(s[1]))
				{
					Throw(src, "The source filename did not begin with an upper case letter [A-Z], which prevents $ translating it to a Sexy type name");
				}

				if (end - s > 64)
				{
					Throw(src, "The source filename was too long, which prevents $ translating it to a Sexy type name");
				}

				if (!IsLegalTypename(s + 1, end))
				{
					Throw(src, "The source filename may only contain alphanumerics to translate $ to a Sexy type name");
				}

				memcpy_s(buf, capacity, s + 1, end - s - 1);
				buf[end - s - 1] = 0;
				return;
			}
		}

		Throw(src, "The source file path must contain a / character to translate $ to a Sexy type name");
	}

	void AppendAlias(IModuleBuilder& module, cr_sex nsName, cr_sex localName)
	{
		cstr body, tail;

		INamespaceBuilder* pNS;

		NamespaceSplitter splitter(nsName.c_str());

		if (Eq(nsName.c_str(), "$"))
		{
			pNS = static_cast<INamespaceBuilder*>(const_cast<INamespace*> (module.DefaultNamespace()));
			if (pNS == nullptr)
			{
				Throw(nsName, "The module has no default namespace. So $ is invalid");
			}
			body = pNS->FullName()->Buffer;
			
			auto* buf = splitter.Raw();

			MakeTypenameFromFilename(buf, NAMESPACE_MAX_LENGTH, nsName.Tree().Source().Name(), nsName);

			tail = buf;
		}
		else
		{
			pNS = &(ValidateSplitTail(splitter, body, tail, nsName, module.Object(), module));
		}

		INamespaceBuilder& ns = *pNS;

		cstr publicFunctionName = tail;

		cstr name = localName.c_str();

		IFunctionBuilder* f = module.FindFunction(name);
		if (f == NULL)
		{
			cstr nsBody, shortName;
			NamespaceSplitter localNameSplitter(name);
			if (localNameSplitter.SplitTail(nsBody, shortName))
			{
				INamespaceBuilder* nsSrc = MatchNamespace(module, nsBody);
				if (nsSrc == NULL)
				{
					Throw(localName, "Cannot resolve alias. Source name '%s' was not a reconigzed namespace", nsBody);
				}

				IStructureBuilder* s = nsSrc->FindStructure(shortName);
				if (s == NULL)
				{
					f = nsSrc->FindFunction(shortName);
					if (f == nullptr)
					{
						Throw(localName, "Cannot find '%s' in %s", shortName, nsBody);
					}
					else
					{
						ns.Alias(tail, *f);
					}
				}
				else
				{
					ns.Alias(tail, *s);
				}

				return;
			}

			IStructureBuilder* s = module.FindStructure(name);
			if (s == NULL)
			{
				Throw(localName, "Cannot resolve alias. Local name '%s' was neither a structure or a function", name);
			}
			else
			{
				if (s->Prototype().IsClass)
				{
					Throw(localName, "Aliasing a class is not allowed: '%s'", name);
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

	void AppendAliases(IScriptSystem& ss, IModuleBuilder& module, IN const ISParserTree& tree)
	{
		cr_sex sSequence = tree.Root();

		for(int i = 0; i < sSequence.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = sSequence[i];
			cr_sex elementNameExpr = GetAtomicArg(topLevelItem, 0);
			cstr elementName = elementNameExpr.c_str();
			if (AreEqual(elementName, "alias"))
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

	int GetIndexOf(int start, cr_sex s, cstr text)
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

	void AddGenericInput(IFunctionBuilder& f, cstr type, cr_sex id, cr_sex elementType, cr_sex src, CScript& script)
	{
		AssertQualifiedIdentifier(elementType);
		AssertLocalIdentifier(id);

		const sexstring idString = id.String();
		const sexstring firstTypeString = elementType.String();

		if (!AreEqual(type, "_Array") && !AreEqual(type, "_List"))
		{
			Throw(*id.Parent(), "Unexpected type in generic input definition");
		}
	
		IArgumentBuilder& arg = f.AddInput(NameString::From(idString), TypeString::From(type), TypeString::From(firstTypeString), (void*) &src);			
		Resolve(arg, type, idString->Buffer, f.Name(), *id.Parent(), firstTypeString->Buffer);

		if (AreEqual(type, "_Array"))
		{
			AddArrayDef(script, f.Builder(), id.c_str(), *arg.GenericTypeArg1(), src);
		}
		else if (AreEqual(type, "_List"))
		{
			AddListDef(script, f.Builder(), id.c_str(), *arg.GenericTypeArg1(), src);
		}
	}

	void AddGenericInput(IFunctionBuilder& f, cstr type, cr_sex id, cr_sex SType, cr_sex TType, cr_sex src, CScript& script)
	{
		AssertQualifiedIdentifier(SType);
		AssertQualifiedIdentifier(TType);
		AssertLocalIdentifier(id);

		const sexstring idString = id.String();
		const sexstring firstTypeString = SType.String();
		const sexstring secondTypeString = TType.String();

		if (!AreEqual(type, "_Map"))
		{
			Throw(*id.Parent(), ("Unexpected type in generic input definition"));
		}
	
		IArgumentBuilder& arg = f.AddInput(NameString::From(idString), TypeString::From(type), TypeString::From(firstTypeString), TypeString::From(secondTypeString), (void*) &src);			
		Resolve(arg, type, idString->Buffer, f.Name(), *id.Parent(), firstTypeString->Buffer);

		if (AreEqual(type, "_Map"))
		{
			AddMapDef(script, f.Builder(), id.c_str(), *arg.GenericTypeArg1(), *arg.GenericTypeArg2(), src);
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
			Throw(type, "The type must be an archetype in a closure input");
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

	void AddArrayOutput(IFunctionBuilder& f, cr_sex genericArg, cr_sex id, cr_sex src, CScript& script)
	{
		AssertQualifiedIdentifier(genericArg);
		AssertLocalIdentifier(id);

		const sexstring genericArgString = genericArg.String();
		const sexstring idString = id.String();

		IArgumentBuilder& arg = f.AddArrayOutput(NameString::From(idString), TypeString::From(genericArgString), (void*)&src);
		Resolve(arg, "_Array", idString->Buffer, f.Name(), src, genericArgString->Buffer);

		AddArrayDef(script, f.Builder(), idString->Buffer, *arg.GenericTypeArg1(), src);
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

	IStructure* GetLocalStructure(CScript& script, cstr className);

	void AddChildConstructor(cr_sex childConstruct)
	{
		// bang();
	}

	void AddThisPointer(REF IFunctionBuilder& method, cr_sex methodDef, CScript& script)
	{
		cstr name = method.Name();

		NamespaceSplitter splitter(name);

		cr_sex nameExpr = GetAtomicArg(methodDef, 1);

		cstr className, methodName;
		if (!splitter.SplitTail(OUT className, OUT methodName))
		{
			Throw(nameExpr, ("Expecting qualified method name using syntax: '<class>.<method-name>'"));
		}

		IStructure* s = GetLocalStructure(script, className);
		if (s == NULL)
		{
			Throw(nameExpr, "Could not find a matching class definition inside the module for %s", className);
		}

		IArgumentBuilder& arg = method.AddInput(NameString::From(THIS_POINTER_TOKEN), TypeString::From(className), (void*) &nameExpr);			
		Resolve(arg, className, THIS_POINTER_TOKEN, method.Name(), nameExpr);	
	}

	void AddAttributeToCurrentArgument(REF IFunctionBuilder& f, cr_sex sAttribute)
	{
		if (sAttribute.NumberOfElements() == 3)
		{
			AssertAtomicMatch(sAttribute[0], "attribute");
			cr_sex attributeName = sAttribute[1];
			if (!IsAtomic(attributeName))
			{
				Throw(attributeName, "Expecting atomic string, one of [default]");
			}

			if (Eq(attributeName.c_str(), "default"))
			{
				// Generation #1 defaults are simple string substitutions
				cr_sex defaultValue = sAttribute[2];
				if (!IsAtomic(defaultValue))
				{
					Throw(defaultValue, "Expecting atomic string, which is the default value of the argument");
				}

				f.AddDefaultToCurrentArgument(defaultValue.c_str());
			}
		}
	}

	void AddInputs(REF IFunctionBuilder& f, cr_sex fdef, int inputStart, int inputEnd, CScript& script)
	{
		for(int i = inputStart; i <= inputEnd; ++i)
		{
			cr_sex inputItem = fdef.GetElement(i);
			AssertNotTooFewElements(inputItem, 2);
			AssertNotTooManyElements(inputItem, 4);

			cr_sex sexType = GetAtomicArg(inputItem, 0);

			cstr type = sexType.c_str();

			if (AreEqual(type, "array"))
			{
				AssertNotTooFewElements(inputItem, 3);
				AssertNotTooManyElements(inputItem, 3);

				cr_sex elementType = GetAtomicArg(inputItem, 1);
				cr_sex sexIdentifier = GetAtomicArg(inputItem, 2);

				AddGenericInput(f, "_Array", sexIdentifier, elementType, inputItem, script);				
			}
			else if (AreEqual(type, "list"))
			{
				AssertNotTooFewElements(inputItem, 3);
				AssertNotTooManyElements(inputItem, 3);

				cr_sex elementType = GetAtomicArg(inputItem, 1);
				cr_sex sexIdentifier = GetAtomicArg(inputItem, 2);
				
				AddGenericInput(f, "_List", sexIdentifier, elementType, inputItem, script);				
			}
			else if (AreEqual(type, "map"))
			{
				AssertNotTooFewElements(inputItem, 4);
				cr_sex keyType = GetAtomicArg(inputItem, 1);
				cr_sex valueType = GetAtomicArg(inputItem, 2);
				cr_sex sexIdentifier = GetAtomicArg(inputItem, 3);

				AddGenericInput(f, "_Map", sexIdentifier, keyType, valueType, inputItem, script);
			}
			else if (AreEqual(type, "closure"))
			{
				AssertNotTooFewElements(inputItem, 3);
				AssertNotTooManyElements(inputItem, 3);

				cr_sex sArchetype = GetAtomicArg(inputItem, 1);
				cr_sex sId = GetAtomicArg(inputItem, 2);
				AddClosureInput(f, sArchetype, sId, inputItem);
			}
			else if (AreEqual(type, "const"))
			{
				AssertNotTooFewElements(inputItem, 3);
				AssertNotTooManyElements(inputItem, 3);

				cr_sex sexStructType = GetAtomicArg(inputItem, 1);
				cr_sex sexIdentifier = GetAtomicArg(inputItem, 2);

				AddInput(f, sexStructType, sexIdentifier, inputItem);
			}
			else if (AreEqual(type, "out"))
			{
				AssertNotTooFewElements(inputItem, 3);
				AssertNotTooManyElements(inputItem, 3);

				cr_sex sexStructType = GetAtomicArg(inputItem, 1);
				cr_sex sexIdentifier = GetAtomicArg(inputItem, 2);

				AddInput(f, sexStructType, sexIdentifier, inputItem);
			}
			else if (inputItem.NumberOfElements() == 2)
			{
				cr_sex sexIdentifier = GetAtomicArg(inputItem, 1);
				AddInput(f, sexType, sexIdentifier, inputItem);
			}
			else if(inputItem.NumberOfElements() == 3)
			{
				cr_sex sexIdentifier = GetAtomicArg(inputItem, 1);
				AddInput(f, sexType, sexIdentifier, inputItem);

				cr_sex sAttribute = inputItem[2];
				if (!IsCompound(sAttribute))
				{
					Throw(sAttribute, "Expecting compound attribute element");
				}

				AddAttributeToCurrentArgument(f, sAttribute);
			}
		}	
	}


	void ValidateChildConstructorExists(int startPos, int endPos, cr_sex constructorDef, const IMember& m)
	{
		int constructCount = 0;

		if (startPos > endPos)
		{
			Throw(constructorDef, "Expected a child constructor indicator '->' after the constructor input arguments,\r\n followed by (construct %s arg1... argN) amongst the child constructors.", m.Name());
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
			Throw(constructorDef, "Could not find child constructor for %s.\r\nExpected (construct %s arg1... argN) amongst the child constructors.", m.Name(), m.Name());
		}
		else if (constructCount > 1)
		{
			Throw(constructorDef, "Conflicting child constructors for %s", m.Name());
		}
	}

	void ValidateChildConstructors(const IFunction& constructor, cr_sex constructorDef, int startPos, int endPos)
	{
		cstr name = constructor.Name();

		NamespaceSplitter splitter(name);

		cstr classType, constructMethod;
		splitter.SplitTail(classType, constructMethod);

		const IStructure* type = constructor.Module().FindStructure(classType);
		for(int i = 0; i < type->MemberCount(); ++i)
		{
			const IMember& m = type->GetMember(i);
			if (m.UnderlyingGenericArg1Type() != NULL && m.UnderlyingType()->VarType() != VARTYPE_Array && m.UnderlyingType()->VarType() != VARTYPE_Map && m.UnderlyingType()->VarType() != VARTYPE_List)
			{
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
		
		cstr name = constructor.Name();

		IStructureBuilder* type = (IStructureBuilder*) constructor.GetType();
		type->SetConstructor(&constructor);

		AssertNotTooFewElements(constructorDef, 3); // -> Minimist constructor: (method Eros.Construct :)
		
		int bodyIndex = GetIndexOf(2, constructorDef, (":"));					
		if (bodyIndex < 0)
		{			
			Throw(constructorDef, ("Expecting body indication token ':' inside the constructor definition"));
		}

		int childConstructIndex = GetIndexOf(2, constructorDef, ("->"));
		if (childConstructIndex < 0)
		{
			childConstructIndex = bodyIndex;
		}	
		else if (childConstructIndex > bodyIndex)
		{
			Throw(constructorDef, ("Expecting child construction token '->' to occur before the body indication token ':' inside the constructor definition"));
		}

		int childConstructorStart = childConstructIndex+1;
		int childConstructorEnd = bodyIndex-1;

		AddInputs(constructor, constructorDef, 2, childConstructIndex-1, script);
		AddThisPointer(constructor, constructorDef, script);
	}

	void AddOutputs(REF IFunctionBuilder& f, int outputStart, int outputEnd, cr_sex fdef, CScript& script)
	{
		for(int i = outputStart; i <= outputEnd; ++i)
		{
			cr_sex outputItem = fdef.GetElement(i);
			AssertCompound(outputItem);
			AssertNotTooFewElements(outputItem, 2);
			AssertNotTooManyElements(outputItem, 3);

			cr_sex sexType = GetAtomicArg(outputItem, 0);

			if (outputItem.NumberOfElements() == 3)
			{
				cr_sex genericArg = GetAtomicArg(outputItem, 1);

				if (!Eq(sexType.c_str(), "array"))
				{
					Throw(sexType, "Expecting 'array' at this position");
				}

				cr_sex arrayIdentifier = GetAtomicArg(outputItem, 2);
				AddArrayOutput(f, genericArg, arrayIdentifier, outputItem, script);
			}
			else
			{
				cr_sex sexIdentifier = GetAtomicArg(outputItem, 1);
				AddOutput(f, sexType, sexIdentifier, outputItem);
			}
		}
	}

	bool IsConstructor(const IFunction& f)
	{		
		if (f.IsVirtualMethod())
		{
			cstr name = f.Name();

			NamespaceSplitter splitter(name);
			cstr path,publicName;
			if (splitter.SplitTail(path, publicName))
			{
				if (AreEqual(publicName, ("Construct")))
				{
					return true;
				}
			}
		}

		return false;
	}

	void CompileProxyFunction(REF IFunctionBuilder& f, IN cr_sex fdef, CScript& script);

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

		cstr classTypeName, methodName;
		if (splitter.SplitTail(classTypeName, methodName))
		{
			const IStructure* s = f.Module().FindStructure(classTypeName);
			if (s == NULL)
			{
				Throw(fdef, "Unknown structure: %s", classTypeName);
			}

			f.SetType(s);
		}

		if (IsConstructor(f))
		{
			CompileConstructor(f, fdef, script);
			return;
		}

		cstr name = f.Name();

		AssertNotTooFewElements(fdef, 4);
	
		int mapIndex = GetIndexOf(2, fdef, ("->"));

		if (mapIndex < 0)
		{			
			Throw(fdef, ("Expecting mapping token '->' inside the function definition"));
		}

		int bodyIndex = GetIndexOf(mapIndex, fdef, (":"));
		if (bodyIndex == -1) Throw(fdef, ("Expecting body indicator token ':' after the mapping token and inside the function definition"));

		AddOutputs(f, mapIndex+1, bodyIndex-1, fdef, script);
		AddInputs(f, fdef, 2, mapIndex-1, script);

		if (f.IsVirtualMethod())
		{
			AddThisPointer(REF f, fdef, script);
			CompileProxyFunction(f, fdef, script);
		}		
	}

	void AppendInvokeCallDestructor(CCompileEnvironment& ce, const IStructure& s, cstr name, int SFoffset)
	{
		TokenBuffer destructorName;
		StringPrint(destructorName, ("%s.Destruct"), s.Name());
		
		if (!IsNullType(s))
		{
			if (s == ce.StructList())
			{
				ce.Builder.Assembler().Append_GetStackFrameValue(SFoffset, VM::REGISTER_D7, BITCOUNT_POINTER);
				AppendInvoke(ce, GetListCallbacks(ce).ListRelease, *(const ISExpression*) s.Definition());
				return;
			}

			if (s == ce.StructMap())
			{
				ce.Builder.Assembler().Append_GetStackFrameValue(SFoffset, VM::REGISTER_D7, BITCOUNT_POINTER);
				AppendInvoke(ce, GetMapCallbacks(ce).MapRelease,  *(const ISExpression*) s.Definition());
				return;
			}

			if (s == ce.StructArray())
			{
				ce.Builder.Assembler().Append_GetStackFrameValue(SFoffset, VM::REGISTER_D7, BITCOUNT_POINTER);
				AppendInvoke(ce, GetArrayCallbacks(ce).ArrayRelease, *(const ISExpression*)s.Definition());
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
		StringPrint(destructorSymbol, ("%s %s.Destruct"), GetFriendlyName(s), name);
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

			if (IsPrimitiveType(memberType.VarType()) || m.IsInterfaceVariable())
			{
				// primitive types are skipped, as are interface variables, as the pointers point to something that are destructed elsewhere
			}
			else	
			{
				AppendInvokeDestructorChildren(ce, memberType, SFoffset + subOffset);
				AppendInvokeCallDestructor(ce, memberType, m.Name(), SFoffset + subOffset);	
			}

			subOffset += m.SizeOfMember();
		}
	}

	void AppendInvokeDestructor(CCompileEnvironment& ce, cstr instanceName, cr_sex sequence, const MemberDef& instanceDef)
	{
		const IStructure& s = *instanceDef.ResolvedType;

		if (s == ce.StructList())
		{
			ce.Builder.AssignVariableToTemp(instanceName, Rococo::ROOT_TEMPDEPTH);
			AppendInvoke(ce, GetListCallbacks(ce).ListRelease, sequence);
			return;
		}
		else if (s == ce.StructMap())
		{
			ce.Builder.AssignVariableToTemp(instanceName, Rococo::ROOT_TEMPDEPTH);
			AppendInvoke(ce, GetMapCallbacks(ce).MapRelease, sequence);
			return;
		}
		else if (AreEqual(s.Name(), "_Lock") && AreEqual(instanceName, "_array", 6))
		{
			char srcName[256];
			SafeFormat(srcName, 256, "%s._lockSource", instanceName);
			ce.Builder.AssignVariableToTemp(srcName, 9); // Lock source Goes to D13
			ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayUnlock);
		}

		AppendInvokeDestructorChildren(ce, s, instanceDef.SFOffset);
		AppendInvokeCallDestructor(ce, s, instanceName, instanceDef.SFOffset);
	}

	void AppendDeconstructOneVariable(CCompileEnvironment& ce, cr_sex sequence, int index)
	{
		MemberDef def;
		cstr instanceName;
		ce.Builder.GetVariableByIndex(OUT def, OUT instanceName, index);

		if (def.SFOffset < 0) Throw(sequence, ("Algorithmic error #2 in deconstruction logic"));

		if (def.Usage == ARGUMENTUSAGE_BYVALUE && def.ResolvedType->VarType() == VARTYPE_Derivative && def.ResolvedType->InterfaceCount() == 0)
		{
			AppendInvokeDestructor(ce, instanceName, sequence, def);
		}

		if (*def.ResolvedType == ce.StructArray())
		{
			CompileArrayDestruct(ce, *def.ResolvedType, instanceName);
		}
		else if (*def.ResolvedType == ce.StructMap())
		{
			AppendInvokeDestructor(ce, instanceName, sequence, def);
		}
		else if (*def.ResolvedType == ce.StructList())
		{
			AppendInvokeDestructor(ce, instanceName, sequence, def);
		}
		else if (*def.ResolvedType == ce.Object.Common().TypeNode())
		{
			// Expecting node to be by ARGUMENTUSAGE_BYVALUE and allocsize to be zero, as the only such here are pseudo variables
			// After the pseudo variable we can expect the actual reference, a pointer type by value

			ce.Builder.AssignVariableRefToTemp(instanceName, Rococo::ROOT_TEMPDEPTH);
			AppendInvoke(ce, GetListCallbacks(ce).NodeReleaseRef, sequence); // release the ref to the node
		}
		else if (*def.ResolvedType == ce.Object.Common().TypeMapNode())
		{
			// Expecting node to be by ARGUMENTUSAGE_BYVALUE and allocsize to be zero, as the only such here are pseudo variables
			// After the pseudo variable we can expect the actual reference, a pointer type by value

			ce.Builder.AssignVariableRefToTemp(instanceName, Rococo::ROOT_TEMPDEPTH);
			AppendInvoke(ce, GetMapCallbacks(ce).MapNodeReleaseRef, sequence); // release the ref to the node
		}
	}

	/* Used to deconstruct all nested sequences. Typically when a return statement is hit*/
	void AppendDeconstructAll(CCompileEnvironment& ce, cr_sex sequence)
	{
		int backRef = ce.Builder.GetVariableCount() - 1;

		int numberOfVariables = ce.Builder.GetVariableCount() - ce.Builder.Owner().NumberOfInputs() - ce.Builder.Owner().NumberOfOutputs();

		for(int i = numberOfVariables; i > 0; i--)
		{
			if (backRef < 0) Throw(sequence, ("Algorithmic error #1 in deconstruction logic"));

			AppendDeconstructOneVariable(ce, sequence, backRef);

			backRef--;
		}

		ce.Builder.PopLastVariables(numberOfVariables, false);
	}

	void AppendDeconstructTailVariables(CCompileEnvironment& ce, cr_sex sequence, bool expire, int tailCount)
	{
		int backRef = ce.Builder.GetVariableCount() - 1;

		int numberOfVariables = ce.Builder.GetVariableCount() - tailCount;

		for (int i = numberOfVariables; i > 0; i--)
		{
			if (backRef < 0) Throw(sequence, ("Algorithmic error #1 in deconstruction logic"));
			AppendDeconstructOneVariable(ce, sequence, backRef);
			backRef--;
		}

		ce.Builder.PopLastVariables(numberOfVariables, expire);
	}

	void AppendDeconstruct(CCompileEnvironment& ce, cr_sex sequence, bool expire)
	{
		AppendDeconstructTailVariables(ce, sequence, expire, ce.Builder.SectionArgCount());
	}

	void CompileExpressionSequenceProtected(CCompileEnvironment& ce, int start, int end, cr_sex sequence)
	{
		if (sequence.Type() == EXPRESSION_TYPE_NULL)
		{
			ce.Builder.Assembler().Append_NoOperation();
			return;
		}

		AssertCompound(sequence);
		if (start == 0 &&  sequence.NumberOfElements() > 0 && end == sequence.NumberOfElements()-1)
		{
			cr_sex firstArg = sequence.GetElement(0);
			if (firstArg.Type() == EXPRESSION_TYPE_ATOMIC)
			{
				ce.Builder.EnterSection();
				CompileExpression(ce, sequence); 
				AppendDeconstruct(ce, sequence, true);
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

		AppendDeconstruct(ce, sequence, true);
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
			char buf[256];
			StackStringBuilder ssb(buf, sizeof buf);
			StreamSTCEX(ssb, ex);
			Throw(sequence, "%s", buf);
		}
	}

	int GetInterfaceImplementingMethod(const IStructure& s, cstr methodName)
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

		cstr className, methodName;

		NamespaceSplitter splitter(f.Name());
		if (!splitter.SplitTail(OUT className, OUT methodName))
			Throw(fdef, "Internal compiler error. Expecting function name to be splittable");

		if (f.NumberOfInputs() <= 0) Throw(fdef, ("No inputs found in method!"));

		const IStructure& s = f.GetArgument(ArgCount(f)-1);

		if (!IsPointerValid(&s))
			Throw(fdef, "Expecting class to be defined in the same module in which the class-method is defined");

		if (AreEqual(methodName, "Destruct"))
		{
			// Destructors are from the vTable aligned to the instance, ergo no correction
			return 0;
		}

		if (AreEqual(methodName, "Construct"))
		{
			// Concrete constructors always take the instance in the this pointer, never an interface, ergo no correction
			return 0;
		}

		int interfIndex = GetInterfaceImplementingMethod(s, methodName);
		if (interfIndex < 0)
		{
			return 0;
		}
			
		// TODO - delete this comment Throw(fdef, ("Expecting method to be found amongst the interfaces of the class for which it is defined"));

		int correction =  ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0 + interfIndex * sizeof(VirtualTable*); 
		return correction;
	}

	void* GetInterfacePtrFromNullInstancePtr(void* instancePtr)
	{
		auto* header = (ObjectStub*) instancePtr;
		return &header->pVTables[0];
	}

	void CompileSetOutputToNull(REF IFunctionBuilder& f)
	{
		// Initialize output refs to null objects
		for(int i = 0; i < f.NumberOfOutputs(); ++i)
		{
			const IArgument& arg = f.Arg(i);
			if (arg.Direction() == ARGDIRECTION_OUTPUT)
			{
				MemberDef def;
				f.Builder().TryGetVariableByName(OUT def, arg.Name());

				const IStructure& argType = *arg.ResolvedType();
				if (IsNullType(argType))
				{
					VariantValue nullValue;
					nullValue.vPtrValue = GetInterfacePtrFromNullInstancePtr(argType.GetInterface(0).UniversalNullInstance());
					char symbol[128];
					SafeFormat(symbol, 128, ("%s = null object"), arg.Name());
					f.Builder().AddSymbol(symbol);
					f.Builder().Assembler().Append_SetStackFrameImmediate(def.SFOffset, nullValue, BITCOUNT_POINTER);
				}
				else if (argType.VarType() == VARTYPE_Closure)
				{
					auto& member0 = def.ResolvedType->GetMember(0);
					auto& member1 = def.ResolvedType->GetMember(0);

					VariantValue nullValue;
					nullValue.int64Value = 0;
					auto bitcount = GetBitCount(argType.VarType());
					f.Builder().Assembler().Append_SetStackFrameImmediate(def.SFOffset, nullValue, BITCOUNT_POINTER);
					f.Builder().Assembler().Append_SetStackFrameImmediate(def.SFOffset + sizeof(size_t), nullValue, BITCOUNT_POINTER);
				}
				else
				{
					VariantValue nullValue;
					nullValue.int64Value = 0;
					auto bitcount = GetBitCount(argType.VarType());
					f.Builder().Assembler().Append_SetStackFrameImmediate(def.SFOffset, nullValue, bitcount);
				}
			}
			else
			{
				OS::BreakOnThrow(OS::Flags::BreakFlag_SS);
			}
		}
	}

	void ConstructMemberByRef(CCompileEnvironment& ce, cr_sex args, int registerIndexToRef, const IStructure& type, int offset)
	{
		if (registerIndexToRef == REGISTER_D7)
		{
			Throw(args, "Algorithmic error. ConstructMemberByRef must not modify D7");
		}

		if (registerIndexToRef == REGISTER_D4)
		{
			Throw(args, "Algorithmic error. ConstructMemberByRef must not modify D4");
		}

		int tempDepth = registerIndexToRef - VM::REGISTER_D4;

		if (IsAtomic(args))
		{
			MemberDef def;
			const fstring& sargs = GetAtomicArg(args);
			if (!ce.Builder.TryGetVariableByName(def, sargs.buffer))
			{
				Throw(args, "Expecting a variable name");
			}
			
			if (def.ResolvedType != &type)
			{
				Throw(args, "Expecting a variable of type %s, but found a %s", GetFriendlyName(type), GetFriendlyName(*def.ResolvedType));
			}

			ce.Builder.AssignVariableAddressToTemp(sargs.buffer, 0); // the src ptr is now in D4
			ce.Builder.Assembler().Append_CopyMemory(registerIndexToRef, REGISTER_D4, def.ResolvedType->SizeOfStruct());
			return;
		}
		else if (args.NumberOfElements() != type.MemberCount())
		{
			Throw(args, "The number of arguments supplied in the memberwise constructor is %d, while the number of members in %s is %d", args.NumberOfElements(), GetFriendlyName(type), type.MemberCount());
		}

		for(int i = 0; i < type.MemberCount(); ++i)
		{
			cr_sex arg = args.GetElement(i);
			const IMember& m = type.GetMember(i);
			const IStructure& mtype = *m.UnderlyingType();
			if (IsPrimitiveType(mtype.VarType()))
			{
				if (IsAtomic(arg))
				{
					cstr svalue = arg.c_str();
					if (svalue[0] == '-' || isdigit(svalue[0]) || Eq(svalue, "true") || Eq(svalue,"false"))
					{
						VariantValue value;
						if (Parse::TryParse(OUT value, mtype.VarType(), svalue) == Parse::PARSERESULT_GOOD)
						{
							ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, value, GetBitCount(mtype.VarType()));
							ce.Builder.Assembler().Append_Poke(VM::REGISTER_D7, GetBitCount(mtype.VarType()), registerIndexToRef, offset); // write the argument to the member	
							offset += m.SizeOfMember();
							continue;
						}
					}
				}

				// ToDo -> remove archiving when the assembly does not overwrite any registers, save D7
				AddArchiveRegister(ce, tempDepth, tempDepth, BITCOUNT_POINTER);
				CompileNumericExpression(ce, arg, mtype.VarType()); // Numeric value in D7
				ce.Builder.PopLastVariables(1,true); // registerIndexToRef contains the value pointer
				ce.Builder.Assembler().Append_Poke(VM::REGISTER_D7, GetBitCount(mtype.VarType()), registerIndexToRef, offset); // write the argument to the member	
			}
			else if (mtype.VarType() == VARTYPE_Derivative)
			{
				if (mtype.Prototype().IsClass && !IsNullType(mtype))
				{
					Throw(arg, ("Internal Compiler Error. The member type was a class, which cannot be memberwise constructed"));
				}

				if (mtype.InterfaceCount() > 0)
				{
					ce.Builder.AssignVariableToTemp(GetAtomicArg(arg), 3); // Numeric value in D7
					ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D4, BITCOUNT_POINTER);
					ce.Builder.Append_IncRef();
					ce.Builder.Assembler().Append_Poke(VM::REGISTER_D7, BITCOUNT_POINTER, registerIndexToRef, offset); // write the argument to the member
				}
				else
				{
					ConstructMemberByRef(ce, arg, registerIndexToRef, mtype, offset);
				}
			}
			else
			{
				Throw(arg, ("Internal Compiler Error. ConstructMemberByRef found an unusual member type"));
			}

			offset += m.SizeOfMember();
		}
	}

	void CompileListConstruct(CCompileEnvironment& ce, cr_sex def, const IMember& member, cstr fullName)
	{
		ce.Builder.AssignVariableRefToTemp(fullName, 0); // List goes to D4

		VariantValue v;
		v.vPtrValue = (void*) member.UnderlyingGenericArg1Type();
		ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, v, BITCOUNT_POINTER); // Element type to D5
				
		AddListDef(ce.Script, ce.Builder, fullName, *member.UnderlyingGenericArg1Type(), def);

		ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).ListInit);		
	}

	void CompileMapConstruct(CCompileEnvironment& ce, cr_sex def, const IMember& member, cstr fullName)
	{
		ce.Builder.AssignVariableRefToTemp(fullName, Rococo::ROOT_TEMPDEPTH); // Map goes to D7

		VariantValue key;
		key.vPtrValue = (void*) member.UnderlyingGenericArg1Type();
		ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, key, BITCOUNT_POINTER); // Key type to D5

		VariantValue val;
		val.vPtrValue = (void*) member.UnderlyingGenericArg2Type();
		ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, val, BITCOUNT_POINTER); // Value type to D4
				
		ce.Builder.Assembler().Append_Invoke(GetMapCallbacks(ce).MapInit);		
	}

	void CompileInvokeChildConstructor(CCompileEnvironment& ce, cr_sex conDef, const IMember& member, cstr instance)
	{		
		// (construct member <arg1>...<argN>)
		const IFunction* childConstructorFn = member.UnderlyingType()->Constructor();
		if (childConstructorFn == NULL)
		{
			Throw(conDef, "[%s] of type %s: superfluous child constructor", member.Name(), GetFriendlyName(*member.UnderlyingType()));
		}
		
		int inputCount = childConstructorFn->NumberOfInputs();

		if (inputCount < conDef.NumberOfElements() - 1)
		{
			Throw(conDef, ("Too many arguments supplied to the child constructor"));
		}
		else if (inputCount > conDef.NumberOfElements() - 1)
		{
			Throw(conDef, ("Insufficient arguments supplied to the child constructor"));
		}


		int inputStackAllocCount = PushInputs(ce, conDef, *childConstructorFn, true, 2);		
		inputStackAllocCount += CompileInstancePointerArg(ce, instance);

		AppendFunctionCallAssembly(ce, *childConstructorFn); 

		ce.Builder.MarkExpression(&conDef);

		RepairStack(ce, *conDef.Parent(), *childConstructorFn);
		ce.Builder.AssignClosureParentSFtoD6();
	}

	void CompileConstructMember(CCompileEnvironment& ce, cr_sex conDef, const IMember& member, cstr instance)
	{
		if (*member.UnderlyingType() == ce.StructList())
		{
			CompileListConstruct(ce, conDef, member, instance);
		}
		else
		{
			CompileInvokeChildConstructor(ce, conDef, member, instance);
		}
	}

	void CompileConstructChild(CCompileEnvironment& ce, cr_sex conDef, const IStructure& parentType, cstr parentInstance)
	{
		AssertNotTooFewElements(conDef, 2);
		
		cr_sex constructDirective = GetAtomicArg(conDef, 0);
		if (!AreEqual(constructDirective.String(), ("construct")))
		{
			Throw(constructDirective, ("Expecting 'construct' keyword in this directive"));
		}

		cr_sex fieldNameExpr = GetAtomicArg(conDef, 1);
		cstr fieldName = fieldNameExpr.c_str();

		TokenBuffer fullFieldName;
		StringPrint(fullFieldName, ("%s.%s"), parentInstance, fieldName);

		int offset = 0;
		const IMember* member = FindMember(parentType, fieldName, OUT offset);
		if (member == NULL) ThrowTokenNotFound(fieldNameExpr, fieldName, parentType.Name(), ("member"));
		
		CompileConstructMember(ce, conDef, *member, fullFieldName);
	}

	void CompileConstructChildren(CCompileEnvironment& ce, int firstChildConstructIndex, int endChildConstructIndex, cr_sex constructorDef, const IStructure& parentType)
	{
		for(int i = firstChildConstructIndex; i <= endChildConstructIndex; ++i)
		{
			cr_sex childConstructDef = constructorDef.GetElement(i);
			CompileConstructChild(ce, childConstructDef, parentType, ("this"));
		}
	}

	void CompileConstructorFromExpression(REF IFunctionBuilder& constructor, IN cr_sex constructorDef, CScript& script)
	{
		cstr name = constructor.Name();

		// (method <type-name>.Construct  (inputType1 inputVar1) ... (inputTypeN inputVarN) -> (construct <memberName1> arg1 arg2 arg3 ) ... (construct <memberName2> arg1 arg2 arg3): body )

		int bodyIndex = GetIndexOf(2, constructorDef, (":"));
		if (bodyIndex == -1) Throw(constructorDef, ("Expecting body indicator token ':' after the optional child construct token '->' and just before the constructor definition"));

		int mapIndex = GetIndexOf(2, constructorDef, ("->"));
		if (mapIndex > bodyIndex) Throw(constructorDef, ("Expecting construct token '->' before the body indicator token inside the constructor definition"));
		else if (mapIndex == -1) mapIndex = bodyIndex;

		ICodeBuilder& builder = constructor.Builder();
		
		// The final input is an interface
		int thisOffset = ComputeThisOffset(builder.Owner(), constructorDef);
		builder.SetThisOffset(thisOffset);

		NamespaceSplitter splitter(name);
		cstr typeName, shortName;
		splitter.SplitTail(typeName, shortName);

		const IStructure* type = constructor.Module().FindStructure(typeName);

		ValidateChildConstructors(constructor, constructorDef, mapIndex+1, bodyIndex-1);
		
		builder.Begin();

			CCompileEnvironment ce(script, builder);

			InitClassMembers(ce, "this");
		
			CompileConstructChildren(ce, mapIndex+1, bodyIndex-1, constructorDef, *type);

			CompileExpressionSequence(ce, bodyIndex+1, constructorDef.NumberOfElements()-1, constructorDef);

		builder.End();
		builder.Assembler().Clear();
	}

	void Disassemble(VM::IDisassembler& disassembler, const IFunction& f, IPublicScriptSystem& ss)
	{
		// Disassemble2(disassembler, f, ss);
	}

	void Disassemble2(VM::IDisassembler& disassembler, const IFunction& f, IPublicScriptSystem& ss)
	{
		CodeSection section;
		f.Code().GetCodeSection(OUT section);

		WriteToStandardOutput("\n------------%s #%lld ---------------\n", f.Name(), section.Id);

		size_t start = ss.PublicProgramObject().ProgramMemory().GetFunctionAddress(section.Id);
		size_t programLength = ss.PublicProgramObject().ProgramMemory().GetFunctionLength(section.Id);
		const uint8* code = ss.PublicProgramObject().ProgramMemory().StartOfMemory() + start;
		size_t i = 0;
		while (i < programLength)
		{
			VM::IDisassembler::Rep rep;
			disassembler.Disassemble(code + i, OUT rep);

			cstr symbol = f.Code().GetSymbol(i).Text;
			if (*symbol != 0)
			{
				WriteToStandardOutput("%8llu %s %s //%s\n", i, rep.OpcodeText, rep.ArgText, symbol);
			}
			else
			{
				WriteToStandardOutput("%8llu %s %s\n", i, rep.OpcodeText, rep.ArgText);
			}

			i += rep.ByteCount;
		}

		WriteToStandardOutput("\n\n");
	}

	void CompileProxyFunction(REF IFunctionBuilder& f, IN cr_sex fdef, CScript& script)
	{
		CodeSection section;
		f.Builder().GetCodeSection(OUT section);

		auto proxyId = f.Object().ProgramMemory().AddBytecode();
		f.SetProxy(proxyId);

		auto& a = f.Builder().Assembler();

		auto& ss = script.System();
		
		a.Append_Invoke(ss.GetScriptCallbacks().idJumpFromProxyToMethod);

		// Dummy instructions: the JumpToEncodedAddress method looks at the [idValue] instruction below and extracts the value, the register is never changed
		// We could put this before the jump, but that would mean every debug session would potentially step through the argument. By extracting the code manually we avoid that step
		VariantValue idValue;
		idValue.byteCodeIdValue = section.Id;
		a.Append_SetRegisterImmediate(REGISTER_D4, idValue, BITCOUNT_POINTER);

		VariantValue functionRef;
		functionRef.vPtrValue = &f;
		a.Append_SetRegisterImmediate(REGISTER_D4, functionRef, BITCOUNT_64);

		f.Object().ProgramMemory().UpdateBytecode(proxyId, a);

		a.Clear();
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

		cstr fname = f.Name();

		if (IsConstructor(f))
		{
			 CompileConstructorFromExpression(REF f, fdef, script);
			 return;
		}

		int mapIndex = GetIndexOf(2, fdef, "->");
		if (mapIndex == -1) Throw(fdef, "Expecting mapping token '->' inside the function definition");

		int bodyIndex = GetIndexOf(mapIndex, fdef, ":");
		if (bodyIndex == -1) Throw(fdef, "Expecting body indicator token ':' after the mapping token and inside the function definition");

		ICodeBuilder& builder = f.Builder();
		
		if (f.IsVirtualMethod())
		{
			// The final input is an interface
			int thisOffset = ComputeThisOffset(builder.Owner(), fdef);
			builder.SetThisOffset(thisOffset);
		}
		
		builder.Begin();

		CCompileEnvironment ce(script, builder);

		try
		{
			CompileSetOutputToNull(REF f);
			CompileExpressionSequence(ce, bodyIndex + 1, fdef.NumberOfElements() - 1, fdef);

			builder.End();
		}
		catch (ParseException&)
		{
			throw;
		}
		catch (IException& ex)
		{
			Throw(fdef, "%s", ex.Message());
		}

#ifdef _DEBUG
		if (Rococo::OS::IsDebugging())
		{
			AutoFree<VM::IDisassembler> disassembler ( ce.Object.VirtualMachine().Core().CreateDisassembler() );
			Disassemble(*disassembler, f, ce.SS);
		}
#endif

		builder.Assembler().Clear();
	}

	IFunctionBuilder& DeclareFunction(CScript& script, cr_sex source, FunctionPrototype& prototype);

	void CompileNullMethod_Throws(IScriptSystem& ss, const IInterface& interface, IFunctionBuilder& f)
	{
		VariantValue v;
		v.vPtrValue = (IInterface*) &interface;
		f.Builder().Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_POINTER);

		v.vPtrValue = (IFunction*) &f;
		f.Builder().Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, v, BITCOUNT_POINTER);

		f.Builder().Assembler().Append_Invoke(ss.GetScriptCallbacks().idThrowNullRef);
	}

	VM_CALLBACK(TransformAt_D4D5retD7)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		auto* i = (InterfacePointer)registers[4].vPtrValue;
		auto* builder = reinterpret_cast<CClassExpression*>(InterfaceToInstance(i));

		cr_sex s = *builder->ExpressionPtr;

		if (!&s)
		{
			Throw(0, "ExpressionBuilder's internal expression pointer was null");
		}

		auto* original = s.GetOriginal();
		if (!original)
		{
			Throw(0, "ExpressionBuilder's original expression pointer was null");
		}

		int index = registers[5].int32Value;

		if (index < 0 || index >= original->NumberOfElements())
		{
			Throw(0, "TransformAt(%d) bad index. Expression has %d elements", index, original->NumberOfElements());
		}

		auto* builderAtIndex = ss.CreateMacroTransform(original->GetElement(index));

		CReflectedClass* childRep = ss.CreateReflectionClass("ExpressionBuilder", (void*)builderAtIndex);
		registers[7].vPtrValue = &childRep->header.pVTables[0];
	}

	VM_CALLBACK(TransformParent_D4retD7)
	{
		// Evaluate return value of (out.TransformParent). <out> is given in D4, the CScript for the macro expansion is given in D5. Value is returned in D7

		IScriptSystem& ss = *(IScriptSystem*)context;
		auto* i = (InterfacePointer) registers[4].vPtrValue;
		auto* builder = reinterpret_cast<CClassExpression*>(InterfaceToInstance(i));
		if (IsNullType(*builder->Header.Desc->TypeInfo))
		{
			Throw(0, "the instance was a null expression builder object.");
		}

		if (!builder->ExpressionPtr)
		{
			Throw(0, "ExpressionBuilder's internal expression pointer was null");
		}

		cr_sex s = *builder->ExpressionPtr;

		auto* original = s.GetOriginal();

		if (!original)
		{
			original = &s;
		}

		auto* parentBuilder = ss.CreateMacroTransformClonedFromParent(*original);

		CReflectedClass* childRep = ss.CreateReflectionClass("ExpressionBuilder", (void*) parentBuilder);
		registers[7].vPtrValue = &childRep->header.pVTables[0];
	}

#define VALIDATE_ARG
#define ALLOW_REWRITE_VTABLES // This should increase vcall speed by about 2.5x, or from 40M to 100M virtual calls per second on an i7-11700 @ 2.50GHz

	VM_CALLBACK(JumpFromProxyToMethod)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		const uint8* pc = registers[REGISTER_PC].uint8PtrValue;

		// This function should have been called via an Invoke, which has moved the PC to just beyond the invoke where we expect...
		// Opcodes::SetRegisterImmediate64 + D4 + functionId ;

#ifdef VALIDATE_ARG
		if (*pc++ != Opcodes::SetRegisterImmediate64)
		{
			Rococo::Throw(0, "JumpFromProxyToMethod: Expecting [SetRegisterImmediate64] D4 <byte_code_id>");
		}

		if (*pc++ != 4)
		{
			Rococo::Throw(0, "JumpFromProxyToMethod: Expecting SetRegisterImmediate64 [D4] <byte_code_id>");
		}
#else
		pc += 2;
#endif

		const ID_BYTECODE* pId = (const ID_BYTECODE*) pc;

		pc += 8;

#ifdef VALIDATE_ARG
		if (*pc++ != Opcodes::SetRegisterImmediate64)
		{
			Rococo::Throw(0, "JumpFromProxyToMethod: Expecting [SetRegisterImmediate64] D4 <byte_code_id>");
		}

		if (*pc++ != 4)
		{
			Rococo::Throw(0, "JumpFromProxyToMethod: Expecting SetRegisterImmediate64 [D4] <byte_code_id>");
		}
#else
		pc += 2;
#endif

		IFunctionBuilder* f = *(IFunctionBuilder**)pc;

		auto& mem = ss.ProgramObject().ProgramMemory();

		bool isImmutable;
		size_t functionAddressOffset = mem.GetFunctionAddress(*pId, OUT isImmutable);
		auto* functionAddress = ss.ProgramObject().VirtualMachine().Cpu().ProgramStart + functionAddressOffset;

#ifdef ALLOW_REWRITE_VTABLES
		if (isImmutable)
		{
			auto* sp = registers[REGISTER_SP].uint8PtrValue;
			InterfacePointer ip = *(InterfacePointer*)(sp - 24);
			auto* methods = &ip[0]->FirstMethodId;
			for (int i = 0; i < 10; i++)
			{
				enum { pc_to_proxy_call_delta = -17 }; // This takes us back to the proxy function address
				if (methods[i] == (ID_BYTECODE)( pc - pc_to_proxy_call_delta))
				{
					methods[i] = (ID_BYTECODE)functionAddress;
					break;
				}
			}
		}
#endif

		registers[REGISTER_PC].uint8PtrValue = functionAddress;
	}

	VM_CALLBACK(ThrowNullRef)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		auto* i = (const IInterface*) registers[4].vPtrValue;
		auto *f = (const IFunction*) registers[5].vPtrValue;

		NamespaceSplitter splitter(f->Name());
		cstr body, tail;
		splitter.SplitTail(body, tail);

		ss.ThrowFromNativeCodeF(0, "%s.%s: null reference", i->Name(), tail);
	}

	VM_CALLBACK(IsDifferentObject)
	{
		auto& sp = registers[VM::REGISTER_SP].uint8PtrValue;
		sp -= sizeof(InterfacePointer);
		auto* rightArg = (InterfacePointer*)sp;

		sp -= sizeof(InterfacePointer);
		auto* leftArg = (InterfacePointer*)sp;

		auto* rightObj = InterfaceToInstance(*rightArg);
		auto* leftObj = InterfaceToInstance(*leftArg);

		registers[VM::REGISTER_D7].int64Value = (leftObj != rightObj) ? 1 : 0;
	}

	VM_CALLBACK(IsSameObject)
	{
		auto& sp = registers[VM::REGISTER_SP].uint8PtrValue;
		sp -= sizeof(InterfacePointer);
		auto* rightArg = (InterfacePointer*)sp;

		sp -= sizeof(InterfacePointer);
		auto* leftArg = (InterfacePointer*)sp;

		auto* rightObj = InterfaceToInstance(*rightArg);
		auto* leftObj = InterfaceToInstance(*leftArg);

		registers[VM::REGISTER_D7].int64Value = (leftObj == rightObj) ? 1 : 0;
	}

	VM_CALLBACK(GetTypeOfClassToD4)
	{
		auto& sp = registers[VM::REGISTER_SP].uint8PtrValue;
		sp -= sizeof(InterfacePointer);

		InterfacePointer ipClassRef = *(InterfacePointer*)sp;
		auto* classObject = InterfaceToInstance(ipClassRef);
		auto* classObjectType = classObject->Desc->TypeInfo;

		auto& ss = *(IScriptSystem*)context;

		CReflectedClass* pStruct = ss.GetReflectedClass(classObjectType);
		if (pStruct == NULL)
		{
			pStruct = ss.CreateReflectionClass("Structure", classObjectType);
		}

		registers[REGISTER_D4].vPtrValue = pStruct->header.AddressOfVTable0();
	}

	VM_CALLBACK(InvokeMethodByName)
	{
		struct InvokeArgs
		{
			Vec2i* pArg;
			InterfacePointer pInterface;
			const IStructure* argType;
			InterfacePointer methodName;
		};

		auto& sp = registers[VM::REGISTER_SP].uint8PtrValue;
		sp -= sizeof(InvokeArgs);
		auto* invokeArgs = (InvokeArgs*)sp;

		IScriptSystem& ss = *(IScriptSystem*)context;

		CStringConstant* methodObject = (CStringConstant*) InterfaceToInstance(invokeArgs->methodName);
				
		ObjectStub* targetObject = InterfaceToInstance(invokeArgs->pInterface);

		MethodInfo m = ss.GetMethodByName(methodObject->pointer, *targetObject->Desc->TypeInfo);
		if (m.f == nullptr)
		{
			// The callee will pop two pointers off the stack
			sp += 2 * sizeof(size_t);
			// For now we fail silently. The consumer can check to see if methods exist with the reflection API if needs be.
			// For most cases we use the API for event handling, and if the handler does not handle the event it is meant to be ignored
			// Throw(0, "InvokeMethodByName failed. No method %s on object %s", methodObject->pointer, GetFriendlyName(*targetObject->Desc->TypeInfo));
			return;
		}

		if (m.f->NumberOfInputs() != 2)
		{
			// 1 implicit and != 1 explicit
			Throw(0, "InvokeMethodByName failed. Method %s on object %s incompatible with invocation. Only methods of 1 explicit value are legal candidates", methodObject->pointer, GetFriendlyName(*targetObject->Desc->TypeInfo));
		}

		if (m.f->NumberOfOutputs() != 0)
		{
			Throw(0, "InvokeMethodByName failed. Method %s on object %s incompatible with invocation. Only methods without output are legal candidates.", methodObject->pointer, GetFriendlyName(*targetObject->Desc->TypeInfo));
		}

		auto& functionsArgType = m.f->GetArgument(0);
		if (&functionsArgType != invokeArgs->argType)
		{
			Throw(0, "InvokeMethodByName failed. Method %s on object %s incompatible with invocation. Type mismatch: arg0 type is %s. Type supplied to invocation is %s.",
				methodObject->pointer, GetFriendlyName(*targetObject->Desc->TypeInfo), GetFriendlyName(*invokeArgs->argType), GetFriendlyName(functionsArgType));
		}

		sp += 2 * sizeof(size_t); // This puts the interface and arg back on the stack, ready for the virtual call

		ptrdiff_t delta = (*invokeArgs->pInterface)->OffsetToInstance + m.offset;

		invokeArgs->pInterface = (InterfacePointer)(((uint8*)invokeArgs->pInterface) + delta);

		CodeSection code;
		m.f->Code().GetCodeSection(code);

		auto& po = ss.ProgramObject();
		size_t functionStart = po.ProgramMemory().GetFunctionAddress(code.Id);

		auto& cpu = po.VirtualMachine().Cpu();

		cpu.Push(registers[REGISTER_SF].vPtrValue);

		const uint8* returnAddress = cpu.PC();
		cpu.Push(returnAddress);

		registers[REGISTER_SF].charPtrValue = registers[REGISTER_SP].charPtrValue; // Create a new stack frame

		cpu.SetPC(cpu.ProgramStart + functionStart);
	}

	VM_CALLBACK(DynamicDispatch)
	{
		struct DDArgs
		{
			void* pArg;
			InterfacePointer pInterface;
			const ISExpression* sourceCode;
			cstr methodName;
			const IStructure* pArgType;
		};
		
		auto& sp = registers[VM::REGISTER_SP].uint8PtrValue;
		sp -= sizeof(DDArgs);
		auto* ddArgs = (DDArgs*)sp;

		ObjectStub* stub = InterfaceToInstance(ddArgs->pInterface);
		auto& type = *stub->Desc->TypeInfo;

		IScriptSystem& ss = *(IScriptSystem*)context;
		MethodInfo m = ss.GetMethodByName(ddArgs->methodName, type);
		auto* f = m.f;

		sp += 2 * sizeof(size_t); // This leaves pArg and instance on the stack, just ripe for a hand rolled virtual call

		if (f != nullptr)
		{
			// 1 arg input and 1 instance = 2 inputs
			if (f->NumberOfInputs() != 2 || f->NumberOfOutputs() != 0)
			{
				Throw(*ddArgs->sourceCode, "Method must have 1 input and no output.\nDispatch mismatch in %s.%s of %s", type.Name(), ddArgs->methodName, type.Module().Name());
			}

			auto& arg = f->GetArgument(0);
			if (&arg != ddArgs->pArgType)
			{
				Throw(*ddArgs->sourceCode, "Method must have 1 input and no output.\nDispatch mismatch in %s.%s of %s", type.Name(), ddArgs->methodName, type.Module().Name());
			}

			ptrdiff_t delta = (*ddArgs->pInterface)->OffsetToInstance + m.offset;

			ddArgs->pInterface = (InterfacePointer)(((uint8*)ddArgs->pInterface) + delta);

			CodeSection code;
			f->Code().GetCodeSection(code);

			auto& po = ss.ProgramObject();
			size_t functionStart = po.ProgramMemory().GetFunctionAddress(code.Id);

			auto& cpu = po.VirtualMachine().Cpu();

			cpu.Push(registers[REGISTER_SF].vPtrValue);

			const uint8 *returnAddress = cpu.PC();
			cpu.Push(returnAddress);

			registers[REGISTER_SF].charPtrValue = registers[REGISTER_SP].charPtrValue; // Create a new stack frame

			cpu.SetPC(cpu.ProgramStart + functionStart);
		}
		else
		{
			// Throw(*ddArgs->sourceCode, "Dispatch failed - Cannot find method %s in %s of %s", ddArgs->methodName, type.Name(), type.Module().Name());
		}
	}

	VM_CALLBACK(YieldMicroseconds)
	{
		IVirtualMachine& vm = *(IVirtualMachine*)context;
		auto waitTime = registers[4].int64Value; // intially the wait period in microseconds, then becomes end time for timer
		auto startCount = registers[5].int64Value;
		auto* pcResetPoint = registers[7].uint8PtrValue;

		if (startCount == 0)
		{
			registers[5].int64Value = startCount = Rococo::Time::TickCount();

			int64 hz = Rococo::Time::TickHz();

			int64 cyclesPerMillisecond = hz >> 10;
			int64 cyclesWait = (waitTime * cyclesPerMillisecond) >> 10;

			registers[4].int64Value = waitTime = (cyclesWait + startCount);
		}
		else
		{
			auto now = Rococo::Time::TickCount();
			if (now >= waitTime)
			{
				vm.NotifyWaitEvent(0);
				return;
			}
		}

		vm.NotifyWaitEvent(waitTime);

		registers[VM::REGISTER_PC].uint8PtrValue = pcResetPoint;
		vm.YieldExecution();
	}

	void CompileNullMethod_NullsOutput(IFunctionBuilder& f)
	{
		// Set all outputs to zero
		for (int i = 0; i < f.NumberOfOutputs(); i++)
		{
			const IArgument& arg = f.Arg(i);
			if (!IsNullType(*arg.ResolvedType()))
			{
				f.Builder().AssignLiteral(NameString::From(arg.Name()), ("0"));
			}
		}

		CompileSetOutputToNull(f);
	}

	void CompileNullMethod(IScriptSystem& ss, const IArchetype& nullMethod, IInterface& interf, IStructureBuilder& nullObject, cr_sex source, INamespace& ns)
	{
		TokenBuffer qualifiedMethodName;
		StringPrint(qualifiedMethodName, ("%s.%s"), nullObject.Name(), nullMethod.Name());

		FunctionPrototype fp(qualifiedMethodName, true);
		IFunctionBuilder& f = Rococo::Script::DeclareFunction(nullObject.Module(), source, fp);

		for (int i = 0; i < nullMethod.NumberOfOutputs(); ++i)
		{
			const IStructure& argStruct = nullMethod.GetArgument(i);
			cstr argType = GetFriendlyName(argStruct);
			cstr argName = nullMethod.GetArgName(i);
			f.AddOutput(NameString::From(argName), argStruct, (void*)&source);
		}

		TokenBuffer qualifiedInterfaceName;
		StringPrint(qualifiedInterfaceName, ("%s.%s"), ns.FullName()->Buffer, interf.Name());
		f.AddInput(NameString::From(THIS_POINTER_TOKEN), TypeString::From(qualifiedInterfaceName), (void*)&source);

		for (int i = 0; i < nullMethod.NumberOfInputs() - 1; ++i)
		{
			int index = nullMethod.NumberOfOutputs() + i;
			const IStructure& argStruct = nullMethod.GetArgument(index);
			f.AddInput(NameString::From(nullMethod.GetArgName(index)), argStruct, (void*)&source);
		}

		if (!f.TryResolveArguments())
		{
			Throw(source, "Error resolving arguments in null method: %s", qualifiedMethodName);
		}

		f.Builder().SetThisOffset(ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0);

		f.Builder().Begin();

		const void* value;
		if (interf.Attributes().FindAttribute("essential", value))
		{
			CompileNullMethod_Throws(ss, interf, f);
		}
		else
		{
			CompileNullMethod_NullsOutput(f);
		}

		f.Builder().End();
		f.Builder().Assembler().Clear();
	}

	void CompileNullObject(IScriptSystem& ss, IInterface& interf, IStructureBuilder& nullObject, cr_sex source, INamespace& ns)
	{
		for(int i = 0; i < interf.MethodCount(); ++i)
		{
			const IArchetype& nullMethod = interf.GetMethod(i);
			CompileNullMethod(ss, nullMethod, interf, nullObject, source, ns);			
		}

		nullObject.FillVirtualTables();
	}

	void RegisterContext(CScript& script, cstr name);
	cstr emptyString = ("");

	class CCompileContext
	{
	private:
		cstr name;
		CScript& script;

	public:
		CCompileContext(CScript& _script, cstr _name): script(_script), name(_name) { RegisterContext(script,name); }
		~CCompileContext() { RegisterContext(script,emptyString); }	
	};

	CStringConstant* CreateStringConstant(CScript& script, int length, cstr pointer, const ISExpression* srcExpression)
	{
		return script.CreateStringConstant(length, pointer, srcExpression);
	}

	void RegisterContext(CScript& script, cstr name) { script.RegisterContext(name); }
	cstr GetContext(const CScript& script) { return script.GetContext(); }

	IModuleBuilder& GetModule(CScript& script) { return script.ProgramModule(); }

	void CompileNextClosures(CScript& script) { script.CompileNextClosures(); }

	const ISExpression* GetTryCatchExpression(CScript& script) { return script.GetTryCatchExpression(); }

	IStructure* GetLocalStructure(CScript& script, cstr className)
	{
		return script.ProgramModule().FindStructure(className);
	}

	class CScripts
	{
		friend CScript;
	private:
		typedef TSexyHashMap<ISParserTree*,CScript*>  TMapTreeToScript;
		TMapTreeToScript scriptMap;

		typedef TSexyVector<std::pair<ISParserTree*,CScript*>> TScriptVector;
		TScriptVector scripts;

		IProgramObject& programObject;
		TNamespaceDefinitions namespaceDefinitions;
		TStructureDefinitions structDefinitions;
		CDefaultExceptionLogic exceptionLogic;
		IScriptSystem& system;
		int globalBaseIndex;
		TSexyHashMap<const IArchetype*, IFunctionBuilder*> nullArchetypeFunctions;

		bool canInlineString;	

		enum { COMPILE_ALL_MODULES = 0 };
		
		struct PartialCompilationLimits
		{		
			size_t startingIndex = COMPILE_ALL_MODULES;
			size_t maxModuleCount = COMPILE_ALL_MODULES;
		} limits;
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

		void EnterCompileLimits(size_t startingIndex, size_t maxModuleCount)
		{
			limits.startingIndex = startingIndex;
			limits.maxModuleCount = maxModuleCount;
		}

		void ReleaseCompileLimits()
		{
			limits.startingIndex = COMPILE_ALL_MODULES;
			limits.maxModuleCount = COMPILE_ALL_MODULES;
		}

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS

		void Clear()
		{
			addedSpecialStructures = false;
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
					virtual void operator()(cstr name, const GlobalValue& variable)
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
			for (auto i = scripts.begin(); i != scripts.end(); ++i)
			{
				CScript* module = i->second;
				if (!module->HasCompiled())
				{
					module->AppendCompiledNamespaces(namespaceDefinitions);
				}
			}

			// Sort the namespace defs by name length ascending. Note Length('A.B.C.D') > Length('A.B.C') > Length('A.B') > Length('A')
			// Sorting gives 'A', 'A.B', 'A.B.C', 'A.B.C.D', which is the same order we need to construct 'A.B.C.D' from its ancestors
			std::sort(namespaceDefinitions.begin(), namespaceDefinitions.end());

			for (auto j = namespaceDefinitions.begin(); j != namespaceDefinitions.end(); ++j)
			{
				cstr nsSymbol = j->E->c_str();
				INamespace* ns = programObject.GetRootNamespace().FindSubspace(nsSymbol);
				if (ns != NULL)
				{
					// Don't whinge. It is too annoying to stipulate namespaces must only be defined once
				}
				else
				{
					try
					{
						ns = j->NS = &programObject.GetRootNamespace().AddNamespace(nsSymbol, ADDNAMESPACEFLAGS_NORMAL);
					}
					catch (IException& e)
					{
						Rococo::Sex::Throw(*(j->E), "%s", e.Message());
					}
					catch (std::exception& e)
					{
						Rococo::Sex::Throw(*(j->E), "std::exception thrown: %s", e.what());
					}
				}

				if (j->isDefault)
				{
					try
					{
						j->Module->ProgramModule().SetDefaultNamespace(ns);
					}
					catch (IException& ex)
					{
						Throw(*j->E, "%s", ex.Message());
					}
				}
			}
		}

		template<class T> void ForEachUncompiledScript(T& t)
		{
			size_t startIndex = limits.startingIndex;
			size_t endIndex = limits.maxModuleCount == COMPILE_ALL_MODULES ? scripts.size() : limits.maxModuleCount;

			for(auto i = startIndex; i != endIndex; ++i)
			{
				auto& scriptBinding = scripts[i];

				CScript* script = scriptBinding.second;
				cstr name = script->ProgramModule().Name();	

				cr_sex root = script->Tree().Root();
				
				try
				{
					if (!script->HasCompiled())
					{
						t.Process(*script, name);
					}
				}
				catch(STCException& ex)
				{
					Rococo::Sex::Throw(root, "%s: %s", ex.Source(), ex.Message());
				}			
				catch (std::exception& e)
				{
					Rococo::Sex::Throw(root, "std::exception thrown: %s", e.what());
				}	
			}
		}

		void CompileNamespaces()
		{
			struct FnctorCompileNamespaces
			{
				void Process(CScript& script, cstr name)
				{
					script.Clear();
					script.ValidateTopLevel();
				}
			} fnctorCompileNamespaces;

			ForEachUncompiledScript(fnctorCompileNamespaces);
			ResolveNamespaces();

			struct FnctorCompileUsingNamespaces
			{
				void Process(CScript& script, cstr name)
				{
					script.ComputePrefixes();
				}
			} fnctorCompileUsingNamespaces;

			ForEachUncompiledScript(fnctorCompileUsingNamespaces);
		}

		void CompileTopLevelMacros()
		{
			struct FnctorCompileMacros
			{
				void Process(CScript& script, cstr name)
				{
					script.CompileTopLevelMacros();
				}
			} fnctorCompileTopLevelMacros;

			ForEachUncompiledScript(fnctorCompileTopLevelMacros);
		}

		void AddSpecialStructures()
		{
			IModuleBuilder& module = programObject.GetModule(0);
			auto ns =  Compiler::MatchNamespace(module, ("Sys.Native"));

			{
				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure("_Array", prototype, NULL);

				s.AddMember(NameString::From("_start"), TypeString::From("Pointer"));
				s.AddMember(NameString::From("_length"), TypeString::From("Int32"));
				s.AddMember(NameString::From("_elementCapacity"), TypeString::From("Int32"));
				s.AddMember(NameString::From("_elementType"), TypeString::From("Pointer"));
				s.AddMember(NameString::From("_elementSize"), TypeString::From("Int32"));
				s.AddMember(NameString::From("_lock"), TypeString::From("Int32"));
				s.AddMember(NameString::From("_refCount"), TypeString::From("Int64"));

				ns->Alias("_Array", s);
			}
			{
				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure("_List", prototype, NULL);
				s.AddMember(NameString::From(("_ListImage")), TypeString::From(("Pointer")));
				ns->Alias(("_List"), s);
			}
			{
				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure(("_Node"), prototype, NULL);

				s.AddMember(NameString::From(("_list")), TypeString::From(("Pointer")));
				s.AddMember(NameString::From(("_elementType")), TypeString::From(("Pointer")));
				s.AddMember(NameString::From(("_previous")), TypeString::From(("Pointer")));
				s.AddMember(NameString::From(("_next")), TypeString::From(("Pointer")));
				s.AddMember(NameString::From(("_refCount")), TypeString::From(("Int32")));
							
				ns->Alias(("_Node"), s);
			}
			{
				cstr lockName = ("_Lock");

				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure(lockName, prototype, NULL);

				s.AddMember(NameString::From(("_lockSource")), TypeString::From(("Pointer")));
				s.AddMember(NameString::From(("_lockMemberOffset")), TypeString::From(("Int32")));

				ns->Alias(lockName, s);
			}
			{
				cstr mapName = ("_Map");

				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure(mapName, prototype, NULL);

				s.AddMember(NameString::From(("_mapImage")), TypeString::From(("Pointer")));

				ns->Alias(mapName, s);
			}
			{
				cstr nodeName = ("_MapNode");

				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);
				const ISExpression* src = NULL;

				IStructureBuilder& s = module.DeclareStructure(nodeName, prototype, NULL);

				s.AddMember(NameString::From(("_container")), TypeString::From(("Pointer")));		
				s.AddMember(NameString::From(("_previous")), TypeString::From(("Pointer")));		
				s.AddMember(NameString::From(("_next")), TypeString::From(("Pointer")));		
				s.AddMember(NameString::From(("_exists")), TypeString::From(("Bool")));	
				s.AddMember(NameString::From(("_refCount")), TypeString::From(("Int32")));	
				s.AddMember(NameString::From(("_hashcode")), TypeString::From(("Int32")));	

				ns->Alias(nodeName, s);
			}
		}

		bool addedSpecialStructures = false;

		void CompileDeclarations()
		{
			exceptionLogic.Clear();

			struct FnctorComputeArchetypeNames
			{
				void Process(CScript& script, cstr name)
				{
					script.ComputeArchetypeNames();
					script.ComputeInterfacePrototypes();
				}
			} fnctorComputeArchetypeNames;
			ForEachUncompiledScript(fnctorComputeArchetypeNames);


			struct FnctorComputeStructNames
			{
				void Process(CScript& script, cstr name)
				{
					// script.ComputePrefixes(); // TODO - delete this comment if everything is working
					script.ComputeStructureNames();	
				}
			} fnctorComputeStructNames;
			ForEachUncompiledScript(fnctorComputeStructNames);

			if (!addedSpecialStructures)
			{
				AddSpecialStructures();
				addedSpecialStructures = true;
			}

			struct FnctorComputeFunctionNames
			{
				void Process(CScript& script, cstr name)
				{
					script.ComputeFunctionNames();	
				}
			} fnctorComputeFunctionNames;
			ForEachUncompiledScript(fnctorComputeFunctionNames);

			struct FnctorAppendAliases
			{
				void Process(CScript& script, cstr name)
				{
					AppendAliases(script.System(), script.ProgramModule(), script.Tree());
				}
			} fnctorAppendAliases;
			ForEachUncompiledScript(fnctorAppendAliases);

			struct FnctorStructureFields
			{
				void Process(CScript& script, cstr name)
				{
					script.ComputeStructureFields();
				}
			} fnctorStructureFields;
			ForEachUncompiledScript(fnctorStructureFields);

			struct FnctorFunctionArgs
			{
				void Process(CScript& script, cstr name)
				{
					script.ComputeFunctionArgs();
				}
			} fnctorFunctionArgs;
			ForEachUncompiledScript(fnctorFunctionArgs);

			struct FnctorComputeArchetypes
			{
				void Process(CScript& script, cstr name)
				{
					script.ComputeArchetypes();
					script.ComputeInterfaces();	
				}
			} fnctorComputeArchetypes;
			ForEachUncompiledScript(fnctorComputeArchetypes);

			const void* pSrcError = nullptr;
			if (!programObject.ResolveDefinitions(&pSrcError))
			{
				if (pSrcError)
				{
					auto& s = *reinterpret_cast<const ISExpression*>(pSrcError);
					Throw(s, "Error resolving type. Check the log to identify for members causing problems.");
				}

				Vec2i start, end;
				start.x = end.x = 0;
				start.y = end.y = 0;

				ParseException ex(start, end, "Sexy Script System", "Failed to resolve definitions", "", NULL);
				OS::TripDebugger();
				throw ex;
			}

			const IStructure* mapNode = programObject.GetModule(0).FindStructure(("_Map"));
			
			programObject.InitCommon();

			struct FnctorValidateConcreteClasses
			{
				void Process(CScript& script, cstr name)
				{
					script.ValidateConcreteClasses();
				}
			} fnctorValidateConcreteClasses;
			ForEachUncompiledScript(fnctorValidateConcreteClasses);

			struct FnctorValidateConstructors
			{
				void Process(CScript& script, cstr name)
				{
					script.ValidateConstructors();
				}
			} fnctorValidateConstructors;
			ForEachUncompiledScript(fnctorValidateConstructors);

			struct FnctorDeclareMacros
			{
				void Process(CScript& script, cstr name)
				{
					script.DeclareMacros();
				}
			} fnctorDeclareMacros;
			ForEachUncompiledScript(fnctorDeclareMacros);

			struct FnctorComputeGlobals
			{
				int globalBaseIndex;
				void Process(CScript& script, cstr name)
				{
					script.ComputeGlobals(REF globalBaseIndex);
				}
			} fnctorComputeGlobals;
			fnctorComputeGlobals.globalBaseIndex = 0;
			ForEachUncompiledScript(fnctorComputeGlobals);

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
							char buf[256];
							SafeFormat(buf, "%s", "Cannot inline IString, as %s / %s implements multiple interfaces.", mod.Name(), s.Name());
							programObject.Log().Write(buf);
							return false;
						}

						if (s.MemberCount() < 5)
						{
							char buf[256];
							SafeFormat(buf, "%s", "Cannot inline IString, as %s / %s does not implement both the buffer and length members.", mod.Name(), s.Name());
							programObject.Log().Write(buf);
							return false;
						}

						const IMember& bufferMember = s.GetMember(4);
						if (bufferMember.UnderlyingType()->VarType() != VARTYPE_Pointer || !AreEqual(("buffer"), bufferMember.Name()))
						{
							char buf[256];
							StackStringBuilder ssb(buf, sizeof buf);
							ssb << ("Cannot inline IString, as ") << mod.Name() << ("/") << s.Name() << (" 5th member is not (Pointer Buffer).");
							programObject.Log().Write(buf);
							return false;
						}

						const IMember& lenMember = s.GetMember(3);
						if (lenMember.UnderlyingType()->VarType() != VARTYPE_Int32 || !AreEqual(("length"), lenMember.Name()))
						{
							char buf[256];
							StackStringBuilder ssb(buf, sizeof buf);
							ssb << ("Cannot inline IString, as ") << mod.Name() << ("/") << s.Name() << (" 4th member is not (Int32 length).");
							programObject.Log().Write(buf);
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
				void Process(CScript& script, cstr name)
				{
					script.CompileNullObjects();
				}
			} fnctorCompileNullObjects;
			ForEachUncompiledScript(fnctorCompileNullObjects);

			canInlineString = TryInlineIString();			
					
			struct FnctorJIT
			{
				void Process(CScript& script, cstr name)
				{
					script.CompileJITStubs();
					script.CompileFactoryJITStubs();		
					script.CompileMacroJITStubs();
				}
			} fnctorJIT;
			ForEachUncompiledScript(fnctorJIT);

			struct FnctorComplete
			{
				void Process(CScript& script, cstr name)
				{
					script.CompileVTables();
					script.MarkCompiled();
				}
			} fnctorComplete;
			ForEachUncompiledScript(fnctorComplete);
			/*
			for(auto i = scripts.begin(); i != scripts.end(); ++i)
			{
				CScript* script = i->second;
				cstr name = script->ProgramModule().Name();	
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
		auto a = scripts.nullArchetypeFunctions.find(&archetype);
		if (a != scripts.nullArchetypeFunctions.end())
		{
			return *a->second;
		}

		TokenBuffer nullFunctionName;
		StringPrint(nullFunctionName, ("_null%s"), archetype.Name());

		cr_sex source = *(const Sex::ISExpression*) archetype.Definition();

      FunctionPrototype fp(nullFunctionName, true);
      IFunctionBuilder& f = Rococo::Script::DeclareFunction(module, source, fp);

		scripts.nullArchetypeFunctions.insert(std::make_pair(&archetype, &f));

		for (int i = 0; i < archetype.NumberOfOutputs(); ++i)
		{
			const IStructure& argStruct = archetype.GetArgument(i);
			cstr argType = GetFriendlyName(argStruct);
			cstr argName = archetype.GetArgName(i);
			f.AddOutput(NameString::From(argName), argStruct, (void*) archetype.Definition());
		}

		for (int i = 0; i < archetype.NumberOfInputs(); ++i)
		{
			int index = archetype.NumberOfOutputs() + i;
			const IStructure& argStruct = archetype.GetArgument(index);
			f.AddInput(NameString::From(archetype.GetArgName(index)), argStruct, (void*) archetype.Definition());
		}

		if (!f.TryResolveArguments())
		{
			Throw(source, "Error resolving arguments in: %s", nullFunctionName);
		}

		f.Builder().Begin();

		// Set all outputs to zero
		for (int i = 0; i < f.NumberOfOutputs(); i++)
		{
			const IArgument& arg = f.Arg(i);
			f.Builder().AssignLiteral(NameString::From(arg.Name()), ("0"));
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

   CScript::CScript(ISParserTree& _tree, IProgramObject& _programObject, CScripts& _scripts) :
      tree(_tree),
      programObject(_programObject),
      isDirty(true),
      scripts(_scripts),
      module(_programObject.AddModule(tree.Source().Name())),
      contextName(emptyString)
   {
      tree.AddRef();
   }

   DEFINE_SEXY_ALLOCATORS_OUTSIDE_OF_CLASS(CScript);

   GlobalValue* CScript::GetGlobalValue(cstr name)
   {
      auto i = globalVariables.find(name);
      return i == globalVariables.end() ? nullptr : &i->second;
   }

   void CScript::EnumerateGlobals(IGlobalEnumerator& cb)
   {
      for (auto i : globalVariables)
      {
         cb(i.first, i.second);
      }
   }

   const ArrayDef* CScript::GetElementTypeForArrayVariable(ICodeBuilder& builder, cstr arrayName)
   {
	   BuilderAndNameKey key;
	   key.Builder = &builder;
	   key.Name = arrayName;

	   TMapNameToArrayDef::const_iterator i = mapNameToArrayDef.find(key);
	   if (i != mapNameToArrayDef.end())
	   {
		   return &i->second;
	   }

	   auto* parent = builder.Owner().Parent();
	   if (parent != nullptr)
	   {
		   return CScript::GetElementTypeForArrayVariable(parent->Builder(), arrayName);
	   }

	   return nullptr;

   }

   const ListDef* CScript::GetListDef(ICodeBuilder& builder, cstr listName)
   {
      BuilderAndNameKey key;
      key.Builder = &builder;
      key.Name = listName;

      TMapNameToListDef::const_iterator i = mapNameToListDef.find(key);
      return i == mapNameToListDef.end() ? NULL : &i->second;
   }

   const NodeDef* CScript::GetNodeDef(ICodeBuilder& builder, cstr listName)
   {
      BuilderAndNameKey key;
      key.Builder = &builder;
      key.Name = listName;

      TMapNameToNodeDef::const_iterator i = mapNameToNodeDef.find(key);
      return i == mapNameToNodeDef.end() ? NULL : &i->second;
   }

   const MapDef* CScript::GetMapDef(ICodeBuilder& builder, cstr listName)
   {
      BuilderAndNameKey key;
      key.Builder = &builder;
      key.Name = listName;

      TMapNameToMapDef::const_iterator i = mapNameToMapDef.find(key);
      return i == mapNameToMapDef.end() ? NULL : &i->second;
   }

   const MapNodeDef* CScript::GetMapNodeDef(ICodeBuilder& builder, cstr name)
   {
      BuilderAndNameKey key;
      key.Builder = &builder;
      key.Name = name;

      TMapNameToMapNodeDef::const_iterator i = mapNameToMapNodeDef.find(key);
      return i == mapNameToMapNodeDef.end() ? NULL : &i->second;
   }

   void CScript::AddArrayDef(ICodeBuilder& builder, cstr arrayName, const IStructure& elementType, cr_sex s)
   {
      BuilderAndNameKey key;
      key.Builder = &builder;
      key.Name = arrayName;

      ArrayDef def(s, elementType);
      mapNameToArrayDef.insert(std::make_pair(key, def));
   }

   void CScript::AddListDef(ICodeBuilder& builder, cstr name, const IStructure& elementType, cr_sex s)
   {
      BuilderAndNameKey key;
      key.Builder = &builder;
      key.Name = name;

      ListDef def(s, elementType);
      mapNameToListDef.insert(std::make_pair(key, def));
   }

   void CScript::AddMapDef(ICodeBuilder& builder, cstr name, const IStructure& keyType, const IStructure& valueType, cr_sex s)
   {
      BuilderAndNameKey key;
      key.Builder = &builder;
      key.Name = name;

      MapDef def(s, keyType, valueType);
      mapNameToMapDef.insert(std::make_pair(key, def));
   }

   void CScript::AddMapNodeDef(ICodeBuilder& builder, const MapDef& mapDef, cstr mapName, cstr nodeName, cr_sex s)
   {
      BuilderAndNameKey key;
      key.Builder = &builder;
      key.Name = nodeName;

      MapNodeDef def(s, mapDef, mapName);
      mapNameToMapNodeDef.insert(std::make_pair(key, def));
   }

   const bool CScript::IsIStringInlined() const
   { 
      return Rococo::Script::IsIStringInlined(scripts);
   }

   IScriptSystem&  CScript::System()
   {
      return GetSystem(scripts);
   }

   void CScript::AddNodeDef(ICodeBuilder& builder, cstr nodeName, const IStructure& elementType, cr_sex s)
   {
      BuilderAndNameKey key;
      key.Builder = &builder;
      key.Name = nodeName;

      NodeDef def(s, elementType);
      mapNameToNodeDef.insert(std::make_pair(key, def));
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

   CStringConstant* CScript::CreateStringConstant(int length, cstr pointer, const ISExpression* srcExpression)
   {
      const IStructure& scStruct = Object().Common().TypeStringLiteral();

      CStringConstant* sc = new CStringConstant;
      sc->header.refCount = ObjectStub::NO_REF_COUNT;
      sc->header.pVTables[0] = (VirtualTable*) scStruct.GetVirtualTable(1);
      sc->header.Desc = (ObjectDesc*)scStruct.GetVirtualTable(0);
      sc->length = length;
      sc->pointer = pointer;
      sc->srcExpression = (void*)srcExpression;

      stringConstants.push_back(sc);
      return sc;
   }

   CScript::~CScript()
   {
      tree.Release();

      for (auto i = stringConstants.begin(); i != stringConstants.end(); ++i)
      {
         CStringConstant* sc = *i;
         delete sc;
      }

      stringConstants.clear();
   }

   void CScript::Clear()
   {
      macros.clear();
      module.Clear();
      closureJobs.clear();
      stringConstants.clear();
      globalVariables.clear();
   }

   void CScript::CompileNullObjects()
   {
	   for (auto n = nullDefs.begin(); n != nullDefs.end(); ++n)
	   {
		   CompileNullObject(System(), *n->Interface, *n->NullObject, *n->Source, *n->NS);
		   n->Interface->PostCompile();
	   }
   }

   const Rococo::Compiler::IMacro* FindMacro(CScript& script, cstr macroNameExcludingHash, cr_sex s)
   {
	   NamespaceSplitter splitter(macroNameExcludingHash);

	   auto& ss = script.System();
	   auto& rootNs = ss.PublicProgramObject().GetRootNamespace();

	   cstr ns, shortMacroName;
	   if (!splitter.SplitTail(ns, shortMacroName))
	   {
		   auto& mod = script.ProgramModule();
		   for (int i = 0; i < mod.PrefixCount(); ++i)
		   {
			   auto& prefix = mod.GetPrefix(i);
			   auto* macro = prefix.FindMacro(macroNameExcludingHash);
			   if (macro)
			   {
				   return macro;
			   }
		   }

		   cstr msg = 
				"Could not find macro amongst all the namespaces specified with 'using' directives in the module.\n"
				"Macros must be compiled before the source files that invoke them.\n"
				"Ensure that your build system used partial compilation as each source module was appended.\n";
		   Throw(s[0], msg);
	   }

	  // auto& ss = script.System();
	  // auto& rootNs = ss.PublicProgramObject().GetRootNamespace();

	   auto* macroNS = rootNs.FindSubspace(ns);
	   if (macroNS == nullptr)
	   {
		   Throw(s[0], "Could not find namespace %s", ns);
	   }

	   auto* macro = macroNS->FindMacro(shortMacroName);
	   if (!macro)
	   {
		   Throw(s[0], "Could not find macro %s in namespace %s", shortMacroName, ns);
	   }

	   return macro;
   }

   void CScript::Invoke_S_Macro(cr_sex s)
   {
	   cstr macroName = GetAtomicArg(s[0]);

	   if (*macroName != '#')
	   {
		   Throw(s, "Expecting # in position 0 of s[0]");
	   }

	   auto* macro = FindMacro(*this, macroName + 1, s);

	   const IFunction& macroFunction = macro->Implementation();

	   auto& ss = System();

	   CallMacro(ss, macroFunction, s);

	   // This should now have mapped the invocation to the transformation

	   auto* pTransform = ss.GetTransform(s);
	   if (!pTransform)
	   {
		   Throw(s, "Expected a macro transformation of the input expression. But none was generated");
	   }
   }

   bool IsMacroInvocation(cr_sex s)
   {
	   // Macros are compound expressions with the first atomic expression led by character #, e.g (#eat fish (type = cod))
	   if (s.NumberOfElements() > 0)
	   {
		   cr_sex sDirective = s[0];
		   if (IsAtomic(sDirective))
		   {
			   cstr directive = sDirective.c_str();
			   if (directive[0] == '#')
			   {
				   return true;
			   }
		   }
	   }

	   return false;
   }

   void CompileMacrosRecursive(CScript& script, cr_sex sParent)
   {
	   for (int i = 0; i < sParent.NumberOfElements(); i++)
	   {
		   auto& s = sParent[i];
		   if (IsMacroInvocation(s))
		   {
			   script.Invoke_S_Macro(s);
		   }
		   else
		   {
			   CompileMacrosRecursive(script, s);
		   }
	   }
   }

   void CScript::CompileTopLevelMacros()
   {
	   cr_sex root = tree.Root();
	   CompileMacrosRecursive(*this, root);
   }

   void  CScript::CompileNextClosures()
   {
	   // Compiling a local function can enqueue a closure for further compilation, so handle that next
	   while (!closureJobs.empty())
	   {
		   CClosureDef cdef = closureJobs.back();
		   closureJobs.pop_back();
		   CompileClosureBody(*cdef.ClosureExpr, *cdef.Closure, *this);
	   }
   }

   void  CScript::CompileLocalFunctions()
   {
	   for (auto i = localFunctions.begin(); i != localFunctions.end(); ++i)
	   {
		   CompileFunctionFromExpression(*i->second.Fn, *i->second.FnDef, *this);
	   }
   }

   void CScript::CompileJITStubs()
   {
	   for (auto i = localFunctions.begin(); i != localFunctions.end(); ++i)
	   {
		   cstr fname = i->second.Fn->Name();
		   CompileJITStub(*i->second.Fn, *i->second.FnDef, *this, GetSystem(*this));
	   }
   }

   void CScript::CompileVTables()
   {
	   for(auto& ls: localStructures)
	   {
		   auto& s = *ls.Struct;
		   s.FillVirtualTables();
	   }
   }

   void AppendCompiledNamespacesForOneDirective(TNamespaceDefinitions& nsDefs, cr_sex sDirective, CScript& script)
   {
		cr_sex sElementName = GetAtomicArg(sDirective, 0);
		sexstring elementName = sElementName.String();
		if (AreEqual(elementName, "namespace"))
		{
			AssertNotTooFewElements(sDirective, 2);

			CBindNSExpressionToModule def;
			def.E = &sDirective[1];
			def.Module = &script;
			def.NS = NULL;

			AssertValidNamespaceDef(*def.E);

			nsDefs.push_back(def);
		}
		else if (AreEqual(elementName, "$"))
		{
			AssertNotTooFewElements(sDirective, 2);

			CBindNSExpressionToModule def;
			def.E = &sDirective[1];
			def.Module = &script;
			def.NS = NULL;
			def.isDefault = true;

			AssertValidNamespaceDef(*def.E);

			nsDefs.push_back(def);
		}
   }

   void AppendCompiledNamespaces(TNamespaceDefinitions& nsDefs, cr_sex root, CScript& script)
   {
	   for (int i = 0; i < root.NumberOfElements(); i++)
	   {
		   AppendCompiledNamespacesForOneDirective(nsDefs, root[i], script);
	   }
   }

	void CScript::AppendCompiledNamespaces(TNamespaceDefinitions& nsDefs)
	{
		cr_sex root = tree.Root();	
		Rococo::Script::AppendCompiledNamespaces(nsDefs, root, *this);
	}

	IStructure* LookupArg(cr_sex s, IModuleBuilder& module, OUT cstr& argName)
	{
		AssertCompound(s);
		AssertNotTooFewElements(s, 2);
		AssertNotTooManyElements(s, 2);

		cr_sex type = GetAtomicArg(s, 0);
		cr_sex name = GetAtomicArg(s, 1);

		AssertLocalIdentifier(name);

		IStructure* st = MatchStructure(type, module);
		if (st == NULL)
		{
			Throw(s, "Cannot find match to %s", type.c_str());
		}

		argName = name.c_str();

		return st;
	}

	IArchetype* LookupArchetype(cr_sex s, IModuleBuilder& module)
	{
		AssertCompound(s);
		AssertNotTooFewElements(s, 2);
		AssertNotTooManyElements(s, 2);

		cr_sex type = GetAtomicArg(s, 0);
		cr_sex name = GetAtomicArg(s, 1);

		//AssertTypeIdentifier(type); TODO - delete this comment
		AssertLocalIdentifier(name);

		return MatchArchetype(type, module);
	}
	
	enum { MAX_ARGS_PER_CLOSURE = 40};

	void AddArchetypeToNamespace(INamespaceBuilder& ns, cstr archetypeName, cr_sex s, CScript& script)
	{
		// (archetype fully-qualified-name (input1)...(inputN) -> (output1) ... (outputN))
		int mapIndex = GetIndexOf(2, s, ("->"));
		if (mapIndex < 0)
		{
			Throw(s, ("Could not find mapping token '->' in the archetype definition"));
		}

		int totalArgs = s.NumberOfElements() - 3;
			
		if (totalArgs > MAX_ARGS_PER_CLOSURE)
		{
			Throw(s, ("Too many arguments supplied to the archetype"));
		}

		const IStructure** st = (const IStructure**) alloca(sizeof(IStructure*) * totalArgs);
		const IArchetype** ar = (const IArchetype**) alloca(sizeof(IArchetype*) * totalArgs);
		const IStructure** genericArg1s = (const IStructure**) alloca(sizeof(IStructure*) * totalArgs);
		cstr* names = (cstr*) alloca(sizeof(cstr) * totalArgs);

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
			Throw(s, "Could not find archetype: '%s'", archetypeName);
		}

		ns.AddArchetype(archetypeName, names, st, ar, genericArg1s, nOutputs, s.NumberOfElements() - nOutputs - 3, &s);
	}

	const IArchetype& AddArchetypeNameToNamespace(INamespaceBuilder& ns, cstr archetypeName, cr_sex s)
	{
		if (ns.FindArchetype(archetypeName))
		{
			Throw(s, "Archetype declaration conflict. Multiple declarations using the same name: '%s'", archetypeName);
		}

		return ns.AddArchetype(archetypeName, NULL, NULL, NULL, NULL, 0, 0, &s);
	}

	void CScript::AddArchetype(cr_sex s)
	{
		// (archetype fully-qualified-name (input_args) -> (output_args) )

		AssertNotTooFewElements(s, 3);
		AssertNotTooManyElements(s, 3 + MAX_ARGS_PER_CLOSURE);

		cr_sex fullyQualifiedNameExpr = GetAtomicArg(s, 1);
		
		int mapIndex = GetIndexOf(2, s, ("->"));

		if (mapIndex < 0)
		{
			Throw(s, ("Expected mapping token '->' in archetype expression"));
		}
		
		AssertQualifiedIdentifier(fullyQualifiedNameExpr);

		NamespaceSplitter splitter(fullyQualifiedNameExpr.c_str());
		cstr body, tail;
		INamespaceBuilder& ns = ValidateSplitTail(splitter, OUT body, OUT tail, fullyQualifiedNameExpr, programObject, module);
		AddArchetypeToNamespace(ns, tail, s, *this);
	}

	void CScript::AddArchetypeName(cr_sex s)
	{
		// (archetype fully-qualified-name (input_args1)...(input_argsN) -> (output_args)...(output_argsN) )

		AssertNotTooFewElements(s, 3);
		AssertNotTooManyElements(s, MAX_ARGS_PER_CLOSURE + 3);

		cr_sex fullyQualifiedNameExpr = GetAtomicArg(s, 1);
		
		int mapIndex = GetIndexOf(2, s, ("->"));
		if (mapIndex < 0)
		{
			Throw(s, ("Could not find mapping token '->' in the archetype"));
		}

		AssertQualifiedIdentifier(fullyQualifiedNameExpr);

		NamespaceSplitter splitter(fullyQualifiedNameExpr.c_str());

		cstr body, tail;
		INamespaceBuilder& ns = ValidateSplitTail(REF splitter, OUT body, OUT tail, IN fullyQualifiedNameExpr, IN programObject, IN module);	

		const IArchetype* existing = ns.FindArchetype(tail);
		if (existing != NULL) Throw(fullyQualifiedNameExpr, ("Archetype already defined"));

		AssertValidArchetypeName(fullyQualifiedNameExpr, tail);

		const IArchetype& a = AddArchetypeNameToNamespace(ns, tail, s);
		IStructureBuilder& st = DeclareStructure(module, tail, StructurePrototype(MEMBERALIGN_1, INSTANCEALIGN_1, true, &a, false), fullyQualifiedNameExpr);
		st.AddMember(NameString::From(("bytecodeId")), TypeString::From(("Pointer")));
		st.AddMember(NameString::From(("parentSF")), TypeString::From(("Pointer")));
		ns.Alias(tail, st);
	}

	void SubstituteChars(cstr src, char* dest, int len, char from, char to)
	{
		for(int i = 0; i < len; ++i)
		{
			char c = src[i];
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

	InterfaceProtoDesc ComputeMethodCount(cr_sex s, CScript& script, cstr interfaceShortName)
	{
		InterfaceProtoDesc desc = { 0 };

		for (int i = 2; i < s.NumberOfElements(); ++i)
		{
			cr_sex child = s.GetElement(i);
			cr_sex childDirective = GetAtomicArg(child, 0);
			sexstring childType = childDirective.String();

			if (AreEqual(childType, ("extends")))
			{
				cr_sex baseExpr = GetAtomicArg(child, 1);
				sexstring baseName = baseExpr.String();

				if (desc.base != NULL) ThrowTokenAlreadyDefined(child, ("extends"), interfaceShortName, ("attribute"));

				desc.base = GetInterfaceFQN(baseExpr, script);
				if (desc.base == NULL)
				{
					Throw(baseExpr, ("The base interface has not been defined prior to this point."));
				}
			}
			else if (AreEqual(childType, ("attribute")))
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
		return s.NumberOfElements() > 1 && IsAtomic(s[0]) && AreEqual(s[0].String(), ("method"));
	}

	struct IMethodEnumerator
	{
		virtual void operator()(cr_sex smethodDef, cstr className, cstr methodName) = 0;
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

				cstr methodClassName, methodName;
				if (splitter.SplitTail(OUT methodClassName, OUT methodName) )
				{
					if (!AreEqual(methodName, ("Construct")))
					{
						callback(smethodDef, methodClassName, methodName);
					}
				}
				else
				{
					Throw(smethodDef[1], ("Could not split full method name into class and short method name"));
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
			virtual void operator()(cr_sex smethodDef, cstr className, cstr methodName)
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
				Throw(sbaseName, ("Expecting base interface name"));
			}

			base = MatchInterface(sbaseName, module);
			if (base == nullptr)
			{
				Throw(sbaseName, ("Could not resolve base interface name"));
			}
		}

		return { cb.count, base };	
	}

	void CScript::AddInterfacePrototype(cr_sex s, bool isInterfaceDefinedFromClassMethods)
	{
		cr_sex nameExpr = GetAtomicArg(s, 1);
		cstr name = nameExpr.c_str();

		NamespaceSplitter splitter(name);

		cstr body, tail;
		INamespaceBuilder& ns = ValidateSplitTail(REF splitter, OUT body, OUT tail, IN nameExpr, IN programObject, IN module);

		if (strstr(body, "IECS") != nullptr)
		{
			OS::TripDebugger();
		}

		AssertValidInterfaceName(nameExpr, tail);

		IInterfaceBuilder* current_interf = ns.FindInterface(tail);
		if (current_interf != NULL)	Throw(nameExpr, "Duplicate interface definition. Original was defined in %s", current_interf->NullObjectType().Module().Name());

		TokenBuffer flatName;
		SubstituteChars(name, flatName.Text, 256, char('.'), char('_'));
		
		TokenBuffer nullName;
		StringPrint(nullName, ("_Null_%s"), (cstr) flatName);

		StructurePrototype prototype(MEMBERALIGN_1, INSTANCEALIGN_1, true, NULL, true);

		IStructureBuilder& nullObject = DeclareStructure(module, nullName, prototype, nameExpr);

		InterfaceProtoDesc desc = isInterfaceDefinedFromClassMethods ?  CountMethodsDefinedForClass(s, module) : ComputeMethodCount(s, *this, tail);
		IInterfaceBuilder* interf = ns.DeclareInterface(tail, desc.methodCount, nullObject, desc.base);

		// We also need to declare a null object that implements the interface			
		nullObject.AddMember(NameString::From(("_typeInfo")), TypeString::From(("Pointer")));
		nullObject.AddMember(NameString::From(("_refCount")), TypeString::From(("Int64")));
		nullObject.AddMember(NameString::From(("_vTable1")), TypeString::From(("Pointer")));

		for (const IInterface* z = interf; z != NULL; z = z->Base())
		{
			if (AreEqual(z->NullObjectType().Name(), ("_Null_Sys_Type_IString")))
			{
				nullObject.AddMember(NameString::From(("length")), TypeString::From(("Int32")));
				nullObject.AddMember(NameString::From(("buffer")), TypeString::From(("Pointer")));
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
			if (AreEqual(elementName.String(), ("archetype")))
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
			if (AreEqual(elementName.String(), ("archetype")))
			{
				AddArchetypeName(e);
			}
		}
	}

	void CScript::AddVirtualMethod(IN const IArchetype& archetype, REF IInterfaceBuilder& inter, IN size_t vmIndex, cr_sex def)
	{
		int nArgs = archetype.NumberOfInputs() + archetype.NumberOfOutputs();

		cstr* names = (cstr*) alloca(nArgs * sizeof(cstr));
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

		names[count] = ("_vTable");
		resolvedTypes[count] = module.Object().IntrinsicModule().FindStructure(("Pointer"));
		archetypes[count] = NULL;
		isOut[count] = false;
		genericArg1s[count] = NULL;

		inter.SetMethod(vmIndex, archetype.Name(), nArgs, names, resolvedTypes, archetypes, genericArg1s, isOut, &def);
	}

	void CScript::AddEnumeratedVirtualMethod(IN cr_sex methodExpr, IN cstr methodName, REF IInterfaceBuilder& inter, IN size_t vmIndex)
	{
		AssertCompound(methodExpr);
		AssertNotTooFewElements(methodExpr, 4); // (method Dog.Bark -> :)

		int bodyIndex = GetIndexOf(2, methodExpr, (":"));
		if (bodyIndex < 0)
		{
			Throw(methodExpr, ("Expecting body indicator token ':' inside method definition"));
		}

		enum { MAX_METHOD_ARGS = 40 };
		
		if (bodyIndex > MAX_METHOD_ARGS + 3)
		{
			Throw(methodExpr, ("Too many arguments in method"));
		}
		
		int mapIndex = GetIndexOf(2, methodExpr, ("->"));
		if (mapIndex < 0)
		{
			Throw(methodExpr, ("Could not find the mapping token '->' in the method definition"));
		}

		int nArgs = bodyIndex - 2;
		cstr* names = (cstr*)alloca(nArgs * sizeof(cstr));
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

			names[count] = name.c_str();
			resolvedTypes[count] = MatchStructure(type, module);
			archetypes[count] = MatchArchetype(type, module);
			genericArg1s[count] = NULL;
			isOut[count] = true;
			if (resolvedTypes[count] == NULL) Throw(typeNameExptr, ("Cannot resolve type. Check the spelling and/or use a fully qualified name."));
			count++;
		}

		for (int i = 2; i < mapIndex; ++i)
		{
			cr_sex typeNameExptr = methodExpr.GetElement(i);
			AssertCompound(typeNameExptr);
			AssertNotTooFewElements(typeNameExptr, 2);

			cr_sex type = GetAtomicArg(typeNameExptr, 0);

			if (AreEqual(type.String(), ("array")))
			{
				AssertNotTooManyElements(typeNameExptr, 3);
				cr_sex name = GetAtomicArg(typeNameExptr, 2);
				cr_sex elementType = GetAtomicArg(typeNameExptr, 1);
				names[count] = name.c_str();
				resolvedTypes[count] = MatchStructure(type, module);
				if (resolvedTypes[count] == NULL) Throw(typeNameExptr, ("Cannot resolve type. Check the spelling and/or use a fully qualified name."));
				archetypes[count] = NULL;
				genericArg1s[count] = MatchStructure(elementType, module);
			}
			else
			{
				AssertNotTooManyElements(typeNameExptr, 2);
				cr_sex name = GetAtomicArg(typeNameExptr, 1);
				names[count] = name.c_str();
				resolvedTypes[count] = MatchStructure(type, module);
				if (resolvedTypes[count] == NULL) Throw(typeNameExptr, ("Cannot resolve type. Check the spelling and/or use a fully qualified name."));
				archetypes[count] = MatchArchetype(type, module);
				genericArg1s[count] = NULL;
			}

			isOut[count] = false;
			count++;;
		}

		names[count] = ("_vTable");
		resolvedTypes[count] = module.Object().IntrinsicModule().FindStructure(("Pointer"));
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
		cstr methodName = methodNameExpr.c_str();

		int mapIndex = GetIndexOf(1, virtualMethodExpr, ("->"));
		if (mapIndex < 0)
		{
			Throw(virtualMethodExpr, ("Could not find the mapping token '->' in the virtual method definition"));
		}

		int nArgs = virtualMethodExpr.NumberOfElements() - 2 + 1;
		cstr* names = (cstr*) alloca(nArgs * sizeof(cstr));
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

			names[count] = name.c_str();
			resolvedTypes[count] = MatchStructure(type, module);
 			archetypes[count] = MatchArchetype(type, module);
			genericArg1s[count] = NULL;
			isOut[count] = true;
			if (resolvedTypes[count] == NULL) Throw(typeNameExptr, ("Cannot resolve type. Check the spelling and/or use a fully qualified name."));		
			count++;
		}

		for(int i = 1; i < mapIndex; ++i)
		{
			cr_sex typeNameExptr = virtualMethodExpr.GetElement(i);
			AssertCompound(typeNameExptr);
			AssertNotTooFewElements(typeNameExptr, 2);

			cr_sex sType = GetAtomicArg(typeNameExptr,0);
			cstr type = sType.c_str();

			bool isQualified = false;
			if (typeNameExptr.NumberOfElements() > 2)
			{
				if (AreEqual(type, "const") || AreEqual(type, "ref") || AreEqual(type, "out"))
				{
					isQualified = true;
				}
			}

			if (AreEqual(type, "array"))
			{				
				AssertNotTooManyElements(typeNameExptr, 3);
				cr_sex name = GetAtomicArg(typeNameExptr,2);
				cr_sex elementType =  GetAtomicArg(typeNameExptr,1);
				names[count] = name.c_str();
				resolvedTypes[count] = MatchStructure(sType, module);
				if (resolvedTypes[count] == NULL) Throw(typeNameExptr, "Cannot resolve type. Check the spelling and/or use a fully qualified name.");	
				archetypes[count] = NULL;
				genericArg1s[count] = MatchStructure(elementType, module);
			}
			else
			{
				AssertNotTooManyElements(typeNameExptr, isQualified ? 3 : 2);

				cr_sex name = GetAtomicArg(typeNameExptr, isQualified ? 2 : 1);
				cr_sex sArgType = GetAtomicArg(typeNameExptr, isQualified ? 1 : 0);
				names[count] = name.c_str();
				resolvedTypes[count] = MatchStructure(sArgType, module);
				if (resolvedTypes[count] == NULL) Throw(typeNameExptr, "Cannot resolve type. Check the spelling and/or use a fully qualified name.");	
				archetypes[count] = MatchArchetype(sArgType, module);
				genericArg1s[count] = NULL;
			}					
			
			isOut[count] = false;
			count++;;
		}

		names[count] = "_vTable";
		resolvedTypes[count] = module.Object().IntrinsicModule().FindStructure("Pointer");
		archetypes[count] = NULL;
		genericArg1s[count] = NULL;
		isOut[count] = false;

		count++;

		inter.SetMethod(vmIndex, methodName, nArgs, names, resolvedTypes, archetypes, genericArg1s, isOut, &virtualMethodExpr);
	}

	void ValidateDefineAttribute(IAttributes& a, cr_sex attributeExpr)
	{
		cr_sex nameExpr = GetAtomicArg(attributeExpr, 1);
		cstr name = nameExpr.c_str();
		if (!a.AddAttribute(name, &attributeExpr))
		{
			Throw(nameExpr, ("Duplicate attribute name in attribute collections are not allowed"));
		}
	}

	void AssertExpectedParse(cr_sex source, VariantValue& value, VARTYPE type, cstr buffer)
	{
		auto result = Parse::TryParse(value, type, buffer);
		switch (result)
		{
		case Parse::PARSERESULT_GOOD:
			return;
		case Parse::PARSERESULT_BAD_DECIMAL_DIGIT:
			Throw(source, ("Bad decimal digit"));
			break;
		case Parse::PARSERESULT_HEXADECIMAL_BAD_CHARACTER:
			Throw(source, ("Bad hexadecimal digit"));
			break;
		case Parse::PARSERESULT_HEXADECIMAL_INCORRECT_NUMBER_OF_DIGITS:
			Throw(source, ("Incorrect number of digits"));
			break;
		case Parse::PARSERESULT_HEX_FOR_FLOAT:
			Throw(source, ("Hexadecimal may not be used to initialize a floating point value"));
			break;
		case Parse::PARSERESULT_OVERFLOW:
			Throw(source, ("Overflow"));
			break;
		case Parse::PARSERESULT_UNDERFLOW:
			Throw(source, ("Underflow"));
			break;
		case Parse::PARSERESULT_UNHANDLED_TYPE:
		default:
			Throw(source, ("Unhandled type/literal"));
			break;
		}
	}

	void CScript::ComputeGlobal(cr_sex globalDef, int& globalBaseIndex)
	{
		if (globalDef.NumberOfElements() != 5)
		{
			Throw(globalDef, ("(global <vartype> <varname> = <initvalue>)"));
		}

		if (!IsAtomic(globalDef[3]) || !AreEqual(globalDef[3].String(), ("=")))
		{
			Throw(globalDef[3], ("Expecting variable assignment character '='"));
		}

		cr_sex stype = GetAtomicArg(globalDef, 1);
		cr_sex sname = globalDef[2];
		cr_sex svalue = GetAtomicArg(globalDef, 4);

		AssertLocalIdentifier(sname);
		sexstring name = sname.String();
		sexstring type = stype.String();
		sexstring value = svalue.String();

		auto i = globalVariables.find(name->Buffer);
		if (i != globalVariables.end())
		{
			Throw(sname, "Duplicate global variable name.");
		}

		auto structure = MatchStructure(stype, module);
		if (structure == nullptr)
		{
			Throw(stype, "Cannot resolve global variable type");
		}

		if (!IsPrimitiveType(structure->VarType()))
		{
			Throw(stype, "Only primitive types can serve as global variables");
		}

		GlobalValue g;
		g.type = structure->VarType();
		g.offset = globalBaseIndex;
		globalBaseIndex += structure->SizeOfStruct();
		AssertExpectedParse(stype, g.initialValue, g.type, value->Buffer);
		globalVariables.insert(name->Buffer, g);
	}

	void CScript::ComputeGlobals(int& globalBaseIndex)
	{
		cr_sex root = tree.Root();
		for (int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = root[i];
			cr_sex elementName = GetAtomicArg(topLevelItem, 0);

			// (global <vartype> <varname> = <initvalue>)

			if (AreEqual(elementName.String(), ("global")))
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

			if (AreEqual(elementName.String(), ("interface")))
			{
				cr_sex name = GetAtomicArg(e, 1);

				NamespaceSplitter splitter(name.c_str());

				cstr body, publicName;
				INamespaceBuilder& ns = ValidateSplitTail(REF splitter, OUT body, OUT publicName, IN e, IN programObject, IN module);		
				
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

					if (AreEqual(methodName, "attribute"))
					{
						ValidateDefineAttribute(interf->Attributes(), virtualMethodExpr);
					}
					else if (AreEqual(methodName, "extends"))
					{						
					}
					else
					{
						AddVirtualMethod(virtualMethodExpr, *interf, methodIndex++);
					}
				}
			}
			else if (AreEqual(elementName.String(), "class"))
			{
				for (int k = 1; k < e.NumberOfElements(); ++k)
				{
					cr_sex classDirective = e[k];
					if (classDirective.NumberOfElements() > 1 && IsAtomic(classDirective[0]) && AreEqual(classDirective[0].String(), "defines"))
					{	
						auto classStruct = module.FindStructure(e[1].c_str());

						if (classStruct->InterfaceCount() != 1)
						{
							Throw(e, "Classes that define an interface may not implement more than one interface");
						}

						cr_sex name = GetAtomicArg(classDirective, 1);

						NamespaceSplitter splitter(name.c_str());

						cstr body, publicName;
						INamespaceBuilder& ns = ValidateSplitTail(REF splitter, OUT body, OUT publicName, IN e, IN programObject, IN module);

						int methodIndex = 0;

						struct : IMethodEnumerator
						{
							sexstring className;
							CScript* This;
							IInterfaceBuilder* interf;
							int methodIndex;

							virtual void operator()(cr_sex smethodDef, cstr className, cstr methodName)
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

	void ValidateClassImplementsMethod(const IArchetype& method, const IStructure& classType, cr_sex src, cstr source)
	{
		TokenBuffer classAndMethod;
		StringPrint(classAndMethod, "%s.%s", classType.Name(), method.Name());

		const IFunction* f = classType.Module().FindFunction(classAndMethod);
		if (f == NULL)
		{
			Throw(classType.Definition() != NULL ? *(const ISExpression*) classType.Definition() : src, "Expecting method named '%s' inside source module", (cstr)classAndMethod);
		}

		TokenBuffer dottedName;
		StringPrint(dottedName, "%s.", source);

		ValidateArchetypeMatchesArchetype(src, *f, method, dottedName);
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
		cstr name = ProgramModule().Name();

		for(int i = 0; i < module.StructCount();  ++i)
		{
			IStructureBuilder& s = module.GetStructure(i);
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
				Throw(*exp, "Type requires an explicit constructor definition inside the module %s. Expecting %s.Construct", type.Module().Name(), type.Name());
			}
		}
	}

	void ValidateMembersForConstructors(IProgramObject& obj, const IStructure& specimen, const IStructure& child)
	{
		for(int i = 0; i < child.MemberCount(); ++i)
		{
			const IMember& m = child.GetMember(i);

			auto type = m.UnderlyingType()->VarType();

			if (!IsPrimitiveType(type) && type != VARTYPE_Array && type != VARTYPE_Map && type != VARTYPE_List)
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
			cr_sex sElementName = GetAtomicArg(e, 0);
			sexstring elementName = sElementName.String();
			if (AreEqual(elementName, "interface"))
			{
				AddInterfacePrototype(e, false);
			}
		}
	}

	cstr topLevelItems[] =
	{
		"namespace",
		"struct",
		"function",
		"alias",
		"archetype",
		"using",
		"interface",
		"class",
		"method",
		"factory",
		"macro",
		"global",
		"$",
		"strong",
		NULL
	};

	bool IsOneOf(cstr item, cstr items[])
	{
		for(cstr* i = items; *i != NULL; i++)
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
			try
			{
				cr_sex e = root.GetElement(i);
				cr_sex elementName = GetAtomicArg(e, 0);

				sexstring directive = elementName.String();
				if (directive->Buffer[0] != '#' && directive->Buffer[0] != '\'' && !IsOneOf(directive->Buffer, topLevelItems))
				{
					Throw(elementName, "Unknown top level item, expecting keyword or data item");
				}
			}
			catch (ParseException& ex)
			{
				char msg[512];
				SafeFormat(msg, "Error validating top level item with index %d. Err %s", i, ex.Message());
				ParseException outerEx(ex.Start(), ex.End(), ex.Name(), msg, ex.Specimen(), ex.Source());
				throw outerEx;
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

			if (AreEqual(elementName.String(), ("using")))
			{
				AssertNotTooFewElements(topLevelItem, 2);
				AssertNotTooManyElements(topLevelItem, 2);

				cr_sex prefixExpr = topLevelItem.GetElement(1);
				AssertQualifiedIdentifier(prefixExpr);

				sexstring prefix = prefixExpr.String();

				INamespace* ns = module.Object().GetRootNamespace().FindSubspace(prefix->Buffer);
				if (ns == nullptr)
				{
					if (Eq(prefix->Buffer, "$"))
					{
						ns = const_cast<INamespace*>(module.DefaultNamespace());
					}
				}

				if (ns == nullptr)
				{
					Throw(prefixExpr, "Could not find namespace specified in the using directive");
				}

				module.UsePrefix(ns->FullName()->Buffer);
			}
		}
	}

	void CompileFactoryBody(IFactoryBuilder& factory, cr_sex factoryDef, int bodyStart,  CScript& script)
	{
		CCompileContext cc(script, ("factory"));

		IFunctionBuilder& f = factory.Constructor();
		cstr fname = f.Name();
		ICodeBuilder& builder = f.Builder();
		builder.Begin();

		CCompileEnvironment ce(script, builder, &factory);

		CompileExpressionSequence(ce, bodyStart, factoryDef.NumberOfElements()-1, factoryDef);

		builder.End();
		builder.Assembler().Clear();

#ifdef _DEBUG
		if (Rococo::OS::IsDebugging())
		{
			AutoFree<VM::IDisassembler> disassembler ( ce.Object.VirtualMachine().Core().CreateDisassembler() );
			Disassemble(*disassembler, f, ce.SS);
		}
#endif
	}

	IFunctionBuilder& GetConcreteMethod(cr_sex src, sexstring className, cstr methodName, IModuleBuilder& module)
	{
		TokenBuffer fullName;
		StringPrint(fullName, ("%s.%s"), (cstr) className->Buffer, methodName);

		IFunctionBuilder* cons = module.FindFunction(fullName);

		if (cons == NULL)
		{
			Throw(src, "Cannot find %s in the module", fullName);
		}

		return *cons;
	}

	const IStructure& GetStructure(cr_sex typeExpr, const sexstring typeName, CScript& script)
	{
		const IStructure* s = Compiler::MatchStructure(script.Object().Log(), typeExpr.c_str(), script.ProgramModule());
		if (s == NULL)
		{
			Throw(typeExpr, ("Could not resolve type"));
		}
		return *s;
	}

	bool HasElementEqualTo(cr_sex s, int index, cstr value)
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


		if (!HasElementEqualTo(body, 0, ("construct")))
		{
			return NULL;
		}

		// Now we know/hope the body expression is of form (construct class-name arg1 arg2 ... argN)
		cr_sex classNameExpr = GetAtomicArg(body, 1);
		sexstring className = classNameExpr.String();

		IStructureBuilder* s = script.ProgramModule().FindStructure(className->Buffer);
		if (s == NULL)
		{
			Throw(classNameExpr, ("Cannot find the specified class in the module"));
		}

		IFunctionBuilder& cons = GetConcreteMethod(classNameExpr, className, ("Construct"), script.ProgramModule());
		if (cons.NumberOfOutputs() > 0)
		{
			Throw(classNameExpr, "Unexpected output %s.Construct in the module", (cstr)className->Buffer);
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

			if (AreEqual(elementName.String(), "factory"))
			{
				cr_sex factoryNameExpr = GetAtomicArg(topLevelItem, 1);
				NamespaceSplitter splitter(factoryNameExpr.c_str());

				cstr nsRoot, publicName;
				splitter.SplitTail(OUT nsRoot, OUT publicName);

				INamespaceBuilder* ns = programObject.GetRootNamespace().FindSubspace(nsRoot);
				if (ns == nullptr)
				{
					ns = static_cast<INamespaceBuilder*>(const_cast<INamespace*>( module.DefaultNamespace()) );
				}

				if (ns == nullptr)
				{
					Throw(elementName, "Could not resolve namespace");
				}

				IFactoryBuilder* f = ns->FindFactory(publicName);

				int bodyIndex = GetIndexOf(1, topLevelItem, (":"));
				if (bodyIndex < 0) Throw(topLevelItem, ("Could not find body indicator ':' in factory definition"));
				if (bodyIndex >= topLevelItem.NumberOfElements()) Throw(topLevelItem, ("Body indicator ':' was at the end of the expression. Expecting body to follow it"));

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

	int CountClassElements(cr_sex classDef, cstr elementName)
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
			
			if (AreEqual(elementName, ("macro")))
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
		
		cstr ns, shortName;
		if (!splitter.SplitTail(OUT ns, OUT shortName))Throw(factoryNameExpr, "Expecting fully qualified name: (factory <fully-qualified-factory-name> <fully-qualified-interface-name> <arg1> ... <argN> : <body>)");

		int bodyIndex = GetIndexOf(3, factoryDef, ":");
		if (bodyIndex < 0)	Throw(factoryDef, "Expecting a body start token ':' in the expression");
		if (bodyIndex == factoryDef.NumberOfElements()-1)	Throw(factoryDef, "Expecting a expressions to follow the body start token ':' in the expression");

		INamespaceBuilder* factoryNS = module.Object().GetRootNamespace().FindSubspace(ns);
		if (factoryNS == nullptr)
		{
			if (Eq(ns, "$"))
			{
				factoryNS =  static_cast<INamespaceBuilder*>( const_cast<INamespace*>(module.DefaultNamespace()) );
			}
		}

		if (factoryNS == nullptr)
		{
			Throw(factoryNameExpr, "Cannot find the namespace [%s] containing the factory  (%s instance (%s ... ) )", ns, interfaceName->Buffer, factoryName->Buffer);
		}

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
		if (interf == NULL)
		{
			Throw(factoryInterfaceExpr, "Unknown interface %s. Factory semantics: (factory <factory> <interface> (arg1) (arg2): <body>)", interfaceName->Buffer);
		}

		IFactory* factory = factoryNS->FindFactory(shortName);
		if (factory != NULL) Throw(factoryNameExpr, "A factory with the same name exists in the same namespace");

		IFunctionBuilder& factoryFunction = module.DeclareFunction(FunctionPrototype(factoryName->Buffer, false), &factoryDef);

		for(int i = 0; i < inputCount; ++i)
		{
			cr_sex inputExpression = factoryDef.GetElement(i + 3);
			cr_sex typeExpr = GetAtomicArg(inputExpression, 0);
			cr_sex nameExpr = GetAtomicArg(inputExpression, 1);
			factoryFunction.AddInput(NameString::From(nameExpr.String()), TypeString::From(typeExpr.String()), NULL);
		}

		factoryFunction.AddInput(NameString::From("_this"), TypeString::From("Sys.Type.Pointer"), NULL);

		factoryNS->RegisterFactory(shortName, factoryFunction, *interf, factoryInterfaceExpr.String());
	}

	cr_sex CScript::GetActiveRoot()
	{
		return tree.Root();
	}

	cr_sex CScript::GetActiveExpression(cr_sex s)
	{
		return s;
	}

	void CScript::ComputeStructureNames()
	{
		localStructures.clear();

		cr_sex root = GetActiveRoot();

		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = GetActiveExpression(root[i]);
			AssertNotTooFewElements(topLevelItem, 1);
			cr_sex elementNameExpr = GetAtomicArg(topLevelItem, 0);
			sexstring elementName = elementNameExpr.String();

			bool isStruct = false;
			bool isClass = false;
			bool isStrong = false;

			if (AreEqual(elementName, "strong"))
			{
				isStrong = true;
			}
			else if (AreEqual(elementName, "struct"))
			{
				isStruct = true;
			}
			else if (AreEqual(elementName, "class"))
			{
				isClass = true;
			}

			if (isStruct || isClass)
			{
				// (struct <name> (<type0> <varname0>) ... (<typeN>, <varnameN>) )
				AssertNotTooFewElements(topLevelItem, 2);

				cr_sex structNameExpr = GetAtomicArg(topLevelItem, 1);
				cstr structName = structNameExpr.c_str();

				AssertValidStructureName(structNameExpr);

				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, isClass);

				IStructureBuilder& s = DeclareStructure(module, structName, prototype, structNameExpr);

				if (isClass)
				{
					s.AddMember(NameString::From("_typeInfo"), TypeString::From("Pointer"));
					s.AddMember(NameString::From("_refCount"), TypeString::From("Int64"));

					int interfaceCount = 1; // The vtable0 interface, giving the destructor Id
					interfaceCount += CountClassElements(topLevelItem, "implements");
					interfaceCount += CountClassElements(topLevelItem, "defines");

					for (int k = 1; k < topLevelItem.NumberOfElements(); ++k)
					{
						cr_sex sdefineInterface = topLevelItem[k];
						if (IsCompound(sdefineInterface) && IsAtomic(sdefineInterface[0]) && AreEqual(sdefineInterface[0].String(), "defines"))
						{
							AddInterfacePrototype(sdefineInterface, true);
						}
					}

					for (int l = 1; l < interfaceCount; l++)
					{
						char vtableName[32];
						SafeFormat(vtableName, 32, "_vTable%d", l);
						s.AddMember(NameString::From(vtableName), TypeString::From("Pointer"));
					}
				}

				CBindStructDefToExpression structDef;
				structDef.Struct = &s;
				structDef.StructDef = &topLevelItem;

				localStructures.push_back(structDef);
			}	

			else if (isStrong)
			{
				// (strong <name> (<primitive-value-holder>))
				AssertNotTooFewElements(topLevelItem, 3);
				AssertNotTooManyElements(topLevelItem, 3);

				cr_sex strongNameExpr = GetAtomicArg(topLevelItem, 1);
				cstr strongName = strongNameExpr.c_str();

				cr_sex wrapper = topLevelItem[2];
				if (wrapper.NumberOfElements() != 1)
				{
					Throw(wrapper, "Expecting a compound expression containing a primitive type. One of {Int32, Int64, Float32, Float64, Bool}");
				}

				cr_sex svalueType = GetAtomicArg(wrapper, 0);
				cstr sPrimitiveType = svalueType.c_str();

				StructurePrototype prototype(MEMBERALIGN_4, INSTANCEALIGN_16, true, NULL, false);

				IStructureBuilder& s = DeclareStrongType(module, strongName, sPrimitiveType, topLevelItem);

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

			if (AreEqual(elementName.String(), "function"))
			{
				isFunction = true;
			}
			else if (AreEqual(elementName.String(), "method"))
			{
				isMethod = true;
			}

			if (isFunction || isMethod)
			{
				// (function <name> (args) -> (results): body)
				// (method <local-class>.<method-name> (args) -> (results): body)
				cr_sex functionName = GetAtomicArg(topLevelItem, 1);
				AssertValidFunctionName(functionName);

				FunctionPrototype prototype(functionName.c_str(), isMethod);
				
				IFunctionBuilder& f = DeclareFunction(*this, topLevelItem, prototype);			

				CBindFnDefToExpression binding;
				binding.Fn = &f;
				binding.FnDef = &topLevelItem;
				localFunctions.insert(f.Name(), binding);
			}
		}

		for(int i = 0; i < root.NumberOfElements(); i++)
		{
			cr_sex topLevelItem = root.GetElement(i);
			cr_sex elementName = GetAtomicArg(topLevelItem, 0);

			if (AreEqual(elementName.String(), "factory"))
			{
				DeclareFactory(topLevelItem);
			}
		}
	}

	void CScript::ComputeStructureFields()
	{
		for(auto j = localStructures.begin(); j != localStructures.end(); ++j)
		{
			if (!j->Struct->IsStrongType())
			{
				for (int i = 2; i < j->StructDef->NumberOfElements(); i++)
				{
					cr_sex field = j->StructDef->GetElement(i);
					AddMember(*j->Struct, field, System());
				}
			}
			else
			{
				// Strong types have their underlying type set when the defintion is parsed.
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
			Throw(source, "%s", e.Message());
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

	const IStructure& GetElementTypeForArrayVariable(CCompileEnvironment& ce, cr_sex src, cstr arrayName)
	{
		NamespaceSplitter splitter(arrayName);

		cstr instance, arrayMember;
		if (splitter.SplitTail(instance, arrayMember))
		{
			const IStructure* s = ce.Builder.GetVarStructure(instance);
			if (s == NULL)
			{
				ThrowTokenNotFound(src, instance, ce.Builder.Owner().Name(), ("variable"));
			}

			int offset = 0;
			const IMember *m = FindMember(*s, arrayMember, REF offset);
			if (m == NULL)
			{
				ThrowTokenNotFound(src, arrayMember, instance, ("member"));
			}

			return *m->UnderlyingGenericArg1Type();
		}

		const ArrayDef* variableDef = ce.Script.GetElementTypeForArrayVariable(ce.Builder, arrayName);
		if (variableDef == NULL)
		{
			ThrowTokenNotFound(src, arrayName, ce.Builder.Owner().Name(), ("member"));
		}

		return variableDef->ElementType;
	}

	const IStructure& GetKeyTypeForMapVariable(CCompileEnvironment& ce, cr_sex src, cstr mapName)
	{
		NamespaceSplitter splitter(mapName);

		cstr instance, mapMember;
		if (splitter.SplitTail(instance, mapMember))
		{
			const IStructure* s = ce.Builder.GetVarStructure(instance);
			if (s == NULL)
			{
				ThrowTokenNotFound(src, instance, ce.Builder.Owner().Name(), ("variable"));
			}

			int offset = 0;
			const IMember* m = FindMember(*s, mapMember, REF offset);
			if (m == NULL)
			{
				ThrowTokenNotFound(src, mapMember, instance, ("member"));
			}

			return *m->UnderlyingGenericArg1Type();
		}

		const MapDef* variableDef = ce.Script.GetMapDef(ce.Builder, mapName);
		if (variableDef == NULL)
		{
			ThrowTokenNotFound(src, mapName, ce.Builder.Owner().Name(), ("member"));
		}

		return variableDef->KeyType;
	}

	const IStructure& GetValueTypeForMapVariable(CCompileEnvironment& ce, cr_sex src, cstr mapName)
	{
		NamespaceSplitter splitter(mapName);

		cstr instance, mapMember;
		if (splitter.SplitTail(instance, mapMember))
		{
			const IStructure* s = ce.Builder.GetVarStructure(instance);
			if (s == NULL)
			{
				ThrowTokenNotFound(src, instance, ce.Builder.Owner().Name(), ("variable"));
			}

			int offset = 0;
			const IMember* m = FindMember(*s, mapMember, REF offset);
			if (m == NULL)
			{
				ThrowTokenNotFound(src, mapMember, instance, ("member"));
			}

			return *m->UnderlyingGenericArg2Type();
		}

		const MapDef* variableDef = ce.Script.GetMapDef(ce.Builder, mapName);
		if (variableDef == NULL)
		{
			ThrowTokenNotFound(src, mapName, ce.Builder.Owner().Name(), ("member"));
		}

		return variableDef->ValueType;
	}

	void AddArrayDef(CScript& script, ICodeBuilder& builder, cstr arrayName, const IStructure& elementType, cr_sex s)
	{
		script.AddArrayDef(builder, arrayName, elementType, s);
	}

	const IStructure& GetListDef(CCompileEnvironment& ce, cr_sex src, cstr listName)
	{
		NamespaceSplitter splitter(listName);

		cstr instance, listMember;
		if (splitter.SplitTail(instance, listMember))
		{
			const IStructure* s = ce.Builder.GetVarStructure(instance);
			if (s == NULL)
			{
				ThrowTokenNotFound(src, instance, ce.Builder.Owner().Name(), ("variable"));
			}

			int offset = 0;
			const IMember *m = FindMember(*s, listMember, REF offset);
			if (m == NULL)
			{
				ThrowTokenNotFound(src, listMember, instance, ("member"));
			}

			return *m->UnderlyingGenericArg1Type();
		}

		const ListDef* variableDef = ce.Script.GetListDef(ce.Builder, listName);
		if (variableDef == NULL)
		{
			ThrowTokenNotFound(src, listName, ce.Builder.Owner().Name(), ("member"));
		}

		return variableDef->ElementType;
	}

	void AddListDef(CScript& script, ICodeBuilder& builder, cstr name, const IStructure& elementType, cr_sex s)
	{
		script.AddListDef(builder, name, elementType, s);
	}

	void AddMapDef(CScript& script, ICodeBuilder& builder, cstr name, const IStructure& keyType, const IStructure& valueType, cr_sex s)
	{
		script.AddMapDef(builder, name, keyType, valueType, s);
	}

	void AddMapNodeDef(CScript& script, ICodeBuilder& builder, const MapDef& def, cstr nodeName, cstr mapName, cr_sex s)
	{
		script.AddMapNodeDef(builder, def, mapName, nodeName, s);
	}

	const MapDef GetMapDef(CCompileEnvironment& ce, cr_sex src, cstr name)
	{
		NamespaceSplitter splitter(name);

		cstr instance, member;
		if (splitter.SplitTail(instance, member))
		{
			const IStructure* s = ce.Builder.GetVarStructure(instance);
			if (s == NULL)
			{
				ThrowTokenNotFound(src, instance, ce.Builder.Owner().Name(), ("variable"));
			}

			int offset = 0;
			const IMember *m = FindMember(*s, member, REF offset);
			if (m == NULL)
			{
				ThrowTokenNotFound(src, member, instance, ("member"));
			}

			return MapDef(*(const ISExpression*) s->Definition(), *m->UnderlyingGenericArg1Type(), *m->UnderlyingGenericArg2Type());
		}

		const MapDef* mapDef = ce.Script.GetMapDef(ce.Builder, name);
		if (mapDef == NULL)
		{
			ThrowTokenNotFound(src, name, ce.Builder.Owner().Name(), ("member"));
		}

		return *mapDef;
	}

	const MapNodeDef& GetMapNodeDef(CCompileEnvironment& ce, cr_sex src, cstr name)
	{
		const MapNodeDef* def = ce.Script.GetMapNodeDef(ce.Builder, name);
		if (def == NULL)
		{
			Throw(src, ("Internal compiler error: No definition for this node was found. "));
		}

		return *def;
	}

	const IStructure& GetNodeDef(CCompileEnvironment& ce, cr_sex src, cstr nodeName)
	{
		NamespaceSplitter splitter(nodeName);

		cstr instance, listMember;
		if (splitter.SplitTail(instance, listMember))
		{
			const IStructure* s = ce.Builder.GetVarStructure(instance);
			if (s == NULL)
			{
				ThrowTokenNotFound(src, instance, ce.Builder.Owner().Name(), ("variable"));
			}

			int offset = 0;
			const IMember *m = FindMember(*s, listMember, REF offset);
			if (m == NULL)
			{
				ThrowTokenNotFound(src, listMember, instance, ("member"));
			}

			return *m->UnderlyingGenericArg1Type();
		}

		const NodeDef* variableDef = ce.Script.GetNodeDef(ce.Builder, nodeName);
		if (variableDef == NULL)
		{
			ThrowTokenNotFound(src, nodeName, ce.Builder.Owner().Name(), ("member"));
		}

		return variableDef->ElementType;
	}

	void AddNodeDef(CScript& script, ICodeBuilder& builder, cstr name, const IStructure& elementType, cr_sex s)
	{
		script.AddNodeDef(builder, name, elementType, s);
	}
}