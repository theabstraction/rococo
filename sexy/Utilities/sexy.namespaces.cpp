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
#include <sexy.stdstrings.h>

namespace Sexy
{
	class NamespaceException: public IException
	{
	private:
		rchar msg[1024];

	public:
		NamespaceException(csexstr _msg)
		{
			CopyString(msg, 1024, _msg, -1);
		}

		virtual int ErrorCode() const { return -1; }
		virtual cstr Message() const { return msg; }
	};

	NamespaceSplitter::NamespaceSplitter(csexstr _src): src(_src)
	{
		length = StringLength(_src);		
		if (length >= NAMESPACE_MAX_LENGTH)
		{
			sexstringstream<1024> streamer;
			streamer.sb << SEXTEXT("Error splitting namespace '") << _src << SEXTEXT("' namespace exceeded maximum length of ") << NAMESPACE_MAX_LENGTH << SEXTEXT(" characters");
			throw NamespaceException(streamer);
		}
	}

	bool NamespaceSplitter::SplitHead(OUT csexstr& _head, OUT csexstr& _body)
	{
		memcpy_s(dottedName, NAMESPACE_MAX_LENGTH, src, sizeof(SEXCHAR) * (length+1));

		for(int i = 0; i < length; i++)
		{
			if (dottedName[i] == '.')
			{
				dottedName[i] = 0;
				_head = dottedName;
				_body = dottedName+i+1;
				return true;
			}
		}

		return false;
	}

	bool NamespaceSplitter::SplitTail(OUT csexstr& _body, OUT csexstr& _tail)
	{
		memcpy_s(dottedName, NAMESPACE_MAX_LENGTH, src, sizeof(SEXCHAR) * (length+1));

		for(int i = length-1; i >= 0; i--)
		{
			if (dottedName[i] == '.')
			{
				dottedName[i] = 0;
				_tail = dottedName+i+1;
				_body = dottedName;
				return true;
			}
		}

		return false;
	}
}