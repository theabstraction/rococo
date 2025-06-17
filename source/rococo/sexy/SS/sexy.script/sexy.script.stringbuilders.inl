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

#pragma pack(push,1)
	struct FastStringBuilder_64
	{
		FastStringBuilder header;
		char rawData[64];

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS;
	};

	struct FastStringBuilder_260
	{
		FastStringBuilder header;
		char rawData[260];

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS;
	};

	struct FastStringBuilder_1024
	{
		FastStringBuilder header;
		char rawData[1024];

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS;
	};
#pragma pack(pop)

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
			return nullptr;
		}

		void FreeObject(void* pMemory) override
		{
			
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
			return 0;
		}
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

		const IStructure* typeFastStringBuilder;

		TSexyVector<FastStringBuilder_64*> freeList_64; // For tokens and such. Could be smaller, but then dwarfed by the FastStringBuilder object
		TSexyVector<FastStringBuilder_260*> freeList_260; // For file paths and whatever
		TSexyVector<FastStringBuilder_1024*> freeList_1024; // Paragraphs
		TSexyHashMap<char*,uint32> allocList;

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
			for (auto i : allocList)
			{
				delete[] i.first;
			}
			delete this;
		}

		FastStringBuilder* Create(int32 capacity)
		{
			if (capacity == 64)
			{
				if (freeList_64.empty())
				{
					auto* sb64 = new FastStringBuilder_64();
					sb64->header.buffer = ((char*)sb64) + sizeof(FastStringBuilder);
					allocList[(char*)sb64] = 0;
					return &sb64->header;
				}
				else
				{
					auto* tail = freeList_64.back();
					freeList_64.pop_back();
					tail->header.buffer = ((char*)tail) + sizeof(FastStringBuilder);
					return &tail->header;
				}
			}
			else if (capacity == 260)
			{
				if (freeList_260.empty())
				{
					auto* sb260 = new FastStringBuilder_260();
					allocList[(char*)sb260] = 0;
					sb260->header.buffer = ((char*)sb260) + sizeof(FastStringBuilder);
					return &sb260->header;
				}
				else
				{
					auto* tail = freeList_260.back();
					freeList_260.pop_back();
					tail->header.buffer = ((char*)tail) + sizeof(FastStringBuilder);
					return &tail->header;
				}
			}
			else if (capacity == 1024)
			{
				if (freeList_1024.empty())
				{
					auto* sb1024 = new FastStringBuilder_1024();
					allocList[(char*)sb1024] = 0;
					sb1024->header.buffer = ((char*)sb1024) + sizeof(FastStringBuilder);
					return &sb1024->header;
				}
				else
				{
					auto* tail = freeList_1024.back();
					freeList_1024.pop_back();
					tail->header.buffer = ((char*)tail) + sizeof(FastStringBuilder);
					return &tail->header;
				}
			}
			else
			{
				auto* sb = new FastStringBuilder();
				sb->flags |= (int32) StringBuilderFlags::Expandable;
				allocList[(char*)sb] = 0;
				sb->buffer = new char[capacity];
				return sb;
			}
		}

		FastStringBuilder* CreateAndInitFields(int32 capacity) override
		{
			auto* sb = Create(capacity);
			sb->buffer[0] = 0;
			sb->capacity = capacity;
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

		void ExpandStringBuilder(FastStringBuilder& sb, size_t deltaLength) override
		{
			uint64 len = sb.length;

			if (!HasFlag(StringBuilderFlags::Expandable, sb.flags))
			{
				// Was of reserved length (64, 260 or 1024 chars)
				Throw(0, "%s: The string builder was not expandable", __ROCOCO_FUNCTION__);
			}

			if (deltaLength >= 1_gigabytes)
			{
				Throw(0, "%s: expansion is capped to 1GB", __ROCOCO_FUNCTION__);
			}

			uint64 targetLen = deltaLength + len;

			if (targetLen >= 1_gigabytes)
			{
				Throw(0, "%s: expansion is capped to 1GB", __ROCOCO_FUNCTION__);
			}

			// At this point the target length is guaranteed to be less than 1GB, so an int32 cast is guaranteed to be a +ve int32. capacity is also limited to 1GB.

			int32 newLen = (int32) targetLen;

			int32 newCapacity = sb.capacity;

			while (newCapacity < newLen)
			{
				if (newCapacity >= 1_gigabytes)
				{
					newCapacity = (int) 1_gigabytes;
					break;
				}

				newCapacity *= 2;
			}

			// since capacity maxes out at 1GB and the length is guaranteed to be less than 1GB, then length < capacity, so we always have a spot for the trailing nul character

			sb.capacity = newCapacity;

			char* newBuffer;

			try
			{
				newBuffer = new char[newCapacity];
			}
			catch (...)
			{
				Throw(0, "%s: expansion failed to allocate %ld bytes", __ROCOCO_FUNCTION__, newCapacity);
			}

			memcpy(newBuffer, sb.buffer, sb.length + 1); // We added 1 to cover the trailing zero

			delete[] sb.buffer;
			sb.buffer = newBuffer;

			// Note, that it is not our job here to update the length, we merely updated the capacity to allow the length to be legal
		}
	};

	void NewStringBuilder(NativeCallEnvironment& e)
	{
		StringPool& pool = *(StringPool*)e.context;

		int32 capacity;
		ReadInput(0, capacity, e);

		if (capacity <= 0) 
		{
			e.ss.ThrowFromNativeCodeF(0, "NewStringBuilder failed. Capacity must be positive");
			return;
		}

		if (capacity > 1024_megabytes)
		{
			e.ss.ThrowFromNativeCodeF(0, "NewStringBuilder failed. Capacity must be no greater than 1 gigabyte");
			return;
		}

		FastStringBuilder* sb = pool.CreateAndInitFields(capacity);

		WriteOutput(0, (void*)&sb->stub.pVTables[0], e);
	}

	void DestructStringBuilder(NativeCallEnvironment& e)
	{
		StringPool& pool = *(StringPool*)e.context;

		InterfacePointer p;
		ReadInput(0, p, e);

		auto* stub = InterfaceToInstance(p);
		auto* sb = (FastStringBuilder*) stub;

		sb->buffer[0] = 0;
		sb->length = 0;

#ifdef SEXY_ENABLE_EXTRA_STRING_SECURITY
		memset(sb->buffer, 0, sb->capacity); // For security we could erase buffer, to stop re-use exposing old contents
#endif

		sb->spec = SPEC_F;
		sb->formatBase = 10;
		
		switch (sb->capacity)
		{
		case 64:
			pool.freeList_64.push_back((FastStringBuilder_64*) sb);
			break;
		case 260:
			pool.freeList_260.push_back((FastStringBuilder_260*)sb);
			break;
		case 1024:
			pool.freeList_1024.push_back((FastStringBuilder_1024*)sb);
			break;
		default:
			delete[] sb->buffer;
			delete sb;
			pool.allocList.erase((char*)sb);
			break;
		}
	}

	FastStringBuilder& ReadBuilder(NativeCallEnvironment& e)
	{
		InterfacePointer p;
		ReadInput(0, (void*&) p, e);

		auto* stub = InterfaceToInstance(p);
		auto* sb = (FastStringBuilder*)stub;
		return *sb;
	}

	void AppendString(NativeCallEnvironment& e, FastStringBuilder& sb, const char* src, int32 srcLen)
	{
		int maxLength = sb.capacity - 1;

		int nextLength = sb.length + srcLen;
		if (nextLength > maxLength)
		{
			if (0 != (sb.flags & (int32)StringBuilderFlags::ThrowIfWouldTruncate))
			{
				e.ss.ThrowFromNativeCodeF(0, "IStringBuilder.Append: Insufficient buffer capacity for append operation and truncation forbidden");
				return;
			}

			int overrun = nextLength - maxLength;
			srcLen -= overrun;
		}

		if (srcLen > 0)
		{
			char* target = sb.buffer + sb.length;

			int i;
			for (i = 0; i < srcLen; ++i)
			{
				target[i] = src[i];
			}

			target[i] = 0;

			sb.length += srcLen;
		}
	}

	void FastStringBuilderAppendIString(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		const char* srcBuffer;
		ReadInput(1, srcBuffer, e);

		int32 srcLength;
		ReadInput(2, srcLength, e);

		if (srcBuffer != NULL && srcLength > 0)
		{
			AppendString(e, sb, srcBuffer, srcLength);
		}
	}

	void FastStringBuilderThrowIfAppendWouldTruncate(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);
		sb.flags |= (int32) StringBuilderFlags::ThrowIfWouldTruncate;
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
		for (int i = 0; i < sb.length; i++)
		{
			Rococo::IO::ToSysPath(sb.buffer);
		}
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

		auto* sfrom = (InlineString*)InterfaceToInstance(ipFrom);
		auto* sto = (InlineString*)InterfaceToInstance(ipTo);

		if (startPosition < 0)
		{
			startPosition = 0;
		}

		if (startPosition >= sb.length)
		{
			// No work to do
			return;
		}

		if (sfrom->length == 0)
		{
			e.ss.ThrowFromNativeCodeF(0, "IStringBuilder.Replace: (IString from) - from was blank");
			return;
		}

		char* nextToken = sb.buffer + startPosition;

		int lenDelta = sfrom->length - sto->length;

		const char* end = sb.buffer + sb.length;

		for (;;)
		{
			nextToken = strstr(nextToken, sfrom->buffer);
			if (nextToken == nullptr)
			{
				return;
			}

			int64 charsToMove = 1 + end - nextToken - sfrom->length;

			int nextLength = sb.length - lenDelta;
			if (nextLength >= sb.capacity)
			{
				// Insufficient space to perform replacement
				e.ss.ThrowFromNativeCodeF(0, "IStringBuilder.Replace: Insufficient space in buffer to replace character sequence");
				return;
			}

			memmove(nextToken + sto->length, nextToken + sfrom->length, charsToMove);
			memcpy(nextToken, sto->buffer, sto->length);

			end -= lenDelta;
			sb.length = nextLength;
			nextToken += sto->length;
		}
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

		if (end < 0) return;
		if (start >= sb.length) return;
		if (start < 0) start = 0;
		if (end >= sb.length) end = sb.length - 1;

		if (isUpper != 0)
		{
			for (int i = start; i <= end; ++i)
			{
				sb.buffer[i] = toupper(sb.buffer[i]);
			}
		}
		else
		{
			for (int i = start; i <= end; ++i)
			{
				sb.buffer[i] = tolower(sb.buffer[i]);
			}
		}
	}

	void AlignedMalloc(NativeCallEnvironment& e)
	{
		IScriptSystem& ss = *(IScriptSystem*) e.context;

		TAllocationMap& allocMap = *(TAllocationMap*) e.context;

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
			WriteOutput(0, sb.length, e);
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

		if (startPos < 0) startPos = 0;
		if (charsToAppend < 0) charsToAppend = sLen;

		int endPos = startPos + charsToAppend;
		int overspill = endPos - sLen;
		if (overspill > 0)
		{
			charsToAppend -= overspill;
			if (charsToAppend < 0) charsToAppend = 0;
		}

		if (s != NULL && charsToAppend > 0)
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

		if (asciiValue > 0)
		{
			char data[2];
			data[0] = asciiValue;
			data[1] = 0;
			AppendString(e, sb, data, 1);
		}
	}

	void FastStringBuilderAppendInt32(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		int32 value;
		ReadInput(1, value, e);

		cstr suffix = sb.formatBase == 10 ? "d" : "X";

		char format[16];
		MergeFormats(format, sb, suffix);

		char rep[32];
		int nChars = SafeFormat(rep, 32, format, value);
		AppendString(e, sb, rep, nChars);
	}

	void FastStringBuilderAppendInt64(NativeCallEnvironment& e)
	{
		FastStringBuilder& sb = ReadBuilder(e);

		int64 value;
		ReadInput(1, value, e);

		cstr suffix = sb.formatBase == 10 ? "I64d" : "I64X";

		char format[16];
		MergeFormats(format, sb, suffix);

		char rep[64];
		int nChars = SafeFormat(rep, 64, format, value);
		AppendString(e, sb, rep, nChars);
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

		char rep[32];
		int nChars = SafeFormat(rep, 32, format, value);
		AppendString(e, sb, rep, nChars);
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

		char rep[64];
		int nChars = SafeFormat(rep, 64, format, value);
		AppendString(e, sb, rep, nChars);
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

		char rep[64];
		int nChars = SafeFormat(rep, 64, "0x%X", value);
		AppendString(e, sb, rep, nChars);
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

		if (leftPos < 0)
		{
			return;
		}

		if (leftPos >= sb.length)
		{
			sb.length = 0;
			if (sb.capacity) *sb.buffer = 0;
		}

		for (char* target = sb.buffer; target < sb.buffer + (sb.length - leftPos); target++)
		{
			*target = target[leftPos];
		}

		sb.length -= leftPos;
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

		if (startPos < 0) startPos = 0;

		if (startPos < containerLength && containerLength > 0 && subStringLength > 0 && containerBuffer != NULL && subStringBuffer != NULL)
		{
			cstr searchStart = containerBuffer + startPos;
			cstr searchEnd = containerBuffer + containerLength;
			
			if (caseIndependent != 0)
			{
				for(cstr s = searchStart; s < searchEnd; ++s)
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
				for(cstr s = searchStart; s < searchEnd; ++s)
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

		if (rightPos < 0 || containerLength <= 0 || subStringLength <= 0 || containerBuffer == NULL || subStringBuffer == NULL)
		{
			position = -1;
		}
		else
		{
			if (rightPos >= containerLength) rightPos = containerLength - 1;

			if (leftPos < 0)
			{
				leftPos = 0;
			}
			else if (leftPos >= rightPos)
			{
				position = -1;
			}
			else
			{
				cstr searchStart = containerBuffer + rightPos;
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