#pragma once
// Note this file is hand-coded. It was written before BennyHill existed, and the layout was the basis for the BennyHill generated code.

#include "..\..\sexy\STC\stccore\sexy.compiler.helpers.h"

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

	void NativeExpressionIndexOf(NativeCallEnvironment& e)
	{
		ISExpression* pParentExpression;
		ReadInput(0, (void*&)pParentExpression, e);

		ISExpression* pChildElement;
		ReadInput(1, pChildElement, e);

		int index = pParentExpression->GetIndexOf(*pChildElement);
		if (index == -1)
		{
			auto* pOriginal = ((IScriptSystem&)e.ss).GetTransform(*pChildElement);
			if (pOriginal)
			{
				index = pParentExpression->GetIndexOf(*pOriginal);
			}
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
		auto s = pExpression->String();
		builder->AppendAndTruncate({s->Buffer, s->Length});
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
		IStructure* pStruct;
		ReadInput(0, (void*&) pStruct, e);
		
		cstr name = pStruct->Name();

		CStringConstant* sc = ((IScriptSystem&)e.ss).GetStringReflection(name);
		WriteString(sc, 0, e);
	}

	void NativeGetInterfaceCount(NativeCallEnvironment& e)
	{
		IStructure* pStruct;
		ReadInput(0, (void*&)pStruct, e);

		int32 count = pStruct->InterfaceCount();
		WriteOutput(0, count, e);
	}

	void NativeAppendInputTypeAndName(NativeCallEnvironment& e)
	{
		int32 interfaceIndex;
		ReadInput(0, interfaceIndex, e);

		int32 methodIndex;
		ReadInput(1, methodIndex, e);

		int32 inputIndex;
		ReadInput(2, inputIndex, e);

		InterfacePointer ipSbType;
		ReadInput(3, ipSbType, e);
		auto* sbTypeObject = (CClassSysTypeStringBuilder*)InterfaceToInstance(ipSbType);

		InterfacePointer ipSbName;
		ReadInput(4, ipSbName, e);
		auto* sbNameObject = (CClassSysTypeStringBuilder*)InterfaceToInstance(ipSbName);

		IStructure* pStruct;
		ReadInput(5, (void*&)pStruct, e);

		int32 count = pStruct->InterfaceCount();
		if (interfaceIndex < 0 || interfaceIndex >= count)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.GetInputTypeAndName: bad [interfaceIndex]");
			return;
		}

		auto& myInterface = pStruct->GetInterface(interfaceIndex);

		int32 nMethods = myInterface.MethodCount();
		if (methodIndex < 0 || methodIndex >= nMethods)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.GetInputTypeAndName: bad [methodIndex]");
			return;
		}

		auto& m = myInterface.GetMethod(methodIndex);
		if (inputIndex < 0 || inputIndex >= m.NumberOfInputs())
		{
			// Too much info for throw from native
			Throw(0, "GetInputTypeAndName: InputIndex %d was out of bounds for method %s.%s", inputIndex, myInterface.Name(), m.Name());
		}

		cstr argName = m.GetArgName(inputIndex + m.NumberOfOutputs());
		sbNameObject->AppendAndTruncate(to_fstring(argName));

		cstr argTypeName = m.GetArgument(inputIndex + m.NumberOfOutputs()).Name();
		sbTypeObject->AppendAndTruncate(to_fstring(argTypeName));
	}

	void NativeAppendOutputTypeAndName(NativeCallEnvironment& e)
	{
		int32 interfaceIndex;
		ReadInput(0, interfaceIndex, e);

		int32 methodIndex;
		ReadInput(1, methodIndex, e);

		int32 outputIndex;
		ReadInput(2, outputIndex, e);

		InterfacePointer ipSbType;
		ReadInput(3, ipSbType, e);

		auto* sbTypeObject = (CClassSysTypeStringBuilder*)InterfaceToInstance(ipSbType);

		InterfacePointer ipSbName;
		ReadInput(4, ipSbName, e);

		auto* sbNameObject = (CClassSysTypeStringBuilder*)InterfaceToInstance(ipSbName);

		IStructure* pStruct;
		ReadInput(5, (void*&)pStruct, e);

		int32 count = pStruct->InterfaceCount();
		if (interfaceIndex < 0 || interfaceIndex >= count)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.GetOutputTypeAndName: bad [interfaceIndex]");
			return;
		}

		auto& myInterface = pStruct->GetInterface(interfaceIndex);

		int32 nMethods = myInterface.MethodCount();
		if (methodIndex < 0 || methodIndex >= nMethods)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.GetOutputTypeAndName: bad [methodIndex]");
			return;
		}

		auto& m = myInterface.GetMethod(methodIndex);
		if (outputIndex < 0 || outputIndex >= m.NumberOfOutputs())
		{
			// Too much info for throw from native
			Throw(0, "GetOutputTypeAndName: OutputIndex %d was out of bounds for method %s.%s", outputIndex, myInterface.Name(), m.Name());
		}

		cstr argName = m.GetArgName(outputIndex);
		sbNameObject->AppendAndTruncate(to_fstring(argName));

		cstr argTypeName = m.GetArgument(outputIndex).Name();
		sbTypeObject->AppendAndTruncate(to_fstring(argTypeName));
	}

	void NativeGetMethodArgCounts(NativeCallEnvironment& e)
	{
		int32 interfaceIndex;
		ReadInput(0, interfaceIndex, e);

		int32 methodIndex;
		ReadInput(1, methodIndex, e);

		IStructure* pStruct;
		ReadInput(2, (void*&)pStruct, e);

		int32 count = pStruct->InterfaceCount();
		if (interfaceIndex < 0 || interfaceIndex >= count)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.GetMethodArgCounts: bad [interfaceIndex]");
			return;
		}

		auto& myInterface = pStruct->GetInterface(interfaceIndex);
		
		int32 nMethods = myInterface.MethodCount();
		if (methodIndex < 0 || methodIndex >= nMethods)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.GetMethodArgCounts: bad [methodIndex]");
			return;
		}

		auto& m = myInterface.GetMethod(methodIndex);
		int nInputs = m.NumberOfInputs();
		int nOutputs = m.NumberOfOutputs();
		WriteOutput(0, nInputs, e);
		WriteOutput(1, nOutputs, e);
	}

	void NativeGetMethodCount(NativeCallEnvironment& e)
	{
		int32 index;
		ReadInput(0, index, e);

		IStructure* pStruct;
		ReadInput(1, (void*&)pStruct, e);

		int32 count = pStruct->InterfaceCount();
		if (index < 0 || index >= count)
		{
			int32 zero = 0;
			WriteOutput(0, zero, e);
		}
		else
		{
			int32 nMethods = pStruct->GetInterface(index).MethodCount();
			WriteOutput(0, nMethods, e);
		}

	}

	void NativeGetInputType(NativeCallEnvironment& e)
	{
		int32 interfaceIndex;
		ReadInput(0, interfaceIndex, e);

		int32 methodIndex;
		ReadInput(1, methodIndex, e);

		int32 inputIndex;
		ReadInput(2, inputIndex, e);

		IStructure* pStruct;
		ReadInput(3, (void*&)pStruct, e);

		int32 count = pStruct->InterfaceCount();
		if (interfaceIndex < 0 || interfaceIndex >= count)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.GetInputType: bad [interfaceIndex]");
			return;
		}

		auto& myInterface = pStruct->GetInterface(interfaceIndex);

		int32 nMethods = myInterface.MethodCount();
		if (methodIndex < 0 || methodIndex >= nMethods)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.GetInputType: bad [methodIndex]");
			return;
		}

		auto& m = myInterface.GetMethod(methodIndex);
		if (inputIndex < 0 || inputIndex >= m.NumberOfInputs())
		{
			// Too much info for throw from native
			Throw(0, "GetInputType: InputIndex %d was out of bounds for method %s.%s", inputIndex, myInterface.Name(), m.Name());
		}

		auto& SS = (IScriptSystem&)e.ss;
		auto& argType = m.GetArgument(inputIndex + m.NumberOfOutputs());
		CReflectedClass* pArgStruct = SS.GetReflectedClass((void*) &argType);
		if (pArgStruct == NULL)
		{
			pArgStruct = SS.CreateReflectionClass("Structure", (void*) &argType);
		}

		WriteOutput(0, pArgStruct->header.AddressOfVTable0(), e);
	}

	void NativeGetOutputType(NativeCallEnvironment& e)
	{
		int32 interfaceIndex;
		ReadInput(0, interfaceIndex, e);

		int32 methodIndex;
		ReadInput(1, methodIndex, e);

		int32 outputIndex;
		ReadInput(2, outputIndex, e);

		IStructure* pStruct;
		ReadInput(3, (void*&)pStruct, e);

		int32 count = pStruct->InterfaceCount();
		if (interfaceIndex < 0 || interfaceIndex >= count)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.GetOutputType: bad [interfaceIndex]");
			return;
		}

		auto& myInterface = pStruct->GetInterface(interfaceIndex);

		int32 nMethods = myInterface.MethodCount();
		if (methodIndex < 0 || methodIndex >= nMethods)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.v: bad [methodIndex]");
			return;
		}

		auto& m = myInterface.GetMethod(methodIndex);
		if (outputIndex < 0 || outputIndex >= m.NumberOfOutputs())
		{
			// Too much info for throw from native
			Throw(0, "GetOutputType: OutputIndex %d was out of bounds for method %s.%s", outputIndex, myInterface.Name(), m.Name());
		}

		auto& SS = (IScriptSystem&)e.ss;
		auto& argType = m.GetArgument(outputIndex);
		CReflectedClass* pArgStruct = SS.GetReflectedClass((void*)&argType);
		if (pArgStruct == NULL)
		{
			pArgStruct = SS.CreateReflectionClass("Structure", (void*)&argType);
		}

		WriteOutput(0, pArgStruct->header.AddressOfVTable0(), e);
	}

	void NativeIsMethodInputOfType(NativeCallEnvironment& e)
	{
		int32 interfaceIndex;
		ReadInput(0, interfaceIndex, e);

		int32 methodIndex;
		ReadInput(1, methodIndex, e);

		int32 inputIndex;
		ReadInput(2, inputIndex, e);

		InterfacePointer ip;
		ReadInput(3, ip, e);

		CReflectedClass* candidateObject = (CReflectedClass*)InterfaceToInstance(ip);
		IStructure* pCandidateStruct = (IStructure*) candidateObject->context;

		IStructure* pStruct;
		ReadInput(4, (void*&)pStruct, e);

		int32 count = pStruct->InterfaceCount();
		if (interfaceIndex < 0 || interfaceIndex >= count)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.IsMethodInputOfType: bad [interfaceIndex]");
			return;
		}

		auto& myInterface = pStruct->GetInterface(interfaceIndex);

		int32 nMethods = myInterface.MethodCount();
		if (methodIndex < 0 || methodIndex >= nMethods)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.IsMethodInputOfType: bad [methodIndex]");
			return;
		}

		auto& m = myInterface.GetMethod(methodIndex);
		if (inputIndex < 0 || inputIndex >= m.NumberOfInputs())
		{
			// Too much info for throw from native
			Throw(0, "IsMethodInputOfType: InputIndex %d was out of bounds for method %s.%s", inputIndex, myInterface.Name(), m.Name());
		}

		auto& argType = m.GetArgument(inputIndex + m.NumberOfOutputs());

		boolean32 match = (argType == *pCandidateStruct);

		WriteOutput(0, match, e);
	}

	void NativeIsMethodOutputOfType(NativeCallEnvironment& e)
	{
		int32 interfaceIndex;
		ReadInput(0, interfaceIndex, e);

		int32 methodIndex;
		ReadInput(1, methodIndex, e);

		int32 outputIndex;
		ReadInput(2, outputIndex, e);

		InterfacePointer ip;
		ReadInput(3, ip, e);

		CReflectedClass* candidateObject = (CReflectedClass*)InterfaceToInstance(ip);
		IStructure* pCandidateStruct = (IStructure*)candidateObject->context;

		IStructure* pStruct;
		ReadInput(4, (void*&)pStruct, e);

		int32 count = pStruct->InterfaceCount();
		if (interfaceIndex < 0 || interfaceIndex >= count)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.IsMethodOutputOfType: bad [interfaceIndex]");
			return;
		}

		auto& myInterface = pStruct->GetInterface(interfaceIndex);

		int32 nMethods = myInterface.MethodCount();
		if (methodIndex < 0 || methodIndex >= nMethods)
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.IsMethodOutputOfType: bad [methodIndex]");
			return;
		}

		auto& m = myInterface.GetMethod(methodIndex);
		if (outputIndex < 0 || outputIndex >= m.NumberOfOutputs())
		{
			// Too much info for throw from native
			Throw(0, "IsMethodOutputOfType: OutputIndex %d was out of bounds for method %s.%s", outputIndex, myInterface.Name(), m.Name());
		}

		auto& argType = m.GetArgument(outputIndex);

		boolean32 match = (argType == *pCandidateStruct);

		WriteOutput(0, match, e);
	}


	void NativeGetInterfaceName(NativeCallEnvironment& e)
	{
		int32 index;
		ReadInput(0, index, e);

		IStructure* pStruct;
		ReadInput(1, (void*&)pStruct, e);

		auto& SS = ((IScriptSystem&)e.ss);

		int32 count = pStruct->InterfaceCount();
		if (index < 0 || index >= count)
		{
			auto* nullObject = SS.ProgramObject().Common().SysTypeIString().UniversalNullInstance();
			WriteOutput(0, &nullObject->pVTables[0], e);
		}
		else
		{
			auto& myInterface = pStruct->GetInterface(index);
			cstr interfaceName = myInterface.Name();
			auto sc = SS.DuplicateStringAsConstant(interfaceName);
			WriteOutput(0, &sc->header.pVTables[0], e);
		}
	}

	void DuplicateExpression(cr_sex s, ISExpressionBuilder& b)
	{
		switch (s.Type())
		{		
		case EXPRESSION_TYPE_STRING_LITERAL:
			b.AddStringLiteral(s.c_str());
			return;
		case EXPRESSION_TYPE_ATOMIC:
			{
				cstr token = s.c_str();
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
         b.AddStringLiteral(s.c_str());
         return;
      case EXPRESSION_TYPE_ATOMIC:
      {
         cstr token = s.c_str();
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
			b.AddStringLiteral(format.c_str());
			return;
		case EXPRESSION_TYPE_ATOMIC:
			{
				cstr token = format.c_str();
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
		ISExpressionBuilder* pBuilder;
		ReadInput(0, (void*&) pBuilder, e);

		const char* pBuffer;
		ReadInput(1, (void*&) pBuffer, e);

		pBuilder->AddAtomic(pBuffer);
	}

	void NewExpressionBuilder(NativeCallEnvironment& e)
	{
		InterfacePointer ipOrigin;
		ReadInput(0, ipOrigin, e);

		auto* classExpr = (CClassExpression*) InterfaceToInstance(ipOrigin);
		auto* pS = classExpr->ExpressionPtr;
		if (pS == nullptr)
		{
			Throw(0, "%s: Unexpected null pointer", __FUNCTION__);
		}

		cr_sex sOrigin = *pS;

		auto& ss = (IScriptSystem&)e.ss;
		ObjectStubWithHandle* object = (ObjectStubWithHandle*) ss.CreateScriptObject("ExpressionBuilder", "!scripts/native/Sys.Reflection.sxy");
		object->handle = ss.CreateMacroTransform(sOrigin);
		InterfacePointer ip = &object->stub.pVTables[0];
		WriteOutput(0, ip, e);
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
		ISExpressionBuilder* pBuilder;
		ReadInput(0, (void*&) pBuilder, e);
		
		ISExpression* pSource;
		ReadInput(1, (void*&) pSource, e);

		DuplicateExpression(*pSource, *pBuilder);
	}

   void NativeExpressionBuilderAddCopyToString(NativeCallEnvironment& e)
   {
      ISExpressionBuilder* pBuilder;
      ReadInput(0, (void*&)pBuilder, e);

      ISExpression* pSource;
      ReadInput(1, (void*&)pSource, e);

      DuplicateExpressionAsString(*pSource, *pBuilder);
   }

	void NativeExpressionBuilderSubstitute(NativeCallEnvironment& e)
	{
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

	void NativeAppendMethodName(NativeCallEnvironment& e)
	{
		InterfacePointer ipSb;
		ReadInput(0, ipSb, e);

		auto* sbObject = (CClassSysTypeStringBuilder*) InterfaceToInstance(ipSb);


		int32 interfaceIndex;
		ReadInput(1, interfaceIndex, e);

		int32 methodIndex;
		ReadInput(2, methodIndex, e);

		IStructure* pStruct;
		ReadInput(3, pStruct, e);

		if (interfaceIndex < 0 || interfaceIndex >= pStruct->InterfaceCount())
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.AppendMethodName: interfaceIndex was out of bounds");
			return;
		}

		auto& myInterface = pStruct->GetInterface(0);
		if (methodIndex < 0 || methodIndex > myInterface.MethodCount())
		{
			e.ss.ThrowFromNativeCode(0, "IStructure.AppendMethodName: method Index was out of bounds");
			return;
		}

		cstr methodName = pStruct->GetInterface(interfaceIndex).GetMethod(methodIndex).Name();

		int nBytesWritten = SafeFormat(sbObject->buffer + sbObject->length, sbObject->capacity - sbObject->length, "%s", methodName);
		if (nBytesWritten < 0)
		{
			e.ss.ThrowFromNativeCode(GetLastError(), "IStructure.AppendMethodName: Error appending method name");
			return;
		}

		sbObject->length = (int32) strlen(sbObject->buffer);

		WriteOutput(0, nBytesWritten, e);
	}

	void AddReflectionCalls(IScriptSystem& ss)
	{
		const INamespace& sysReflection = ss.AddNativeNamespace("Sys.Reflection");
		ss.AddNativeCall(sysReflection, NewExpressionBuilder, &ss, "NewExpressionBuilder (Sys.Reflection.IExpression origin) -> (Sys.Reflection.IExpressionBuilder builder)", __FILE__, __LINE__, true);

		const INamespace& sysReflectionNative = ss.AddNativeNamespace("Sys.Reflection.Native");
		ss.AddNativeCall(sysReflectionNative, NativeAppendMethodName, &ss, "AppendMethodName (Sys.Type.IStringBuilder sb) (Int32 interfaceIndex)(Int32 methodIndex) (Pointer structPtr) -> (Int32 nameLength)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionGetChild, &ss, "ExpressionGetChild (Pointer sPtr) (Int32 index) ->  (Sys.Reflection.IExpression child)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionIndexOf, &ss, "ExpressionIndexOf (Pointer sParentHandle) (Pointer sChildHandle) -> (Int32 index)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionGetParent, &ss, "ExpressionGetParent (Pointer sPtr) -> (Sys.Reflection.IExpression parent)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionChildCount, &ss, "ExpressionChildCount (Pointer sPtr) -> (Int32 count)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionAppendTextTo, &ss, "ExpressionAppendTextTo  (Pointer sPtr) (Sys.Type.IStringBuilder sb)->", __FILE__, __LINE__);
		ss.AddNativeCall(sysReflectionNative, NativeGetExpressionText, &ss, "GetExpressionText  (Pointer sPtr) -> (Sys.Type.IString name)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetExpressionType, &ss, "GetExpressionType  (Pointer sPtr) -> (Sys.Type.Int32 type)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionThrow, &ss, "ExpressionThrow  (Pointer sPtr)(Int32 errorCode)(Sys.Type.Pointer buffer) ->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetScriptSystem, &ss, "GetScriptSystem -> (Sys.Reflection.IScriptSystem ss)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeModuleCount, &ss, "ModuleCount -> (Int32 count)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeAppendInputTypeAndName, &ss, "AppendInputTypeAndName(Int32 interfaceIndex)(Int32 methodIndex)(Int32 inputIndex)(IStringBuilder typeBuilder) (IStringBuilder nameBuilder) (Pointer pStruct) -> ", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeAppendOutputTypeAndName, &ss, "AppendOutputTypeAndName(Int32 interfaceIndex)(Int32 methodIndex)(Int32 outputIndex)(IStringBuilder typeBuilder) (IStringBuilder nameBuilder) (Pointer pStruct) -> ", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetInputType, &ss, "GetInputType (Int32 interfaceIndex)(Int32 methodIndex)(Int32 inputIndex)(Sys.Type.Pointer pStruct) -> (Sys.Reflection.IStructure argType)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetOutputType, &ss, "GetOutputType (Int32 interfaceIndex)(Int32 methodIndex)(Int32 outputIndex)(Sys.Type.Pointer pStruct) -> (Sys.Reflection.IStructure argType)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeIsMethodInputOfType, &ss, "IsMethodInputOfType(Int32 interfaceIndex)(Int32 methodIndex)(Int32 inputIndex) (Sys.Reflection.IStructure argTypeCandidate)(Sys.Type.Pointer pStruct)-> (Bool match)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeIsMethodOutputOfType, &ss, "IsMethodOutputOfType(Int32 interfaceIndex)(Int32 methodIndex)(Int32 inputIndex) (Sys.Reflection.IStructure argTypeCandidate)(Sys.Type.Pointer pStruct)-> (Bool match)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetModule, &ss, "GetModule (Int32 index) -> (Sys.Reflection.IModule module)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetModuleName, &ss, "GetModuleName (Pointer modulePtr) -> (Sys.Type.IString name)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetStructCount, &ss, "GetStructCount (Pointer modulePtr) -> (Int32 structCount)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetStruct, &ss, "GetStruct (Pointer modulePtr) (Int32 index) -> (Sys.Reflection.IStructure structure)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetStructName, &ss, "GetStructName (Pointer structPtr) -> (Sys.Type.IString name)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetInterfaceCount, &ss, "GetInterfaceCount (Pointer structPtr) -> (Int32 count)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetInterfaceName, &ss, "GetInterfaceName (Int32 interfaceIndex) (Pointer structPtr) -> (IString nameOrNull)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetMethodArgCounts, &ss, "GetMethodArgCounts (Int32 interfaceIndex)(Int32 methodIndex) (Pointer structPtr) -> (Int32 inputCount)(Int32 outputCount)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeGetMethodCount, &ss, "GetMethodCount (Int32 interfaceIndex) (Pointer structPtr) -> (Int32 count)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddAtomic, &ss, "ExpressionBuilderAddAtomic (Pointer builderPtr) (Pointer strBuffer) ->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddCompound, &ss, "ExpressionBuilderAddCompound (Pointer builderPtr) -> (Sys.Reflection.IExpressionBuilder child)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderInsertCompoundAfter, &ss, "ExpressionBuilderInsertCompoundAfter (Pointer builderPtr)(Int32 index) -> (Sys.Reflection.IExpressionBuilder child)", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddCopy, &ss, "ExpressionBuilderAddCopy (Pointer builderPtr) (Pointer xpressPtr) ->", __FILE__, __LINE__, true);
        ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderAddCopyToString, &ss, "ExpressionBuilderAddCopyToString (Pointer builderPtr) (Pointer xpressPtr) ->", __FILE__, __LINE__, true);
		ss.AddNativeCall(sysReflectionNative, NativeExpressionBuilderSubstitute, &ss, "ExpressionBuilderSubstitute (Pointer builderPtr) (Pointer inputPtr) (Pointer formatPtr) -> (Int32 errorCount)", __FILE__, __LINE__, true);
	}
}