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

namespace
{
	template<uint32 capacity>
	struct Memo
	{
		enum { CAPACITY = capacity };
		SEXCHAR Data[CAPACITY];
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

		const SEXCHAR* src = (const SEXCHAR*) vSrc;

		int32 srcLen;
		ReadInput(1, srcLen, e);

		if (src == NULL)
		{
			src = SEXTEXT("");
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
		memcpy(memo->Data, src, sizeof(SEXCHAR) * truncLen);
		memo->Data[sizeof(SEXCHAR) * truncLen] = 0;

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

	enum SPEC
	{
		SPEC_E = 1,
		SPEC_F = 2,
		SPEC_G = 3,
	};

#pragma pack(push,1)
	struct CStringBuilderHeader
	{
		int capacity;
		int length;
		int formatBase;
		SPEC spec;
		SEXCHAR prefix[16];

		CStringBuilderHeader(int _capacity): capacity(_capacity), length(0), formatBase(10), spec(SPEC_F)
		{
			prefix[0] = '%';
			prefix[1] = 0;
		}
	};

	struct CStringBuilder
	{		
		CStringBuilderHeader Header;
		CStringBuilder(int capacity): Header(capacity)
		{
			buffer[0] = 0;
		}

		SEXCHAR buffer[4];
	};
#pragma pack(pop)
	void MergeFormats(SEXCHAR* format, const CStringBuilder& sb, csexstr suffix)
	{
		CopyString(format, 16, sb.Header.prefix);
		StringCat(format, suffix, 16);
	}

	typedef std::tr1::unordered_map<CStringBuilder*,int> TStringBuilders;

	void Clear(TStringBuilders& z)
	{
		for(auto i = z.begin(); i != z.end(); ++i)
		{
			delete i->first;
		}

		z.clear();
	}

	void CreateStringBuilder(NativeCallEnvironment& e)
	{
		
		TStringBuilders& sbStore = *(TStringBuilders*) e.context;

		int32 capacity;
		ReadInput(0, capacity, e);

		SEXCHAR* pData = new SEXCHAR[capacity + sizeof(CStringBuilderHeader)];
		CStringBuilder* sb = new (pData) CStringBuilder(std::max(capacity, 4));
		sbStore.insert(std::make_pair(sb, 0));

		WriteOutput(0, (void*) sb->buffer, e);
	}

	CStringBuilder* ReadStringBuilder(NativeCallEnvironment& e, int input)
	{
		uint8* buffer;
		ReadInput(0, (void*&) buffer, e);
		CStringBuilder* sb = (CStringBuilder*) (buffer - sizeof(CStringBuilderHeader));
		return sb;
	}

	void FreeStringBuilder(NativeCallEnvironment& e)
	{
		
		TStringBuilders& sbStore = *(TStringBuilders*) e.context;

		CStringBuilder* sb = ReadStringBuilder(e, 0);
		delete (SEXCHAR*) sb;

		sbStore.erase(sb);
	}

	void AppendString(CStringBuilder& sb, csexstr src, int length)
	{
		int maxLength = sb.Header.capacity-1;

		int nextLength = sb.Header.length + length;
		if (nextLength > maxLength)
		{
			int overrun = nextLength - maxLength;
			length -= overrun;			
		}

		if (length > 0)
		{
			SEXCHAR* target = sb.buffer + sb.Header.length;

			int i;
			for(i = 0; i < length; ++i)
			{
				target[i] = src[i];
			}

			target[i] = 0;

			sb.Header.length += length;
		}
	}

	void StringBuilderAppendIString(NativeCallEnvironment& e)
	{
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		csexstr src;
		ReadInput(1, (void*&) src, e);

		int32 srcLength;
		ReadInput(2, srcLength, e);

		if (src == NULL) { src = SEXTEXT("<null>"); srcLength = 6; }

		if (srcLength > 0) AppendString(*sb, src, srcLength);

		WriteOutput(0, sb->Header.length, e);
	}

	void StringBuilderClear(NativeCallEnvironment& e)
	{
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		sb->Header.length = 0;
		sb->buffer[0] = 0;
	}

	void StringBuilderAppendAsDecimal(NativeCallEnvironment& e)
	{
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		sb->Header.formatBase = 10;
	}

	void StringBuilderAppendAsHex(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		sb->Header.formatBase = 16;
	}

	void StringBuilderSetCase(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		int32 start, end;
		ReadInput(1, start, e);
		ReadInput(2, end, e);

		int32 isUpper;
		ReadInput(3, isUpper, e);

		if (end < 0) return;
		if (start >= sb->Header.length) return;
		if (start < 0) start = 0;
		if (end >= sb->Header.length) end = sb->Header.length-1;

		if (isUpper != 0)
		{
			for(int i = start; i <= end; ++i)
			{
				sb->buffer[i] = toupper(sb->buffer[i]);
			}
		}
		else
		{
			for(int i = start; i <= end; ++i)
			{
				sb->buffer[i] = tolower(sb->buffer[i]);
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

	void StringBuilderSetLength(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		int32 length;
		ReadInput(1, length, e);

		if (length > sb->Header.length)
		{
			WriteOutput(0, sb->Header.length, e);
		}
		else if (length >= 0)
		{
			sb->buffer[length] = 0;
			sb->Header.length = length;
		}
		else
		{
			sb->buffer[0] = 0;
			sb->Header.length = 0;
		}

		WriteOutput(0, sb->Header.length, e);
	}

	void StringBuilderAppendSubstring(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		csexstr s;
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

		if (s == NULL && charsToAppend > 0) AppendString(*sb, SEXTEXT("<null>"), 6);
		else if (charsToAppend > 0)
		{
			AppendString(*sb, s + startPos, charsToAppend);
		}

		WriteOutput(0, sb->Header.length, e);
	}

	void StringBuilderAppendAsSpec(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		int32 value;
		ReadInput(1, value, e);

		sb->Header.spec = (SPEC) value;
	}

	void StringBuilderAppendInt32(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		int32 value;
		ReadInput(1, value, e);

		csexstr suffix = sb->Header.formatBase == 10 ? SEXTEXT("d") : SEXTEXT("X");

		SEXCHAR format[16];
		MergeFormats(format, *sb, suffix);

		SEXCHAR rep[32];
		int nChars = StringPrint(rep, 32, format, value);
		AppendString(*sb, rep, nChars);
		WriteOutput(0, sb->Header.length, e);
	}

	void StringBuilderAppendInt64(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		int64 value;
		ReadInput(1, value, e);

		csexstr suffix = sb->Header.formatBase == 10 ? SEXTEXT("I64d") : SEXTEXT("I64X");

		SEXCHAR format[16];
		MergeFormats(format, *sb, suffix);

		SEXCHAR rep[64];
		int nChars = StringPrint(rep, 64, format, value);
		AppendString(*sb, rep, nChars);
		WriteOutput(0, sb->Header.length, e);
	}

	void StringBuilderAppendFloat32(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		float32 value;
		ReadInput(1, value, e);

		csexstr suffix;
		switch(sb->Header.spec)
		{
		case SPEC_E:
			suffix = SEXTEXT("e");
			break;
		case SPEC_G:
			suffix = SEXTEXT("g");
			break;
		default: // F
			suffix = SEXTEXT("f");
			break;
		}

		SEXCHAR format[16];
		MergeFormats(format, *sb, suffix);

		SEXCHAR rep[32];
		int nChars = StringPrint(rep, 32, format, value);
		AppendString(*sb, rep, nChars);
		WriteOutput(0, sb->Header.length, e);
	}

	void StringBuilderAppendFloat64(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		float64 value;
		ReadInput(1, value, e);

		csexstr suffix;
		switch(sb->Header.spec)
		{
		case SPEC_E:
			suffix = SEXTEXT("le");
			break;
		case SPEC_G:
			suffix = SEXTEXT("lg");
			break;
		default: // F
			suffix = SEXTEXT("lf");
			break;
		}

		SEXCHAR format[16];
		MergeFormats(format, *sb, suffix);

		SEXCHAR rep[64];
		int nChars = StringPrint(rep, 64, format, value);
		AppendString(*sb, rep, nChars);
		WriteOutput(0, sb->Header.length, e);
	}

	void StringBuilderAppendBool(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		int32 value;
		ReadInput(1, value, e);

		if (value != 0) AppendString(*sb, SEXTEXT("true"), 4);
		else			AppendString(*sb, SEXTEXT("false"), 5);

		WriteOutput(0, sb->Header.length, e);
	}

	void StringBuilderAppendPointer(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

		void* value;
		ReadInput(1, value, e);

		SEXCHAR rep[64];
		int nChars = StringPrint(rep, 64, SEXTEXT("0x%X"), value);
		AppendString(*sb, rep, nChars);
		
		WriteOutput(0, sb->Header.length, e);
	}

	void StringBuilderSetFormat(NativeCallEnvironment& e)
	{
		
		CStringBuilder* sb = ReadStringBuilder(e, 0);

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

		SEXCHAR *t = sb->Header.prefix;

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
		csexstr s, t;

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
			diff = Sexy::Compare(s, t);
		}

		WriteOutput(0, diff, e);
	}

	void StringCompareI(NativeCallEnvironment& e)
	{
		csexstr s, t;

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
			diff = Sexy::CompareI(s, t);
		}

		WriteOutput(0, diff, e);
	}

	void StringFindLeft(NativeCallEnvironment& e)
	{
		csexstr containerBuffer;
		ReadInput(0, (void*&) containerBuffer, e);

		int32 containerLength;
		ReadInput(1, containerLength, e);

		int32 startPos;
		ReadInput(2, startPos, e);

		csexstr subStringBuffer;
		ReadInput(3, (void*&) subStringBuffer, e);

		int32 subStringLength;
		ReadInput(4, subStringLength, e);

		int32 caseIndependent;
		ReadInput(5, caseIndependent, e);

		int32 position = -1;

		if (startPos < 0) startPos = 0;

		if (startPos < containerLength && containerLength > 0 && subStringLength > 0 && containerBuffer != NULL && subStringBuffer != NULL)
		{
			csexstr searchStart = containerBuffer + startPos;
			csexstr searchEnd = containerBuffer + containerLength;
			
			if (caseIndependent != 0)
			{
				for(csexstr s = searchStart; s < searchEnd; ++s)
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
				for(csexstr s = searchStart; s < searchEnd; ++s)
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

	void StringFindRight(NativeCallEnvironment& e)
	{
		csexstr containerBuffer;
		ReadInput(0, (void*&) containerBuffer, e);

		int32 containerLength;
		ReadInput(1, containerLength, e);

		int32 rightPos;
		ReadInput(2, rightPos, e);

		csexstr subStringBuffer;
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
			csexstr searchStart = containerBuffer + rightPos;
			csexstr searchEnd = containerBuffer;
			
			if (caseIndependent != 0)
			{
				for(csexstr s = searchStart; s >= searchEnd; --s)
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
				for(csexstr s = searchStart; s >= searchEnd; --s)
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
