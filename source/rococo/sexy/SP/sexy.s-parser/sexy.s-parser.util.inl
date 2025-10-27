/*
	Sexy Scripting Language - Copright(c)2013-2025. Mark Anthony Taylor. All rights reserved.

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

#pragma once

namespace
{
	SEXY_SPARSER_API sexstring CreateSexString(const SEXCHAR* src, int32 length)
	{
		int32 nBytes = sizeof(int32) + sizeof(SEXCHAR) * (length+1);
		sexstring s = (sexstring) new char[nBytes];
		s->Length = length;

		memcpy_s(s->Buffer, sizeof(SEXCHAR) * (length+1), src, sizeof(SEXCHAR) * length);

		s->Buffer[length] = 0;
		return s;
	}

	SEXY_SPARSER_API void FreeSexString(sexstring s)
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

	bool ParseEscapeCharacter(SEXCHAR& finalChar, SEXCHAR c)
	{
		switch(c)
		{
		case '\\': // \\ maps to single backslash
			finalChar = (SEXCHAR)'\\';
			break;
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

	bool TryParseSexHex(SEXCHAR& finalChar, const SEXCHAR* s)
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
}