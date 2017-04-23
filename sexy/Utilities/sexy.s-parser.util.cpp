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
#include <sexy.strings.h>
#include <Sexy.S-Parser.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>
#include <sexy.vm.h>

#include <stdarg.h>
#include <stdio.h>

using namespace Sexy;
using namespace Sexy::Sex;
using namespace Sexy::Compiler;

namespace  Sexy
{ 
   void Throw(int errCode, csexstr format, ...)
   {
      struct SexyException : public IException
      {
         SEXCHAR message[4096];
         int32 exceptionNumber;

         virtual cstr Message() const { return message; }
         virtual int32 ErrorCode() const { return exceptionNumber; }
      } ex;

      va_list args;
      va_start(args, format);
      SafeVFormat(ex.message, _TRUNCATE, format, args);
      ex.exceptionNumber = errCode;

      OS::BreakOnThrow(OS::BreakFlag_All);

      throw ex;
   }

   namespace Sex
   {
      ParseException::ParseException() : startPos{ 0,0 }, endPos{ 0, 0 }
      {
         srcName[0] = 0;
         errText[0] = 0;
         specimenText[0] = 0;
         source = nullptr;
      }

      ParseException::ParseException(const Vec2i& start, const Vec2i& end, csexstr name, csexstr err, csexstr specimen, const ISExpression* _source) :
         startPos(start),
         endPos(end),
         source(_source)
      {
         CopyString(srcName, MAX_ERRMSG_LEN, name, -1);
         CopyString(errText, MAX_ERRMSG_LEN, err, -1);
         CopyString(specimenText, MAX_ERRMSG_LEN, specimen, -1);
      }

      void Throw(ParseException& ex)
      {
         OS::BreakOnThrow(OS::BreakFlag_SS);
         throw ex;
      }

      void Throw(cr_sex e, csexstr format, ...)
      {
         va_list args;
         va_start(args, format);

         rchar message[4096];
         SafeVFormat(message, _TRUNCATE, format, args);

         SEXCHAR specimen[64];
         GetSpecimen(specimen, e);
         ParseException ex(e.Start(), e.End(), e.Tree().Source().Name(), message, specimen, &e);
         Throw(ex);
      }
   }
}