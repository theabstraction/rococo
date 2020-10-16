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

#include <cerrno>

namespace Rococo
{
   namespace Script
   {
	   void DeleteMembers(IScriptSystem& ss, const IStructure& type, uint8* pInstance);

	   struct ArrayImage
	   {
		   void* Start;
		   int32 NumberOfElements;
		   int32 ElementCapacity;
		   const IStructure* ElementType;
		   int32 ElementLength;
		   int32 LockNumber;
	   };

	   size_t SizeOfElement(const IStructure& type)
	   {
		   return type.InterfaceCount() > 0 ? sizeof(size_t) : type.SizeOfStruct();
	   }

	   VM_CALLBACK(ArrayInit)
	   {
		   IScriptSystem& ss = *(IScriptSystem*) context;
		   const IStructure* elementType = (const IStructure*) registers[VM::REGISTER_D4].vPtrValue;
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D5].vPtrValue;
		   int32 capacity = registers[VM::REGISTER_D7].int32Value;
		
		   a->ElementCapacity = capacity;
		   a->ElementLength = (int32) SizeOfElement(*elementType);
		   a->ElementType = elementType;
		   a->NumberOfElements = 0;
		   a->LockNumber = 0;
		   a->Start = ss.AlignedMalloc(16, capacity * a->ElementLength);
	   }

	   void ArrayDelete(ArrayImage* a, IScriptSystem& ss)
	   {
		   ss.AlignedFree(a->Start);
	   }

	   VM_CALLBACK(ArrayDelete)
	   {
		   IScriptSystem& ss = *(IScriptSystem*) context;
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D7].vPtrValue;
		   ArrayDelete(a, ss);
	   }

	   void DestroyElements(ArrayImage& a, IScriptSystem& ss)
	   {
		   auto& obj = ss.ProgramObject();
		   if (a.ElementType->InterfaceCount() > 0)
		   {
			   int offset = 0;
			   for (int i = 0; i < a.NumberOfElements; ++i)
			   {
				   uint8* item = (uint8*)a.Start + offset;
				   InterfacePointer pInterface = *(InterfacePointer*)item;
				   obj.DecrementRefCount(pInterface);
				   offset += sizeof(size_t);
			   }
		   }
		   else if (a.ElementType->VarType() == VARTYPE_Derivative)
		   {
			   for (int i = 0; i < a.NumberOfElements; ++i)
			   {
				   uint8* item = (uint8*)a.Start + i * a.ElementLength;
				   DeleteMembers(ss, *a.ElementType, item);
			   }
		   }

		   a.NumberOfElements = 0;
	   }

	   VM_CALLBACK(ArrayDestructElements)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D7].vPtrValue;
		   IScriptSystem& ss = *(IScriptSystem*) context;
		   DestroyElements(*a, ss);
	   }

	   VM_CALLBACK(ArrayPushAndGetRef)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D7].vPtrValue;

		   if (a->NumberOfElements < a->ElementCapacity)
		   {
			   uint8* pElement = ((uint8* )a->Start) + a->NumberOfElements * a->ElementLength;
			   memset(pElement, 0, a->ElementLength);
			   a->NumberOfElements++;
			   registers[VM::REGISTER_D8].vPtrValue = pElement;
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Push failed: the array was full"));
		   }
	   }

	   VM_CALLBACK(ArrayPush32)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   int32 value = registers[VM::REGISTER_D7].int32Value;

		   if (a->NumberOfElements < a->ElementCapacity)
		   {
			   int32* intBuffer = (int32*) a->Start;
			   intBuffer[a->NumberOfElements++] = value;
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Push failed: the array was full"));
		   }
	   }

	   VM_CALLBACK(ArrayPushInterface)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D4].vPtrValue;
		   InterfacePointer pObject = (InterfacePointer) registers[VM::REGISTER_D7].vPtrValue;

		   IScriptSystem& ss = *(IScriptSystem*)context;

		   if (a->NumberOfElements < a->ElementCapacity)
		   {
			   InterfacePointer* intBuffer = (InterfacePointer*) a->Start;
			   intBuffer[a->NumberOfElements++] = pObject;
			   ss.ProgramObject().IncrementRefCount(pObject);
		   }
		   else
		   {
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Push failed: the array was full"));
		   }
	   }

	   VM_CALLBACK(ArrayPush64)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   int64 value = registers[VM::REGISTER_D7].int64Value;

		   if (a->NumberOfElements < a->ElementCapacity)
		   {
			   int64* intBuffer = (int64*) a->Start;
			   intBuffer[a->NumberOfElements++] = value;
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Push failed: the array was full"));
		   }
	   }

	   VM_CALLBACK(ArraySet32)
	   {
		   const int index = registers[VM::REGISTER_D4].int32Value;

		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D5].vPtrValue;
		   int32 value = registers[VM::REGISTER_D7].int32Value;

		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   int32* intBuffer = (int32*) a->Start;
			   intBuffer[index] = value;
		   }
		   else
		   {
			   if (index >= 0 && index < a->ElementCapacity)
			   {
				   // init everything new to zero
				   int32* p = ((int32*) a->Start) + a->NumberOfElements;
				   for (int i = 0; i < index - a->NumberOfElements; ++i)
				   {
					   *p++ = 0; 
				   }

				   *p = value;
				   a->NumberOfElements = index + 1;
				   return;
			   }

			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Set failed: the index was out of range"));
		   }
	   }

	   VM_CALLBACK(ArraySet64)
	   {
		   const int index = registers[VM::REGISTER_D4].int32Value;
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D5].vPtrValue;
		   int64 value = registers[VM::REGISTER_D7].int64Value;

		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   int64* int64Buffer = (int64*) a->Start;
			   int64Buffer[index] = value;
		   }
		   else
		   {
			   if (index >= 0 && index < a->ElementCapacity)
			   {
				   // init everything new to zero
				   int64* p = ((int64*) a->Start) + a->NumberOfElements;
				   for (int i = 0; i < index - a->ElementLength; ++i)
				   {
					   *p++ = 0; 
				   }

				   *p = value;
				   a->NumberOfElements = index + 1;
				   return;
			   }

			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Set failed: the index was out of range"));
		   }
	   }

	   VM_CALLBACK(ArraySetByRef)
	   {
		   const int index = registers[VM::REGISTER_D4].int32Value;
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D5].vPtrValue;
		   const void* pValue = registers[VM::REGISTER_D7].vPtrValue;

		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   void* pTarget =  ((uint8*) a->Start) + index * a->ElementLength;
			   AlignedMemcpy(pTarget, pValue, a->ElementLength);
		   }
		   else
		   {
			   if (index >= 0 && index < a->ElementCapacity)
			   {
				   // init everything under the index to zero
				   void* p = ((uint8*) a->Start) + a->NumberOfElements * a->ElementLength;
				   memset(p, 0, (index - a->NumberOfElements) * a->ElementLength);
				   p = ((uint8*) a->Start) + index * a->ElementLength;
				   AlignedMemcpy(p, pValue, a->ElementLength);
				   a->NumberOfElements = index + 1;
				   return;
			   }

			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Set failed: the index was out of range"));
		   }
	   }

	   bool IsLocked(const ArrayImage& a) { return a.LockNumber != 0; }

	   void ReleaseElement(ArrayImage& a, int32 index, IScriptSystem& ss)
	   {
		   if (a.ElementType->InterfaceCount() > 0)
		   {
			   auto& obj = ss.ProgramObject();

			   int offset = sizeof(InterfacePointer) * index;

			   uint8* item = (uint8*)a.Start + offset;
			   InterfacePointer pInterface = *(InterfacePointer*)item;
			   obj.DecrementRefCount(pInterface);
		   }
		   else if (a.ElementType->VarType() == VARTYPE_Derivative)
		   {
			   uint8* item = (uint8*)a.Start + index * a.ElementLength;
			   DeleteMembers(ss, *a.ElementType, item);
		   }
	   }

	   VM_CALLBACK(ArrayPop)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D7].vPtrValue;

		   IScriptSystem& ss = *(IScriptSystem*)context;

		   if (IsLocked(*a))
		   {
			   ss.ThrowFromNativeCode(ERANGE, ("Array.PopOut failed: the array was locked for enumeration"));
			   return;
		   }
		
		   if (a->NumberOfElements > 0)
		   {
			   a->NumberOfElements--;
			   ReleaseElement(*a, a->NumberOfElements, ss);
		   }
		   else
		   {
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Pop failed: the array was empty"));
			   return;
		   }
	   }

	   VM_CALLBACK(ArrayClear)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D7].vPtrValue;
		   IScriptSystem& ss = *(IScriptSystem*)context;

		   if (IsLocked(*a))
		   {
			   ss.ThrowFromNativeCode(ERANGE, "Array.Clear failed: the array was locked for enumeration");
			   return;
		   }

		   while(a->NumberOfElements > 0)
		   {
			   a->NumberOfElements--;
			   ReleaseElement(*a, a->NumberOfElements, ss);
		   }
	   }

	   VM_CALLBACK(ArrayPopOut32)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;

		   if (IsLocked(*a))
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.PopOut failed: the array was locked for enumeration"));
		   }
		
		   if (a->NumberOfElements > 0)
		   {
			   a->NumberOfElements--;
			   registers[VM::REGISTER_D7].int32Value = ((const int32*) a->Start)[a->NumberOfElements];
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.PopOut failed: the array was empty"));
		   }
	   }

	   VM_CALLBACK(ArrayPopOut64)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;

		   if (IsLocked(*a))
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.PopOut failed: the array was locked for enumeration"));
		   }
		
		   if (a->NumberOfElements > 0)
		   {
			   a->NumberOfElements--;
			   registers[VM::REGISTER_D7].int64Value = ((const int64*) a->Start)[a->NumberOfElements];
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.PopOut failed: the array was empty"));
		   }
	   }

	   VM_CALLBACK(ArrayPushByRef)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   const void* pValue = registers[VM::REGISTER_D7].vTable;

		   if (a->NumberOfElements < a->ElementCapacity)
		   {
			   void* pTargetElement = ((uint8*) a->Start) + (a->ElementLength * a->NumberOfElements);
			   AlignedMemcpy(pTargetElement, pValue, a->ElementLength);
			   a->NumberOfElements++;
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Push failed: the array was full"));
		   }
	   }

	   VM_CALLBACK(ArrayGet32)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   int32 index = registers[VM::REGISTER_D7].int32Value;

		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   const int32* p = ((const int32*) a->Start) + index;
			   registers[VM::REGISTER_D7].int32Value = *p;
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Get failed: the index was out of range"));
		   }
	   }

	   VM_CALLBACK(ArrayGet64)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   int32 index = registers[VM::REGISTER_D7].int32Value;

		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   const int64* p = ((const int64*) a->Start) + index;
			   registers[VM::REGISTER_D7].int64Value = *p;
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Get failed: the index was out of range"));
		   }
	   }

	   VM_CALLBACK(ArrayGetMember32)
	   {
		   int32 offset = registers[VM::REGISTER_D5].int32Value;
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   int32 index = registers[VM::REGISTER_D7].int32Value;
		
		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   const uint8* pElement = ((const uint8*) a->Start) + index * a->ElementLength;
			   const int32* pMember = (const int32*) (pElement + offset);
			   registers[VM::REGISTER_D7].int32Value = *pMember;
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Get failed: the index was out of range"));
		   }
	   }

	   VM_CALLBACK(ArrayGetMember64)
	   {
		   int32 offset = registers[VM::REGISTER_D5].int32Value;
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   int32 index = registers[VM::REGISTER_D7].int32Value;

		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   const uint8* pElement = ((const uint8*) a->Start) + index * a->ElementLength;
			   const int64* pMember = (const int64*) (pElement + offset);
			   registers[VM::REGISTER_D7].int64Value = *pMember;
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Get failed: the index was out of range"));
		   }
	   }

	   VM_CALLBACK(ArrayGetByRef)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   int32 index = registers[VM::REGISTER_D7].int32Value;
		   void* structRef = registers[VM::REGISTER_D5].vPtrValue;

		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   const void* src = ((const uint8*) a->Start) + index * a->ElementLength;
			   AlignedMemcpy(structRef, src, a->ElementLength);
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.Get failed: the index was out of range"));
		   }
	   }

	   VM_CALLBACK(ArrayGetRefUnchecked)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D13].vPtrValue;
		   int32 index = registers[VM::REGISTER_D12].int32Value;
		   uint8* pElement = ((uint8*) a->Start) + index * a->ElementLength;
		   registers[VM::REGISTER_D7].vPtrValue = pElement;

   #ifdef _DEBUG
		   if (!IsLocked(*a))
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.ArrayGetRefUnchecked failed: array was unlocked, internal compiler error"));
		   }

		   if (index < 0 || index >= a->NumberOfElements)
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.ArrayGetRefUnchecked failed: index out of range, internal compiler error"));
		   }
   #endif

	   }

	   VM_CALLBACK(ArrayGetInterfaceUnchecked)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D13].vPtrValue;
		   int32 index = registers[VM::REGISTER_D12].int32Value;
		   uint8* pElement = ((uint8*)a->Start) + index * a->ElementLength;
		   InterfacePointer* ppInterface = (InterfacePointer*)pElement;
		   ObjectStub* pObject = InterfaceToInstance(*ppInterface);
		   pObject->refCount++;
		   registers[VM::REGISTER_D7].vPtrValue = *ppInterface;

#ifdef _DEBUG
		   if (!IsLocked(*a))
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.ArrayGetRefUnchecked failed: array was unlocked, internal compiler error"));
		   }

		   if (index < 0 || index >= a->NumberOfElements)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCode(ERANGE, ("Array.ArrayGetRefUnchecked failed: index out of range, internal compiler error"));
		   }
#endif

	   }

	   VM_CALLBACK(ArrayLock)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D13].vPtrValue;

		   if (!IsLocked(*a))
		   {			
			   a->LockNumber = 1;			
		   }
		   else
		   {
			   if (a->LockNumber == 0x7FFFFFFF)
			   {
				   IScriptSystem& ss = *(IScriptSystem*) context;
				   ss.ThrowFromNativeCode(0, "ArrayLock failed: the array lock reached the upper limit of 0x7FFFFFFF");
			   }

			   a->LockNumber++;
		   }
	   }

	   VM_CALLBACK(ArrayUnlock)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D13].vPtrValue;

		   if (IsLocked(*a))
		   {			
			   if (a->LockNumber <= 0)
			   {
				   IScriptSystem& ss = *(IScriptSystem*)context;
				   ss.ThrowFromNativeCode(0, "ArrayUnlock failed: the array lock was not positive");
			   }
			   a->LockNumber--;			
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCode(0, "Array.Unlock: the array was not locked");
		   }
	   }

	   void CompileConstructArrayElementFromRef(CCompileEnvironment& ce, cr_sex s, const IStructure& elementType, cstr arrayInstance)
	   {
		   const IFunction* constructor = elementType.Constructor();

		   // (<instance>.Push <constructorArg1> ... <constructorArgN>)
		   int inputCount = s.NumberOfElements() - 1;
		   if (inputCount < constructor->NumberOfInputs() - 1)
		   {
			   Throw(s, ("Too few arguments in push constructor call"));
		   }
		   else if (inputCount >  constructor->NumberOfInputs() - 1)
		   {
			   Throw(s, ("Too many arguments in push constructor call"));
		   }

		 
		   int inputStackAllocCount = PushInputs(ce, s, *constructor, true, 1);

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);
		   ce.Builder.AssignVariableRefToTemp(arrayInstance, Rococo::ROOT_TEMPDEPTH, 0); // array goes to D7
		   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayPushAndGetRef); // This returns the address of the blank slot in D8

		   inputStackAllocCount += CompileInstancePointerArgFromTemp(ce, Rococo::ROOT_TEMPDEPTH + 1);

		   AppendFunctionCallAssembly(ce, *constructor); 

		   ce.Builder.MarkExpression(&s);

		   RepairStack(ce, *s.Parent(), *constructor);
		   ce.Builder.AssignClosureParentSFtoD6();
	   }

	   void CompileAsPushToArrayByRef(CCompileEnvironment& ce, cr_sex s, cstr instanceName)
	   {
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, instanceName);

		   if (s.NumberOfElements() == 3)
		   {
			   cr_sex memberwiseIndicator = GetAtomicArg(s, 1);
			   if (!AreEqual(memberwiseIndicator.String(), GetFriendlyName(elementType)))
			   {
				   Throw(s, "Expecting either (%s.Push <arg>), (%s.Push ( element-constructor-args...)) or (%s.Push %s (memberwise-constructor-args...)", instanceName, instanceName, instanceName, GetFriendlyName(elementType));
			   }

			   if (IsNullType(elementType))
			   {
				   ce.Builder.AssignVariableToTemp(instanceName, Rococo::ROOT_TEMPDEPTH, 0); // array goes to D7
				   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayPushAndGetRef); // D8 now contains the ref to the newly created element
			   }
			   else
			   {
				   ce.Builder.AssignVariableRefToTemp(instanceName, Rococo::ROOT_TEMPDEPTH, 0); // array goes to D7
				   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayPushAndGetRef); // D8 now contains the ref to the newly created element
			   }

			   cr_sex memberwiseArgs = s.GetElement(2);
			   if (!IsNull(memberwiseArgs) && !IsCompound(memberwiseArgs))
			   {
				   Throw(memberwiseArgs, ("Expecting either a null or compound expression"));
			   }

			   ConstructMemberByRef(ce, memberwiseArgs, REGISTER_D8, elementType, 0);
		   }
		   else // 2
		   {
			   const IFunction* constructor = elementType.Constructor();
			   if (constructor != NULL)
			   {				
				   CompileConstructArrayElementFromRef(ce, s, elementType, instanceName);
			   }
			   else
			   {
				   // No constructor, so need to copy element
				   cr_sex value = s.GetElement(1);
				   CompileGetStructRef(ce, value, elementType, "newArrayElement"); // The address of the value is in D7
				   ce.Builder.AssignVariableRefToTemp(instanceName, 0, 0); // array goes to D4

				   if (elementType.InterfaceCount() > 0)
				   {
					   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayPushInterface);
				   }
				   else
				   {
					   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayPushByRef);
				   }
			   }
		   }	
	   }

	   void CompileAsPushToArray(CCompileEnvironment& ce, cr_sex s, cstr instanceName)
	   {
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, instanceName);

		   AssertNotTooFewElements(s, 2);

		   if (elementType.VarType() == VARTYPE_Derivative)
		   {
			   AssertNotTooManyElements(s, 3);
			   CompileAsPushToArrayByRef( ce, s, instanceName);
		   }
		   else if (elementType.VarType() == VARTYPE_Closure)
		   {
			   AssertNotTooManyElements(s, 2);

			   cr_sex value = s.GetElement(1);

			   if (!TryCompileAssignArchetype(ce, value, elementType, false))
			   {
				   sexstringstream<1024> streamer;
				   streamer.sb << ("Could not evaluate the expression as type ") << GetTypeName(elementType.VarType());
				   Throw(value, streamer);
			   } // The value is in D7

			   ce.Builder.AssignVariableRefToTemp(instanceName, 0, 0); // array goes to D4
			   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayPush64);

		   }
		   else
		   {
			   AssertNotTooManyElements(s, 2);
			
			   cr_sex value = s.GetElement(1);

			   if (!TryCompileArithmeticExpression(ce, value, true, elementType.VarType()))
			   {
				   sexstringstream<1024> streamer;
				   streamer.sb << ("Could not evaluate the expression as type ") << GetTypeName(elementType.VarType());
				   Throw(value, streamer);
			   } // The value is in D7
		
			   ce.Builder.AssignVariableRefToTemp(instanceName, 0, 0); // array goes to D4

			   switch(SizeOfElement(elementType))
			   {
			   case 4:
				   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayPush32);
				   break;
			   case 8:
				   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayPush64);
				   break;
			   default:
				   Throw(s, ("Internal compiler error compiling Array.Push. Unhandled element size"));
				   break;
			   }
		   }
	   }

	   void CompileAsPopFromArray(CCompileEnvironment& ce, cr_sex s, cstr instanceName)
	   {
		   AssertNotTooFewElements(s, 1);
		   AssertNotTooManyElements(s, 1);
	
		   ce.Builder.AssignVariableRefToTemp(instanceName, Rococo::ROOT_TEMPDEPTH, 0); // array goes to D7

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);
		   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayPop);
	   }

	   void CompileAsClearArray(CCompileEnvironment& ce, cr_sex s, cstr instanceName)
	   {
		   AssertNotTooFewElements(s, 1);
		   AssertNotTooManyElements(s, 1);

		   ce.Builder.AssignVariableRefToTemp(instanceName, Rococo::ROOT_TEMPDEPTH, 0); // array goes to D7

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);
		   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayClear);
	   }

	   void CompileAsPopOutFromArray(CCompileEnvironment& ce, cr_sex s, cstr instanceName, VARTYPE requiredType)
	   {
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, instanceName);
		   if (elementType.VarType() != requiredType)
		   {
			   sexstringstream<1024> streamer;
			   streamer.sb << ("The array pops out type ") << GetTypeName(elementType.VarType()) << (", but the expression required type ") << requiredType;
			   Throw(s, streamer);
		   }

		   ce.Builder.AssignVariableRefToTemp(instanceName, 0, 0); // array goes to D4

		   switch(GetBitCount(requiredType))
		   {
		   case BITCOUNT_32:
			   ce.Builder.Assembler().Append_Invoke( GetArrayCallbacks(ce).ArrayPopOut32);
			   break;
		   case BITCOUNT_64:
			   ce.Builder.Assembler().Append_Invoke( GetArrayCallbacks(ce).ArrayPopOut64);
			   break;
		   default:
			   Throw(s, ("Derived Array.PopOut are not implemented"));
		   }

		   // The front of the queue was popped into D7
	   }

	   void CompileAsPopOutFromArrayToVariable(CCompileEnvironment& ce, cr_sex s, cstr instanceName)
	   {
		   AssertNotTooFewElements(s, 3);
		   AssertNotTooManyElements(s, 3);

		   cr_sex mapsTo = GetAtomicArg(s,1);
		   cr_sex target = GetAtomicArg(s,2);

		   cstr targetToken = target.String()->Buffer;

		   if (!AreEqual(mapsTo.String(), ("->")))
		   {
			   Throw(mapsTo, ("Expecting mapping token '->'"));
		   }

		   VARTYPE requiredType = ce.Builder.GetVarType(targetToken);
		   if (requiredType == VARTYPE_Bad)
		   {
			   ThrowTokenNotFound(s, target.String()->Buffer, ce.Builder.Owner().Name(), ("variable"));
		   }

		   CompileAsPopOutFromArray(ce, s, instanceName, requiredType);

		   // The front of the queue was popped into D7
		   ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetToken);
	   }

	   void CompileArraySet(CCompileEnvironment& ce, cr_sex s)
	   {
		   AssertNotTooFewElements(s, 3);
		   AssertNotTooFewElements(s, 3);

		   cstr instance = s.GetElement(0).String()->Buffer;
		   cr_sex index = s.GetElement(1);
		   if (!TryCompileArithmeticExpression(ce, index, true, VARTYPE_Int32))
		   {
			   Throw(index, ("Could not evaluate the expression as index type Int32"));
		   }

		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH, 0, BITCOUNT_32); // save the value to D7 for popping to D4
		
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, instance);

		   cr_sex value = s.GetElement(2);

		   VARTYPE elementVarType = elementType.VarType();
		
		   if (IsPrimitiveType(elementVarType))		
		   {
			   if (!TryCompileArithmeticExpression(ce, value, true, elementVarType))
			   {
				   sexstringstream<1024> streamer;
				   streamer.sb << ("Could not evaluate the expression as type ") << GetTypeName(elementVarType);
				   Throw(value, streamer);
			   } // The value is in D7
		   }
		   else
		   {
			   if (!IsAtomic(value))
			   {
				   Throw(value, ("Expecting variable reference"));
			   }

			   MemberDef def;
			   AssertGetVariable(def, value.String()->Buffer, ce, value);
			   ce.Builder.AssignVariableRefToTemp(value.String()->Buffer, Rococo::ROOT_TEMPDEPTH); // The value reference is in D7 
		   }

		   ce.Builder.PopLastVariables(1, true);
		   ce.Builder.AssignVariableRefToTemp(instance, 1, 0); // The array is in D5

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);

		   switch (elementVarType)
		   {
		   case VARTYPE_Int32:
		   case VARTYPE_Float32:
			   ce.Builder.Assembler().Append_Invoke(callbacks.ArraySet32);
			   break;
		   case VARTYPE_Float64:
		   case VARTYPE_Int64:
			   ce.Builder.Assembler().Append_Invoke(callbacks.ArraySet64);
			   break;
		   case VARTYPE_Derivative:
			   if (elementType.InterfaceCount() > 0)
			   {
				   ce.Builder.Assembler().Append_Invoke(sizeof(size_t) == 8 ? callbacks.ArraySet64 : callbacks.ArraySet32);
			   }
			   else
			   {
				   ce.Builder.Assembler().Append_Invoke(callbacks.ArraySetByRef);
			   }
			   break;
		   default:
			   Throw(value, ("Bad type"));
		   }
	   }

	   bool TryCompileAsArrayCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName)
	   {
 		   if (AreEqual(methodName, ("Push")))
		   {
			   CompileAsPushToArray(ce, s, instanceName);
			   return true;
		   }
		   else if (AreEqual(methodName, ("Pop")))
		   {
			   CompileAsPopFromArray(ce, s, instanceName);
			   return true;
		   }
		   else if (AreEqual(methodName, ("PopOut")))
		   {
			   CompileAsPopOutFromArrayToVariable(ce, s, instanceName);
			   return true;
		   }
		   else if (AreEqual(methodName, ("Clear")))
		   {
			   CompileAsClearArray(ce, s, instanceName);
			   return true;
		   }
		   return false;
	   }

	   void ValidateElementType(CCompileEnvironment& ce, cr_sex s, cstr instanceName, VARTYPE type, const IStructure* structType)
	   {
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, instanceName);

		   if (elementType.VarType() != type)
		   {
			   Throw(s, ("The array element type does not match the type of the variable in the assignment"));
		   }

		   if (type == VARTYPE_Derivative && *structType != elementType)
		   {
			   Throw(s, ("The array element type does not match the type of the variable in the assignment"));
		   }
	   }

	   void CompileGetArrayElement(CCompileEnvironment& ce, cr_sex s, cstr instanceName, VARTYPE varType, const IStructure* structType)
	   {
		   ValidateElementType(ce, s, instanceName, varType, structType);

		   if (!TryCompileArithmeticExpression(ce, s, true, VARTYPE_Int32))
		   {
			   Throw(s, ("Expected expression to evaluate to type Int32 to serve as index to array"));
		   } // D7 now contains the array index

		
		   ce.Builder.AssignVariableRefToTemp(instanceName, 0, 0); // array goes to D4

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);

		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, instanceName);
		   switch(SizeOfElement(elementType))
		   {
		   case 4:
			   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayGet32);
			   break;
		   case 8:
			   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayGet64);
			   break;
		   default:
			   Throw(s, ("Array.Get only support 32-bit and 64-bit types"));
			   break;
		   }
	   }

	   void CompileGetArraySubelement(CCompileEnvironment& ce, cr_sex indexExpr, cr_sex subItemName, cstr instanceName, VARTYPE type, const IStructure* structType)
	   {
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, indexExpr, instanceName);

		   int offset = 0;
		   const IMember* member = FindMember(elementType, subItemName.String()->Buffer, OUT offset);
		   if (member == NULL)
		   {
			   ThrowTokenNotFound(subItemName, subItemName.String()->Buffer, elementType.Name(), ("member"));
		   }

		   const IStructure& memberType = *member->UnderlyingType();

		   if (memberType.VarType() != type || (type == VARTYPE_Derivative && memberType != *structType))
		   {
			   sexstringstream<1024> streamer;
			   streamer.sb << ("The array element type ") << elementType.Name() << (" does not match the type required: ");
			   if (structType == NULL) streamer.sb << GetTypeName(type);
			   else streamer.sb << structType->Name();
			   Throw(subItemName, streamer);
		   }

		   if (!TryCompileArithmeticExpression(ce, indexExpr, true, VARTYPE_Int32)) 
		   {
			   Throw(indexExpr, ("Expected expression to evaluate to type Int32 to serve as index to array"));
		   } // D7 now contains the array index

		   ce.Builder.AssignVariableRefToTemp(instanceName, 0, 0); // array goes to D4

		   VariantValue v;
		   v.int32Value = offset;
		   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, v, BITCOUNT_32); // offset goes to D5

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);

		   switch(SizeOfElement(memberType))
		   {
		   case 4:
			   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayGetMember32);
			   break;
		   case 8:
			   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayGetMember64);
			   break;
		   default:
			   Throw(indexExpr, ("Array.GetSubElement only support 32-bit and 64-bit types"));
			   break;
		   }
	   }

	   void CompileValidateIndexPositive(CCompileEnvironment& ce, cr_sex s, int tempDepth)
	   {
		   ce.Builder.AddSymbol(("ValidateIndexPositive..."));
		   ce.Builder.Assembler().Append_MoveRegister(tempDepth + VM::REGISTER_D4, VM::REGISTER_D4, BITCOUNT_32); 
		   ce.Builder.Assembler().Append_Test(VM::REGISTER_D4, BITCOUNT_32);

		   size_t branchPos = ce.Builder.Assembler().WritePosition();
		   ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_GREATER_OR_EQUAL, 6);

		   const IFunction& fnThrow = GetFunctionByFQN(ce, s, ("Sys.ThrowIndexNegative"));
		   CodeSection section;
		   fnThrow.Code().GetCodeSection(section);
		
		   ce.Builder.Assembler().Append_CallById(section.Id);
		   ce.Builder.AddSymbol(("...ValidateIndexPositive"));
		   ce.Builder.Assembler().Append_NoOperation();

		   size_t skipDelta = ce.Builder.Assembler().WritePosition() - branchPos;
		   ce.Builder.Assembler().SetWriteModeToOverwrite(branchPos);
		   ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_GREATER_OR_EQUAL, (int32) skipDelta);
		   ce.Builder.Assembler().SetWriteModeToAppend();
	   }

	   void CompileValidateIndexLowerThanArrayElementCount(CCompileEnvironment& ce, cr_sex s, int indexTempDepth, cstr arrayName)
	   {
		   TokenBuffer arrayLenName;
		   StringPrint(arrayLenName, ("%s._length"), arrayName);

		   ce.Builder.AssignVariableToTemp(arrayLenName, 2, 0);

		   ce.Builder.AddSymbol(("ValidateIndexLowerThanArrayElementCount..."));
		   ce.Builder.Assembler().Append_IntSubtract(VM::REGISTER_D6, BITCOUNT_32, VM::REGISTER_D4 + indexTempDepth);
		   ce.Builder.Assembler().Append_Test(VM::REGISTER_D5, BITCOUNT_32);

		   size_t branchPos = ce.Builder.Assembler().WritePosition();
		   ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_GREATER_THAN, 6);

		   const IFunction& fnThrow = GetFunctionByFQN(ce, s, ("Sys.ThrowIndexExceededBounds"));
		   CodeSection section;
		   fnThrow.Code().GetCodeSection(section);
		
		   ce.Builder.Assembler().Append_CallById(section.Id);
		   ce.Builder.AddSymbol(("ValidateIndexLowerThanArrayElementCount..."));
		   ce.Builder.Assembler().Append_NoOperation();

		   size_t skipDelta = ce.Builder.Assembler().WritePosition() - branchPos;		
		   ce.Builder.Assembler().SetWriteModeToOverwrite(branchPos);
		   ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_GREATER_THAN, (int32) skipDelta);
		   ce.Builder.Assembler().SetWriteModeToAppend();
	   }

	   bool TryParseAsIndexPositiveLiteralInt32(OUT int32& value, cr_sex s)
	   {
		   value = 0;
		   if (IsAtomic(s))
		   {
			   if (Parse::PARSERESULT_GOOD == Parse::TryParseDecimal(OUT value, s.String()->Buffer))
			   {
				   if (value < 0) Throw(s, ("Index must not be negative"));
				   return true;
			   }
		   }

		   return false;
	   }

	   int AddVariableArrayLock(CCompileEnvironment& ce, int arrayTempDepth)
	   {
		   TokenBuffer lockName;
		   StringPrint(lockName, ("_arrayLock_%d"), ce.SS.NextID());

		   const IStructure& lockStruct = *ce.Object.Common().SysNative().FindStructure(("_Lock"));
		
		   ce.Builder.AddSymbol(lockName);
		   AddVariable(ce, NameString::From(lockName), lockStruct);

		   TokenBuffer lockSource;
		   StringPrint(lockSource, ("%s._lockSource"), (cstr) lockName);
		   ce.Builder.AssignTempToVariable(arrayTempDepth, lockSource);

		   TokenBuffer lockMemberOffset;
		   StringPrint(lockMemberOffset, ("%s._lockMemberOffset"), (cstr) lockName);

		   MemberDef def;
		   ce.Builder.TryGetVariableByName(def, lockMemberOffset);

		   int offset;
		   const IMember* member = FindMember(ce.StructArray(), ("_lock"), OUT offset);

		   VariantValue lockMemberOffsetValue;
		   lockMemberOffsetValue.int32Value = offset;
		   ce.Builder.Assembler().Append_SetStackFrameImmediate(def.SFOffset + def.MemberOffset, lockMemberOffsetValue, BITCOUNT_32);

		   return lockStruct.SizeOfStruct();
	   }

	   void CompileLockRef(CCompileEnvironment& ce, cr_sex s, int hashIndex)
	   {	
		   // (foreach v # (a <index> ) (action1) ... (actionN) )

		   if (hashIndex != 2)
		   {
			   Throw(s.GetElement(2), ("Expecting #"));
		   }

		   cr_sex elementSpecifier = s.GetElement(hashIndex + 1);
		   AssertCompound(elementSpecifier);
		   AssertNotTooFewElements(elementSpecifier, 2);
		   AssertNotTooManyElements(elementSpecifier, 2);

		   cr_sex collection = GetAtomicArg(elementSpecifier, 0);
		   cstr collectionName = collection.String()->Buffer;
		   AssertLocalVariableOrMember(collection);			

		   cr_sex refExpr =  GetAtomicArg(s, 1);
		   AssertLocalIdentifier(refExpr);
		   cstr refName = refExpr.String()->Buffer;
		
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, collectionName);

		   AddVariableRef(ce, NameString::From(refName), elementType);

		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 6, Rococo::ROOT_TEMPDEPTH + 6, BITCOUNT_POINTER);

		   ce.Builder.AddSymbol(collectionName);
		   ce.Builder.AssignVariableRefToTemp(collectionName, 9, 0); // Array ref is now in D13
				
		   ce.Builder.AddSymbol(("(foreach...")); 

		   cr_sex indexExpr = elementSpecifier.GetElement(1);
		
		   int32 indexValue;
		   if (TryParseAsIndexPositiveLiteralInt32(OUT indexValue, indexExpr))
		   {
			   VariantValue v;
			   v.int32Value = indexValue;
			   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D12, v, BITCOUNT_32);
		   }
		   else
		   {
			   if (!TryCompileArithmeticExpression(ce, indexExpr, true, VARTYPE_Int32)) Throw(indexExpr, ("Failed to parse expression as (Int32 startIndex)"));
			   CompileValidateIndexPositive(ce, collection, Rococo::ROOT_TEMPDEPTH); // index now validated to be positive
			   ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D12, BITCOUNT_32); // D12 contains the working index throughout the entire iteration
		   }
				
		   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayLock); // Prevent popping of the array during enumeration

		   int lockSize = AddVariableArrayLock(ce, 9);

		   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayGetRefUnchecked); // returns pointer to element in D7, D12 is element index and D11 is array ref
				
		   MemberDef refDef;
		   ce.Builder.TryGetVariableByName(OUT refDef, refName);
		   ce.Builder.Assembler().Append_SetStackFrameValue(refDef.SFOffset, VM::REGISTER_D7, BITCOUNT_POINTER);

		   CompileExpressionSequence(ce, 4, s.NumberOfElements()-1, s);				
	
		   ce.Builder.AddSymbol(("...foreach)"));
		//   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayUnlock); // Enable popping of the array after enumeration has finished
	   }

		// (foreach i v # <collection-name> (...) (...) )
		// (foreach i v # (<collection-name> <lower-bound> <upper-bound>) (...) (...) )
		// (foreach v # (<collection-name> <lower-bound> <upper-bound>) (...) (...) )
	   cstr GetCollectionNameFromForeachExpression(cr_sex scollection)
	   {
		   if (IsCompound(scollection))
		   {
			   AssertNotTooFewElements(scollection, 3);
			   AssertNotTooManyElements(scollection, 3);

			   cr_sex collectionNameExpr = GetAtomicArg(scollection, 0);
			   cstr collectionName = collectionNameExpr.String()->Buffer;
			   AssertLocalVariableOrMember(collectionNameExpr);
			   return collectionName;
		   }
		   else
		   {
			   cstr collectionName = scollection.String()->Buffer;
			   AssertLocalVariableOrMember(scollection);
			   return collectionName;
		   }
	   }

	   void CompileEnumerateArray(CCompileEnvironment& ce, cr_sex s, int hashIndex)
	   {	
		   // (foreach i v # a (...) (...) )
		   // (foreach v # a (...) (...) )
		   // (foreach i v # (a <lower-bound> <upper-bound>) (...) (...) )
		   // (foreach v # (a <lower-bound> <upper-bound>) (...) (...) )

		   cr_sex collection = s[hashIndex + 1];
		   cstr collectionName = GetCollectionNameFromForeachExpression(collection);

		   cstr indexName;
		   cstr refName;

		   if (hashIndex == 2)
		   {
			   // (foreach v # a (...) (...) ) or
			   // (foreach v # (a <lower-bound> <upper-bound>) (...) (...) )
			   indexName = nullptr;
			   cr_sex refExpr = s[1];
			   AssertLocalIdentifier(refExpr);
			   refName = refExpr.String()->Buffer;
		   }
		   else
		   {
			   // (foreach i v # a (...) (...) )
		       // (foreach i v # (a <lower-bound> <upper-bound>) (...) (...) )
			   cr_sex indexExpr = s[1];
			   indexName = indexExpr.String()->Buffer;
			   AssertLocalIdentifier(indexExpr);

			   cr_sex refExpr = s[2];
			   AssertLocalIdentifier(refExpr);
			   refName = refExpr.String()->Buffer;			
			   AddVariable(ce, NameString::From(indexName), ce.Object.Common().TypeInt32());
		   }

		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, collectionName);

		   if (elementType.InterfaceCount() == 0)
		   {
			   AddVariableRef(ce, NameString::From(refName), elementType);
		   }
		   else
		   {
			   AddVariable(ce, NameString::From(refName), elementType);
			   VariantValue nullVal;
			   nullVal.vPtrValue = (uint8*) elementType.GetInterface(0).UniversalNullInstance()->pVTables;
			   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, nullVal, BITCOUNT_POINTER);
			   ce.Builder.AssignTempToVariable(0, refName);
		   }

		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 6, Rococo::ROOT_TEMPDEPTH + 6, BITCOUNT_POINTER);

		   ce.Builder.AddSymbol(collectionName);
		   ce.Builder.AssignVariableRefToTemp(collectionName, 9, 0); // Array ref is now in D13
				
		   ce.Builder.AddSymbol("D12 - working index");
		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 5, Rococo::ROOT_TEMPDEPTH + 5, BITCOUNT_POINTER);

		   ce.Builder.AddSymbol("(foreach..."); 

		   if (IsCompound(collection))
		   {
			   cr_sex startIndexExpr = collection.GetElement(1);
		
			   int32 startIndexValue;
			   if (TryParseAsIndexPositiveLiteralInt32(OUT startIndexValue, startIndexExpr))
			   {
				   VariantValue v;
				   v.int32Value = startIndexValue;
				   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D12, v, BITCOUNT_32);
			   }
			   else
			   {
				   if (!TryCompileArithmeticExpression(ce, startIndexExpr, true, VARTYPE_Int32)) Throw(startIndexExpr, ("Failed to parse expression as (Int32 startIndex)"));
				   CompileValidateIndexPositive(ce, collection, Rococo::ROOT_TEMPDEPTH); // index now validated to be positive
				   ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D12, BITCOUNT_32); // D12 contains the working index throughout the entire iteration
			   }
		   }
		   else
		   {
			   VariantValue zero;
			   zero.int32Value = 0;
			   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D12, zero, BITCOUNT_32);
		   }
		
		   // We may be nested in a function that overwrites D11, so save it
		   ce.Builder.AddSymbol("D11 - final index");
		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 4, Rococo::ROOT_TEMPDEPTH + 4, BITCOUNT_POINTER);

		   if (IsCompound(collection))
		   {
			   cr_sex endIndexExpr = collection[2];

			   int32 endIndexValue;
			   if (TryParseAsIndexPositiveLiteralInt32(OUT endIndexValue, endIndexExpr))
			   {
				   VariantValue v;
				   v.int32Value = endIndexValue;
				   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D11, v, BITCOUNT_32);
			   }
			   else
			   {
				   if (!TryCompileArithmeticExpression(ce, endIndexExpr, true, VARTYPE_Int32)) Throw(endIndexExpr, "Failed to parse expression as (Int32 endIndex)");
				   ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D11, BITCOUNT_32); // D11 contains the final index throughout the entire iteration
			   }		

			   CompileValidateIndexLowerThanArrayElementCount(ce, collection, 7, collectionName);
		   }
		   else
		   {
			   TokenBuffer arrayLengthName;
			   StringPrint(arrayLengthName, "%s._length", collectionName);
			   ce.Builder.AssignVariableToTemp(arrayLengthName, 7, 0);

			   VariantValue minusOne;
			   minusOne.int32Value = -1;
			   ce.Builder.Assembler().Append_AddImmediate(VM::REGISTER_D11, BITCOUNT_32, VM::REGISTER_D11, minusOne);
		   }
		
		   // We may be nested in a function that overwrites D10, which is used as the result of D10-11
		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 3, Rococo::ROOT_TEMPDEPTH + 3, BITCOUNT_POINTER);
		
		   // Prevent popping of the array during enumeration, since this may corrupt our references
		   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayLock);

		   int lockSize = AddVariableArrayLock(ce, 9);

		   struct ConditionSection: public ICompileSection
		   {
			   ConditionSection(CCompileEnvironment& _ce): ce(_ce) {}

			   CCompileEnvironment& ce;

			   void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
			   {
				   ce.Builder.AddSymbol(("while (endIndex > currentIndex)"));
				   ce.Builder.Assembler().Append_IntSubtract(VM::REGISTER_D11, BITCOUNT_32, VM::REGISTER_D12);
				   builder.Assembler().Append_Test(VM::REGISTER_D10, BITCOUNT_32);				
			   }
		   } loopCriterion(ce);

		   struct BodySection: public ICompileSection
		   {			
			   BodySection(cr_sex _s, CCompileEnvironment& _ce, cstr _indexName, cstr _refName, int _firstBodyIndex, int _lastBodyIndex):
				   s(_s), ce(_ce), indexName(_indexName), refName(_refName), firstBodyIndex(_firstBodyIndex), lastBodyIndex(_lastBodyIndex) {}

			   cr_sex s;
			   CCompileEnvironment& ce;
			   cstr indexName;
			   cstr refName;
			   int firstBodyIndex;
			   int lastBodyIndex;

			   void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
			   {
				   ce.Builder.AddSymbol("while..{ ");
				   builder.PushControlFlowPoint(*controlFlowData);
		  
				   MemberDef refDef;
				   ce.Builder.TryGetVariableByName(OUT refDef, refName);

				   if (refDef.ResolvedType->InterfaceCount() == 0)
				   {
					   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayGetRefUnchecked); // returns pointer to element in D7, D12 is element index and D11 is array ref
				   }
				   else
				   {
					   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayGetInterfaceUnchecked);
				   }

				   ce.Builder.Assembler().Append_SetStackFrameValue(refDef.SFOffset, VM::REGISTER_D7, BITCOUNT_POINTER);

				   if (indexName != NULL) ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH + 5, indexName); // index is now = running index
				   VariantValue one;
				   one.int32Value = 1;
				   ce.Builder.Assembler().Append_AddImmediate(VM::REGISTER_D12, BITCOUNT_32, VM::REGISTER_D12, one); // increment the running index

				   CompileExpressionSequence(ce, firstBodyIndex, lastBodyIndex, s);				

				   builder.PopControlFlowPoint();
				   ce.Builder.AddSymbol("...while }");
			   }
		   } bodySection(s, ce, indexName, refName, hashIndex + 2, s.NumberOfElements()-1);

		   ce.Builder.AppendWhileDo(loopCriterion, CONDITION_IF_GREATER_OR_EQUAL, bodySection);
		
		//   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayUnlock); // Enable popping of the array after enumeration has finished
		   ce.Builder.AddSymbol("...foreach)");
		   ce.Builder.Assembler().Append_NoOperation();
	   }

	   void CompileArrayDestruct(CCompileEnvironment& ce, const IStructure& s, cstr instanceName)
	   {
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, *(const ISExpression*) s.Definition(), instanceName);
		   if (RequiresDestruction(elementType))
		   {
			   ce.Builder.AssignVariableRefToTemp(instanceName, Rococo::ROOT_TEMPDEPTH);
			   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayDestructElements);
		   }

		   ce.Builder.AssignVariableRefToTemp(instanceName, Rococo::ROOT_TEMPDEPTH);
		   AppendInvoke(ce, GetArrayCallbacks(ce).ArrayDelete, *(const ISExpression*) s.Definition());
	   }

	   void CompileNumericExpression(CCompileEnvironment& ce, cr_sex valueExpr, VARTYPE type)
	   {
		   if (IsCompound(valueExpr))
		   {			
			   if (TryCompileFunctionCallAndReturnValue(ce, valueExpr, type, NULL, NULL))
			   {
				   return;
			   }

			   if (valueExpr.NumberOfElements() == 1 && IsCompound(valueExpr))
			   {
				   CompileNumericExpression(ce, valueExpr.GetElement(0), type);
			   }

			   return;
		   }
		   else if (IsAtomic(valueExpr))
		   {
			   cstr svalue = valueExpr.String()->Buffer;
			   if (svalue[0] == '-' ||  isdigit(svalue[0]))
			   {
				   VariantValue value;
				   if (Parse::TryParse(OUT value, type, svalue) == Parse::PARSERESULT_GOOD)
				   {
					   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, value, GetBitCount(type));
					   return;
				   }
			   }
			   else
			   {
				   VARTYPE atomicType = GetAtomicValueAnyNumeric(ce, valueExpr, valueExpr.String()->Buffer, Rococo::ROOT_TEMPDEPTH);
				   if (atomicType == type) return;
			   }
		   }

		   sexstringstream<1024> streamer;
		   streamer.sb <<  ("Expecting an expression that evaluates to ") << GetTypeName(type);
		   Throw(valueExpr, streamer);						
	   }

	   // Example: (array Int32 a 4)
	   void CompileArrayDeclaration(CCompileEnvironment& ce, cr_sex s)
	   {
		   AssertNotTooFewElements(s, 4);
		   AssertNotTooManyElements(s, 4);

		   cr_sex typeName = GetAtomicArg(s, 1);
		   cr_sex arrayName = GetAtomicArg(s, 2);

		   cstr arrayNameTxt = arrayName.String()->Buffer;

		   AssertLocalIdentifier(arrayName);

		   cr_sex capacity = s.GetElement(3);

		   // (array <element-type-name> <array-name> <capacity>
		   const IStructure& arrayStruct = ce.StructArray();

		   const IStructure* elementStruct = MatchStructure(typeName, ce.Builder.Module());
		   if (elementStruct == NULL) ThrowTokenNotFound(s, typeName.String()->Buffer, ce.Builder.Module().Name(), ("type"));

		   if (elementStruct->InterfaceCount() != 0 && !IsNullType(*elementStruct))
		   {
			   Throw(s[3], "Arrays cannot have class type elements. Use an interface to %s instead.", elementStruct->Name());
		   }

		   AddVariable(ce, NameString::From(arrayNameTxt), ce.StructArray());

		   CompileNumericExpression(ce, capacity, VARTYPE_Int32); // capacity to D7
		   ce.Builder.AssignVariableRefToTemp(arrayNameTxt, 1); // Array goes to D5

		   VariantValue v;
		   v.vPtrValue = (void*) elementStruct;
		   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_POINTER); // Element type to D4
				
		   AddArrayDef(ce.Script, ce.Builder, arrayNameTxt, *elementStruct, s);

		   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayInit);
	   }
   }//Script
}//Sexy