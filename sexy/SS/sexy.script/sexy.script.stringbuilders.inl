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

namespace
{
	template<uint32 capacity>
	struct Memo
	{
		enum { CAPACITY = capacity };
		char Data[CAPACITY];
	};

	template<class T>
	class TFastAllocator
	{
	private:
		typedef std::vector<T*> TItems;
		TItems items;
		TItems freeItems;

	public:
		~TFastAllocator()
		{
			clear();
		}

		void clear()
		{
			for(auto i = items.begin(); i != items.end(); ++i)
			{
				delete *i;
			}

			items.clear();
			freeItems.clear();
		}

		T* alloc()
		{
			T* item;

			if (freeItems.empty())
			{
				item = new T();
			}
			else
			{
				item = freeItems.back();
				items.push_back(item);
				freeItems.pop_back();
			}

			return item;
		}

		void free(T* item)
		{
			freeItems.push_back(item);
		}
	};

	typedef Memo<128> TMemo;

	typedef TFastAllocator<TMemo> TMemoAllocator;

	void CreateMemoString(NativeCallEnvironment& e)
	{
		TMemoAllocator& allocator = *(TMemoAllocator*) e.context;

		void* vSrc;
		ReadInput(0, vSrc, e);

		const char* src = (const char*) vSrc;

		int32 srcLen;
		ReadInput(1, srcLen, e);

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

	void MergeFormats(char* format, const FastStringBuilder& sb, cstr suffix)
	{
		CopyString(format, 16, sb.prefix);
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

		const IStructure* typeFastStringBuilder;

		std::vector<FastStringBuilder_64*, Memory::SexyAllocator<FastStringBuilder_64*>> freeList_64; // For tokens and such. Could be smaller, but then dwarfed by the FastStringBuilder object
		std::vector<FastStringBuilder_260*, Memory::SexyAllocator<FastStringBuilder_260*>> freeList_260; // For file paths and whatever
		std::vector<FastStringBuilder_1024*, Memory::SexyAllocator<FastStringBuilder_1024*>> freeList_1024; // Paragraphs
		std::unordered_map<char*,uint32,std::hash<char*>, std::equal_to<char*>, Memory::SexyAllocator<std::pair<char* const, uint32>>> allocList;

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
					allocList[(char*)sb64] = 0;
					return &sb64->header;
				}
				else
				{
					auto* tail = freeList_64.back();
					freeList_64.pop_back();
					return &tail->header;
				}
			}
			else if (capacity == 260)
			{
				if (freeList_260.empty())
				{
					auto* sb260 = new FastStringBuilder_260();
					allocList[(char*)sb260] = 0;
					return &sb260->header;
				}
				else
				{
					auto* tail = freeList_260.back();
					freeList_260.pop_back();
					return &tail->header;
				}
			}
			else if (capacity == 1024)
			{
				if (freeList_1024.empty())
				{
					auto* sb1024 = new FastStringBuilder_1024();
					allocList[(char*)sb1024] = 0;
					return &sb1024->header;
				}
				else
				{
					auto* tail = freeList_1024.back();
					freeList_1024.pop_back();
					return &tail->header;
				}
			}
			else
			{
				auto* sb = (FastStringBuilder*) new char[sizeof(FastStringBuilder) + capacity];
				allocList[(char*)sb] = 0;
				return sb;
			}
		}

		FastStringBuilder* CreateAndInitFields(int32 capacity) override
		{
			auto* sb = Create(capacity);

			sb->buffer = ((char*)sb) + sizeof(FastStringBuilder);
			sb->buffer[0] = 0;
			sb->capacity = capacity;
			sb->length = 0;
			sb->formatBase = 10;
			sb->spec = SPEC_F;
			sb->stub.Desc = (ObjectDesc*) typeFastStringBuilder->GetVirtualTable(0);
			sb->stub.refCount = 1;
			sb->stub.pVTables[0] = (VirtualTable*) typeFastStringBuilder->GetVirtualTable(1);
			sb->flags = 0;
			SafeFormat(sb->prefix, 8, "%s", "%4.4");

			return sb;
		}
	};

	void NewStringBuilder(NativeCallEnvironment& e)
	{
		StringPool& pool = *(StringPool*)e.context;

		int32 capacity;
		ReadInput(0, capacity, e);

		if (capacity <= 0) 
		{
			e.ss.ThrowFromNativeCode(0, "NewStringBuilder failed. Capacity needs to be positive");
			return;
		}

		if (capacity >= 1024_megabytes)
		{
			e.ss.ThrowFromNativeCode(0, "NewStringBuilder failed. Capacity needs to be less than 1 gigabyte");
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
			delete[](char*) sb;
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
				e.ss.ThrowFromNativeCode(0, "Insufficient buffer capacity for append operation");
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
			Rococo::OS::ToSysPath(sb.buffer);
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
			e.ss.ThrowFromNativeCode(0, "FastStringBuilderReplace: (IString from) - from was blank");
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
				e.ss.ThrowFromNativeCode(0, "FastStringBuilderReplace: Insufficient space in buffer to replace character sequence");
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

		if (precision > 9) precision = 9;

		int width;
		ReadInput(2, width, e);

		if (width > 9) precision = 9;

		int isZeroPrefixed; 
		ReadInput(3, isZeroPrefixed, e);

		int isRightAligned;
		ReadInput(4, isRightAligned, e);

		char *t = sb.prefix;

		*t++ = '%';

		if (isRightAligned == 0) *t++ = '-';
		if (isZeroPrefixed != 0) *t++ = '0';
		if (width >= 0) *t++ = ('0' + width);
		if (precision >= 0) 
		{
			*t++ = '.';
			*t++ = ('0' + precision);
		}

		*t = 0;
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
			diff = Rococo::Compare(s, t);
		}

		WriteOutput(0, diff, e);
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
			diff = Rococo::CompareI(s, t);
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
		cstr containerBuffer;
		ReadInput(0, (void*&) containerBuffer, e);

		int32 containerLength;
		ReadInput(1, containerLength, e);

		int32 rightPos;
		ReadInput(2, rightPos, e);

		cstr subStringBuffer;
		ReadInput(3, (void*&) subStringBuffer, e);

		int32 subStringLength;
		ReadInput(4, subStringLength, e);

		int32 caseIndependent;
		ReadInput(5, caseIndependent, e);

		int32 position = -1;

		if (rightPos < 0) rightPos = 0;
		if (rightPos >= containerLength) rightPos = containerLength-1; 

		if (containerLength > 0 && subStringLength > 0 && containerBuffer != NULL && subStringBuffer != NULL)
		{
			cstr searchStart = containerBuffer + rightPos;
			cstr searchEnd = containerBuffer;
			
			if (caseIndependent != 0)
			{
				for(cstr s = searchStart; s >= searchEnd; --s)
				{
					if (CompareI(s, subStringBuffer, subStringLength))
					{
						position = (int32)(s - containerBuffer);
						break;
					}
				}
			}
			else
			{
				for(cstr s = searchStart; s >= searchEnd; --s)
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
}

namespace Rococo
{
	namespace Script
	{
		IStringPool* NewStringPool()
		{
			return new StringPool();
		}
	}
}