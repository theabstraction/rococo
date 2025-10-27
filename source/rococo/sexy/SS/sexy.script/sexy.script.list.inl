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

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM 'AS IS' WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

namespace Rococo::Script
{
	struct OrphanedNodeList
	{
		DEFINE_SEXY_ALLOCATORS_FOR_CLASS

		TSexyVector<ListNode*> items;
	};

	ListNode* CreateNewNode(ListImage* l, IScriptSystem& ss)
	{
		ListNode* n = (ListNode*)ss.AlignedMalloc(16, sizeof(ListNode) + l->ElementSize - 4);
		n->Container = l;
		n->ElementType = l->ElementType;
		n->RefCount = 1;
		return n;
	}

	struct ListLifetimeManager: Rococo::Compiler::IMemberLifeSupervisor
	{
		IScriptSystem& ss;

		void Release(uint8* instance) override
		{
			ListImage* l = *reinterpret_cast<ListImage**>(instance);
			if (l != nullptr)
			{
				ListRelease(l, ss);
			}
		}

		ListLifetimeManager(IScriptSystem& _ss): ss(_ss)
		{

		}

		void Free() override
		{
			delete this;
		}
	};

	IMemberLifeSupervisor* CreateListLifetimeManager(IScriptSystem& ss)
	{
		return new ListLifetimeManager(ss);
	}

	int ReleaseNode(ListNode* n, IScriptSystem& ss)
	{
		n->RefCount--;
		if (n->RefCount == 0)
		{
			DestroyObject(*n->ElementType, (uint8*)n->Element, ss);
			ss.AlignedFree(n);
			return 0;
		}
		else
		{
			return n->RefCount;
		}
	}

	void AddRef(ListNode* n)
	{
		n->RefCount++;
	}

	ListImage* CreateListImage(IScriptSystem& ss, const IStructure& valueType)
	{
		auto* p = ss.AlignedMalloc(16, sizeof ListImage);

		ListImage* l = new (p) ListImage;

		l->refCount = 1;
		l->ElementSize = valueType.InterfaceCount() != 0 ? sizeof(size_t) : valueType.SizeOfStruct();
		l->ElementType = &valueType;
		l->LockNumber = 0;
		l->NumberOfElements = 0;
		l->Head = NULL;
		l->Tail = NULL;
		l->OrphanedNodeList = NULL;

		return l;
	}

	VM_CALLBACK(ListInit)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		const IStructure* elementType = (const IStructure*)registers[VM::REGISTER_D5].vPtrValue;
		auto* l = CreateListImage(ss, *elementType);
		registers[VM::REGISTER_D4].vPtrValue = l;
	}

	ListNode* AppendToList(ListImage* l, IScriptSystem& ss)
	{
		ListNode* newNode = CreateNewNode(l, ss);

		if (l->Tail == NULL)
		{
			l->Head = newNode;
			newNode->Next = NULL;
			newNode->Previous = NULL;
		}
		else
		{
			l->Tail->Next = newNode;
			newNode->Next = NULL;
			newNode->Previous = l->Tail;
		}

		l->Tail = newNode;

		l->NumberOfElements++;

		return newNode;
	}

	// This function is used by reflection APIs to manually create a new node
	uint8* AppendListNode(IScriptSystem& ss, ListImage& l)
	{
		ListNode* newNode = AppendToList(&l, ss);
		return (uint8*)newNode->Element;
	}

	ListNode* AppendToNode(ListNode* n, IScriptSystem& ss)
	{
		ListImage* l = n->Container;
		ListNode* newNode = CreateNewNode(l, ss);

		// Asumme node is not null

		if (l->Tail == n)
		{
			l->Tail = newNode;
			n->Next = newNode;
			newNode->Next = NULL;
			newNode->Previous = n;
		}
		else
		{
			newNode->Previous = n;
			newNode->Next = n->Next;
			n->Next->Previous = newNode;
			n->Next = newNode;
		}

		l->NumberOfElements++;

		return newNode;
	}

	ListNode* PrependToList(ListImage* l, IScriptSystem& ss)
	{
		ListNode* newNode = CreateNewNode(l, ss);

		if (l->Tail == NULL)
		{
			l->Tail = newNode;
			newNode->Next = NULL;
			newNode->Previous = NULL;
		}
		else
		{
			l->Head->Previous = newNode;
			newNode->Next = l->Head;
			newNode->Previous = NULL;
		}

		l->Head = newNode;
		l->NumberOfElements++;

		return newNode;
	}

	ListNode* PrependToNode(ListNode* n, IScriptSystem& ss)
	{
		ListImage* l = n->Container;
		ListNode* newNode = CreateNewNode(l, ss);

		if (n == l->Head)
		{
			l->Head = newNode;
			newNode->Next = n;
			newNode->Previous = NULL;
			n->Previous = newNode;
		}
		else
		{
			newNode->Next = n;
			newNode->Previous = n->Previous;
			n->Previous->Next = newNode;
			n->Previous = newNode;
		}

		l->NumberOfElements++;

		return newNode;
	}

	void ListClear(ListImage& l, IScriptSystem& ss)
	{
		ListNode* n = l.Head;
		while (n != NULL)
		{
			ListNode* next = n->Next;
			n->Next = n->Previous = NULL;
			if (n->RefCount > 1)
			{
				ReleaseNode(n, ss);
			}
			else
			{
				// We need to maintain a reference to the nodes, as they can be referenced during enumeration, which does not increase the ref count.
				if (l.OrphanedNodeList == NULL)
				{
					l.OrphanedNodeList = new OrphanedNodeList();
				}
				n->Container = nullptr;
				l.OrphanedNodeList->items.push_back(n);
			}
			n = next;
		}

		l.Head = l.Tail = nullptr;
		l.NumberOfElements = 0;
	}

	void ListRelease(ListImage* l, IScriptSystem& ss)
	{
		if (!l)
		{
			return;
		}

		l->refCount--;

		if (l->refCount <= 0)
		{
			ListClear(*l, ss);
			if (l->OrphanedNodeList)
			{
				// The list is out of scope, which means all orphaned nodes have to be out of scope, so time to release them
				for (auto* orphan : l->OrphanedNodeList->items)
				{
					ReleaseNode(orphan, ss);
				}
				delete l->OrphanedNodeList;
			}
			ss.AlignedFree(l);
		}
	}

	VM_CALLBACK(ListAssign)
	{
		ListImage* target = (ListImage*)registers[VM::REGISTER_D4].vPtrValue;
		ListImage* source = (ListImage*)registers[VM::REGISTER_D5].vPtrValue;

		if (target == source) return;

		if (source != nullptr)
		{
			source->refCount++;
		}

		if (target != nullptr)
		{
			ListRelease(target, *reinterpret_cast<IScriptSystem*>(context));
		}
	}

	VM_CALLBACK(ListAppend)
	{
		void* src = registers[VM::REGISTER_D7].vPtrValue;
		ListImage* l = (ListImage*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		if (l)
		{
			ListNode* newNode = AppendToList(l, ss);
			AlignedMemcpy(newNode->Element, src, l->ElementSize);
		}
		else
		{
			ss.ThrowFromNativeCodeF(-1, "ListAppend: the list was null");
			return;
		}
	}

	VM_CALLBACK(ListClear)
	{
		ListImage* l = (ListImage*)registers[VM::REGISTER_D7].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;
		if (l) ListClear(*l, ss);
	}

	VM_CALLBACK(ListRelease)
	{
		ListImage* l = (ListImage*)registers[VM::REGISTER_D7].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;
		ListRelease(l, ss);
	}

	VM_CALLBACK(NodeGoPrevious)
	{
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		VariantValue boolResult;
		boolResult.int32Value = n->Previous == nullptr ? 0 : 1;
		registers[VM::REGISTER_D7] = boolResult;

		if (boolResult.int32Value != 0)
		{
			ReleaseNode(n, ss);
			n = n->Previous;
			AddRef(n);
			registers[VM::REGISTER_D4].vPtrValue = n;
		}
	}

	VM_CALLBACK(NodeGoNext)
	{
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		VariantValue boolResult;
		boolResult.int32Value = n->Next == nullptr ? 0 : 1;
		registers[VM::REGISTER_D7] = boolResult;

		if (boolResult.int32Value != 0)
		{
			ReleaseNode(n, ss);
			n = n->Next;
			AddRef(n);
			registers[VM::REGISTER_D4].vPtrValue = n;
		}
	}

	VM_CALLBACK(NodeAppend)
	{
		void* src = registers[VM::REGISTER_D7].vPtrValue;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		ListNode* newNode = AppendToNode(n, ss);
		AlignedMemcpy(newNode->Element, src, n->Container->ElementSize);
	}

	VM_CALLBACK(ListAppend32)
	{
		int32 src = registers[VM::REGISTER_D7].int32Value;
		ListImage* l = (ListImage*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		if (l)
		{
			ListNode* newNode = AppendToList(l, ss);
			*((int32*)newNode->Element) = src;
		}
		else
		{
			ss.ThrowFromNativeCodeF(0, "Append failed - the list was null");
			return;
		}
	}

	VM_CALLBACK(NodeAppend32)
	{
		int32 src = registers[VM::REGISTER_D7].int32Value;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		ListNode* newNode = AppendToNode(n, ss);
		*((int32*)newNode->Element) = src;
	}

	VM_CALLBACK(ListAppend64)
	{
		int64 src = registers[VM::REGISTER_D7].int64Value;
		ListImage* l = (ListImage*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		if (l)
		{
			ListNode* newNode = AppendToList(l, ss);
			*((int64*)newNode->Element) = src;
		}
		else
		{
			ss.ThrowFromNativeCodeF(0, "Append failed - the list was null");
			return;
		}
	}

	VM_CALLBACK(ListAppendInterface)
	{
		auto src = (InterfacePointer)registers[VM::REGISTER_D7].vPtrValue;
		ListImage* l = (ListImage*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		if (l)
		{
			ListNode* newNode = AppendToList(l, ss);

			*(InterfacePointer*)newNode->Element = src;

			ss.ProgramObject().IncrementRefCount(src);
		}
		else
		{
			ss.ThrowFromNativeCodeF(0, "Append failed - the list was null");
			return;
		}
	}

	VM_CALLBACK(NodeAppendInterface)
	{
		auto src = (InterfacePointer)registers[VM::REGISTER_D7].vPtrValue;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		ListNode* newNode = AppendToNode(n, ss);

		*(InterfacePointer*)newNode->Element = src;

		ss.ProgramObject().IncrementRefCount(src);
	}

	VM_CALLBACK(NodeAppend64)
	{
		int64 src = registers[VM::REGISTER_D7].int64Value;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		ListNode* newNode = AppendToNode(n, ss);
		*((int64*)newNode->Element) = src;
	}

	VM_CALLBACK(ListPrepend)
	{
		void* src = registers[VM::REGISTER_D7].vPtrValue;
		ListImage* l = (ListImage*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		if (l)
		{
			ListNode* newNode = PrependToList(l, ss);
			AlignedMemcpy(newNode->Element, src, l->ElementSize);
		}
		else
		{
			ss.ThrowFromNativeCodeF(0, "Prepend failed - the list was null");
			return;
		}
	}

	VM_CALLBACK(NodePrepend)
	{
		void* src = registers[VM::REGISTER_D7].vPtrValue;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		ListNode* newNode = PrependToNode(n, ss);
		AlignedMemcpy(newNode->Element, src, (n->Container->ElementSize));
	}

	VM_CALLBACK(NodePrependInterface)
	{
		auto src = (InterfacePointer)registers[VM::REGISTER_D7].vPtrValue;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		ListNode* newNode = PrependToNode(n, ss);

		*(InterfacePointer*)newNode->Element = src;

		ss.ProgramObject().IncrementRefCount(src);
	}

	VM_CALLBACK(ListPrepend32)
	{
		int32 src = registers[VM::REGISTER_D7].int32Value;
		ListImage* l = (ListImage*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		if (l)
		{
			ListNode* newNode = PrependToList(l, ss);
			*((int32*)newNode->Element) = src;
		}
		else
		{
			ss.ThrowFromNativeCodeF(0, "Prepend failed - the list was null");
			return;
		}
	}

	VM_CALLBACK(NodePrepend32)
	{
		int32 src = registers[VM::REGISTER_D7].int32Value;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		ListNode* newNode = PrependToNode(n, ss);
		*((int32*)newNode->Element) = src;
	}

	VM_CALLBACK(ListPrependInterface)
	{
		auto src = (InterfacePointer)registers[VM::REGISTER_D7].vPtrValue;
		ListImage* l = (ListImage*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		if (l)
		{
			ListNode* newNode = PrependToList(l, ss);
			*(InterfacePointer*)newNode->Element = src;
			ss.ProgramObject().IncrementRefCount(src);
		}
		else
		{
			ss.ThrowFromNativeCodeF(0, "Prepend failed - the list was null");
			return;
		}
	}

	VM_CALLBACK(ListPrepend64)
	{
		int64 src = registers[VM::REGISTER_D7].int64Value;
		ListImage* l = (ListImage*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		if (l)
		{
			ListNode* newNode = PrependToList(l, ss);
			*((int64*)newNode->Element) = src;
		}
		else
		{
			ss.ThrowFromNativeCodeF(0, "Prepend failed - the list was null");
			return;
		}
	}

	VM_CALLBACK(NodePrepend64)
	{
		int64 src = registers[VM::REGISTER_D7].int64Value;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		ListNode* newNode = PrependToNode(n, ss);
		*((int64*)newNode->Element) = src;
	}

	VM_CALLBACK(NodePop)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D7].vPtrValue;
		ListImage* l = n->Container;

		if (n == NULL)
		{
			ss.ThrowFromNativeCodeF(-1, ("Node.Pop failed, as the node was null"));
			return;
		}

		if (l == NULL)
		{
			ss.ThrowFromNativeCodeF(-1, ("Node.Pop failed, as the node was not in a list"));
			return;
		}

		l->NumberOfElements--;

		if (l->NumberOfElements == 0)
		{
			l->Head = l->Tail = NULL;
		}
		else
		{
			if (n->Previous != NULL) 	n->Previous->Next = n->Next;
			else						l->Head = n->Next;

			if (n->Next != NULL)		n->Next->Previous = n->Previous;
			else						l->Tail = n->Previous;
		}

		n->Previous = NULL;
		n->Next = NULL;
		n->Container = NULL;

		if (n->RefCount != 1)
		{
			int refCount = ReleaseNode(n, ss);
			UNUSED(refCount);
		}
		else
		{
			if (l->OrphanedNodeList == NULL)
			{
				l->OrphanedNodeList = new OrphanedNodeList();
			}
			
			l->OrphanedNodeList->items.push_back(n);
		}
	}

	VM_CALLBACK(NodeEnumNext)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D12].vPtrValue;
		ListNode* next = n->Next;

		ReleaseNode(n, ss);

		if (next != NULL)
		{
			AddRef(next);
		}

		registers[VM::REGISTER_D12].vPtrValue = next;
	}

	VM_CALLBACK(NodeHasNext)
	{
		ListNode* n = (ListNode*)registers[VM::REGISTER_D7].vPtrValue;
		registers[VM::REGISTER_D12].int32Value = (int32)(n->Next != NULL);
	}

	VM_CALLBACK(NodeHasPrevious)
	{
		ListNode* n = (ListNode*)registers[VM::REGISTER_D7].vPtrValue;
		registers[VM::REGISTER_D7].int32Value = (int32)(n->Previous != NULL);
	}

	VM_CALLBACK(ListGetHead)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		ListImage* l = (ListImage*)registers[VM::REGISTER_D7].vPtrValue;

		if (!l || !l->Head)
		{
			ss.ThrowFromNativeCodeF(-1, "ListGetHead: The list was empty");
			return;
		}

		AddRef(l->Head);

		registers[VM::REGISTER_D7].vPtrValue = l->Head;
	}

	VM_CALLBACK(ListGetHeadUnreferenced)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		ListImage* l = (ListImage*)registers[VM::REGISTER_D7].vPtrValue;

		if (!l || !l->Head)
		{
			ss.ThrowFromNativeCodeF(-1, "ListGetHead: The list was empty");
			return;
		}

		registers[VM::REGISTER_D7].vPtrValue = l->Head;
	}

	VM_CALLBACK(ListGetLength)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		ListImage* l = (ListImage*)registers[VM::REGISTER_D7].vPtrValue;
		registers[VM::REGISTER_D7].int32Value = l ? l->NumberOfElements : 0;
	}

	VM_CALLBACK(ListGetTail)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;

		ListImage* l = (ListImage*)registers[VM::REGISTER_D7].vPtrValue;

		if (!l || !l->Tail)
		{
			ss.ThrowFromNativeCodeF(-1, "ListGetTail: The list was empty");
			return;
		}

		AddRef(l->Tail);

		registers[VM::REGISTER_D7].vPtrValue = l->Tail;
	}

	VM_CALLBACK(NodeReleaseRef)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D7].vPtrValue;
		ReleaseNode(n, ss);
	}

	VM_CALLBACK(NodeGet32)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;

		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		registers[VM::REGISTER_D7].int32Value = *(int32*)n->Element;
	}

	VM_CALLBACK(NodeGet64)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		registers[VM::REGISTER_D7].int64Value = *(int64*)n->Element;
	}

	VM_CALLBACK(NodeGetInterface)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D4].vPtrValue;
		auto pInterface = *(InterfacePointer*)n->Element;
		ss.ProgramObject().IncrementRefCount(pInterface);
		registers[VM::REGISTER_D7].vPtrValue = pInterface;
	}

	VM_CALLBACK(NodeGetElementRef)
	{
		ListNode* n = (ListNode*)registers[VM::REGISTER_D7].vPtrValue;
		registers[VM::REGISTER_D7].vPtrValue = n->Element;
	}

	VM_CALLBACK(NodeNext)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D7].vPtrValue;
		if (n->Next == NULL)
		{
			ss.ThrowFromNativeCodeF(-1, ("The node had no successor"));
			return;
		}

		AddRef(n->Next);

		registers[VM::REGISTER_D7].vPtrValue = n->Next;
	}

	VM_CALLBACK(NodePrevious)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		ListNode* n = (ListNode*)registers[VM::REGISTER_D7].vPtrValue;
		if (n->Previous == nullptr)
		{
			ss.ThrowFromNativeCodeF(-1, "The node had no predecessor");
			return;
		}

		AddRef(n->Previous);

		registers[VM::REGISTER_D7].vPtrValue = n->Previous;
	}

	VM_CALLBACK(ListAppendAndGetRef)
	{
		ListImage* l = (ListImage*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		if (l)
		{
			ListNode* newNode = AppendToList(l, ss);
			registers[VM::REGISTER_D7].vPtrValue = newNode->Element;
		}
		else
		{
			ss.ThrowFromNativeCodeF(-1, "ListAppendAndGetRef: null list");
			return;
		}
	}

	VM_CALLBACK(ListPrependAndGetRef)
	{
		ListImage* l = (ListImage*)registers[VM::REGISTER_D4].vPtrValue;
		IScriptSystem& ss = *(IScriptSystem*)context;

		if (l)
		{
			ListNode* newNode = PrependToList(l, ss);
			registers[VM::REGISTER_D7].vPtrValue = newNode->Element;
		}
		else
		{
			ss.ThrowFromNativeCodeF(-1, "ListPrependAndGetRef: null list");
			return;
		}
	}

	void CompileAsClearList(CCompileEnvironment& ce, cr_sex s, cstr instanceName)
	{
		ce.Builder.AssignVariableToTemp(instanceName, Rococo::ROOT_TEMPDEPTH);
		AppendInvoke(ce, GetListCallbacks(ce).ListClear, s);
	}

	void CompileConstructListElementFromRef(CCompileEnvironment& ce, cr_sex s, const IStructure& elementType, cstr instance)
	{
		const IFunction* constructor = elementType.Constructor();

		// (<list-instance>.Append <constructorArg1> ... <constructorArgN>)
		int inputCount = s.NumberOfElements() - 1;

		if (inputCount == 1 && IsNull(s.GetElement(1))) inputCount = 0;

		if (inputCount < constructor->NumberOfInputs() - 1)
		{
			Throw(s, ("Too few arguments in push constructor call"));
		}
		else if (inputCount > constructor->NumberOfInputs() - 1)
		{
			Throw(s, ("Too many arguments in push constructor call"));
		}

		int inputStackAllocCount = PushInputs(ce, s, *constructor, true, 1);

		ce.Builder.AssignVariableToTemp(instance, 0, 0); // list goes to D4
		ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).ListAppendAndGetRef); // This returns the address of the blank slot in D7

		inputStackAllocCount += CompileInstancePointerArgFromTemp(ce, Rococo::ROOT_TEMPDEPTH);

		AppendFunctionCallAssembly(ce, *constructor);

		ce.Builder.MarkExpression(&s);

		RepairStack(ce, *s.Parent(), *constructor);
		ce.Builder.AssignClosureParentSFtoD6();
	}

	void CompileAsAppendToList(CCompileEnvironment& ce, cr_sex s, cstr instanceName, bool toHead)
	{
		const IStructure& elementType = GetListDef(ce, s, instanceName);

		AssertNotTooFewElements(s, 2);
		AssertNotTooManyElements(s, 2);

		cr_sex value = s.GetElement(1);

		const ListCallbacks& callbacks = GetListCallbacks(ce);

		if (elementType.InterfaceCount() > 0)
		{
			if (IsStringLiteral(value) && elementType == ce.Object.Common().SysTypeIString().NullObjectType())
			{
				// Elements are IStrings, so we can assign by string literal
				auto literalValue = value.String();
				auto strConst = ce.SS.ReflectTransientStringByDuplication(literalValue->Buffer, literalValue->Length);

				VariantValue v;
				v.vPtrValue = strConst->header.AddressOfVTable0();
				ce.Builder.Assembler().Append_SetRegisterImmediate(REGISTER_D7, v, BITCOUNT_POINTER);
				ce.Builder.AssignVariableRefToTemp(instanceName, 0, 0); // list goes to 4
				ce.Builder.Assembler().Append_Invoke(toHead ? callbacks.ListPrependInterface : callbacks.ListAppendInterface);
				return;
			}

			cr_sex arg = GetAtomicArg(s, 1);
			AssertLocalVariableOrMember(arg);
			ce.Builder.AssignVariableToTemp(arg.c_str(), 3);
			ce.Builder.AssignVariableRefToTemp(instanceName, 0, 0); // list goes to 4
			ce.Builder.Assembler().Append_Invoke(toHead ? callbacks.ListPrependInterface : callbacks.ListAppendInterface);
		}
		else if (elementType.VarType() == SexyVarType_Derivative)
		{
			const IFunction* constructor = elementType.Constructor();
			if (constructor != NULL)
			{
				CompileConstructListElementFromRef(ce, s, elementType, instanceName);
			}
			else
			{
				// No constructor, so need to copy element
				CompileGetStructRef(ce, value, elementType, "newListElement"); // The address of the value is in D7

				auto variable = GetAtomicArg(value);
				ce.Builder.Append_IncDerivativeRefs(variable.buffer);
				ce.Builder.AssignVariableToTemp(instanceName, 0, 0); // list goes to 4
				ce.Builder.Assembler().Append_Invoke(toHead ? callbacks.ListPrepend : callbacks.ListAppend);
			}
		}
		else
		{
			if (elementType.VarType() == SexyVarType_Closure)
			{
				if (!TryCompileAssignArchetype(ce, value, elementType, false))
				{
					Throw(value, "Could not evaluate the expression as type %s", GetTypeName(elementType.VarType()));
				}
			}
			else if (!TryCompileArithmeticExpression(ce, value, true, elementType.VarType()))
			{
				Throw(value, "Could not evaluate the expression as type %s", GetTypeName(elementType.VarType()));
			} // The value is in D7

			ce.Builder.AssignVariableToTemp(instanceName, 0, 0); // list goes to D4

			switch (elementType.VarType())
			{
			case SexyVarType_Bool:
			case SexyVarType_Int32:
			case SexyVarType_Float32:
				ce.Builder.Assembler().Append_Invoke(toHead ? callbacks.ListPrepend32 : callbacks.ListAppend32);
				break;
			case SexyVarType_Int64:
			case SexyVarType_Float64:
				ce.Builder.Assembler().Append_Invoke(toHead ? callbacks.ListPrepend64 : callbacks.ListAppend64);
				break;
			case SexyVarType_Closure:
				ce.Builder.Assembler().Append_Invoke(toHead ? callbacks.ListPrepend64 : callbacks.ListAppend64);
				break;
			default:
				Throw(value, ("Unhandled value element type"));
			}
		}
	}

	void CompileAsAppendToNode(CCompileEnvironment& ce, cr_sex s, cstr instanceName, bool toHead)
	{
		const IStructure& elementType = GetNodeDef(ce, s, instanceName);

		AssertNotTooFewElements(s, 2);
		AssertNotTooManyElements(s, 2);

		cr_sex value = s.GetElement(1);

		const ListCallbacks& callbacks = GetListCallbacks(ce);

		if (elementType.InterfaceCount() > 0)
		{
			AssertLocalVariableOrMember(value);
			ce.Builder.AssignVariableToTemp(value.c_str(), 3); // value to D7
			ce.Builder.AssignVariableRefToTemp(instanceName, 0, 0); // node goes to D4
			ce.Builder.Assembler().Append_Invoke(toHead ? callbacks.NodePrependInterface : callbacks.NodeAppendInterface);
		}
		else if (elementType.VarType() == SexyVarType_Derivative)
		{
			const IFunction* constructor = elementType.Constructor();
			if (constructor != NULL)
			{
				Throw(s, ("Construction not supported"));
			}
			else
			{
				// No constructor, so need to copy element
				Throw(s, ("Construction not supported"));
			}
		}
		else
		{
			if (elementType.VarType() == SexyVarType_Closure)
			{
				if (!TryCompileAssignArchetype(ce, value, elementType, false))
				{
					Throw(value, "Could not evaluate the expression as type %s", GetTypeName(elementType.VarType()));
				}
			}
			else if (!TryCompileArithmeticExpression(ce, value, true, elementType.VarType()))
			{
				Throw(value, "Could not evaluate the expression as type %s", GetTypeName(elementType.VarType()));
			} // The value is in D7

			ce.Builder.AssignVariableRefToTemp(instanceName, 0, 0); // node goes to D4

			switch (elementType.VarType())
			{
			case SexyVarType_Bool:
			case SexyVarType_Int32:
			case SexyVarType_Float32:
				ce.Builder.Assembler().Append_Invoke(toHead ? callbacks.NodePrepend32 : callbacks.NodeAppend32);
				break;
			case SexyVarType_Int64:
			case SexyVarType_Float64:
				ce.Builder.Assembler().Append_Invoke(toHead ? callbacks.NodePrepend64 : callbacks.NodeAppend64);
				break;
			case SexyVarType_Derivative:
				ce.Builder.Assembler().Append_Invoke(toHead ? callbacks.NodePrepend : callbacks.NodeAppend);
				break;
			default:
				Throw(value, "Unhandled value element type");
			}
		}
	}

	void CompileAsPopNode(CCompileEnvironment& ce, cr_sex s, cstr instanceName)
	{
		AssertNotTooFewElements(s, 1);
		AssertNotTooManyElements(s, 1);

		ce.Builder.AssignVariableRefToTemp(instanceName, Rococo::ROOT_TEMPDEPTH, 0); // node goes to D7

		AppendInvoke(ce, GetListCallbacks(ce).NodePop, s);
	}

	bool TryCompileAsListCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName)
	{
		if (AreEqual(methodName, ("Append")))
		{
			CompileAsAppendToList(ce, s, instanceName, false);
			return true;
		}
		if (AreEqual(methodName, ("Clear")))
		{
			CompileAsClearList(ce, s, instanceName);
			return true;
		}
		else if (AreEqual(methodName, ("Prepend")))
		{
			CompileAsAppendToList(ce, s, instanceName, true);
			return true;
		}
		else
		{
			Throw(s, ("Unknown list method. Known methods: Append, Clear, Prepend."));
		}
	}

	bool TryCompileAsNodeCall(CCompileEnvironment& ce, cr_sex s, cstr instanceName, cstr methodName)
	{
		if (AreEqual(methodName, "Append"))
		{
			CompileAsAppendToNode(ce, s, instanceName, false);
			return true;
		}
		else if (AreEqual(methodName, "Prepend"))
		{
			CompileAsAppendToNode(ce, s, instanceName, true);
			return true;
		}
		else if (AreEqual(methodName, "Pop"))
		{
			CompileAsPopNode(ce, s, instanceName);
			return true;
		}
		else
		{
			Throw(s, ("Unknown node method. Known methods: Append, Prepend."));
		}
	}

	void CompileAsListNodeDeclaration(CCompileEnvironment& ce, cstr nodeName, cr_sex source)
	{
		// Assumes s has 4 elements and third is '=' and source is atomic
		// (Node <name> = <listName>.<listDirective>)
		AddVariableRef(ce, NameString::From(nodeName), ce.Object.Common().TypeNode());

		cstr sourceCommand = source.c_str();

		NamespaceSplitter splitter(sourceCommand);

		cstr srcName, listDirective;
		if (!splitter.SplitTail(srcName, listDirective))
		{
			Throw(source, ("Expecting <source-name>.<property> There was no dot operator '.' in the atomic expression"));
		}

		VariantValue v;
		v.vPtrValue = nullptr;
		ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, v, BITCOUNT_POINTER);
		AssignTempToVariableRef(ce, Rococo::ROOT_TEMPDEPTH, nodeName);

		if (*ce.Builder.GetVarStructure(srcName) == ce.StructList())
		{
			AddNodeDef(ce.Script, ce.Builder, nodeName, GetListDef(ce, source, srcName), source);

			ce.Builder.AddSymbol(srcName);
			ce.Builder.AssignVariableToTemp(srcName, Rococo::ROOT_TEMPDEPTH); // A pointer to the list is now in D7

			if (AreEqual(listDirective, ("Tail")))
			{
				ce.Builder.AddSymbol(("tail now in D7"));
				AppendInvoke(ce, GetListCallbacks(ce).ListGetTail, source); // tail is now in D7
				AssignTempToVariableRef(ce, Rococo::ROOT_TEMPDEPTH, nodeName); // node now points to the tail
			}
			else if (AreEqual(listDirective, ("Head")))
			{
				ce.Builder.AddSymbol(("head now in D7"));
				AppendInvoke(ce, GetListCallbacks(ce).ListGetHead, source); // head is now in D7
				AssignTempToVariableRef(ce, Rococo::ROOT_TEMPDEPTH, nodeName); // node now points to the head
			}
			else
			{
				Throw(source, ("Unrecognized node accessor. Recognized accessors: Head, Tail"));
			}
		}
		else if (*ce.Builder.GetVarStructure(srcName) == ce.Object.Common().TypeNode())
		{
			AddNodeDef(ce.Script, ce.Builder, nodeName, GetNodeDef(ce, source, srcName), source);

			ce.Builder.AddSymbol(srcName);
			ce.Builder.AssignVariableRefToTemp(srcName, Rococo::ROOT_TEMPDEPTH); // A pointer to the node is now in D7

			if (AreEqual(listDirective, ("Next")))
			{
				ce.Builder.AddSymbol(("tail now in D7"));
				AppendInvoke(ce, GetListCallbacks(ce).NodeNext, source); // tail is now in D7
				AssignTempToVariableRef(ce, Rococo::ROOT_TEMPDEPTH, nodeName); // node now points to the tail
			}
			else if (AreEqual(listDirective, ("Previous")))
			{
				ce.Builder.AddSymbol(("head now in D7"));
				AppendInvoke(ce, GetListCallbacks(ce).NodePrevious, source); // tail is now in D7
				AssignTempToVariableRef(ce, Rococo::ROOT_TEMPDEPTH, nodeName); // node now points to the tail
			}
			else
			{
				Throw(source, ("Unrecognized node accessor. Recognized accessors: Next, Previous"));
			}
		}
		else
		{
			Throw(source, ("Expecting <source-name>.<property>, where <source-name> is a list or node variable."));
		}
	}

	void CompileListDeclaration(CCompileEnvironment& ce, cr_sex s)
	{
		// (array <element-type-name> <array-name>
		// Example (list Int32 a) creates a doubly-linked list named 'a' with elements of type Int32
		// Example (list Int32 a *) creates a reference to doubly-linked lists 'a' and initializes it to null
		AssertNotTooFewElements(s, 3);
		AssertNotTooManyElements(s, 4);

		cr_sex typeName = GetAtomicArg(s, 1);
		cr_sex listNameExpr = GetAtomicArg(s, 2);

		if (s.NumberOfElements() == 4)
		{
			cr_sex qualifier = GetAtomicArg(s, 3);
			if (!Eq("*", qualifier.c_str()))
			{
				Throw(s[3], "Expecting * at this position");
			}
		}

		cstr listName = listNameExpr.c_str();

		AssertLocalIdentifier(listNameExpr);

		const IStructure& listStruct = ce.StructList();

		const IStructure* elementStruct = MatchStructure(typeName, ce.Builder.Module());
		if (elementStruct == NULL) ThrowTokenNotFound(s, typeName.c_str(), ce.Builder.Module().Name(), "type");

		ce.Builder.AddSymbol(listName);
		AddVariableRef(ce, NameString::From(listName), ce.StructList());

		VariantValue v;
		v.vPtrValue = (void*)elementStruct;

		TokenBuffer symbol;
		StringPrint(symbol, "typeof(%s)", GetFriendlyName(*elementStruct));
		ce.Builder.AddSymbol(symbol);
		ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, v, BITCOUNT_POINTER); // Element type to D5

		AddListDef(ce.Script, ce.Builder, listName, *elementStruct, s);

		if (s.NumberOfElements() == 4)
		{
			VariantValue nullRef;
			nullRef.vPtrValue = nullptr;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, nullRef, BITCOUNT_POINTER);
		}
		else
		{
			ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).ListInit);
		}

		ce.Builder.AssignTempToVariable(0, listName); // D4 goes to the list variable
	}

	void CompileEnumerateList(CCompileEnvironment& ce, cr_sex s, int hashIndex)
	{
		// (foreach i v # l (...) (...) )
		// (foreach v # l (...) (...) )

		cr_sex collection = s.GetElement(hashIndex + 1);
		cstr collectionName = collection.c_str();
		AssertLocalVariableOrMember(collection);

		cstr indexName;
		cstr refName;

		if (hashIndex == 2)
		{
			indexName = NULL;
			cr_sex refExpr = s.GetElement(1);
			AssertLocalIdentifier(refExpr);
			refName = refExpr.c_str();
		}
		else
		{
			cr_sex indexVar = s.GetElement(1);
			indexName = indexVar.c_str();
			AssertLocalIdentifier(indexVar);
			cr_sex refExpr = s.GetElement(2);
			AssertLocalIdentifier(refExpr);
			refName = refExpr.c_str();

			AddVariable(ce, NameString::From(indexName), ce.Object.Common().TypeInt32());
		}

		ce.Builder.AddSymbol(("(foreach..."));

		ce.Builder.AddSymbol(collectionName);
		AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 6, Rococo::ROOT_TEMPDEPTH + 6, BITCOUNT_POINTER);
		ce.Builder.AssignVariableToTemp(collectionName, 9, 0);	// List ref is in D13

		//////////////////////////////////////////////////////// Test list to see if it is empty //////////////////////////////////////////////////////
		ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D13, VM::REGISTER_D7, BITCOUNT_POINTER);
		ce.Builder.Assembler().Append_Invoke(GetListCallbacks(ce).ListGetLength);
		ce.Builder.Assembler().Append_Test(VM::REGISTER_D7, BITCOUNT_32);

		AddSymbol(ce.Builder, "if (list.length > 0) skip next two branches");
		size_t firstIterationJumpPos = ce.Builder.Assembler().WritePosition();
		ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_NOT_EQUAL, sizeof(ArgsBranchIf) + 3 * (sizeof(int32) + 1)); // takes us just beyond the next two branch statements
		size_t bailoutPos = ce.Builder.Assembler().WritePosition();
		AddSymbol(ce.Builder, "branch to abort point");
		ce.Builder.Assembler().Append_Branch(0); // if Eq, means counter = 0, and we branch here to the end

		size_t rearBreakPos = ce.Builder.Assembler().WritePosition();
		AddSymbol(ce.Builder, "branch to break point");
		ce.Builder.Assembler().Append_Branch(0); // if Eq, means counter = 0, and we we branch here to the end, this is where (break) is directed

		AddSymbol(ce.Builder, "branch to continue point");
		size_t continueRearPos = ce.Builder.Assembler().WritePosition(); // this is where (continue) is directed
		ce.Builder.Assembler().Append_Branch(0);


		size_t firstIterationJumpLength = ce.Builder.Assembler().WritePosition() - firstIterationJumpPos;
		//if (firstIterationJumpLength != sizeof(ArgsBranchIf) + 3 * (sizeof(int32) + 1)) Throw(0, "Unexpected %llu vs % llu", firstIterationJumpLength, sizeof(ArgsBranchIf) + 3 * (sizeof(int32) + 1));
		ce.Builder.Assembler().SetWriteModeToOverwrite(firstIterationJumpPos);
		ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_NOT_EQUAL, (int)firstIterationJumpLength); // takes us just beyond the next two branch statements
		ce.Builder.Assembler().SetWriteModeToAppend();

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 5, Rococo::ROOT_TEMPDEPTH + 5, BITCOUNT_POINTER); // D12 to be used as node pointer
		ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D13, VM::REGISTER_D7, BITCOUNT_POINTER);
		AppendInvoke(ce, GetListCallbacks(ce).ListGetHead, collection);
		ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D12, BITCOUNT_POINTER); // D12 is current node pointer

		if (indexName != NULL)
		{
			ce.Builder.AddSymbol(("D11 - working index"));
			AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 4, Rococo::ROOT_TEMPDEPTH + 4, BITCOUNT_32); // Index is D11
			VariantValue zero;
			zero.int32Value = 0;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D11, zero, BITCOUNT_32); // init index to zero

			// We may be nested in a function that overwrites D10, which is used as the result of D11-1
			AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 3, Rococo::ROOT_TEMPDEPTH + 3, BITCOUNT_POINTER);
		}

		AddVariableRef(ce, NameString::From(refName), ce.Object.Common().TypeNode());

		AddNodeDef(ce.Script, ce.Builder, refName, GetListDef(ce, s, collectionName), s);

		ptrdiff_t startLoop = ce.Builder.Assembler().WritePosition();

		AssignTempToVariableRef(ce, 8, refName); // refName now gives us the head node

		if (indexName != NULL)
		{
			// Init our index var to the current index value then increment the index value
			ce.Builder.AssignTempToVariable(7, indexName);

			VariantValue one;
			one.int32Value = 1;
			ce.Builder.Assembler().Append_AddImmediate(VM::REGISTER_D11, BITCOUNT_32, VM::REGISTER_D11, one);
		}

		ControlFlowData cfd;
		cfd.ContinuePosition = continueRearPos;
		cfd.BreakPosition = rearBreakPos;
		ce.Builder.PushControlFlowPoint(cfd);
		CompileExpressionSequence(ce, hashIndex + 2, s.NumberOfElements() - 1, s);
		ce.Builder.PopControlFlowPoint();

		size_t continueForwardPos = ce.Builder.Assembler().WritePosition();
		size_t rearToForward = continueForwardPos - continueRearPos;

		ce.Builder.Assembler().SetWriteModeToOverwrite(continueRearPos);
		ce.Builder.Assembler().Append_Branch((int32)rearToForward);
		ce.Builder.Assembler().SetWriteModeToAppend();

		AppendInvoke(ce, GetListCallbacks(ce).NodeEnumNext, s); // Gives the raw node next value from D12 to D12

		ce.Builder.Assembler().Append_Test(VM::REGISTER_D12, BITCOUNT_POINTER);

		ptrdiff_t endLoop = ce.Builder.Assembler().WritePosition();
		ce.Builder.Assembler().Append_BranchIf(Rococo::CONDITION_IF_NOT_EQUAL, (int32)(startLoop - endLoop));
		size_t branchToEverythingElsePos = ce.Builder.Assembler().WritePosition();
		ce.Builder.Assembler().Append_Branch(0);
		size_t breakPos = ce.Builder.Assembler().WritePosition();
		// We broke, which meant we never enumerated to the final position, so the last thing referenced has an outstanding reference count
		ce.Builder.AssignVariableRefToTemp(refName, Rococo::ROOT_TEMPDEPTH);
		AppendInvoke(ce, GetListCallbacks(ce).NodeReleaseRef, s); // release the ref to the node

		size_t popEverythingElsePos = ce.Builder.Assembler().WritePosition();
		size_t everythingElseDelta = popEverythingElsePos - branchToEverythingElsePos;
		ce.Builder.Assembler().SetWriteModeToOverwrite(branchToEverythingElsePos);
		ce.Builder.Assembler().Append_Branch((int)everythingElseDelta);
		ce.Builder.Assembler().SetWriteModeToAppend();

		ce.Builder.PopLastVariables(indexName != NULL ? 4 : 2, true); // Release the D10,D11,D12 and the ref or just D12 and the ref
		size_t exitPos = ce.Builder.Assembler().WritePosition();
		size_t bailoutToExit = exitPos - bailoutPos;
		ce.Builder.Assembler().SetWriteModeToOverwrite(bailoutPos);
		ce.Builder.Assembler().Append_Branch((int32)bailoutToExit);

		size_t breakToExit = breakPos - rearBreakPos;
		ce.Builder.Assembler().SetWriteModeToOverwrite(rearBreakPos);
		ce.Builder.Assembler().Append_Branch((int32)breakToExit);
		ce.Builder.Assembler().SetWriteModeToAppend();

		ce.Builder.PopLastVariables(1, true);
	}

	void RegisterLists(ListCallbacks& listCallbacks, VM::ICore& core, IScriptSystem& ss)
	{
		listCallbacks.ListInit = core.RegisterCallback(OnInvokeListInit, &ss, "ListInit");
		listCallbacks.ListAppend = core.RegisterCallback(OnInvokeListAppend, &ss, "ListAppend");
		listCallbacks.ListAppendAndGetRef = core.RegisterCallback(OnInvokeListAppendAndGetRef, &ss, "ListAppendAndGetRef");
		listCallbacks.ListAppend32 = core.RegisterCallback(OnInvokeListAppend32, &ss, "ListAppend32");
		listCallbacks.ListAppend64 = core.RegisterCallback(OnInvokeListAppend64, &ss, "ListAppend64");
		listCallbacks.ListAppendInterface = core.RegisterCallback(OnInvokeListAppendInterface, &ss, "ListAppendInterface");
		listCallbacks.ListPrepend = core.RegisterCallback(OnInvokeListPrepend, &ss, "ListPrepend");
		listCallbacks.ListPrependAndGetRef = core.RegisterCallback(OnInvokeListPrependAndGetRef, &ss, "ListPrependAndGetRef");
		listCallbacks.ListPrepend32 = core.RegisterCallback(OnInvokeListPrepend32, &ss, "ListPrepend32");
		listCallbacks.ListPrepend64 = core.RegisterCallback(OnInvokeListPrepend64, &ss, "ListPrepend64");
		listCallbacks.ListPrependInterface = core.RegisterCallback(OnInvokeListPrependInterface, &ss, "ListPrependInterface");
		listCallbacks.ListGetHead = core.RegisterCallback(OnInvokeListGetHead, &ss, "ListGetHead");
		listCallbacks.ListGetHeadUnreferenced = core.RegisterCallback(OnInvokeListGetHeadUnreferenced, &ss, "ListGetHeadUnreferenced");
		listCallbacks.ListGetTail = core.RegisterCallback(OnInvokeListGetTail, &ss, "ListGetTail");
		listCallbacks.ListGetLength = core.RegisterCallback(OnInvokeListGetLength, &ss, "ListGetLength");
		listCallbacks.NodeGet32 = core.RegisterCallback(OnInvokeNodeGet32, &ss, "NodeGet32");
		listCallbacks.NodeGet64 = core.RegisterCallback(OnInvokeNodeGet64, &ss, "NodeGet64");
		listCallbacks.NodeGetInterface = core.RegisterCallback(OnInvokeNodeGetInterface, &ss, "NodeGetInterface");
		listCallbacks.NodeGetElementRef = core.RegisterCallback(OnInvokeNodeGetElementRef, &ss, "NodeGetElementRef");
		listCallbacks.NodeNext = core.RegisterCallback(OnInvokeNodeNext, &ss, "NodeNext");
		listCallbacks.NodePrevious = core.RegisterCallback(OnInvokeNodePrevious, &ss, "NodePrevious");
		listCallbacks.NodeAppend = core.RegisterCallback(OnInvokeNodeAppend, &ss, "NodeAppend");
		listCallbacks.NodeAppendInterface = core.RegisterCallback(OnInvokeNodeAppendInterface, &ss, "NodeAppendInterface");
		listCallbacks.NodeAppend32 = core.RegisterCallback(OnInvokeNodeAppend32, &ss, "NodeAppend32");
		listCallbacks.NodeAppend64 = core.RegisterCallback(OnInvokeNodeAppend64, &ss, "NodeAppend64");
		listCallbacks.NodePrepend = core.RegisterCallback(OnInvokeNodePrepend, &ss, "NodePrepend");
		listCallbacks.NodePrependInterface = core.RegisterCallback(OnInvokeNodePrependInterface, &ss, "NodePrependInterface");
		listCallbacks.NodePrepend32 = core.RegisterCallback(OnInvokeNodePrepend32, &ss, "NodePrepend32");
		listCallbacks.NodePrepend64 = core.RegisterCallback(OnInvokeNodePrepend64, &ss, "NodePrepend64");
		listCallbacks.NodePop = core.RegisterCallback(OnInvokeNodePop, &ss, "NodePop");
		listCallbacks.NodeEnumNext = core.RegisterCallback(OnInvokeNodeEnumNext, &ss, "NodeEnumNext");
		listCallbacks.NodeHasNext = core.RegisterCallback(OnInvokeNodeHasNext, &ss, "NodeHasNext");
		listCallbacks.NodeHasPrevious = core.RegisterCallback(OnInvokeNodeHasPrevious, &ss, "NodeHasPrevious");
		listCallbacks.NodeReleaseRef = core.RegisterCallback(OnInvokeNodeReleaseRef, &ss, "NodeReleaseRef");
		listCallbacks.ListRelease = core.RegisterCallback(OnInvokeListRelease, &ss, "ListRelease");
		listCallbacks.ListClear = core.RegisterCallback(OnInvokeListClear, &ss, "ListClear");
		listCallbacks.ListAssign = core.RegisterCallback(OnInvokeListAssign, &ss, "ListAssign");
		listCallbacks.NodeGoPrevious = core.RegisterCallback(OnInvokeNodeGoPrevious, &ss, "NodeGoPrevious");
		listCallbacks.NodeGoNext = core.RegisterCallback(OnInvokeNodeGoNext, &ss, "NodeGoNext");
	}
}// Rococo::Script