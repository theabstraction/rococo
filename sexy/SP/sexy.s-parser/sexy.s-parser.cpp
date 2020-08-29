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

#include "sexy.s-parser.stdafx.h"

#define ROCOCO_USE_SAFE_V_FORMAT
#include "sexy.strings.h"
#include "sexy.stdstrings.h"

#include <rococo.io.h>
#include <unordered_map>
#include <vector>

#include <rococo.api.h>

#include "sexy.s-parser.source.inl"
#include "sexy.s-parser.symbols.inl"

using namespace Rococo;
using namespace Rococo::Sex;

namespace ANON
{
	struct EscapeSequence
	{
		char text[8];
		size_t len;
	};

	static EscapeSequence* maprcharToSequence = nullptr;

	void ClearEscapeMap()
	{
		delete[] maprcharToSequence;
		maprcharToSequence = nullptr;
	}

	void InitEscapeSequences()
	{
		if (maprcharToSequence == nullptr)
		{
			maprcharToSequence = new EscapeSequence[256];
			atexit(ClearEscapeMap);

			for (uint32 i = 0; i < 32; ++i)
			{
				SafeFormat(maprcharToSequence[i].text, 8, "&x%x", i);
				maprcharToSequence[i].len = 6;
			}

			for (uint32 i = 32; i <= 255; ++i)
			{
				SafeFormat(maprcharToSequence[i].text, 8, "%c", i);
				maprcharToSequence[i].len = 1;
			}

			SafeFormat(maprcharToSequence[L'\r'].text, 8, "&r");
			maprcharToSequence[L'\r'].len = 2;

			SafeFormat(maprcharToSequence[L'\n'].text, 8, "&n");
			maprcharToSequence[L'\n'].len = 2;

			SafeFormat(maprcharToSequence[L'\t'].text, 8, "&t");
			maprcharToSequence[L'\t'].len = 2;

			SafeFormat(maprcharToSequence[L'\"'].text, 8, "&q");
			maprcharToSequence[L'\"'].len = 2;

			SafeFormat(maprcharToSequence[L'&'].text, 8, "&&");
			maprcharToSequence[L'&'].len = 2;
		}
	}
}

namespace ANON
{
	void ThrowBadArg(cstr format, ...)
	{
		struct : public IException
		{
			char msg[256];
			int32 errorCode;

			virtual cstr Message() const override
			{
				return msg;
			}

			virtual int32 ErrorCode() const override
			{
				return errorCode;
			}

			Debugging::IStackFrameEnumerator* StackFrames() override { return nullptr; }
		} ex;

		va_list args;
		va_start(args, format);

		SafeVFormat(ex.msg, sizeof(ex.msg), format, args);

		ex.errorCode = 0;

		throw ex;
	}
}

namespace Rococo
{
	namespace Sex
	{
		using namespace Rococo::IO;

		bool IsToken(cr_sex s, cstr text)
		{
			return IsAtomic(s) && AreEqual(s.String(), text);
		}

		void EscapeScriptStringToAnsi(IBinaryWriter& writer, cstr text)
		{
			if (*text == 0) return;

			ANON::InitEscapeSequences();

			size_t segmentLength = 0;

			for (cstr s = text; *s != 0; s++)
			{
				segmentLength += ANON::maprcharToSequence[(size_t)*s].len;
			}

			if (segmentLength > 0xFFFFFFFFull)
			{
				ANON::ThrowBadArg(("EscapeScriptStringToAnsi -> string length too long"));
			}

			uint8* segment = (uint8*)alloca(segmentLength);

			uint8* writePos = segment;
			for (cstr s = text; *s != 0; s++)
			{
				memcpy(writePos, ANON::maprcharToSequence[(size_t)*s].text, ANON::maprcharToSequence[(size_t)*s].len);
				writePos += ANON::maprcharToSequence[(size_t)*s].len;
			}

			writer.Write(segment, (uint32)segmentLength);
		}

		void EscapeScriptStringToUnicode(IUnicode16Writer& writer, cstr text)
		{
			if (*text == 0) return;

			ANON::InitEscapeSequences();

			size_t segmentLength = 1;

			for (cstr s = text; *s != 0; s++)
			{
				segmentLength += ANON::maprcharToSequence[(size_t)*s].len;
			}

			char* segment = (char*)alloca(sizeof(char) * segmentLength);

			char* writePos = segment;
			for (cstr s = text; *s != 0; s++)
			{
				SecureFormat(writePos, segment + segmentLength - writePos, ("%s"), ANON::maprcharToSequence[(size_t)*s].text);
				writePos += ANON::maprcharToSequence[(size_t)*s].len;
			}

			*writePos = 0;

			writer.Append(L"%hs", segment);
		}

		SEXY_SPARSER_API cr_sex GetAtomicArg(cr_sex e, int argIndex)
		{
			AssertCompound(e);
			AssertNotTooFewElements(e, argIndex + 1);
			cr_sex arg = e.GetElement(argIndex);
			AssertAtomic(arg);
			return arg;
		}

		SEXY_SPARSER_API cstr ToString(EXPRESSION_TYPE type)
		{
			switch (type)
			{
			case EXPRESSION_TYPE_ATOMIC: return ("atomic");
			case EXPRESSION_TYPE_STRING_LITERAL: return ("string-literal");
			case EXPRESSION_TYPE_COMPOUND: return ("compound");
			case EXPRESSION_TYPE_NULL: return ("null");
			default: return ("unknown");
			}
		}

		SEXY_SPARSER_API void AssertExpressionType(cr_sex e, EXPRESSION_TYPE type)
		{
			if (e.Type() != type)
			{
				sexstringstream<1024> streamer;
				streamer.sb << ("Expecting ") << ToString(type) << " expression, but found " << ToString(e.Type()) << " expression";
				Throw(e, *streamer.sb);
			}
		}

		SEXY_SPARSER_API void AssertCompound(cr_sex e) { AssertExpressionType(e, EXPRESSION_TYPE_COMPOUND); }
		SEXY_SPARSER_API void AssertAtomic(cr_sex e) { AssertExpressionType(e, EXPRESSION_TYPE_ATOMIC); }
		SEXY_SPARSER_API void AssertStringLiteral(cr_sex e) { AssertExpressionType(e, EXPRESSION_TYPE_STRING_LITERAL); }
		SEXY_SPARSER_API void AssertNull(cr_sex e) { AssertExpressionType(e, EXPRESSION_TYPE_NULL); }

		SEXY_SPARSER_API void AssertNotTooManyElements(cr_sex e, int32 maxElements)
		{
			int32 elementCount = e.NumberOfElements();
			if (maxElements > 0 && elementCount > maxElements)
			{
				sexstringstream<1024> streamer;
				streamer.sb << ("Expression had more than the maximum of ") << maxElements << " element" << (maxElements > 1 ? ("s") : (""));
				Throw(e, streamer);
			}
		}

		SEXY_SPARSER_API void AssertNotTooFewElements(cr_sex e, int32 minElements)
		{
			int32 elementCount = e.NumberOfElements();
			if (minElements > 0 && elementCount < minElements)
			{
				Throw(e, "Expression had fewer than the minimum of %d element%s", minElements, (minElements > 1) ? "s" : "");
			}
		}
	}
}
