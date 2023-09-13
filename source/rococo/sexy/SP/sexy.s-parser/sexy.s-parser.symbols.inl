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

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Sex;

	struct sexstring_key
	{
		int64 Length;
		cstr Text;

		sexstring_key(cstr text, int64 length) : Length(length), Text(text) {}
	};

	bool operator == (const sexstring_key& a, const sexstring_key& b)
	{
		return a.Length == b.Length && memcmp(a.Text, b.Text, a.Length) == 0;
	}

	struct hash_sexstring_key
	{
		size_t operator()(const sexstring_key& s) const { return Strings::HashArg(s.Text, s.Length); }
	};

	bool operator < (const sexstring_key& a, const sexstring_key& b)
	{
		int64 lengthDelta = a.Length - b.Length;
		if (lengthDelta < 0)
		{
			return true;
		}
		else if (lengthDelta > 0)
		{
			return false;
		}
		else
		{
			return Compare(a.Text, b.Text, a.Length) < 0;
		}
	}

	class CHashedSymbols
	{
	private:
		using TSymbolPtrs = Rococo::TSexyHashMap<sexstring_key,sexstring,hash_sexstring_key,std::equal_to<sexstring_key>>;
		TSymbolPtrs symbols;

	public:
		CHashedSymbols(size_t /* _sourceLength */)
		{

		}

		~CHashedSymbols() 
		{
			for(auto i = symbols.begin(); i != symbols.end(); ++i)
			{
				sexstring symbol = i->second;
				FreeSexString(symbol);
			}
		}

		const sexstring AddSymbol(cstr start, int32 length)
		{
			sexstring_key key(start, length);
			auto i = symbols.find(key);
			if (i != symbols.end())
			{
				return i->second;
			}
			else
			{
				sexstring s = CreateSexString(start, length);
				sexstring_key newKey(s->Buffer, s->Length);
				symbols.insert(std::make_pair(newKey,s));
				return s;
			}
		}
	};

	class CSequentialSymbols
	{
	private:
		typedef TSexyVector<sexstring> TSymbolPtrs;
		TSymbolPtrs symbols;

	public:
		CSequentialSymbols(size_t _sourceLength)
		{
			UNUSED(_sourceLength);
		}

		~CSequentialSymbols() 
		{
			for(auto i = symbols.begin(); i != symbols.end(); ++i)
			{
				sexstring symbol = *i;
				FreeSexString(symbol);
			}
		}

		const sexstring AddSymbol(cstr start, int32 length)
		{
			sexstring s = CreateSexString(start, length);
			symbols.push_back(s);
			return s;
		}
	};

	class CPrivateHeapSymbols
	{
	private:
		size_t maxAlloc;
		char* heap;
		char* heapEnd;
		char* writePos;
	public:
		CPrivateHeapSymbols(size_t _sourceLength)
		{
			maxAlloc = 4 * _sourceLength + 16;
			 // figure worst case is single character atomics, each seperated by a space, giving filelength/2 atomic symbols. each takes 8 bytes of aligned data to store. Add 16 for good measure
			heap = (char*) Rococo::Memory::AllocateSexyMemory(maxAlloc);
			heapEnd = heap + maxAlloc - 8; // leave a safe zone
			writePos = heap;
		}

		~CPrivateHeapSymbols() 
		{
			Rococo::Memory::FreeSexyUnknownMemory(heap);
		}

		const sexstring AddSymbol(cstr start, int32 length)
		{
			if (length + writePos + sizeof(int32) >= heapEnd)
			{
            Rococo::Throw(0, ("sexy.s-parser.symbols.inl: private heap symbols exhausted. Increase CPrivateHeapSymbols::_maxAllocHint"));
				return NULL;
			}

			sexstring_header& header = *(sexstring_header*) writePos;
			header.Length = length;
			memcpy(header.Buffer, start, sizeof(char) * length);
			header.Buffer[length] = 0;
			writePos += sizeof(int32);
			writePos += length + 1;

			// align write pos to be on the next 4-byte boundary, so that sexstring.length is aligned
			size_t mask = (size_t) -3;
			size_t writePosSize = (size_t) writePos;
			size_t alignedWritePos = (writePosSize & mask) + 4;
			writePos = (char*) alignedWritePos;
			return &header;
		}
	};

	//typedef CHashedSymbols CSymbols; // Least memory, as two tokens with matching data consume the same memory units, but slowest
	//typedef CSequentialSymbols CSymbols; // Less memory consumption than fastest, on average, but not much faster than hashed
	typedef CPrivateHeapSymbols CSymbols; // Fastest, but takes 4 chars per source char

	class CSParserTree;
	sexstring AddSymbol(CSParserTree& tree, cstr data, int dataLen);
}