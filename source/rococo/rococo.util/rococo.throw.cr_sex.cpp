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
#include <sexy.strings.h>
#include "sexy.s-parser.h"
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Compiler;
using namespace Rococo::Strings;

namespace Rococo
{
	namespace OS
	{
		ROCOCO_API void BreakOnThrow(Flags::BreakFlag flag);
	}

	namespace Sex
	{
		ROCOCO_API cstr ReadUntil(const Vec2i& pos, const ISourceCode& src)
		{
			Vec2i origin = src.Origin();

			int X = origin.x, Y = origin.y;

			int i;
			for (i = 0; i < src.SourceLength(); ++i)
			{
				if (Y > pos.y) break;
				else if (pos.y == Y && X == pos.x)
				{
					break;
				}
				char c = src.SourceStart()[i];
				switch (c)
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

		ROCOCO_API  void GetSpecimen(char specimen[64], const ISExpression& e)
		{
			auto& tree = e.Tree();
			cstr startPos = ReadUntil(e.Start(), tree.Source());
			cstr endPos = ReadUntil(e.End(), tree.Source());

			if (endPos - startPos >= 64)
			{
				SafeFormat(specimen, 64, ("%.28s... ...%.28s"), startPos, endPos - 28);
			}
			else
			{
				if (endPos > startPos)
				{
					memcpy_s(specimen, 63 * sizeof(char), startPos, (endPos - startPos) * sizeof(char));
				}
				specimen[endPos - startPos] = 0;
			}
		}

		ROCOCO_API ParseException::ParseException() : startPos{ 0,0 }, endPos{ 0, 0 }
		{
			srcName[0] = 0;
			errText[0] = 0;
			specimenText[0] = 0;
			source = nullptr;
		}

		ROCOCO_API ParseException::ParseException(const Vec2i& start, const Vec2i& end, cstr name, cstr err, cstr specimen, const ISExpression* _source) :
			startPos(start),
			endPos(end),
			source(_source)
		{
			CopyString(srcName, ParseException::MAX_SRC_LEN, name);
			CopyString(errText, ParseException::MAX_ERR_LEN, err);
			CopyString(specimenText, ParseException::MAX_SPECIMEN_LEN, specimen);
		}

		ROCOCO_API void Throw(ParseException& ex)
		{
			OS::BreakOnThrow(OS::Flags::BreakFlag_SS);
			throw ex;
		}

		ROCOCO_API const ISExpression* GetFirstAtomic(cr_sex s)
		{
			if (IsAtomic(s))
			{
				return &s;
			}

			for (int i = 0; i < s.NumberOfElements(); ++i)
			{
				auto* theAtomic = GetFirstAtomic(s[i]);
				if (theAtomic)
				{
					return theAtomic;
				}
			}

			return nullptr;
		}

		ROCOCO_API void Throw(cr_sex e, _Printf_format_string_ cstr format, ...)
		{
			va_list args;
			va_start(args, format);

			char message[4096];
			int len = SafeVFormat(message, sizeof(message), format, args);

			StackStringBuilder sb(message + len, sizeof message - len);

			auto* pOriginal = e.GetOriginal();
			if (pOriginal != nullptr)
			{
				cr_sex s = *pOriginal;

				const ISExpression* theAtomic = GetFirstAtomic(e);
				if (theAtomic)
				{
					sb.AppendFormat("(near %s)", theAtomic->c_str());
				}

				char specimen[64];
				GetSpecimen(specimen, s);
				ParseException ex(s.Start(), s.End(), s.Tree().Source().Name(), message, specimen, &s);
				Throw(ex);
			}
			else
			{
				char specimen[64];
				GetSpecimen(specimen, e);
				ParseException ex(e.Start(), e.End(), e.Tree().Source().Name(), message, specimen, &e);
				Throw(ex);
			}
		}
	}
}