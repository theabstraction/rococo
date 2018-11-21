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

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#ifdef _WIN32
#include <malloc.h>
#endif

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdexcept>

namespace Rococo
{
	sexstring CreateSexString(cstr src, int32 length)
	{
		if (length < 0) length = StringLength(src);
		int32 nBytes = sizeof(int32) + sizeof(char) * (length+1);
		sexstring s = (sexstring) new char[nBytes];
		s->Length = length;

		memcpy_s(s->Buffer, sizeof(char) * (length+1), src, sizeof(char) * length);

		s->Buffer[length] = 0;
		return s;
	}

	void FreeSexString(sexstring s)
	{
		char* buf = (char*) s;
		delete[] buf;
	}

	bool TryGetcharFromHex(char& value, char hex)
	{
		if (hex >= (char) '0' && hex <= (char) '9')
		{
			value = hex -(char) '0';
			return true;
		}

		if (hex >= (char) 'A' && hex <= (char) 'F')
		{
			value = 10 + hex -(char) 'A';
			return true;
		}

		if (hex >= (char) 'a' && hex <= (char) 'f')
		{
			value = 10 + hex -(char) 'a';
			return true;
		}

		return false;
	}

	const char ESCAPECHAR = '&';

	bool ParseEscapeCharacter(char& finalChar, char c)
	{
		switch(c)
		{
		case ESCAPECHAR: // ESCAPECHAR maps to ESCAPECHAR
			finalChar = (char) ESCAPECHAR;
			break;
		case 'q': // \q maps to "
		case '"': // \" maps to "
			finalChar =  (char)'"';
			break;
		case 't': // \t maps to horizontal tab
			finalChar =  (char)'\t';
			break;
		case 'r': // \r maps to linefeed
			finalChar =  (char)'\r';
			break;
		case 'n': // \n maps to newline
			finalChar =  (char)'\n';
			break;
		case '0': // \0 maps to character null
			finalChar =  (char)'\0';
			break;
		case 'a': // \a maps to bell (alert)
			finalChar =  (char)'\a';
			break;
		case 'b': // \b maps to backspace
			finalChar =  (char)'\b';
			break;
		case 'f': // \b maps to formfeed
			finalChar =  (char)'\f';
			break;
		case 'v': // \b maps to vertical tab
			finalChar =  (char)'\v';
			break;
		case '\'': // \' maps to '
			finalChar =  (char)'\'';
			break;
		case '?': // \? maps to ?'
			finalChar =  (char)'?';
			break;
		default:
			return false;
		}

		return true;
	}

	bool TryParseSexHex(char& finalChar, cstr s)
	{
		if (sizeof(char) == 1)
		{			
			char c16;
			char c1;
			if (!TryGetcharFromHex(c16, s[0])) return false;
			if (!TryGetcharFromHex(c1, s[1]))  return false;
			finalChar = c1 + (c16 << 4);
		}
		else if (sizeof(char) == 2)
		{
			// \X means insert byte by four hex digits (16-bit UNICODE)
			char c4096;
			char c256;
			char c16;
			char c1;

			if (!TryGetcharFromHex(c4096, s[0])) return false;
			if (!TryGetcharFromHex(c256,  s[1])) return false;
			if (!TryGetcharFromHex(c16,   s[2])) return false;
			if (!TryGetcharFromHex(c1,    s[3])) return false;
			finalChar = c1 + (c16 << 4) + (c256 << 8) + (c4096 << 12);
		}

		return true;
	}

	void GetRefName(TokenBuffer& token, cstr name)
	{
		NamespaceSplitter splitter(name);
		cstr root, tail;
		if (splitter.SplitTail(root, tail))
		{
			SafeFormat(token.Text, TokenBuffer::MAX_TOKEN_CHARS, ("%s._ref_%s"), root, tail);
		}
		else
		{
         SafeFormat(token.Text, TokenBuffer::MAX_TOKEN_CHARS, ("_ref_%s"), name);
		}
	}

   int CALLTYPE_C StringPrint(TokenBuffer& token, const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      return SafeVFormat(token.Text, TokenBuffer::MAX_TOKEN_CHARS, format, args);
   }

	namespace Sex
	{
		cstr ReadUntil(const Vec2i& pos, const ISourceCode& src)
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
				char c = src.SourceStart()[i];
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

		void GetSpecimen(char specimen[64], const ISExpression& e)
		{
			auto& tree = e.Tree();
			cstr startPos = ReadUntil(e.Start(), tree.Source());
			cstr endPos = ReadUntil(e.End(), tree.Source());

			if (endPos - startPos >= 64)
			{
				Rococo::SafeFormat(specimen, 64, ("%.28s... ...%.28s"), startPos, endPos-28);
			}
			else
			{
				if (endPos > startPos)
				{
					memcpy_s(specimen, 63 * sizeof(char), startPos, (endPos-startPos) * sizeof(char));
				}
				specimen[endPos-startPos] = 0;
			}
		}
	}
}