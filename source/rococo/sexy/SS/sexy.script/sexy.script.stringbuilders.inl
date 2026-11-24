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

namespace Rococo::Strings
{
	void CreateMemoString(NativeCallEnvironment& e)
	{
		TMemoAllocator& allocator = *(TMemoAllocator*) e.context;

		InterfacePointer mutableStringPtr;
		ReadInput(0, mutableStringPtr, e);

		auto* strSrc = (CStringConstant*) InterfaceToInstance(mutableStringPtr);
		if (!strSrc->header.Desc->flags.IsSystem)
		{
			Throw(0, "%s: the source string was not a Sys based string, which is a security violation.", __ROCOCO_FUNCTION__);
		}

		cstr src = strSrc->pointer;
		int srcLen = strSrc->length;

		if (src == NULL)
		{
			src = ("");
			srcLen = 0;
		}

		int32 truncLen;
		if (srcLen < 0)
		{
			// Consider throwing the VM at this point
			truncLen = 0;
		}
		else if (srcLen < TMemo::CAPACITY)
		{
			truncLen = srcLen;
		}
		else
		{
			truncLen = TMemo::CAPACITY - 1;
		}
		
		TMemo* memo = allocator.alloc();
		memcpy(memo->Data, src, sizeof(char) * truncLen);
		memo->Data[sizeof(char) * truncLen] = 0;

		WriteOutput(0, (void*) memo, e);
		WriteOutput(1, truncLen, e);

		_CrtCheckMemory();
	}

	void FreeMemoString(NativeCallEnvironment& e)
	{
		TMemoAllocator& allocator = *(TMemoAllocator*) e.context;

		void* vSrc;
		ReadInput(0, vSrc, e);
		
		allocator.free((TMemo*)vSrc);
	}

	// Builders made by NewTokenBuilder, NewPathBuilder and NewParagraphBuilder (and NewStringBuilder of the same initial capacity) take a pooled header with an inline buffer.
	// The pool a header belongs to is recorded in its flags (StringBuilderFlags::Pool64/260/1024) at creation and never changes, so a builder always returns to its own free list,
	// whatever its capacity has become. An expandable pooled builder that grows moves to a heap buffer; on destruct the heap buffer is freed and the inline buffer restored.
	// All headers and buffers are allocated with AllocateSexyMemory and freed with FreeSexyUnknownMemory.
#pragma pack(push,1)
	struct FastStringBuilder_64
	{
		FastStringBuilder header;
		char rawData[64];
	};

	struct FastStringBuilder_260
	{
		FastStringBuilder header;
		char rawData[260];
	};

	struct FastStringBuilder_1024
	{
		FastStringBuilder header;
		char rawData[1024];
	};
#pragma pack(pop)

	static_assert(offsetof(FastStringBuilder_64, rawData) == sizeof(FastStringBuilder), "The inline buffer must follow the header");
	static_assert(offsetof(FastStringBuilder_260, rawData) == sizeof(FastStringBuilder), "The inline buffer must follow the header");
	static_assert(offsetof(FastStringBuilder_1024, rawData) == sizeof(FastStringBuilder), "The inline buffer must follow the header");

	enum : int32
	{
		SB_POOL_64 = (int32)StringBuilderFlags::Pool64,
		SB_POOL_260 = (int32)StringBuilderFlags::Pool260,
		SB_POOL_1024 = (int32)StringBuilderFlags::Pool1024,
		SB_POOL_MASK = (int32)StringBuilderFlags::PoolMask,
		SB_EXPANDABLE = (int32)StringBuilderFlags::Expandable,
		SB_THROW_IF_TRUNCATED = (int32)StringBuilderFlags::ThrowIfWouldTruncate,
		SB_MAX_CAPACITY = 0x40000000 // 1GB, including the nul terminator
	};

	inline char* InlineBufferOf(FastStringBuilder& sb)
	{
		return ((char*)&sb) + sizeof(FastStringBuilder);
	}

	inline int32 PoolCapacityOf(int32 poolFlag)
	{
		switch (poolFlag)
		{
		case SB_POOL_64: return 64;
		case SB_POOL_260: return 260;
		case SB_POOL_1024: return 1024;
		default: return 0;
		}
	}

	inline bool IsExpandable(const FastStringBuilder& sb)
	{
		return (sb.flags & SB_EXPANDABLE) != 0;
	}

	// True if the builder's buffer was allocated separately from its header and is owned by the builder
	inline bool HasHeapBuffer(FastStringBuilder& sb)
	{
		return (sb.flags & SB_POOL_MASK) == 0 || sb.buffer != InlineBufferOf(sb);
	}

	inline bool IsBuilderValid(const FastStringBuilder& sb)
	{
		return sb.buffer != nullptr && sb.capacity > 0 && sb.length >= 0 && sb.length < sb.capacity;
	}

	// Cheap sanity check on a length taken from an IString. Script classes that implement IString set their own length and buffer fields.
	// By policy we do not scan for the terminator on every call: a script that lies about a positive length can read past its buffer, as a malicious script
	// can do other harm anyway. Null buffers and negative lengths count as empty, and all length arithmetic is done in 64 bits with the 1GB cap
	inline int32 SaneLength(cstr s, int32 length)
	{
		return (s == nullptr || length < 0) ? 0 : length;
	}

	char* AllocateBuilderMemory(size_t nBytes)
	{
		void* p = nullptr;

		try
		{
			p = Rococo::Memory::AllocateSexyMemory(nBytes);
		}
		catch (...)
		{
			p = nullptr;
		}

		if (p == nullptr)
		{
			// The default sexy allocator returns null on failure, others throw. Either way we report it the same way
			Throw(0, "IStringBuilder: failed to allocate %llu bytes", (uint64)nBytes);
		}

		return (char*)p;
	}

	void MergeFormats(char* format, const FastStringBuilder& sb, cstr suffix)
	{
		*format = '%';
		CopyString(format + 1, 15, sb.prefix);
		StringCat(format, suffix, 16);
	}

	struct NullAllocator: public IScriptObjectAllocator
	{
		void* AllocateObject(size_t nBytes) override
		{
			UNUSED(nBytes);
			return nullptr;
		}

		void FreeObject(void* pMemory) override
		{
			UNUSED(pMemory);
		}

		refcount_t AddRef() override
		{
			return 0;
		}

		refcount_t ReleaseRef() override
		{
			return 0;
		}

		size_t FreeAll(IEventCallback<LeakArgs>* leakCallback) override
		{
			UNUSED(leakCallback);
			return 0;
		}
	};

	// Scratch space that lives on the stack for small requests and falls back to AllocateBuilderMemory, freed when it goes out of scope
	struct BuilderScratch
	{
		enum { INLINE_CAPACITY = 1024 };
		char inlineData[INLINE_CAPACITY];
		char* data = nullptr;

		// Never throws. data is null if the allocation failed
		BuilderScratch(size_t nBytes)
		{
			if (nBytes <= INLINE_CAPACITY)
			{
				data = inlineData;
			}
			else
			{
				try
				{
					data = AllocateBuilderMemory(nBytes);
				}
				catch (...)
				{
					data = nullptr;
				}
			}
		}

		~BuilderScratch()
		{
			if (data != inlineData && data != nullptr)
			{
				Rococo::Memory::FreeSexyUnknownMemory(data);
			}
		}

		BuilderScratch(const BuilderScratch&) = delete;
		BuilderScratch& operator = (const BuilderScratch&) = delete;
	};

	// String pools can massively increase alloc and free speed when capacity is in one of the sweet spots.
	struct StringPool: IStringPool
	{
		AllocatorBinding binding;
		NullAllocator nullAllocator;

		StringPool()
		{
			binding.associatedStructure = nullptr;
			binding.memoryAllocator = &nullAllocator;
			binding.standardDestruct = false;
		}

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS;

		const IStructure* typeFastStringBuilder = nullptr;

		TSexyVector<FastStringBuilder_64*> freeList_64; // For tokens and such. Could be smaller, but then dwarfed by the FastStringBuilder object
		TSexyVector<FastStringBuilder_260*> freeList_260; // For file paths and whatever
		TSexyVector<FastStringBuilder_1024*> freeList_1024; // Paragraphs
		TSexyHashMap<FastStringBuilder*, int32> allocList; // Every header the pool has allocated, live or free, mapped to its pool flag (0 for heap headers)

		void SetStringBuilderType(const IStructure* typeFastStringBuilder) override
		{
			binding.associatedStructure = typeFastStringBuilder;
			this->typeFastStringBuilder = typeFastStringBuilder;
		}

		const IStructure* FastStringBuilderType() const override
		{
			return typeFastStringBuilder;
		}

		AllocatorBinding* GetBinding() override
		{
			// StringPool memory allocators manage memory in the creation methods and the Destruct methods manually so use null allocators
			return &binding;
		}

		void Free() override
		{
			// Builders still alive here (leaked by a script or held by the host) own any heap buffer they have, so free those too.
			// Any pointer the host still holds to such a builder is invalid after this
			for (auto i : allocList)
			{
				FastStringBuilder* sb = i.first;
				if (HasHeapBuffer(*sb))
				{
					Rococo::Memory::FreeSexyUnknownMemory(sb->buffer);
				}

				Rococo::Memory::FreeSexyUnknownMemory(sb);
			}

			allocList.clear();

			delete this;
		}

		template<class T> FastStringBuilder* NewPooled(TSexyVector<T*>& freeList, int32 poolFlag)
		{
			T* item;
			if (freeList.empty())
			{
				char* mem = AllocateBuilderMemory(sizeof(T));
				item = new (mem) T();

				try
				{
					allocList[&item->header] = poolFlag;
				}
				catch (...)
				{
					Rococo::Memory::FreeSexyUnknownMemory(mem);
					throw;
				}
			}
			else
			{
				item = freeList.back();
				freeList.pop_back();
			}

			item->header.buffer = item->rawData;
			item->header.capacity = (int32) sizeof(item->rawData);
			item->header.flags = poolFlag; // Clears ThrowIfWouldTruncate and Expandable left by the previous user
			return &item->header;
		}

		FastStringBuilder* NewHeapBuilder(int32 capacity)
		{
			char* mem = AllocateBuilderMemory(sizeof(FastStringBuilder));

			char* buffer;
			try
			{
				buffer = AllocateBuilderMemory((size_t)capacity);
			}
			catch (...)
			{
				Rococo::Memory::FreeSexyUnknownMemory(mem);
				throw;
			}

			auto* sb = new (mem) FastStringBuilder();
			sb->buffer = buffer;
			sb->capacity = capacity;
			sb->flags = 0;

			try
			{
				allocList[sb] = 0;
			}
			catch (...)
			{
				Rococo::Memory::FreeSexyUnknownMemory(buffer);
				Rococo::Memory::FreeSexyUnknownMemory(mem);
				throw;
			}

			return sb;
		}

		FastStringBuilder* Create(int32 capacity, bool isExpandable)
		{
			if (capacity <= 0 || capacity > SB_MAX_CAPACITY)
			{
				Throw(0, "%s: capacity %d must be from 1 to %d", __ROCOCO_FUNCTION__, capacity, (int32) SB_MAX_CAPACITY);
			}

			FastStringBuilder* sb;

			switch (capacity)
			{
			case 64:
				sb = NewPooled(freeList_64, SB_POOL_64);
				break;
			case 260:
				sb = NewPooled(freeList_260, SB_POOL_260);
				break;
			case 1024:
				sb = NewPooled(freeList_1024, SB_POOL_1024);
				break;
			default:
				sb = NewHeapBuilder(capacity);
				break;
			}

			if (isExpandable)
			{
				sb->flags |= SB_EXPANDABLE;
			}

			return sb;
		}

		FastStringBuilder* CreateAndInitFields(int32 capacity, bool isExpandable) override
		{
			auto* sb = Create(capacity, isExpandable);
			sb->buffer[0] = 0;
			sb->length = 0;
			sb->control = this;
			sb->formatBase = 10;
			sb->spec = SPEC_F;
			sb->stub.Desc = (ObjectDesc*) typeFastStringBuilder->GetVirtualTable(0);
			sb->stub.refCount = 1;
			sb->stub.pVTables[0] = (VirtualTable*) typeFastStringBuilder->GetVirtualTable(1);
			SafeFormat(sb->prefix, PREFIX_LEN, "4.4");
			return sb;
		}

		void Destruct(FastStringBuilder& sb)
		{
			int32 poolFlag = sb.flags & SB_POOL_MASK;

#ifdef SEXY_ENABLE_EXTRA_STRING_SECURITY
			memset(sb.buffer, 0, sb.capacity); // For security we could erase buffer, to stop re-use exposing old contents
#endif

			if (poolFlag == 0)
			{
				allocList.erase(&sb);
				Rococo::Memory::FreeSexyUnknownMemory(sb.buffer);
				Rococo::Memory::FreeSexyUnknownMemory(&sb);
				return;
			}

			if (sb.buffer != InlineBufferOf(sb))
			{
				// An expandable pooled builder that grew. Give back the heap buffer and restore the inline one
				Rococo::Memory::FreeSexyUnknownMemory(sb.buffer);
				sb.buffer = InlineBufferOf(sb);
			}

			sb.capacity = PoolCapacityOf(poolFlag);
			sb.buffer[0] = 0;
			sb.length = 0;
			sb.flags = poolFlag;
			sb.spec = SPEC_F;
			sb.formatBase = 10;

			switch (poolFlag)
			{
			case SB_POOL_64:
				freeList_64.push_back((FastStringBuilder_64*) &sb);
				break;
			case SB_POOL_260:
				freeList_260.push_back((FastStringBuilder_260*) &sb);
				break;
			case SB_POOL_1024:
				freeList_1024.push_back((FastStringBuilder_1024*) &sb);
				break;
			default:
				// More than one pool bit set: the header is corrupt. Leave it in allocList for Free() rather than guess which list it belongs to
				break;
			}
		}

		void ExpandStringBuilder(FastStringBuilder& sb, size_t deltaLength) override
		{
			if (sb.control != this)
			{
				Throw(0, "%s: The string builder does not belong to this string pool", __ROCOCO_FUNCTION__);
			}

			if (!IsExpandable(sb))
			{
				// Was of reserved length (NewTokenBuilder, NewPathBuilder or NewParagraphBuilder)
				Throw(0, "%s: The string builder was not expandable", __ROCOCO_FUNCTION__);
			}

			if (!IsBuilderValid(sb))
			{
				Throw(0, "%s: The string builder was corrupt (length %d, capacity %d)", __ROCOCO_FUNCTION__, sb.length, sb.capacity);
			}

			if (deltaLength > (size_t) SB_MAX_CAPACITY)
			{
				Throw(0, "%s: expansion is capped to 1GB", __ROCOCO_FUNCTION__);
			}

			uint64 targetCapacity = (uint64)sb.length + (uint64)deltaLength;

			if (targetCapacity > (uint64) SB_MAX_CAPACITY)
			{
				Throw(0, "%s: expansion is capped to 1GB", __ROCOCO_FUNCTION__);
			}

			if (targetCapacity <= (uint64) sb.capacity)
			{
				return; // Already big enough
			}

			uint64 newCapacity = sb.capacity < 16 ? 16 : (uint64) sb.capacity;
			while (newCapacity < targetCapacity)
			{
				newCapacity *= 2;
			}

			if (newCapacity > (uint64) SB_MAX_CAPACITY)
			{
				newCapacity = SB_MAX_CAPACITY; // >= targetCapacity, which was checked above
			}

			// Allocate before changing anything, so that on failure the builder is exactly as it was
			char* newBuffer = AllocateBuilderMemory((size_t)newCapacity);

			memcpy(newBuffer, sb.buffer, (size_t)sb.length + 1); // We added 1 to cover the trailing zero

			if (HasHeapBuffer(sb))
			{
				Rococo::Memory::FreeSexyUnknownMemory(sb.buffer);
			}

			sb.buffer = newBuffer;
			sb.capacity = (int32) newCapacity;

			// Note, that it is not our job here to update the length, we merely updated the capacity to allow the length to be legal
		}

		// Appends using the builder's rules: expandable builders grow, others truncate or throw if ThrowIfWouldTruncate is set. [src] may point into the builder itself.
		int32 Append(IPublicScriptSystem& ss, FastStringBuilder& sb, const char* src, int32 srcLen)
		{
			if (src == nullptr || srcLen <= 0)
			{
				return 0;
			}

			if (!IsBuilderValid(sb))
			{
				ss.ThrowFromNativeCodeF(0, "IStringBuilder.Append: the builder was corrupt (length %d, capacity %d)", sb.length, sb.capacity);
				return 0;
			}

			int64 required = (int64)sb.length + (int64)srcLen; // chars, excluding the nul terminator
			if (required >= (int64)sb.capacity)
			{
				if (IsExpandable(sb))
				{
					// The source may be the builder itself, whose buffer is about to move
					ptrdiff_t selfOffset = -1;
					if (src >= sb.buffer && src < sb.buffer + sb.capacity)
					{
						selfOffset = src - sb.buffer;
					}

					try
					{
						ExpandStringBuilder(sb, (size_t)srcLen + 1);
					}
					catch (IException& ex)
					{
						ss.ThrowFromNativeCodeF(ex.ErrorCode(), "IStringBuilder.Append: %s", ex.Message());
						return 0;
					}

					if (selfOffset >= 0)
					{
						src = sb.buffer + selfOffset;
					}
				}
				else if (0 != (sb.flags & SB_THROW_IF_TRUNCATED))
				{
					ss.ThrowFromNativeCodeF(0, "IStringBuilder.Append: Insufficient buffer capacity for append operation and truncation forbidden");
					return 0;
				}
				else
				{
					srcLen = sb.capacity - 1 - sb.length; // Truncate
				}
			}

			if (srcLen <= 0)
			{
				return 0;
			}

			memmove(sb.buffer + sb.length, src, (size_t)srcLen); // memmove, as the source may overlap the builder
			sb.length += srcLen;
			sb.buffer[sb.length] = 0;
			return srcLen;
		}

		int32 AppendToBuilder(IPublicScriptSystem& ss, FastStringBuilder& sb, cstr text, int32 length) override
		{
			return Append(ss, sb, text, SaneLength(text, length));
		}
	};

	StringPool& PoolOf(NativeCallEnvironment& e)
	{
		return *(StringPool*)e.context;
	}

	void CreateBuilder(NativeCallEnvironment& e, bool isExpandable)
	{
		StringPool& pool = PoolOf(e);

		int32 capacity;
		ReadInput(0, capacity, e);

		if (capacity <= 0) 
		{
			e.ss.ThrowFromNativeCodeF(0, "NewStringBuilder failed. Capacity must be positive");
			return;
		}

		if (capacity > SB_MAX_CAPACITY)
		{
			e.ss.ThrowFromNativeCodeF(0, "NewStringBuilder failed. Capacity must be no greater than 1 gigabyte");
			return;
		}

		FastStringBuilder* sb;
		
		try
		{
			sb = pool.CreateAndInitFields(capacity, isExpandable);
		}
		catch (IException& ex)
		{
			e.ss.ThrowFromNativeCodeF(ex.ErrorCode(), "NewStringBuilder failed. %s", ex.Message());
			return;
		}

		WriteOutput(0, (void*)&sb->stub.pVTables[0], e);
	}

	// Fixed capacity builders, used by NewTokenBuilder (64), NewPathBuilder (260) and NewParagraphBuilder (1024). Appends truncate, or throw with ThrowIfAppendWouldTruncate
	void NewStringBuilder(NativeCallEnvironment& e)
	{
		CreateBuilder(e, false);
	}

	// Used by NewStringBuilder. The capacity is only the initial capacity: appends grow the builder up to 1GB
	void NewExpandableStringBuilder(NativeCallEnvironment& e)
	{
		CreateBuilder(e, true);
	}

	void DestructStringBuilder(NativeCallEnvironment& e)
	{
		StringPool& pool = PoolOf(e);

		InterfacePointer p;
		ReadInput(0, p, e);

		auto* stub = InterfaceToInstance(p);
		auto* sb = (FastStringBuilder*) stub;

		pool.Destruct(*sb);
	}

	FastStringBuilder& ReadBuilder(NativeCallEnvironment& e)
	{
		InterfacePointer p;
		ReadInput(0, (void*&) p, e);

		auto* stub = InterfaceToInstance(p);
		auto* sb = (FastStringBuilder*)stub;
		return *sb;
	}

	int32 AppendString(NativeCallEnvironment& e, FastStringBuilder& sb, const char* src, int32 srcLen)
	{
		return PoolOf(e).Append(e.ss, sb, src, srcLen);
	}

	// Throws through the script system and returns false if the builder's length or capacity are out of range
	bool ValidateBuilder(NativeCallEnvironment& e, const FastStringBuilder& sb, cstr method)
	{
		if (!IsBuilderValid(sb))
		{
			e.ss.ThrowFromNativeCodeF(0, "IStringBuilder.%s: the builder was corrupt (length %d, capacity %d)", method, sb.length, sb.capacity);
			return false;
		}

		return true;
	}

	void FastStringBuilderAppendIString(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		const char* srcBuffer;
		ReadInput(1, srcBuffer, e);

		int32 srcLength;
		ReadInput(2, srcLength, e);

		srcLength = SaneLength(srcBuffer, srcLength);

		if (srcLength > 0)
		{
			AppendString(e, sb, srcBuffer, srcLength);
		}
	}

	void FastStringBuilderThrowIfAppendWouldTruncate(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);
		sb.flags |= SB_THROW_IF_TRUNCATED;
	}

	void FastStringBuilderClear(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);
		sb.length = 0;
		sb.buffer[0] = 0;
	}

	void FastStringBuilderMakeSysSlashes(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);
		if (!ValidateBuilder(e, sb, "MakeSysSlashes")) return;
		Rococo::IO::ToSysPath(sb.buffer); // Converts the whole nul terminated buffer in one pass
	}

	void FastStringBuilderReplace(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);
		
		int startPosition;
		ReadInput(1, startPosition, e);

		InterfacePointer ipFrom;
		ReadInput(2, ipFrom, e);

		InterfacePointer ipTo;
		ReadInput(3, ipTo, e);

		// The compiler requires every IString implementation to have the length and buffer fields of InlineString, but the values are the script's to set
		auto* sfrom = (InlineString*)InterfaceToInstance(ipFrom);
		auto* sto = (InlineString*)InterfaceToInstance(ipTo);

		if (!ValidateBuilder(e, sb, "Replace")) return;

		if (startPosition < 0)
		{
			startPosition = 0;
		}

		if (startPosition >= sb.length)
		{
			// No work to do
			return;
		}

		int32 fromLength = SaneLength(sfrom->buffer, sfrom->length);
		int32 toLength = SaneLength(sto->buffer, sto->length);

		if (fromLength == 0)
		{
			e.ss.ThrowFromNativeCodeF(0, "IStringBuilder.Replace: (IString from) - from was blank");
			return;
		}

		const int32 length = sb.length;

		// Count the non-overlapping matches, left to right
		int64 matches = 0;
		for (int32 i = startPosition; i <= length - fromLength; )
		{
			if (memcmp(sb.buffer + i, sfrom->buffer, (size_t)fromLength) == 0)
			{
				matches++;
				i += fromLength;
			}
			else
			{
				i++;
			}
		}

		if (matches == 0)
		{
			return;
		}

		int64 newLength = (int64)length + matches * ((int64)toLength - (int64)fromLength);

		// Snapshot the builder, from and to before anything changes. Either of from and to may be the builder itself, and expansion moves the buffer.
		// Only reached when there is at least one match, and small cases stay on the stack
		BuilderScratch scratch((size_t)length + (size_t)fromLength + (size_t)toLength + 3);
		if (scratch.data == nullptr)
		{
			e.ss.ThrowFromNativeCodeF(0, "IStringBuilder.Replace: failed to allocate scratch memory");
			return;
		}

		char* original = scratch.data;
		char* from = original + length + 1;
		char* to = from + fromLength + 1;
		memcpy(original, sb.buffer, (size_t)length);
		original[length] = 0;
		memcpy(from, sfrom->buffer, (size_t)fromLength);
		from[fromLength] = 0;
		memcpy(to, sto->buffer, (size_t)toLength);
		to[toLength] = 0;

		if (newLength >= (int64)sb.capacity)
		{
			if (!IsExpandable(sb))
			{
				// Insufficient space to perform replacement. Nothing has been changed
				e.ss.ThrowFromNativeCodeF(0, "IStringBuilder.Replace: Insufficient space in buffer to replace character sequence");
				return;
			}

			try
			{
				PoolOf(e).ExpandStringBuilder(sb, (size_t)(newLength - length) + 1);
			}
			catch (IException& ex)
			{
				e.ss.ThrowFromNativeCodeF(ex.ErrorCode(), "IStringBuilder.Replace: %s", ex.Message());
				return;
			}
		}

		char* out = sb.buffer;
		int64 w = startPosition; // The prefix before startPosition is already in place

		for (int32 i = startPosition; i < length; )
		{
			if (i <= length - fromLength && memcmp(original + i, from, (size_t)fromLength) == 0)
			{
				memcpy(out + w, to, (size_t)toLength);
				w += toLength;
				i += fromLength;
			}
			else
			{
				out[w++] = original[i++];
			}
		}

		out[w] = 0;
		sb.length = (int32)w;
	}

	void FastStringBuilderAppendAsDecimal(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);
		sb.formatBase = 10;
	}

	void FastStringBuilderAppendAsHex(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);
		sb.formatBase = 16;
	}

	void FastStringBuilderSetCase(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		int32 start, end;
		ReadInput(1, start, e);
		ReadInput(2, end, e);

		int32 isUpper;
		ReadInput(3, isUpper, e);

		if (!ValidateBuilder(e, sb, "SetCase")) return;

		if (end < 0) return;
		if (start >= sb.length) return;
		if (start < 0) start = 0;
		if (end >= sb.length) end = sb.length - 1;

		// toupper and tolower take an unsigned char value: a negative char (>= 128 as a byte) is undefined behaviour and asserts in the debug CRT
		if (isUpper != 0)
		{
			for (int i = start; i <= end; ++i)
			{
				sb.buffer[i] = (char) toupper((unsigned char) sb.buffer[i]);
			}
		}
		else
		{
			for (int i = start; i <= end; ++i)
			{
				sb.buffer[i] = (char) tolower((unsigned char) sb.buffer[i]);
			}
		}
	}

	void AlignedMalloc(NativeCallEnvironment& e)
	{
		IScriptSystem& ss = *(IScriptSystem*) e.context;

		int32 capacity;
		ReadInput(0, capacity, e); 

		int32 alignment;
		ReadInput(1, alignment, e); 

		void* alignedData = ss.AlignedMalloc(alignment, capacity);

		WriteOutput(0, alignedData, e);
	}

	void AlignedFree(NativeCallEnvironment& e)
	{	
		IScriptSystem& ss = *(IScriptSystem*) e.context;

		void* alignedData;
		ReadInput(0, alignedData, e);
		
		if (alignedData == NULL) return;

		ss.AlignedFree(alignedData);
	}


	void FastStringBuilderSetLength(NativeCallEnvironment& e)
	{	
		FastStringBuilder& sb = ReadBuilder(e);

		int32 length;
		ReadInput(1, length, e);

		if (length > sb.length)
		{
			// SetLength only truncates. Lengthening is a no-op (the method has no outputs, so nothing is written back)
			return;
		}
		else if (length >= 0)
		{
			sb.buffer[length] = 0;
			sb.length = length;
		}
		else
		{
			sb.buffer[0] = 0;
			sb.length = 0;
		}
	}

	void FastStringBuilderAppendSubstring(NativeCallEnvironment& e)
	{	
		FastStringBuilder& sb = ReadBuilder(e);

		cstr s;
		ReadInput(1, (void*&) s, e);

		int32 sLen;
		ReadInput(2, sLen, e);

		int32 startPos, charsToAppend;
		ReadInput(3, startPos, e);
		ReadInput(4, charsToAppend, e);

		sLen = SaneLength(s, sLen);

		if (startPos < 0) startPos = 0;
		if (startPos >= sLen) return;

		int32 available = sLen - startPos; // > 0 and no overflow, as 0 <= startPos < sLen
		if (charsToAppend < 0 || charsToAppend > available) charsToAppend = available;

		if (charsToAppend > 0)
		{
			AppendString(e, sb, s + startPos, charsToAppend);
		}
	}

	void FastStringBuilderAppendAsSpec(NativeCallEnvironment& e)
	{	
		FastStringBuilder& sb = ReadBuilder(e);

		int32 value;
		ReadInput(1, value, e);

		sb.spec = (SPEC) value;
	}

	void FastStringBuilderAppendChar(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		int32 asciiValue;
		ReadInput(1, asciiValue, e);

		// 0 would end the string, and values above 255 do not fit in a char. Both are ignored
		if (asciiValue > 0 && asciiValue <= 255)
		{
			char data[2];
			data[0] = (char) asciiValue;
			data[1] = 0;
			AppendString(e, sb, data, 1);
		}
	}

	// SetFormat allows widths and precisions up to 255, and a float64 in %f format can have over 300 digits before the point, so the buffers must be large
	enum { INT_REP_CAPACITY = 300, FLOAT_REP_CAPACITY = 1024 };

	void AppendFormatted(NativeCallEnvironment& e, FastStringBuilder& sb, cstr rep, int nChars)
	{
		if (nChars < 0)
		{
			e.ss.ThrowFromNativeCodeF(0, "IStringBuilder.Append: the formatted value did not fit the internal buffer. Check SetFormat");
			return;
		}

		AppendString(e, sb, rep, nChars);
	}

	void FastStringBuilderAppendInt32(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		int32 value;
		ReadInput(1, value, e);

		cstr suffix = sb.formatBase == 10 ? "d" : "X";

		char format[16];
		MergeFormats(format, sb, suffix);

		char rep[INT_REP_CAPACITY];
		int nChars = SafeFormat(rep, sizeof rep, format, value);
		AppendFormatted(e, sb, rep, nChars);
	}

	void FastStringBuilderAppendInt64(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		int64 value;
		ReadInput(1, value, e);

		cstr suffix = sb.formatBase == 10 ? "I64d" : "I64X";

		char format[16];
		MergeFormats(format, sb, suffix);

		char rep[INT_REP_CAPACITY];
		int nChars = SafeFormat(rep, sizeof rep, format, value);
		AppendFormatted(e, sb, rep, nChars);
	}

	void FastStringBuilderAppendFloat32(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		float32 value;
		ReadInput(1, value, e);

		cstr suffix;
		switch(sb.spec)
		{
		case SPEC_E:
			suffix = "e";
			break;
		case SPEC_G:
			suffix = "g";
			break;
		default: // F
			suffix = "f";
			break;
		}

		char format[16];
		MergeFormats(format, sb, suffix);

		char rep[FLOAT_REP_CAPACITY];
		int nChars = SafeFormat(rep, sizeof rep, format, value);
		AppendFormatted(e, sb, rep, nChars);
	}

	void FastStringBuilderAppendFloat64(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		float64 value;
		ReadInput(1, value, e);

		cstr suffix;
		switch(sb.spec)
		{
		case SPEC_E:
			suffix = "e";
			break;
		case SPEC_G:
			suffix = "g";
			break;
		default: // F
			suffix = "f";
			break;
		}

		char format[16];
		MergeFormats(format, sb, suffix);

		char rep[FLOAT_REP_CAPACITY];
		int nChars = SafeFormat(rep, sizeof rep, format, value);
		AppendFormatted(e, sb, rep, nChars);
	}

	void FastStringBuilderAppendBool(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		int32 value;
		ReadInput(1, value, e);

		if (value != 0) AppendString(e, sb, "true", 4);
		else			AppendString(e, sb, "false", 5);
	}

	void FastStringBuilderAppendPointer(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		void* value;
		ReadInput(1, value, e);

		// %X takes an unsigned int, which would drop the top 32 bits of a 64-bit pointer
		char rep[64];
		int nChars = SafeFormat(rep, sizeof rep, "0x%llX", (unsigned long long)(uintptr_t) value);
		AppendFormatted(e, sb, rep, nChars);
	}

	void FastStringBuilderSetFormat(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		int precision;
		ReadInput(1, precision, e);

		if (precision > 255) precision = 255;

		int width;
		ReadInput(2, width, e);

		if (width > 255) width = 255;

		boolean32 isZeroPrefixed; 
		ReadInput(3, isZeroPrefixed, e);

		boolean32 isRightAligned;
		ReadInput(4, isRightAligned, e);

		char *t = sb.prefix;

		// *t++ = '%';

		if (!isRightAligned) *t++ = '-';
		if (isZeroPrefixed) *t++ = '0';

		if (width >= 0)
		{	
			t += SafeFormat(t, PREFIX_LEN - (t - sb.prefix), "%d", width);
		}

		if (precision >= 0) 
		{
			t += SafeFormat(t, PREFIX_LEN - (t - sb.prefix), ".%d", precision);
		}

		*t = 0;
	}

	void FastStringBuilderStripLeft(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		int leftPos;
		ReadInput(1, leftPos, e);

		if (leftPos <= 0)
		{
			return;
		}

		if (!ValidateBuilder(e, sb, "StripLeft")) return;

		if (leftPos >= sb.length)
		{
			sb.length = 0;
			*sb.buffer = 0;
			return;
		}

		int32 newLength = sb.length - leftPos;
		memmove(sb.buffer, sb.buffer + leftPos, (size_t)newLength);
		sb.length = newLength;
		sb.buffer[sb.length] = 0;
	}

	void StringCompare(NativeCallEnvironment& e)
	{
		cstr s, t;

		int diff;

		ReadInput(0, (void*&) s, e);
		ReadInput(1, (void*&) t, e);

		if (s == NULL) 
		{
			diff = (t == NULL) ? 0 : -1;
		}
		else if (t == NULL)
		{
			diff = 1;
		}
		else
		{
			diff = Rococo::Strings::Compare(s, t);
		}

		WriteOutput(0, diff, e);
	}

	void AssertPascalCaseNamespace(NativeCallEnvironment& e)
	{
		InterfacePointer pInterf;

		ReadInput(0, (void*&)pInterf, e);

		int32 maxLength = 0;

		ReadInput(1, maxLength, e);

		auto* s = (InlineString*) InterfaceToInstance(pInterf);
		
		AssertPascalCaseNameValid(s->buffer, s->length, 256, "namespace");
	}

	void StringCompareI(NativeCallEnvironment& e)
	{
		cstr s, t;

		int diff;

		ReadInput(0, (void*&) s, e);
		ReadInput(1, (void*&) t, e);

		if (s == NULL) 
		{
			diff = (t == NULL) ? 0 : -1;
		}
		else if (t == NULL)
		{
			diff = 1;
		}
		else
		{
			diff = Rococo::Strings::CompareI(s, t);
		}

		WriteOutput(0, diff, e);
	}


	void StringFindLeft(NativeCallEnvironment& e)
	{
		cstr containerBuffer;
		ReadInput(0, (void*&) containerBuffer, e);

		int32 containerLength;
		ReadInput(1, containerLength, e);

		int32 startPos;
		ReadInput(2, startPos, e);

		cstr subStringBuffer;
		ReadInput(3, (void*&) subStringBuffer, e);

		int32 subStringLength;
		ReadInput(4, subStringLength, e);

		int32 caseIndependent;
		ReadInput(5, caseIndependent, e);

		int32 position = -1;

		// The lengths come from IString fields, which a script class sets for itself
		containerLength = SaneLength(containerBuffer, containerLength);
		subStringLength = SaneLength(subStringBuffer, subStringLength);

		if (startPos < 0) startPos = 0;

		if (startPos < containerLength && containerLength > 0 && subStringLength > 0 && subStringLength <= containerLength && containerBuffer != NULL && subStringBuffer != NULL)
		{
			cstr searchStart = containerBuffer + startPos;
			cstr searchEnd = containerBuffer + (containerLength - subStringLength); // Last position at which the substring fits, inclusive
			
			if (caseIndependent != 0)
			{
				for(cstr s = searchStart; s <= searchEnd; ++s)
				{
					if (CompareI(s, subStringBuffer, subStringLength) == 0)
					{
						position = (int32)(s - containerBuffer);
						break;
					}
				}
			}
			else
			{
				for(cstr s = searchStart; s <= searchEnd; ++s)
				{
					if (AreEqual(s, subStringBuffer, subStringLength))
					{
						position = (int32)(s - containerBuffer);
						break;
					}
				}
			}
		}
		
		WriteOutput(0, position, e);
	}

	void StringStartsWith(NativeCallEnvironment& e)
	{
		InterfacePointer ipBigString;
		ReadInput(0, ipBigString, e);

		InterfacePointer ipPrefixString;
		ReadInput(1, ipPrefixString, e);

		CStringConstant* bigString = (CStringConstant*)InterfaceToInstance(ipBigString);
		CStringConstant* prefixString = (CStringConstant*)InterfaceToInstance(ipPrefixString);

		cstr big = bigString->pointer;
		cstr prefix = prefixString->pointer;

		bool result;

		if (big && prefix)
		{
			result = StartsWith(big, prefix);
		}
		else
		{
			result = false;
		}

		WriteOutput(0, result ? 1 : 0, e);
	}

	void StringEndsWith(NativeCallEnvironment& e)
	{
		InterfacePointer ipBigString;
		ReadInput(0, ipBigString, e);

		InterfacePointer ipSuffixString;
		ReadInput(1, ipSuffixString, e);

		CStringConstant* bigString = (CStringConstant*)InterfaceToInstance(ipBigString);
		CStringConstant* suffixString = (CStringConstant*)InterfaceToInstance(ipSuffixString);

		cstr big = bigString->pointer;
		cstr suffix = suffixString->pointer;

		bool result;

		if (big && suffix)
		{
			result = EndsWith(big, suffix);
		}
		else
		{
			result = false;
		}

		WriteOutput(0, result ? 1 : 0, e);
	}


	void StringFindRight(NativeCallEnvironment& e)
	{
		// Say we have a string 1245BILL6789 - which is 12 chars in length and we have a search string of BILL
		// Lets say left pos is 2 and right pos is 7, then we are looking at the string 45BILL6789 with the right pos on the 6
		// We want to work backwards from 6789 to 45BILL6789 until we hit BILL, then return positin of BILL with 0 indexing the digit 1
		cstr containerBuffer;
		ReadInput(0, (void*&) containerBuffer, e);

		int32 containerLength;
		ReadInput(1, containerLength, e);

		int32 leftPos;
		ReadInput(2, leftPos, e);

		int32 rightPos;
		ReadInput(3, rightPos, e);

		cstr subStringBuffer;
		ReadInput(4, (void*&) subStringBuffer, e);

		int32 subStringLength;
		ReadInput(5, subStringLength, e);

		int32 caseIndependent;
		ReadInput(6, caseIndependent, e);

		int32 position = -1;

		// The lengths come from IString fields, which a script class sets for itself
		containerLength = SaneLength(containerBuffer, containerLength);
		subStringLength = SaneLength(subStringBuffer, subStringLength);

		if (rightPos >= 0 && containerLength > 0 && subStringLength > 0 && subStringLength <= containerLength && containerBuffer != NULL && subStringBuffer != NULL)
		{
			if (rightPos >= containerLength) rightPos = containerLength - 1;

			// A negative left position means search from the start of the string. It used to be clamped to 0 and then skip the search altogether
			if (leftPos < 0) leftPos = 0;

			if (leftPos < rightPos)
			{
				int32 lastFit = containerLength - subStringLength; // The substring cannot start any further right than this
				int32 startIndex = rightPos < lastFit ? rightPos : lastFit;

				cstr searchStart = containerBuffer + startIndex;
				cstr searchEnd = containerBuffer + leftPos;

				if (caseIndependent != 0)
				{
					for (cstr s = searchStart; s >= searchEnd; --s)
					{
						if (CompareI(s, subStringBuffer, subStringLength) == 0)
						{
							position = (int32)(s - containerBuffer);
							break;
						}
					}
				}
				else
				{
					for (cstr s = searchStart; s >= searchEnd; --s)
					{
						if (AreEqual(s, subStringBuffer, subStringLength))
						{
							position = (int32)(s - containerBuffer);
							break;
						}
					}
				}
			}
		}
		
		WriteOutput(0, position, e);
	}
}

namespace SysTypeStrings
{
	void IsUpperCase(NativeCallEnvironment& e)
	{
		int32 asciiChar;
		ReadInput(0, asciiChar, e);

		boolean32 isSo = (asciiChar >= 'A' && asciiChar <= 'Z');
		WriteOutput(0, isSo, e);
	}

	void IsLowerCase(NativeCallEnvironment& e)
	{
		int32 asciiChar;
		ReadInput(0, asciiChar, e);

		boolean32 isSo = (asciiChar >= 'a' && asciiChar <= 'z');
		WriteOutput(0, isSo, e);
	}

	void IsAlpha(NativeCallEnvironment& e)
	{
		int32 asciiChar;
		ReadInput(0, asciiChar, e);

		boolean32 isSo = (asciiChar >= 'a' && asciiChar <= 'z') || (asciiChar >= 'A' && asciiChar <= 'Z');
		WriteOutput(0, isSo, e);
	}

	void IsNumeric(NativeCallEnvironment& e)
	{
		int32 asciiChar;
		ReadInput(0, asciiChar, e);

		boolean32 isSo = (asciiChar >= '0' && asciiChar <= '9');
		WriteOutput(0, isSo, e);
	}

	void IsAlphaNumeric(NativeCallEnvironment& e)
	{
		int32 asciiChar;
		ReadInput(0, asciiChar, e);

		boolean32 isSo = (asciiChar >= 'a' && asciiChar <= 'z') || (asciiChar >= 'A' && asciiChar <= 'Z') || (asciiChar >= '0' && asciiChar <= '9');
		WriteOutput(0, isSo, e);
	}
}

namespace Rococo
{
	namespace Script
	{
		IStringPool* NewStringPool()
		{
			return new Rococo::Strings::StringPool();
		}
	}
}
