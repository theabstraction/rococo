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

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM �AS IS� WITHOUT
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

//#define IS_SPARSER_DLL 1

#ifdef IS_SPARSER_DLL
# ifndef SEXY_SPARSER_API
#  define SEXY_SPARSER_API extern "C" __declspec(dllimport)
# endif
#else
# define SEXY_SPARSER_API
#endif

#ifndef SEXY_H
#error include "sexy.types.h" before including this file
#endif

namespace Rococo { namespace Sex
{
	struct ISParser;
	struct ISParserTree;
	struct ISourceCode;
	
	inline bool IsAtomic(cr_sex s)
	{
		return s.Type() == EXPRESSION_TYPE_ATOMIC;
	}

	inline bool IsStringLiteral(cr_sex s)
	{
		return s.Type() == EXPRESSION_TYPE_STRING_LITERAL;
	}

	inline bool IsCompound(cr_sex s)
	{
		return s.Type() == EXPRESSION_TYPE_COMPOUND;
	}

	inline bool IsNull(cr_sex s)
	{
		return s.Type() == EXPRESSION_TYPE_NULL;
	}

	bool IsToken(cr_sex s, cstr text);

	class ParseException: public IException
	{
	private:
		Vec2i startPos;
		Vec2i endPos;
		
		enum {MAX_ERRMSG_LEN = 512};
		char srcName[MAX_ERRMSG_LEN];
		char errText[MAX_ERRMSG_LEN];
		char specimenText[MAX_ERRMSG_LEN];
		const ISExpression* source;

	public:
      ParseException();
      ParseException(const Vec2i& start, const Vec2i& end, cstr name, cstr err, cstr specimen, const ISExpression* _source);
		const Vec2i& Start() const { return startPos; }
		const Vec2i& End() const { return endPos; }
		cstr Name() const { return srcName; }
		cstr Message() const { return errText; }
		int ErrorCode() const { return -1; }
		const ISExpression* Source() const { return source; }
		cstr Specimen() const { return specimenText; }
		Debugging::IStackFrameEnumerator* StackFrames() { return nullptr; }
	};

	template<class T> class Auto
	{
	private:
		T* instance;

	public:
		Auto(T* _instance = nullptr): instance(_instance) {}
		~Auto() { if (instance) instance->Release(); }
		Auto<T>& operator = (T* _instance)
		{
			if (instance) instance->Release();
			instance = _instance;
			return *this;
		}

		T& operator()()
		{
			return *instance;
		}

		T* operator -> ()
		{
			return instance;
		}

		T& operator *()
		{
			return *instance;
		}
	};
}}

enum { SEXY_STANDARD_MAX_ATOMIC_STRING_LENGTH = 32768 }; 

// N.B f you override the default maxStringLength value, you are using a non-standard dialect of sexy s-expressions. 
SEXY_SPARSER_API Rococo::Sex::ISParser* Sexy_CreateSexParser_2_0(Rococo::IAllocator& allocator, size_t maxStringLength = SEXY_STANDARD_MAX_ATOMIC_STRING_LENGTH);

namespace Rococo
{
	namespace IO
	{
		struct IBinaryWriter;
		struct IUnicode16Writer;
	}
}

namespace Rococo { namespace Sex
{
	void EscapeScriptStringToAnsi(Rococo::IO::IBinaryWriter& writer, cstr text);
	void EscapeScriptStringToUnicode(Rococo::IO::IUnicode16Writer& writer, cstr text);
}}
