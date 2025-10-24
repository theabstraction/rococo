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

namespace Rococo
{
	namespace Script
	{
		int FindIndexOfMatchingAtomic(int startIndex, cr_sex s, cstr token);
		void PostClosure(cr_sex s, IFunctionBuilder& closure, CScript& script);

		template<class T>
		void EnumerateInputs(const IArchetype& callee, bool isImplicitInput, bool reverse, T lambda)
		{
			int virtualIndex = isImplicitInput ? 1 : 0;
			int inputCount = callee.NumberOfInputs() - virtualIndex;

			if (reverse)
			{
				for (int i = inputCount - 1; i >= 0; --i)
				{
					int argIndex = i + callee.NumberOfOutputs();

					cstr inputName = callee.GetArgName(argIndex);
					const IStructure& argType = callee.GetArgument(argIndex);
					const IArchetype* archetype;

					if (argType.VarType() != SexyVarType_Closure)
					{
						archetype = NULL;
					}
					else
					{
						archetype = argType.Archetype();
					}

					lambda(inputName, argType, archetype, i);
				}
			}
			else
			{
				for (int i = 0; i < inputCount; ++i)
				{
					int argIndex = i + callee.NumberOfOutputs();

					cstr inputName = callee.GetArgName(argIndex);
					const IStructure& argType = callee.GetArgument(argIndex);
					const IArchetype* archetype;

					if (argType.VarType() != SexyVarType_Closure)
					{
						archetype = NULL;
					}
					else
					{
						archetype = argType.Archetype();
					}

					lambda(inputName, argType, archetype, i);
				}
			}
		}

		int PushInputs(CCompileEnvironment& ce, cr_sex s, const IArchetype& callee, bool isImplicitInput, int firstArgIndex)
		{
			int inputStackAllocCount = 0;

			int virtualIndex = isImplicitInput ? 1 : 0;

			int inputCount = callee.NumberOfInputs() - virtualIndex;

			if (inputCount > 0)
			{
				if (inputCount + firstArgIndex - 1 >= s.NumberOfElements())
				{
					Throw(s, "Insufficient inputs");
				}
			}

			for (int i = 0; i < inputCount; ++i)
			{
				cr_sex inputExpression = s.GetElement(i + firstArgIndex);

				int argIndex = i + callee.NumberOfOutputs();

				cstr inputName = callee.GetArgName(argIndex);
				const IStructure& argType = callee.GetArgument(argIndex);
				const IArchetype* archetype;

				if (argType.VarType() != SexyVarType_Closure)
				{
					archetype = NULL;
				}
				else
				{
					archetype = argType.Archetype();
				}

				cstr defaultValue = callee.GetDefaultValue(argIndex);

				int inputStackCost = PushInput(ce, s, i + firstArgIndex, argType, archetype, inputName, callee.GetGenericArg1(argIndex), defaultValue);

				inputStackAllocCount += inputStackCost;
			}
			return inputStackAllocCount;
		}

		int GetMapIndex(cr_sex s, int firstIndex, int outputCount, int inputExpressionCount)
		{
			int mapIndex = GetIndexOf(firstIndex, s, ("->"));
			if (mapIndex < 0)
			{
				// We have a function call with no outputs, so the number of args must match the number of inputs
				if (s.NumberOfElements() - firstIndex < inputExpressionCount) Throw(s, "Too few inputs to function call");
				mapIndex = s.NumberOfElements();
			}
			else
			{
				if (mapIndex < inputExpressionCount + 1) Throw(s, "Too few inputs in function call");
				else if (mapIndex > inputExpressionCount + 1) Throw(s, "Too many inputs in function call");
			}

			return mapIndex;
		}

		int GetOutputSFOffset(CCompileEnvironment& ce, int inputStackAllocCount, int outputStackAllocCount)
		{
			int outputOffset = 0;
			int nTempVariables = ce.Builder.GetVariableCount() - ArgCount(ce.Builder.Owner());
			if (nTempVariables > 0)
			{
				MemberDef lastDef;
				cstr lastName;
				ce.Builder.GetVariableByIndex(OUT lastDef, lastName, ce.Builder.GetVariableCount() - 1);

				outputOffset = lastDef.SFOffset + lastDef.AllocSize;
			}

			outputOffset += (outputStackAllocCount);
			return outputOffset;
		}

		void ValidateInputCount(cr_sex s, int inputCount)
		{
			int nExtraElements = IsCompound(s) ? s.NumberOfElements() - 1 : 0;

			if (inputCount < nExtraElements)
			{
				Throw(s, "Too many input arguments supplied to the function");
			}
			else if (inputCount > nExtraElements)
			{
				Throw(s, "Too few input arguments supplied to the function");
			}
		}

		// We assume that s has been checked to be a correct sequence of tokens in a lambda. (<archetype-input-args> ->  <archetype-output-args> : (body-directives))"
		// We do NOT assume that any tokens are correct, other than :, => and ->
		void CompileParsedLambda(CCompileEnvironment& ce, cr_sex lambdaDef, const IStructure& inputType, const IArchetype& archetype, bool mayUseParentSF, int bodyIndicatorIndex)
		{
			// (<archetype-input-args> -> <archetype-output-args> : <body-directives> -> )

			IFunctionBuilder& lambda = ce.Builder.Module().DeclareClosure(ce.Builder.Owner(), mayUseParentSF, &lambdaDef);

			if (archetype.NumberOfOutputs() > 0)
			{
				int mapIndex = archetype.NumberOfInputs();

				for (int i = 0; i < archetype.NumberOfOutputs(); i++)
				{
					cr_sex outputExpr = lambdaDef.GetElement(i + mapIndex + 1);
					const IStructure& neededType = archetype.GetArgument(i);
					cstr argName = archetype.GetArgName(i);

					if (!IsAtomic(outputExpr))
					{
						Throw(outputExpr, "Expecting atomic value for output %d(%s %s) of %s", i, neededType.Name(), argName, archetype.Name());
					}

					AssertLocalIdentifier(outputExpr);

					cstr name = outputExpr.c_str();
					
					// Note that in the lambda output names are derived from the lambda itself, rather than the archetype arguments
					lambda.AddOutput(NameString::From(argName), neededType, (void*)&outputExpr);
				}
			}

			for (int i = 0; i < archetype.NumberOfInputs(); ++i)
			{
				cr_sex inputExpr = lambdaDef.GetElement(i);

				int inputIndex = i + archetype.NumberOfOutputs();
				const IStructure& neededType = archetype.GetArgument(inputIndex);
				cstr argName = archetype.GetArgName(inputIndex);

				AssertLocalIdentifier(inputExpr);

				if (!IsAtomic(inputExpr))
				{
					Throw(inputExpr, "Expecting atomic value for input %d(%s %s) of %s", i, neededType.Name(), argName, archetype.Name());
				}

				// Note that in the lambda input names are derived from the lambda itself, rather than the archetype arguments
				lambda.AddInput(NameString::From(inputExpr.c_str()), neededType, (void*)&inputExpr);
			}

			if (!lambda.TryResolveArguments())
			{
				try
				{
					// We expect ValidateArguments to fail, but rather than return false it throws an informative exception
					lambda.ValidateArguments();
				}
				catch (IException& ex)
				{
					// Re-throw specifying the lambdaDef as the source code
					Throw(lambdaDef, "%s", ex.Message());
				}

				// We expected validate arguments to fail, but strangely it didn't so throw an opaque error exception
				Throw(lambdaDef, "Could not resolve all of the function arguments");
			}

			CodeSection section;
			lambda.Builder().GetCodeSection(OUT section);

			char targetVariable[MAX_FQ_NAME_LEN];
			SafeFormat(targetVariable, "lambda_%llu", ce.Builder.NextId());

			AddVariable(ce, NameString::From(targetVariable), inputType);				
			ce.Builder.EnableClosures(targetVariable);

			AddSymbol(ce.Builder, "%s.bytecodeId = (closure ...)", targetVariable);

			TokenBuffer targetVariableByteCodeId;
			StringPrint(targetVariableByteCodeId, "%s.bytecodeId", targetVariable);

			VariantValue byteCodeId;
			byteCodeId.byteCodeIdValue = section.Id;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, byteCodeId, BITCOUNT_64);
			ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetVariableByteCodeId);

			AddSymbol(ce.Builder, "%s.parentSF = SF", targetVariable);

			TokenBuffer targetVariableParentSF;
			StringPrint(targetVariableParentSF, "%s.parentSF", targetVariable);
			ce.Builder.AssignTempToVariable(-2, targetVariableParentSF);
			
			MemberDef def;
			if (!ce.Builder.TryGetVariableByName(OUT def, targetVariable) || def.ResolvedType->Archetype() != &archetype)
			{
				Throw(lambdaDef, "Failed to interpret expression as a lambda argument: %s", inputType.Name());
			}

			PostClosure(lambdaDef, lambda, ce.Script);

			ce.Builder.PushLambdaVar(targetVariable);
		}

		void CompileLambda(CCompileEnvironment& ce, cr_sex s, const IStructure& inputType, const IArchetype& archetype, cstr argName)
		{
			if (s.NumberOfElements() < 1)
			{
				Throw(s, "Expecting lambda for %s to have at least one elements.\n(<archetype-input-args> -> <archetype-output-args> : (body-directives)", archetype.Name());
			}

			int bodyIndicatorIndex = FindIndexOfMatchingAtomic(0, s, ":");
			if (bodyIndicatorIndex == -1)
			{
				Throw(s, "Expecting body indicator colon ':' in the lambda expression for %s.\n(<archetype-input-args> -> <archetype-output-args> : (body-directives)", archetype.Name());
			}

			// Form of lambda: (: <archetype-input-args> => (body-directives) -> <archetype-output-args>)
			
			if (s.NumberOfElements() < archetype.NumberOfInputs() + 1)
			{
				Throw(s, "The archetype %s takes %d inputs, but the expression was not long enough to specify the inputs", archetype.Name(), archetype.NumberOfInputs());
			}

			if (archetype.NumberOfOutputs() > 0)
			{
				if (s.NumberOfElements() < archetype.NumberOfInputs() + 2 + archetype.NumberOfOutputs())
				{
					Throw(s, "Expecting %d element%s for the output of %s following the mapping operator '->' that succeeds the body-directives.\n(<archetype-input-args> -> <archetype-output-args> : (body-directives)", archetype.NumberOfOutputs(), archetype.NumberOfOutputs() > 1 ? "s" : "", archetype.Name());
				}

				// (<inputs> -> <outputs> : <body>)
				auto& sMapIndicator = s[archetype.NumberOfInputs()];
				if (!IsAtomic(sMapIndicator) || !AreEqual(sMapIndicator.String(), "->"))
				{
					Throw(sMapIndicator, "Expecting mapping operator ->");
				}
			}

			CompileParsedLambda(ce, s, inputType, archetype, argName, bodyIndicatorIndex);
		}

		void CompilePushLambda(CCompileEnvironment& ce, cr_sex s, const IStructure& inputType, const IArchetype& archetype, cstr argName)
		{
			// We should have a lambda variable on the lambda variable stack ready for consumption
			char lambdaName[MAX_FQ_NAME_LEN];
			ce.Builder.PopLambdaVar(OUT lambdaName);

			MemberDef def;
			if (!ce.Builder.TryGetVariableByName(OUT def, lambdaName) || def.ResolvedType->Archetype() != &archetype)
			{
				Throw(s, "Failed to interpret expression as a lambda argument: %s %s", inputType.Name(), lambdaName);
			}

			AddArgVariable("lambda", ce, inputType);
			AddSymbol(ce.Builder, "%s", lambdaName);

			ce.Builder.PushVariable(def);
		}

		void CompilePushClosure(CCompileEnvironment& ce, cr_sex s, const IStructure& inputType, const IArchetype& archetype, cstr argName)
		{
			if (IsCompound(s))
			{
				CompilePushLambda(ce, s, inputType, archetype, argName);
				return;
			}

			AssertAtomic(s);

			cstr fname = s.c_str();

			AddArgVariable("input_other", ce, inputType);
			ce.Builder.AddSymbol(argName);

			IFunction* f = ce.Builder.Module().FindFunction(fname);
			if (f != NULL)
			{
				ValidateArchetypeMatchesArchetype(s, *f, archetype, "archetype");

				CodeSection section;
				f->Code().GetCodeSection(OUT section);

				VariantValue fnRef;
				fnRef.byteCodeIdValue = section.Id;
				ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_64, fnRef);

				VariantValue zeroRef;
				zeroRef.charPtrValue = nullptr;
				ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_64, zeroRef);
			}
			else if (AreEqual("0", fname))
			{
				auto& nf = GetNullFunction(ce.Script, archetype);

				CodeSection section;
				nf.Code().GetCodeSection(OUT section);

				VariantValue fnRef;
				fnRef.byteCodeIdValue = section.Id;
				ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_64, fnRef);

				VariantValue zeroRef;
				zeroRef.charPtrValue = nullptr;
				ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_64, zeroRef);
			}
			else
			{
				MemberDef def;
				if (!ce.Builder.TryGetVariableByName(OUT def, fname) || def.ResolvedType->Archetype() != &archetype)
				{
					Throw(s, "Failed to interpret expression as a closure argument: %s %s", inputType.Name(), argName);
				}

				ce.Builder.PushVariable(def);
			}
		}

		void CompileInputRefFromFunctionRef(CCompileEnvironment& ce, cr_sex inputExpression, const IStructure& argStruct)
		{
			const IFunction* f = MatchFunction(inputExpression, ce.Builder.Module());

			if (f == NULL)
			{
				cstr name = inputExpression.c_str();
				Throw(inputExpression, "The input '%s' was not recognized ", name);
			}

			AddArgVariable(("input_function_ref"), ce, argStruct);

			CodeSection cs;
			f->Code().GetCodeSection(cs);

			VariantValue fnRef;
			fnRef.byteCodeIdValue = cs.Id;

			ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_64, fnRef);

			VariantValue zeroRef;
			zeroRef.charPtrValue = nullptr;
			ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_64, zeroRef);
		}

		void CompileInputRefFromGetAccessor(CCompileEnvironment& ce, cr_sex inputExpression, const IStructure& argStruct)
		{
			if (!TryCompileFunctionCallAndReturnValue(ce, inputExpression, SexyVarType_Derivative, &argStruct, NULL))
			{
				cstr name = inputExpression.c_str();
				Throw(inputExpression, "The input '%s' was not recognized. Expecting a reference to an object of type %s", name, GetFriendlyName(argStruct));
			}

			AddArgVariable("input_getaccessor_return_ref", ce, argStruct);
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4 + ROOT_TEMPDEPTH, BITCOUNT_POINTER);
		}

		bool TryCompileStringLiteralInput(CCompileEnvironment& ce, cr_sex s, bool expectingStructRef, const IStructure& inputType)
		{
			cstr inputTypeName = inputType.Name();
			if (!IsStringLiteral(s) || !AreEqual(inputTypeName, ("_Null_Sys_Type_IString")))
			{
				return false;
			}

			const sexstring value = s.String();

			CStringConstant* sc = CreateStringConstant(ce.Script, value->Length, value->Buffer, &s);

			char debugInfo[256];
			cstr format = (value->Length > 24) ? (" = '%.24s...'") : (" = '%s'");
			int len = SafeFormat(debugInfo, 256, format, (cstr)value->Buffer);

			for (int i = 0; i < len; i++)
			{
				if (debugInfo[i] < 32)
				{
					debugInfo[i] = '?';
				}
			}

			ce.Builder.AddSymbol(debugInfo);

			VariantValue ptr;
			ptr.vPtrValue = (void*)&sc->header.pVTables[0];

			AddArgVariable("input_string_literal", ce, inputType);
			ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_POINTER, ptr);

			return true;
		}

		bool TryCompileStringLiteralInputToTemp(CCompileEnvironment& ce, cr_sex s, int tempDepth, const IStructure& inputType)
		{
			if (!IsStringLiteral(s) || inputType != ce.Object.Common().SysTypeIString().NullObjectType())
			{
				return false;
			}

			const sexstring value = s.String();

			CStringConstant* sc = CreateStringConstant(ce.Script, value->Length, value->Buffer, &s);

			cstr format = (value->Length > 24) ? (" = '%.24s...'") : (" = '%s'");
			AddSymbol(ce.Builder, format, (cstr)value->Buffer);

			VariantValue ptr;
			ptr.vPtrValue = (void*)&sc->header.pVTables[0];

			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4 + tempDepth, ptr, BITCOUNT_POINTER);

			return true;
		}

		void CompileGetStructRefFromVariable(CCompileEnvironment& ce, cr_sex s, const IStructure& inputType, cstr name, const MemberDef& def)
		{
			cstr varName = s.c_str();
			const IStructure& varStruct = *def.ResolvedType;

			SexyVarType vType = varStruct.VarType();
			if (vType != SexyVarType_Derivative)
			{
				Throw(s, "The variable is not a derived type. Expected: %s %s", GetFriendlyName(inputType), name);
			}

			if (varStruct.Prototype().IsClass)
			{
				int cii = (&varStruct == &inputType) ? 0 : GetCommonInterfaceIndex(varStruct, inputType);
				if (cii < 0)
				{
					Throw(s, "The input type '%s' did not match the argument type '%s %s'", varStruct.Name(), GetFriendlyName(inputType), name);
				}
			}

			ce.Builder.AddSymbol(varName);
			ce.Builder.AssignVariableRefToTemp(varName, Rococo::ROOT_TEMPDEPTH, 0);
		}

		void CompileGetStructRefFromAtomic(CCompileEnvironment& ce, cr_sex s, const IStructure& inputType, cstr name)
		{
			cstr token = s.c_str();

			if (islower(token[0]))
			{
				MemberDef def;
				if (ce.Builder.TryGetVariableByName(def, token))
				{
					if (*def.ResolvedType != inputType)
					{
						Throw(s, "The input '%s %s' did not match the input variable '%s %s'", GetFriendlyName(inputType), name, GetFriendlyName(*def.ResolvedType), token);
					}
					else
					{
						CompileGetStructRefFromVariable(ce, s, inputType, name, def);
					}
				}
				else
				{
					NamespaceSplitter splitter(token);

					cstr instance, method;
					if (splitter.SplitTail(instance, method))
					{
						if (isupper(method[0]))
						{
							CompileInputRefFromGetAccessor(ce, s, inputType);
						}
						else
						{
							if (ce.Builder.TryGetVariableByName(def, instance))
							{
								ThrowTokenNotFound(s, instance, ce.Builder.Owner().Name(), ("variable"));
							}
							else
							{
								ThrowTokenNotFound(s, instance, instance, ("member"));
							}
						}
					}
					else
					{
						ThrowTokenNotFound(s, token, ce.Builder.Owner().Name(), ("variable"));
					}
				}
			}
			else if (isupper(token[0]))
			{
				if (inputType.VarType() != SexyVarType_Closure)
				{
					CompileInputRefFromGetAccessor(ce, s, inputType); // Possible function call
				}
				else
				{
					CompileInputRefFromFunctionRef(ce, s, inputType);
				}
			}
			else
			{
				Throw(s, "Expecting an expression that returns a reference to '%s %s'", GetFriendlyName(inputType), name);
			}
		}

		void CompileGetStructRef(CCompileEnvironment& ce, cr_sex s, const IStructure& inputType, cstr name)
		{
			switch (s.Type())
			{
			case EXPRESSION_TYPE_STRING_LITERAL:
			{
				auto* stringConstant = ce.SS.ReflectImmutableStringPointer(s.c_str());
				InterfacePointer pIString = &stringConstant->header.pVTables[0];
				VariantValue v;
				v.vPtrValue = pIString;
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, v, BITCOUNT_POINTER);
			}
			break;
			case EXPRESSION_TYPE_ATOMIC:
				CompileGetStructRefFromAtomic(ce, s, inputType, name);
				break;
			case EXPRESSION_TYPE_COMPOUND:
				if (!TryCompileFunctionCallAndReturnValue(ce, s, SexyVarType_Derivative, &inputType, NULL))
				{
					Throw(s, "Expecting compound expression to return input for '%s %s'", GetFriendlyName(inputType), name);
				}
				break;
			default:
				Throw(s, "Expecting atomic, compound or string literal expression for '%s %s'", GetFriendlyName(inputType), name);
			}
		}

		bool TryCompilePushStructRef(CCompileEnvironment& ce, cr_sex s, bool expectingStructRef, const IStructure& inputType, cstr name, const IStructure* genericArg1)
		{
			// TODO refactor all of this, its old and ugly

			switch (s.Type())
			{
			case EXPRESSION_TYPE_STRING_LITERAL:
				return TryCompileStringLiteralInput(ce, s, expectingStructRef, inputType);
			case EXPRESSION_TYPE_ATOMIC:
				break;
			case EXPRESSION_TYPE_COMPOUND:
				if (!TryCompileFunctionCallAndReturnValue(ce, s, SexyVarType_Derivative, &inputType, NULL))
				{
					if (s.NumberOfElements() == 1)
					{
						return TryCompilePushStructRef(ce, s[0], expectingStructRef, inputType, name, genericArg1);
					}
					return false;
				}

				AddArgVariable("input_return_as_input_ref", ce, inputType);
				ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4 + ROOT_TEMPDEPTH, BITCOUNT_POINTER);
				return true;
			default:
				if (expectingStructRef)
				{
					Throw(s, "Error with %s.Expecting atomic, compound or string literal expression for '%s %s'", ce.Builder.Owner().Name(), GetFriendlyName(inputType), name);
				}
				return false;
			}

			cstr vname = s.c_str();

			if (!IsAlphabetical(vname[0]))
			{
				cstr friendlyName = GetFriendlyName(inputType);
				if (strcmp(friendlyName, "IString") == 0)
				{
					Throw(s, "Expected either a quoted string constant or alphabetic character as first in token to specify a reference to argument '%s %s'", friendlyName, name);
				}
				else
				{
					Throw(s, "Expected alphabetic character as first in token to specify a reference to argument '%s %s'", friendlyName, name);
				}
			}

			MemberDef def;
			if (!ce.Builder.TryGetVariableByName(OUT def, vname))
			{
				CompileInputRefFromGetAccessor(ce, s, inputType);
				return true;
			}

			SexyVarType vType = def.ResolvedType->VarType();
			if (vType != SexyVarType_Derivative && !IsContainerType(vType))
			{
				if (expectingStructRef)
				{
					Throw(s, "Error with %s. The variable '%s' is not a derived type. Expected: '%s %s'", ce.Builder.Owner().Name(), vname, GetFriendlyName(inputType), name);
				}
			}

			const IStructure* varStruct = def.ResolvedType;

			int cii;
			if (varStruct == &inputType)
			{
				cii = expectingStructRef ? -1 : 0;
			}
			else
			{
				cii = GetCommonInterfaceIndex(*varStruct, inputType);
				if (cii < 0)
				{
					if (expectingStructRef)
					{
						Throw(s, "Error with %s. The input type '%s' did not match the argument type '%s %s'", ce.Builder.Owner().Name(), GetFriendlyName(*varStruct), GetFriendlyName(inputType), name);
					}

					return false;
				}
			}

			ce.Builder.AddSymbol(vname);

			if (varStruct == &ce.StructArray())
			{
				const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, vname);

				if (&elementType != genericArg1)
				{
					char buf[256];
					StackStringBuilder ssb(buf, sizeof buf);

					ssb << "Error with " << ce.Builder.Owner().Name() << ". ";
					ssb << "The input supplied was (array " << GetFriendlyName(elementType) << " " << vname << ") ";

					if (genericArg1 != NULL)
					{
						ssb << "but input required was (array " << GetFriendlyName(*genericArg1) << " " << name << ") ";
					}
					else
					{
						ssb << "but input required was (" << GetFriendlyName(inputType) << " " << name << ") ";
					}

					Throw(s, "%s", buf);
				}
			}

			AddArgVariable("input_variable_ref", ce, inputType);
			PushVariableRef(s, ce.Builder, def, vname, cii);

			return true;
		}

		int PushInput(CCompileEnvironment& ce, cr_sex s, int index, const IStructure& inputStruct, const IArchetype* archetype, cstr inputName, const IStructure* genericArg1, cstr defaultValue)
		{
			cr_sex inputExpression = s.GetElement(index);

			if (!IsPointerValid(&inputStruct))
			{
				Throw(inputExpression, ("Function input type has not been resolved"));
			}

			SexyVarType inputType = inputStruct.VarType();
			BITCOUNT bits = GetBitCount(inputType);

			int stackAllocByteCount = 0;

			switch (bits)
			{
			case BITCOUNT_32:
				stackAllocByteCount += 4;
				break;
			case BITCOUNT_64:
				stackAllocByteCount += 8;
				break;
			case BITCOUNT_128:
				stackAllocByteCount += 16;
				break;
			default:
				Throw(s, "Algorithmic error: unhandled bitcount in argument to function");
			}

			if (IsAtomic(inputExpression))
			{			
				// Try pushing direct, more efficient than evaluating to a temp variable then pushing the variable
				cstr inputToken = inputExpression.c_str();

				if (Eq(inputToken, "."))
				{
					if (defaultValue == nullptr)
					{
						Throw(inputExpression, "No default value matches %s %s", GetFriendlyName(inputStruct), inputName);
					}
					else
					{
						inputToken = defaultValue;
					}
				}

				if (IsNumericTypeOrBoolean(inputType))
				{
					VariantValue immediateValue;
					if (Parse::TryParse(OUT immediateValue, inputType, inputToken) == Parse::PARSERESULT_GOOD)
					{
						AddArgVariable("input_literal", ce, inputStruct);
						ce.Builder.AddSymbol(inputName);
						ce.Builder.Assembler().Append_PushLiteral(bits, immediateValue);
						return stackAllocByteCount;
					}
					else
					{
						MemberDef def;
						if (ce.Builder.TryGetVariableByName(OUT def, inputToken) && def.ResolvedType->VarType() == inputType)
						{
							if (inputStruct.IsStrongType() && &inputStruct != def.ResolvedType)
							{
								Throw(s, "While compiling function %s: Pushing argument to a function rejected. (%s %s) was strongly-typed but the supplied type was %s,"
									"which is the correct underlying type but not directly convertible.", 
									ce.Builder.Owner().Name(), GetFriendlyName(inputStruct), inputName, GetFriendlyName(*def.ResolvedType));
							}

							AddArgVariable("input_variable", ce, inputStruct);
							ce.Builder.AddSymbol(inputName);
							ce.Builder.PushVariable(def);
							return stackAllocByteCount;
						}
					}
				}
			}

			if (inputType == SexyVarType_Bool)
			{
				bool negate = false;
				if (!TryCompileBooleanExpression(ce, inputExpression, true, negate))
				{
					Throw(inputExpression, ("Expected boolean input"));
				}
				if (negate) ce.Builder.Assembler().Append_BooleanNot(VM::REGISTER_D4 + ROOT_TEMPDEPTH);
			}
			else if (IsPrimitiveType(inputType))
			{
				if (!TryCompileArithmeticExpression(ce, inputExpression, true, inputType))
				{
					Throw(inputExpression, "Expected %s valued expression", GetTypeName(inputType));
				}
			}
			else if (inputType == SexyVarType_Derivative || IsContainerType(inputType))
			{
				ce.Builder.AddSymbol(inputName);
				if (!TryCompilePushStructRef(ce, inputExpression, true, inputStruct, inputName, genericArg1))
				{
					Throw(inputExpression, "Expected a reference to a %s", GetFriendlyName(inputStruct));
				}
				return sizeof(size_t);
			}
			else if (inputType == SexyVarType_Closure)
			{
				CompilePushClosure(ce, inputExpression, inputStruct, *archetype, inputName);
				return inputStruct.SizeOfStruct();
			}

			AddArgVariable("input_other", ce, inputStruct);
			ce.Builder.AddSymbol(inputName);
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D7, bits);

			return stackAllocByteCount;
		}

		void AppendCopyPointer(ICodeBuilder& builder, int srcSFoffset, const MemberDef& targetDef)
		{
			builder.Assembler().Append_SetSFValueFromSFValue(targetDef.SFOffset, srcSFoffset, BITCOUNT_POINTER);
		}

		void CompilePopOutputToVariable(const IStructure& requiredOutputStruct, ICodeBuilder& builder, int outputPos, int outputIndex, int sfOffset, cr_sex s)
		{
			if (outputPos + outputIndex >= s.NumberOfElements())
			{
				if (requiredOutputStruct.InterfaceCount() > 0)
				{
					// We skipped an output parameter, so we need to decrement the ref
					builder.Assembler().Append_GetStackFrameValue(sfOffset, VM::REGISTER_D4, BITCOUNT_POINTER);
					builder.Append_DecRef();
				}
				// Output parameters not specified are skipped
				return;
			}

			cr_sex outputExpr = s.GetElement(outputPos + outputIndex);
			AssertAtomic(outputExpr);
			cstr outputVar = outputExpr.c_str();

			MemberDef outputDef;
			if (!builder.TryGetVariableByName(OUT outputDef, outputVar))
			{
				Throw(outputExpr, "The output token was not a recognized variable");
			}

			TokenBuffer symbol;
			SafeFormat(symbol.Text, TokenBuffer::MAX_TOKEN_CHARS, (" -> %s"), outputVar);
			builder.AddSymbol(symbol);

			const IStructure* exprOutputStruct = outputDef.ResolvedType;

			if (&requiredOutputStruct != exprOutputStruct)
			{
				Throw(outputExpr, "Function expects type %s but identifier was of type %s", GetFriendlyName(requiredOutputStruct), GetFriendlyName(*exprOutputStruct));
			}

			const IArchetype* targetArchetype = exprOutputStruct->Archetype();

			if (requiredOutputStruct.Archetype() != targetArchetype)
			{
				Throw(outputExpr, ("The archetype of the output variable did not match that supplied to the function call"));
			}

			if (requiredOutputStruct.VarType() == SexyVarType_Derivative)
			{
				if (outputDef.MemberOffset != 0)
				{
					Throw(outputExpr, ("The output variable was a reference to a derivative type, but not the first or only member"));
				}

				AppendCopyPointer(builder, sfOffset, outputDef);
			}
			else
			{
				BITCOUNT bits = GetBitCount(requiredOutputStruct.VarType());
				if (!outputDef.IsParentValue && outputDef.Usage == ARGUMENTUSAGE_BYVALUE && outputDef.MemberOffset == 0)
				{
					builder.Assembler().Append_SetSFValueFromSFValue(outputDef.SFOffset, sfOffset, bits);
				}
				else
				{
					builder.Assembler().Append_GetStackFrameValue(sfOffset, VM::REGISTER_D5, GetBitCount(requiredOutputStruct.VarType()));
					builder.AssignTempToVariable(1, outputVar);
				}
			}
		}

		int GetOutputStackByteCount(const IArchetype& a, cr_sex sExpr)
		{
			int total = 0;
			for (int i = 0; i < a.NumberOfOutputs(); i++)
			{
				const IStructure& s = a.GetArgument(i);
				switch (s.VarType())
				{
				case SexyVarType_Derivative:
				case SexyVarType_Array:
				case SexyVarType_Map:
					total += sizeof size_t;
					break;
				default:
					total += s.SizeOfStruct();
				}
			}

			return total;
		}

		bool TryCompilePlainFunctionCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, SexyVarType type, const IStructure* derivedType, const IArchetype* returnArchetype)
		{
			if (IsCompound(s))
			{
				cr_sex firstArg = GetAtomicArg(s, 0);
				IFunction* f = MatchFunction(firstArg, ce.Builder.Module());
				if (f != NULL)
				{
					CompileFunctionCallAndReturnValue(ce, s, *f, type, returnArchetype, derivedType);
					return true;
				}
			}
			else if (IsAtomic(s))
			{
				IFunction* f = MatchFunction(s, ce.Builder.Module());
				if (f != NULL)
				{
					if (f->NumberOfInputs() > 0)
					{
						return false;
					}

					if (f->NumberOfOutputs() != 1)
					{
						return false;
					}

					CompileFunctionCallAndReturnValue(ce, s, *f, type, returnArchetype, derivedType);
					return true;
				}
			}

			return false;
		}

		void ValidateCorrectOutput(cr_sex s, const IStructure& outputStruct, SexyVarType returnType)
		{
			if (!IsPointerValid(&outputStruct))
			{
				Throw(s, ("Output structure was NULL. Internal algorithmic error."));
			}

			if (outputStruct.VarType() != returnType)
			{
				Throw(s, "Function returns %s but expression expects %s", GetTypeName(outputStruct.VarType()), GetTypeName(returnType));
			}
		}

		void ValidateSingleOutput(cr_sex s, int outputCount)
		{
			if (outputCount == 0)
				Throw(s, "Function has no output, hence no hence no return value");

			if (outputCount != 1)
				Throw(s, "Function has multiple outputs, and no specific return value");
		}

		void ValidateSingleOutputAndType(CCompileEnvironment& ce, cr_sex s, const IArchetype& callee, SexyVarType returnType, const IArchetype* returnArchetype, const IStructure* returnTypeStruct)
		{
			ValidateSingleOutput(s, callee.NumberOfOutputs());

			const IStructure* argStruct;

			switch (returnType)
			{
			case SexyVarType_Array:
				argStruct = callee.GetGenericArg1(0);
				if (argStruct == nullptr || callee.GetArgument(0).VarType() != SexyVarType_Array)
				{
					Throw(s, "Expecting output to be an array");
				}
				if (returnTypeStruct != argStruct)
				{
					Throw(s, "Function returns %s but expression expects an array of type %s", GetFriendlyName(*argStruct), GetFriendlyName(*returnTypeStruct));
				}
				break;
			default:
				argStruct = &callee.GetArgument(0);
				if (returnTypeStruct != NULL && returnTypeStruct != argStruct)
				{
					Throw(s, "Function returns %s but expression expects %s", GetFriendlyName(*argStruct), GetFriendlyName(*returnTypeStruct));
				}
				ValidateCorrectOutput(s, *argStruct, returnType);
			}
		}

		void ReturnOutput(CCompileEnvironment& ce, int outputOffset, SexyVarType returnType)
		{
			if (returnType == SexyVarType_Closure)
			{
				outputOffset -= 8;
				ce.Builder.Assembler().Append_GetStackFrameValue(outputOffset, VM::REGISTER_D14, BITCOUNT_POINTER);

				outputOffset -= 8;
				ce.Builder.Assembler().Append_GetStackFrameValue(outputOffset, VM::REGISTER_D7, BITCOUNT_POINTER);
			}
			else
			{
				BITCOUNT bc = GetBitCount(returnType);
				outputOffset -= bc / 8;
				ce.Builder.Assembler().Append_GetStackFrameValue(outputOffset, VM::REGISTER_D7, bc);
			}
		}

		int AllocFunctionOutput(CCompileEnvironment& ce, const IArchetype& callee, cr_sex sExpr)
		{
			int outputStackAllocByteCount = GetOutputStackByteCount(callee, sExpr);
			if (outputStackAllocByteCount > 0)
			{
				int total = 0;
				for (int i = 0; i < callee.NumberOfOutputs(); i++)
				{
					const IStructure& s = callee.GetArgument(i);
					AddArgVariable("output", ce, s);
				}

				char stackAllocHint[256];
				SafeFormat(stackAllocHint, 256, "Output for %s", callee.Name());
				ce.Builder.AddSymbol(stackAllocHint);

				ce.Builder.Assembler().Append_StackAlloc(outputStackAllocByteCount);
			}

			return outputStackAllocByteCount;
		}

		void RepairStack(CCompileEnvironment& ce, cr_sex s, const IArchetype& callee, int extraArgs)
		{
			MarkStackRollback(ce, s);
			ce.Builder.PopLastVariables(callee.NumberOfInputs() + callee.NumberOfOutputs() + extraArgs, true);
		}

		int CompileVirtualCallKernel(CCompileEnvironment& ce, bool callAtomic, const IArchetype& callee, cr_sex s, int interfaceIndex, int methodIndex, cstr instanceName, const IObjectInterface& interfaceRef)
		{
			MemberDef def;
			ce.Builder.TryGetVariableByName(def, instanceName);

			char localName[128] = "";

			if (def.IsParentValue)
			{
				static int64 index = 0;

				SafeFormat(localName, 128, "parentSF_%s_%llu", GetFriendlyName(*def.ResolvedType), index++);
				AddVariable(ce, NameString::From(localName), ce.Object.Common().TypePointer());
				ce.Builder.AssignVariableToTemp(instanceName, 0);
				ce.Builder.AssignTempToVariable(0, localName);
			}

			int outputStackAllocByteCount = AllocFunctionOutput(ce, callee, s);

			int inputStackAllocCount = callAtomic ? 0 : PushInputs(ce, s, callee, true, 1);
			inputStackAllocCount += sizeof(size_t); // Allow a few bytes for the instance pointer

			AddArgVariable("input_interface", ce, ce.Object.Common().TypePointer());
			AppendVirtualCallAssembly(instanceName, interfaceIndex, methodIndex, ce.Builder, interfaceRef, s, localName);
			ce.Builder.MarkExpression(&s);

			RepairStack(ce, s, callee);

			int outputOffset = GetOutputSFOffset(ce, inputStackAllocCount, outputStackAllocByteCount);

			return outputOffset;
		}

		void AppendFunctionCallAssembly(CCompileEnvironment& ce, const IFunction& callee)
		{
			CodeSection section;
			callee.Code().GetCodeSection(OUT section);

			ce.Builder.AddSymbol(callee.Name());
			ce.Builder.Assembler().Append_CallById(section.Id);
		}

		bool IsInputLambda(cr_sex s)
		{
			// Expressions of this form are lambdas:
			// (<input names> -> (output names>) : <directives>)
			// Hence if we detect the colon in an input directive it is assumed to be a lambda
			// If there are no outputs this simplifies to (<input-names>: <directives>)
			// If there are no inputs nor outputs this further simplfies to (: <directives>)
			// If there are no inputs, outputs, nor directives this simplies to the null lambda (:)

			if (s.NumberOfElements() > 0)
			{
				cr_sex arg0 = s[0];
				if (IsAtomic(arg0) && s[0].c_str()[0] == '#')
				{
					// A macro, not a lamba
					return false;
				}
			}

			for (int i = 0; i < s.NumberOfElements(); i++)
			{
				cr_sex arg = s[i];
				if (IsAtomic(arg) && AreEqual(s[i].String(), ":"))
				{
					return true;
				}
			}

			return false;
		}

		void AllocateLambda(CCompileEnvironment& ce, cr_sex s, cstr inputName, const IStructure& argType, const IArchetype& archetype)
		{
			CompileLambda(ce, s, argType, archetype, inputName);
		}

		void AllocateLambdas(CCompileEnvironment& ce, cr_sex s, const IFunction& callee, bool isImplicitInput, int firstArgIndex)
		{
			EnumerateInputs(callee, isImplicitInput, true, [&ce, &s, firstArgIndex, &callee](cstr inputName, const IStructure& argType, const IArchetype* archetype, int index)
				{
					if (archetype)
					{
						if (s.NumberOfElements() + firstArgIndex <= index)
						{
							// Insufficient items in expression to invoke function
							Throw(s, "Insufficient arguments to invoke function %s", callee.Name());
						}

						cr_sex sInputArg = s[firstArgIndex + index];
						if (IsInputLambda(sInputArg))
						{
							AllocateLambda(ce, sInputArg, inputName, argType, *archetype);
						}
					}
				}
			);
		}

		int CompileFunctionCallKernel(CCompileEnvironment& ce, cr_sex s, const IFunction& callee)
		{
			auto* security = callee.Security();
			if (security)
			{
				bool success = false;

				try
				{
					success = security->handler->IsCallerPermitted(*security, s);
				}
				catch (IException& ex)
				{
					Throw(s, "The source file %s is not permitted to call %s directly.\n  - %s\n", s.Tree().Source().Name(), callee.Name(), ex.Message());
				}

				if (!success)
				{
					Throw(s, "The source file %s is not permitted to call %s directly.", s.Tree().Source().Name(), callee.Name());
				}
			}

			AllocateLambdas(ce, s, callee, false, 1);

			const int outputStackAllocCount = AllocFunctionOutput(ce, callee, s);
			int inputStackAllocCount = PushInputs(ce, s, callee, false, 1);
			AppendFunctionCallAssembly(ce, callee);
			ce.Builder.MarkExpression(&s);
			RepairStack(ce, s, callee);

			int outputOffset = GetOutputSFOffset(ce, inputStackAllocCount, outputStackAllocCount);
			return outputOffset;
		}

		void CompileFunctionCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, IFunction& callee, SexyVarType returnType, const IArchetype* returnArchetype, const IStructure* returnTypeStruct)
		{
			cstr calleeName = callee.Name();
			cstr callerName = ce.Builder.Owner().Name();

			ValidateSingleOutputAndType(ce, s, callee, returnType, returnArchetype, returnTypeStruct);
			ValidateInputCount(s, ArgCount(callee) - 1);

			try
			{
				int outputOffset = CompileFunctionCallKernel(ce, s, callee);
				ReturnOutput(ce, outputOffset, returnType);
			}
			catch (ParseException& sex)
			{
				Rococo::Sex::Throw(s, "Error compiling %s: %s", callee.Name(), sex.Message());
			}
			catch (IException& ex)
			{
				Rococo::Sex::Throw(s, "Error compiling %s: %s", callee.Name(), ex.Message());
			}

			ce.Builder.AssignClosureParentSFtoD6();
		}

		void CompileVirtualCallAndReturnValue(CCompileEnvironment& ce, bool callAtomic, const IArchetype& callee, cr_sex s, int interfaceIndex, int methodIndex, cstr instanceName, SexyVarType returnType, const IStructure* returnTypeStruct, const IArchetype* returnArchetype, const IObjectInterface& interfaceRef)
		{
			cstr calleeName = callee.Name();
			cstr callerName = ce.Builder.Owner().Name();

			// (<instance.method-name> input1 input2 input3.... inputN -> output1...output2...outputN)
			ValidateSingleOutputAndType(ce, s, callee, returnType, returnArchetype, returnTypeStruct);
			ValidateNumberOfInputArgs(s, callee, callAtomic ? 0 : s.NumberOfElements());

			int outputOffset = CompileVirtualCallKernel(ce, callAtomic, callee, s, interfaceIndex, methodIndex, instanceName, interfaceRef);
			ReturnOutput(ce, outputOffset, returnType);
			ce.Builder.AssignClosureParentSFtoD6();
		}

		int CompileCloseCallHeader(CCompileEnvironment& ce, cr_sex s, const IArchetype& callee, cstr closureVariable)
		{
			TokenBuffer parentSF;
			SafeFormat(parentSF.Text, TokenBuffer::MAX_TOKEN_CHARS, "%s.parentSF", closureVariable);

			MemberDef def;
			ce.Builder.TryGetVariableByName(OUT def, parentSF);
			ce.Builder.AddSymbol(parentSF);

			AddArgVariable("closure_parentSF", ce, *def.ResolvedType);
			ce.Builder.PushVariable(def);

			int sfSize = sizeof(size_t);
			int outputAndSFstackAllocByteCount = sfSize; // Add space for the parentSF
			outputAndSFstackAllocByteCount += AllocFunctionOutput(ce, callee, s);
			return outputAndSFstackAllocByteCount;
		}

		int CompileClosureCallKernel(CCompileEnvironment& ce, cr_sex s, const IArchetype& archetype, cstr closureVariable)
		{
			int inputStackAllocByteCount = PushInputs(ce, s, archetype, false, 1);

			TokenBuffer pathToId;
			SafeFormat(pathToId.Text, TokenBuffer::MAX_TOKEN_CHARS, "%s.bytecodeId", closureVariable);
			ce.Builder.AssignVariableToTemp(pathToId, 0); // Copy the closure bytecode id to D4

			TokenBuffer callSymbol;
			SafeFormat(callSymbol.Text, TokenBuffer::MAX_TOKEN_CHARS, "call %s %s", archetype.Name(), closureVariable);
			ce.Builder.AddSymbol(callSymbol);
			ce.Builder.Assembler().Append_CallByIdIndirect(VM::REGISTER_D4);

			ce.Builder.MarkExpression(&s);

			return inputStackAllocByteCount;
		}

		void CompileClosureCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, cstr name, SexyVarType returnType, const IArchetype* returnArchetype)
		{
			const IStructure* varStruct = ce.Builder.GetVarStructure(name);
			const IArchetype& archetype = *varStruct->Archetype();

			ValidateSingleOutput(s, archetype.NumberOfOutputs());

			const IStructure& outputStruct = archetype.GetArgument(0);
			ValidateCorrectOutput(s, outputStruct, returnType);

			const IArchetype* argArchetype = outputStruct.Archetype();
			if (returnArchetype != argArchetype)
			{
				Throw(s, ("The archetype of the output does not match that required by the invocation"));
			}

			ValidateInputCount(s, archetype.NumberOfInputs());

			int outputAndSFstackAllocByteCount = CompileCloseCallHeader(ce, s, archetype, name);
			int inputStackAllocByteCount = CompileClosureCallKernel(ce, s, archetype, name);

			RepairStack(ce, s, archetype, 1);

			int outputOffset = GetOutputSFOffset(ce, inputStackAllocByteCount, outputAndSFstackAllocByteCount);

			ReturnOutput(ce, outputOffset, returnType);
			ce.Builder.AssignClosureParentSFtoD6();
		}

		cstr GetIndexedMethod(CCompileEnvironment& ce, cr_sex invocation, const IStructure* s)
		{
			if (s == NULL) return NULL;
			if (!s->Prototype().IsClass) return NULL;
			if (invocation.NumberOfElements() != 2) return NULL;

			for (int i = 0; i < s->InterfaceCount(); ++i)
			{
				const IObjectInterface& interf = s->GetInterface(i);

				const ISExpression* src;
				if (interf.Attributes().FindAttribute("indexed", (const void*&)src))
				{
					cr_sex methodNameExpr = GetAtomicArg(*src, 3);
					for (int j = 0; j < interf.MethodCount(); ++j)
					{
						const IArchetype& method = interf.GetMethod(j);
						if (AreEqual(methodNameExpr.String(), method.Name()))
						{
							if (method.NumberOfInputs() == 2)
							{
								return method.Name();
							}
							else
							{
								Throw(methodNameExpr, "%s attribute 'indexed' found, but the index function must take only one input", interf.Name());
							}
						}
					}

					Throw(methodNameExpr, "%s attribute 'indexed' found, but the name was not found in the list of methods for the interfafce", interf.Name());
				}
			}

			return NULL;
		}

		bool IsIStringInlined(CScript& script);

		void CompileAsInlinedItemDirectAndReturnValue(CCompileEnvironment& ce, cstr instance, cstr item, SexyVarType returnType, int interfaceToInstanceOffsetByRef)
		{
			MemberDef instanceDef;
			ce.Builder.TryGetVariableByName(OUT instanceDef, instance);

			if (instanceDef.IsContained && IsNullType(*instanceDef.ResolvedType))
			{
				static int index = 0;
				char tempString[64];
				SafeFormat(tempString, sizeof(tempString), "_inlineStringArg%d", index++);

				AddVariable(ce, NameString::From(tempString), ce.Object.Common().TypePointer());

				UseStackFrameFor(ce.Builder, instanceDef);
				ce.Builder.AssignVariableToTemp(instance, 0); // This gives IString.VTable1 in D4
				RestoreStackFrameFor(ce.Builder, instanceDef);

				MemberDef tempDef;
				ce.Builder.TryGetVariableByName(OUT tempDef, tempString);

				// We create a weak reference in tempString
				ce.Builder.Assembler().Append_SetStackFrameValue(tempDef.SFOffset, VM::REGISTER_D4, BITCOUNT_POINTER);

				int offset = 0;
				const IMember* member = FindMember(*instanceDef.ResolvedType, item, OUT offset);
				if (member == nullptr)
				{
					Throw(0, "Cannot find %s of %s", item, instance);
				}

				offset -= ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0;

				ce.Builder.PopLastVariables(1, true);

				BITCOUNT bitCount = GetBitCount(returnType);
				ce.Builder.Assembler().Append_GetStackFrameMember(VM::REGISTER_D7, tempDef.SFOffset, offset, bitCount);

				return;
			}

			if (instanceDef.location == VARLOCATION_TEMP && instanceDef.ResolvedType->Name()[0] == '_')
			{
				TokenBuffer fqItemName;
				StringPrint(fqItemName, "%s.%s", instance, item);

				MemberDef itemDef;
				ce.Builder.TryGetVariableByName(OUT itemDef, fqItemName);

				BITCOUNT bitCount = GetBitCount(returnType);

				UseStackFrameFor(ce.Builder, itemDef);
				ce.Builder.Assembler().Append_GetStackFrameMember(VM::REGISTER_D7, itemDef.SFOffset, itemDef.MemberOffset - interfaceToInstanceOffsetByRef, (BITCOUNT)(8 * itemDef.ResolvedType->SizeOfStruct()));
				RestoreStackFrameFor(ce.Builder, itemDef);
			}
			else
			{
				TokenBuffer fqn;
				StringPrint(fqn, "%s.%s", instance, item);

				ce.Builder.TryGetVariableByName(OUT instanceDef, fqn);

				int interfaceToInstanceOffset = instanceDef.Usage == ARGUMENTUSAGE_BYREFERENCE ? interfaceToInstanceOffsetByRef : 0;
				ce.Builder.AssignVariableToTemp(fqn, Rococo::ROOT_TEMPDEPTH, -interfaceToInstanceOffset);
			}
		}

		void ValidateReturnType(cr_sex s, SexyVarType returnType, SexyVarType type)
		{
			if (returnType == SexyVarType_AnyNumeric && IsPrimitiveType(type)) return;
			if (returnType != type)
			{
				Throw(s, "The property returns type %s only", GetTypeName(returnType));
			}
		}

		bool TryCompileAsInlineArrayAndReturnValue(CCompileEnvironment& ce, cr_sex s, cstr instance, cstr methodName, SexyVarType returnType, const IStructure& instanceStruct)
		{
			if (instanceStruct == ce.StructArray())
			{
				const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);

				if (AreEqual("Length", methodName))
				{
					ce.Builder.AssignVariableToTemp(instance, 0, 0); // This shifts the array pointer to D4
					AddSymbol(ce.Builder, "D7 = %s.Length", instance);
					AppendInvoke(ce, callbacks.ArrayReturnLength, s); // the length is now written to D7
					ValidateReturnType(s, returnType, SexyVarType_Int32);
					return true;
				}
				else if (AreEqual("Capacity", methodName))
				{
					ce.Builder.AssignVariableToTemp(instance, 0, 0); // This shifts the array pointer to D4
					AddSymbol(ce.Builder, "D7 = %s.Capacity", instance);
					AppendInvoke(ce, callbacks.ArrayReturnCapacity, s); // the length is now written to D7
					ValidateReturnType(s, returnType, SexyVarType_Int32);
					return true;
				}
				else if (AreEqual("PopOut", methodName))
				{
					CompileAsPopOutFromArray(ce, s, instance, returnType);
					ValidateReturnType(s, returnType, SexyVarType_Int32);
					return true;
				}
				else
				{
					Throw(s, "The property is not recognized for array types. Known properties for arrays: Length, Capacity & PopOut");
				}
			}

			return false;
		}

		bool TryCompileAsInlineListAndReturnValue(CCompileEnvironment& ce, cr_sex s, cstr instance, cstr methodName, SexyVarType returnType, const IStructure& instanceStruct, SexyVarType& outType)
		{
			if (instanceStruct == ce.StructList())
			{
				if (!IsAtomic(s) && s.NumberOfElements() != 1) return false;

				if (Eq("Length", methodName))
				{
					ValidateReturnType(s, returnType, SexyVarType_Int32);
					ce.Builder.AddSymbol(instance);
					ce.Builder.AssignVariableToTemp(instance, Rococo::ROOT_TEMPDEPTH, 0); // list ref goes to D7
					ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).ListGetLength); // list length goes to D7
					outType = SexyVarType_Int32;
					return true;
				}
				else
				{
					Throw(s, "The property is not recognized for list types. Known properties for lists: Length");
				}
			}
			else if (instanceStruct == ce.Object.Common().TypeNode())
			{
				if (!IsAtomic(s) && s.NumberOfElements() != 1) return false;

				if (Eq("Value", methodName))
				{
					const IStructure& st = GetNodeDef(ce, s, instance);
					ValidateReturnType(s, returnType, st.VarType());

					ce.Builder.AssignVariableRefToTemp(instance, 0); // Node ptr goes to D4

					if (st.InterfaceCount() > 0)
					{
						ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).NodeGetInterface); // value goes to D7
						outType = st.VarType();
						return true;
					}

					switch (st.VarType())
					{
					case SexyVarType_Bool:
					case SexyVarType_Int32:
					case SexyVarType_Float32:
					case SexyVarType_Pointer:
					case SexyVarType_Int64:
					case SexyVarType_Float64:
					case SexyVarType_Closure:
						ce.Builder.Assembler().Append_Invoke(st.SizeOfStruct() == 4 ? GetListCallbacks(ce).NodeGet32 : GetListCallbacks(ce).NodeGet64); // value goes to D7
						break;
					default:
						Throw(s, "Node.Value only supports primitive types and interfaces");
					}

					outType = st.VarType();
					return true;
				}
				else if (Eq("HasNext", methodName))
				{
					ValidateReturnType(s, returnType, SexyVarType_Bool);

					ce.Builder.AssignVariableRefToTemp(instance, Rococo::ROOT_TEMPDEPTH); // Node ptr goes to D7
					ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).NodeHasNext); // Boolean (n.Next != NULL) goes to D7

					return true;
				}
				else if (Eq("HasPrevious", methodName))
				{
					ValidateReturnType(s, returnType, SexyVarType_Bool);

					ce.Builder.AssignVariableRefToTemp(instance, Rococo::ROOT_TEMPDEPTH); // Node ptr goes to D7
					ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).NodeHasPrevious); // Boolean (n.Next != NULL) goes to D7

					return true;
				}
				else if (Eq("GoPrevious", methodName))
				{
					ce.Builder.AssignVariableRefToTemp(instance, 0); // Node ptr goes to 4
					ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).NodeGoPrevious); // Next node updates D4, D7 if previous exists
					AssignTempToVariableRef(ce, 0, instance);
					return true;
				}
				else if (AreEqual("GoNext", methodName))
				{
					ce.Builder.AssignVariableRefToTemp(instance, 0); // Node ptr goes to D4
					ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).NodeGoNext); // Previous node updates D4, D7 if next exists
					AssignTempToVariableRef(ce, 0, instance);
					return true;
				}
				else
				{
					Throw(s, "The property is not recognized for node types. Known properties for nodes: Value, Previous, Next, Exists");
				}
			}

			return false;
		}

		bool TryCompileAsInlineIStringAndReturnValue(CCompileEnvironment& ce, cstr instance, cstr methodName, SexyVarType returnType, const IStructure& instanceStruct)
		{
			if (IsIStringInlined(ce.Script))
			{
				// The fact that IStrings are inlined means there is only one interface supported by anything that implements IString, and that Length and Buffer inline
				// to length and buffer respectively.

				int interfaceToInstanceOffsetByRef = ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0;

				if (returnType == SexyVarType_Int32 && AreEqual("Length", methodName))
				{
					if (Compiler::DoesClassImplementInterface(instanceStruct, ce.Object.Common().SysTypeIString()))
					{
						CompileAsInlinedItemDirectAndReturnValue(ce, instance, "length", SexyVarType_Int32, interfaceToInstanceOffsetByRef);
						return true;
					}
				}
				else if (returnType == SexyVarType_Pointer && AreEqual("Buffer", methodName))
				{
					if (Compiler::DoesClassImplementInterface(instanceStruct, ce.Object.Common().SysTypeIString()))
					{
						CompileAsInlinedItemDirectAndReturnValue(ce, instance, "buffer", SexyVarType_Pointer, interfaceToInstanceOffsetByRef);
						return true;
					}
				}
			}

			return false;
		}

		bool TryCompileAsTestExistenceAndReturnBool(CCompileEnvironment& ce, cr_sex s, cstr instance, const IStructure& instanceType)
		{
			ce.Builder.AssignVariableToTemp(instance, 0);

			VariantValue v;
			v.vPtrValue = (uint8*)(instanceType.GetInterface(0).UniversalNullInstance()) + ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, v, BITCOUNT_POINTER);

			AddSymbol(ce.Builder, "%s ?", instance);
			ce.Builder.Assembler().Append_Invoke(ce.SS.GetScriptCallbacks().idTestD4neqD5_retBoolD7);

			return true;
		}

		bool TryCompileExpressionBuilderCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, cstr instance, cstr methodName, const IStructure* returnTypeStruct, const IStructure& instanceStruct)
		{
			if (AreEqual(methodName, "TransformParent"))
			{
				if (!returnTypeStruct || (*returnTypeStruct != ce.StructExpressionBuilderInterface() && *returnTypeStruct != ce.StructExpressionInterface()))
				{
					Throw(s, "(%s.TransformParent) may only be used to assign to an IExpressionBuilder interface", instance);
				}

				if (!IsAtomic(s))
				{
					Throw(s, "(%s.TransformParent) takes no arguments", instance);
				}

				AddSymbol(ce.Builder, "%s -> D4", instance);
				ce.Builder.AssignVariableToTemp(instance, 0, 0); // Reference to the interface goes to D4

				AppendInvoke(ce, ce.SS.GetScriptCallbacks().idTransformParent_D4retIExpressionBuilderD7, s); // output is now D7
				return true;
			}

			if (AreEqual(methodName, "TransformAt"))
			{
				if (!returnTypeStruct || *returnTypeStruct != ce.StructExpressionBuilderInterface())
				{
					Throw(s, "(%s.TransformAt <index>) may only be used to assign to an IExpressionBuilder interface", instance);
				}

				if (s.NumberOfElements() != 2)
				{
					Throw(s, "(%s.TransformAt <index>) takes one argument", instance);
				}

				cr_sex sIndex = GetAtomicArg(s, 1);

				cstr indexString = sIndex.c_str();

				int index;
				if (Parse::TryParseDecimal(index, indexString) == Parse::PARSERESULT_GOOD)
				{
					VariantValue v;
					v.int32Value = index;
					ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, v, BITCOUNT_32);
				}
				else
				{
					ce.Builder.AssignVariableToTemp(indexString, 1);
				}
				
				AddSymbol(ce.Builder, "%s -> D4", instance);
				ce.Builder.AssignVariableToTemp(instance, 0, 0); // Reference to the builder expression goes to D4

				AddSymbol(ce.Builder, "index -> D5");
				ce.Builder.AssignVariableToTemp(indexString, 1, 0); // Index value goes to D5

				AddSymbol(ce.Builder, "%s -> D7", ce.Script.ProgramModule().Name());

				VariantValue scriptPtr;
				scriptPtr.vPtrValue = &ce.Script;
				ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, scriptPtr, BITCOUNT_POINTER); // script goes to D7

				AppendInvoke(ce, ce.SS.GetScriptCallbacks().idTransformAt_D4D5retIExpressionBuilderD7, s); // output is now D7
				return true;
			}

			return false;
		}


		bool TryCompileMethodCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, SexyVarType returnType, const IStructure* returnTypeStruct, const IArchetype* returnArchetype)
		{
			cr_sex firstArg = IsCompound(s) ? s.GetElement(0) : s;
			cstr fname = firstArg.c_str();

			NamespaceSplitter splitter(fname);

			const IStructure* instanceStruct = NULL;

			cstr instance, methodName;
			if (!splitter.SplitTail(OUT instance, OUT methodName))
			{
				// Could be index method or a ?
				if (s.NumberOfElements() == 2)
				{
					instanceStruct = ce.Builder.GetVarStructure(fname);
					instance = fname;

					if (IsAtomic(s[1]) && returnType == SexyVarType_Bool)
					{
						cstr arg = s[1].c_str();
						if (Eq(arg, "?") && instanceStruct)
						{
							if (instanceStruct->InterfaceCount() > 0 && IsNullType(*instanceStruct))
							{
								return TryCompileAsTestExistenceAndReturnBool(ce, s, instance, *instanceStruct);
							}
							else
							{
								Throw(s[0], "(... exists) can only be applied to interface types");
							}
						}
					}

					methodName = GetIndexedMethod(ce, s, instanceStruct);
					if (!methodName)
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				// Could be index method
				if (s.NumberOfElements() == 2 && IsAtomic(s[1]) && AreEqual(s[1].String(), "?") && returnType == SexyVarType_Bool)
				{
					instanceStruct = ce.Builder.GetVarStructure(fname);
					instance = fname;

					if (instanceStruct && instanceStruct->InterfaceCount() > 0 && IsNullType(*instanceStruct))
					{
						return TryCompileAsTestExistenceAndReturnBool(ce, s, instance, *instanceStruct);
					}
					else
					{
						Throw(s[0], "(... exists) can only be applied to interface types");
					}
				}

				instanceStruct = ce.Builder.GetVarStructure(instance);
			}

			if (instanceStruct == NULL)
			{
				return false;
			}

			if (!IsCapital(methodName[0]))
			{
				return false;
			}

			if (TryCompileAsInlineArrayAndReturnValue(ce, s, instance, methodName, returnType, *instanceStruct))
			{
				return true;
			}

			SexyVarType outType;
			if (TryCompileAsInlineListAndReturnValue(ce, s, instance, methodName, returnType, *instanceStruct, outType))
			{
				return true;
			}

			if (TryCompileAsInlineMapAndReturnValue(ce, s, instance, methodName, returnType, *instanceStruct, OUT outType))
			{
				return true;
			}

			if (TryCompileExpressionBuilderCallAndReturnValue(ce, s, instance, methodName, returnTypeStruct, *instanceStruct))
			{
				return true;
			}

			if (!instanceStruct->Prototype().IsClass)
			{
				return false;
			}

			if (TryCompileAsInlineIStringAndReturnValue(ce, instance, methodName, returnType, *instanceStruct))
			{
				return true;
			}

			OUT int interfaceIndex, OUT methodIndex;
			if (GetMethodIndices(OUT interfaceIndex, OUT methodIndex, *instanceStruct, methodName))
			{
				const IObjectInterface& interf = instanceStruct->GetInterface(interfaceIndex);
				const IArchetype& method = interf.GetMethod(methodIndex);

				CompileVirtualCallAndReturnValue(ce, false, method, s, interfaceIndex, methodIndex, instance, returnType, returnTypeStruct, returnArchetype, interf);
				return true;
			}

			auto sDef = (const ISExpression*) instanceStruct->Definition();
			if (sDef)
			{
				Throw(firstArg, "Could not identify method %s in %s of %s near line %d)", methodName, GetFriendlyName(*instanceStruct), instanceStruct->Module().Name(), sDef->Start().y);
			}
			else
			{
				Throw(firstArg, "Could not identify method %s in %s of %s", methodName, GetFriendlyName(*instanceStruct), instanceStruct->Module().Name());
			}
		}

		bool TryCompileMethodCallWithoutInputAndReturnValue(CCompileEnvironment& ce, cr_sex s, cstr instance, cstr methodName, SexyVarType returnType, const IStructure* returnTypeStruct, const IArchetype* returnArchetype)
		{
			const IStructure* instanceStruct = ce.Builder.GetVarStructure(instance);

			if (instanceStruct == NULL)
			{
				return false;
			}

			if (!IsCapital(methodName[0]))
			{
				return false;
			}

			if (TryCompileAsInlineArrayAndReturnValue(ce, s, instance, methodName, returnType, *instanceStruct))
			{
				return true;
			}

			SexyVarType outputType;
			if (TryCompileAsInlineListAndReturnValue(ce, s, instance, methodName, returnType, *instanceStruct, outputType))
			{
				return true;
			}

			if (TryCompileAsInlineMapAndReturnValue(ce, s, instance, methodName, returnType, *instanceStruct, OUT outputType))
			{
				return true;
			}

			if (!instanceStruct->Prototype().IsClass)
			{
				Throw(s, ("Only classes support methods."));
			}

			if (TryCompileAsInlineIStringAndReturnValue(ce, instance, methodName, returnType, *instanceStruct))
			{
				return true;
			}

			OUT int interfaceIndex, OUT methodIndex;
			if (GetMethodIndices(OUT interfaceIndex, OUT methodIndex, *instanceStruct, methodName))
			{
				const IObjectInterface& interf = instanceStruct->GetInterface(interfaceIndex);
				const IArchetype& method = interf.GetMethod(methodIndex);

				if (!IsGetAccessor(method))
				{
					return false;
				}

				CompileVirtualCallAndReturnValue(ce, true, method, s, interfaceIndex, methodIndex, instance, returnType, returnTypeStruct, returnArchetype, interf);
				return true;
			}

			return false;
		}

		SexyVarType CompileMethodCallWithoutInputAndReturnNumericValue(CCompileEnvironment& ce, cr_sex s, cstr instance, cstr methodName)
		{
			const IStructure* instanceStruct = ce.Builder.GetVarStructure(instance);

			if (instanceStruct == NULL)
			{
				Throw(s, "Unknown variable '%s'", instance);
			}

			if (!IsCapital(methodName[0]))
			{
				return SexyVarType_Bad;
			}

			SexyVarType outputType;
			if (TryCompileAsInlineMapAndReturnValue(ce, s, instance, methodName, SexyVarType_AnyNumeric, *instanceStruct, outputType))
			{
				return outputType;
			}

			if (TryCompileAsInlineListAndReturnValue(ce, s, instance, methodName, SexyVarType_AnyNumeric, *instanceStruct, outputType))
			{
				return outputType;
			}

			if (TryCompileAsInlineIStringAndReturnValue(ce, instance, methodName, SexyVarType_Int32, *instanceStruct))
			{
				return SexyVarType_Int32;
			}

			if (!instanceStruct->Prototype().IsClass)
			{
				Throw(s, ("Only classes support methods."));
			}

			OUT int interfaceIndex, OUT methodIndex;
			if (GetMethodIndices(OUT interfaceIndex, OUT methodIndex, *instanceStruct, methodName))
			{
				const IObjectInterface& interf = instanceStruct->GetInterface(interfaceIndex);
				const IArchetype& method = interf.GetMethod(methodIndex);

				if (!IsGetAccessor(method))
				{
					return SexyVarType_Bad;
				}

				SexyVarType type = method.GetArgument(0).VarType();

				if (!IsNumericTypeOrBoolean(type)) return SexyVarType_Bad;

				CompileVirtualCallAndReturnValue(ce, true, method, s, interfaceIndex, methodIndex, instance, type, NULL, NULL, interf);

				return type;
			}

			return SexyVarType_Bad;
		}

		int GetOutputArgSize(const IStructure& s)
		{
			if (s.VarType() == SexyVarType_Derivative)
			{
				return sizeof(size_t);
			}
			else
			{
				return s.SizeOfStruct();
			}
		}

		void PopOutputs(CCompileEnvironment& ce, cr_sex invocation, const IArchetype& callee, int outputOffset, bool isVirtualCall)
		{
			const int outputCount = callee.NumberOfOutputs();
			const int inputEnd = callee.NumberOfInputs() - (isVirtualCall ? 1 : 0) + 1;

			for (int i = 0; i < outputCount; i++)
			{
				int outputIndex = outputCount - 1 - i;
				const IStructure& arg = callee.GetArgument(outputIndex);
				outputOffset -= GetOutputArgSize(arg);

				if (arg.VarType() == SexyVarType_Array)
				{
					ValidateAssignment(invocation);
				}

				CompilePopOutputToVariable(arg, ce.Builder, inputEnd + 1, outputIndex, outputOffset, invocation);
			}
		}

		void AppendVirtualCallAssembly(cstr instanceName, int interfaceIndex, int methodIndex, ICodeBuilder& builder, const IObjectInterface& interf, cr_sex s, cstr localName)
		{
			MemberDef refDef;
			builder.TryGetVariableByName(OUT refDef, instanceName);

			const IArchetype& arch = interf.GetMethod(methodIndex);

			TokenBuffer fullName;
			StringPrint(fullName, ("%s.%s"), instanceName, arch.Name());
			builder.AddSymbol(fullName);

			int vTableByteOffset = (methodIndex + 1) * sizeof(ID_BYTECODE);

			if (IsNullType(*refDef.ResolvedType))
			{
				if (refDef.IsParentValue)
				{
					MemberDef localDef; // Local defs are copies of parent interface put onto the local stack by value
					builder.TryGetVariableByName(OUT localDef, localName);
					builder.Assembler().Append_CallVirtualFunctionViaRefOnStack(localDef.SFOffset, vTableByteOffset);
				}
				else
				{
					if (!refDef.IsContained || refDef.Usage == ARGUMENTUSAGE_BYVALUE)
					{
						builder.Assembler().Append_CallVirtualFunctionViaRefOnStack(refDef.SFOffset + refDef.MemberOffset, vTableByteOffset);
					}
					else
					{
						builder.Assembler().Append_CallVirtualFunctionViaMemberOffsetOnStack(refDef.SFOffset, refDef.MemberOffset, vTableByteOffset);
					}
				}
			}
			else
			{
				// We have a reference to a concrete object, an instance pointer

				for (int i = 0; i < refDef.ResolvedType->InterfaceCount(); ++i)
				{
					if (&refDef.ResolvedType->GetInterface(i) == &interf)
					{
						// Concrete class

						if (refDef.IsParentValue)
						{
							MemberDef localDef; // Local defs are copies of parent interface put onto the local stack by value
							builder.TryGetVariableByName(OUT localDef, localName);
							int instanceToInterfaceOffset = Compiler::GetInstanceToInterfaceOffset(i);
							builder.Assembler().Append_CallVirtualFunctionViaRefOnStack(localDef.SFOffset, vTableByteOffset);
						}
						else
						{
							int instanceToInterfaceOffset = Compiler::GetInstanceToInterfaceOffset(i) + refDef.MemberOffset;
							builder.Assembler().Append_CallVirtualFunctionViaRefOnStack(refDef.SFOffset, vTableByteOffset, instanceToInterfaceOffset);
						}
						return;
					}
				}

				Throw(s, "Could not compile concrete method invocation");
			}
		}

		void CompileClosureCall(CCompileEnvironment& ce, cr_sex s, cstr aVarName, const IStructure& type)
		{
			const IArchetype& callee = *type.Archetype();
			cstr callerName = ce.Builder.Owner().Name();

			int mapIndex = GetMapIndex(s, 1, callee.NumberOfOutputs(), callee.NumberOfInputs());

			int outputAndSFstackAllocByteCount = CompileCloseCallHeader(ce, s, callee, aVarName);
			int inputStackAllocByteCount = CompileClosureCallKernel(ce, s, callee, aVarName);

			RepairStack(ce, s, callee, 1);

			int outputOffset = GetOutputSFOffset(ce, inputStackAllocByteCount, outputAndSFstackAllocByteCount);

			PopOutputs(ce, s, callee, outputOffset, false);
			ce.Builder.AssignClosureParentSFtoD6();
		}

		void FormatWithCallDescription(const IArchetype& callee, char* message, size_t capacity, cstr extraInfo)
		{
			StackStringBuilder sb(message, capacity);
			sb << extraInfo << "\nExpecting: (" << callee.Name();

			for (int i = 0; i < callee.NumberOfInputs() - 1; ++i)
			{
				auto& argType = callee.GetArgument(i);
				sb << "(" << GetFriendlyName(argType) << " ";
				sb << callee.GetArgName(i) << ")";
			}

			sb << "->";

			for (int i = 0; i < callee.NumberOfOutputs(); ++i)
			{
				int index = i + callee.NumberOfInputs();
				auto& argType = callee.GetArgument(index);
				sb << "(" << GetFriendlyName(argType) << " ";
				sb << callee.GetArgName(index) << ")";
			}

			sb << ")";
		}

		void CompileVirtualCall(CCompileEnvironment& ce, const IArchetype& callee, cr_sex s, int interfaceIndex, int methodIndex, cstr instanceName, const IObjectInterface& interfaceRef)
		{
			// (<instance.method-name> input1 input2 input3.... inputN -> output1...output2...outputN)
			cstr calleeName = callee.Name();
			cstr callerName = ce.Builder.Owner().Name();

			int mapIndex;

			try
			{
				mapIndex = GetMapIndex(s, 1, callee.NumberOfOutputs(), callee.NumberOfInputs() - 1);
			}
			catch (IException& ex)
			{
				char message[2048];
				FormatWithCallDescription(callee, message, sizeof message, ex.Message());
				Throw(s, message);
			}

			int nSuppliedInputs = mapIndex - 1;
			if (nSuppliedInputs > callee.NumberOfInputs() - 1)
			{
				char message[2048];
				FormatWithCallDescription(callee, message, sizeof message, "More inputs were supplied than are needed by the function call");
				Throw(s, message);
			}

			int outputOffset = CompileVirtualCallKernel(ce, false, callee, s, interfaceIndex, methodIndex, instanceName, interfaceRef);
			PopOutputs(ce, s, callee, outputOffset, true);
			ce.Builder.AssignClosureParentSFtoD6();
		}

		int CompileInstancePointerArg(CCompileEnvironment& ce, cstr classInstance)
		{
			MemberDef def;
			ce.Builder.TryGetVariableByName(OUT def, classInstance);

			ce.Builder.AddSymbol(classInstance);

			AddArgVariable("instance", ce, ce.Object.Common().TypePointer());

			ce.Builder.AssignVariableRefToTemp(classInstance, 0);
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);

			return (int)sizeof(void*);
		}

		int CompileInstancePointerArgFromTemp(CCompileEnvironment& ce, int tempDepth)
		{
			ce.Builder.AddSymbol("// reference to instance");
			AddArgVariable("instance", ce, ce.Object.Common().TypePointer());
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4 + tempDepth, BITCOUNT_POINTER);
			return (int)sizeof(void*);
		}

		void CompileFunctionCall(CCompileEnvironment& ce, IFunction& callee, cr_sex s)
		{
			cstr calleeName = callee.Name();
			cstr callerName = ce.Builder.Owner().Name();

			int mapIndex = GetMapIndex(s, 1, callee.NumberOfOutputs(), callee.NumberOfInputs());

			if (mapIndex >= s.NumberOfElements())
			{
				if (s.NumberOfElements() - 1 > callee.NumberOfInputs())
				{
					Throw(s, "Too many inputs");
				}
			}

			if (mapIndex + callee.NumberOfOutputs() > s.NumberOfElements())
			{
				// More outputs that we have supplied, some will be skipped
			}

			// (<function-name> input1 input2 input3.... inputN -> output1...output2...outputN)
			int outputOffset = CompileFunctionCallKernel(ce, s, callee);
			PopOutputs(ce, s, callee, outputOffset, false);
			ce.Builder.AssignClosureParentSFtoD6();
		}

		void ValidateAssignment(cr_sex callDef)
		{
			bool isAssignment = false;

			cr_sex sParent = *callDef.Parent();
			for (int32 i = 1; i < sParent.NumberOfElements(); ++i)
			{
				if (&sParent[i] == &callDef)
				{
					cr_sex sAssign = sParent[i - 1];
					if (IsAtomic(sAssign) && Eq(sAssign.c_str(), "="))
					{
						return;
					}
					break;
				}
			}

			if (!isAssignment)
			{
				Throw(callDef, "Expecting assignment, as the function returns a value that must be referenced.");
			}
		}

		bool TryCompileFunctionCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, SexyVarType type, const IStructure* derivedType, const IArchetype* returnArchetype)
		{
			if (type == SexyVarType_Array)
			{
				ValidateAssignment(s);
			}

			cr_sex firstArg = IsCompound(s) ? s.GetElement(0) : s;

			if (IsAtomic(firstArg))
			{
				cstr fname = firstArg.c_str();

				if (IsCapital(fname[0]))
				{
					return TryCompilePlainFunctionCallAndReturnValue(ce, s, type, derivedType, returnArchetype);
				}
				else
				{
					const IStructure* st = ce.Builder.GetVarStructure(fname);
					if (st != NULL)
					{
						const IArchetype* a = st->Archetype();
						if (a != NULL)
						{
							CompileClosureCallAndReturnValue(ce, s, fname, type, returnArchetype);
							return true;
						}
						else if (st == &ce.StructArray())
						{
							if (s.NumberOfElements() == 2)
							{
								CompileGetArrayElement(ce, s.GetElement(1), fname, type, derivedType);
							}
							else if (s.NumberOfElements() == 3)
							{
								CompileGetArraySubelement(ce, s.GetElement(1), GetAtomicArg(s, 2), fname, type, derivedType);
							}
							else
							{
								return false;
							}
							return true;
						}
						else
						{
							if (st->InterfaceCount() > 0)
							{
								// Could be an implicit method
								return TryCompileMethodCallAndReturnValue(ce, s, type, derivedType, returnArchetype);
							}
						}
					}
					else
					{
						return TryCompileMethodCallAndReturnValue(ce, s, type, derivedType, returnArchetype);
					}
				}
			}

			return false;
		}

		const IStructure* GuessTypeAtomicLiteral(CCompileEnvironment& ce, cr_sex arg)
		{
			using namespace Rococo::Parse;

			cstr value = arg.c_str();

			if (ContainsPoint(value))
			{
				float32 valuef32;
				if (TryParseFloat(OUT valuef32, value) == PARSERESULT_GOOD)
				{
					return &ce.Object.Common().TypeFloat32();
				}

				float64 valuef64;
				if (TryParseFloat(OUT valuef64, value) == PARSERESULT_GOOD)
				{
					return &ce.Object.Common().TypeFloat64();
				}
			}
			else
			{
				if (AreEqual(value, ("0x"), 2))
				{
					int32 value32;
					if (TryParseHex(OUT value32, value + 2) == PARSERESULT_GOOD)
					{
						return &ce.Object.Common().TypeInt32();
					}

					int64 value64;
					if (TryParseHex(OUT value64, value + 2) == PARSERESULT_GOOD)
					{
						return &ce.Object.Common().TypeInt64();
					}

					return nullptr;
				}

				int32 value32;
				if (TryParseDecimal(OUT value32, value + 2) == PARSERESULT_GOOD)
				{
					return &ce.Object.Common().TypeInt32();
				}

				int64 value64;
				if (TryParseDecimal(OUT value64, value + 2) == PARSERESULT_GOOD)
				{
					return &ce.Object.Common().TypeInt64();
				}
			}

			return nullptr;
		}

		const IStructure* GuessTypeAtomic(CCompileEnvironment& ce, cr_sex arg)
		{
			using namespace Rococo::Parse;

			cstr value = arg.c_str();

			char c = value[0];
			int firstDigit;
			if (TryGetDigit(firstDigit, c) || c == '.' || c == '+' || c == '-')
			{
				return GuessTypeAtomicLiteral(ce, arg);
			}

			if (IsCapital(c))
			{
				IFunction* f = MatchFunction(arg, ce.Builder.Module());

				if (f == NULL) return NULL;

				if (f->NumberOfOutputs() == 0) Throw(arg, ("The function has no output"));
				if (f->NumberOfOutputs() > 1) Throw(arg, ("The function has more than one output"));
				if (f->NumberOfInputs() > 0) Throw(arg, ("The function requires an input"));

				return &f->GetArgument(0);
			}
			else
			{
				const IStructure* s = ce.Builder.GetVarStructure(value);
				if (s != NULL) return s;

				NamespaceSplitter splitter(value);
				cstr instance, method;
				if (!splitter.SplitTail(OUT instance, OUT method))
				{
					return NULL;
				}

				s = ce.Builder.GetVarStructure(instance);
				if (s == NULL) return NULL;

				int interfaceIndex, methodIndex;
				if (!GetMethodIndices(OUT interfaceIndex, OUT methodIndex, *s, method))
				{
					return NULL;
				}

				const IObjectInterface& interf = s->GetInterface(interfaceIndex);
				const IArchetype& methodArch = interf.GetMethod(methodIndex);

				if (methodArch.NumberOfOutputs() == 0) Throw(arg, ("The method has no output"));
				if (methodArch.NumberOfOutputs() > 1) Throw(arg, ("The method has more than one output"));
				if (methodArch.NumberOfInputs() > 2) Throw(arg, ("The method requires an explicit input"));

				return &methodArch.GetArgument(0);
			}
		}

		const IStructure* GuessTypeCompound(CCompileEnvironment& ce, cr_sex s)
		{
			using namespace Rococo::Parse;

			if (s.NumberOfElements() == 0) return NULL;

			if (s.NumberOfElements() == 1)
			{
				cr_sex tokenExpr = s.GetElement(0);
				if (IsCompound(tokenExpr))
				{
					return GuessTypeCompound(ce, tokenExpr);
				}
				else if (IsAtomic(tokenExpr))
				{
					cstr value = tokenExpr.c_str();
					char c = value[0];
					int firstDigit;
					if (TryGetDigit(firstDigit, c) || c == '.' || c == '+' || c == '-')
					{
						return GuessTypeAtomicLiteral(ce, tokenExpr);
					}
				}
				else
				{
					return NULL;
				}
			}

			cr_sex tokenExpr = s.GetElement(0);
			if (!IsAtomic(tokenExpr))
			{
				return NULL;
			}

			cstr token = tokenExpr.c_str();

			if (IsCapital(token[0]))
			{
				IFunction* f = MatchFunction(tokenExpr, ce.Builder.Module());

				if (f == NULL) return NULL;

				if (f->NumberOfOutputs() == 0) Throw(tokenExpr, ("The function has no output"));
				if (f->NumberOfOutputs() > 1) Throw(tokenExpr, ("The function has more than one output"));

				return &f->GetArgument(0);
			}
			else
			{
				const IStructure* st = ce.Builder.GetVarStructure(token);
				if (st != NULL)
				{
					if (st == &ce.StructArray() && s.NumberOfElements() == 2)
					{
						return &GetElementTypeForArrayVariable(ce, s, token);
					}
					else
					{
						return st;
					}
				}

				NamespaceSplitter splitter(token);
				cstr instance, method;
				if (!splitter.SplitTail(OUT instance, OUT method))
				{
					return NULL;
				}

				st = ce.Builder.GetVarStructure(instance);
				if (st == NULL) return NULL;

				int interfaceIndex, methodIndex;
				if (!GetMethodIndices(OUT interfaceIndex, OUT methodIndex, *st, method))
				{
					return NULL;
				}

				const IObjectInterface& interf = st->GetInterface(interfaceIndex);
				const IArchetype& methodArch = interf.GetMethod(methodIndex);

				if (methodArch.NumberOfOutputs() == 0) Throw(tokenExpr, ("The method has no output"));
				if (methodArch.NumberOfOutputs() > 1) Throw(tokenExpr, ("The method has more than one output"));

				return &methodArch.GetArgument(0);
			}
		}

		const IStructure* GuessType(CCompileEnvironment& ce, cr_sex arg)
		{
			switch (arg.Type())
			{
			case EXPRESSION_TYPE_ATOMIC:
				return GuessTypeAtomic(ce, arg);
			case EXPRESSION_TYPE_STRING_LITERAL:
				return &ce.Object.Common().SysTypeIString().NullObjectType();
			case EXPRESSION_TYPE_COMPOUND:
				return GuessTypeCompound(ce, arg);
			default:
				return nullptr;
			}
		}

		// We have a dispatch interface, marked with (attribute dispatch).
		// If it is used to call a method not part of the interface, then type info is used to route to the correct method
		// Generally very useful for writing strongly typed eventing interfaces.
		// Since an interface ref can be dynamically assigned to different objects, we have to determine whether methods exist at runtime
		void CompileDynamicDispatch(CCompileEnvironment& ce, cr_sex s, const IObjectInterface& dispatcher, cstr instanceName, cstr methodName)
		{
			if (s.NumberOfElements() != 2)
			{
				Throw(s, "Cannot dispatch method %s. Dispatch methods have one and only one argument.", methodName);
			}

			cr_sex arg = s[1];
			if (arg.Type() != EXPRESSION_TYPE_ATOMIC)
			{
				Throw(arg, "Cannot dispatch call. Argument needs to be an atomic variable");
			}

			cstr argText = arg.c_str();

			MemberDef argDef;
			if (!ce.Builder.TryGetVariableByName(argDef, argText))
			{
				Throw(arg, "Cannot dispatch call. Argument needs to be a variable");
			}

			if (Eq(methodName, "Destruct") || Eq(methodName, "Construct"))
			{
				Throw(arg, "Cannot dispatch call. Method must not be Construct or Destruct");
			}

			if (argDef.ResolvedType->VarType() != SexyVarType_Derivative || argDef.ResolvedType->InterfaceCount() > 0)
			{
				Throw(arg, "Cannot dispatch call. Argument variable needs to be a structure");
			}

			MemberDef instanceDef;
			if (!ce.Builder.TryGetVariableByName(instanceDef, instanceName))
			{
				Throw(arg, "Cannot dispatch call. %s is not a variable", instanceName);
			}

			AddSymbol(ce.Builder, "Dispatch %s.%s", instanceName, methodName);

			ce.Builder.PushVariableRef(argText, -1);
			ce.Builder.PushVariable(instanceDef);

			// The three arguments below are popped off by the dynamic dispatch callback

			VariantValue srcValue;
			srcValue.vPtrValue = (ISExpression*)&s;
			ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_POINTER, srcValue);

			VariantValue methodNameValue;
			methodNameValue.charPtrValue = (char*)ce.SS.GetPersistentString(methodName);
			ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_POINTER, methodNameValue);

			VariantValue argTypeValue;
			argTypeValue.vPtrValue = (void*)argDef.ResolvedType;
			ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_POINTER, argTypeValue);

			ce.Builder.Assembler().Append_Invoke(ce.SS.GetScriptCallbacks().idDynamicDispatch);

			ce.Builder.Assembler().Append_Pop(sizeof(size_t) * 2);
		}

		void PushPrivateInstance(CCompileEnvironment& ce, cr_sex s, const MemberDef& instanceDef)
		{
			// Scencarios -> we are calling the method from an interface, or from a class

			ce.Builder.PushVariable(instanceDef);
		}

		bool TryCompilePrivateMethodCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName)
		{
			MemberDef def;
			if (!ce.Builder.TryGetVariableByName(OUT def, IN instanceName))
			{
				return false;
			}

			auto& c = *def.ResolvedType;

			auto& classModule = c.Module();

			char fullMethodName[128];
			SafeFormat(fullMethodName, sizeof(fullMethodName), "%s.%s", c.Name(), methodName);

			auto* method = classModule.FindFunction(fullMethodName);

			if (!method) return false;

			if (def.IsContained)
			{
				Throw(s, "Calling a private method on an instance referenced on a structure is not implemented. Copy instance to a local variable first");
			}

			if (def.IsParentValue)
			{
				Throw(s, "Calling a private method on an instance referenced via a parent variable is not implemented. Copy instance to a local variable first");
			}

			const int outputStackAllocCount = AllocFunctionOutput(ce, *method, s);
			int inputStackAllocCount = PushInputs(ce, s, *method, true, 1);

			ce.Builder.Assembler().Append_GetStackFrameValue(def.SFOffset, VM::REGISTER_D4, BITCOUNT_POINTER);
			ce.Builder.Assembler().Append_IncrementPtr(VM::REGISTER_D4, def.MemberOffset);
			ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);

			AddArgVariable("_instance", ce, ce.Object.Common().TypePointer());

			inputStackAllocCount += sizeof(size_t);

			AppendFunctionCallAssembly(ce, *method);
			ce.Builder.MarkExpression(&s);
			RepairStack(ce, s, *method);

			int outputOffset = GetOutputSFOffset(ce, inputStackAllocCount, outputStackAllocCount);

			PopOutputs(ce, s, *method, outputOffset, true);
			ce.Builder.AssignClosureParentSFtoD6();

			return true;
		}

		// Only safe to call from TryCompileAsFunctionCall(...)
		bool TryCompileAsMethodCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName)
		{
			MemberDef def;
			if (!ce.Builder.TryGetVariableByName(OUT def, IN instanceName))
			{
				return false;
			}

			const IStructure& c = *def.ResolvedType;

			switch (c.VarType())
			{
			case SexyVarType_Array:
				if (!TryCompileAsArrayCall(ce, s, instanceName, methodName))
				{
					Throw(s, "%s is an array, but method name unknown. Check log above", instanceName);
				}
				else
				{
					return true;
				}
			case SexyVarType_List:
				if (!TryCompileAsListCall(ce, s, instanceName, methodName))
				{
					Throw(s, "%s is a list, but method name unknown. Check log above", instanceName);
				}
				else
				{
					return true;
				}
			case SexyVarType_Map:
				if (!TryCompileAsMapCall(ce, s, instanceName, methodName))
				{
					Throw(s, "%s is a map, but method name unknown. Check log above", instanceName);
				}
				else
				{
					return true;
				}
			case SexyVarType_Derivative:
				break;
			default:
				return false;
			}

			if (c == ce.Object.Common().TypeMapNode())
			{
				return TryCompileAsMapNodeCall(ce, s, instanceName, methodName);
			}

			if (c == ce.Object.Common().TypeNode())
			{
				return TryCompileAsNodeCall(ce, s, instanceName, methodName);
			}

			if (!c.Prototype().IsClass)
			{
				Throw(s, ("Only classes, containers and nodes support methods."));
			}

			OUT int interfaceIndex, OUT methodIndex;
			if (GetMethodIndices(OUT interfaceIndex, OUT methodIndex, c, methodName))
			{
				const IObjectInterface& interf = c.GetInterface(interfaceIndex);
				const IArchetype& method = interf.GetMethod(methodIndex);
				CompileVirtualCall(ce, method, s, interfaceIndex, methodIndex, instanceName, interf);
				return true;
			}

			const void* unused;
			if (IsNullType(c) && c.GetInterface(0).Attributes().FindAttribute("dispatch", unused))
			{
				CompileDynamicDispatch(ce, s, c.GetInterface(0), instanceName, methodName);
				return true;
			}

			if (TryCompilePrivateMethodCall(ce, s, instanceName, methodName))
			{
				return true;
			}

			auto sDef = (const Sex::ISExpression*) def.ResolvedType->Definition();
			Throw(s, "Could not find method %s in %s of %s (near line %d)", methodName, GetFriendlyName(*def.ResolvedType), def.ResolvedType->Module().Name(), sDef->Start().y);
		}

		bool TryCompileAsBuilderCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName)
		{
			cr_sex arg = s.GetElement(1);
			const IStructure* argType = GuessType(ce, arg);
			if (argType == NULL)
			{
				auto* original = arg.Parent()->GetOriginal();
				cr_sex sHighlight = original ? *original : arg;

				if (original && IsAtomic(arg))
				{
					Throw(sHighlight, "%s: %s.%s: Cannot guess type of argument '%s'. Try assigning value to a variable and use the variable as the argument.", __ROCOCO_FUNCTION__, instanceName, methodName, arg.c_str());
				}
				else
				{
					Throw(sHighlight, "%s: %s.%s: Cannot guess type of argument. Try assigning value to a variable and use the variable as the argument.", __ROCOCO_FUNCTION__, instanceName, methodName);
				}
			}

			cstr typeName = GetFriendlyName(*argType);

			TokenBuffer fullMethodName;
			StringPrint(fullMethodName, ("%s%s"), methodName, typeName);

			return TryCompileAsMethodCall(ce, s, instanceName, fullMethodName);
		}

		bool TryCompileAsPlainFunctionCallWithFQN(CCompileEnvironment& ce, cstr body, cstr tail, cr_sex s)
		{
			INamespaceBuilder* ns = Compiler::MatchNamespace(GetModule(ce.Script), body);
			if (ns == NULL)
			{
				Throw(s, "Could not find namespace: %s", body);
			}

			IFunctionBuilder* f = ns->FindFunction(tail);
			if (f != NULL)
			{
				CompileFunctionCall(ce, *f, s);
				return true;
			}
			else
			{
				return false;
			}
		}

		bool TryCompileAsPlainFunctionCallWithPrefix(CCompileEnvironment& ce, cstr fname, cr_sex s)
		{
			IModuleBuilder& module = GetModule(ce.Script);

			IFunction* f = NULL;
			INamespace* NS = NULL;

			for (int i = 0; i < module.PrefixCount(); ++i)
			{
				INamespaceBuilder& prefix = module.GetPrefix(i);
				IFunction* g = prefix.FindFunction(fname);
				if (f == NULL)
				{
					f = g;
					NS = &prefix;
				}
				else if (g != NULL)
				{
					Throw(s, "Ambiguity: '%s' could belong to %s or %s", fname, prefix.FullName()->Buffer, NS->FullName()->Buffer);
				}
			}

			if (f != NULL)
			{
				CompileFunctionCall(ce, *f, s);
				return true;
			}
			else
			{
				return false;
			}
		}

		bool TryCompileAsPlainFunctionCall(CCompileEnvironment& ce, cr_sex s)
		{
			if (!IsCompound(s) || s.NumberOfElements() == 0) return false;

			cr_sex nameExpr = GetAtomicArg(s, 0);
			cstr fname = nameExpr.c_str();

			NamespaceSplitter splitter(fname);

			cstr body, tail;
			if (splitter.SplitTail(OUT body, OUT tail))
			{
				return TryCompileAsPlainFunctionCallWithFQN(ce, body, tail, s);
			}
			else
			{
				IFunction* f = ce.Builder.Module().FindFunction(fname);
				if (f != NULL)
				{
					CompileFunctionCall(ce, *f, s);
					return true;
				}
				else
				{
					return TryCompileAsPlainFunctionCallWithPrefix(ce, fname, s);
				}
			}
		}

		bool TryCompileAsDerivativeFunctionCall(CCompileEnvironment& ce, cr_sex s)
		{
			if (!IsCompound(s) || s.NumberOfElements() == 0) return false;

			cr_sex nameExpr = GetAtomicArg(s, 0);
			cstr fname = nameExpr.c_str();

			NamespaceSplitter splitter(fname);

			cstr instance, fnName;
			if (splitter.SplitTail(OUT instance, OUT fnName))
			{
				if (IsCapital(fnName[0]))
				{
					return TryCompileAsMethodCall(ce, s, instance, fnName);
				}
			}

			// A variable on its own could be an archetype invocation, or be associated with implicit methods
			const IStructure* type = ce.Builder.GetVarStructure(fname);
			if (type != NULL)
			{
				if (IsNullType(*type))
				{
					const IObjectInterface& interf = type->GetInterface(0);
					if (s.NumberOfElements() == 2)
					{
						const ISExpression* attr;
						if (interf.Attributes().FindAttribute("indexed", (const void*&)attr))
						{
							cr_sex indexMethodExpr = GetAtomicArg(*attr, 3);
							cstr indexMethodName = indexMethodExpr.c_str();
							return TryCompileAsMethodCall(ce, s, fname, indexMethodName);
						}
						else if (interf.Attributes().FindAttribute("builder", (const void*&)attr))
						{
							cr_sex appendPrefixExpr = GetAtomicArg(*attr, 2);
							cstr appendPrefix = appendPrefixExpr.c_str();
							return TryCompileAsBuilderCall(ce, s, fname, appendPrefix);
						}
					}
				}

				if (type == &ce.StructArray())
				{
					Throw(s, "A directive that consists of an array name followed by optional arguments is illegal");
				}

				const IArchetype* a = type->Archetype();
				if (a != NULL)
				{
					CompileClosureCall(ce, s, fname, *type);
					return true;
				}
			}

			return false;
		}

		void AddArchiveRegister(CCompileEnvironment& ce, int saveTempDepth, int restoreTempDepth, BITCOUNT bits)
		{
			char declText[256];
			SafeFormat(declText, 256, ("save D%d. restore to D%d"), saveTempDepth + 4, restoreTempDepth + 4);
			ce.Builder.AddSymbol(declText);
			ce.Builder.ArchiveRegister(saveTempDepth, restoreTempDepth, bits, (void*)GetTryCatchExpression(ce.Script));
		}

		void AddVariable(CCompileEnvironment& ce, const NameString& ns, const TypeString& ts)
		{
			ce.Builder.AddVariable(ns, ts, (void*)GetTryCatchExpression(ce.Script));
		}

		INamespaceBuilder& GetNamespaceByFQN(CCompileEnvironment& ce, cstr ns, cr_sex s)
		{
			INamespaceBuilder* NS = ce.RootNS.FindSubspace(ns);
			if (NS == NULL)
			{
				Throw(s, "Could not find the namespace %s", ns);
			}
			return *NS;
		}

		const IFunction& GetConstructor(const IStructure& st, cr_sex s)
		{
			const IFunction* constructor = st.Constructor();
			if (constructor == NULL)
			{
				Throw(s, "Cannot find %s.Construct in ", st.Name(), st.Module().Name());
			}

			return *constructor;
		}

		IFunction& GetFunctionByFQN(CCompileEnvironment& ce, cr_sex s, cstr name)
		{
			cstr ns, shortName;
			NamespaceSplitter splitter(name);
			if (!splitter.SplitTail(ns, shortName))
			{
				Throw(s, ("Expecting fully qualified name"));
			}

			INamespaceBuilder& NS = GetNamespaceByFQN(ce, ns, s);

			IFunctionBuilder* f = NS.FindFunction(shortName);
			if (f == NULL)
			{
				Throw(s, "Could not find %s in %s", shortName, ns);
			}

			return *f;
		}
	}// Script
}//Sexy 