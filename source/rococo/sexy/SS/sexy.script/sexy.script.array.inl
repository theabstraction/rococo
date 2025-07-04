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

#include <cerrno>

namespace Rococo::Compiler::Impl
{
	int CountRefCountedMembers(const IStructure& member);
}

namespace Rococo
{
   namespace Script
   {
	   void DeleteMembers(IScriptSystem& ss, const IStructure& type, uint8* pInstance);

	   size_t SizeOfElement(const IStructure& type)
	   {
		   return type.InterfaceCount() > 0 ? sizeof(size_t) : type.SizeOfStruct();
	   }


	   ArrayImage* CreateArrayImage(IScriptSystem& ss, const IStructure& elementType, int32 capacity)
	   {
		   if (capacity < 0)
		   {
			   Throw(0, "Could not allocate array. Negative element count: %d", capacity);
		   }

		   int elementSize = elementType.InterfaceCount() > 0 ? sizeof(InterfacePointer) : elementType.SizeOfStruct();

		   if (elementSize > 0x7FFFFFFFLL)
		   {
			   Throw(0, "Could not allocate array. Element size was > 2GB");
		   }

		   size_t szCapacity = elementSize * (size_t)capacity;
		   if (szCapacity > 0x7FFFFFFFLL)
		   {
			   Throw(0, "Could not allocate array. The maximum size is 2GB");
		   }

		   ArrayImage* a = new ArrayImage();

		   a->ElementCapacity = capacity;
		   a->ElementLength = (int32)elementSize;
		   a->ElementType = &elementType;
		   a->NumberOfElements = 0;
		   a->LockNumber = 0;

		   try
		   {
			   a->Start = capacity == 0 ? nullptr : ss.AlignedMalloc(16, capacity * a->ElementLength);
		   }
		   catch (...)
		   {
			   delete a;
			   Throw(0, "Could not allocate array with %d elements and %d bytes per element. ", capacity, a->ElementLength);
		   }

		   a->RefCount = 1;

		   return a;
	   }

	   VM_CALLBACK(ArrayInit)
	   {
		   IScriptSystem& ss = *(IScriptSystem*)context;
		   const IStructure* elementType = (const IStructure*)registers[VM::REGISTER_D4].vPtrValue;
		   int32 capacity = registers[VM::REGISTER_D7].int32Value;

		   if (capacity == 0)
		   {
			   // array.Capacity returns 0 in the case of a null array, so prohibit zero and 
			   // thus the Capacity method can then be used to tell if an array is non-null.
			   Throw(0, "Could not allocate array. Zero capacity");
		   }

		   ArrayImage* a = CreateArrayImage(ss, *elementType, capacity);

		   registers[VM::REGISTER_D5].vPtrValue = a;
	   }


	   void ArrayDelete(ArrayImage* a, IScriptSystem& ss)
	   {
		   const IStructure& elementType = *a->ElementType;

		   if (RequiresDestruction(elementType))
		   {
			   DestroyElements(*a, ss);
		   }

			// Delete the elements
			ss.AlignedFree(a->Start);

			// Now delete the structure
			delete a;
	   }

	   void ReleaseArray(ArrayImage* a, IScriptSystem& ss)
	   {
		   if (a)
		   {
			   a->RefCount--;
			   if (a->RefCount == 0)
			   {
				   ArrayDelete(a, ss);
			   }
		   }
	   }

	   VM_CALLBACK(ArrayReleaseRef)
	   {
		   IScriptSystem& ss = *(IScriptSystem*) context;
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D7].vPtrValue;
		   ReleaseArray(a, ss);
	   }

	   struct ArrayLifetimeManager : Rococo::Compiler::IMemberLifeSupervisor
	   {
		   IScriptSystem& ss;

		   void Release(uint8* instance) override
		   {
			   ArrayImage* a = *reinterpret_cast<ArrayImage**>(instance);
			   ReleaseArray(a, ss);
		   }

		   ArrayLifetimeManager(IScriptSystem& _ss) : ss(_ss)
		   {

		   }

		   void Free() override
		   {
			   delete this;
		   }
	   };

	   IMemberLifeSupervisor* CreateArrayLifetimeManager(IScriptSystem& ss)
	   {
		   return new ArrayLifetimeManager(ss);
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
		   else if (a.ElementType->VarType() == SexyVarType_Derivative)
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

	   void ArrayDoubleCapacity(ArrayImage& a, IScriptSystem& ss)
	   {
		   int64 currentLengthInBytes = (int64) a.ElementCapacity * (int64) a.ElementLength;
		   if (currentLengthInBytes >= (int64) 1024_megabytes)
		   {
			   ss.ThrowFromNativeCodeF(0, "ArrayDoubleCapacity failed: the array was at maximum capacity");
			   return;
		   }

		   try
		   {
			   auto* newBuffer = (char*)ss.AlignedMalloc(16, (int32) currentLengthInBytes * 2);
			   if (newBuffer == nullptr) throw 0;

			   memcpy(newBuffer, a.Start, currentLengthInBytes);
			   memset(newBuffer + currentLengthInBytes, 0, currentLengthInBytes);

			   ss.AlignedFree(a.Start);
			   a.Start = newBuffer;
		   }
		   catch (...)
		   {
			   ss.ThrowFromNativeCodeF(0, "ArrayDoubleCapacity failed: out of heap memory");
			   return;
		   }

		   a.ElementCapacity *= 2;
	   }

	   VM_CALLBACK(ArrayPushAndGetRef)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D7].vPtrValue;

		   if (a == nullptr)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.PushAndGetRef failed: null array");
			   return;
		   }

		   if (a->LockNumber > 0)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.PushAndGetRef failed: the array was locked for enumeration");
			   return;
		   }

		   if (a->NumberOfElements >= a->ElementCapacity)
		   {
			   ArrayDoubleCapacity(*a, *(IScriptSystem*)context);
		   }

		   uint8* pElement = ((uint8*)a->Start) + a->NumberOfElements * a->ElementLength;
		   memset(pElement, 0, a->ElementLength);
		   a->NumberOfElements++;
		   registers[VM::REGISTER_D8].vPtrValue = pElement;
	   }

	   VM_CALLBACK(ArrayPush32)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D4].vPtrValue;
		   int32 value = registers[VM::REGISTER_D7].int32Value;

		   if (a == nullptr)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayPush32 failed: null array");
			   return;
		   }

		   if (a->LockNumber > 0)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayPush32 failed: the array was locked for enumeration");
			   return;
		   }

		   if (a->NumberOfElements >= a->ElementCapacity)
		   {
			   ArrayDoubleCapacity(*a, *(IScriptSystem*)context);
		   }

		   int32* intBuffer = (int32*)a->Start;
		   intBuffer[a->NumberOfElements++] = value;
	   }

	   VM_CALLBACK(ArrayPushInterface)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D4].vPtrValue;
		   InterfacePointer pObject = (InterfacePointer)registers[VM::REGISTER_D7].vPtrValue;

		   IScriptSystem& ss = *(IScriptSystem*)context;

		   if (a == nullptr)
		   {
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayPushInterface failed: null array");
			   return;
		   }

		   if (a->LockNumber > 0)
		   {
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayPushInterface failed: the array was locked for enumeration");
			   return;
		   }

		   if (a->NumberOfElements >= a->ElementCapacity)
		   {
			   ArrayDoubleCapacity(*a, *(IScriptSystem*)context);
		   }

		   InterfacePointer* intBuffer = (InterfacePointer*)a->Start;
		   intBuffer[a->NumberOfElements++] = pObject;
		   ss.ProgramObject().IncrementRefCount(pObject);
	   }

	   VM_CALLBACK(ArrayPush64)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D4].vPtrValue;
		   int64 value = registers[VM::REGISTER_D7].int64Value;

		   if (a == nullptr)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayPush64 failed: null array");
			   return;
		   }

		   if (a->LockNumber > 0)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayPush64 failed: the array was locked for enumeration");
			   return;
		   }

		   if (a->NumberOfElements >= a->ElementCapacity)
		   {
			   ArrayDoubleCapacity(*a, *(IScriptSystem*)context);
		   }

		   int64* intBuffer = (int64*)a->Start;
		   intBuffer[a->NumberOfElements++] = value;
	   }

	   VM_CALLBACK(ArrayPushByRef)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D4].vPtrValue;
		   const void* pValue = registers[VM::REGISTER_D7].vPtrValue;

		   if (a == nullptr)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayPushByRef failed: null array");
			   return;
		   }

		   if (a->LockNumber > 0)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayPushByRef failed: the array was locked for enumeration");
			   return;
		   }

		   if (a->NumberOfElements >= a->ElementCapacity)
		   {
			   ArrayDoubleCapacity(*a, *(IScriptSystem*)context);
		   }

		   void* pTargetElement = ((uint8*)a->Start) + (a->ElementLength * a->NumberOfElements);
		   AlignedMemcpy(pTargetElement, pValue, a->ElementLength);
		   a->NumberOfElements++;
	   }

	   void UpdateRefCounts(uint8* pMemberVariable, const IMember& m)
	   {
		   auto& mtype = *m.UnderlyingType();

		   if (m.IsInterfaceVariable())
		   {
			   auto* ip = *(InterfacePointer*)pMemberVariable;
			   ObjectStub* stub = InterfaceToInstance(ip);
			   if (stub->refCount != ObjectStub::NO_REF_COUNT)
			   {
				   stub->refCount++;
			   }
		   }
		   else if (m.UnderlyingGenericArg1Type() != nullptr)
		   {
			   if (Eq(mtype.Name(), "_Array"))
			   {
				   ArrayImage* subArray = *(ArrayImage**)pMemberVariable;
				   if (subArray)
				   {
					   subArray->RefCount++;
				   }
			   }
			   else if (Eq(mtype.Name(), "_Map"))
			   {
				   MapImage* subMap = *(MapImage**)pMemberVariable;
				   IncrementRef(subMap);
			   }
			   else if (Eq(mtype.Name(), "_List"))
			   {
				   ListImage* subList = *(ListImage**)pMemberVariable;
				   if (subList)
				   {
					   subList->refCount++;
				   }
			   }
		   }
		   else if (mtype.VarType() == SexyVarType_Derivative)
		   {
			   size_t offset = 0;

			   for (int i = 0; i < mtype.MemberCount(); ++i)
			   {
				   auto& subMember = mtype.GetMember(i);
				   uint8* pSubMemberVariable = pMemberVariable + offset;
				   UpdateRefCounts(pSubMemberVariable, subMember);
				   offset += subMember.SizeOfMember();
			   }
		   }
	   }

	   VM_CALLBACK(ArrayUpdateRefCounts)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D4].vPtrValue;
		   uint8* pValue = registers[VM::REGISTER_D7].uint8PtrValue;
		   auto& elementType = *a->ElementType;
		 
		   size_t offset = 0;
		   for (int i = 0; i < elementType.MemberCount(); ++i)
		   {
			   auto& m = elementType.GetMember(i); 
			   uint8* pMemberVariable = pValue + offset;
			   UpdateRefCounts(pMemberVariable, m);
			   offset += m.SizeOfMember();
		   }
	   }

	   VM_CALLBACK(ArraySet32)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D5].vPtrValue;

		   if (a == nullptr)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.Set failed: the array reference was null");
			   return;
		   }

		   const int index = registers[VM::REGISTER_D4].int32Value;
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
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.Set failed: the index was out of range");
			   return;
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
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.Set failed: the index was out of range");
			   return;
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
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.Set failed: the index was out of range");
			   return;
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
		   else if (a.ElementType->VarType() == SexyVarType_Derivative)
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
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.PopOut failed: the array was locked for enumeration");
			   return;
		   }
		
		   if (a->NumberOfElements > 0)
		   {
			   a->NumberOfElements--;
			   ReleaseElement(*a, a->NumberOfElements, ss);
		   }
		   else
		   {
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.Pop failed: the array was empty");
			   return;
		   }
	   }

	   VM_CALLBACK(ArrayClear)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D7].vPtrValue;
		   IScriptSystem& ss = *(IScriptSystem*)context;

		   if (IsLocked(*a))
		   {
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.Clear failed: the array was locked for enumeration");
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
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.PopOut failed: the array was locked for enumeration");
			   return;
		   }
		
		   if (a->NumberOfElements > 0)
		   {
			   a->NumberOfElements--;
			   registers[VM::REGISTER_D7].int32Value = ((const int32*) a->Start)[a->NumberOfElements];
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.PopOut failed: the array was empty");
			   return;
		   }
	   }

	   VM_CALLBACK(ArrayPopOut64)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;

		   if (IsLocked(*a))
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.PopOut failed: the array was locked for enumeration");
			   return;
		   }
		
		   if (a->NumberOfElements > 0)
		   {
			   a->NumberOfElements--;
			   registers[VM::REGISTER_D7].int64Value = ((const int64*) a->Start)[a->NumberOfElements];
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCodeF(0, "Array.PopOut failed: the array was empty");
			   return;
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
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.Get failed: the index was out of range");
			   return;
		   }
	   }

	   VM_CALLBACK(ArrayGet64)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   int32 index = registers[VM::REGISTER_D7].int32Value;

		   if (a == nullptr)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayGet64 failed: the array was null");
			   return;
		   }

		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   const int64* p = ((const int64*) a->Start) + index;
			   registers[VM::REGISTER_D7].int64Value = *p;
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.Get failed: the index was out of range");
			   return;
		   }
	   }

	   VM_CALLBACK(ArrayGetMember32)
	   {
		   int32 offset = registers[VM::REGISTER_D5].int32Value;
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   int32 index = registers[VM::REGISTER_D7].int32Value;

		   if (a == nullptr)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayGetMember32 failed: the array was null");
			   return;
		   }
		
		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   const uint8* pElement = ((const uint8*) a->Start) + index * a->ElementLength;
			   const int32* pMember = (const int32*) (pElement + offset);
			   registers[VM::REGISTER_D7].int32Value = *pMember;
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.Get failed: the index was out of range");
			   return;
		   }
	   }

	   VM_CALLBACK(ArrayGetMember64)
	   {
		   int32 offset = registers[VM::REGISTER_D5].int32Value;
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   int32 index = registers[VM::REGISTER_D7].int32Value;

		   if (a == nullptr)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayGetMember64 failed: the array was null");
			   return;
		   }

		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   const uint8* pElement = ((const uint8*) a->Start) + index * a->ElementLength;
			   const int64* pMember = (const int64*) (pElement + offset);
			   registers[VM::REGISTER_D7].int64Value = *pMember;
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.Get failed: the index was out of range");
			   return;
		   }
	   }

	   VM_CALLBACK(ArrayCopyByRef)
	   {
		   ArrayImage* a = (ArrayImage*) registers[VM::REGISTER_D4].vPtrValue;
		   int32 index = registers[VM::REGISTER_D7].int32Value;
		   void* structRef = registers[VM::REGISTER_D5].vPtrValue;

		   if (a == nullptr)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(0, "Array.ArrayCopyByRef failed: the array was null");
			   return;
		   }

		   if (index >= 0 && index < a->NumberOfElements)
		   {
			   const void* src = ((const uint8*) a->Start) + index * a->ElementLength;
			   AlignedMemcpy(structRef, src, a->ElementLength);
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.Get failed: the index was out of range");
			   return;
		   }
	   }

	   VM_CALLBACK(ArrayAssign)
	   {
		   ArrayImage* target = (ArrayImage*)registers[VM::REGISTER_D4].vPtrValue;
		   ArrayImage* source = (ArrayImage*)registers[VM::REGISTER_D5].vPtrValue;

		   if (target == source) return;

		   if (source != nullptr)
		   {
			   source->RefCount++;
		   }

		   if (target != nullptr)
		   {
			   target->RefCount--;
			   if (target->RefCount == 0)
			   {
				   ArrayDelete(target, *(IScriptSystem*)context);
			   }
		   }
	   }

	   VM_CALLBACK(ArrayGetLastIndexToD12)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D13].vPtrValue;
		   if (a == nullptr)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(ERANGE, "ArrayGetLastIndex: array was null");
			   return;
		   }

		   if (a->NumberOfElements == 0)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(ERANGE, "ArrayGetLastIndex: array element count was zero");
			   return;
		   }

		   registers[VM::REGISTER_D12].int32Value = a->NumberOfElements - 1;
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
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.ArrayGetRefUnchecked failed: array was unlocked, internal compiler error");
			   return;
		   }

		   if (index < 0 || index >= a->NumberOfElements)
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCodeF(ERANGE, "Array.ArrayGetRefUnchecked failed: index out of range, internal compiler error");
			   return;
		   }
   #endif

	   }

	   VM_CALLBACK(ArrayGetLength)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D4].vPtrValue;
		   registers[VM::REGISTER_D6].int32Value = a ? a->NumberOfElements : 0;
	   }

	   VM_CALLBACK(ArrayReturnLength)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D4].vPtrValue;
		   registers[VM::REGISTER_D7].int32Value = a ? a->NumberOfElements : 0;
	   }

	   VM_CALLBACK(ArrayReturnCapacity)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D4].vPtrValue;
		   registers[VM::REGISTER_D7].int32Value = a ? a->ElementCapacity : 0;
	   }

	   VM_CALLBACK(ArrayGetLastIndex)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D13].vPtrValue;
		   registers[VM::REGISTER_D11].int32Value = (a ? a->NumberOfElements : 0) - 1;
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
			   ss.ThrowFromNativeCodeF(ERANGE, "ArrayGetRefUnchecked failed: array was unlocked, internal compiler error");
			   return;
		   }

		   if (index < 0 || index >= a->NumberOfElements)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(ERANGE, "ArrayGetRefUnchecked failed: index out of range, internal compiler error");
			   return;
		   }
#endif

	   }

	   VM_CALLBACK(ArrayGetInterfaceLockless)
	   {
		   ArrayImage* a = (ArrayImage*)registers[VM::REGISTER_D4].vPtrValue;

		   int32 index = registers[VM::REGISTER_D7].int32Value;

		   if (index < 0 || index >= a->NumberOfElements)
		   {
			   IScriptSystem& ss = *(IScriptSystem*)context;
			   ss.ThrowFromNativeCodeF(ERANGE, "ArrayGetInterfaceLockless failed: index out of range");
			   return;
		   }

		   uint8* pElement = ((uint8*)a->Start) + index * a->ElementLength;
		   InterfacePointer* ppInterface = (InterfacePointer*)pElement;
		   ObjectStub* pObject = InterfaceToInstance(*ppInterface);
		   pObject->refCount++;
		   registers[VM::REGISTER_D7].vPtrValue = *ppInterface;
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
				   ss.ThrowFromNativeCodeF(0, "ArrayLock failed: the array lock reached the upper limit of 0x7FFFFFFF");
				   return;
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
				   ss.ThrowFromNativeCodeF(0, "ArrayUnlock failed: the array lock was not positive");
				   return;
			   }
			   a->LockNumber--;			
		   }
		   else
		   {
			   IScriptSystem& ss = *(IScriptSystem*) context;
			   ss.ThrowFromNativeCodeF(0, "Array.Unlock: the array was not locked");
			   return;
		   }
	   }

	   void CompileConstructArrayElementFromRef(CCompileEnvironment& ce, cr_sex s, const IStructure& elementType, cstr arrayInstance)
	   {
		   const IFunction* constructor = elementType.Constructor();

		   // (<instance>.Push <constructorArg1> ... <constructorArgN>)
		   int inputCount = s.NumberOfElements() - 1;
		   if (inputCount < constructor->NumberOfInputs() - 1)
		   {
			   Throw(s, "Too few arguments in push constructor call");
		   }
		   else if (inputCount >  constructor->NumberOfInputs() - 1)
		   {
			   Throw(s, "Too many arguments in push constructor call");
		   }

		 
		   int inputStackAllocCount = PushInputs(ce, s, *constructor, true, 1);

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);
		   ce.Builder.AssignVariableToTemp(arrayInstance, Rococo::ROOT_TEMPDEPTH, 0); // array goes to D7
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
				   Throw(s, "Expecting either (%s.Push <arg>), (%s.Push ( element-constructor-args...)) or (%s.Push %s (memberwise-constructor-args...).", instanceName, instanceName, instanceName, GetFriendlyName(elementType));
			   }

			   ce.Builder.AssignVariableToTemp(instanceName, Rococo::ROOT_TEMPDEPTH, 0);
			   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayPushAndGetRef);

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
				   ce.Builder.AssignVariableToTemp(instanceName, 0, 0); // array goes to D4

				   if (elementType.InterfaceCount() > 0)
				   {
					   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayPushInterface);
				   }
				   else
				   {
					   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayPushByRef);

					   if (Rococo::Compiler::Impl::CountRefCountedMembers(elementType) > 0)
					   {
						   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayUpdateRefCounts);
					   }
				   }
			   }
		   }	
	   }

	   void CompileAsPushToArray(CCompileEnvironment& ce, cr_sex s, cstr instanceName)
	   {
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, instanceName);

		   AssertNotTooFewElements(s, 2);

		   if (elementType.VarType() == SexyVarType_Derivative)
		   {
			   AssertNotTooManyElements(s, 3);
			   CompileAsPushToArrayByRef( ce, s, instanceName);
		   }
		   else if (elementType.VarType() == SexyVarType_Closure)
		   {
			   AssertNotTooManyElements(s, 2);

			   cr_sex value = s.GetElement(1);

			   if (!TryCompileAssignArchetype(ce, value, elementType, false))
			   {
				   Throw(value, "Could not evaluate the expression as type %s ", GetTypeName(elementType.VarType()));
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
				   Throw(value, "Could not evaluate the expression as type %s", GetTypeName(elementType.VarType()));
			   } // The value is in D7
		
			   ce.Builder.AssignVariableToTemp(instanceName, 0, 0); // array goes to D4

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
	
		   ce.Builder.AssignVariableToTemp(instanceName, Rococo::ROOT_TEMPDEPTH, 0); // array goes to D7

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);
		   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayPop);
	   }

	   void CompileAsClearArray(CCompileEnvironment& ce, cr_sex s, cstr instanceName)
	   {
		   AssertNotTooFewElements(s, 1);
		   AssertNotTooManyElements(s, 1);

		   ce.Builder.AssignVariableToTemp(instanceName, Rococo::ROOT_TEMPDEPTH, 0); // array goes to D7

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);
		   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayClear);
	   }

	   void CompileAsNullifyArray(CCompileEnvironment& ce, cr_sex s, cstr instanceName)
	   {
		   AssertNotTooFewElements(s, 1);
		   AssertNotTooManyElements(s, 1);

		   ce.Builder.AssignVariableToTemp(instanceName, Rococo::ROOT_TEMPDEPTH, 0); // array goes to D7

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);
		   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayRelease);

		   ce.Builder.AssignLiteral(NameString::From(instanceName), "0");
	   }

	   void CompileAsPopOutFromArray(CCompileEnvironment& ce, cr_sex s, cstr instanceName, SexyVarType requiredType)
	   {
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, instanceName);
		   if (elementType.VarType() != requiredType)
		   {
			   Throw(s, "The array pops out type %s, but the expression required type %s", GetTypeName(elementType.VarType()), requiredType);
		   }

		   ce.Builder.AssignVariableToTemp(instanceName, 0, 0); // array goes to D4

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

		   cstr targetToken = target.c_str();

		   if (!AreEqual(mapsTo.String(), ("->")))
		   {
			   Throw(mapsTo, ("Expecting mapping token '->'"));
		   }

		   SexyVarType requiredType = ce.Builder.GetVarType(targetToken);
		   if (requiredType == SexyVarType_Bad)
		   {
			   ThrowTokenNotFound(s, target.c_str(), ce.Builder.Owner().Name(), ("variable"));
		   }

		   CompileAsPopOutFromArray(ce, s, instanceName, requiredType);

		   // The front of the queue was popped into D7
		   ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, targetToken);
	   }

	   void CompileArraySet(CCompileEnvironment& ce, cr_sex s, cstr arrayName)
	   {
		   AssertNotTooFewElements(s, 3);
		   AssertNotTooFewElements(s, 3);

		   cr_sex index = s.GetElement(1);
		   if (!TryCompileArithmeticExpression(ce, index, true, SexyVarType_Int32))
		   {
			   Throw(index, ("Could not evaluate the expression as index type Int32"));
		   }

		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH, 0, BITCOUNT_32); // save the value to D7 for popping to D4
		
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, arrayName);

		   cr_sex value = s.GetElement(2);

		   SexyVarType elementVarType = elementType.VarType();
		
		   if (IsPrimitiveType(elementVarType))		
		   {
			   if (!TryCompileArithmeticExpression(ce, value, true, elementVarType))
			   {
				   Throw(value, "Could not evaluate the expression as type %s", GetTypeName(elementVarType));
			   } // The value is in D7
		   }
		   else
		   {
			   if (!IsAtomic(value))
			   {
				   Throw(value, ("Expecting variable reference"));
			   }

			   MemberDef def;
			   AssertGetVariable(def, value.c_str(), ce, value);
			   ce.Builder.AssignVariableRefToTemp(value.c_str(), Rococo::ROOT_TEMPDEPTH); // The value reference is in D7 
		   }

		   ce.Builder.PopLastVariables(1, true);
		   ce.Builder.AssignVariableToTemp(arrayName, 1, 0); // The array is in D5

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);

		   switch (elementVarType)
		   {
		   case SexyVarType_Int32:
		   case SexyVarType_Float32:
			   ce.Builder.Assembler().Append_Invoke(callbacks.ArraySet32);
			   break;
		   case SexyVarType_Float64:
		   case SexyVarType_Int64:
			   ce.Builder.Assembler().Append_Invoke(callbacks.ArraySet64);
			   break;
		   case SexyVarType_Derivative:
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
			   Throw(value, "Bad type");
		   }
	   }

	   bool TryCompileAsArrayCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName)
	   {
 		   if (AreEqual(methodName, "Push"))
		   {
			   CompileAsPushToArray(ce, s, instanceName);
			   return true;
		   }
		   else if (AreEqual(methodName, "Pop"))
		   {
			   CompileAsPopFromArray(ce, s, instanceName);
			   return true;
		   }
		   else if (AreEqual(methodName, "PopOut"))
		   {
			   CompileAsPopOutFromArrayToVariable(ce, s, instanceName);
			   return true;
		   }
		   else if (AreEqual(methodName, "Clear"))
		   {
			   CompileAsClearArray(ce, s, instanceName);
			   return true;
		   }
		   else if (AreEqual(methodName, "Null"))
		   {
			   CompileAsNullifyArray(ce, s, instanceName);
			   return true;
		   }
		   else if (AreEqual(methodName, "Set"))
		   {
			   CompileArraySet(ce, s, instanceName);
			   return true;
		   }
		   else
		   {
			   char hint[256];
			   SafeFormat(hint, "Is the desired semantic %s.[Push|Pop|PopOut|Clear|Set|Null]? %s not found.", instanceName, methodName);
			   ce.Object.Log().Write(hint);
			   return false;
		   }
	   }

	   void ValidateElementType(CCompileEnvironment& ce, cr_sex s, cstr instanceName, SexyVarType type, const IStructure* structType)
	   {
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, instanceName);

		   if (elementType.VarType() != type)
		   {
			   Throw(s, "The array element type does not match the type of the variable in the assignment");
		   }

		   if (type == SexyVarType_Derivative && *structType != elementType)
		   {
			   Throw(s, "The array element type does not match the type of the variable in the assignment");
		   }
	   }

	   void CompileGetArrayElement(CCompileEnvironment& ce, cr_sex s, cstr instanceName, SexyVarType varType, const IStructure* structType)
	   {
		   ValidateElementType(ce, s, instanceName, varType, structType);

		   if (!TryCompileArithmeticExpression(ce, s, true, SexyVarType_Int32))
		   {
			   Throw(s, "Expected expression to evaluate to type Int32 to serve as index to array");
		   } // D7 now contains the array index

		
		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);

		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, instanceName);

		   ce.Builder.AssignVariableToTemp(instanceName, 0, 0); // array goes to D13

		   if (elementType.InterfaceCount() >= 1)
		   {
			   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayGetInterfaceLockless);
		   }
		   else
		   {
			   switch (SizeOfElement(elementType))
			   {
			   case 4:
				   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayGet32);
				   break;
			   case 8:
				   ce.Builder.Assembler().Append_Invoke(callbacks.ArrayGet64);
				   break;
			   default:
				   Throw(s, "Array.Get only support 32-bit and 64-bit types");
				   break;
			   }
		   }
	   }

	   void CompileGetArraySubelement(CCompileEnvironment& ce, cr_sex indexExpr, cr_sex subItemName, cstr instanceName, SexyVarType type, const IStructure* structType)
	   {
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, indexExpr, instanceName);

		   int offset = 0;
		   const IMember* member = FindMember(elementType, subItemName.c_str(), OUT offset);
		   if (member == NULL)
		   {
			   ThrowTokenNotFound(subItemName, subItemName.c_str(), elementType.Name(), "member");
		   }

		   const IStructure& memberType = *member->UnderlyingType();

		   if (memberType.VarType() != type || (type == SexyVarType_Derivative && memberType != *structType))
		   {
			   cstr requiredType = structType == NULL ? GetTypeName(type) : structType->Name();
			   Throw(subItemName, "The array element type %s does not match the type required: %s", elementType.Name(), requiredType);   
		   }

		   if (!TryCompileArithmeticExpression(ce, indexExpr, true, SexyVarType_Int32)) 
		   {
			   Throw(indexExpr, "Expected expression to evaluate to type Int32 to serve as index to array");
		   } // D7 now contains the array index

		   ce.Builder.AssignVariableToTemp(instanceName, 0, 0); // array goes to D4

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
			   Throw(indexExpr, "Array.GetSubElement only support 32-bit and 64-bit types");
			   break;
		   }
	   }

	   void CompileValidateIndexPositive(CCompileEnvironment& ce, cr_sex s, int tempDepth)
	   {
		   ce.Builder.AddSymbol("ValidateIndexPositive...");
		   ce.Builder.Assembler().Append_MoveRegister(tempDepth + VM::REGISTER_D4, VM::REGISTER_D4, BITCOUNT_32); 
		   ce.Builder.Assembler().Append_Test(VM::REGISTER_D4, BITCOUNT_32);

		   size_t branchPos = ce.Builder.Assembler().WritePosition();
		   ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_GREATER_OR_EQUAL, 6);

		   const IFunction& fnThrow = GetFunctionByFQN(ce, s, "Sys.ThrowIndexNegative");
		   CodeSection section;
		   fnThrow.Code().GetCodeSection(section);
		
		   ce.Builder.Assembler().Append_CallById(section.Id);
		   ce.Builder.AddSymbol("...ValidateIndexPositive");
		   ce.Builder.Assembler().Append_NoOperation();

		   size_t skipDelta = ce.Builder.Assembler().WritePosition() - branchPos;
		   ce.Builder.Assembler().SetWriteModeToOverwrite(branchPos);
		   ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_GREATER_OR_EQUAL, (int32) skipDelta);
		   ce.Builder.Assembler().SetWriteModeToAppend();
	   }

	   void CompileValidateIndexLowerThanArrayElementCount(CCompileEnvironment& ce, cr_sex s, int indexTempDepth, cstr arrayName)
	   {
		   ce.Builder.AssignVariableToTemp(arrayName, 0, 0); // This shifts the array pointer to D4

		   const ArrayCallbacks& callbacks = GetArrayCallbacks(ce);

		   AppendInvoke(ce, callbacks.ArrayGetLength, s); // the length is now written to D6

		   ce.Builder.AddSymbol("ValidateIndexLowerThanArrayElementCount...");
		   ce.Builder.Assembler().Append_IntSubtract(VM::REGISTER_D6, BITCOUNT_32, VM::REGISTER_D4 + indexTempDepth);
		   ce.Builder.Assembler().Append_Test(VM::REGISTER_D5, BITCOUNT_32);

		   size_t branchPos = ce.Builder.Assembler().WritePosition();
		   ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_GREATER_THAN, 6);

		   const IFunction& fnThrow = GetFunctionByFQN(ce, s, "Sys.ThrowIndexExceededBounds");
		   CodeSection section;
		   fnThrow.Code().GetCodeSection(section);
		
		   ce.Builder.Assembler().Append_CallById(section.Id);
		   MarkStackRollback(ce, s);
		   ce.Builder.AddSymbol("ValidateIndexLowerThanArrayElementCount...");
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
			   if (Parse::PARSERESULT_GOOD == Parse::TryParseDecimal(OUT value, s.c_str()))
			   {
				   if (value < 0) Throw(s, "Index must not be negative");
				   return true;
			   }
		   }

		   return false;
	   }

	   int AddVariableArrayLock(CCompileEnvironment& ce, int arrayTempDepth)
	   {
		   TokenBuffer lockName;
		   StringPrint(lockName, "_arrayLock_%d", ce.SS.NextID());

		   const IStructure& lockStruct = *ce.Object.Common().SysNative().FindStructure(("_Lock"));
		
		   ce.Builder.AddSymbol(lockName);
		   AddVariable(ce, NameString::From(lockName), lockStruct);

		   TokenBuffer lockSource;
		   StringPrint(lockSource, "%s._lockSource", (cstr) lockName);
		   ce.Builder.AssignTempToVariable(arrayTempDepth, lockSource);

		   TokenBuffer lockMemberOffset;
		   StringPrint(lockMemberOffset, "%s._lockMemberOffset", (cstr) lockName);

		   MemberDef def;
		   ce.Builder.TryGetVariableByName(def, lockMemberOffset);

		   int offset;
		   const IMember* member = FindMember(ce.StructArray(), "_lock", OUT offset);

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
			   Throw(s.GetElement(2), "Expecting #");
		   }

		   cr_sex elementSpecifier = s.GetElement(hashIndex + 1);
		   AssertCompound(elementSpecifier);
		   AssertNotTooFewElements(elementSpecifier, 2);
		   AssertNotTooManyElements(elementSpecifier, 2);

		   cr_sex collection = GetAtomicArg(elementSpecifier, 0);
		   cstr collectionName = collection.c_str();
		   AssertLocalVariableOrMember(collection);			

		   cr_sex refExpr =  GetAtomicArg(s, 1);
		   AssertLocalIdentifier(refExpr);
		   cstr refName = refExpr.c_str();
		
		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, collectionName);

		   AddVariableRef(ce, NameString::From(refName), elementType);

		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 6, Rococo::ROOT_TEMPDEPTH + 6, BITCOUNT_POINTER);

		   ce.Builder.AddSymbol(collectionName);
		   ce.Builder.AssignVariableToTemp(collectionName, 9, 0); // Array ref is now in D13
				
		   ce.Builder.AddSymbol("(foreach..."); 

		   cr_sex indexExpr = elementSpecifier.GetElement(1);
		
		   int32 indexValue;
		   if (TryParseAsIndexPositiveLiteralInt32(OUT indexValue, indexExpr))
		   {
			   VariantValue v;
			   v.int32Value = indexValue;
			   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D12, v, BITCOUNT_32);
		   }
		   else if (IsAtomic(indexExpr))
		   {
			   if (AreEqual(indexExpr.String(), "@last"))
			   {
				   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayGetLastIndexToD12);
			   }
		   }
		   else
		   {
			   if (!TryCompileArithmeticExpression(ce, indexExpr, true, SexyVarType_Int32)) Throw(indexExpr, ("Failed to parse expression as (Int32 startIndex)"));
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
	
		   ce.Builder.AddSymbol("...foreach)");
		//   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayUnlock); // Enable popping of the array after enumeration has finished
	   }

		// (foreach v # <collection-name> (...) (...) )
	   cstr GetCollectionNameFromForeachExpression(cr_sex scollection)
	   {
		   if (IsCompound(scollection))
		   {
			   AssertNotTooFewElements(scollection, 3);
			   AssertNotTooManyElements(scollection, 3);

			   cr_sex collectionNameExpr = GetAtomicArg(scollection, 0);
			   cstr collectionName = collectionNameExpr.c_str();
			   AssertLocalVariableOrMember(collectionNameExpr);
			   return collectionName;
		   }
		   else
		   {
			   cstr collectionName = scollection.c_str();
			   AssertLocalVariableOrMember(scollection);
			   return collectionName;
		   }
	   }

	   void CompileEnumerateArray(CCompileEnvironment& ce, cr_sex s, int hashIndex)
	   {
		   // (foreach v # a (...) (...) )

		   if (hashIndex != 2)
		   {
			   Throw(s, "Expecting # at position 2");
		   }

		   cr_sex collectionNameExpr = GetAtomicArg(s, 3);

		   cstr collectionName = collectionNameExpr.c_str();

		   // (foreach v # a (...) )
		   cr_sex refExpr = s[1];
		   AssertLocalIdentifier(refExpr);
		   cstr refName = refExpr.c_str();

		   const IStructure& elementType = GetElementTypeForArrayVariable(ce, s, collectionName);

		   if (elementType.InterfaceCount() == 0)
		   {
			   AddVariableRef(ce, NameString::From(refName), elementType);
		   }
		   else
		   {
			   AddVariable(ce, NameString::From(refName), elementType);
			   VariantValue nullVal;
			   nullVal.vPtrValue = (uint8*)elementType.GetInterface(0).UniversalNullInstance()->pVTables;
			   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, nullVal, BITCOUNT_POINTER);
			   ce.Builder.AssignTempToVariable(0, refName);
		   }

		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 6, Rococo::ROOT_TEMPDEPTH + 6, BITCOUNT_POINTER);

		   ce.Builder.AddSymbol(collectionName);
		   ce.Builder.AssignVariableToTemp(collectionName, 9, 0); // Array ref is now in D13

		   ce.Builder.AddSymbol("D12 - working index");
		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 5, Rococo::ROOT_TEMPDEPTH + 5, BITCOUNT_POINTER);

		   ce.Builder.AddSymbol("(foreach...");

		   VariantValue zero;
		   zero.int32Value = 0;
		   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D12, zero, BITCOUNT_32);

		   // We may be nested in a function that overwrites D11, so save it
		   ce.Builder.AddSymbol("D11 - final index");
		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 4, Rococo::ROOT_TEMPDEPTH + 4, BITCOUNT_POINTER);

		   ce.Builder.AddSymbol("(D13 array)->(D11 lastIndex)");
		   AppendInvoke(ce, GetArrayCallbacks(ce).ArrayGetLastIndex, s);

		   // We may be nested in a function that overwrites D10, which is used as the result of D10-11
		   AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 3, Rococo::ROOT_TEMPDEPTH + 3, BITCOUNT_POINTER);

		   // Prevent popping of the array during enumeration, since this may corrupt our references
		   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayLock);

		   int lockSize = AddVariableArrayLock(ce, 9);

		   struct ConditionSection : public ICompileSection
		   {
			   ConditionSection(CCompileEnvironment& _ce) : ce(_ce) {}

			   CCompileEnvironment& ce;

			   void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
			   {
				   ce.Builder.AddSymbol("while (endIndex > currentIndex)");
				   ce.Builder.Assembler().Append_IntSubtract(VM::REGISTER_D11, BITCOUNT_32, VM::REGISTER_D12);
				   builder.Assembler().Append_Test(VM::REGISTER_D10, BITCOUNT_32);
			   }
		   } loopCriterion(ce);

		   struct BodySection : public ICompileSection
		   {
			   BodySection(CCompileEnvironment& _ce, cr_sex _s, int _firstBodyIndex, int _lastBodyIndex, cstr _refName) :
				   ce(_ce), s(_s), refName(_refName), firstBodyIndex(_firstBodyIndex), lastBodyIndex(_lastBodyIndex) {}

			   CCompileEnvironment& ce;
			   cr_sex s;
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
					   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayGetRefUnchecked); // returns pointer to element in D7, D12 is element index and D13 is array ref
				   }
				   else
				   {
					   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayGetInterfaceUnchecked);
				   }

				   ce.Builder.Assembler().Append_SetStackFrameValue(refDef.SFOffset, VM::REGISTER_D7, BITCOUNT_POINTER);

				   VariantValue one;
				   one.int32Value = 1;
				   ce.Builder.Assembler().Append_AddImmediate(VM::REGISTER_D12, BITCOUNT_32, VM::REGISTER_D12, one); // increment the running index

				   CompileExpressionSequence(ce, firstBodyIndex, lastBodyIndex, s);

				   builder.PopControlFlowPoint();
				   ce.Builder.AddSymbol("...while }");
			   }
		   } bodySection(ce, s, hashIndex + 2, s.NumberOfElements() - 1, refName);

		   struct NoFinalSection : public ICompileSection
		   {
			   void Compile(ICodeBuilder& builder, IProgramObject& object, ControlFlowData* controlFlowData) override
			   {
			   }
		   } noFinalSection;

		   ce.Builder.AppendWhileDo(loopCriterion, CONDITION_IF_GREATER_OR_EQUAL, bodySection, noFinalSection);

		   //   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayUnlock); // Enable popping of the array after enumeration has finished
		   ce.Builder.AddSymbol("...foreach)");
		   ce.Builder.Assembler().Append_NoOperation();
	   }

	   void CompileArrayDestruct(CCompileEnvironment& ce, const IStructure& s, cstr instanceName)
	   {
		   MemberDef def;
		   ce.Builder.TryGetVariableByName(def, instanceName);

		   if (!def.IsContained || def.Usage == ARGUMENTUSAGE_BYVALUE)
		   {
			   ce.Builder.AssignVariableToTemp(instanceName, Rococo::ROOT_TEMPDEPTH);
			   AppendInvoke(ce, GetArrayCallbacks(ce).ArrayRelease, *(const ISExpression*)s.Definition());
		   }
	   }

	   void CompileNumericExpression(CCompileEnvironment& ce, cr_sex valueExpr, SexyVarType type)
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
			   cstr svalue = valueExpr.c_str();
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
				   SexyVarType atomicType = GetAtomicValueAnyNumeric(ce, valueExpr, valueExpr.c_str(), Rococo::ROOT_TEMPDEPTH);
				   if (atomicType == type) return;
			   }
		   }

		   Throw(valueExpr, "Expecting an expression that evaluates to %s ", GetTypeName(type));
	   }

	   void CompileArrayDeclarationAndAssign(CCompileEnvironment& ce, cr_sex s)
	   {
		   // (array Int32 a = (this.GetArray))
		   if (s.NumberOfElements() != 5)
		   {
			   Throw(s, "Expecting array declaration with assign consisting of five elements");
		   }

		   cr_sex assignChar = GetAtomicArg(s, 3);

		   if (!Eq(assignChar.c_str(), "="))
		   {
			   Throw(assignChar, "Expecting array assignment operator = at this position");
		   }

		   cr_sex typeName = GetAtomicArg(s, 1);
		   cr_sex arrayName = GetAtomicArg(s, 2);

		   cstr arrayNameTxt = arrayName.c_str();

		   AssertLocalIdentifier(arrayName);

		   const IStructure& arrayStruct = ce.StructArray();

		   const IStructure* elementStruct = MatchStructure(typeName, ce.Builder.Module());
		   if (elementStruct == NULL) ThrowTokenNotFound(s, typeName.c_str(), ce.Builder.Module().Name(), ("type"));

		   if (elementStruct->InterfaceCount() != 0 && !IsNullType(*elementStruct))
		   {
			   Throw(s, "Arrays cannot have class type elements. Use an interface to %s instead.", elementStruct->Name());
		   }

		   AddArrayDef(ce.Script, ce.Builder, arrayNameTxt, *elementStruct, s);

		   AddVariableRef(ce, NameString::From(arrayNameTxt), ce.StructArray());

		   // The assignment may throw an exception, so we need to null out the reference 
		   ce.Builder.AssignPointer(NameString::From(arrayNameTxt), nullptr);

		   if (TryCompileFunctionCallAndReturnValue(ce, s[4], SexyVarType_Array, elementStruct, nullptr))
		   {
				AddSymbol(ce.Builder, "D7 -> %s", (cstr)arrayNameTxt);
				ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, arrayNameTxt);
		   }
		   else
		   {
			   Throw(s, "The RHS of the array assignment must be a method or function call that returns an array of type %s", GetFriendlyName(*elementStruct));
		   }
	   }

	   // Example: (array Int32 a 4)...but also (array Int32 a)....this latter definition specifies an array reference
	   // We can also initialize from  a function or method (array Int32 a = (container.GetArray))
	   void CompileArrayDeclaration(CCompileEnvironment& ce, cr_sex s)
	   {
		   if (s.NumberOfElements() == 5)
		   {
			   CompileArrayDeclarationAndAssign(ce, s);
			   return;
		   }
		   AssertNotTooFewElements(s, 3);
		   AssertNotTooManyElements(s, 4);

		   cr_sex typeName = GetAtomicArg(s, 1);
		   cr_sex arrayName = GetAtomicArg(s, 2);

		   cstr arrayNameTxt = arrayName.c_str();

		   AssertLocalIdentifier(arrayName);

		   const ISExpression* scapacity = s.NumberOfElements() == 4 ? &s.GetElement(3) : nullptr;

		   // (array <element-type-name> <array-name> <capacity>
		   const IStructure& arrayStruct = ce.StructArray();

		   const IStructure* elementStruct = MatchStructure(typeName, ce.Builder.Module());
		   if (elementStruct == NULL) ThrowTokenNotFound(s, typeName.c_str(), ce.Builder.Module().Name(), ("type"));

		   if (elementStruct->InterfaceCount() != 0 && !IsNullType(*elementStruct))
		   {
			   Throw(s, "Arrays cannot have class type elements. Use an interface to %s instead.", elementStruct->Name());
		   }

		   AddArrayDef(ce.Script, ce.Builder, arrayNameTxt, *elementStruct, s);

		   if (scapacity)
		   {
			   AddSymbol(ce.Builder, "int32 capacity");
			   CompileNumericExpression(ce, *scapacity, SexyVarType_Int32); // capacity to D7

			   VariantValue v;
			   v.vPtrValue = (void*)elementStruct;
			   AddSymbol(ce.Builder, "type %s", GetFriendlyName(*elementStruct));
			   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_POINTER); // Element type to D4

			   AddSymbol(ce.Builder, "(D4,D7)->(D5)", arrayNameTxt);
			   AppendInvoke(ce, GetArrayCallbacks(ce).ArrayInit, s);
		   }

		   AddVariableRef(ce, NameString::From(arrayNameTxt), ce.StructArray());

		   if (scapacity)
		   {
			   AddSymbol(ce.Builder, "assign array '%s' to SF", arrayNameTxt);

			   // Array ref is copied from D5, the result of the ArrayInit
			   MemberDef def;
			   ce.Builder.TryGetVariableByName(def, arrayNameTxt);
			   ce.Builder.Assembler().Append_SetStackFrameValue(def.SFOffset + def.MemberOffset, VM::REGISTER_D5, BITCOUNT_POINTER);
		   }
		   else
		   {
			   ce.Builder.AssignPointer(NameString::From(arrayNameTxt), nullptr);
		   }
	   }

	   void CompileArrayConstruct(CCompileEnvironment& ce, cr_sex conDef, const IMember& member, cstr fullName)
	   {
		   AssertNotTooManyElements(conDef, 3);
		   cr_sex value = conDef.GetElement(2);
		   CompileNumericExpression(ce, value, SexyVarType_Int32); // The capacity is now in D7

		   VariantValue v;
		   v.vPtrValue = (void*)member.UnderlyingGenericArg1Type();
		   ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_POINTER); // Element type to D4

		   AddArrayDef(ce.Script, ce.Builder, fullName, *member.UnderlyingGenericArg1Type(), conDef);

		   ce.Builder.Assembler().Append_Invoke(GetArrayCallbacks(ce).ArrayInit); // new array pointer ends up in D5

		   ce.Builder.AssignTempToVariable(1, fullName);
	   }

	   void RegisterArrays(ArrayCallbacks& arrayCallbacks, VM::ICore& core, IScriptSystem& ss)
	   {
		   arrayCallbacks.ArrayAssign = core.RegisterCallback(OnInvokeArrayAssign, &ss, "ArrayAssign");
		   arrayCallbacks.ArrayGetLastIndexToD12 = core.RegisterCallback(OnInvokeArrayGetLastIndexToD12, &ss, "ArrayGetLastIndexToD12");
		   arrayCallbacks.ArrayGetRefUnchecked = core.RegisterCallback(OnInvokeArrayGetRefUnchecked, &ss, "ArrayGetRefUnchecked");
		   arrayCallbacks.ArrayLock = core.RegisterCallback(OnInvokeArrayLock, &ss, "ArrayLock");
		   arrayCallbacks.ArrayUnlock = core.RegisterCallback(OnInvokeArrayUnlock, &ss, "ArrayUnlock");
		   arrayCallbacks.ArrayClear = core.RegisterCallback(OnInvokeArrayClear, &ss, "ArrayClear");
		   arrayCallbacks.ArrayPushAndGetRef = core.RegisterCallback(OnInvokeArrayPushAndGetRef, &ss, "ArrayPushAndGetRef");
		   arrayCallbacks.ArrayPushByRef = core.RegisterCallback(OnInvokeArrayPushByRef, &ss, "ArrayPushByRef");
		   arrayCallbacks.ArrayUpdateRefCounts = core.RegisterCallback(OnInvokeArrayUpdateRefCounts, &ss, "ArrayUpdateRefCounts");
		   arrayCallbacks.ArrayPush32 = core.RegisterCallback(OnInvokeArrayPush32, &ss, "ArrayPush32");
		   arrayCallbacks.ArrayPushInterface = core.RegisterCallback(OnInvokeArrayPushInterface, &ss, "ArrayPushInterface");
		   arrayCallbacks.ArrayPush64 = core.RegisterCallback(OnInvokeArrayPush64, &ss, "ArrayPush64");
		   arrayCallbacks.ArrayGet32 = core.RegisterCallback(OnInvokeArrayGet32, &ss, "ArrayGet32");
		   arrayCallbacks.ArrayGet64 = core.RegisterCallback(OnInvokeArrayGet64, &ss, "ArrayGet64");
		   arrayCallbacks.ArrayGetMember32 = core.RegisterCallback(OnInvokeArrayGetMember32, &ss, "ArrayGetMember32");
		   arrayCallbacks.ArrayGetMember64 = core.RegisterCallback(OnInvokeArrayGetMember64, &ss, "ArrayGetMember64");
		   arrayCallbacks.ArrayCopyByRef = core.RegisterCallback(OnInvokeArrayCopyByRef, &ss, "ArrayCopyByRef");
		   arrayCallbacks.ArrayInit = core.RegisterCallback(OnInvokeArrayInit, &ss, "ArrayInit");
		   arrayCallbacks.ArrayRelease = core.RegisterCallback(OnInvokeArrayReleaseRef, &ss, "ArrayReleaseRef");
		   arrayCallbacks.ArraySet32 = core.RegisterCallback(OnInvokeArraySet32, &ss, "ArraySet32");
		   arrayCallbacks.ArraySet64 = core.RegisterCallback(OnInvokeArraySet64, &ss, "ArraySet64");
		   arrayCallbacks.ArraySetByRef = core.RegisterCallback(OnInvokeArraySetByRef, &ss, "ArraySetByRef");
		   arrayCallbacks.ArrayPop = core.RegisterCallback(OnInvokeArrayPop, &ss, "ArrayPop");
		   arrayCallbacks.ArrayPopOut32 = core.RegisterCallback(OnInvokeArrayPopOut32, &ss, "ArrayPopOut32");
		   arrayCallbacks.ArrayPopOut64 = core.RegisterCallback(OnInvokeArrayPopOut64, &ss, "ArrayPopOut64");
		   arrayCallbacks.ArrayDestructElements = core.RegisterCallback(OnInvokeArrayDestructElements, &ss, "ArrayDestructElements");
		   arrayCallbacks.ArrayGetInterfaceUnchecked = core.RegisterCallback(OnInvokeArrayGetInterfaceUnchecked, &ss, "ArrayGetInterface");
		   arrayCallbacks.ArrayGetInterfaceLockless = core.RegisterCallback(OnInvokeArrayGetInterfaceLockless, &ss, "ArrayGetInterfaceLockless");
		   arrayCallbacks.ArrayGetLength = core.RegisterCallback(OnInvokeArrayGetLength, &ss, "ArrayGetLength");
		   arrayCallbacks.ArrayGetLastIndex = core.RegisterCallback(OnInvokeArrayGetLastIndex, &ss, "ArrayGetLastIndex");
		   arrayCallbacks.ArrayReturnLength = core.RegisterCallback(OnInvokeArrayReturnLength, &ss, "ArrayReturnLength");
		   arrayCallbacks.ArrayReturnCapacity = core.RegisterCallback(OnInvokeArrayReturnCapacity, &ss, "ArrayReturnCapacity");
	   }
   }//Script
}//Sexy