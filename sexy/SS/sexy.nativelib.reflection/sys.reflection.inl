namespace
{
	void WriteString(CStringConstant* constant, int outputIndex, NativeCallEnvironment& e)
	{
		WriteOutput(outputIndex, &constant->header._vTables[0], e);
	}

	void WriteClass(CClassHeader* header, int outputIndex, NativeCallEnvironment& e)
	{
		WriteOutput(outputIndex, &header->_vTables[0], e);
	}

	void NativeExpressionGetChild(NativeCallEnvironment& e)
	{
		ISExpression* pExpression;
		ReadInput(0, (void*&) pExpression, e);

		int32 index;

		ReadInput(1, index, e);

		const ISExpression& child = pExpression->GetElement(index);	
		const CClassExpression* childExpressObject = ((IScriptSystem&) e.ss).GetExpressionReflection(child);
		const VTABLEDEF* pInterf = &childExpressObject->Header._vTables[0];
		WriteOutput(0, (void*) pInterf, e);
	}

	void NativeExpressionGetParent(NativeCallEnvironment& e)
	{
		ISExpression* pExpression;
		ReadInput(0, (void*&) pExpression, e);

		const ISExpression* parent = pExpression ? pExpression->Parent() : NULL;	
		const CClassExpression* parentExpress = ((IScriptSystem&) e.ss).GetExpressionReflection(*parent);
		const VTABLEDEF* pInterf = &parentExpress->Header._vTables[0];

		WriteOutput(0, (void*) pInterf, e);
	}

	void NativeExpressionChildCount(NativeCallEnvironment& e)
	{
		ISExpression* pExpression;
		ReadInput(0, (void*&) pExpression, e);

		int nElements = pExpression ? pExpression->NumberOfElements() : 0;
		WriteOutput(0, nElements, e);
	}

	void NativeGetExpressionText(NativeCallEnvironment& e)
	{
		ISExpression* pExpression;
		ReadInput(0, (void*&) pExpression, e);

		const sexstring s = pExpression->String();

		CStringConstant* sc =  ((IScriptSystem&) e.ss).GetStringReflection(s == NULL ? SEXTEXT("") : s->Buffer);
		WriteString(sc, 0, e);
	}

	void NativeGetScriptSystem(NativeCallEnvironment& e)
	{
		CScriptSystemClass* instance = ((IScriptSystem&)e.ss).GetScriptSystemClass();
		WriteOutput(0, &instance->header._vTables[0], e);
	}

	void NativeModuleCount(NativeCallEnvironment& e)
	{
		WriteOutput(0, e.ss.PublicProgramObject().ModuleCount(), e);
	}

	void NativeGetModule(NativeCallEnvironment& e)
	{
		IScriptSystem& SS = (IScriptSystem&) e.ss;

		int32 index;
		ReadInput(0, index, e);

		const IModule& module = SS.PublicProgramObject().GetModule(index);

		CReflectedClass* pModule = SS.GetReflectedClass((void*) &module);
		if (pModule == NULL)
		{
			pModule = SS.CreateReflectionClass(SEXTEXT("Module"),(void*)  &module);
		}

		WriteClass(&pModule->header, 0, e);
	}

	void NativeGetModuleName(NativeCallEnvironment& e)
	{
		IScriptSystem& SS = (IScriptSystem&) e.ss;

		IModule* pModule;
		ReadInput(0, (void*&) pModule, e);

		csexstr name = pModule->Name();

		CStringConstant* sc = SS.GetStringReflection(name);
		WriteString(sc, 0, e);
	}

	void NativeGetStructCount(NativeCallEnvironment& e)
	{
		IModule* pModule;
		ReadInput(0, (void*&) pModule, e);

		WriteOutput(0, pModule->StructCount(), e);
	}

	void NativeGetStruct(NativeCallEnvironment& e)
	{
		IModule* pModule;
		ReadInput(0, (void*&) pModule, e);

		int32 index;
		ReadInput(1, index, e);
		
		const IStructure& s = pModule->GetStructure(index);

		CReflectedClass* pStruct = ((IScriptSystem&) e.ss).GetReflectedClass((void*) &s);
		if (pStruct == NULL)
		{
			pStruct = ((IScriptSystem&) e.ss).CreateReflectionClass(SEXTEXT("Structure"), (void*) &s);
		}

		WriteClass(&pStruct->header, 0, e);
	}

	void NativeGetStructName(NativeCallEnvironment& e)
	{
		IScriptSystem& SS = (IScriptSystem&) e.ss;

		IStructure* pStruct;
		ReadInput(0, (void*&) pStruct, e);
		
		csexstr name = pStruct->Name();

		CStringConstant* sc = ((IScriptSystem&)e.ss).GetStringReflection(name);
		WriteString(sc, 0, e);
	}

	void DuplicateExpression(cr_sex s, ISExpressionBuilder& b)
	{
		switch (s.Type())
		{		
		case EXPRESSION_TYPE_STRING_LITERAL:
			b.AddStringLiteral(s.String()->Buffer);
			return;
		case EXPRESSION_TYPE_ATOMIC:
			{
				csexstr token = s.String()->Buffer;
				b.AddAtomic(token);
			}
			return;
		case EXPRESSION_TYPE_COMPOUND:
			{
				ISExpressionBuilder* builderChild = b.AddChild();
				for(int i = 0; i < s.NumberOfElements(); ++i)
				{
					cr_sex child = s.GetElement(i);
					DuplicateExpression(child, *builderChild);
				}
			}
			return;
		default:
			break;
		}
	}

   void DuplicateExpressionAsString(cr_sex s, ISExpressionBuilder& b)
   {
      switch (s.Type())
      {
      case EXPRESSION_TYPE_STRING_LITERAL:
         b.AddStringLiteral(s.String()->Buffer);
         return;
      case EXPRESSION_TYPE_ATOMIC:
      {
         csexstr token = s.String()->Buffer;
         b.AddStringLiteral(token);
      }
      return;
      case EXPRESSION_TYPE_COMPOUND:
      {
         Rococo::Sex::Throw(*b.Parent(), SEXTEXT("Could not duplicate as string. Element is compound"));
      }
      return;
      default:
         Rococo::Sex::Throw(*b.Parent(), SEXTEXT("Could not duplicate. Element type not applicable"));
         break;
      }
   }

	int SubstituteAtomic(cr_sex input, csexstr token, ISExpressionBuilder& b)
	{
		using namespace Rococo::Parse;

		if (token[0] != '$')
		{
			b.AddAtomic(token);		
			return 0;
		}

		int32 value;
		if (PARSERESULT_GOOD != TryParseDecimal(OUT value, IN token+1))
		{
			b.AddAtomic(SEXTEXT("!Expecting-argindex-after-$sign"));
			return 1;
		}
		else
		{
			if (value < 0)
			{
				b.AddAtomic(SEXTEXT("!Expecting-positive-argindex-after-$sign"));
				return 1;
			}
			else if (value >= input.NumberOfElements())
			{
				b.AddAtomic(SEXTEXT("!Expecting-positive-integer-argument-to-$sign"));
				return 1;
			}
			else
			{
				cr_sex arg = input.GetElement(value);
				DuplicateExpression(arg, b);
				return 0;
			}
		}
	}

	void SubstituteExpression(cr_sex input, cr_sex format, ISExpressionBuilder& b, int& errorCount)
	{
		switch (format.Type())
		{		
		case EXPRESSION_TYPE_STRING_LITERAL:
			b.AddStringLiteral(format.String()->Buffer);
			return;
		case EXPRESSION_TYPE_ATOMIC:
			{
				csexstr token = format.String()->Buffer;
				errorCount += SubstituteAtomic(input, token, b);
			}
			return;
		case EXPRESSION_TYPE_COMPOUND:
			{
				ISExpressionBuilder* builderChild = b.AddChild();
				for(int i = 0; i < format.NumberOfElements(); ++i)
				{
					cr_sex subFormat = format.GetElement(i);
					SubstituteExpression(input, subFormat, *builderChild, errorCount);
				}
			}
			return;
		default:
			break;
		}
	}

	void NativeExpressionBuilderAddAtomic(NativeCallEnvironment& e)
	{
		IScriptSystem& SS = (IScriptSystem&) e.ss;

		ISExpressionBuilder* pBuilder;
		ReadInput(0, (void*&) pBuilder, e);

		const SEXCHAR* pBuffer;
		ReadInput(1, (void*&) pBuffer, e);

		pBuilder->AddAtomic(pBuffer);
	}

	void NativeExpressionBuilderAddCompound(NativeCallEnvironment& e)
	{
		IScriptSystem& SS = (IScriptSystem&) e.ss;

		ISExpressionBuilder* pBuilder;
		ReadInput(0, (void*&) pBuilder, e);

		ISExpressionBuilder* pChild = pBuilder->AddChild();

		CReflectedClass* childRep = SS.CreateReflectionClass(SEXTEXT("ExpressionBuilder"), pChild);
		WriteClass(&childRep->header, 0, e);
	}

	void NativeExpressionBuilderAddCopy(NativeCallEnvironment& e)
	{
		IScriptSystem& SS = (IScriptSystem&) e.ss;

		ISExpressionBuilder* pBuilder;
		ReadInput(0, (void*&) pBuilder, e);
		
		ISExpression* pSource;
		ReadInput(1, (void*&) pSource, e);

		DuplicateExpression(*pSource, *pBuilder);
	}

   void NativeExpressionBuilderAddCopyToString(NativeCallEnvironment& e)
   {
      IScriptSystem& SS = (IScriptSystem&)e.ss;

      ISExpressionBuilder* pBuilder;
      ReadInput(0, (void*&)pBuilder, e);

      ISExpression* pSource;
      ReadInput(1, (void*&)pSource, e);

      DuplicateExpressionAsString(*pSource, *pBuilder);
   }

	void NativeExpressionBuilderSubstitute(NativeCallEnvironment& e)
	{
		IScriptSystem& SS = (IScriptSystem&) e.ss;

		ISExpressionBuilder* pBuilder;
		ReadInput(0, (void*&) pBuilder, e);
		
		ISExpression* pInput;
		ReadInput(1, (void*&) pInput, e);

		ISExpression* pFormat;
		ReadInput(2, (void*&) pFormat, e);

		int errorCount = 0;
		SubstituteExpression(*pInput, *pFormat, *pBuilder, REF errorCount);

		WriteOutput(0, errorCount, e);
	}

	void AddReflectionCalls(IScriptSystem& ss)
	{
		const INamespace& sysReflectionNative = ss.AddNativeNamespace(SEXTEXT("Sys.Reflection.Native"));
		ss.AddNativeCall(sysReflectionNative, NativeExpressionGetChild, &ss, SEXTEXT("ExpressionGetChild (Pointer sPtr) (Int32 index) ->  (Sys.Reflection.IExpression child)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionGetParent, &ss, SEXTEXT("ExpressionGetParent (Pointer sPtr) -> (Sys.Reflection.IExpression parent)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionChildCount, &ss, SEXTEXT("ExpressionChildCount (Pointer sPtr) -> (Int32 count)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeGetExpressionText, &ss, SEXTEXT("GetExpressionText  (Pointer sPtr) -> (Sys.Type.IString name)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeGetScriptSystem, &ss, SEXTEXT("GetScriptSystem -> (Sys.Reflection.IScriptSystem ss)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeModuleCount, &ss, SEXTEXT("ModuleCount -> (Int32 count)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeGetModule, &ss, SEXTEXT("GetModule (Int32 index) -> (Sys.Reflection.IModule module)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeGetModuleName, &ss, SEXTEXT("GetModuleName (Pointer modulePtr) -> (Sys.Type.IString name)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeGetStructCount, &ss, SEXTEXT("GetStructCount (Pointer modulePtr) -> (Int32 structCount)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeGetStruct, &ss, SEXTEXT("GetStruct (Pointer modulePtr) (Int32 index) -> (Sys.Reflection.IStructure structure)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeGetStructName, &ss, SEXTEXT("GetStructName (Pointer structPtr) -> (Sys.Type.IString name)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddAtomic, &ss, SEXTEXT("ExpressionBuilderAddAtomic (Pointer builderPtr) (Pointer strBuffer) ->"), true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddCompound, &ss, SEXTEXT("ExpressionBuilderAddCompound (Pointer builderPtr) -> (Sys.Reflection.IExpressionBuilder child)"), true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddCopy, &ss, SEXTEXT("ExpressionBuilderAddCopy (Pointer builderPtr) (Pointer xpressPtr) ->"), true);
      ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddCopyToString, &ss, SEXTEXT("ExpressionBuilderAddCopyToString (Pointer builderPtr) (Pointer xpressPtr) ->"), true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderSubstitute, &ss, SEXTEXT("ExpressionBuilderSubstitute (Pointer builderPtr) (Pointer inputPtr) (Pointer formatPtr) -> (Int32 errorCount)"), true);
	}
}