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

#include <sexy.types.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdexcept>

namespace Sexy
{
	size_t Hash(csexstr s)
	{
		struct ANON
		{
			static size_t jenkins_one_at_a_time_hash(csexstr s, size_t len)
			{
				size_t hash, i;
				for(hash = i = 0; i < len; ++i)
				{
					hash += s[i];
					hash += (hash << 10);
					hash ^= (hash >> 6);
				}
				hash += (hash << 3);
				hash ^= (hash >> 11);
				hash += (hash << 15);
				return hash;
			}
		};

		if (s == nullptr) return -1;
		return ANON::jenkins_one_at_a_time_hash(s, StringLength(s));
	}

	int32 Hash(csexstr s, int64 length)
	{
		struct ANON
		{
			static int jenkins_one_at_a_time_hash(csexstr s, int64 len)
			{
				int32 hash = 0;
				for(int64 i = 0; i < len; ++i)
				{
					hash += s[i];
					hash += (hash << 10);
					hash ^= (hash >> 6);
				}
				hash += (hash << 3);
				hash ^= (hash >> 11);
				hash += (hash << 15);
				return hash;
			}
		};

		if (s == nullptr) return -1LL;
		return ANON::jenkins_one_at_a_time_hash(s, length);
	}

	int32 Hash(int32 x)
	{
		struct ANON
		{
			static int robert_jenkins_32bit_hash(int32 key)
			{
				key = ~key + (key << 15); // key = (key << 15) - key - 1;
				key = key ^ (key >> 12);
				key = key + (key << 2);
				key = key ^ (key >> 4);
				key = key * 2057; // key = (key + (key << 3)) + (key << 11);
				key = key ^ (key >> 16);
				return key;
			}
		};

		return ANON::robert_jenkins_32bit_hash(x);
	}

	int32 Hash(int64 x)
	{
		struct ANON
		{
			static int robert_jenkins_64bit_hash(int64 key)
			{
				key = (~key) + (key << 18); // key = (key << 18) - key - 1;
				key = key ^ (key >> 31);
				key = key * 21; // key = (key + (key << 2)) + (key << 4);
				key = key ^ (key >> 11);
				key = key + (key << 6);
				key = key ^ (key >> 22);
				return (int) key;
			}
		};

		return ANON::robert_jenkins_64bit_hash(x);
	}

	sexstring CreateSexString(csexstr src, int32 length)
	{
		if (length < 0) length = StringLength(src);
		int32 nBytes = sizeof(int32) + sizeof(SEXCHAR) * (length+1);
		sexstring s = (sexstring) new char[nBytes];
		s->Length = length;

		memcpy_s(s->Buffer, sizeof(SEXCHAR) * (length+1), src, sizeof(SEXCHAR) * length);

		s->Buffer[length] = 0;
		return s;
	}

	void FreeSexString(sexstring s)
	{
		char* buf = (char*) s;
		delete[] buf;
	}

	bool TryGetSexCharFromHex(SEXCHAR& value, SEXCHAR hex)
	{
		if (hex >= (SEXCHAR) '0' && hex <= (SEXCHAR) '9')
		{
			value = hex -(SEXCHAR) '0';
			return true;
		}

		if (hex >= (SEXCHAR) 'A' && hex <= (SEXCHAR) 'F')
		{
			value = 10 + hex -(SEXCHAR) 'A';
			return true;
		}

		if (hex >= (SEXCHAR) 'a' && hex <= (SEXCHAR) 'f')
		{
			value = 10 + hex -(SEXCHAR) 'a';
			return true;
		}

		return false;
	}

	const char ESCAPECHAR = '&';

	bool ParseEscapeCharacter(SEXCHAR& finalChar, SEXCHAR c)
	{
		switch(c)
		{
		case ESCAPECHAR: // ESCAPECHAR maps to ESCAPECHAR
			finalChar = (SEXCHAR) ESCAPECHAR;
			break;
		case 'q': // \q maps to "
		case '"': // \" maps to "
			finalChar =  (SEXCHAR)'"';
			break;
		case 't': // \t maps to horizontal tab
			finalChar =  (SEXCHAR)'\t';
			break;
		case 'r': // \r maps to linefeed
			finalChar =  (SEXCHAR)'\r';
			break;
		case 'n': // \n maps to newline
			finalChar =  (SEXCHAR)'\n';
			break;
		case '0': // \0 maps to character null
			finalChar =  (SEXCHAR)'\0';
			break;
		case 'a': // \a maps to bell (alert)
			finalChar =  (SEXCHAR)'\a';
			break;
		case 'b': // \b maps to backspace
			finalChar =  (SEXCHAR)'\b';
			break;
		case 'f': // \b maps to formfeed
			finalChar =  (SEXCHAR)'\f';
			break;
		case 'v': // \b maps to vertical tab
			finalChar =  (SEXCHAR)'\v';
			break;
		case '\'': // \' maps to '
			finalChar =  (SEXCHAR)'\'';
			break;
		case '?': // \? maps to ?'
			finalChar =  (SEXCHAR)'?';
			break;
		default:
			return false;
		}

		return true;
	}

	bool TryParseSexHex(SEXCHAR& finalChar, csexstr s)
	{
		if (sizeof(SEXCHAR) == 1)
		{			
			SEXCHAR c16;
			SEXCHAR c1;
			if (!TryGetSexCharFromHex(c16, s[0])) return false;
			if (!TryGetSexCharFromHex(c1, s[1]))  return false;
			finalChar = c1 + (c16 << 4);
		}
		else if (sizeof(SEXCHAR) == 2)
		{
			// \X means insert byte by four hex digits (16-bit UNICODE)
			SEXCHAR c4096;
			SEXCHAR c256;
			SEXCHAR c16;
			SEXCHAR c1;

			if (!TryGetSexCharFromHex(c4096, s[0])) return false;
			if (!TryGetSexCharFromHex(c256,  s[1])) return false;
			if (!TryGetSexCharFromHex(c16,   s[2])) return false;
			if (!TryGetSexCharFromHex(c1,    s[3])) return false;
			finalChar = c1 + (c16 << 4) + (c256 << 8) + (c4096 << 12);
		}

		return true;
	}

	void GetRefName(TokenBuffer& token, csexstr name)
	{
		NamespaceSplitter splitter(name);
		csexstr root, tail;
		if (splitter.SplitTail(root, tail))
		{
			StringPrint(token, SEXTEXT("%s._ref_%s"), root, tail);
		}
		else
		{
			StringPrint(token, SEXTEXT("_ref_%s"), name);
		}
	}

	int __cdecl StringPrintV(char* buf, size_t sizeInChars, va_list argList, const char* format)
	{
		return _vsnprintf_s(buf, sizeInChars, _TRUNCATE, format, argList);
	}

	int __cdecl StringPrintV(wchar_t* buf, size_t sizeInChars, va_list argList, const wchar_t* format)
	{
		return _vsnwprintf_s(buf, sizeInChars, _TRUNCATE, format, argList);
	}

	int __cdecl StringPrint(char* buf, size_t sizeInChars, const char* format, ...) // N.B if you are having crashes passing a SEXCHAR array try casting it to (const SEXCHAR*)
	{
		va_list args;
		va_start(args,format);
		return _vsnprintf_s(buf, sizeInChars, _TRUNCATE, format, args);
	}

	int __cdecl StringPrint(wchar_t* buf, size_t sizeInChars, const wchar_t* format, ...)
	{
		va_list args;
		va_start(args,format);
		return _vsnwprintf_s(buf, sizeInChars, _TRUNCATE, format, args);
	}

	int __cdecl StringPrint(TokenBuffer& buf, const SEXCHAR* format, ...)
	{
		va_list args;
		va_start(args, format);

		int status = StringPrintV(buf.Text, buf.MAX_TOKEN_CHARS, args, format);
		if (status == -1)
		{
			throw std::invalid_argument("The string buffer exceeded the maximum allowed characters");
		}

		return status;
	}

	int _cdecl GetErrorString(char* buf, size_t sizeInChars, int errNum)
	{
		return strerror_s(buf, sizeInChars, errNum);
	}

	int _cdecl GetErrorString(wchar_t* buf, size_t sizeInChars, int errNum)
	{
		char err[256];
		GetErrorString(err, sizeInChars, errNum);

		return _snwprintf_s(buf, sizeInChars, _TRUNCATE, L"%S", err);		
	}

	int32 __cdecl StringLength(const char* s)
	{
		size_t l = strlen(s);
		if (l > INT_MAX)
		{
			throw std::invalid_argument("The string length exceeded INT_MAX characters");
		}

		return (int32) l;
	}

	int32 __cdecl StringLength(const wchar_t* s)
	{
		size_t l = wcslen(s);
		if (l > INT_MAX)
		{
			throw std::invalid_argument("The string length exceeded INT_MAX characters");
		}

		return (int32) l;
	}

	void __cdecl CopyChars(SEXCHAR* dest, const sexstring source)
	{
		for(int i = 0; i < source->Length; ++i)
		{
			dest[i] = source->Buffer[i];
		}

		dest[source->Length] = 0;
	}

   int __cdecl WriteToStandardOutput(const char* format, ...)
	{	
		va_list args;
		va_start(args, format);
		return vprintf_s(format, args);
	}

	int __cdecl WriteToStandardOutput(const wchar_t* format, ...)
	{
		va_list args;
		va_start(args, format);
		return vwprintf_s(format, args);
	}

	void __cdecl CopyString(char* dest, size_t capacity, const char* source)
	{
		strcpy_s(dest, capacity, source);
	}

	void __cdecl CopyString(wchar_t* dest, size_t capacity, const wchar_t* source, int maxChars)
	{
		wcsncpy_s(dest, capacity, source,  maxChars < 0 ? _TRUNCATE : maxChars);
	}

	void __cdecl CopyString(wchar_t* dest, size_t capacity, const wchar_t* source)
	{
		wcscpy_s(dest, capacity, source);
	}

	void __cdecl CopyString(char* dest, size_t capacity, const char* source, int maxChars)
	{
		strncpy_s(dest, capacity, source, maxChars < 0 ? _TRUNCATE : maxChars);
	}

	void __cdecl CopyString(wchar_t* dest, const char* source, int maxChars)
	{
		_snwprintf_s(dest, maxChars, maxChars, L"%S", source);
	}

	void __cdecl StringCat(wchar_t* buf, const wchar_t* source, int maxChars)
	{
		wcscat_s(buf, maxChars, source);
	}

	void __cdecl StringCat(char* buf, const char* source, int maxChars)
	{
		strcat_s(buf, maxChars, source);
	}

	bool __cdecl IsCapital(SEXCHAR c)
	{
		return c >= 'A' && c <= 'Z';
	}

	bool __cdecl IsLowerCase(SEXCHAR c)
	{
		return c >= 'a' && c <= 'z';
	}

	bool __cdecl IsAlphabetical(SEXCHAR c)
	{
		return IsCapital(c) || IsLowerCase(c);
	}

	bool __cdecl IsNumeric(SEXCHAR c)
	{
		return c >= '0' && c <= '9';
	}

	bool __cdecl IsAlphaNumeric(SEXCHAR c)
	{
		return IsAlphabetical(c) || IsNumeric(c);
	}

	namespace Sex
	{
		csexstr ReadUntil(const Vec2i& pos, const ISourceCode& src)
		{
			Vec2i origin = src.Origin();

			int X = origin.x, Y = origin.y;

			int i;
			for(i = 0; i < src.SourceLength(); ++i)
			{
				if (Y > pos.y) break;
				else if (pos.y == Y && X == pos.x)
				{
					break;
				}
				SEXCHAR c = src.SourceStart()[i];
				switch(c)
				{
				case '\r':
					break;
				case '\n':
					Y++;
					X = origin.x;
					break;
				default:
					X++;
				}
			}

			return src.SourceStart() + i;
		}

		void GetSpecimen(SEXCHAR specimen[64], const ISExpression& e)
		{
			auto& tree = e.Tree();
			csexstr startPos = ReadUntil(e.Start(), tree.Source());
			csexstr endPos = ReadUntil(e.End(), tree.Source());

			if (endPos - startPos >= 64)
			{
				StringPrint(specimen, 64, SEXTEXT("%.28s... ...%.28s"), startPos, endPos-28);
			}
			else
			{
				if (endPos > startPos)
				{
					memcpy_s(specimen, 63 * sizeof(SEXCHAR), startPos, (endPos-startPos) * sizeof(SEXCHAR));
				}
				specimen[endPos-startPos] = 0;
			}
		}
	}
}