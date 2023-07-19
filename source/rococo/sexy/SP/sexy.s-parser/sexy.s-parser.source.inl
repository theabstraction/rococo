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
 
namespace Anon
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Memory;

	class CSourceCodeCopy final: public ISourceCode
	{
	private:
		refcount_t refcount;
		char* buffer;
		int segmentLength;
		Vec2i origin;
		stdstring name;

	public:
		CSourceCodeCopy(cstr _buffer, int _segmentLength, const Vec2i& _origin, cstr _name):
			refcount(1),
         segmentLength(_segmentLength),
         origin(_origin),
			name(_name)
		{
			buffer = (char*) AllocateSexyMemory(_segmentLength + 1);
			memcpy(buffer, _buffer, segmentLength * sizeof(char));
			buffer[_segmentLength] = 0;
		}

		~CSourceCodeCopy()
		{
			FreeSexyUnknownMemory(buffer);
		}

		virtual const Vec2i& Origin() const		{	return origin; }
		virtual cstr SourceStart() const				{	return buffer; }
		virtual const int SourceLength() const		{	return segmentLength;	}
		virtual cstr Name() const							{	return name.c_str();	}
		virtual refcount_t AddRef()							
      {
         return ++refcount;	
      }

		virtual refcount_t Release()
		{
			refcount--;
			if (refcount == 0)
			{
				delete this;
				return 0;
			}

			return refcount;
		}
	};

	class CSourceCodeProxy final: public ISourceCode
	{
	private:
		refcount_t refcount;
		cstr buffer;
		int segmentLength;
		Vec2i origin;
		cstr name;

	public:
		CSourceCodeProxy(cstr _buffer, int _segmentLength, const Vec2i& _origin, cstr _name):
			refcount(1),
         buffer(_buffer),
         segmentLength(_segmentLength),
         origin(_origin),
			name(_name)	
		{
		}

		virtual const Vec2i& Origin() const		{	return origin;	}
		virtual cstr SourceStart() const	{	return buffer;	}
		virtual const int SourceLength() const			{	return segmentLength;	}
		virtual cstr Name() const					{	return name; }
		virtual refcount_t AddRef()									{	return ++refcount; }

		virtual refcount_t Release()
		{
			refcount--;
			if (refcount == 0)
			{
				delete this;
				return 0;
			}

			return refcount;
		}
	};
}