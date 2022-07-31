namespace
{
	void WriteString(CStringConstant* constant, int outputIndex, NativeCallEnvironment& e)
	{
		WriteOutput(outputIndex, &constant->header.pVTables[0], e);
	}

	void WriteClass(ObjectStub* header, int outputIndex, NativeCallEnvironment& e)
	{
		WriteOutput(outputIndex, &header->pVTables[0], e);
	}

	void NativeExpressionGetChild(NativeCallEnvironment& e)
	{
		ISExpression* pExpression;
		ReadInput(0, (void*&) pExpression, e);

		int32 index;

		ReadInput(1, index, e);

		const ISExpression& child = pExpression->GetElement(index);	
		const CClassExpression* childExpressObject = ((IScriptSystem&) e.ss).GetExpressionReflection(child);
		VirtualTable* const * pInterf = &childExpressObject->Header.pVTables[0];
		WriteOutput(0, (void*) pInterf, e);
	}

	void NativeExpressionGetIndexOf(NativeCallEnvironment& e)
	{
		ISExpression* pExpression;
		ReadInput(0, (void*&)pExpression, e);

		InterfacePointer pChildInterface;
		ReadInput(1, pChildInterface, e);

		int index = -1;

		auto* pChild = (CClassExpression*) InterfaceToInstance(pChildInterface);
		if (pChild->Header.Desc->TypeInfo == e.ss.GetExpressionType())
		{
			auto& sChild = *pChild->ExpressionPtr;
			index = pExpression->GetIndexOf(sChild);
		}
		
		WriteOutput(0, index, e);
	}

	void NativeExpressionGetParent(NativeCallEnvironment& e)
	{
		ISExpression* pExpression;
		ReadInput(0, (void*&) pExpression, e);

		const ISExpression* parent = pExpression ? pExpression->Parent() : NULL;	
		const CClassExpression* parentExpress = ((IScriptSystem&) e.ss).GetExpressionReflection(*parent);
		VirtualTable* const * pInterf = &parentExpress->Header.pVTables[0];

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

		CStringConstant* sc =  ((IScriptSystem&) e.ss).GetStringReflection(s == NULL ? ("") : s->Buffer);
		WriteString(sc, 0, e);
	}

	void NativeExpressionAppendTextTo(NativeCallEnvironment& e)
	{
		ISExpression* pExpression;
		ReadInput(0, (void*&)pExpression, e);

		auto type = pExpression->Type();
		if (type != EXPRESSION_TYPE_ATOMIC && type != EXPRESSION_TYPE_STRING_LITERAL)
		{
			Throw(0, "IExpression.AppendTextTo failed. The expression was neither atomic nor a string literal");
		}

		InterfacePointer pStringBuilderVTable0;
		ReadInput(1, pStringBuilderVTable0, e);

		ObjectStub* object = InterfaceToInstance(pStringBuilderVTable0);
		if (&object->Desc->TypeInfo->Module() != &e.ss.PublicProgramObject().GetModule(0))
		{
			Throw(0, "IExpression.AppendTextTo failed. The concrete StringBuilder has to be that from 'Sys.Type.Strings.sxy'");
		}

		CClassSysTypeStringBuilder* builder = (CClassSysTypeStringBuilder*)object;

		StackStringBuilder sb(builder->buffer, builder->capacity, StringBuilder::BUILD_EXISTING);
		sb << pExpression->String()->Buffer;
		builder->length = sb.Length();
	}

	void NativeExpressionThrow(NativeCallEnvironment& e)
	{
		ISExpression* pExpression;
		ReadInput(0, (void*&)pExpression, e);

		int errorCode;
		ReadInput(1, errorCode, e);

		char* message;
		ReadInput(2, message, e);

		if (errorCode != 0)
		{
			char buf[1024];
			SafeFormat(buf, sizeof(buf), "%d: %s", errorCode, message);
			Throw(*pExpression, "%s", buf);
		}
		else
		{
			Throw(*pExpression, "%s", message);
		}
	}

	void NativeGetExpressionType(NativeCallEnvironment& e)
	{
		ISExpression* pExpression;
		ReadInput(0, (void*&)pExpression, e);

		const EXPRESSION_TYPE type = pExpression->Type();

		WriteOutput(0, type, e);
	}

	void NativeGetScriptSystem(NativeCallEnvironment& e)
	{
		CScriptSystemClass* instance = ((IScriptSystem&)e.ss).GetScriptSystemClass();
		WriteOutput(0, &instance->header.pVTables[0], e);
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
			pModule = SS.CreateReflectionClass(("Module"),(void*)  &module);
		}

		WriteClass(&pModule->header, 0, e);
	}

	void NativeGetModuleName(NativeCallEnvironment& e)
	{
		IScriptSystem& SS = (IScriptSystem&) e.ss;

		IModule* pModule;
		ReadInput(0, (void*&) pModule, e);

		cstr name = pModule->Name();

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
			pStruct = ((IScriptSystem&) e.ss).CreateReflectionClass(("Structure"), (void*) &s);
		}

		WriteClass(&pStruct->header, 0, e);
	}

	void NativeGetStructName(NativeCallEnvironment& e)
	{
		IScriptSystem& SS = (IScriptSystem&) e.ss;

		IStructure* pStruct;
		ReadInput(0, (void*&) pStruct, e);
		
		cstr name = pStruct->Name();

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
				cstr token = s.String()->Buffer;
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
         cstr token = s.String()->Buffer;
         b.AddStringLiteral(token);
      }
      return;
      case EXPRESSION_TYPE_COMPOUND:
      {
         Rococo::Sex::Throw(*b.Parent(), ("Could not duplicate as string. Element is compound"));
      }
      return;
      default:
         Rococo::Sex::Throw(*b.Parent(), ("Could not duplicate. Element type not applicable"));
         break;
      }
   }

	int SubstituteAtomic(cr_sex input, cstr token, ISExpressionBuilder& b)
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
			b.AddAtomic(("!Expecting-argindex-after-$sign"));
			return 1;
		}
		else
		{
			if (value < 0)
			{
				b.AddAtomic(("!Expecting-positive-argindex-after-$sign"));
				return 1;
			}
			else if (value >= input.NumberOfElements())
			{
				b.AddAtomic(("!Expecting-positive-integer-argument-to-$sign"));
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
				cstr token = format.String()->Buffer;
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

		const char* pBuffer;
		ReadInput(1, (void*&) pBuffer, e);

		pBuilder->AddAtomic(pBuffer);
	}

	void NativeExpressionBuilderAddCompound(NativeCallEnvironment& e)
	{
		IScriptSystem& SS = (IScriptSystem&) e.ss;

		ISExpressionBuilder* pBuilder;
		ReadInput(0, (void*&) pBuilder, e);

		ISExpressionBuilder* pChild = pBuilder->AddChild();

		CReflectedClass* childRep = SS.CreateReflectionClass(("ExpressionBuilder"), pChild);
		WriteClass(&childRep->header, 0, e);
	}

	void NativeExpressionBuilderInsertCompoundAfter(NativeCallEnvironment& e)
	{
		IScriptSystem& SS = (IScriptSystem&)e.ss;

		ISExpressionBuilder* pBuilder;
		ReadInput(0, (void*&)pBuilder, e);

		int index;
		ReadInput(1, index, e);

		ISExpressionBuilder* pChild = pBuilder->InsertChildAfter(index);

		CReflectedClass* childRep = SS.CreateReflectionClass(("ExpressionBuilder"), pChild);
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
		const INamespace& sysReflectionNative = ss.AddNativeNamespace(("Sys.Reflection.Native"));
		ss.AddNativeCall(sysReflectionNative, NativeExpressionGetChild, &ss, ("ExpressionGetChild (Pointer sPtr) (Int32 index) ->  (Sys.Reflection.IExpression child)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionGetIndexOf, &ss, ("ExpressionGetIndexOf (Pointer sPtr) (Sys.Reflection.IExpression child) -> (Int32 index)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionGetParent, &ss, ("ExpressionGetParent (Pointer sPtr) -> (Sys.Reflection.IExpression parent)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionChildCount, &ss, ("ExpressionChildCount (Pointer sPtr) -> (Int32 count)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionAppendTextTo, &ss, "ExpressionAppendTextTo  (Pointer sPtr) (Sys.Type.IStringBuilder sb)->", __FILE__, __LINE__);
		ss.AddNativeCall(sysReflectionNative, NativeGetExpressionText, &ss, ("GetExpressionText  (Pointer sPtr) -> (Sys.Type.IString name)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetExpressionType, &ss, ("GetExpressionType  (Pointer sPtr) -> (Sys.Type.Int32 type)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionThrow, &ss, ("ExpressionThrow  (Pointer sPtr)(Int32 errorCode)(Sys.Type.Pointer buffer) ->"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetScriptSystem, &ss, ("GetScriptSystem -> (Sys.Reflection.IScriptSystem ss)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeModuleCount, &ss, ("ModuleCount -> (Int32 count)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetModule, &ss, ("GetModule (Int32 index) -> (Sys.Reflection.IModule module)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetModuleName, &ss, ("GetModuleName (Pointer modulePtr) -> (Sys.Type.IString name)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetStructCount, &ss, ("GetStructCount (Pointer modulePtr) -> (Int32 structCount)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetStruct, &ss, ("GetStruct (Pointer modulePtr) (Int32 index) -> (Sys.Reflection.IStructure structure)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetStructName, &ss, ("GetStructName (Pointer structPtr) -> (Sys.Type.IString name)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddAtomic, &ss, ("ExpressionBuilderAddAtomic (Pointer builderPtr) (Pointer strBuffer) ->"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddCompound, &ss, ("ExpressionBuilderAddCompound (Pointer builderPtr) -> (Sys.Reflection.IExpressionBuilder child)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderInsertCompoundAfter, &ss, ("ExpressionBuilderInsertCompoundAfter (Pointer builderPtr)(Int32 index) -> (Sys.Reflection.IExpressionBuilder child)"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddCopy, &ss, ("ExpressionBuilderAddCopy (Pointer builderPtr) (Pointer xpressPtr) ->"), __FILE__, __LINE__, true);
        ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddCopyToString, &ss, ("ExpressionBuilderAddCopyToString (Pointer builderPtr) (Pointer xpressPtr) ->"), __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderSubstitute, &ss, ("ExpressionBuilderSubstitute (Pointer builderPtr) (Pointer inputPtr) (Pointer formatPtr) -> (Int32 errorCount)"), __FILE__, __LINE__, true);
	}
}