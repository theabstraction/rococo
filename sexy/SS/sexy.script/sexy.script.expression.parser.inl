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
	size_t GetOffsetTo(csexstr memberName, const IStructure& s)
	{
		size_t offset = 0;
		for(int i = 0; i < s.MemberCount(); i++)
		{
			const IMember& member = s.GetMember(i);
			if (Sexy::AreEqual(member.Name(), SEXTEXT("_allocSize")))
			{
				break;							
			}

			offset += member.SizeOfMember();	
		}

		return offset;
	}

	int GetStackRequirement(const IStructure& str, cr_sex s)
	{
		VARTYPE outputType = str.VarType();
		BITCOUNT bits = GetBitCount(outputType);
		switch(bits)
		{
		case BITCOUNT_32:
			return 4;
			break;
		case BITCOUNT_64:
			return 8;
			break;
		default:
			Throw(s, SEXTEXT("Algorithmic error: unhandled bitcount in argument to function"));
			return 0;
		}
	}

	void PushVariableRef(cr_sex s, ICodeBuilder& builder, const MemberDef& def, csexstr name, int interfaceIndex)
	{
		if (def.AllocSize == 0 || (IsNullType(*def.ResolvedType) && def.IsContained))
		{
			// Pseudo variable
			TokenBuffer refName;
			GetRefName(refName, name);

			MemberDef refDef;
			builder.TryGetVariableByName(OUT refDef, refName);

			if (interfaceIndex == 0 || interfaceIndex == 1)
			{
				builder.PushVariable(IN refDef);
			}
			else
			{
				Throw(s, SEXTEXT("Unexpected interface specified in PushVariableRef(...)"));
			}
		}
		else
		{
			builder.PushVariableRef(name, interfaceIndex);
		}
	}

	int GetCommonInterfaceIndex(const IStructure& object, const IStructure& argType)
	{
		if (&object == &argType) return 0; // 0 means same structures, 1 means interface 0 matches, 2 means interface 1 matches...

		if (argType.InterfaceCount() == 0) return -1; // -1 incompatable types

		const IInterface& argInterf = argType.GetInterface(0);

		for(int i = 0; i < object.InterfaceCount(); ++i)
		{
			const IInterface& objectInterf = object.GetInterface(i);

			if (&objectInterf == &argInterf || (objectInterf.Base() != NULL && objectInterf.Base() == &argInterf))
			{
				return i + 1;
			}
		}

		return -1;
	}

	void AddMember(IStructureBuilder& s, cr_sex field)
	{
		AssertCompound(field);
		AssertNotTooFewElements(field, 2);

		cr_sex argTypeExpr = GetAtomicArg(field, 0);
		sexstring type = argTypeExpr.String();

		if (s.Prototype().IsClass)
		{
			if (AreEqual(type, SEXTEXT("implements")))
			{
				for (int i = 1; i < field.NumberOfElements(); ++i)
				{
					cr_sex nameExpr = field.GetElement(i);
					AssertQualifiedIdentifier(nameExpr);
					s.AddInterface(nameExpr.String()->Buffer);
				}

				return;
			}
			else if (AreEqual(type, SEXTEXT("defines")))
			{
				if (field.NumberOfElements() == 2 && IsAtomic(field[1]))
				{
					s.AddInterface(field[1].String()->Buffer);
				} 
				else if (field.NumberOfElements() == 4 && IsAtomic(field[1]) && IsAtomic(field[2]) && IsAtomic(field[3]) && AreEqual(field[2].String(), SEXTEXT("extends")))
				{
					s.AddInterface(field[1].String()->Buffer);
				}
				else
				{
					Throw(field, SEXTEXT("Expecting (defines <fully-qualified-interface-name>) or (defines <fully-qualified-interface-name> extends <fully-qualified-interface-name>)"));
				}
				return;
			}
		}

		if (AreEqual(type, SEXTEXT("array")))
		{
			cr_sex elementTypeExpr = GetAtomicArg(field, 1);
			sexstring elementType = elementTypeExpr.String();

			AssertTypeIdentifier(elementTypeExpr);

			for(int i = 2; i < field.NumberOfElements(); ++i)
			{
				cr_sex nameExpr = field.GetElement(i);
				AssertLocalIdentifier(nameExpr);			

				s.AddMember(NameString::From(nameExpr.String()), TypeString::From(SEXTEXT("_Array")), elementType->Buffer);
			}	
		}
		else if (AreEqual(type, SEXTEXT("list")))
		{
			cr_sex elementTypeExpr = GetAtomicArg(field, 1);
			sexstring elementType = elementTypeExpr.String();

			AssertTypeIdentifier(elementTypeExpr);

			for(int i = 2; i < field.NumberOfElements(); ++i)
			{
				cr_sex nameExpr = field.GetElement(i);
				AssertLocalIdentifier(nameExpr);			

				s.AddMember(NameString::From(nameExpr.String()), TypeString::From(SEXTEXT("_List")), elementType->Buffer);
			}	
		}
		else if (AreEqual(type, SEXTEXT("map")))
		{
			cr_sex keyTypeExpr = GetAtomicArg(field, 1);
			cr_sex valueTypeExpr = GetAtomicArg(field, 2);
			sexstring keyType = keyTypeExpr.String();
			sexstring valueType = valueTypeExpr.String();

			AssertTypeIdentifier(keyTypeExpr);
			AssertTypeIdentifier(valueTypeExpr);

			for(int i = 3; i < field.NumberOfElements(); ++i)
			{
				cr_sex nameExpr = field.GetElement(i);
				AssertLocalIdentifier(nameExpr);			

				s.AddMember(NameString::From(nameExpr.String()), TypeString::From(SEXTEXT("_Map")), keyType->Buffer, valueType->Buffer);
			}	
		}
		else if (AreEqual(type, SEXTEXT("ref")))
		{
			if (field.NumberOfElements() != 3) Throw(field, SEXTEXT("Expecting (ref <type> <name>)"));

			cr_sex srefType = field.GetElement(1);
			cr_sex sname = field.GetElement(2);

			AssertQualifiedIdentifier(srefType);
			AssertLocalIdentifier(sname);

			s.AddPseudoMember(NameString::From(sname.String()), TypeString::From(srefType.String()));

			IInterfaceBuilder* interf = MatchInterface(srefType, s.Module());
			if (interf != NULL)
			{
				TokenBuffer memberRefName;
				GetRefName(memberRefName, sname.String()->Buffer);
				s.AddMember(NameString::From(memberRefName), TypeString::From(SEXTEXT("Sys.Type.Pointer")));
			}
			else
			{
				Throw(srefType, SEXTEXT("Only interface types can be referenced by a member"));
			}
		}
		else
		{
			AssertQualifiedIdentifier(argTypeExpr);

			for(int i = 1; i < field.NumberOfElements(); ++i)
			{
				cr_sex nameExpr = field.GetElement(i);
				AssertLocalIdentifier(nameExpr);			

				s.AddMember(NameString::From(nameExpr.String()), TypeString::From(type));

				IInterfaceBuilder* interf = MatchInterface(argTypeExpr, s.Module());
				if (interf != NULL)
				{
					TokenBuffer memberRefName;
					GetRefName(memberRefName, nameExpr.String()->Buffer);
					s.AddMember(NameString::From(memberRefName), TypeString::From(SEXTEXT("Sys.Type.Pointer")));
				}
			}	
		}
	}

	csexstr Comparative(int delta) { return delta > 0 ? SEXTEXT("more") : SEXTEXT("fewer"); } 

	void ValidateArchetypeMatchesArchetype(cr_sex s, const IArchetype& f, const IArchetype& archetype, csexstr source)
	{
		int delta = archetype.NumberOfInputs() - f.NumberOfInputs();
		if (delta != 0)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("There are ") << Comparative(delta) << SEXTEXT(" inputs in ") << source << archetype.Name() << SEXTEXT(" than in that of ") << f.Name();
			Throw(s, streamer);
		}

		delta = archetype.NumberOfOutputs() - f.NumberOfOutputs();
		if (delta != 0)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("There are ") << Comparative(delta) << SEXTEXT(" outputs in ") << source << archetype.Name() << SEXTEXT(" than that of ") << f.Name();
			Throw(s, streamer);
		}

		int32 argCount = ArgCount(f);

		for(int32 i = 0; i < argCount; ++i)
		{
			const IStructure& st = archetype.GetArgument(i);
			const IStructure& stf = f.GetArgument(i);
			csexstr argname = f.GetArgName(i);

			if (archetype.IsVirtualMethod())
			{
				if (i == 0 && st.VarType() == VARTYPE_Pointer && AreEqual(archetype.GetArgName(0), SEXTEXT("_typeInfo")))
				{
					if (stf.Prototype().IsClass && AreEqual(THIS_POINTER_TOKEN,argname))
					{
						continue;
					}
				}
				else if (st.VarType() == VARTYPE_Pointer && AreEqual(archetype.GetArgName(i), SEXTEXT("_vTable"), 7))
				{
					if (stf.Prototype().IsClass && AreEqual(THIS_POINTER_TOKEN,argname))
					{
						continue;
					}
				}
			}
			
			if (&stf != &st)
			{
				sexstringstream streamer;
				streamer << source << archetype.Name() << SEXTEXT(": Argument [") << i << "] (" << GetFriendlyName(st) << " " << argname << SEXTEXT("). Type did not match that of the implementation. Expected '") << GetFriendlyName(stf) << SEXTEXT("'");
				Throw(s, streamer);
			}

			const IStructure* interfGenericArg1 = archetype.GetGenericArg1(i);
			const IStructure* concreteGenericArg1 = f.GetGenericArg1(i);

			if (interfGenericArg1 != concreteGenericArg1)
			{
				sexstringstream streamer;

				// Not really expecting the generic args to be NULL, as we should already have bailed out above, but handle the case

				streamer << SEXTEXT("Error validating concrete method against the interface's specification for (") << f.Name() << SEXTEXT("...). ") << std::endl;
				if (archetype.GetGenericArg1(i) != NULL)
				{
					streamer << SEXTEXT("Interface's method with generic argument type '") << GetFriendlyName(*interfGenericArg1) << SEXTEXT("' does not match ");
				}
				else
				{
					streamer << SEXTEXT("Interface's method has no generic argument type and so does not match ");
				}
				
				if (f.GetGenericArg1(i) != NULL)
				{
					streamer << SEXTEXT("concrete generic argument type '") << GetFriendlyName(*concreteGenericArg1) << SEXTEXT("'");
				}
				else
				{
					streamer << SEXTEXT("concrete method with no generic argument type ");
				}

				
				Throw(f.Definition() != NULL ? *(const ISExpression*)(f.Definition()) : s, streamer);
			}
		}
	}

	void CompileAssignmentDirectiveFromAtomic(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, bool explicitKeyword)
	{
		int offset = explicitKeyword ? 0 : -1; // explicit means the directive begins with the assignment keyword, else it begins with the target variable
		cr_sex targetExpr = GetAtomicArg(directive, 1 + offset);
		csexstr targetVariable = targetExpr.String()->Buffer;
		cr_sex assignmentChar = GetAtomicArg(directive, 2 + offset);
		cr_sex sourceValue = directive.GetElement(3 + offset);
		VARTYPE targetType = varStruct.VarType();

		csexstr sourceText = sourceValue.String()->Buffer;

		TokenBuffer symbol;
		StringPrint(symbol, SEXTEXT("%s=%s"), targetVariable, (csexstr) sourceValue.String()->Buffer);
		
		if (targetType == VARTYPE_Closure)
		{
			IFunctionBuilder* f = ce.Builder.Module().FindFunction(sourceText);
			if (f != NULL)
			{
				ValidateArchetypeMatchesArchetype(directive, *f, *varStruct.Archetype(), SEXTEXT("archetype "));

				if (f->Builder().NeedsParentsSF() && ce.Builder.Owner().GetArgumentByName(targetVariable) != NULL)
				{
					Throw(sourceValue, SEXTEXT("Cannot return the closure from the function, as it accesses its parent's stack variables"));
				}

				CodeSection section;
				f->Code().GetCodeSection(OUT section);

				ce.Builder.AddSymbol(symbol);

				VariantValue v;
				v.byteCodeIdValue = section.Id;
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_64);

				TokenBuffer targetBytecodeId;
				StringPrint(targetBytecodeId, SEXTEXT("%s.bytecodeId"), targetVariable);
				ce.Builder.AddSymbol(targetBytecodeId);
				ce.Builder.AssignTempToVariable(0, targetBytecodeId);

				VariantValue parentSF;
				parentSF.charPtrValue = NULL;
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, parentSF, BITCOUNT_POINTER);

				TokenBuffer targetParentSF;
				StringPrint(targetParentSF, SEXTEXT("%s.parentSF"), targetVariable);
				ce.Builder.AddSymbol(targetParentSF);
				ce.Builder.AssignTempToVariable(0, targetParentSF);
				return;
			}
		}

		// Either a literal or an identifier
		
		VARTYPE varType = Parse::GetLiteralType(sourceText);
		if (IsPrimitiveType(varType))
		{
			try
			{
				ce.Builder.AddSymbol(symbol);
				ce.Builder.AssignLiteral(NameString::From(targetVariable), sourceText);
				return;
			}
			catch(IException& stcex)
			{
				Throw(directive, stcex.Message());
			}
		}
		else
		{
			VARTYPE sourceType = ce.Builder.GetVarType(sourceText);
			if (IsPrimitiveType(sourceType))
			{
				if (targetType == sourceType)
				{
					ce.Builder.AddSymbol(symbol);
					ce.Builder.AssignVariableToVariable(sourceText, targetVariable);
					return;
				}
				else
				{
					sexstringstream streamer;
					streamer << SEXTEXT("The type of ") << targetVariable << SEXTEXT(" does not match the type of ") << sourceText;
					Throw(directive, streamer);
				}
			}
			else if (sourceType == VARTYPE_Closure)
			{
				if (targetType == sourceType)
				{
					MemberDef sourceDef, targetDef;
					ce.Builder.TryGetVariableByName(sourceDef, sourceText);
					ce.Builder.TryGetVariableByName(targetDef, targetVariable);

					if (sourceDef.CapturesLocalVariables && !targetDef.CapturesLocalVariables)
					{
						sexstringstream streamer;
						streamer << SEXTEXT("Could not copy ") << sourceText << SEXTEXT(" to ") << targetVariable << SEXTEXT(". The target variable accepts regular function references, but not closures.");
						Throw(directive, streamer);
					}

					ce.Builder.AddSymbol(symbol);
					ce.Builder.AssignVariableToVariable(sourceText, targetVariable);
					return;
				}
				else
				{
					sexstringstream streamer;
					streamer << SEXTEXT("The type of ") << targetVariable << SEXTEXT(" does not match the type of ") << sourceText;
					Throw(directive, streamer);
				}
			}
			else if (sourceType == VARTYPE_Derivative)
			{
				Throw(directive, SEXTEXT("not implemented"));
			}
			else
			{
				if (varStruct.VarType() == VARTYPE_Derivative && !IsNullType(varStruct))
				{
					Throw(directive, SEXTEXT("Only interface references and primitive types can be initialized in an assignment"));
				}				

				if (AreEqual(sourceText, SEXTEXT("GetCurrentExpression")))
				{
					const CClassExpression* express =  ce.SS.GetExpressionReflection(directive);
					VariantValue sexPtr;
					sexPtr.vPtrValue = (void*) &express->Header._vTables[0]; // sexPtr is the interface ptr, not the instance ptr

					TokenBuffer token;
					GetRefName(token, targetVariable);
					ce.Builder.AddSymbol(SEXTEXT("Current Expression"));

					MemberDef def;
					ce.Builder.TryGetVariableByName(OUT def, token);

					ce.Builder.Assembler().Append_SetStackFrameImmediate(def.SFOffset + def.MemberOffset, sexPtr, BITCOUNT_POINTER);
					return;
				}

				if (TryCompileFunctionCallAndReturnValue(ce, sourceValue, targetType, &varStruct, NULL))
				{
					TokenBuffer symbol;
					if (targetType == VARTYPE_Derivative)
					{
						// If a function returns a derivative type then it returns a pointer to a derivative type
						TokenBuffer refTarget;
						GetRefName(refTarget, targetVariable);

						StringPrint(symbol, SEXTEXT("-> %s"), (csexstr) refTarget);
						ce.Builder.AddSymbol(symbol);
						ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH, refTarget);
					}
					else if (targetType == VARTYPE_Closure)
					{	
						StringPrint(symbol, SEXTEXT("-> %s"), (csexstr)targetVariable);
						ce.Builder.AddSymbol(symbol);

						TokenBuffer bytecodeId;
						StringPrint(bytecodeId, SEXTEXT("%s.bytecodeId"), targetVariable);
						ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH, bytecodeId);

						TokenBuffer parentSF;
						StringPrint(parentSF, SEXTEXT("%s.parentSF"), targetVariable);
						ce.Builder.AssignTempToVariable(10, parentSF);
					}
					else
					{						
						StringPrint(symbol, SEXTEXT("-> %s"), (csexstr) targetVariable);
						ce.Builder.AddSymbol(symbol);
						ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH, targetVariable);
					}
				}
				else
				{
					Throw(sourceValue, SEXTEXT("The RHS of the assignment was unrecognized"));
				}
			}
		}
	}

	void CompileAssignmentDirectiveFromCompound(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, bool explicitKeyword)
	{
		int offset = explicitKeyword ? 0 : -1; // explicit means the directive begins with the assignment keyword, else it begins with the target variable
		csexstr targetVariable = GetAtomicArg(directive, 1 + offset).String()->Buffer;
		cr_sex sourceValue = directive.GetElement(3 + offset);
		
		VARTYPE targetType = varStruct.VarType();

		TokenBuffer symbol;
		StringPrint(symbol, SEXTEXT("%s=(...)"), targetVariable);

		bool negate = false;

		switch(targetType)
		{
		case VARTYPE_Bool:
			if (TryCompileBooleanExpression(ce, sourceValue, true, negate))
			{
				ce.Builder.AddSymbol(symbol);
				ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH, targetVariable);
				if (negate) ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D7);
				return;
			}
			break;
		case VARTYPE_Float32:
		case VARTYPE_Int32:
		case VARTYPE_Int64:
		case VARTYPE_Float64:
			if (TryCompileArithmeticExpression(ce, sourceValue, true, targetType))
			{
				ce.Builder.AddSymbol(symbol);
				ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH, targetVariable);
				return;
			}
			break;
		case VARTYPE_Closure:
			if (TryCompileFunctionCallAndReturnValue(ce, sourceValue, targetType, NULL, varStruct.Archetype()))
			{
				TokenBuffer targetId;
				StringPrint(targetId, SEXTEXT("%s.bytecodeId"), targetVariable);

				ce.Builder.AddSymbol(symbol);
				ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH, targetId); 

				TokenBuffer targetSF;
				StringPrint(targetSF, SEXTEXT("%s.parentSF"), targetVariable);

				ce.Builder.AddSymbol(symbol);
				
				// hotfix for 64-bit binaries - id goest to D7 while parentSF goes to D14, an unused register
				ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH + 7, targetSF); 
				return;
			}
			else if (TryCompileClosureDef(ce, sourceValue, *varStruct.Archetype(), ce.Builder.Owner().GetArgumentByName(targetVariable) == NULL))
			{
				ce.Builder.EnableClosures(targetVariable);
				TokenBuffer symbol;
				StringPrint(symbol, SEXTEXT("%s.bytecodeId = (closure ...)"), targetVariable);
				ce.Builder.AddSymbol(symbol);

				TokenBuffer targetVariableByteCodeId;
				StringPrint(targetVariableByteCodeId, SEXTEXT("%s.bytecodeId"), targetVariable);
				ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH, targetVariableByteCodeId);

				TokenBuffer targetVariableParentSF;
				StringPrint(symbol, SEXTEXT("%s.parentSF = SF"), targetVariable);
				ce.Builder.AddSymbol(symbol);
				StringPrint(targetVariableParentSF, SEXTEXT("%s.parentSF"), targetVariable);
				ce.Builder.AssignTempToVariable(-2, targetVariableParentSF);
				return;
			}
			break;
		case VARTYPE_Pointer:
			if (TryCompileFunctionCallAndReturnValue(ce, sourceValue, targetType, NULL, NULL))
			{				
				ce.Builder.AddSymbol(symbol);
				ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH, targetVariable);
				return;
			}			
			break;
		case VARTYPE_Derivative: // Function returns a pointer to a derivative type
			if (TryCompileFunctionCallAndReturnValue(ce, sourceValue, targetType, &varStruct, NULL))
			{				
				TokenBuffer refTarget;
				GetRefName(refTarget, targetVariable);
				ce.Builder.AddSymbol(symbol);
				ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH, refTarget);
				return;
			}			
			break;
		default:
			break;
		}

		Throw(directive, SEXTEXT("Cannot determine RHS of assignment"));
	}

	void CompileAssignAtomicValueToMember(CCompileEnvironment& ce, const IMember& member, cr_sex src, csexstr targetVariable)
	{
		// Either a literal or an identifier
		csexstr sourceText = src.String()->Buffer;

		TokenBuffer symbol;
		StringPrint(symbol, SEXTEXT("%s=%s"), targetVariable, sourceText);

		VARTYPE varType = Parse::GetLiteralType(sourceText);
		if (IsPrimitiveType(varType))
		{
			try
			{
				ce.Builder.AddSymbol(symbol);
				ce.Builder.AssignLiteral(NameString::From(targetVariable), sourceText);
				return;
			}
			catch(IException& stcex)
			{
				Throw(src, stcex.Message());
			}
		}
		else
		{
			VARTYPE sourceType = ce.Builder.GetVarType(sourceText);
			if (IsPrimitiveType(sourceType))
			{
				if (member.UnderlyingType()->VarType() == sourceType)
				{
					AssignVariableToVariable(ce, src, targetVariable, sourceText);
					return;
				}
				else
				{
					sexstringstream streamer;
					streamer << SEXTEXT("The type of ") << targetVariable << SEXTEXT(" does not match the type of ") << sourceText;
					Throw(src, streamer);
				}
			}
			else
			{
				sexstringstream streamer;
				streamer << SEXTEXT("Cannot assign to type of ") << targetVariable << SEXTEXT(". The source is not a primitive type");
				Throw(src, streamer);
			}
		}
	}

	void CompileAssignToStringMember(CCompileEnvironment& ce, csexstr variableName, const IStructure& st, const IMember& member, cr_sex src)
	{
		// Assume member underlying type is null-string
		if (IsStringLiteral(src))
		{
			if (!IsIStringInlined(ce.Script))
			{
				Throw(src, SEXTEXT("Memberwise initialization of an IString member is not allowed unless all IString implementations support string inlining"));
			}

			sexstring valueStr = src.String();
			CStringConstant* sc = CreateStringConstant(ce.Script, valueStr->Length, valueStr->Buffer, &src);

			ce.Builder.Append_InitializeVirtualTable(variableName, ce.Object.Common().TypeStringLiteral());
			AddSymbol(ce.Builder, SEXTEXT("StringConstant %s"), (csexstr) valueStr->Buffer);

			TokenBuffer token;
			StringPrint(token, SEXTEXT("%s.length"), variableName);

			SEXCHAR value[32];
			StringPrint(value, 32, SEXTEXT("%d"), sc->length);
			ce.Builder.AssignLiteral(NameString::From(token), value);

			StringPrint(token, SEXTEXT("%s.buffer"), variableName);
			ce.Builder.AssignPointer(NameString::From(token), sc->pointer);

			TokenBuffer vTableBuffer;
			StringPrint(vTableBuffer, SEXTEXT("%s._vTable1"), variableName);
			ce.Builder.AssignVariableRefToTemp(vTableBuffer, Sexy::ROOT_TEMPDEPTH);

			TokenBuffer refName;
			GetRefName(refName, variableName);
			ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH, refName);
		}
		else
		{
			Throw(src, SEXTEXT("Memberwise initialization of a string member requires the argument be an atomic string literal"));
		}
	}

	void CompileAssignMember(CCompileEnvironment& ce, csexstr variableName, const IStructure& st, const IMember& member, cr_sex src)
	{
		const IStructure& memberType = *member.UnderlyingType();

		switch(memberType.VarType())
		{
		default:
			if (IsAtomic(src))
			{
				CompileAssignAtomicValueToMember(ce, member, src, variableName);
			}
			else
			{
				Throw(src, SEXTEXT("Do not know how to assign the value of the compound expression to the member variable"));
			}
			return;
		case VARTYPE_Derivative:
			{
				if (*member.UnderlyingType() == ce.Object.Common().SysTypeIString().NullObjectType())
				{
					CompileAssignToStringMember(ce, variableName, st, member, src);	
					return;
				}

				if (!IsCompound(src))
				{
					sexstringstream streamer;
					streamer << memberType.Name() << SEXTEXT(" is a derived type, and requires a compound initializer");
					Throw(src, streamer);
					return;
				}

				if (src.NumberOfElements() != memberType.MemberCount())
				{
					sexstringstream streamer;
					streamer << member.Name() << SEXTEXT(" has ") << memberType.MemberCount() << SEXTEXT(" elements. But ") << src.NumberOfElements() << SEXTEXT(" were supplied ");
					Throw(src, streamer);
					return;
				}

				for(int i = 0; i < memberType.MemberCount(); ++i)
				{
					const IMember& subMember = memberType.GetMember(i);

					TokenBuffer memberName;
					StringPrint(memberName, SEXTEXT("%s.%s"), variableName, subMember.Name());

					CompileAssignMember(ce, memberName, memberType, subMember, src.GetElement(i));
				}
				return;
			}
		case VARTYPE_Bad:
			{
				sexstringstream streamer;
				streamer << memberType.Name() << SEXTEXT(" is a bad type, and cannot be initialized");
				Throw(src, streamer);
				return;
			}
		}
	}

	void CompileAssignExpressionDirective(CCompileEnvironment& ce, cr_sex expression, csexstr refName)
	{
		MemberDef def;
		ce.Builder.TryGetVariableByName(OUT def, refName);
		
		VariantValue v;
		v.vPtrValue = (void*) &GetSystem(ce.Script).GetExpressionReflection(expression)->Header._vTables[0];

		TokenBuffer symbol;
		StringPrint(symbol, SEXTEXT("%s = '..."), refName);

		ce.Builder.AddSymbol(symbol);

		if (def.MemberOffset != 0 || def.Usage != ARGUMENTUSAGE_BYVALUE)
		{			
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_POINTER);
			ce.Builder.AssignTempToVariable(0, refName);
		}
		else
		{
			ce.Builder.Assembler().Append_SetStackFrameImmediate(def.SFOffset, v, BITCOUNT_POINTER);
		}
	}

	bool IsPublic(const IMember& member)
	{
		return member.Name()[0] != '_';
	}

	int PublicMemberCount(const IStructure& s)
	{
		int count = 0;
		for(int i = 0; i < s.MemberCount(); ++i)
		{
			const IMember& member = s.GetMember(i);
			if (IsPublic(member))
			{
				count++;
			}
		}

		return count;
	}
	
	void CompileMemberwiseAssignment(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, int offset)
	{
		csexstr targetVariable = GetAtomicArg(directive, 1 + offset).String()->Buffer;
			
		if (PublicMemberCount(varStruct) + 3 + offset != directive.NumberOfElements())
		{
			Throw(directive, SEXTEXT("Expecting one element in the constructor for every member of the structure"));
		}

		int publicCount = 0;

		for(int i = 0; i < varStruct.MemberCount(); ++i)
		{
			const IMember& member = varStruct.GetMember(i);
			TokenBuffer memberName;
			StringPrint(memberName, SEXTEXT("%s.%s"), targetVariable, member.Name());

			if (IsPublic(member))
			{				
				CompileAssignMember(ce, memberName, varStruct, member, directive.GetElement(3 + offset + publicCount));
				publicCount++;
			}
		}
	}

	void CompileAssignmentDirective(CCompileEnvironment& ce, cr_sex directive, const IStructure& varStruct, bool explicitKeyword)
	{
		int offset = explicitKeyword ? 0 : -1; // explicit means the directive begins with the assignment keyword, else it begins with the target variable
		AssertNotTooFewElements(directive, 4 + offset);
	
		if (varStruct.VarType() == VARTYPE_Derivative && !varStruct.Prototype().IsClass)
		{
			CompileMemberwiseAssignment(ce, directive, varStruct, offset);
			return;
		}
			
		AssertNotTooManyElements(directive, 4 + offset);
		cr_sex sourceValue = directive.GetElement(3 + offset);

		if (IsAtomic(sourceValue))
		{
			CompileAssignmentDirectiveFromAtomic(ce, directive, varStruct, explicitKeyword);
		}
		else if (IsCompound(sourceValue))
		{
			CompileAssignmentDirectiveFromCompound(ce, directive, varStruct, explicitKeyword);
		}
		else
		{		
			sexstringstream streamer;
			csexstr targetVariable = GetAtomicArg(directive, 1 + offset).String()->Buffer;
			streamer << SEXTEXT("Bad expression on RHS of assignment: ") << targetVariable;
			Throw(directive, streamer);
		}
	}

	void ValidateUnusedVariable(cr_sex identifierExpr, ICodeBuilder& builder)
	{
		csexstr id = identifierExpr.String()->Buffer;
		if (builder.GetVarType(id) != VARTYPE_Bad)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Variable name ") << id << SEXTEXT(" is already defined in the context");
			Throw(identifierExpr, streamer);
		}
	}	

	int CompileThisToInstancePointerArg(CCompileEnvironment& ce, cr_sex s, csexstr classInstance, const IInterface& interfaceRef)
	{
		MemberDef refDef;
		ce.Builder.TryGetVariableByName(OUT refDef, classInstance);

		AddArgVariable(SEXTEXT("instance"), ce, ce.Object.Common().TypePointer());

		if (refDef.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			ce.Builder.Assembler().Append_PushStackFrameAddress(refDef.SFOffset);		
			return (int) sizeof(void*);			
		}
		else
		{			
			int offset = ce.Builder.GetThisOffset();
			if (offset == 0)
			{
				ce.Builder.Assembler().Append_PushStackVariable(refDef.SFOffset, BITCOUNT_POINTER);
			}
			else
			{
				ce.Builder.Assembler().Append_GetStackFrameMemberPtr(VM::REGISTER_D4, refDef.SFOffset, ce.Builder.GetThisOffset());
				ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);
			}
			
			return (int) sizeof(void*);			
		}				
	}

	IFunction& GetConstructor(IModuleBuilder& module, cr_sex typeExpr)
	{
		sexstring type = typeExpr.String();

		TokenBuffer qualifiedConstructorName;
		StringPrint(qualifiedConstructorName, SEXTEXT("%s.%s"), type->Buffer, SEXTEXT("Construct"));
		IFunction* constructor = module.FindFunction(qualifiedConstructorName);
		if (constructor == NULL)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Cannot find constructor in source module: ") << qualifiedConstructorName;
			Throw(typeExpr, streamer);
		}
		return *constructor;
	}

	void AddVariableAndSymbol(CCompileEnvironment& ce, csexstr type, csexstr name)
	{
		SEXCHAR declText[256];
		StringPrint(declText, 256, SEXTEXT("%s %s"), type, name);
		ce.Builder.AddSymbol(declText);

		AddVariable(ce, NameString::From(name), TypeString::From(type));
	}

	void InitClassMember(CCompileEnvironment& ce, int sfMemberOffset, const IStructure& memberType)
	{
		VariantValue classDesc;
		classDesc.vPtrValue = (void*) memberType.GetVirtualTable(0);
		ce.Builder.Assembler().Append_SetStackFrameImmediate(sfMemberOffset, classDesc, BITCOUNT_POINTER);

		VariantValue allocSize;
		allocSize.int32Value = memberType.SizeOfStruct();
		ce.Builder.Assembler().Append_SetStackFrameImmediate(sfMemberOffset + sizeof(size_t), allocSize, BITCOUNT_32);

		int interfaceOffset = sizeof(size_t) + sizeof(int32);
		for(int i = 0; i < memberType.InterfaceCount(); ++i)
		{
			VariantValue interfVTable;
			interfVTable.vPtrValue = (void*) memberType.GetVirtualTable(1 + i);
			ce.Builder.Assembler().Append_SetStackFrameImmediate(sfMemberOffset + interfaceOffset, interfVTable, BITCOUNT_POINTER);	
			interfaceOffset += sizeof(void*);
		}
	}

	void InitSubmembers(CCompileEnvironment& ce, const IStructure& s, int sfMemberOffset)
	{
		for(int i = 0; i < s.MemberCount(); ++i)
		{
			const IMember& member = s.GetMember(i);
			const IStructure* memberType = member.UnderlyingType();
			if (memberType)
			{
				if (memberType->Prototype().IsClass)
				{
					if (!member.IsPseudoVariable()) InitClassMember(ce, sfMemberOffset, *memberType);					
					// Whenever we have a null member we have a null member ref prefixed with '_ref_'

					if (IsNullType(*memberType))
					{
						TokenBuffer reftoken;
						GetRefName(reftoken, member.Name());

						int refOffset = 0;
						const IMember* refMember = FindMember(s, reftoken, REF refOffset);
						if (!member.IsPseudoVariable())
						{
							ce.Builder.Assembler().Append_GetStackFrameAddress(VM::REGISTER_D4, sfMemberOffset + sizeof(size_t) + sizeof(int32)); // &interface1
							ce.Builder.Assembler().Append_SetStackFrameValue(refOffset, VM::REGISTER_D4, BITCOUNT_POINTER); // _ref_instance = &interface1
						}
						else
						{
							VariantValue v;
							v.vPtrValue = memberType->GetInterface(0).UniversalNullInstance();
							ce.Builder.Assembler().Append_SetStackFrameImmediate(refOffset, v, BITCOUNT_POINTER);
						}
					}
					else
					{
						InitSubmembers(ce, *memberType, sfMemberOffset);
					}
				}
				else
				{
					InitSubmembers(ce, *memberType, sfMemberOffset);
				}
			}

			sfMemberOffset += member.SizeOfMember();
		}
	}

	bool HasClassMembers(const IStructure& s)
	{
		if (IsPrimitiveType(s.VarType())) return false;

		for(int i = 0; i < s.MemberCount(); ++i)
		{
			const IMember& member = s.GetMember(i);
			const IStructure* memberType = member.UnderlyingType();
			
			if (memberType->Prototype().IsClass)
			{
				return true;
			}

			if (HasClassMembers(*memberType))
			{
				return true;
			}
		}

		return false;
	}

	void InitDefaultReferences(cr_sex s, CCompileEnvironment& ce, csexstr id, const IStructure& st)
	{
		if (st.VarType() == VARTYPE_Closure)
		{
			MemberDef def;
			if (!ce.Builder.TryGetVariableByName(OUT def, id) || def.Usage != ARGUMENTUSAGE_BYVALUE)
			{
				Throw(s, SEXTEXT("Cannot compile closure. Unhandled syntax"));
			}

			IFunctionBuilder& nullFunction = GetNullFunction(ce.Script, *st.Archetype());

			CodeSection section;
			nullFunction.Builder().GetCodeSection(section);

			TokenBuffer idcode, idoffset;
			StringPrint(idcode, SEXTEXT("%s.bytecodeId"), id);
			StringPrint(idoffset, SEXTEXT("%s.parentSF"), id);

			VariantValue codeRef;
			codeRef.byteCodeIdValue = section.Id;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, codeRef, BITCOUNT_64);
			ce.Builder.AssignTempToVariable(0, idcode);

			VariantValue zeroRef;
			zeroRef.charPtrValue = 0;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, zeroRef, BITCOUNT_64);
			ce.Builder.AssignTempToVariable(0, idoffset);
		}
	}

	void InitClassMembers(CCompileEnvironment& ce, csexstr id)
	{
		MemberDef def;
		ce.Builder.TryGetVariableByName(OUT def, id);

		if (def.Usage == ARGUMENTUSAGE_BYVALUE)
		{
			InitSubmembers(ce, *def.ResolvedType, def.SFOffset + def.MemberOffset);
		}
		else if (HasClassMembers(*def.ResolvedType))
		{
			ce.Builder.Assembler().Append_GetStackFrameValue(def.SFOffset, VM::REGISTER_D5, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_SwapRegister(VM::REGISTER_D5, VM::REGISTER_SF);

			InitSubmembers(ce, *def.ResolvedType, def.MemberOffset);

			ce.Builder.Assembler().Append_SwapRegister(VM::REGISTER_D5, VM::REGISTER_SF);
		}
	}

	void CompileConstructFromConcreteConstructor(CCompileEnvironment& ce, const IStructure& type, csexstr id, cr_sex args)
	{
		// (Class instance1 (<arg1>...<argN>)))
		const IFunction* constructor = type.Constructor();
		if (constructor == NULL)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Needs a constructor function ") << type.Name() << SEXTEXT(".Construct inside ") << type.Module().Name(); 
			Throw(*args.Parent(), streamer);
		}

		AddSymbol(ce.Builder, SEXTEXT("%s %s (...)"), type.Name(), id); 
		AddVariable(ce, NameString::From(id), type);
	
		int inputCount = constructor->NumberOfInputs();

		int mapIndex = GetIndexOf(1, args, SEXTEXT("->"));
		if (mapIndex > 0) Throw(args, SEXTEXT("Mapping token are not allowed in constructor calls, which have no output"));
		if (args.NumberOfElements() < inputCount - 1) Throw(args, SEXTEXT("Too few arguments to constructor call"));
		if (args.NumberOfElements() > inputCount - 1) Throw(args, SEXTEXT("Too many arguments to constructor call"));

		int inputStackAllocCount = PushInputs(ce, args, *constructor, true, 0);		
		inputStackAllocCount += CompileInstancePointerArg(ce, id);

		AppendFunctionCallAssembly(ce, *constructor); 

		ce.Builder.MarkExpression(args.Parent());

		RepairStack(ce, *args.Parent(), *constructor);
		ce.Builder.AssignClosureParentSF();
	}

	void CompileContructorCall(CCompileEnvironment& ce, const IStructure& type, csexstr id, cr_sex constructorArgs)
	{
		if (IsAtomic(constructorArgs) || IsStringLiteral(constructorArgs))
		{
			Throw(*constructorArgs.Parent(), SEXTEXT("Unrecognized variable initialization syntax. Expecting null or compound constructor args"));
		}

		if (type.VarType() != VARTYPE_Derivative)
		{
			Throw(*constructorArgs.Parent(), SEXTEXT("Constructor declarations are only available for derivative types"));
		}

		if (IsNullType(type))
		{
			// Expecting factory invocation of form (<interface> <id> (<factory> <factarg1> ... <factargN>))
			CompileConstructFromFactory(ce, type, id, constructorArgs);
			return;
		}

		// Expecting concrete invocation of form (<class> <id> (<arg1> ... <argN>))
		CompileConstructFromConcreteConstructor(ce, type, id, constructorArgs);
	}

	IModule& GetSysTypeMemoModule(CScript& script)
	{
		return GetModule(script).Object().GetModule(0);
	}

	void CompileCreateStringConstant(CCompileEnvironment& ce, csexstr id, cr_sex decl, cr_sex value)
	{
		sexstring valueStr = value.String();

		CStringConstant* sc = CreateStringConstant(ce.Script, valueStr->Length, valueStr->Buffer, &value);

		AddSymbol(ce.Builder, SEXTEXT("StringConstant %s"), id);

		TokenBuffer stringRef;
		GetRefName(stringRef, id);
		AddVariable(ce, NameString::From(stringRef), ce.Object.Common().TypePointer());

		const IInterface& interf = ce.Object.Common().SysTypeIString();
		ce.Builder.AddPseudoVariable(NameString::From(id), interf.NullObjectType());
				
		MemberDef ptrDef;
		ce.Builder.TryGetVariableByName(OUT ptrDef, stringRef);
				
		csexstr format = (value.String()->Length > 24) ? SEXTEXT(" = '%.24s...'") : SEXTEXT(" = '%s'");
		AddSymbol(ce.Builder, format, (csexstr) value.String()->Buffer);
		
		VariantValue ptr;
		ptr.vPtrValue = (void*) &sc->header._vTables[0];
		ce.Builder.Assembler().Append_SetStackFrameImmediate(ptrDef.SFOffset, ptr, BITCOUNT_POINTER);		
	}

	bool TryCompileAsStringConstantAssignment(CCompileEnvironment& ce, const IStructure& type, csexstr id, cr_sex decl)
	{
		if (ce.Object.Common().SysTypeIString().NullObjectType() != type)  return false;
		if (decl.NumberOfElements() != 4) return false;	
		
		cr_sex value = decl.GetElement(3);
		if (value.Type() != EXPRESSION_TYPE_STRING_LITERAL) return false;		
		
		CompileCreateStringConstant(ce, id, decl, value);

		return true;
	}

	bool IsDeclarationAssignment(cr_sex s)
	{
		if (s.NumberOfElements() == 4)
		{
			cr_sex assignCharExpr = s.GetElement(2);
			if (AreEqual(assignCharExpr.String(), SEXTEXT("=")))
			{
				return true;
			}
		}

		return false;
	}

	void AddPointerToInterface(CCompileEnvironment& ce, csexstr id, const IStructure& nullType)
	{
		TokenBuffer declText;
		StringPrint(declText, SEXTEXT("%s* %s"), GetFriendlyName(nullType), id);

		TokenBuffer refName;
		GetRefName(refName, id);
		ce.Builder.AddSymbol(declText);

		AddVariable(ce, NameString::From(refName), ce.Object.Common().TypePointer());
		ce.Builder.AddPseudoVariable(NameString::From(id), nullType);	
	}

	void CompileAsExpressionArg(CCompileEnvironment& ce, const IStructure& type, csexstr id, cr_sex decl)
	{
		if (type.InterfaceCount() == 0 || &type.GetInterface(0) != &ce.Object.Common().SysTypeIExpression())
		{
			Throw(decl, SEXTEXT("Expecting tyoe Sys.Reflection.IExpression, the only known variable declarations with\r\n")
						SEXTEXT("5 elements are expressions defintions of the format (Sys.Type.IExpression s = ' (<s_def>))")); 
		}

		cr_sex expression = decl.GetElement(4);

		AddPointerToInterface(ce, id, type);

		TokenBuffer refName;
		GetRefName(refName, id);
		CompileAssignExpressionDirective(ce, expression, refName);
	}

	void CompileAsNodeValueAssign(CCompileEnvironment& ce, const IStructure& type, csexstr id, cr_sex decl)
	{
		// (type id = & <node-name>)
		cr_sex nodeNameExpr = decl.GetElement(4);
		if (!IsAtomic(nodeNameExpr)) Throw(nodeNameExpr, SEXTEXT("Expecting (type id = & <node-name>). The <node-name> element needs to be atomic"));

		csexstr nodeName = nodeNameExpr.String()->Buffer;
		ce.Builder.AssignVariableRefToTemp(nodeName, Sexy::ROOT_TEMPDEPTH); // The node pointer is now in D7

		const IStructure* nodeType = ce.Builder.GetVarStructure(nodeName);
		if (*nodeType == ce.Object.Common().TypeNode())
		{
			const IStructure& elementType = GetNodeDef(ce, decl, nodeName);
			if (elementType != type)
			{
				ThrowTypeMismatch(decl, elementType, type, SEXTEXT("The node element type did not match the declaration type"));
			}
			
			AppendInvoke(ce, GetListCallbacks(ce).NodeGetElementRef, decl); // The element ref is now in D7
		}
		else if (*nodeType == ce.Object.Common().TypeMapNode())
		{
			const MapNodeDef& def = GetMapNodeDef(ce, decl, nodeName);
			if (def.mapdef.ValueType != type)
			{
				ThrowTypeMismatch(decl, def.mapdef.ValueType, type, SEXTEXT("The node element type did not match the declaration type"));
			}

			AppendInvoke(ce, GetMapCallbacks(ce).MapNodeGetRef, decl); // The element ref is now in D7
		}
		else
		{
			Throw(decl, SEXTEXT("The name did not match a known node type"));
		}
		
		AddVariableRef(ce, NameString::From(id), type);
		AssignTempToVariableRef(ce, Sexy::ROOT_TEMPDEPTH, id); // The id variable is now pointing to the element
	}

	void AssertDefaultConstruction(CCompileEnvironment& ce, cr_sex decl, const IStructure& s)
	{
		if (IsPrimitiveType(s.VarType())) return;

		if (s == ce.StructArray())
		{
			Throw(decl, SEXTEXT("Objects of the given type cannot be default constructed as one of the members is an array.\r\nProvide an explicit constructor and declare instances using the syntax: ( <type> <name> ( <arg1>...<argN> ) )"));
		}
		else if (s == ce.StructList())
		{
			Throw(decl, SEXTEXT("Objects of the given type cannot be default constructed as one of the members is a linked list.\r\nProvide an explicit constructor and declare instances using the syntax: ( <type> <name> ( <arg1>...<argN> ) )"));
		}
		else if (s == ce.StructMap())
		{
			Throw(decl, SEXTEXT("Objects of the given type cannot be default constructed as one of the members is a map.\r\nProvide an explicit constructor and declare instances using the syntax: ( <type> <name> ( <arg1>...<argN> ) )"));
		}

		for(int i = 0; i < s.MemberCount(); ++i)
		{
			const IMember& m = s.GetMember(i);
			AssertDefaultConstruction(ce, decl, *m.UnderlyingType());
		}
	}

	void CompileClassAsDefaultVariableDeclaration(CCompileEnvironment& ce, const IStructure& st, csexstr id, cr_sex decl)
	{	
		TokenBuffer refName;
		GetRefName(refName, id);

		ce.Builder.AddSymbol(refName);
		AddVariable(ce, NameString::From(refName), ce.Object.Common().TypePointer());

		AddSymbol(ce.Builder, SEXTEXT("%s %s"), GetFriendlyName(st), id);
		AssertDefaultConstruction(ce, decl, st);
		AddVariable(ce, NameString::From(id), st);

		ce.Builder.Append_InitializeVirtualTable(id);
		ce.Builder.AssignVariableRefToTemp(id, 0);
		ce.Builder.AssignTempToVariable(0, refName);

		InitClassMembers(ce, id);
	}

	void CompileAsDefaultVariableDeclaration(CCompileEnvironment& ce, const IStructure& st, csexstr id, cr_sex decl)
	{	
		if (st.Prototype().IsClass)
		{
			 CompileClassAsDefaultVariableDeclaration(ce, st, id, decl);
		}
		else
		{
			AddSymbol(ce.Builder, SEXTEXT("%s %s"), GetFriendlyName(st), id);
			AssertDefaultConstruction(ce, decl, st);
			AddVariable(ce, NameString::From(id), st);
			InitClassMembers(ce, id);
			InitDefaultReferences(decl, ce, id, st);
		}
	}

	void CompileAsVariableDeclarationWithAssignment(CCompileEnvironment& ce, const IStructure& type, csexstr id, cr_sex decl)
	{
		// (<type> <id> = <...>)


		if (type.Prototype().IsClass)
		{	
			if (TryCompileAsStringConstantAssignment(ce, type, id, decl))
			{
				return;
			}

			if (IsDeclarationAssignment(decl))
			{
				AddPointerToInterface(ce, id, type);
				CompileAssignmentDirective(ce, decl, type, true);
				return;
			}
		}		
		
		CompileAsDefaultVariableDeclaration(ce, type, id, decl);
		CompileAssignmentDirective(ce, decl, type, true);
	}

	bool IsStructOrArchetypeOrInterface(cr_sex typeExpr, IModuleBuilder& module)
	{
		IStructure* s = MatchStructure(typeExpr, module);
		if (s == NULL)
		{
			// Failed to find a structure with the given name, so look for a closure
			IArchetype* a = MatchArchetype(typeExpr, module);
			if (a == NULL)
			{
				return false;
			}
			else
			{
				s = module.Object().IntrinsicModule().FindStructure(SEXTEXT("__Closure"));
				if (s == NULL)
				{
					Throw(typeExpr, SEXTEXT("Error could not find __Closure intrinsic"));
				}
			}
		}

		return true;
	}

	void CompileAsVariableDeclarationAndInit(CCompileEnvironment& ce, const IStructure& type, csexstr id, cr_sex decl)
	{		
		if (decl.NumberOfElements() == 3)
		{
			cr_sex constructorArgs = decl.GetElement(2);			
			CompileContructorCall(ce, type, id, constructorArgs);
		}
		else
		{
			// (type id = (constructor_args)) used in member by member construction
			AssertAtomicMatch(decl.GetElement(2), SEXTEXT("="));
							
			if (decl.NumberOfElements() == 5)
			{			
				if (IsAtomicMatch(decl.GetElement(3), SEXTEXT("'")))
				{
					CompileAsExpressionArg(ce, type, id, decl);
					return;
				}
				else if (IsAtomicMatch(decl.GetElement(3), SEXTEXT("&")))
				{
					CompileAsNodeValueAssign(ce, type, id, decl);
					return;
				}
			}
					
			// (type id = value)
			CompileAsVariableDeclarationWithAssignment(ce, type, id, decl);	
		}
	}

	void ValidateLocalDeclarationVariable(const IStructure& st, cr_sex idExpr)
	{
		if (!IsAtomic(idExpr))
		{
			Throw(idExpr, SEXTEXT("Expecting a local identifier variable name in this position"));
		}

		AssertLocalIdentifier(idExpr);
	}

	void CompileAsVariableDeclaration(CCompileEnvironment& ce, cr_sex decl)
	{
		AssertCompound(decl);

		cr_sex typeExpr = decl.GetElement(0);
		cr_sex idExpr = decl.GetElement(1);

		IModuleBuilder& source = ce.Builder.Module();
		int nElements = decl.NumberOfElements();

		const IStructure* st = MatchStructure(typeExpr, source);
		if (st == NULL)
		{
			const IInterface* interf = MatchInterface(typeExpr, source);
			if (interf != NULL)
			{
				st = &interf->NullObjectType();
			}
		}

		if (st == NULL)
		{
			ThrowTokenNotFound(decl, typeExpr.String()->Buffer, source.Name(), SEXTEXT("type"));
		}

		ValidateLocalDeclarationVariable(*st, idExpr);
		csexstr id = idExpr.String()->Buffer;
		if (nElements == 2)
		{
			CompileAsDefaultVariableDeclaration(ce, *st, id, decl);
		}
		else
		{
			CompileAsVariableDeclarationAndInit(ce, *st, id, decl);
		}
	}

	bool IsAssignment(cr_sex s)
	{
		if (s.NumberOfElements() != 3)	
		{
			return false;
		}

		if (!IsAtomic(s.GetElement(0)))
		{
			return false;
		}

		cr_sex operatorExpr = s.GetElement(1);
		if (!IsAtomic(operatorExpr))
		{
			return false;
		}

		csexstr op = operatorExpr.String()->Buffer;
		if (!AreEqual(op, SEXTEXT("=")))
		{
			return false;
		}

		return true;
	}

	void AssertGetVariable(OUT MemberDef& def, csexstr name, CCompileEnvironment& ce, cr_sex exceptionSource)
	{
		AssertLocalIdentifier(exceptionSource);

		if (!ce.Builder.TryGetVariableByName(def, name))
		{
			NamespaceSplitter splitter(name);

			csexstr instance, member;
			if (splitter.SplitTail(instance, member))
			{
				if (ce.Builder.TryGetVariableByName(def, instance))
				{
					ThrowTokenNotFound(exceptionSource, member, instance, SEXTEXT("member"));
				}
			}
			
			ThrowTokenNotFound(exceptionSource, name, ce.Builder.Owner().Name(), SEXTEXT("variable"));
		}
	}

	// Returns true iff structure is Plain Old Data.
	bool IsPLOD(const IStructure& type)
	{
		VARTYPE vt = type.VarType();
		if (vt == VARTYPE_Pointer)
		{
			// Pointers represent native data
			return false;
		}

		if (IsPrimitiveType(vt))
			return true;

		if (type.Name()[0] == '_')
		{
			// System types are not PLOD
			return false;
		}

		TokenBuffer destructorName;
		StringPrint(destructorName, SEXTEXT("%s.Destruct"), type.Name());
		
		if (type.Module().FindFunction(destructorName) != NULL)
		{
			// Destructors are generally used to tidy up native resources, hence indicate we are not PLODs
			return false;
		}

		for(int i = 0; i < type.MemberCount(); ++i)
		{
			const IMember& m = type.GetMember(i);
			if (!IsPLOD(*m.UnderlyingType()))
			{
				return false;
			}			
		}

		return true;
	}

	void CompileAsStructureAssignmentFromCompound(CCompileEnvironment& ce, const IStructure& varStruct, csexstr varName, cr_sex value)
	{
		if (value.NumberOfElements() == 1)
		{
			CompileAsStructureAssignmentFromCompound(ce, varStruct, varName, value.GetElement(0));
			return;
		}


		if (value.NumberOfElements() == 2)
		{
			cr_sex command = GetAtomicArg(value, 0);
			csexstr commandText = command.String()->Buffer;

			cr_sex arg = value.GetElement(1);

			if (islower(commandText[0]))
			{
				MemberDef def;
				AssertGetVariable(OUT def, commandText, ce, command);

				if (*def.ResolvedType == ce.StructArray())
				{					
					const IStructure& elementType = GetArrayDef(ce, value, commandText);
					const IStructure* varType = ce.Builder.GetVarStructure(varName);

					if (varType == NULL)
					{
						Throw(*value.Parent(), SEXTEXT("Cannot determine structure type on LHS of assignment"));
					}

					if (*varType != elementType)
					{
						ThrowTypeMismatch(*value.Parent(), *varType, elementType, SEXTEXT("Array type does not match assignment target type"));
					}

					if (!IsPLOD(elementType))
					{
						Throw(value, SEXTEXT("The array element type is not plain data"));
					}

					if (!TryCompileArithmeticExpression(ce, arg, true, VARTYPE_Int32))
					{
						Throw(command, SEXTEXT("Expecting Int32 valued expression for array index"));
					} // D7 now contains the array index

					ce.Builder.AssignVariableRefToTemp(commandText, 0); // D4 contains the array ptr
					ce.Builder.AssignVariableRefToTemp(varName, 1); // D5 contains the structure ptr

					ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayGetByRef);
				}
				else
				{
					Throw(command, SEXTEXT("Expecting array name"));
				}
			}
			else
			{
				Throw(command, SEXTEXT("Expecting variable name"));
			}
		}
	}

	void CompileAsStructureAssignment(CCompileEnvironment& ce, const IStructure& varStruct, cr_sex directive)
	{
		csexstr varName = directive.GetElement(0).String()->Buffer;
		cr_sex rhs = directive.GetElement(2);

		switch(rhs.Type())
		{
		case EXPRESSION_TYPE_COMPOUND:
			 CompileAsStructureAssignmentFromCompound(ce, varStruct, varName, rhs);
			 break;
		default:
			Throw(directive, SEXTEXT("Non-compound structure assignment is not yet implemented"));
		}
	}

	bool TryCompileAsImplicitSetDirective(CCompileEnvironment& ce, cr_sex directive)
	{
		if (!IsAssignment(directive))
		{
			return false;
		}

		cr_sex keywordExpr = GetAtomicArg(directive, 0);
		csexstr token = keywordExpr.String()->Buffer;

		MemberDef def;
		if (ce.Builder.TryGetVariableByName(OUT def, token))
		{
			const IStructure* varStruct = def.ResolvedType;
			VARTYPE tokenType = varStruct->VarType();

			if (IsPrimitiveType(tokenType) || tokenType == VARTYPE_Closure)
			{
				CompileAssignmentDirective(ce, directive, *varStruct, false);
				return true;
			}
			else if (tokenType == VARTYPE_Derivative)
			{
				if (varStruct->Prototype().IsClass)
				{
					if (!TryCompileAsLateFactoryCall(ce, def, directive))
					{
						Throw(directive, SEXTEXT("Cannot interpret RHS as late factory call"));
					}

					return true;
				}
				else
				{
					CompileAsStructureAssignment(ce, *varStruct, directive);
					return true;
				}
			}
		}

		return false;
	}

	const ISExpression* GetTryCatchExpression(CScript& script);

	void NoteDestructorPositions(CCompileEnvironment& ce, const IStructure& structureToDelete, int baseSFoffset)
	{
		if (RequiresDestruction(structureToDelete))
		{
			ce.Builder.NoteDestructorPosition(baseSFoffset, structureToDelete);
		}
	}
	
	void MarkStackRollback(CCompileEnvironment& ce, cr_sex invokeExpression)
	{
		const ISExpression* tryCatchBlock = GetTryCatchExpression(ce.Script);

		int stackCorrection = 0;

		int nTempVariables = ce.Builder.GetVariableCount() - ArgCount(ce.Builder.Owner());

		for(int i = 0; i < nTempVariables; i++)
		{
			size_t lastIndex = ce.Builder.GetVariableCount() - i - 1;
			MemberDef def;
			csexstr name;
			ce.Builder.GetVariableByIndex(OUT def, OUT name, (int32) lastIndex);

			if (def.AllocSize != 0 && tryCatchBlock != NULL && def.Userdata != tryCatchBlock)
			{
				// We mark the rollback position to the tryCatchBlock
				break;
			}

			stackCorrection += def.AllocSize;			
		}

		ce.Builder.NoteStackCorrection(stackCorrection);

		for(int i = 0; i < nTempVariables; i++)
		{
			size_t lastIndex = ce.Builder.GetVariableCount() - i - 1;
			MemberDef def;
			csexstr name;
			ce.Builder.GetVariableByIndex(OUT def, OUT name, (int32) lastIndex);

			if (def.AllocSize != 0 && tryCatchBlock != NULL && def.Userdata != tryCatchBlock)
			{
				break;
			}

			if (def.Usage == ARGUMENTUSAGE_BYVALUE && def.location != VARLOCATION_NONE)
			{
				NoteDestructorPositions(ce, *def.ResolvedType, def.SFOffset);
			}
			else if (def.Usage == ARGUMENTUSAGE_BYREFERENCE && (*def.ResolvedType == ce.Object.Common().TypeNode() || *def.ResolvedType == ce.Object.Common().TypeMapNode()))
			{
				NoteDestructorPositions(ce, *def.ResolvedType, def.SFOffset);
			}
		}
	}

	void CompileConstructInterfaceCall(CCompileEnvironment& ce, const IFunction& constructor, const MemberDef& refDef, csexstr instanceName, const IInterface& refCast, const IStructure& classType, cr_sex s)
	{
		// (construct class-name arg1 arg2 ... argN)
		ce.Builder.Append_InitializeVirtualTable(instanceName, classType);

		int inputCount = constructor.NumberOfInputs() - 1;
		int nRequiredElements = 2 + inputCount;

		int mapIndex = GetIndexOf(1, s, SEXTEXT("->"));
		if (mapIndex > 0) Throw(s, SEXTEXT("Mapping token are not allowed in constructor calls, which have no output"));
		if (s.NumberOfElements() < nRequiredElements) Throw(s, SEXTEXT("Too few arguments to constructor call"));
		if (s.NumberOfElements() > nRequiredElements) Throw(s, SEXTEXT("Too many arguments to constructor call"));
				
		int inputStackAllocCount = PushInputs(ce, s, constructor, true, 2);	
		
		ce.Builder.AddSymbol(instanceName);
		inputStackAllocCount += CompileThisToInstancePointerArg(ce, s, instanceName, refCast);			

		AppendFunctionCallAssembly(ce, constructor);
		RepairStack(ce, s, constructor);
	
		if (AreEqual(SEXTEXT("factory"), GetContext(ce.Script)))
		{
			// 'construct' terminates a factory call and puts the interface pointer into D4
			int interfaceIndex = GetInterfaceIndex(refCast, classType);
			int interfaceOffset = GetInterfaceOffset(interfaceIndex);

			ce.Builder.AssignVariableRefToTemp(instanceName, 0, interfaceOffset); // The instance ref is now in D4
		}
		
		ce.Builder.AssignClosureParentSF();
	}

	void CompileConstructInterfaceCall(CCompileEnvironment& ce, cr_sex s)
	{
		// (construct class-name arg1 arg2 ... argN)
		AssertNotTooFewElements(s, 2);

		cr_sex classNameExpr = GetAtomicArg(s, 1);
		sexstring className = classNameExpr.String();
		
		const IStructure& classType = GetClass(classNameExpr, ce.Script);
		
		MemberDef thisDef;
		const IStructure& thisType = GetThisInterfaceRefDef(OUT thisDef,  ce.Builder, s);		
		const IInterface& thisInterf = thisType.GetInterface(0);
		const IFunction& constructor = GetConstructor(classType, classNameExpr);

		for(int j = 0; j < classType.InterfaceCount(); j++)
		{
			const IInterface& targetInterf = thisType.GetInterface(j);
			if (&thisInterf == &targetInterf)
			{				
				CompileConstructInterfaceCall(REF ce, IN constructor, IN thisDef, SEXTEXT("this"), IN thisInterf, IN classType, IN s);
				return;
			}
		}

		Throw(s, SEXTEXT("The class does not implement the interface associated with the local variable"));
	}

	void CompileInterfaceCast(CCompileEnvironment& ce, IStructure& toType, cr_sex toNameExpr, cr_sex fromNameExpr)
	{
		sexstring toName = toNameExpr.String();
		sexstring fromName = fromNameExpr.String();
		AddVariableRef(ce, NameString::From(toName->Buffer), toType);

		IFunction& fnDynCast = GetFunctionByFQN(ce, *toNameExpr.Parent(), SEXTEXT("Sys.Native._DynamicCast"));

		const IInterface& castToInterf = toType.GetInterface(0);
		csexstr castToInterfName = castToInterf.Name();

		VariantValue v;
		v.vPtrValue = (void*) &castToInterf;
		ce.Builder.AddSymbol(castToInterfName);
		ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_POINTER, v);
		ce.Builder.PushVariableRef(fromName->Buffer, 0);
		ce.Builder.AddSymbol(SEXTEXT("_DynamicCast to D4"));

		AddArgVariable(SEXTEXT("cast_to_interface"), ce, ce.Object.Common().TypePointer());
		AddArgVariable(SEXTEXT("cast_from_ref"), ce, ce.Object.Common().TypePointer());

		AppendFunctionCallAssembly(ce, fnDynCast);
		MarkStackRollback(ce, *toNameExpr.Parent());
		ce.Builder.AddSymbol(toName->Buffer);

		MemberDef nameDef;
		ce.Builder.TryGetVariableByName(nameDef, toName->Buffer);
		ce.Builder.Assembler().Append_SetStackFrameValue(nameDef.SFOffset, VM::REGISTER_D4, BITCOUNT_POINTER);

		ce.Builder.PopLastVariables(2);
	}

	void CompileCast(CCompileEnvironment& ce, cr_sex s)
	{
		// (cast <from_variable> -> <to_type> <to_variable> )
		AssertNotTooFewElements(s, 5);
		AssertNotTooManyElements(s, 5);

		cr_sex toTypeExpr = GetAtomicArg(s, 3);
		cr_sex toName = GetAtomicArg(s, 4);
		cr_sex fromName = GetAtomicArg(s, 1);
		cr_sex mapToken = GetAtomicArg(s, 2);

		if (!AreEqual(mapToken.String(), SEXTEXT("->")))
		{
			Throw(s, SEXTEXT("Expecting syntax: (cast <from_variable> -> <to_type> <to_variable> )"));
		}

		AssertQualifiedIdentifier(toTypeExpr);
		AssertLocalIdentifier(toName);
		AssertLocalIdentifier(fromName);

		IStructure* toType = MatchStructure(toTypeExpr, ce.Builder.Module());
		if (toType == NULL)
		{
			Throw(toTypeExpr, SEXTEXT("Unknown target type for cast"));
		}

		if (IsNullType(*toType))
		{
			CompileInterfaceCast(ce, *toType, toName, fromName);
		}
		else
		{
			Throw(toTypeExpr, SEXTEXT("Only interfaces can be the target type of a cast"));
		}
	}

	void AppendDeconstructAll(CCompileEnvironment& ce, cr_sex sequence);

	void CompileTrip(CCompileEnvironment& ce, cr_sex s)
	{
		ce.Builder.Assembler().Append_TripDebugger();
	}

	void CompileReturnFromFunction(CCompileEnvironment& ce, cr_sex s)
	{
		AppendDeconstructAll(ce, s);
		ce.Builder.Assembler().Append_Return();
	}

	void CompleAsNodeDeclaration(CCompileEnvironment& ce, cr_sex s)
	{
		// (node n = <source>)
		AssertNotTooFewElements(s, 4);
		AssertNotTooManyElements(s, 4);

		AssertAtomicMatch(s.GetElement(2), SEXTEXT("="));

		cr_sex nodeNameExpr = GetAtomicArg(s, 1);
		csexstr nodeName = nodeNameExpr.String()->Buffer;
		AssertLocalIdentifier(nodeNameExpr);

		cr_sex source = s.GetElement(3);
		if (IsAtomic(source))
		{
			CompileAsListNodeDeclaration(ce, nodeName, source);
		}
		else if (IsCompound(source))
		{
			CompileAsMapNodeDeclaration(ce, nodeName, source);
		}
		else
		{
			Throw(s, SEXTEXT("Expecting atomic or compound expression as the final item in the node definition"));
		}
	}

	void CompileAsYield(CCompileEnvironment& ce, cr_sex s)
	{
		// (yield)
		AssertNotTooManyElements(s, 1);
		ce.Builder.Assembler().Append_Yield();
	}

	void CompileAsGlobalAccess(CCompileEnvironment& ce, cr_sex s)
	{
		// Either (global <varname> -> <target_variable>)
		// or 
		// (global <varname> <- <source_variable>)

		if (s.NumberOfElements() != 4 || !IsAtomic(s[1]) && !IsAtomic(s[2]) || !IsAtomic(s[3]))
		{
			Throw(s, SEXTEXT("Expecting (global <varname> -> <target_variable>) or (global <varname> <- <source_variable>)"));
		}

		sexstring globalName = s[1].String();
		sexstring localVarName = s[3].String();
		sexstring operation = s[2].String();

		GlobalValue* g = GetGlobalValue(ce.Script, globalName->Buffer);
		if (g == nullptr)
		{
			Throw(s[1], SEXTEXT("Unrecognized global variable name"));
		}

		VariantValue literalValue;
		if (Parse::PARSERESULT_GOOD == Parse::TryParse(literalValue, g->type, localVarName->Buffer))
		{
			if (!AreEqual(operation, SEXTEXT("<-")))
			{
				Throw(s[3], SEXTEXT("Expecting <- when assigning a literal expression to a global variable"));
			}

			ce.Builder.AssignLiteralToGlobal(*g, literalValue);
			return;
		}

		MemberDef def;
		if (!ce.Builder.TryGetVariableByName(def, localVarName->Buffer))
		{
			Throw(s[3], SEXTEXT("Expecting local variable name"));
		}

		if (def.ResolvedType->VarType() != g->type)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("The global variable type ") << GetTypeName(g->type) << SEXTEXT(" does not match the local variable type ") << def.ResolvedType->Name() << std::ends;
			Throw(s, streamer);
		}

		if (AreEqual(operation, SEXTEXT("->")))
		{
			// assign local value
			ce.Builder.AssignVariableFromGlobal(*g, def);
		}
		else if (AreEqual(operation, SEXTEXT("<-")))
		{
			// assign global value
			ce.Builder.AssignVariableToGlobal(*g, def);
		}
		else
		{
			Throw(s[2], SEXTEXT("Expecting <- or ->"));
		}
	}

	bool TryCompileAsKeyword(CCompileEnvironment& ce, sexstring token, cr_sex s)
	{
		if (AreEqual(token, SEXTEXT("if")))
		{
			CompileIfThenElse(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("cast")))
		{
			CompileCast(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("while")))
		{
			CompileWhileLoop(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("break")))
		{
			CompileBreak(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("continue")))
		{
			CompileContinue(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("do")))
		{
			CompileDoWhile(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("foreach")))
		{
			CompileForEach(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("throw")))
		{
			CompileThrow(ce, s);			
			return true;
		}
		else if (AreEqual(token, SEXTEXT("construct")))
		{
			CompileConstructInterfaceCall(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("trip")))
		{
			CompileTrip(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("try")))
		{
			CompileExceptionBlock(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("return")))
		{
			CompileReturnFromFunction(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("array")))
		{
			CompileArrayDeclaration(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("list")))
		{
			CompileListDeclaration(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("map")))
		{
			CompileMapDeclaration(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("node")))
		{
			CompleAsNodeDeclaration(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("yield")))
		{
			CompileAsYield(ce, s);
			return true;
		}
		else if (AreEqual(token, SEXTEXT("global")))
		{
			CompileAsGlobalAccess(ce, s);
			return true;
		}
		else
		{
			return false;
		}
	}

	void ReturnVariableInterface(CCompileEnvironment& ce, cr_sex exceptionSource, csexstr sourceName, csexstr outputName, const MemberDef& output)
	{
		MemberDef def;
		if (!ce.Builder.TryGetVariableByName(OUT def, sourceName))
		{
			ThrowTokenNotFound(exceptionSource, sourceName, ce.Builder.Owner().Name(), SEXTEXT("structure")); 
		}

		const IInterface& outputInterface = output.ResolvedType->GetInterface(0);

		const IStructure& src = *def.ResolvedType;
		if (src.InterfaceCount() == 0)
		{
			sexstringstream streamer;
			streamer <<  SEXTEXT("The source type '") << GetFriendlyName(src) << SEXTEXT("' implements no interfaces");
			Throw(exceptionSource, streamer);
		}

		if (def.location != Compiler::VARLOCATION_INPUT)
		{
			sexstringstream streamer;
			streamer <<  SEXTEXT("Only inputs can be used as the source for an interface to an output");
			Throw(exceptionSource, streamer);
		}

		if (!IsNullType(src))
		{
			for(int i = 0; i < src.InterfaceCount(); ++i)
			{
				if (outputInterface == src.GetInterface(i))
				{
					int instanceToInterfaceOffset = sizeof(ObjectStub) + ((i - 1) * sizeof(void*));
					ce.Builder.Assembler().Append_GetStackFrameMemberPtr(VM::REGISTER_D4, def.SFOffset, def.MemberOffset + instanceToInterfaceOffset);
					ce.Builder.Assembler().Append_SetStackFrameValue(output.SFOffset, VM::REGISTER_D4, BITCOUNT_POINTER);
					return;
				}
			}

			sexstringstream streamer;
			streamer <<  SEXTEXT("The RHS type '") << GetFriendlyName(src) << SEXTEXT("' does not implement interface ") << outputInterface.Name();
			Throw(exceptionSource, streamer);
		}
		else
		{
			ce.Builder.AssignVariableToVariable(sourceName, outputName);
		}
	}

	void AssignVariableToVariable(CCompileEnvironment& ce, cr_sex exceptionSource, csexstr lhs, csexstr rhs)
	{
		try
		{
			MemberDef def;
			if (ce.Builder.TryGetVariableByName(OUT def, lhs))
			{
				if (def.location == Compiler::VARLOCATION_OUTPUT)
				{
					if (def.Usage == Compiler::ARGUMENTUSAGE_BYREFERENCE)
					{
						if (IsNullType(*def.ResolvedType))
						{
							ReturnVariableInterface(ce, exceptionSource, rhs, lhs, def);
							return;
						}
					}
				}
			}

			SEXCHAR symbol[256];
			StringPrint(symbol, 256, SEXTEXT("%s = %s"), lhs, rhs);

			NamespaceSplitter splitter(rhs);

			csexstr member = rhs;

			csexstr body, tail;
			if (splitter.SplitHead(body, tail))
			{
				csexstr mappedToken = ce.MapMethodToMember(tail);
				if (mappedToken != NULL)
				{
					NamespaceSplitter subSplitter(body);

					csexstr instance, subInstance;
					if (!subSplitter.SplitTail(instance, subInstance))
					{
						subInstance = body;
					}

					const IStructure* st = ce.Builder.GetVarStructure(subInstance);
					if (st == NULL)
					{
						ThrowTokenNotFound(exceptionSource, subInstance, ce.Builder.Owner().Name(), SEXTEXT("structure")); 
					}

					if (st == &ce.StructArray())
					{
						TokenBuffer mappedSymbol;
						StringPrint(mappedSymbol, SEXTEXT("%s.%s"), body, mappedToken);
						ce.Builder.AddSymbol(symbol);
						ce.Builder.AssignVariableToVariable(mappedSymbol, lhs);
						return;
					}
				}
				else if (AreEqual(tail, SEXTEXT("PopOut")))
				{
					VARTYPE type = ce.Builder.GetVarType(lhs);
					if (!IsPrimitiveType(type))
					{
						Throw(exceptionSource, SEXTEXT("Only primitive types can be popped out from an array"));
					}
					CompileAsPopOutFromArray(ce, exceptionSource, body, ce.Builder.GetVarType(lhs));
					ce.Builder.AssignTempToVariable(Sexy::ROOT_TEMPDEPTH, lhs);
					return;
				}
			}

			ce.Builder.AddSymbol(symbol);
			ce.Builder.AssignVariableToVariable(rhs, lhs);
		}
		catch (IException& e)
		{
			Throw(exceptionSource, e.Message());
		}		
	}

	void CompileTrivialAssignment(CCompileEnvironment& ce, cr_sex s, sexstring lhs, sexstring rhs)
	{
		AssignVariableToVariable(ce, s, lhs->Buffer, rhs->Buffer);	
	}

	void CompileTrivialStringAssign(CCompileEnvironment& ce, cr_sex s, sexstring lhs, sexstring rhs)
	{		
		MemberDef def;
		if (!ce.Builder.TryGetVariableByName(OUT def, lhs->Buffer))
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Cannot find ") << (csexstr) lhs->Buffer << SEXTEXT(" in this context") << std::ends;
			Throw(s, streamer.str().c_str());
		}

		if (def.Usage != ARGUMENTUSAGE_BYREFERENCE)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Cannot assign a string to ") << (csexstr) lhs->Buffer << SEXTEXT(": the variable was a value type");
			Throw(s, streamer);
		}

		if (def.SFOffset >= 0)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Cannot assign a string to the temporary variable ") << (csexstr) lhs->Buffer;
			Throw(s, streamer);
		}

		const IInterface& istring = ce.Object.Common().SysTypeIString();
		if (def.ResolvedType != &istring.NullObjectType())
		{
			sexstringstream streamer;
			streamer << SEXTEXT("Cannot assign a string to anything other than a Sys.Type.IString reference") << (csexstr) lhs->Buffer;
			Throw(s, streamer);
		}

		if (ce.Builder.Owner().GetArgumentByName(lhs->Buffer)->Direction() != ARGDIRECTION_OUTPUT)
		{
			sexstringstream streamer;
			streamer << SEXTEXT("The Sys.Type.IString was not an output reference: ") << (csexstr) lhs->Buffer;
			Throw(s, streamer);
		}

		CStringConstant* sc = CreateStringConstant(ce.Script, rhs->Length, rhs->Buffer, &s);

		SEXCHAR debugInfo[256];
		csexstr format = (rhs->Length > 24) ? SEXTEXT(" = '%.24s...'") : SEXTEXT(" = '%s'");
		StringPrint(debugInfo, 256, format, (csexstr) rhs->Buffer);
		ce.Builder.AddSymbol(debugInfo);

		VariantValue ptr;
		ptr.vPtrValue = (void*) &sc->header._vTables[0];		

		ce.Builder.Assembler().Append_SetStackFrameImmediate(def.SFOffset, ptr, BITCOUNT_POINTER);
	}

	bool IsValidVariableName(csexstr token)
	{
		NamespaceSplitter splitter(token);

		csexstr instance, member;
		if (splitter.SplitTail(instance, member))
		{
			return IsLowerCase(member[0]);
		}
		else
		{
			return IsLowerCase(token[0]);
		}
	}

	bool TryCompileAsTrivialAssignment(CCompileEnvironment& ce, cr_sex s)
	{
		int nElements = s.NumberOfElements();
		if (nElements == 3)
		{
			cr_sex lhs = s.GetElement(0);
			cr_sex middle = s.GetElement(1);
			cr_sex rhs = s.GetElement(2);

			if (IsAtomic(lhs) && IsAtomic(middle) && (IsAtomic(rhs) || IsStringLiteral(rhs)) )
			{
				sexstring lhsToken = lhs.String();
				sexstring rhsToken = rhs.String();
				sexstring middleToken = middle.String();

				if (AreEqual(middleToken, SEXTEXT("=")))
				{
					if (IsStringLiteral(rhs))
					{
						CompileTrivialStringAssign(ce, s, lhsToken, rhsToken);
						return true;
					}

					if (IsValidVariableName(lhsToken->Buffer) && IsValidVariableName(rhsToken->Buffer))
					{
						if (AreEqual(rhsToken, SEXTEXT("true")) || AreEqual(rhsToken, SEXTEXT("false")))
						{
							return false;
						}

						MemberDef targetDef;
						if (!ce.Builder.TryGetVariableByName(targetDef, lhsToken->Buffer))
						{
							Throw(lhs, SEXTEXT("Unrecognized variable name"));
						}

						if (targetDef.CapturesLocalVariables)
						{
							Throw(lhs, SEXTEXT("The target variable refers to a closure. It is immutable."));
						}

						CompileTrivialAssignment(ce, s, lhsToken, rhsToken);
						return true;
					}
				}
			}
		}

		return false;
	}

	bool TryCompileMacroInvocation(CCompileEnvironment& ce, cr_sex s, sexstring token)
	{
		if (token->Buffer[0] != '#')
		{
			return false;
		}

		csexstr macroName = token->Buffer+1;

		NamespaceSplitter splitter(macroName);	

		const IFunction *f;

		csexstr nsBody, fname;
		if (splitter.SplitTail(OUT nsBody, OUT fname))
		{
			INamespace& ns = AssertGetNamespace(ce.Builder.Module().Object(), s, nsBody);

			const IMacro* macro = ns.FindMacro(fname);
			if (macro == NULL) ThrowTokenNotFound(s, fname, ns.FullName()->Buffer, SEXTEXT("macro"));		
			f = &macro->Implementation();
		}
		else
		{
			f = ce.Builder.Module().FindFunction(macroName);
			if (f == NULL)
			{
				INamespace* firstPrefix = NULL;

				for(int i = 0; i < ce.Builder.Module().PrefixCount(); ++i)
				{
					INamespace& prefix = ce.Builder.Module().GetPrefix(i);
					const IMacro* macro = prefix.FindMacro(macroName);
					if (macro != NULL)
					{
						if (f == NULL)
						{
							f = &macro->Implementation();
							firstPrefix = &prefix;
						}
						else
						{
							ThrowNamespaceConflict(s, *firstPrefix, prefix, SEXTEXT("macro"), macroName);
						}
					}
				}
			}

			if (f == NULL)
			{
				ThrowTokenNotFound(s, macroName, ce.Builder.Module().Name(), SEXTEXT("macro"));
			}
		}

		CallMacro(ce, *f, s);

		return true;
	}

	void CompileCommand(CCompileEnvironment& ce, cr_sex s, sexstring token)
	{
		if (TryCompileMacroInvocation(ce, s, token))
		{
			const ISExpression* t = s.GetTransform();
			if (t != NULL)
			{
				CompileExpression(ce, *t);
			}

			return;
		}

		if (token->Buffer[0] == SEXTEXT('\''))
		{
			// Data expression, skip
			return;
		}

		if (IsCapital(token->Buffer[0]))
		{
			if (TryCompileAsPlainFunctionCall(ce, s))
			{
				return;
			}
			else if (s.NumberOfElements() >= 2)
			{
				CompileAsVariableDeclaration(ce, s);	
				return;
			}
			else
			{
				Throw(s, SEXTEXT("Expression was not recognized as a valid function call, and has too few elements to be a variable declaration"));
			}
		}
		else
		{
			if (TryCompileAsTrivialAssignment(ce, s))
				return;

			if (TryCompileAsImplicitSetDirective(ce, s))
				return;		

			if (TryCompileAsDerivativeFunctionCall(ce, s))
				return;		

			if (TryCompileAsKeyword(ce, token, s))
				return;
		}
		
		sexstringstream streamer;

		const IStructure* varStruct = ce.Builder.GetVarStructure(token->Buffer);
		if (NULL == varStruct)
		{
			streamer << SEXTEXT("Unrecognized keyword/variable/function/namespace/syntax in expression: ") << token->Buffer;
		}
		else
		{
			streamer << SEXTEXT("Variable recognized, but the syntax in which it is used was not: ") << GetFriendlyName(*varStruct) << SEXTEXT(" ") << token->Buffer;
		}
		
		Throw(s, streamer);
	}

	void StreamSTCEX(sexstringstream& streamer, const STCException& ex)
	{
		streamer << SEXTEXT("Compiler exception code: ") << ex.Code() << SEXTEXT(".") << std::endl << SEXTEXT("Source: ") << ex.Source() << std::endl << SEXTEXT(". Message: ") << ex.Message() << std::ends;
	}

	void CompileExpression(CCompileEnvironment& ce, cr_sex s)
	{
		ce.Builder.MarkExpression(&s);

		try
		{
			AssertCompound(s);

			if (s.NumberOfElements() == 0) return;

			cr_sex arg = s.GetElement(0);
			if (IsAtomic(arg))
			{
				sexstring command = arg.String();
				CompileCommand(ce, s, command);
			}
			else
			{
				CompileExpressionSequence(ce, 0, s.NumberOfElements()-1, s);
			}
		}
		catch(STCException& ex)
		{
			sexstringstream streamer;
			StreamSTCEX(streamer, ex);
			Throw(s, streamer.str().c_str());
		}
	}
} // Anonymous