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
	class CSexParser;
	class CSExpression;

	void Release(CSParserTree* tree);
	CSParserTree* ConstructTransform(CSParserTree& prototype, const Vec2i& start, const Vec2i& end, CSExpression* original);
	const ISExpression* GetOriginal(CSParserTree& tree);

	struct FileReader
	{
		FILE* fp;

		FileReader(const cstr name, long& len)
		{
			errno_t errNo = OS::OpenForRead((void**)&fp, name);

			if (errNo != 0)
			{
				char err[256];
				OS::Format_C_Error(errNo, err, sizeof(err));
				sexstringstream<1024> streamer;
				streamer.sb << ("Error ") << errNo << (" opening file: ") << err;

				throw ParseException(Vec2i{ 0,0 }, Vec2i{ 0,0 }, name, streamer, ("<none>"), NULL);
			}

			long startLen = ftell(fp);
			fseek(fp, 0, SEEK_END);
			len = ftell(fp) - startLen;
			fseek(fp, 0, SEEK_SET);
		}

		~FileReader()
		{
			fclose(fp);
		}
	};

	template <class T> class allocator
	{
	public:
		using value_type = T;

		allocator() noexcept {}
		template <class U> allocator(allocator<U> const&) noexcept {}

		value_type* allocate(std::size_t n)
		{
			return static_cast<value_type*>(::operator new (n * sizeof(value_type)));
		}

		void deallocate(value_type* p, std::size_t) noexcept
		{
			::operator delete(p);
		}
	};

	template <class T, class U> bool operator==(allocator<T> const&, allocator<U> const&) noexcept
	{
		return true;
	}

	template <class T, class U> bool operator!=(allocator<T> const& x, allocator<U> const& y) noexcept
	{
		return !(x == y);
	}

	class CSExpression final : public ISExpression
	{
	private:
		CSParserTree& tree;
		Vec2i start;
		Vec2i end;
		EXPRESSION_TYPE type;
		CSExpression* parent;
		sexstring Symbol;

		typedef std::vector<CSExpression*> TChildrenVector;
		TChildrenVector children;

		void AddChild(CSExpression* child)
		{
			children.push_back(child);
			if (type != EXPRESSION_TYPE_COMPOUND && type != EXPRESSION_TYPE_NULL)
			{
				char specimen[64];
				GetSpecimen(specimen, *child);
				throw ParseException(start, end, ("Algorithmic error"), ("Error adding child. ExpressionType was not NULL type"), specimen, NULL);
			}
			type = EXPRESSION_TYPE_COMPOUND;
		}
	public:
		CSExpression(CSParserTree& _tree, const Vec2i& _start, CSExpression* _parent) :
			tree(_tree),
			start(_start),
			end(_start),
			type(EXPRESSION_TYPE_NULL),
			parent(_parent),
			Symbol(NULL)
		{
			children.reserve(4);
			if (_parent != NULL)
			{
				_parent->AddChild(this);
			}
		}

		CSExpression(CSParserTree& _tree, const Vec2i& _start, const Vec2i& _end, CSExpression* _parent, cstr data, int dataLen, bool isStringLiteral) :
			tree(_tree),
			start(_start),
			end(_end),
			type(isStringLiteral ? EXPRESSION_TYPE_STRING_LITERAL : EXPRESSION_TYPE_ATOMIC),
			parent(_parent),
			Symbol(NULL)
		{
			Symbol = AddSymbol(tree, data, dataLen);

			if (_parent != NULL)
			{
				_parent->AddChild(this);
			}
		}

		~CSExpression()
		{
			for (auto i = children.begin(); i != children.end(); ++i)
			{
				CSExpression* child = *i;
				delete child;
			}
		}

		const ISExpression* GetOriginal() const override
		{
			return ANON::GetOriginal(tree);
		}

		const Vec2i Start() const override { return start; }
		const Vec2i End() const override { return end; }
		EXPRESSION_TYPE Type() const override { return type; }
		const sexstring String() const override;
		const ISParserTree& Tree() const override;
		int NumberOfElements() const override { return (int32)children.size(); }
		const ISExpression& GetElement(int index) const override { return *children[index]; }
		const ISExpression* Parent() const override { return parent; }

		CSExpression* ConcreteParent() { return parent; }

		void AddAtomic(cstr sourceSegmentOrigin, int32 sourceSegmentLength, const Vec2i& start, const Vec2i& end);
		void AddEscapedString(cstr sourceSegmentOrigin, int32 sourceSegmentLength, const Vec2i& start, const Vec2i& end);

		void SetEnd(const Vec2i& _end)
		{
			end = _end;
		}

		virtual bool operator == (const char* token) const
		{
			if (token == nullptr) return Symbol == nullptr;
			else return Symbol != nullptr && AreEqual(Symbol, token);
		}
	};

#ifdef _WIN32
# define STATE_CALL __fastcall
#else
# define STATE_CALL
#endif

	class CSParserTree final : public ISParserTree
	{
	private:
		int transformDepth;
		CSExpression* root;
		CSexParser& parser;
		refcount_t refcount;
		ISourceCode& sourceCode;
		CSymbols symbols;
		typedef std::vector<char> TStringBuilder;
		TStringBuilder literalStringBuilder;

		int generationLength;
		cstr generationCurrentPtr;
		cstr generationEnd;
		Vec2i generationTokenStartPos;
		Vec2i generationCursorPos;
		Vec2i generationPrevCursorPos;
		CSExpression* generationNode;
		cstr startOfToken;
		CSExpression* original;

		typedef void (STATE_CALL CSParserTree::*FN_STATE)();

		FN_STATE generationState;

		void AddToken(cstr start, cstr end);
		void AddStringToken(cstr start, int32 length);
		void UpdateCursor(char c);
		void OpenInnerExpression();
		void CloseInnerExpression();

		void STATE_CALL ReadingStringLiteral();
		void STATE_CALL ReadingLineComment();
		void STATE_CALL ReadingParagraphComment();
		void STATE_CALL ReadingToken();
		void STATE_CALL ReadingWhitespace();

	public:
		CSParserTree(CSexParser& _parser, ISourceCode& _sourceCode, const Vec2i& start, const Vec2i& end, int _transformDepth, CSExpression* _original) :
			transformDepth(_transformDepth),
			parser(_parser),
			refcount(1),
			sourceCode(_sourceCode),
			symbols(_sourceCode.SourceLength()),
			original(_original)
		{
			sourceCode.AddRef();
			root = new CSExpression(*this, start, NULL);
			root->SetEnd(end);
		}

		~CSParserTree()
		{
			delete root;
			sourceCode.Release();
		}

		CSymbols& Symbols() { return symbols; }
		void Generate();

		virtual const int TransformDepth() const { return transformDepth; }
		virtual ISParser& Parser();
		virtual ISExpression& Root() { return *root; }
		virtual const ISExpression& Root() const { return *root; }
		virtual const ISourceCode& Source()	const { return sourceCode; }
		virtual refcount_t AddRef() { return ++refcount; }
		virtual const ISExpression* const GetOriginal() { return original; }

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

	const sexstring CSExpression::String() const
	{
		return this->Symbol;
	}

	sexstring AddSymbol(CSParserTree& tree, cstr data, int dataLen)
	{
		sexstring s = tree.Symbols().AddSymbol(data, dataLen);
		return s;
	}

	void CSExpression::AddAtomic(cstr sourceSegmentOrigin, int32 sourceSegmentLength, const Vec2i& start, const Vec2i& end)
	{
		new CSExpression(tree, start, end, this, sourceSegmentOrigin, sourceSegmentLength, false);
	}

	void CSExpression::AddEscapedString(cstr sourceSegmentOrigin, int32 sourceSegmentLength, const Vec2i& start, const Vec2i& end)
	{
		new CSExpression(tree, start, end, this, sourceSegmentOrigin, sourceSegmentLength, true);
	}

	const ISParserTree& CSExpression::Tree() const
	{
		return tree;
	}

	void CSParserTree::AddToken(cstr start, cstr end)
	{
		generationNode->AddAtomic(start, (int32)(end - start), generationTokenStartPos, generationPrevCursorPos);
	}

	void CSParserTree::AddStringToken(cstr start, int32 length)
	{
		Vec2i theEnd = generationPrevCursorPos;
		theEnd.x++;
		generationNode->AddEscapedString(start, length, generationTokenStartPos, theEnd);
	}

	void CSParserTree::Generate()
	{
		generationLength = sourceCode.SourceLength();
		generationCurrentPtr = sourceCode.SourceStart();
		generationEnd = generationCurrentPtr + generationLength;
		generationPrevCursorPos = generationCursorPos = sourceCode.Origin();
		generationState = &CSParserTree::ReadingWhitespace;
		generationNode = root;

		while (generationState != NULL)
		{
			(this->*generationState)();
		}

		root->SetEnd(generationCursorPos);

		if (generationNode != root)
		{
			generationNode->SetEnd(generationCursorPos);
			char specimen[64];
			GetSpecimen(specimen, *generationNode);
			throw ParseException(sourceCode.Origin(), generationPrevCursorPos, sourceCode.Name(), ("Syntax error: missing close parenthesis character: ')'"), specimen, NULL);
		}
	}

	void CSParserTree::UpdateCursor(char c)
	{
		generationPrevCursorPos = generationCursorPos;

		switch (c)
		{
		case '\r':
			return;
		case '\n':
			generationCursorPos.x = root->Start().x;
			generationCursorPos.y++;
			return;
		default:
			generationCursorPos.x++;
			return;
		}
	}

	const char ESCAPECHAR = '&';

	void STATE_CALL CSParserTree::ReadingStringLiteral()
	{
		literalStringBuilder.clear();

		while (generationCurrentPtr < generationEnd)
		{
			char c = *generationCurrentPtr++;
			UpdateCursor(c);

			if (c == ESCAPECHAR)
			{
				if (generationCurrentPtr == generationEnd)
				{
					char specimen[64];
					GetSpecimen(specimen, *generationNode);
					throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), ("Literal string escape character missing trailing string"), specimen, NULL);
				}

				char c = *generationCurrentPtr++;
				UpdateCursor(c);

				char finalChar;

				// N.B from MAT: I decided not to code octal escape sequences, as base-8 is practically deprecated in every sphere

				if (c == (char)'x')
				{
					if (generationCurrentPtr + sizeof(char) >= generationEnd)
					{
						char specimen[64];
						GetSpecimen(specimen, *generationNode);
						throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), ("Literal string escape character missing trailing hexes"), specimen, NULL);
					}

					if (!TryParseSexHex(finalChar, generationCurrentPtr))
					{
						char specimen[64];
						GetSpecimen(specimen, *generationNode);
						throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), ("Literal string escape character. Unrecognized escape sequence"), specimen, NULL);
					}

					generationCursorPos.x += 2 * sizeof(char);
					generationCurrentPtr += 2 * sizeof(char);
				}
				else
				{
					if (!ParseEscapeCharacter(finalChar, c))
					{
						char specimen[64];
						GetSpecimen(specimen, *generationNode);
						throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), ("Unrecognized escape sequence in literal string."), specimen, NULL);
					}
				}

				literalStringBuilder.push_back(finalChar);
			}
			else if (c == '"')
			{
				size_t len = literalStringBuilder.size();
				if (len > INT_MAX)
				{
					char specimen[64];
					GetSpecimen(specimen, *generationNode);
					throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), ("Literal string was too long (> INT_MAX)"), specimen, NULL);
				}
				AddStringToken(len == 0 ? ("") : &literalStringBuilder[0], (int32)literalStringBuilder.size());
				generationState = &CSParserTree::ReadingWhitespace;
				return;
			}
			else
			{
				literalStringBuilder.push_back(c);
			}
		}

		char specimen[64];
		GetSpecimen(specimen, *generationNode);
		throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), ("Literal string not closed with a double quote character"), specimen, NULL);
	}

	void CSParserTree::OpenInnerExpression()
	{
		generationNode = new CSExpression(*this, generationPrevCursorPos, generationNode);
	}

	void CSParserTree::CloseInnerExpression()
	{
		generationNode->SetEnd(generationCursorPos);

		CSExpression* parent = generationNode->ConcreteParent();
		if (parent == NULL)
		{
			char specimen[64];
			GetSpecimen(specimen, *generationNode);
			throw ParseException(sourceCode.Origin(), generationPrevCursorPos, sourceCode.Name(), ("Too many close parenthesis characters: ')'"), specimen, NULL);
		}

		generationNode = parent;
	}

	void STATE_CALL CSParserTree::ReadingLineComment()
	{
		while (generationCurrentPtr < generationEnd)
		{
			char c = *generationCurrentPtr++;
			UpdateCursor(c);

			if (c == '\n')
			{
				generationState = &CSParserTree::ReadingWhitespace;
				return;
			}
		}

		generationState = NULL;
	}

	void STATE_CALL CSParserTree::ReadingParagraphComment()
	{
		while (generationCurrentPtr < generationEnd)
		{
			char c = *generationCurrentPtr++;
			UpdateCursor(c);

			if (c == '*')
			{
				if (generationCurrentPtr < generationEnd)
				{
					char c = *generationCurrentPtr++;
					UpdateCursor(c);

					if (c == '/')
					{
						generationState = &CSParserTree::ReadingWhitespace;
						return;
					}
				}
			}
		}

		char specimen[64];
		GetSpecimen(specimen, *generationNode);
		throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), ("Paragraph-style comment /* ... */ not closed with '*/'"), specimen, NULL);
	}

	void STATE_CALL CSParserTree::ReadingToken()
	{
		while (generationCurrentPtr < generationEnd)
		{
			char c = *generationCurrentPtr++;
			UpdateCursor(c);
			switch (c)
			{
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				AddToken(this->startOfToken, generationCurrentPtr - 1);
				generationState = &CSParserTree::ReadingWhitespace;
				return;
			case '(':
				AddToken(this->startOfToken, generationCurrentPtr - 1);
				OpenInnerExpression();
				generationState = &CSParserTree::ReadingWhitespace;
				return;
			case ')':
				AddToken(this->startOfToken, generationCurrentPtr - 1);
				CloseInnerExpression();
				generationState = &CSParserTree::ReadingWhitespace;
				return;
			default:
				continue;

			}
		}

		AddToken(this->startOfToken, generationCurrentPtr);
		generationState = NULL;
	}

	void STATE_CALL CSParserTree::ReadingWhitespace()
	{
		while (generationCurrentPtr < generationEnd)
		{
			char c = *generationCurrentPtr++;
			UpdateCursor(c);
			switch (c)
			{
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				break;
			case '(':
				OpenInnerExpression();
				break;
			case ')':
				CloseInnerExpression();
				break;
			case '"':
				generationTokenStartPos = generationPrevCursorPos;
				startOfToken = generationCurrentPtr;
				generationState = &CSParserTree::ReadingStringLiteral;
				return;
			case '/':
				if (*generationCurrentPtr == '/')
				{
					generationTokenStartPos = generationPrevCursorPos;
					generationCurrentPtr++;
					UpdateCursor('/');
					generationState = &CSParserTree::ReadingLineComment;
				}
				else if (*generationCurrentPtr == '*')
				{
					generationTokenStartPos = generationPrevCursorPos;
					UpdateCursor('*');
					generationCurrentPtr++;
					generationState = &CSParserTree::ReadingParagraphComment;
				}
				else
				{
					generationTokenStartPos = generationPrevCursorPos;
					startOfToken = generationCurrentPtr - 1;
					generationState = &CSParserTree::ReadingToken;
				}
				return;
			default:
				generationTokenStartPos = generationPrevCursorPos;
				startOfToken = generationCurrentPtr - 1;
				generationState = &CSParserTree::ReadingToken;
				return;

			}
		}

		generationState = NULL;
	}

	bool IsUnicode(const char* buffer, long len)
	{
		return (len > 2 && buffer[0] == 0) || buffer[1] == 0;
	}

	class CSexParser final : public ISParser
	{
	private:
		refcount_t refcount;
		std::vector<char> tempBuffer;
	public:
		CSexParser() : refcount(1)
		{

		}

		virtual ISParserTree* CreateTree(ISourceCode& sourceCode)
		{
			CSParserTree* tree = new CSParserTree(*this, sourceCode, sourceCode.Origin(), sourceCode.Origin(), 0, NULL);

			try
			{
				tree->Generate();
			}
			catch (ParseException&)
			{
				delete tree;
				throw;
			}
			catch (std::exception&)
			{
				delete tree;
				throw;
			}
			catch (IException&)
			{
				delete tree;
				throw;
			}

			return tree;
		}

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

		virtual ISourceCode* DuplicateSourceBuffer(cstr buffer, int segmentLength, const Vec2i& origin, cstr name)
		{
			if (segmentLength < 0)
			{
				size_t computedSegmentLength = StringLength(buffer);
				if (computedSegmentLength > INT_MAX - 1)
				{
					cstr specimen = ("!!!Bad source buffer!!!");
					throw ParseException(Vec2i{ 0,0 }, Vec2i{ 0,0 }, name, ("The string length of the input buffer was greater than INT_MAX"), specimen, NULL);
				}
				segmentLength = (int32)computedSegmentLength;
			}
			return new Anon::CSourceCodeCopy(buffer, segmentLength, origin, name);
		}

		virtual ISourceCode* ProxySourceBuffer(cstr bufferRef, int segmentLength, const Vec2i& origin, cstr nameRef)
		{
			if (segmentLength < 0)
			{
				size_t computedSegmentLength = StringLength(bufferRef);
				if (computedSegmentLength > INT_MAX)
				{
					cstr specimen = ("!!!Bad source buffer!!!");
					throw ParseException(Vec2i{ 0,0 }, Vec2i{ 0,0 }, nameRef, ("The string length of the input buffer was greater than INT_MAX"), specimen, NULL);
				}
				segmentLength = (int32)computedSegmentLength;
			}
			return new Anon::CSourceCodeProxy(bufferRef, segmentLength, origin, nameRef);
		}

		virtual ISourceCode* LoadSource(cstr filename, const Vec2i& origin)
		{
			long len;
			FileReader reader(filename, len);

			tempBuffer.resize(len + 1);
			size_t nBytes = fread(&tempBuffer[0], 1, len, reader.fp);
			tempBuffer[nBytes] = 0;

			ISourceCode* src = LoadSource(filename, origin, &tempBuffer[0], (long)nBytes);

			return src;
		}

		virtual ISourceCode* LoadSource(cstr moduleName, const Vec2i& origin, const char* buffer, long len)
		{
			ISourceCode* src;

			if (IsUnicode(buffer, len))
			{
				if (sizeof(char) != 1)
				{
					src = DuplicateSourceBuffer((cstr)buffer, len >> 1, origin, moduleName);
				}
				else
				{
					char* sexBuffer = (char*)alloca(len << 1);
					for (int i = 0; i < (len << 1); ++i)
					{
						char c = buffer[i];
						sexBuffer[i] = c > 127 ? '?' : c;
					}
					sexBuffer[len] = 0;

					src = DuplicateSourceBuffer(sexBuffer, len >> 1, origin, moduleName);
				}
			}
			else
			{
				if (sizeof(char) == 1)
				{
					src = DuplicateSourceBuffer((cstr)buffer, len, origin, moduleName);
				}
				else
				{
					char* sexBuffer = (char*)alloca((len << 1) + sizeof(char));
					for (int i = 0; i < len; ++i)
					{
						char c = buffer[i];
						sexBuffer[i] = c & 0x00FF;
					}

					sexBuffer[len] = 0;

					src = DuplicateSourceBuffer(sexBuffer, len, origin, moduleName);
				}
			}

			return src;
		}
	};

	ISParser& CSParserTree::Parser()
	{
		return parser;
	}

	void Release(CSParserTree* tree)
	{
		if (tree != NULL)
		{
			tree->Release();
		}
	}

	CSParserTree* ConstructTransform(CSParserTree& prototype, const Vec2i& start, const Vec2i& end, CSExpression* original)
	{
		CSParserTree* tree = new CSParserTree((CSexParser&)prototype.Parser(), (ISourceCode&)prototype.Source(), start, end, prototype.TransformDepth() + 1, original);
		return tree;
	}

	const ISExpression* GetOriginal(CSParserTree& tree)
	{
		return tree.GetOriginal();
	}
}

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

			writer.Append(L"%S", segment);
		}
	}
}

/////////////////////// Entry point ///////////////////////////
SEXY_SPARSER_API Rococo::Sex::ISParser* Sexy_CreateSexParser()
{
	return new ANON::CSexParser();
}