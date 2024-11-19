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

#include <rococo.sexy.map.expert.h>

namespace Rococo::Script
{
	bool HasClassMembers(const IStructure& s);
	void InitSubmembers(CCompileEnvironment& ce, const IStructure& s, int sfMemberOffset);

	inline TMapNodes& GetRow(int hashcode, MapImage& img)
	{
		int32 i = (hashcode & 0x7FFFFFFF) % img.rows.size();
		return img.rows[i];
	}

	void ExpandRows(MapImage& m)
	{
		int32 currentRowCount = (int32)m.rows.size();
		if (m.NumberOfElements >= currentRowCount / 2)
		{
			m.rows.clear();
			if (currentRowCount < 8)
			{
				m.rows.resize(8);
			}
			else
			{
				m.rows.resize(currentRowCount * 2);
			}

			currentRowCount = (int32)m.rows.size();

			for (MapNode* i = m.Head; i != NULL; i = i->Next)
			{
				TMapNodes& row = GetRow(i->HashCode, m);
				row.push_back(i);
			}
		}
	}

	size_t GetAlignmentPadding(int alignment, int objectSize)
	{
		int32 x = (alignment - objectSize % alignment) % alignment;
		return (size_t)x;
	}

	SCRIPTEXPORT_API uint8* GetKeyPointer(MapNode* m)
	{
		size_t firstpadding = GetAlignmentPadding(16, sizeof(MapNode));
		return ((uint8*)m) + sizeof(MapNode) + firstpadding;
	}

	size_t GetKeySize(MapImage* m)
	{
		if (m->KeyType->VarType() == VARTYPE_Derivative)
		{
			return sizeof(size_t);
		}
		else
		{
			return m->KeyType->SizeOfStruct();
		}
	}

	SCRIPTEXPORT_API uint8* GetValuePointer(MapNode* m)
	{
		size_t keySize = GetKeySize(m->Container);
		size_t firstpadding = GetAlignmentPadding(16, sizeof(MapNode));
		size_t secondpadding = GetAlignmentPadding(16, (int)keySize);
		size_t offset_to_value = sizeof(MapNode) + firstpadding + keySize + secondpadding;
		return ((uint8*)m) + sizeof(MapNode) + firstpadding + keySize + secondpadding;
	}

	void FreeNode(MapNode* n, IScriptSystem& ss)
	{
		if (n->IsExistant)
		{
			//   DestroyObject(*n->Container->KeyType, (uint8*)GetKeyPointer(n), ss);
			DestroyObject(*n->Container->ValueType, (uint8*)GetValuePointer(n), ss);
		}
		ss.AlignedFree(n);
	}

	int ReleaseNode(MapNode* n, IScriptSystem& ss)
	{
		n->RefCount--;
		int refcount = n->RefCount;
		if (n->RefCount == 0) FreeNode(n, ss);
		return refcount;
	}

	size_t GetMemberSize(const IStructure& type)
	{
		size_t nBytes = type.InterfaceCount() > 0 ? sizeof(size_t) : type.SizeOfStruct();
		return nBytes;
	}

	size_t GetValueSize(MapImage* m)
	{
		return GetMemberSize(*m->ValueType);
	}

	MapNode* CreateMapNode(MapImage* m, IScriptSystem& ss)
	{
		size_t keysize = GetKeySize(m);
		size_t firstpadding = GetAlignmentPadding(16, sizeof(MapNode));
		size_t secondpadding = GetAlignmentPadding(16, (int)keysize);
		size_t allocSize = sizeof(MapNode) + firstpadding + keysize + secondpadding + GetValueSize(m);
		MapNode* n = (MapNode*)ss.AlignedMalloc(16, (int32)allocSize);
		n->Container = m;
		n->Previous = NULL;
		n->Next = NULL;
		n->RefCount = 1;
		return n;
	}

	void MapClear(MapImage* m, IScriptSystem& ss)
	{
		MapNode* i = m->Head;
		while (i != NULL)
		{
			MapNode* n = i;
			i = i->Next;
			ReleaseNode(n, ss);
		}

		ReleaseNode(m->NullNode, ss);

		m->~MapImage();
		ss.AlignedFree(m);
	}

	void ReleaseMap(MapImage* m, IScriptSystem& ss)
	{
		if (m)
		{
			m->refCount--;
			if (m->refCount <= 0)
			{
				MapClear(m, ss);
			}
		}
	}

	struct MapLifetimeManager : Rococo::Compiler::IMemberLifeSupervisor
	{
		IScriptSystem& ss;

		void Release(uint8* instance) override
		{
			MapImage* m = *reinterpret_cast<MapImage**>(instance);
			ReleaseMap(m, ss);
		}

		MapLifetimeManager(IScriptSystem& _ss) : ss(_ss)
		{

		}

		void Free() override
		{
			delete this;
		}
	};

	IMemberLifeSupervisor* CreateMapLifetimeManager(IScriptSystem& ss)
	{
		return new MapLifetimeManager(ss);
	}

	VM_CALLBACK(MapRelease)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapImage* m = (MapImage*)registers[VM::REGISTER_D7].vPtrValue;
		ReleaseMap(m, ss);
	}

	VM_CALLBACK(MapGetHead)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapImage* m = (MapImage*)registers[VM::REGISTER_D7].vPtrValue;
		MapNode* head = m->Head;
		if (head != NULL) head->AddRef();
		registers[VM::REGISTER_D7].vPtrValue = head;
	}

	VM_CALLBACK(MapGetLength)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapImage* m = (MapImage*)registers[VM::REGISTER_D13].vPtrValue;
		int32 length = m ? m->NumberOfElements : 0;
		registers[VM::REGISTER_D7].int32Value = length;
	}

	VM_CALLBACK(MapUpdateRefCounts)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapImage* m = (MapImage*)registers[VM::REGISTER_D4].vPtrValue;
		
		size_t offset = 0;
		auto* pValue = registers[VM::REGISTER_D7].uint8PtrValue; // The value of the last value pointer inserted into the map. Check InsertByRef
		for (int i = 0; i < m->ValueType->MemberCount(); ++i)
		{
			auto& member = m->ValueType->GetMember(i);
			uint8* pMemberVariable = pValue + offset;
			UpdateRefCounts(pMemberVariable, member);
			offset += member.SizeOfMember();
		}
	}

	VM_CALLBACK(MapNodeEnumNext)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapNode* n = (MapNode*)registers[VM::REGISTER_D12].vPtrValue;
		MapNode* next = n->Next;
		ReleaseNode(n, ss);
		if (next != NULL) next->AddRef();
		registers[VM::REGISTER_D12].vPtrValue = next;
	}

	MapNode* CreateExistantMapNode(MapImage& m, IScriptSystem& ss, int hashcode)
	{
		MapNode* newNode = CreateMapNode(&m, ss);
		newNode->IsExistant = 1;
		newNode->HashCode = hashcode;
		if (m.Tail != NULL)
		{
			m.Tail->Next = newNode;
		}
		else
		{
			// If no tail, then also no head, and vica versa
			m.Head = newNode;
		}

		newNode->Previous = m.Tail;
		m.Tail = newNode;
		m.NumberOfElements++;

		return newNode;
	}

	void DeleteNodeFromMap(MapNode* node, IScriptSystem& ss)
	{
		TMapNodes& row = GetRow(node->HashCode, *node->Container);

		for (auto i = row.begin(); i != row.end(); ++i)
		{
			MapNode* x = *i;
			if (x == node)
			{
				row.erase(i);
				break;
			}
		}

		MapImage* image = node->Container;

		MapNode* previous = node->Previous;
		MapNode* next = node->Next;

		node->Container->NumberOfElements--;
		ReleaseNode(node, ss);

		if (previous != NULL) previous->Next = next;
		if (next != NULL) next->Previous = previous;

		if (node == image->Head)
		{
			image->Head = next;
		}

		if (node == image->Tail)
		{
			image->Tail = previous;
		}
	}

	struct InlineStringKeyResolver : public IKeyResolver
	{
		void Delete(MapNode* node, IScriptSystem& ss) override
		{
			DeleteNodeFromMap(node, ss);
		}

		MapNode* InsertKey(VariantValue keySource, MapImage& m, IScriptSystem& ss) override
		{
			InterfacePointer key = (InterfacePointer)keySource.vPtrValue;
			InlineString* strKey = (InlineString*)InterfaceToInstance(key);

			if (*strKey->stub.Desc->TypeInfo != ss.ProgramObject().Common().TypeStringLiteral())
			{
				// Our string was not a StringConstant, ergo it might be mutable, so we have to copy to an immutable string and refer to that.
				auto* sc = ss.ReflectTransientStringByDuplication(strKey->buffer, strKey->length);
				strKey = (InlineString*)sc;
			}

			int hashcode = Rococo::Strings::HashArg(strKey->buffer, strKey->length);
			ExpandRows(m);

			TMapNodes& row = GetRow(hashcode, m);

			for (auto i = row.begin(); i != row.end(); ++i)
			{
				MapNode* x = *i;
				InlineString* s = *(InlineString**)GetKeyPointer(x);
				if (x->HashCode == hashcode && AreEqual(s->buffer, strKey->buffer))
				{
					return x;
				}
			}

			MapNode* newNode = CreateExistantMapNode(m, ss, hashcode);

			row.push_back(newNode);

			auto** writePtr = (InlineString**)GetKeyPointer(newNode);
			*writePtr = strKey;

			return newNode;
		}

		MapNode* FindItem(VariantValue keySource, MapImage& m)
		{
			if (m.NumberOfElements == 0) return m.NullNode;

			VirtualTable** key = (VirtualTable**)keySource.vPtrValue;
			InlineString* strKey = (InlineString*)(((uint8*)key) + (*key)->OffsetToInstance);
			int hashcode = Rococo::Strings::HashArg(strKey->buffer, strKey->length);

			TMapNodes& row = GetRow(hashcode, m);

			for (auto i = row.begin(); i != row.end(); ++i)
			{
				MapNode* x = *i;
				InlineString* s = *(InlineString**)GetKeyPointer(x);
				if (x->HashCode == hashcode && AreEqual(s->buffer, strKey->buffer))
				{
					return x;
				}
			}

			return m.NullNode;
		}
	};

	struct Bitcount32KeyResolver : public IKeyResolver
	{
		virtual void Delete(MapNode* node, IScriptSystem& ss)
		{
			DeleteNodeFromMap(node, ss);
		}

		MapNode* InsertKey(VariantValue keySource, MapImage& m, IScriptSystem& ss)
		{
			int32 key = keySource.int32Value;

			int hashcode = Rococo::Strings::HashArg(key);
			ExpandRows(m);

			TMapNodes& row = GetRow(hashcode, m);

			for (auto i = row.begin(); i != row.end(); ++i)
			{
				MapNode* x = *i;
				int32* s = (int32*)GetKeyPointer(x);
				if (*s == key)
				{
					return x;
				}
			}

			MapNode* newNode = CreateExistantMapNode(m, ss, hashcode);

			row.push_back(newNode);

			*(int32*)GetKeyPointer(newNode) = key;

			return newNode;
		}

		MapNode* FindItem(VariantValue keySource, MapImage& m)
		{
			if (m.NumberOfElements == 0) return m.NullNode;

			int32 key = keySource.int32Value;

			int hashcode = Rococo::Strings::HashArg(key);

			TMapNodes& row = GetRow(hashcode, m);

			for (auto i = row.begin(); i != row.end(); ++i)
			{
				MapNode* x = *i;
				int32* s = (int32*)GetKeyPointer(x);
				if (*s == key)
				{
					return x;
				}
			}

			return m.NullNode;
		}
	};

	struct Bitcount64KeyResolver : public IKeyResolver
	{
		virtual void Delete(MapNode* node, IScriptSystem& ss)
		{
			DeleteNodeFromMap(node, ss);
		}

		MapNode* InsertKey(VariantValue keySource, MapImage& m, IScriptSystem& ss)
		{
			int64 key = keySource.int64Value;

			int hashcode = Rococo::Strings::HashArg(key);
			ExpandRows(m);

			TMapNodes& row = GetRow(hashcode, m);

			for (auto i = row.begin(); i != row.end(); ++i)
			{
				MapNode* x = *i;
				int64* s = (int64*)GetKeyPointer(x);
				if (*s == key)
				{
					return x;
				}
			}

			MapNode* newNode = CreateExistantMapNode(m, ss, hashcode);

			row.push_back(newNode);

			*(int64*)GetKeyPointer(newNode) = key;

			return newNode;
		}

		MapNode* FindItem(VariantValue keySource, MapImage& m)
		{
			if (m.NumberOfElements == 0) return m.NullNode;

			int64 key = keySource.int64Value;

			int hashcode = Rococo::Strings::HashArg(key);

			TMapNodes& row = GetRow(hashcode, m);

			for (auto i = row.begin(); i != row.end(); ++i)
			{
				MapNode* x = *i;
				int64* s = (int64*)GetKeyPointer(x);
				if (*s == key)
				{
					return x;
				}
			}

			return m.NullNode;
		}
	};

	typedef void(*KEYTYPE_INIT)(MapImage* m);

	void InitInlineStringKeyResolver(MapImage* m)
	{
		static_assert(sizeof NullResolver >= sizeof InlineStringKeyResolver);
		new (&m->KeyResolver) InlineStringKeyResolver;
	}

	void InitBitcount32KeyResolver(MapImage* m)
	{
		static_assert(sizeof NullResolver >= sizeof Bitcount32KeyResolver);
		new (&m->KeyResolver) Bitcount32KeyResolver;
	}

	void InitBitcount64KeyResolver(MapImage* m)
	{
		static_assert(sizeof NullResolver >= sizeof Bitcount64KeyResolver);
		new (&m->KeyResolver) Bitcount64KeyResolver;
	}

	void InitializeMembersToNullObjectAtLocation(void* pValue, const IStructure& type, size_t offset)
	{
		for (auto i = 0; i < type.MemberCount(); ++i)
		{
			auto& m = type.GetMember(i);
			auto& mtype = *m.UnderlyingType();
			if (mtype.InterfaceCount() > 0)
			{
				pValue = mtype.GetInterface(0).UniversalNullInstance();
				offset += sizeof(size_t);
			}
			else if (mtype.VarType() == VARTYPE_Derivative)
			{
				InitializeMembersToNullObjectAtLocation(pValue, mtype, offset);
				offset += mtype.SizeOfStruct();
			}			
			else if (mtype.VarType() == VARTYPE_Array || mtype.VarType() == VARTYPE_List || mtype.VarType() == VARTYPE_Map)
			{
				memset(((uint8*)pValue) + offset, 0, sizeof(void*));
				offset += sizeof(void*);
			}
			else
			{
				memset(((uint8*)pValue) + offset, 0, mtype.SizeOfStruct());
				offset += mtype.SizeOfStruct();
			}
		}

#ifdef _DEBUG
		if (offset > type.SizeOfStruct())
		{
			Throw(0, "%s. Incorrect size estimation for %s. Offset was %llu. Size was %d", __FUNCTION__, GetFriendlyName(type), offset, type.SizeOfStruct());
		}
#endif
	}

	void InitializeNullObjectAtLocation(void* pValue, const IStructure& type)
	{
		if (type.InterfaceCount() > 0)
		{
			pValue = type.GetInterface(0).UniversalNullInstance();
		}
		else
		{
			if (type.VarType() == VARTYPE_Derivative)
			{
				InitializeMembersToNullObjectAtLocation(pValue, type, 0);
			}
			else
			{
				memset(pValue, 0, GetMemberSize(type));
			}
		}
	}

	MapImage* CreateMapImage(IScriptSystem& ss, const IStructure& keyType, const IStructure& valueType)
	{
		auto* p = ss.AlignedMalloc(16, sizeof MapImage);
		auto* m = new (p) MapImage;

		if (keyType == ss.ProgramObject().Common().SysTypeIString().NullObjectType())
		{
			InitInlineStringKeyResolver(m);
		}
		else  if (IsPrimitiveType(keyType.VarType()))
		{
			if (keyType.SizeOfStruct() == 4)
			{
				InitBitcount32KeyResolver(m);
			}
			else
			{
				InitBitcount64KeyResolver(m);
			}
		}
		else
		{
			ss.AlignedFree(p);
			ss.ThrowFromNativeCodeF(-1, "Derivative key type not implemented. Legal types: Int32, Int64, Float32, Float64, Bool, IString");
			return nullptr;
		}

		m->NumberOfElements = 0;
		m->reserved = 0;
		m->KeyType = &keyType;
		m->ValueType = &valueType;
		m->NullNode = CreateMapNode(m, ss);
		m->NullNode->IsExistant = 0;
		m->Head = m->Tail = NULL;
		m->refCount = 1;

		InitializeNullObjectAtLocation(GetValuePointer(m->NullNode), *m->ValueType);
		return m;
	}

	VM_CALLBACK(MapInit)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;

		const IStructure* keyType = (const IStructure*)registers[VM::REGISTER_D5].vPtrValue;
		const IStructure* valueType = (const IStructure*)registers[VM::REGISTER_D4].vPtrValue;

		MapImage* m = CreateMapImage(ss, *keyType, *valueType);
		registers[VM::REGISTER_D7].vPtrValue = m;

		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(-1, "Derivative key type not implemented. Legal types: Int32, Int64, Float32, Float64, Bool, IString");
			return;
		}
	}

	SCRIPTEXPORT_API MapNode* InsertKey(MapImage& theMap, VariantValue source, IScriptSystem& ss)
	{
		// Need to explicity cast to virtual, as OSX 'optimizes' by using direct function calls to NullObject in every case
		IKeyResolver& virtualResolver = static_cast<IKeyResolver&>(theMap.KeyResolver);
		MapNode* n = virtualResolver.InsertKey(source, theMap, ss);
		return n;
	}

	void IncrementRef(MapImage* mapImage)
	{
		if (mapImage) mapImage->refCount++;
	}

	VM_CALLBACK(MapInsert32)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapImage* m = (MapImage*)registers[VM::REGISTER_D4].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Insert failed. Map was null");
			return;
		}
		MapNode* n = InsertKey(*m, registers[VM::REGISTER_D8], ss);
		int32 value = registers[VM::REGISTER_D7].int32Value;
		*(int32*)GetValuePointer(n) = value;
	}

	VM_CALLBACK(MapInsert64)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapImage* m = (MapImage*)registers[VM::REGISTER_D4].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Insert failed. Map was null");
			return;
		}
		MapNode* n = InsertKey(*m, registers[VM::REGISTER_D8], ss);
		int64 value = registers[VM::REGISTER_D7].int64Value;
		*(int64*)GetValuePointer(n) = value;
	}

	VM_CALLBACK(MapInsertInterface)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapImage* m = (MapImage*)registers[VM::REGISTER_D4].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Insert failed. Map was null");
			return;
		}
		MapNode* n = InsertKey(*m, registers[VM::REGISTER_D8], ss);
		InterfacePointer value = (InterfacePointer)registers[VM::REGISTER_D7].vPtrValue;
		*(InterfacePointer*)GetValuePointer(n) = value;
		ss.ProgramObject().IncrementRefCount(value);
	}

	VM_CALLBACK(MapInsertValueByRef)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapImage* m = (MapImage*)registers[VM::REGISTER_D4].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Insert failed. Map was null");
			return;
		}
		MapNode* n = InsertKey(*m, registers[VM::REGISTER_D8], ss);
		uint8* valuePointer = GetValuePointer(n);
		const void* valueSrc = registers[VM::REGISTER_D7].vPtrValue;
		AlignedMemcpy(valuePointer, valueSrc, GetValueSize(m));
		registers[VM::REGISTER_D7].vPtrValue = valuePointer;
	}

	VM_CALLBACK(MapInsertAndGetRef)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapImage* m = (MapImage*)registers[VM::REGISTER_D4].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Insert failed. Map was null");
			return;
		}
		MapNode* n = InsertKey(*m, registers[VM::REGISTER_D8], ss);
		registers[VM::REGISTER_D7].vPtrValue = GetValuePointer(n);
	}

	VM_CALLBACK(MapTryGet)
	{
		MapImage* m = (MapImage*)registers[VM::REGISTER_D7].vPtrValue;
		if (m == nullptr)
		{
			IScriptSystem& ss = *(IScriptSystem*)context;
			ss.ThrowFromNativeCodeF(0, "TryGet failed. Map was null");
			return;
		}
		MapNode* node = static_cast<IKeyResolver&>(m->KeyResolver).FindItem(registers[VM::REGISTER_D8], *m);
		node->AddRef();
		registers[VM::REGISTER_D7].vPtrValue = node;
	}

	VM_CALLBACK(DoesMapNodeExist)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapNode* m = (MapNode*)registers[VM::REGISTER_D7].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Map Node-DoesMapNodeExist failed. Map was null");
			return;
		}

		registers[VM::REGISTER_D7].int32Value = m->IsExistant;
	}

	VM_CALLBACK(MapNodeGet32)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapNode* m = (MapNode*)registers[VM::REGISTER_D7].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Map Node-Get failed. Map was null");
			return;
		}
		registers[VM::REGISTER_D7].int32Value = *(int32*)GetValuePointer(m);
		if (!m->IsExistant) ss.ThrowFromNativeCodeF(-1, "MapNodeGet32 failed. The node did not represent an entry in the map");
	}

	VM_CALLBACK(MapNodeGet64)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapNode* m = (MapNode*)registers[VM::REGISTER_D7].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Map Node-Get failed. Map was null");
			return;
		}
		registers[VM::REGISTER_D7].int64Value = *(int64*)GetValuePointer(m);
		if (!m->IsExistant) ss.ThrowFromNativeCodeF(-1, "MapNodeGet64 failed. The node did not represent an entry in the map");
	}

	VM_CALLBACK(MapNodeGetInterface)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapNode* m = (MapNode*)registers[VM::REGISTER_D7].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Map Node-Get failed. Map was null");
			return;
		}

		if (!m->IsExistant)
		{
			ss.ThrowFromNativeCodeF(-1, "MapNodeGetInterface failed. The node did not represent an entry in the map");
			return;
		}

		InterfacePointer ip = *(InterfacePointer*)GetValuePointer(m);
		ObjectStub* stub = InterfaceToInstance(ip);
		if (stub->refCount != ObjectStub::NO_REF_COUNT)
		{
			stub->refCount++;
		}
		registers[VM::REGISTER_D7].vPtrValue = ip;
		
	}

	VM_CALLBACK(MapNodeGetKey32)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapNode* m = (MapNode*)registers[VM::REGISTER_D7].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Map Node-GetKey32 failed. node was null");
			return;
		}
		registers[VM::REGISTER_D7].int32Value = *(int32*)GetKeyPointer(m);
		if (!m->IsExistant) ss.ThrowFromNativeCodeF(-1, "MapNodeGetKey32 failed. The node did not represent an entry in the map");
	}

	VM_CALLBACK(MapNodeGetKey64)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapNode* m = (MapNode*)registers[VM::REGISTER_D7].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Map Node-GetKey64 failed. node was null");
			return;
		}
		registers[VM::REGISTER_D7].int64Value = *(int64*)GetKeyPointer(m);
		if (!m->IsExistant) ss.ThrowFromNativeCodeF(-1, "MapNodeGetKey64 failed. The node did not represent an entry in the map");
	}

	VM_CALLBACK(MapNodeGetKeyIString)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapNode* m = (MapNode*)registers[VM::REGISTER_D7].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "node.Key(IString) failed. node was null");
			return;
		}

		InlineString* pKey = *(InlineString**) GetKeyPointer(m);
		
		if (pKey->stub.refCount != ObjectStub::NO_REF_COUNT)
		{
			pKey->stub.refCount++;
		}
		
		if (!m->IsExistant)
		{
			ss.ThrowFromNativeCodeF(-1, "node.Key(IString) failed.The node did not represent an entry in the map");
			return;
		}
		
		registers[VM::REGISTER_D7].vPtrValue = pKey->stub.pVTables;		
	}

	VM_CALLBACK(MapNodeGetRef)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapNode* m = (MapNode*)registers[VM::REGISTER_D7].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Map Node-Get failed. Map was null");
			return;
		}

		registers[VM::REGISTER_D7].vPtrValue = GetValuePointer(m);

		if (!m->IsExistant)
		{
			ss.ThrowFromNativeCodeF(-1, "MapNodeGetRef failed. The node did not represent an entry in the map");
			return;
		}
	}

	VM_CALLBACK(MapNodePop)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapNode* m = (MapNode*)registers[VM::REGISTER_D7].vPtrValue;
		if (m == nullptr)
		{
			ss.ThrowFromNativeCodeF(0, "Map Node-Pop failed. Map was null");
			return;
		}

		if (!m->IsExistant)
		{
			ss.ThrowFromNativeCodeF(-1, "MapNodePop failed. The node did not represent an entry in the map");
			return;
		}

		else static_cast<IKeyResolver&>(m->Container->KeyResolver).Delete(m, ss);
	}

	VM_CALLBACK(MapNodeReleaseRef)
	{
		IScriptSystem& ss = *(IScriptSystem*)context;
		MapNode* node = (MapNode*)registers[VM::REGISTER_D7].vPtrValue;
		ReleaseNode(node, ss);
	}

	VM_CALLBACK(MapAssign)
	{
		MapImage* target = (MapImage*)registers[VM::REGISTER_D4].vPtrValue;
		MapImage* source = (MapImage*)registers[VM::REGISTER_D5].vPtrValue;

		if (target == source) return;

		if (source != nullptr)
		{
			source->refCount++;
		}

		if (target != nullptr)
		{
			OnInvokeMapRelease(registers, context);
		}
	}

	void CompileAsMapNodePop(CCompileEnvironment& ce, cr_sex s, cstr name)
	{
		ce.Builder.AssignVariableRefToTemp(name, Rococo::ROOT_TEMPDEPTH);
		AppendInvoke(ce, GetMapCallbacks(ce).MapNodePop, s);
	}

	bool HasInterface(const IInterface& interface, const IStructure& classspec);

	void CompileAsKeyToTemp(CCompileEnvironment& ce, cr_sex keyExpr, const MapDef& mapDef, int tempIndex)
	{
		auto& typeIStringInterface = ce.Object.Common().SysTypeIString();
		auto& typeIString = typeIStringInterface.NullObjectType();
		if (mapDef.KeyType == typeIString)
		{
			if (TryCompileStringLiteralInputToTemp(ce, keyExpr, 4, typeIString)) // key ptr goes to D8
			{
				return;
			}

			if (!IsAtomic(keyExpr))
			{
				Throw(keyExpr, "Expecting atomic expression");
			}

			cstr keyName = keyExpr.c_str();

			MemberDef def;
			if (ce.Builder.TryGetVariableByName(def, keyName))
			{
				if (HasInterface(typeIStringInterface, *def.ResolvedType))
				{
					ce.Builder.AssignVariableToTemp(keyName, 4);
					return;
				}
			}

			Throw(keyExpr, "Could not parse key as an IString");
		}
		else
		{
			VARTYPE keyVarType = mapDef.KeyType.VarType();
			if (IsPrimitiveType(keyVarType))
			{
				if (IsAtomic(keyExpr))
				{
					cstr svalue = keyExpr.c_str();
					if (svalue[0] == '-' || isdigit(svalue[0]))
					{
						VariantValue value;
						if (Parse::TryParse(OUT value, keyVarType, svalue) == Parse::PARSERESULT_GOOD)
						{
							ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4 + tempIndex, value, GetBitCount(keyVarType));
							return;
						}
					}
				}

				CompileNumericExpression(ce, keyExpr, keyVarType); // D7 now contains the key
				ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D7, VM::REGISTER_D4 + tempIndex, GetBitCount(keyVarType));
			}
			else
			{
				Throw(keyExpr, "Expecting IString or numeric as key");
			}
		}
	}

	void ConstructExplicitByRef(CCompileEnvironment& ce, cr_sex args, int tempDepth, const IStructure& type)
	{
		// The space has been allocated on the map and the reference is in REGISTER_(tempDepth + D4)

		const IFunction& constructor = *type.Constructor();

		if (type.VarType() == VARTYPE_Derivative && type.MemberCount() > 0)
		{
			ce.Builder.Assembler().Append_SwapRegister(VM::REGISTER_D4 + tempDepth, VM::REGISTER_SF);
			InitSubmembers(ce, type, 0);
			ce.Builder.Assembler().Append_SwapRegister(VM::REGISTER_D4 + tempDepth, VM::REGISTER_SF);
		}

		int inputStackAllocCount = 0;
		inputStackAllocCount = PushInputs(ce, args, constructor, true, 0);
		inputStackAllocCount += CompileInstancePointerArgFromTemp(ce, tempDepth);

		AppendFunctionCallAssembly(ce, constructor);

		ce.Builder.MarkExpression(&args);

		RepairStack(ce, *args.Parent(), constructor);
		ce.Builder.AssignClosureParentSFtoD6();
	}

	void ConstructMemberwiseByRef(CCompileEnvironment& ce, cr_sex args, int tempDepth, const IStructure& type)
	{
		if (args.NumberOfElements() != type.MemberCount())
		{
			Throw(args, "The number of arguments supplied in the memberwise constructor is %d, while the number of members in %s is %d", args.NumberOfElements(), GetFriendlyName(type), type.MemberCount());
		}

		AddArchiveRegister(ce, 6, 6, BITCOUNT_POINTER); // save D10
		ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D4 + tempDepth, VM::REGISTER_D10, BITCOUNT_POINTER); // copy our ref to D10

		ConstructMemberByRef(ce, args, REGISTER_D10, type, 0);

		ce.Builder.PopLastVariables(1, true);
	}

	void ConstructByRef(CCompileEnvironment& ce, cr_sex args, int tempDepth, const IStructure& type)
	{
		const IFunction* constructor = type.Constructor();
		if (constructor != NULL)
		{
			ConstructExplicitByRef(ce, args, tempDepth, type);
		}
		else
		{
			ConstructMemberwiseByRef(ce, args, tempDepth, type);
		}
	}

	void CompileAsMapInsert(CCompileEnvironment& ce, cr_sex s, cstr mapName)
	{
		// (a.Insert <key> <value> )

		auto def = GetMapDef(ce, s, mapName);
		if (def.ValueType.VarType() == VARTYPE_Derivative)
		{
			if (s.NumberOfElements() != 3 && s.NumberOfElements() != 4)
				Throw(s,
					"Expecting three or four elements:\r\n (<map-name>.Insert <key> <valueref>)\r\n  OR "
					"\r\n (<map-name>.Insert <key> <map-value-type> (<constructor-args>))");
		}
		else
		{
			if (s.NumberOfElements() != 3) Throw(s, ("Expecting three elements: (<map-name>.Insert <key> <value>)"));
		}

		cr_sex keyExpr = s.GetElement(1);

		CompileAsKeyToTemp(ce, keyExpr, def, Rococo::ROOT_TEMPDEPTH + 1);

		if (IsPrimitiveType(def.ValueType.VarType()))
		{
			cr_sex valueExpr = s.GetElement(2);
			cstr value = valueExpr.c_str();
			CompileNumericExpression(ce, valueExpr, def.ValueType.VarType()); // value goes to D7
			ce.Builder.AssignVariableToTemp(mapName, 0); // map ref goes to D4
			AppendInvoke(ce, GetMemberSize(def.ValueType) == 4 ? GetMapCallbacks(ce).MapInsert32 : GetMapCallbacks(ce).MapInsert64, s);
		}
		else if (def.ValueType.VarType() == VARTYPE_Closure)
		{
			if (!TryCompileAssignArchetype(ce, s[2], def.ValueType, false))
			{
				Throw(s[2], "Expecting archetype %s", GetFriendlyName(def.ValueType));
			}
			ce.Builder.AssignVariableToTemp(mapName, 0); // map ref goes to D4
			AppendInvoke(ce, GetMapCallbacks(ce).MapInsert64, s);
		}
		else if (def.ValueType.VarType() == VARTYPE_Derivative)
		{
			if (s.NumberOfElements() == 3)
			{
				cr_sex valueExpr = s.GetElement(2);
				CompileGetStructRef(ce, valueExpr, def.ValueType, ("insert-ref")); // structure ref goes to D7
				ce.Builder.AssignVariableToTemp(mapName, 0); // map ref goes to D4

				if (def.ValueType.InterfaceCount() > 0)
				{
					AppendInvoke(ce, GetMapCallbacks(ce).MapInsertInterface, s);
				}
				else
				{
					AppendInvoke(ce, GetMapCallbacks(ce).MapInsertValueByRef, s);
					if (Rococo::Compiler::Impl::CountRefCountedMembers(def.ValueType) > 0)
					{
						ce.Builder.Assembler().Append_Invoke(GetMapCallbacks(ce).MapUpdateRefCounts);
					}
				}
			}
			else // 4
			{
				cr_sex typeExpr = s.GetElement(2);
				if (!IsAtomic(typeExpr) || !AreEqual(typeExpr.String(), GetFriendlyName(def.ValueType)))
				{
					Throw(s, "Bad value type, syntax is: (<map-name>.Insert <key> %s (<constructor-args>))", GetFriendlyName(def.ValueType));
				}

				cr_sex argsExpr = s.GetElement(3);
				if (!IsCompound(argsExpr) && !IsNull(argsExpr))
				{
					Throw(s, "Value should be a compound expression, syntax is: (<map-name>.Insert <key> %s (<constructor-args>))", GetFriendlyName(def.ValueType));
				}

				ce.Builder.AssignVariableToTemp(mapName, 0); // map ref goes to D4
				AppendInvoke(ce, GetMapCallbacks(ce).MapInsertAndGetRef, s); // The value ref is now in D7

				ConstructByRef(ce, argsExpr, Rococo::ROOT_TEMPDEPTH, def.ValueType);
			}
		}
		else
		{
			Throw(keyExpr, ("Non primitive value types are not supported"));
		}
	}

	bool TryCompileAsMapCall(CCompileEnvironment& ce, cr_sex s, cstr mapName, cstr methodName)
	{
		if (AreEqual(methodName, "Insert"))
		{
			CompileAsMapInsert(ce, s, mapName);
			return true;
		}
		else
		{
			Throw(s, "Unknown map method. Known methods: Insert");
		}
	}

	bool TryCompileAsMapNodeCall(CCompileEnvironment& ce, cr_sex s, cstr name, cstr methodName)
	{
		if (AreEqual(methodName, "Pop"))
		{
			CompileAsMapNodePop(ce, s, name);
			return true;
		}
		else
		{
			Throw(s, "Unknown node method. Known methods: Exists, Value, Pop");
		}
	}

	void CompileGetMapElement(CCompileEnvironment& ce, cr_sex s, cstr instanceName, VARTYPE varType, const IStructure* structType)
	{
	}

	void CompileAsMapNodeDeclaration(CCompileEnvironment& ce, cstr nodeName, cr_sex source)
	{
		// (node n = (<map-name> <key>))
		if (source.NumberOfElements() != 2) Throw(source, "Expecting map lookup expression, in the form (node n = (<map-name> <key>))");

		cr_sex mapExpr = GetAtomicArg(source, 0);
		cr_sex keyExpr = source.GetElement(1);
		cstr mapName = mapExpr.c_str();

		const MapDef def = GetMapDef(ce, source, mapName);

		AddVariableRef(ce, NameString::From(nodeName), ce.Object.Common().TypeMapNode());

		AddMapNodeDef(ce.Script, ce.Builder, def, nodeName, mapName, source);

		CompileAsKeyToTemp(ce, keyExpr, def, Rococo::ROOT_TEMPDEPTH + 1);

		ce.Builder.AssignVariableToTemp(mapName, Rococo::ROOT_TEMPDEPTH); // D7 refers to the map
		AppendInvoke(ce, GetMapCallbacks(ce).MapTryGet, source); // now D7 refers to the correct node

		AssignTempToVariableRef(ce, Rococo::ROOT_TEMPDEPTH, nodeName);
	}

	void CompileMapDeclaration(CCompileEnvironment& ce, cr_sex s)
	{
		// (map <key-type> <value-type> <name>) and ((map <key-type> <value-type> <name> *)
		// Example (map IString Int32 a) creates a mapping with key IString and value type Int32 called a
		// Example (map IString Int32 a *) creates a reference object to a map, initially null

		AssertNotTooFewElements(s, 4);
		AssertNotTooManyElements(s, 5);

		cr_sex keytypeExpr = GetAtomicArg(s, 1);
		cr_sex valuetypeExpr = GetAtomicArg(s, 2);
		cr_sex nameExpr = GetAtomicArg(s, 3);

		cstr name = nameExpr.c_str();
		cstr keyType = keytypeExpr.c_str();
		cstr valueType = valuetypeExpr.c_str();

		if (s.NumberOfElements() == 5)
		{
			cr_sex extPtr = GetAtomicArg(s, 4);
			if (!Eq(nameExpr.c_str(), "*"))
			{
				Throw(s[4], "Expecting * in this position, which indicates a reference to an array that is initially null");
			}
		}

		AssertLocalIdentifier(nameExpr);

		const IStructure* keyStruct = MatchStructure(keytypeExpr, ce.Builder.Module());
		if (keyStruct == NULL) ThrowTokenNotFound(s, keyType, ce.Builder.Module().Name(), ("type"));

		const IStructure* valueStruct = MatchStructure(valuetypeExpr, ce.Builder.Module());
		if (valueStruct == NULL) ThrowTokenNotFound(s, valueType, ce.Builder.Module().Name(), ("type"));

		ce.Builder.AddSymbol(name);
		AddVariableRef(ce, NameString::From(name), ce.StructMap());

		if (s.NumberOfElements() == 4)
		{
			VariantValue k;
			k.vPtrValue = (void*)keyStruct;

			AddSymbol(ce.Builder, "keytype: %s", GetFriendlyName(*keyStruct));
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D5, k, BITCOUNT_POINTER); // Key type to D5

			VariantValue v;
			v.vPtrValue = (void*)valueStruct;
			AddSymbol(ce.Builder, "valuetype: %s", GetFriendlyName(*valueStruct));
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D4, v, BITCOUNT_POINTER); // Value type to D4
		}

		AddMapDef(ce.Script, ce.Builder, name, *keyStruct, *valueStruct, s);

		if (s.NumberOfElements() == 4)
		{
			ce.Builder.Assembler().Append_Invoke(GetMapCallbacks(ce).MapInit);
			ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, name);
		}
		else
		{
			VariantValue nullPtr;
			nullPtr.vPtrValue = nullptr;
			ce.Builder.Assembler().Append_SetRegisterImmediate(VM::REGISTER_D7, nullPtr, BITCOUNT_POINTER);
			ce.Builder.AssignTempToVariable(Rococo::ROOT_TEMPDEPTH, name);
		}
	}

	bool TryCompileAsInlineMapAndReturnValue(CCompileEnvironment& ce, cr_sex s, cstr instance, cstr methodName, VARTYPE returnType, const IStructure& instanceStruct, VARTYPE& outputType)
	{
		if (instanceStruct == ce.StructMap())
		{
			if (AreEqual(methodName, "Length"))
			{
				if (!IsAtomic(s) && s.NumberOfElements() != 1) return false;

				ce.Builder.Assembler().Append_PushRegister(VM::REGISTER_D13, BITCOUNT_POINTER);
				AddSymbol(ce.Builder, "%s -> D13", instance);
				ce.Builder.AssignVariableToTemp(instance, 9, 0); // Reference to the map goes to D13
				AppendInvoke(ce, GetMapCallbacks(ce).MapGetLength, s);
				ce.Builder.Assembler().Append_RestoreRegister(VM::REGISTER_D13, BITCOUNT_POINTER);
				return true;
			}
			else
			{
				Throw(s, "%s.%s: The only known method for a map in this context is Length", instance, methodName);
			}
		}
		else if (instanceStruct == ce.Object.Common().TypeMapNode())
		{
			const MapNodeDef& mnd = GetMapNodeDef(ce, s, instance);

			if (!IsAtomic(s) && s.NumberOfElements() != 1) return false;

			TokenBuffer field;
			if (AreEqual("Exists", methodName))
			{
				StringPrint(field, "%s._exists", instance);
				ValidateReturnType(s, returnType, VARTYPE_Bool);
				ce.Builder.AddSymbol(field);
				ce.Builder.AssignVariableToTemp(field, Rococo::ROOT_TEMPDEPTH, 0);
				outputType = VARTYPE_Bool;
				return true;
			}
			if (AreEqual("Key", methodName))
			{
				ce.Builder.AssignVariableRefToTemp(instance, Rococo::ROOT_TEMPDEPTH, 0); // node goes to D7

				VARTYPE valType = mnd.mapdef.KeyType.VarType();

				if (returnType != VARTYPE_AnyNumeric && valType != returnType)
				{
					Throw(s, "The node is for a map with key type %s but the context requires a type ", GetFriendlyName(mnd.mapdef.KeyType), GetTypeName(returnType));
				}

				if (returnType == VARTYPE_AnyNumeric)
				{
					returnType = valType;
				}

				outputType = valType;

				switch (returnType)
				{
				case VARTYPE_Bool:
				case VARTYPE_Int32:
				case VARTYPE_Float32:
					AppendInvoke(ce, GetMapCallbacks(ce).MapNodeGetKey32, s);
					break;
				case VARTYPE_Int64:
				case VARTYPE_Float64:
				case VARTYPE_Closure:
					AppendInvoke(ce, GetMapCallbacks(ce).MapNodeGetKey64, s);
					break;
				case VARTYPE_Pointer:
					AppendInvoke(ce, ((int)BITCOUNT_POINTER == 32) ? GetMapCallbacks(ce).MapNodeGetKey32 : GetMapCallbacks(ce).MapNodeGetKey64, s);
					break;
				case VARTYPE_Derivative:
					{
						auto& keyType = mnd.mapdef.KeyType;
						if (keyType != ce.Object.Common().SysTypeIString().NullObjectType())
						{
							Throw(0, "Do not know how to retrieve key for map, with type: %s", GetFriendlyName(keyType));
						}
					}

					// Reference count logic should not matter here, because maps with IString as a key use CStringConstant which disables reference counting

					AppendInvoke(ce, GetMapCallbacks(ce).MapNodeGetKeyIString, s);
					break;
				default:
					Throw(s, "Unexpected type in map invoke");
					break;
				}
			}
			else if (AreEqual("Value", methodName))
			{
				ce.Builder.AssignVariableRefToTemp(instance, Rococo::ROOT_TEMPDEPTH, 0); // node goes to D7

				auto& valueType = mnd.mapdef.ValueType;
				VARTYPE valType = valueType.VarType();

				if (valType == VARTYPE_Derivative)
				{
					if (IsNullType(valueType))
					{
						// Null object
						AppendInvoke(ce, GetMapCallbacks(ce).MapNodeGetInterface, s);
						return true;
					}

					Throw(s, "%s. Value not supported for derivative value types. Use this syntax to access values: (%s value = & %s)", instance, GetFriendlyName(mnd.mapdef.ValueType), instance);
				}

				if (returnType != VARTYPE_AnyNumeric && valType != returnType)
				{
					Throw(s, "The node is for a map with value type %s but the context requires a type %s", GetFriendlyName(mnd.mapdef.ValueType), GetTypeName(returnType));
				}

				if (returnType == VARTYPE_AnyNumeric)
				{
					returnType = valType;
				}

				outputType = valType;

				switch (returnType)
				{
				case VARTYPE_Bool:
				case VARTYPE_Int32:
				case VARTYPE_Float32:
					AppendInvoke(ce, GetMapCallbacks(ce).MapNodeGet32, s);
					break;
				case VARTYPE_Int64:
				case VARTYPE_Float64:
				case VARTYPE_Closure:
					AppendInvoke(ce, GetMapCallbacks(ce).MapNodeGet64, s);
					break;
				case VARTYPE_Pointer:
					AppendInvoke(ce, ((int)BITCOUNT_POINTER == 32) ? GetMapCallbacks(ce).MapNodeGet32 : GetMapCallbacks(ce).MapNodeGet64, s);
					break;
				default:
					Throw(s, "Unexpected type in map invoke");
					break;
				}
			}
			else
			{
				Throw(s, "The property is not recognized for map node types. Known properties for maps: Exists, Value");
			}

			return true;
		}

		return false;
	}

	void CompileEnumerateMap(CCompileEnvironment& ce, cr_sex s, int hashIndex)
	{
		// (foreach i n # a (...) (...) )
		// (foreach n # a (...) (...) )

		cr_sex collection = s.GetElement(hashIndex + 1);
		cstr collectionName = collection.c_str();

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

			AddVariable(ce, NameString::From(indexName), ce.Object.Common().TypeInt32());
			cr_sex refExpr = s.GetElement(2);
			AssertLocalIdentifier(refExpr);
			refName = refExpr.c_str();
		}

		const MapDef& mapdef = GetMapDef(ce, s, collectionName);

		ce.Builder.AddSymbol("(foreach...");

		ce.Builder.AddSymbol(collectionName);
		AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 6, Rococo::ROOT_TEMPDEPTH + 6, BITCOUNT_POINTER);
		ce.Builder.AssignVariableToTemp(collectionName, 9, 0);	// Map ref is in D13

		AppendInvoke(ce, GetMapCallbacks(ce).MapGetLength, collection); // MapGetLength takes map ref at D13 and assigns map length to D7

		//////////////////////////////////////////////////////// Test map to see if it is empty //////////////////////////////////////////////////////
		ce.Builder.Assembler().Append_Test(VM::REGISTER_D7, BITCOUNT_32);
		size_t bailoutPos = ce.Builder.Assembler().WritePosition();
		ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_EQUAL, 0);
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		AddArchiveRegister(ce, Rococo::ROOT_TEMPDEPTH + 5, Rococo::ROOT_TEMPDEPTH + 5, BITCOUNT_POINTER);

		ce.Builder.Assembler().Append_MoveRegister(VM::REGISTER_D13, VM::REGISTER_D7, BITCOUNT_POINTER);
		AppendInvoke(ce, GetMapCallbacks(ce).MapGetHead, collection);
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

		AddVariableRef(ce, NameString::From(refName), ce.Object.Common().TypeMapNode());

		AddMapNodeDef(ce.Script, ce.Builder, mapdef, refName, collectionName, collection);

		ptrdiff_t startLoop = ce.Builder.Assembler().WritePosition();

		AssignTempToVariableRef(ce, 8, refName); // refref now gives us the head node

		if (indexName != NULL)
		{
			// Init our index var to the current index value then increment the index value
			ce.Builder.AssignTempToVariable(7, indexName);

			VariantValue one;
			one.int32Value = 1;
			ce.Builder.Assembler().Append_AddImmediate(VM::REGISTER_D11, BITCOUNT_32, VM::REGISTER_D11, one);
		}

		CompileExpressionSequence(ce, hashIndex + 2, s.NumberOfElements() - 1, s);

		AppendInvoke(ce, GetMapCallbacks(ce).NodeEnumNext, s); // Gives the raw node next value from D12 to D12

		ce.Builder.Assembler().Append_Test(VM::REGISTER_D12, BITCOUNT_POINTER);

		ptrdiff_t endLoop = ce.Builder.Assembler().WritePosition();
		ce.Builder.Assembler().Append_BranchIf(Rococo::CONDITION_IF_NOT_EQUAL, (int32)(startLoop - endLoop));

		ce.Builder.PopLastVariables(indexName != NULL ? 4 : 2, true); // Release the D10-D12 and the ref. We need to release the ref manually to stop the refcount decrement

		size_t exitPos = ce.Builder.Assembler().WritePosition();
		size_t bailoutToExit = exitPos - bailoutPos;
		ce.Builder.Assembler().SetWriteModeToOverwrite(bailoutPos);
		ce.Builder.Assembler().Append_BranchIf(CONDITION_IF_EQUAL, (int32)bailoutToExit);
		ce.Builder.Assembler().SetWriteModeToAppend();

		ce.Builder.PopLastVariables(1, true);
	}

	void RegisterMaps(MapCallbacks& mapCallbacks, VM::ICore& core, IScriptSystem& ss)
	{
		mapCallbacks.MapAssign = core.RegisterCallback(OnInvokeMapAssign, &ss, "MapAssign");
		mapCallbacks.MapRelease = core.RegisterCallback(OnInvokeMapRelease, &ss, "MapRelease");
		mapCallbacks.NodeEnumNext = core.RegisterCallback(OnInvokeMapNodeEnumNext, &ss, "MapNodeEnumNext");
		mapCallbacks.MapGetHead = core.RegisterCallback(OnInvokeMapGetHead, &ss, "MapGetHead");
		mapCallbacks.MapInit = core.RegisterCallback(OnInvokeMapInit, &ss, "MapInit");
		mapCallbacks.DoesMapNodeExist = core.RegisterCallback(OnInvokeDoesMapNodeExist, &ss, "DoesMapNodeExist");
		mapCallbacks.MapInsert32 = core.RegisterCallback(OnInvokeMapInsert32, &ss, "MapInsert32");
		mapCallbacks.MapInsert64 = core.RegisterCallback(OnInvokeMapInsert64, &ss, "MapInsert64");
		mapCallbacks.MapInsertValueByRef = core.RegisterCallback(OnInvokeMapInsertValueByRef, &ss, "MapInsertValueByRef");
		mapCallbacks.MapInsertAndGetRef = core.RegisterCallback(OnInvokeMapInsertAndGetRef, &ss, "MapInsertAndGetRef");
		mapCallbacks.MapInsertInterface = core.RegisterCallback(OnInvokeMapInsertInterface, &ss, "MapInsertInterface");
		mapCallbacks.MapTryGet = core.RegisterCallback(OnInvokeMapTryGet, &ss, "MapTryGet");
		mapCallbacks.MapNodeGet32 = core.RegisterCallback(OnInvokeMapNodeGet32, &ss, "MapNodeGet32");
		mapCallbacks.MapNodeGet64 = core.RegisterCallback(OnInvokeMapNodeGet64, &ss, "MapNodeGet64");
		mapCallbacks.MapNodeGetInterface = core.RegisterCallback(OnInvokeMapNodeGetInterface, &ss, "MapNodeGetInterface");
		mapCallbacks.MapNodeGetKey32 = core.RegisterCallback(OnInvokeMapNodeGetKey32, &ss, "MapNodeGetKey32");
		mapCallbacks.MapNodeGetKey64 = core.RegisterCallback(OnInvokeMapNodeGetKey64, &ss, "MapNodeGetKey64");
		mapCallbacks.MapNodeGetKeyIString = core.RegisterCallback(OnInvokeMapNodeGetKeyIString, &ss, "MapNodeGetKeyIString");
		mapCallbacks.MapNodeGetRef = core.RegisterCallback(OnInvokeMapNodeGetRef, &ss, "MapNodeGetRef");
		mapCallbacks.MapNodePop = core.RegisterCallback(OnInvokeMapNodePop, &ss, "MapNodePop");
		mapCallbacks.MapNodeReleaseRef = core.RegisterCallback(OnInvokeMapNodeReleaseRef, &ss, "MapNodeReleaseRef");
		mapCallbacks.MapGetLength = core.RegisterCallback(OnInvokeMapGetLength, &ss, "MapGetLength");
		mapCallbacks.MapUpdateRefCounts = core.RegisterCallback(OnInvokeMapUpdateRefCounts, &ss, "MapUpdateRefCounts");
	}
}// Rococo::Script