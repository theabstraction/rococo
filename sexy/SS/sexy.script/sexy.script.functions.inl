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

namespace Rococo
{ 
   namespace Script
   {
      int PushInputs(CCompileEnvironment& ce, cr_sex s, const IArchetype& callee, bool isImplicitInput, int firstArgIndex)
      {
	      int inputStackAllocCount = 0;

	      int virtualIndex =  isImplicitInput ? 1 : 0;

	      int inputCount = callee.NumberOfInputs() - virtualIndex;

	      if (inputCount > 0)
	      {
		      if (inputCount + firstArgIndex - 1 >= s.NumberOfElements())
		      {
			      Throw(s, ("Insufficient inputs"));
		      }
	      }

	      for(int i = 0; i < inputCount; ++i)
	      {
		      cr_sex inputExpression = s.GetElement(i + firstArgIndex);

		      int argIndex = i + callee.NumberOfOutputs();

		      cstr inputName = callee.GetArgName(argIndex);
		      const IStructure& argType = callee.GetArgument(argIndex); 
		      const IArchetype* archetype;

		      if (argType.VarType() != VARTYPE_Closure)
		      {
			      archetype = NULL;
		      }
		      else
		      {
			      archetype = argType.Archetype();
		      }

		      int inputStackCost = PushInput(ce, s, i + firstArgIndex, argType, archetype, inputName, callee.GetGenericArg1(argIndex));

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
		      if (s.NumberOfElements() - firstIndex < inputExpressionCount) Throw(s, ("Too few inputs to function call"));
		      mapIndex = s.NumberOfElements();
	      }
	      else
	      {
		      if (mapIndex < inputExpressionCount + 1) Throw(s, ("Too few inputs in function call"));
		      else if (mapIndex > inputExpressionCount + 1) Throw(s, ("Too many inputs in function call"));
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
		      ce.Builder.GetVariableByIndex(OUT lastDef, lastName, ce.Builder.GetVariableCount()-1);

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
		      Throw(s, ("Too many input arguments supplied to the function"));
	      }
	      else if (inputCount >nExtraElements)
	      {
		      Throw(s, ("Too few input arguments supplied to the function"));
	      }
      }

      bool TryCompilePushClosure(CCompileEnvironment& ce, cr_sex s, bool expecting, const IStructure& inputType, const IArchetype& archetype, cstr argName)
      {
	      AssertAtomic(s);

	      cstr fname = s.String()->Buffer;

	      AddArgVariable(("input_other"), ce, inputType);
	      ce.Builder.AddSymbol(argName); 

	      IFunction* f = ce.Builder.Module().FindFunction(fname);
	      if (f != NULL)
	      {
		      ValidateArchetypeMatchesArchetype(s, *f, archetype, ("archetype "));

		      CodeSection section;
		      f->Code().GetCodeSection(OUT section);

		      VariantValue fnRef;
		      fnRef.byteCodeIdValue = section.Id;
		      ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_64, fnRef);

		      VariantValue zeroRef;
		      zeroRef.charPtrValue = nullptr;
		      ce.Builder.Assembler().Append_PushLiteral(BITCOUNT_64, zeroRef);
	      }
	      else if (AreEqual(("0"), fname))
	      {
		      auto& f = GetNullFunction(ce.Script, archetype);

		      CodeSection section;
		      f.Code().GetCodeSection(OUT section);

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
			      sexstringstream<1024> streamer;
			      streamer.sb << ("Failed to interpret expression as a closure argument: ") << inputType.Name() << (" ") << argName;
			      Throw(s, streamer);
		      }
		

		      ce.Builder.PushVariable(def);
	      }

	      return true;
      }

      void CompileInputRefFromFunctionRef(CCompileEnvironment& ce, cr_sex inputExpression, const IStructure& argStruct)
      {
	      const IFunction* f = MatchFunction(inputExpression, ce.Builder.Module());

	      if (f == NULL)
	      {
		      cstr name = inputExpression.String()->Buffer;

		      sexstringstream<1024> streamer;
		      streamer.sb << ("The input '") << name << ("' was not recognized ");
		      Throw(inputExpression, streamer);
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
	      if (!TryCompileFunctionCallAndReturnValue(ce, inputExpression, VARTYPE_Derivative, &argStruct, NULL))
	      {
		      cstr name = inputExpression.String()->Buffer;

		      sexstringstream<1024> streamer;
		      streamer.sb << ("The input '") << name << ("' was not recognized ");
		      Throw(inputExpression, streamer);
	      }

	      AddArgVariable(("input_getaccessor_return_ref"), ce, argStruct);
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
	      SafeFormat(debugInfo, 256, format, (cstr) value->Buffer);
	      ce.Builder.AddSymbol(debugInfo);

	      VariantValue ptr;
	      ptr.vPtrValue = (void*) &sc->header.pVTables[0];

	      AddArgVariable(("input_string_literal"), ce, inputType);
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
	      AddSymbol(ce.Builder, format, (cstr) value->Buffer);

	      VariantValue ptr;
	      ptr.vPtrValue = (void*) &sc->header.pVTables[0];

	      ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4 + tempDepth, ptr, BITCOUNT_POINTER);	

	      return true;
      }


      void CompileGetStructRefFromVariable(CCompileEnvironment& ce, cr_sex s, const IStructure& inputType, cstr name, const MemberDef& def)
      {
	      cstr varName = s.String()->Buffer;
	      const IStructure& varStruct = *def.ResolvedType;

	      VARTYPE vType = varStruct.VarType();
	      if (vType != VARTYPE_Derivative)
	      {
		      sexstringstream<1024> streamer;
		      streamer.sb << ("The variable is not a derived type. Expected: ") << GetFriendlyName(inputType) << (" ") << name;
		      Throw(s, streamer);
	      }

		  if (varStruct.Prototype().IsClass)
		  {
			  int cii = GetCommonInterfaceIndex(varStruct, inputType);
			  if (cii < 0)
			  {
				  sexstringstream<1024> streamer;
				  streamer.sb << ("The input type '") << varStruct.Name() << ("' did not match the argument type '") << GetFriendlyName(inputType) << (" ") << name << ("'");
				  Throw(s, streamer);
			  }
		  }
	      
		  ce.Builder.AddSymbol(varName);
		  ce.Builder.AssignVariableRefToTemp(varName, Rococo::ROOT_TEMPDEPTH, 0);						
      }

      void CompileGetStructRefFromAtomic(CCompileEnvironment& ce, cr_sex s, const IStructure& inputType, cstr name)
      {
	      cstr token = s.String()->Buffer;

	      if (islower(token[0]))
	      {
		      MemberDef def;
		      if (ce.Builder.TryGetVariableByName(def, token))
		      {
			      if (*def.ResolvedType != inputType)
			      {
				      sexstringstream<1024> streamer;
				      streamer.sb << ("The input ") << GetFriendlyName(inputType) << (" ") << name << (" did not match the input variable ") << GetFriendlyName(*def.ResolvedType) << (" ") << token;
				      Throw(s, streamer);
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
		      if (inputType.VarType() != VARTYPE_Closure)
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
		      sexstringstream<1024> streamer;
		      streamer.sb << ("Expecting an expression that returns a reference to ") << GetFriendlyName(inputType) << (" ") << name;
		      Throw(s, streamer);
	      }
      }

      void CompileGetStructRef(CCompileEnvironment& ce, cr_sex s, const IStructure& inputType, cstr name)
      {
	      switch(s.Type())
	      {
	      case EXPRESSION_TYPE_STRING_LITERAL:
		      {
			      sexstringstream<1024> streamer;
			      streamer.sb << ("Cannot yet handle string literal expression for ") << GetFriendlyName(inputType) << (" ") << name;
			      Throw(s, streamer);
		      }
		      break;
	      case EXPRESSION_TYPE_ATOMIC:
		      CompileGetStructRefFromAtomic(ce, s, inputType, name);
		      break;
	      case EXPRESSION_TYPE_COMPOUND:
		      if (!TryCompileFunctionCallAndReturnValue(ce, s, VARTYPE_Derivative, &inputType, NULL))
		      {
			      sexstringstream<1024> streamer;
			      streamer.sb << ("Expecting compound expression to return input for ") << GetFriendlyName(inputType) << (" ") << name;
			      Throw(s, streamer);
		      }			
		      break;
	      default:
		      {
			      sexstringstream<1024> streamer;
			      streamer.sb << ("Expecting atomic, compound or string literal expression for ") << GetFriendlyName(inputType) << (" ") << name;
			      Throw(s, streamer);
		      }
	      }
      }

      bool TryCompilePushStructRef(CCompileEnvironment& ce, cr_sex s, bool expectingStructRef, const IStructure& inputType, cstr name, const IStructure* genericArg1)
      {
	      switch(s.Type())
	      {
	      case EXPRESSION_TYPE_STRING_LITERAL:
		      return TryCompileStringLiteralInput(ce, s, expectingStructRef, inputType);
	      case EXPRESSION_TYPE_ATOMIC:
		      break;
	      case EXPRESSION_TYPE_COMPOUND:
		      if (!TryCompileFunctionCallAndReturnValue(ce, s, VARTYPE_Derivative, &inputType, NULL))
		      {
			      return false;
		      }			

		      AddArgVariable(("input_return_as_input_ref"), ce, inputType);
		      ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4 + ROOT_TEMPDEPTH, BITCOUNT_POINTER);
		      return true;
	      default:
		      if (expectingStructRef)
		      {
			      sexstringstream<1024> streamer;
			      streamer.sb << ("Expecting atomic, compound or string literal expression for ") << GetFriendlyName(inputType) << (" ") << name;
			      Throw(s, streamer);
		      }
		      return false;
	      }

	      cstr vname = s.String()->Buffer;

	      if (!Rococo::IsAlphabetical(vname[0]))
	      {
		      sexstringstream<1024> streamer;
		      streamer.sb << ("Could not interpret token as function or variable. Expected: ") << GetFriendlyName(inputType) << (" ") << name;
		      Throw(s, streamer);
	      }

	      MemberDef def;
	      if (!ce.Builder.TryGetVariableByName(OUT def, vname))
	      {
		      CompileInputRefFromGetAccessor(ce, s, inputType);
		      return true;
	      }

	      VARTYPE vType = def.ResolvedType->VarType();
	      if (vType != VARTYPE_Derivative)
	      {
		      if (expectingStructRef)
		      {
			      sexstringstream<1024> streamer;
			      streamer.sb << ("The variable is not a derived type. Expected: ") << GetFriendlyName(inputType) << (" ") << name;
			      Throw(s, streamer);
		      }
	      }

	      const IStructure* varStruct = def.ResolvedType;

	      int cii = GetCommonInterfaceIndex(*varStruct, inputType);
	      if (cii < 0)
	      {
		      if (expectingStructRef)
		      {
			      sexstringstream<1024> streamer;
			      streamer.sb << ("The input type '") << varStruct->Name() << ("' did not match the argument type '") << GetFriendlyName(inputType) << (" ") << name << ("'");
			      Throw(s, streamer);
		      }

		      return false;
	      }

	      ce.Builder.AddSymbol(vname);

	      if (varStruct == &ce.StructArray())
	      {
		      const IStructure& elementType = GetArrayDef(ce, s, vname);

		      if (&elementType != genericArg1)
		      {				
			      sexstringstream<1024> streamer;
			      streamer.sb << ("The input supplied was (array ") << GetFriendlyName(elementType) << (" ") << vname << (") ");

			      if (genericArg1 != NULL)
			      {
				      streamer.sb << ("but input required was (array ") << GetFriendlyName(*genericArg1) << (" ") << name << (") ");
			      }
			      else
			      {
				      streamer.sb << ("but input required was (") << GetFriendlyName(inputType) << (" ") << name << (") ");
			      }
			      Throw(s, streamer);
		      }
	      }

	      AddArgVariable(("input_variable_ref"), ce, inputType);
	      PushVariableRef(s, ce.Builder, def, vname, cii);
					
	      return true;
      }

      int PushInput(CCompileEnvironment& ce, cr_sex s, int index, const IStructure& inputStruct, const IArchetype* archetype, cstr inputName, const IStructure* genericArg1)
      {
	      cr_sex inputExpression = s.GetElement(index);

	      if (!IsPointerValid(&inputStruct))
	      {
		      Throw(inputExpression, ("Function input type has not been resolved"));
	      }

	      VARTYPE inputType = inputStruct.VarType();
	      BITCOUNT bits = GetBitCount(inputType);

	      int stackAllocByteCount = 0;

	      switch(bits)
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
		      Throw(s, ("Algorithmic error: unhandled bitcount in argument to function"));
	      }

	      if (IsAtomic(inputExpression))
	      {
		      // Try pushing direct, more efficient than evaluating to a temp variable then pushing the variable
		      cstr inputToken = inputExpression.String()->Buffer;
		      if (IsNumericTypeOrBoolean(inputType))
		      {
			      VariantValue immediateValue;
			      if (Parse::TryParse(OUT immediateValue, inputType, inputToken) == Parse::PARSERESULT_GOOD)
			      {
				      AddArgVariable(("input_literal"), ce, inputStruct);
				      ce.Builder.AddSymbol(inputName); 
				      ce.Builder.Assembler().Append_PushLiteral(bits, immediateValue);
				      return stackAllocByteCount;
			      }	
			      else
			      {
				      MemberDef def;
				      if (ce.Builder.TryGetVariableByName(OUT def, inputToken) && def.ResolvedType->VarType() == inputType)
				      {	
					      AddArgVariable(("input_variable"), ce, inputStruct);
					      ce.Builder.AddSymbol(inputName); 
					      ce.Builder.PushVariable(def);
					      return stackAllocByteCount;
				      }
			      }
		      }

	      }

	      if (inputType == VARTYPE_Bool)
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
			      sexstringstream<1024> streamer;
			      streamer.sb << ("Expected ") << GetTypeName(inputType) << (" valued expression");
			      Throw(inputExpression, streamer);
		      }
	      }
	      else if (inputType == VARTYPE_Derivative)
	      {
		      ce.Builder.AddSymbol(inputName); 
		      if (!TryCompilePushStructRef(ce, inputExpression, true, inputStruct, inputName, genericArg1))
		      {
			      sexstringstream<1024> streamer;
			      streamer.sb << ("Expected a reference to a ") << GetFriendlyName(inputStruct);
			      Throw(inputExpression, streamer);
		      }
		      return sizeof(size_t);
	      }
	      else if (inputType == VARTYPE_Closure)
	      {
		      if (!TryCompilePushClosure(ce, inputExpression, true, inputStruct, *archetype, inputName))
		      {
			      sexstringstream<1024> streamer;
			      streamer.sb << ("Expected an archetype ") << archetype->Name();
			      Throw(inputExpression, streamer);
		      }

		      return inputStruct.SizeOfStruct();
	      }

	      AddArgVariable(("input_other"), ce, inputStruct);
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
	      cstr outputVar = outputExpr.String()->Buffer;

	      MemberDef outputDef;
	      if (!builder.TryGetVariableByName(OUT outputDef, outputVar))
	      {
		      Throw(outputExpr, ("The output token was not a recognized variable"));
	      }

	      TokenBuffer symbol;
         SafeFormat(symbol.Text, TokenBuffer::MAX_TOKEN_CHARS, (" -> %s"), outputVar);
	      builder.AddSymbol(symbol);

	      const IStructure* exprOutputStruct = outputDef.ResolvedType;
	
	      if (&requiredOutputStruct != exprOutputStruct)
	      {
		      sexstringstream<1024> streamer;
		      streamer.sb << ("Function expects type ") << GetFriendlyName(requiredOutputStruct) << (" but identifier was of type ") << GetFriendlyName(*exprOutputStruct);
		      Throw(outputExpr, streamer);
	      }

	      const IArchetype* targetArchetype = exprOutputStruct->Archetype();
		
	      if (requiredOutputStruct.Archetype() != targetArchetype)
	      {
		      Throw(outputExpr, ("The archetype of the output variable did not match that supplied to the function call"));
	      }

	      if (requiredOutputStruct.VarType() == VARTYPE_Derivative)
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
		      if (outputDef.Usage == ARGUMENTUSAGE_BYVALUE && outputDef.MemberOffset == 0)
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

      int GetOutputStackByteCount(const IArchetype& a, cr_sex s)
      {
	      int total = 0;
	      for(int i = 0; i < a.NumberOfOutputs(); i++)
	      {
		      const IStructure& s = a.GetArgument(i);			
		      total += s.VarType() == VARTYPE_Derivative ? sizeof(size_t) : s.SizeOfStruct();
	      }

	      return total;
      }

      bool TryCompilePlainFunctionCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, VARTYPE type, const IStructure* derivedType, const IArchetype* returnArchetype)
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

      void ValidateCorrectOutput(cr_sex s, const IStructure& outputStruct, VARTYPE returnType)
      {
	      if (!IsPointerValid(&outputStruct))
	      {
		      Throw(s, ("Output structure was NULL. Internal algorithmic error."));
	      }

	      if (outputStruct.VarType() != returnType)
	      {
		      sexstringstream<1024> streamer;
		      streamer.sb << ("Function returns ") << GetTypeName(outputStruct.VarType()) << (" but expression expects ") << GetTypeName(returnType);
		      Throw(s, streamer);
	      }
      }

      void ValidateSingleOutput(cr_sex s, int outputCount)
      {
	      if (outputCount == 0)
		      Throw(s, ("Function has no output, hence no hence no return value"));

	      if (outputCount != 1)
		      Throw(s, ("Function has multiple outputs, and no specific return value"));
      }

      void ValidateSingleOutputAndType(CCompileEnvironment& ce, cr_sex s, const IArchetype& callee, VARTYPE returnType, const IArchetype* returnArchetype, const IStructure* returnTypeStruct)
      {
	      ValidateSingleOutput(s, callee.NumberOfOutputs());
	      const IStructure& argStruct = callee.GetArgument(0);
	      if (argStruct.Archetype() != returnArchetype)
	      {
		      Throw(s, ("The function does not return the correct archetype required by the function call"));
	      }

	      if (returnTypeStruct != NULL && returnTypeStruct != &argStruct)
	      {
		      sexstringstream<1024> streamer;
		      streamer.sb << ("Function returns ") <<  GetFriendlyName(argStruct) << (" but expression expects ") << GetFriendlyName(*returnTypeStruct);
		      Throw(s, streamer);
	      }

	      ValidateCorrectOutput(s, argStruct, returnType);
      }

      void ReturnOutput(CCompileEnvironment& ce, int outputOffset, VARTYPE returnType)
      {
	      if (returnType == VARTYPE_Closure)
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

      int AllocFunctionOutput(CCompileEnvironment& ce, const IArchetype& callee, cr_sex s)
      {
	      int outputStackAllocByteCount = GetOutputStackByteCount(callee, s);
	      if (outputStackAllocByteCount > 0)
	      {
		      int total = 0;
		      for(int i = 0; i < callee.NumberOfOutputs(); i++)
		      {
			      const IStructure& s = callee.GetArgument(i);			
			      AddArgVariable(("output"), ce, s);
		      }

		      char stackAllocHint[256];
            SafeFormat(stackAllocHint, 256, ("Output for %s"), callee.Name());
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

      int CompileVirtualCallKernel(CCompileEnvironment& ce, bool callAtomic, const IArchetype& callee, cr_sex s, int interfaceIndex, int methodIndex, cstr instanceName, const IInterface& interfaceRef)
      {
	      int outputStackAllocByteCount = AllocFunctionOutput(ce, callee, s);

	      int inputStackAllocCount = callAtomic ? 0 : PushInputs(ce, s, callee, true, 1);
	      inputStackAllocCount += sizeof(size_t); // Allow a few bytes for the instance pointer

	      AddArgVariable(("input_interface"), ce, ce.Object.Common().TypePointer());
	      AppendVirtualCallAssembly(instanceName, interfaceIndex, methodIndex, ce.Builder, interfaceRef, s);
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

      int CompileFunctionCallKernel(CCompileEnvironment& ce, cr_sex s, const IFunction& callee)
      {
	      const int outputStackAllocCount = AllocFunctionOutput(ce, callee, s);
	      int inputStackAllocCount = PushInputs(ce, s, callee, false, 1);
	      AppendFunctionCallAssembly(ce, callee);
	      ce.Builder.MarkExpression(&s);
	      RepairStack(ce, s, callee);

	      int outputOffset = GetOutputSFOffset(ce, inputStackAllocCount, outputStackAllocCount);
	      return outputOffset;
      }

      void CompileFunctionCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, IFunction& callee, VARTYPE returnType, const IArchetype* returnArchetype, const IStructure* returnTypeStruct)
      {
	      cstr calleeName = callee.Name();
	      cstr callerName = ce.Builder.Owner().Name();

	      ValidateSingleOutputAndType(ce, s, callee, returnType, returnArchetype, returnTypeStruct);
	      ValidateInputCount(s, ArgCount(callee) - 1);

	      int outputOffset = CompileFunctionCallKernel(ce, s, callee);
	      ReturnOutput(ce, outputOffset, returnType);
	      ce.Builder.AssignClosureParentSF();
      }

      void CompileVirtualCallAndReturnValue(CCompileEnvironment& ce, bool callAtomic, const IArchetype& callee, cr_sex s, int interfaceIndex, int methodIndex, cstr instanceName, VARTYPE returnType, const IStructure* returnTypeStruct, const IArchetype* returnArchetype, const IInterface& interfaceRef)
      {
	      cstr calleeName = callee.Name();
	      cstr callerName = ce.Builder.Owner().Name();

	      // (<instance.method-name> input1 input2 input3.... inputN -> output1...output2...outputN)
	      ValidateSingleOutputAndType(ce, s, callee, returnType, returnArchetype, returnTypeStruct);
	      ValidateNumberOfInputArgs(s, callee, callAtomic ? 0 : s.NumberOfElements());

	      int outputOffset = CompileVirtualCallKernel(ce, callAtomic, callee, s, interfaceIndex, methodIndex, instanceName, interfaceRef);
	      ReturnOutput(ce, outputOffset, returnType);	
	      ce.Builder.AssignClosureParentSF();
      }

      int CompileCloseCallHeader(CCompileEnvironment& ce, cr_sex s, const IArchetype& callee, cstr closureVariable)
      {
	      TokenBuffer parentSF;
         SafeFormat(parentSF.Text, TokenBuffer::MAX_TOKEN_CHARS, ("%s.parentSF"), closureVariable);

	      MemberDef def;
	      ce.Builder.TryGetVariableByName(OUT def, parentSF);
	      ce.Builder.AddSymbol(parentSF);

	      AddArgVariable(("closure_parentSF"), ce, *def.ResolvedType);
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
         SafeFormat(pathToId.Text, TokenBuffer::MAX_TOKEN_CHARS, ("%s.bytecodeId"), closureVariable);
	      ce.Builder.AssignVariableToTemp(pathToId, 0); // Copy the closure bytecode id to D4

	      TokenBuffer callSymbol;
         SafeFormat(callSymbol.Text, TokenBuffer::MAX_TOKEN_CHARS, ("call %s %s"), archetype.Name(), closureVariable);
	      ce.Builder.AddSymbol(callSymbol);
	      ce.Builder.Assembler().Append_CallByIdIndirect(VM::REGISTER_D4);

	      ce.Builder.MarkExpression(&s);

	      return inputStackAllocByteCount;
      }

      void CompileClosureCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, cstr name, VARTYPE returnType, const IArchetype* returnArchetype)
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
	      ce.Builder.AssignClosureParentSF();
      }

      cstr GetIndexedMethod(CCompileEnvironment& ce, cr_sex invocation, const IStructure* s)
      {
	      if (s == NULL) return NULL;
	      if (!s->Prototype().IsClass) return NULL;
	      if (invocation.NumberOfElements() != 2) return NULL;
		
	      for(int i = 0; i < s->InterfaceCount(); ++i)
	      {
		      const IInterface& interf = s->GetInterface(i);

		      const ISExpression* src;
		      if (interf.Attributes().FindAttribute(("indexed"), (const void*&) src))
		      {
			      cr_sex methodNameExpr = GetAtomicArg(*src, 3);
			      for(int j = 0; j < interf.MethodCount(); ++j)
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
						      sexstringstream<1024> streamer;
						      streamer.sb << interf.Name() << (" attribute 'indexed' found, but the index function must take only one input");
						      Throw(methodNameExpr, streamer);
					      }
					      return NULL;
				      }
			      }

			      sexstringstream<1024> streamer;
			      streamer.sb << interf.Name() << (" attribute 'indexed' found, but the name was not found in the list of methods for the interfafce");
			      Throw(methodNameExpr, streamer);
		      }
	      }

	      return NULL;
      }

      bool IsIStringInlined(CScript& script);

      void CompileAsInlinedItemDirectAndReturnValue(CCompileEnvironment& ce, cstr instance, cstr item, VARTYPE returnType, int interfaceToInstanceOffsetByRef)
      {
	      MemberDef instanceDef;
	      ce.Builder.TryGetVariableByName(OUT instanceDef, instance);

	      if (instanceDef.location == VARLOCATION_TEMP && instanceDef.ResolvedType->Name()[0] == '_')
	      {
		      TokenBuffer fqItemName;
		      StringPrint(fqItemName, ("%s.%s"), instance, item);

		      MemberDef itemDef;
		      ce.Builder.TryGetVariableByName(OUT itemDef, fqItemName);

		      BITCOUNT bitCount = GetBitCount(returnType);

		      if (itemDef.IsParentValue)	{	ce.Builder.Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6); }

			  ce.Builder.Assembler().Append_GetStackFrameMember(VM::REGISTER_D7, itemDef.SFOffset, itemDef.MemberOffset - interfaceToInstanceOffsetByRef, (BITCOUNT) (8 * itemDef.ResolvedType->SizeOfStruct()));
			
		      if (itemDef.IsParentValue)	{	ce.Builder.Assembler().Append_SwapRegister(VM::REGISTER_SF, VM::REGISTER_D6); }
	      }
	      else
	      {
		      TokenBuffer fqn;
		      StringPrint(fqn, ("%s.%s"), instance, item);

		      ce.Builder.TryGetVariableByName(OUT instanceDef, fqn);

		      int interfaceToInstanceOffset = instanceDef.Usage == ARGUMENTUSAGE_BYREFERENCE ? interfaceToInstanceOffsetByRef : 0;
		      ce.Builder.AssignVariableToTemp(fqn, Rococo::ROOT_TEMPDEPTH, -interfaceToInstanceOffset);
	      }
      }

      void ValidateReturnType(cr_sex s, VARTYPE returnType, VARTYPE type)
      {
	      if (returnType == VARTYPE_AnyNumeric && IsPrimitiveType(type)) return;
	      if (returnType != type)
	      {
		      sexstringstream<1024> streamer;
		      streamer.sb << ("The property returns type ") << GetTypeName(returnType) << (" only");
		      Throw(s, streamer);
	      }
      }

      bool TryCompileAsInlineArrayAndReturnValue(CCompileEnvironment& ce, cr_sex s, cstr instance, cstr methodName, VARTYPE returnType, const IStructure& instanceStruct)
      {
	      if (instanceStruct == ce.StructArray())
	      {
		      TokenBuffer field;
		      if (AreEqual(("Length"), methodName))
		      {
			      StringPrint(field, ("%s._length"), instance);
			      ValidateReturnType(s, returnType, VARTYPE_Int32);
		      }
		      else if (AreEqual(("Capacity"), methodName))
		      {
			      StringPrint(field, ("%s._elementCapacity"), instance);
			      ValidateReturnType(s, returnType, VARTYPE_Int32);
		      }
		      else if (AreEqual(("PopOut"), methodName))
		      {
			      CompileAsPopOutFromArray(ce, s, instance, returnType);
			      ValidateReturnType(s, returnType, VARTYPE_Int32);
			      return true;
		      }
		      else
		      {
			      Throw(s, ("The property is not recognized for array types. Known properties for arrays: Length, Capacity & PopOut"));
		      }

		      ce.Builder.AddSymbol(field);
		      ce.Builder.AssignVariableToTemp(field, Rococo::ROOT_TEMPDEPTH, 0);
		      return true;
	      }

	      return false;
      }

      bool TryCompileAsInlineListAndReturnValue(CCompileEnvironment& ce, cr_sex s, cstr instance, cstr methodName, VARTYPE returnType, const IStructure& instanceStruct)
      {
	      if (instanceStruct == ce.StructList())
	      {
		      if (!IsAtomic(s) && s.NumberOfElements() != 1) return false;

		      TokenBuffer field;
		      if (AreEqual(("Length"), methodName))
		      {
			      StringPrint(field, ("%s._length"), instance);
			      ValidateReturnType(s, returnType, VARTYPE_Int32);
		      }
		      else
		      {
			      Throw(s, ("The property is not recognized for list types. Known properties for lists: Length"));
		      }

		      ce.Builder.AddSymbol(field);
		      ce.Builder.AssignVariableToTemp(field, Rococo::ROOT_TEMPDEPTH, 0);
		      return true;
	      }
	      else if (instanceStruct == ce.Object.Common().TypeNode())
	      {
		      if (!IsAtomic(s) && s.NumberOfElements() != 1) return false;

		      if (AreEqual(("Value"), methodName))
		      {
			      const IStructure& st = GetNodeDef(ce, s, instance);
			      ValidateReturnType(s, returnType, st.VarType());

			      ce.Builder.AssignVariableRefToTemp(instance, 0); // Node ptr goes to D4

				  if (st.InterfaceCount() > 0)
				  {
					  ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).NodeGetInterface); // value goes to D7
					  return true;
				  }

			      switch(st.VarType())
			      {
			      case VARTYPE_Bool:
			      case VARTYPE_Int32:
			      case VARTYPE_Float32:
			      case VARTYPE_Pointer:
			      case VARTYPE_Int64:
			      case VARTYPE_Float64:
			      case VARTYPE_Closure:
				      ce.Builder.Assembler().Append_Invoke(st.SizeOfStruct() == 4 ? GetListCallbacks(ce).NodeGet32 : GetListCallbacks(ce).NodeGet64); // value goes to D7
				      break;
			      default:
				      Throw(s, ("Node.Value only supports primitive types and interfaces"));
			      }

			      return true;
		      }
		      else if (AreEqual(("HasNext"), methodName))
		      {
			      ValidateReturnType(s, returnType, VARTYPE_Bool);

			      ce.Builder.AssignVariableRefToTemp(instance, Rococo::ROOT_TEMPDEPTH); // Node ptr goes to D7
			      ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).NodeHasNext); // Boolean (n.Next != NULL) goes to D7

			      return true;
		      }
		      else if (AreEqual(("HasPrevious"), methodName))
		      {
			      ValidateReturnType(s, returnType, VARTYPE_Bool);

			      ce.Builder.AssignVariableRefToTemp(instance, Rococo::ROOT_TEMPDEPTH); // Node ptr goes to D7
			      ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).NodeHasPrevious); // Boolean (n.Next != NULL) goes to D7

			      return true;
		      }
		      else
		      {
			      Throw(s, ("The property is not recognized for node types. Known properties for nodes: Value, Previous, Next, Exists"));
		      }
	      }

	      return false;
      }

      bool TryCompileAsInlineIStringAndReturnValue(CCompileEnvironment& ce, cstr instance, cstr methodName, VARTYPE returnType, const IStructure& instanceStruct)
      {
	      if (IsIStringInlined(ce.Script))
	      {
		      // The fact that IStrings are inlined means there is only one interface supported by anything that implements IString, and that Length and Buffer inline
		      // to length and buffer respectively. 

			  int interfaceToInstanceOffsetByRef = ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0;


		      if (returnType == VARTYPE_Int32 && AreEqual(("Length"), methodName))
		      {
			      if (Compiler::DoesClassImplementInterface(instanceStruct, ce.Object.Common().SysTypeIString()))
			      {
				      CompileAsInlinedItemDirectAndReturnValue(ce, instance, ("length"), VARTYPE_Int32, interfaceToInstanceOffsetByRef);
				      return true;
			      }
		      }
		      else if (returnType == VARTYPE_Pointer && AreEqual(("Buffer"), methodName))
		      {
			      if (Compiler::DoesClassImplementInterface(instanceStruct, ce.Object.Common().SysTypeIString()))
			      {
				      CompileAsInlinedItemDirectAndReturnValue(ce, instance, ("buffer"), VARTYPE_Pointer, interfaceToInstanceOffsetByRef);
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
		  ce.Builder.Assembler().Append_Invoke(ce.SS.GetScriptCallbacks().IdTestD4neqD5_retBoolD7); 

		  return true;
	  }

	  VM_CALLBACK(TestD4neqD5_retBoolD7)
	  {
		  auto diff = registers[4].int64Value - registers[5].int64Value;
		  registers[7].int64Value = diff != 0 ? 1 : 0;
	  }

      bool TryCompileMethodCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, VARTYPE returnType, const IStructure* returnTypeStruct, const IArchetype* returnArchetype)
      {
	      cr_sex firstArg = IsCompound(s) ? s.GetElement(0) : s;
	      cstr fname = firstArg.String()->Buffer;

	      NamespaceSplitter splitter(fname);

	      const IStructure* instanceStruct = NULL;

	      cstr instance, methodName;
	      if (!splitter.SplitTail(OUT instance, OUT methodName))
	      {
		      // Could be index method
		      if (s.NumberOfElements() == 2)
		      {
			      instanceStruct = ce.Builder.GetVarStructure(fname);
			      instance = fname;
			      methodName = GetIndexedMethod(ce, s, instanceStruct);

				  if (methodName == nullptr)
				  {
					  if (IsAtomic(s[1]) && returnType == VARTYPE_Bool)
					  {
						  cstr arg = s[1].String()->Buffer;
						  if (Eq(arg, "exists") && instanceStruct)
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

	      if (TryCompileAsInlineListAndReturnValue(ce, s, instance, methodName, returnType, *instanceStruct))
	      {
		      return true;
	      }

	      VARTYPE outputType;
	      if (TryCompileAsInlineMapAndReturnValue(ce, s, instance, methodName, returnType, *instanceStruct, OUT outputType))
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
		      const IInterface& interf = instanceStruct->GetInterface(interfaceIndex);
		      const IArchetype& method = interf.GetMethod(methodIndex);

		      CompileVirtualCallAndReturnValue(ce, false, method, s, interfaceIndex, methodIndex, instance, returnType, returnTypeStruct, returnArchetype, interf);
		      return true;
	      }

	      return false;
      }

      bool TryCompileMethodCallWithoutInputAndReturnValue(CCompileEnvironment& ce, cr_sex s, cstr instance, cstr methodName, VARTYPE returnType, const IStructure* returnTypeStruct, const IArchetype* returnArchetype)
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

	      if (TryCompileAsInlineListAndReturnValue(ce, s, instance, methodName, returnType, *instanceStruct))
	      {
		      return true;
	      }

	      VARTYPE outputType;
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
		      const IInterface& interf = instanceStruct->GetInterface(interfaceIndex);
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

      VARTYPE CompileMethodCallWithoutInputAndReturnNumericValue(CCompileEnvironment& ce, cr_sex s, cstr instance, cstr methodName)
      {
	      const IStructure* instanceStruct = ce.Builder.GetVarStructure(instance);

	      if (instanceStruct == NULL)
	      {
		      return VARTYPE_Bad;
	      }

	      if (!IsCapital(methodName[0]))
	      {
		      return VARTYPE_Bad;
	      }

	      VARTYPE outputType;
	      if (TryCompileAsInlineMapAndReturnValue(ce, s, instance, methodName, VARTYPE_AnyNumeric, *instanceStruct, outputType))
	      {
		      return outputType;
	      }

	      if (TryCompileAsInlineIStringAndReturnValue(ce, instance, methodName, VARTYPE_Int32, *instanceStruct))
	      {
		      return VARTYPE_Int32;
	      }

	      if (!instanceStruct->Prototype().IsClass)
	      {
		      Throw(s, ("Only classes support methods."));
	      }

	      OUT int interfaceIndex, OUT methodIndex;
	      if (GetMethodIndices(OUT interfaceIndex, OUT methodIndex, *instanceStruct, methodName))
	      {
		      const IInterface& interf = instanceStruct->GetInterface(interfaceIndex);
		      const IArchetype& method = interf.GetMethod(methodIndex);

		      if (!IsGetAccessor(method))
		      {
			      return VARTYPE_Bad;
		      }

		      VARTYPE type = method.GetArgument(0).VarType();

		      if (!IsNumericTypeOrBoolean(type)) return VARTYPE_Bad;

		      CompileVirtualCallAndReturnValue(ce, true, method, s, interfaceIndex, methodIndex, instance, type, NULL, NULL, interf);

		      return type;
	      }

	      return VARTYPE_Bad;
      }

      int GetOutputArgSize(const IStructure& s)
      {
	      if (s.VarType() == VARTYPE_Derivative)
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

	      for(int i = 0; i < outputCount; i++)
	      {
		      int outputIndex = outputCount - 1 - i;
		      const IStructure& arg = callee.GetArgument(outputIndex);
		      outputOffset -= GetOutputArgSize(arg);						
		      CompilePopOutputToVariable(arg, ce.Builder, inputEnd+1, outputIndex, outputOffset, invocation);			
	      }
      }

      void AppendVirtualCallAssembly(cstr instanceName, int interfaceIndex, int methodIndex, ICodeBuilder& builder, const IInterface& interf, cr_sex s)
      {	
	      MemberDef refDef;
	      builder.TryGetVariableByName(OUT refDef, instanceName);
		
	      const IArchetype& arch = interf.GetMethod(methodIndex);
		
	      TokenBuffer fullName;
	      StringPrint(fullName, ("%s.%s"), instanceName, arch.Name());
	      builder.AddSymbol(fullName);

	      int vTableByteOffset = (methodIndex+1) * sizeof(ID_BYTECODE);

	      if (IsNullType(*refDef.ResolvedType))
	      {
			  if (!refDef.IsContained || refDef.Usage == ARGUMENTUSAGE_BYVALUE)
			  {
				  builder.Assembler().Append_CallVitualFunctionViaRefOnStack(refDef.SFOffset + refDef.MemberOffset, vTableByteOffset);
			  }
			  else
			  {
				  builder.Assembler().Append_CallVitualFunctionViaMemberOffsetOnStack(refDef.SFOffset, refDef.MemberOffset, vTableByteOffset);
			  }
	      }
		  else
		  {
				// We have a reference to a concrete object, an instance pointer

				for(int i = 0; i < refDef.ResolvedType->InterfaceCount(); ++i)
				{
					if (&refDef.ResolvedType->GetInterface(i) == &interf)
					{
						// Concrete class
						int instanceToInterfaceOffset = Compiler::GetInstanceToInterfaceOffset(i) + refDef.MemberOffset;
						builder.Assembler().Append_CallVitualFunctionViaRefOnStack(refDef.SFOffset, vTableByteOffset, instanceToInterfaceOffset);
						return;
					}
				}
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
	      ce.Builder.AssignClosureParentSF();
      }

      void CompileVirtualCall(CCompileEnvironment& ce, const IArchetype& callee, cr_sex s, int interfaceIndex, int methodIndex, cstr instanceName, const IInterface& interfaceRef)
      {
	      // (<instance.method-name> input1 input2 input3.... inputN -> output1...output2...outputN)
	      cstr calleeName = callee.Name();
	      cstr callerName = ce.Builder.Owner().Name();

	      int mapIndex = GetMapIndex(s, 1, callee.NumberOfOutputs(), callee.NumberOfInputs()-1);

	      int nSuppliedInputs = mapIndex - 1;
	      if (nSuppliedInputs > callee.NumberOfInputs() - 1)
	      {
		      Throw(s, ("More inputs were supplied than are needed by the function call"));
	      }

	      int outputOffset = CompileVirtualCallKernel(ce, false, callee, s, interfaceIndex, methodIndex, instanceName, interfaceRef);
	      PopOutputs(ce, s, callee, outputOffset, true);		
	      ce.Builder.AssignClosureParentSF();
      }

      int CompileInstancePointerArg(CCompileEnvironment& ce, cstr classInstance)
      {
	      MemberDef def;
	      ce.Builder.TryGetVariableByName(OUT def, classInstance);

	      ce.Builder.AddSymbol(classInstance);

	      AddArgVariable(("instance"), ce, ce.Object.Common().TypePointer());

	      if (def.ResolvedType == &ce.Object.Common().TypePointer() || def.Usage == ARGUMENTUSAGE_BYREFERENCE)
	      {
			  ce.Builder.AssignVariableToTemp(classInstance, 0);
	      }
	      else
	      {
		      ce.Builder.AssignVariableRefToTemp(classInstance, 0);	
	      }

		  ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4, BITCOUNT_POINTER);
			
	      return (int) sizeof(void*);		
      }

      int CompileInstancePointerArgFromTemp(CCompileEnvironment& ce, int tempDepth)
      {
	      ce.Builder.AddSymbol(("// reference to instance"));
	      AddArgVariable(("instance"), ce, ce.Object.Common().TypePointer());
	      ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D4 + tempDepth, BITCOUNT_POINTER);			
	      return (int) sizeof(void*);		
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
			      Throw(s, ("Too many inputs"));
		      }
	      }

	      if (mapIndex + callee.NumberOfOutputs() > s.NumberOfElements())
	      {
		      // More outputs that we have supplied, some will be skipped
	      }

	      // (<function-name> input1 input2 input3.... inputN -> output1...output2...outputN)		
	      int outputOffset = CompileFunctionCallKernel(ce, s, callee);		
	      PopOutputs(ce, s, callee, outputOffset, false);		
	      ce.Builder.AssignClosureParentSF();
      }

      bool TryCompileFunctionCallAndReturnValue(CCompileEnvironment& ce, cr_sex s, VARTYPE type, const IStructure* derivedType, const IArchetype* returnArchetype)
      {
	      cr_sex firstArg = IsCompound(s) ? s.GetElement(0) : s;

	      if (IsAtomic(firstArg)) 
	      {			
		      cstr fname = firstArg.String()->Buffer;

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

	      cstr value = arg.String()->Buffer;

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

	      cstr value = arg.String()->Buffer;

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

		      const IInterface& interf = s->GetInterface(interfaceIndex);
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
			      cstr value = tokenExpr.String()->Buffer;
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

	      cstr token = tokenExpr.String()->Buffer;

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
				      return &GetArrayDef(ce, s, token);
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

		      const IInterface& interf = st->GetInterface(interfaceIndex);
		      const IArchetype& methodArch = interf.GetMethod(methodIndex);

		      if (methodArch.NumberOfOutputs() == 0) Throw(tokenExpr, ("The method has no output"));
		      if (methodArch.NumberOfOutputs() > 1) Throw(tokenExpr, ("The method has more than one output"));

		      return &methodArch.GetArgument(0);
	      }

	      return NULL;
      }

      const IStructure* GuessType(CCompileEnvironment& ce, cr_sex arg)
      {
	      switch(arg.Type())
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

      // Only safe to call from TryCompileAsFunctionCall(...)
      bool TryCompileAsMethodCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName)
      {
	      MemberDef def;
	      if (!ce.Builder.TryGetVariableByName(OUT def, IN instanceName))
	      {
		      return false;
	      }

	      const IStructure& c = *def.ResolvedType;
		
	      if (c.VarType() != VARTYPE_Derivative)
	      {
		      return false;
	      }

	      if (c == ce.StructArray())
	      {
		      return TryCompileAsArrayCall(ce, s, instanceName, methodName);
	      }

	      if (c == ce.StructList())
	      {
		      return TryCompileAsListCall(ce, s, instanceName, methodName);
	      }

	      if (c == ce.StructMap())
	      {
		      return TryCompileAsMapCall(ce, s, instanceName, methodName);
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
		      const IInterface& interf = c.GetInterface(interfaceIndex);
		      const IArchetype& method = interf.GetMethod(methodIndex);
		      CompileVirtualCall(ce, method, s, interfaceIndex, methodIndex, instanceName, interf);
		      return true;
	      }

	      ThrowTokenNotFound(s, methodName, GetFriendlyName(*def.ResolvedType), ("method"));

	      return false; // No matching method in all the interfaces
      }

      bool TryCompileAsBuilderCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName)
      {
	      cr_sex arg = s.GetElement(1);
	      const IStructure* argType = GuessType(ce, arg);
	      if (argType == NULL)
	      {
		      Throw(arg, ("Cannot guess type of argument. Try assigning value to a variable and use the variable as the argument."));
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
		      sexstringstream<1024> streamer;
		      streamer.sb << ("Could not find namespace: ") << body;
		      Throw(s, streamer);
		      return false;
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

	      for(int i = 0; i < module.PrefixCount(); ++i)
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
			      sexstringstream<1024> streamer;
			      streamer.sb << ("Ambiguity: '") << fname << "' could belong to " << prefix.FullName()->Buffer << " or " << NS->FullName()->Buffer;
			      Throw(s, streamer);
			      return false;
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
	      cstr fname = nameExpr.String()->Buffer;

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

	      return false;
      }

      bool TryCompileAsDerivativeFunctionCall(CCompileEnvironment& ce, cr_sex s)
      {
	      if (!IsCompound(s) || s.NumberOfElements() == 0) return false;

	      cr_sex nameExpr = GetAtomicArg(s, 0);
	      cstr fname = nameExpr.String()->Buffer;

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
			      const IInterface& interf = type->GetInterface(0);
			      if (s.NumberOfElements() == 2)
			      {
				      const ISExpression* attr;
				      if (interf.Attributes().FindAttribute(("indexed"), (const void*&) attr))
				      {
					      cr_sex indexMethodExpr = GetAtomicArg(*attr, 3);
					      cstr indexMethodName = indexMethodExpr.String()->Buffer;
					      return TryCompileAsMethodCall(ce, s, fname, indexMethodName);
				      }	
				      else if (interf.Attributes().FindAttribute(("builder"), (const void*&) attr))
				      {
					      cr_sex appendPrefixExpr = GetAtomicArg(*attr, 2);
					      cstr appendPrefix = appendPrefixExpr.String()->Buffer;
					      return TryCompileAsBuilderCall(ce, s, fname, appendPrefix);
				      }
			      }
		      }

		      if (type == &ce.StructArray())
		      {
			      CompileArraySet(ce, s);
			      return true;
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
		      sexstringstream<1024> streamer;
		      streamer.sb << ("Could not find the namespace ") << ns;
		      Throw(s, streamer);
	      }
	      return *NS;
      }

      const IFunction& GetConstructor(const IStructure& st, cr_sex s)
      {
	      const IFunction* constructor = st.Constructor();
	      if (constructor == NULL)
	      {
		      sexstringstream<1024> streamer;
		      streamer.sb << ("Cannot find ") << st.Name() << (".Construct in ") << st.Module().Name();
		      Throw(s, streamer);
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
		      sexstringstream<1024> streamer;
		      streamer.sb << ("Could not find ") << shortName << (" in ") << ns;
		      Throw(s, streamer);
	      }

	      return *f;
      }
   }// Script
}//Sexy 