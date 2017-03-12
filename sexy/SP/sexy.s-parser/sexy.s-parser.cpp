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
#include "sexy.strings.h"
#include "sexy.stdstrings.h"
#include "sexy.s-parser.source.inl"
#include "sexy.s-parser.symbols.inl"

#include "..\..\include\rococo.io.h"

#include <stdarg.h>

using namespace Sexy;
using namespace Sexy::Sex;

namespace
{
	class CSexParser;
	class CSExpression;

	void Release(CSParserTree* tree);
	int TransformDepth(CSParserTree& tree);
	CSParserTree* ConstructTransform(CSParserTree& prototype, const Vec2i& start, const Vec2i& end, CSExpression* original);
	const ISExpression* GetOriginal(CSParserTree& tree);

	struct FileReader
	{
		FILE* fp;

		FileReader(const csexstr name, long& len)
		{
#ifdef SEXCHAR_IS_WIDE
			errno_t errNo = _wfopen_s(&fp, name, SEXTEXT("rb"));
#else
			errno_t errNo = fopen_s(&fp, name, "r");
#endif
			if (errNo != 0)
			{
				char err[256];
				strerror_s(err, 256, errNo);

				sexstringstream streamer;
				streamer << SEXTEXT("Error ") << errNo << SEXTEXT(" opening file: ") << err << std::ends;
				
            throw ParseException(Vec2i{ 0,0 }, Vec2i{ 0,0 }, name, streamer.str().c_str(), SEXTEXT("<none>"), NULL);
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

	class CSExpression: public ISExpressionBuilder
	{
	private:
		CSParserTree& tree;
		Vec2i start;
		Vec2i end;
		size_t startOffset;
		size_t endOffset;
		EXPRESSION_TYPE type;
		sexstring Symbol;
		CSExpression* parent;
		CSParserTree* transform; // 'this->transform' is the macro transform of 'this'

		typedef std::list<CSExpression*> TChildrenList;
		typedef std::vector<CSExpression*> TChildrenVector;
		TChildrenVector children;
	
		void AddChild(CSExpression* child)
		{
			children.push_back(child);
			if (type != EXPRESSION_TYPE_COMPOUND && type != EXPRESSION_TYPE_NULL)
			{
				SEXCHAR specimen[64];
				GetSpecimen(specimen, *child);
				throw ParseException(start, end, SEXTEXT("Algorithmic error"), SEXTEXT("Error adding child. ExpressionType was not NULL type"), specimen, NULL);
			}
			type = EXPRESSION_TYPE_COMPOUND;
		}
	public:
		CSExpression(CSParserTree& _tree, const Vec2i& _start, size_t _startOffset, CSExpression* _parent):
			tree(_tree),
			type(EXPRESSION_TYPE_NULL),
			start(_start),
			end(_start),
			parent(_parent),
			Symbol(NULL),
			transform(NULL),
			children(NULL),
			startOffset(_startOffset)
		{
			children.reserve(4);
			if (_parent != NULL)
			{				
				_parent->AddChild(this);
			}
		}

		CSExpression(CSParserTree& _tree, const Vec2i& _start, const Vec2i& _end, size_t _startOffset, size_t _endOffset, CSExpression* _parent, csexstr data, int dataLen, bool isStringLiteral) :
			tree(_tree),
			type(isStringLiteral ? EXPRESSION_TYPE_STRING_LITERAL : EXPRESSION_TYPE_ATOMIC),
			start(_start),
			end(_end),
			parent(_parent),
			Symbol(NULL),
			transform(NULL),
			children(NULL),
			startOffset(_startOffset),
			endOffset(_endOffset)
		{
			Symbol = AddSymbol(tree, data, dataLen);

			if (_parent != NULL)
			{
				_parent->AddChild(this);
			}			
		}

		~CSExpression()
		{
			for(auto i = children.begin(); i != children.end(); ++i)
			{
				CSExpression* child = *i;
				delete child;
			}

			Release(transform);
		}

		virtual const ISExpression* GetOriginal() const
		{
			return ::GetOriginal(tree);
		}

		virtual void AddAtomic(csexstr token)
		{
			CSExpression* atomic = new CSExpression(tree, start, end, startOffset, endOffset, this, token, StringLength(token), false);
		}

		virtual void AddStringLiteral(csexstr token)
		{
			CSExpression* strliteral = new CSExpression(tree, start, end, startOffset, endOffset, this, token, StringLength(token), true);
		}

		virtual ISExpressionBuilder* AddChild()
		{
			CSExpression* child = new CSExpression(tree, start, startOffset, this);
			child->SetEnd(end, endOffset);
			return child;
		}

		virtual const Vec2i& Start() const { return start; }
		virtual const Vec2i& End() const		{ return end;		}
		virtual size_t StartOffset() const { return startOffset; }
		virtual size_t EndOffset() const { return endOffset; }
		virtual EXPRESSION_TYPE Type() const		{ return type;	}
		virtual const sexstring String() const;
		virtual const ISParserTree& Tree() const;
		virtual int NumberOfElements() const { return (int32) children.size(); }
		virtual const ISExpression& GetElement(int index) const { return *children[index]; }
		virtual const ISExpression* Parent() const { return parent; }
		virtual const int TransformDepth() const;

		virtual ISExpressionBuilder* CreateTransform();
		virtual ISExpression* GetTransform() const;

		CSExpression* ConcreteParent() { return parent; }
		
		void AddAtomic(csexstr sourceSegmentOrigin, int32 sourceSegmentLength, const Vec2i& start, const Vec2i& end, size_t startOffset, size_t endOffset);
		void AddEscapedString(csexstr sourceSegmentOrigin, int32 sourceSegmentLength, const Vec2i& start, const Vec2i& end, size_t startOffset, size_t endOffset);

		void SetEnd(const Vec2i& _end, size_t _endOffset)
		{ 
			end = _end;
			endOffset = _endOffset;
		}

		virtual bool operator == (const SEXCHAR* token) const
		{
			if (token == nullptr) return Symbol == nullptr;
			else return Symbol != nullptr && AreEqual(Symbol, token);
		}
	};

#define STATE_CALL __fastcall

	class CSParserTree: public ISParserTree
	{
	private:
		int transformDepth;
		CSExpression* root;
		CSexParser& parser;
		refcount_t refcount;
		ISourceCode& sourceCode;
		CSymbols symbols;
		typedef std::vector<SEXCHAR> TStringBuilder;
		TStringBuilder literalStringBuilder;

		int generationLength;
		csexstr generationCurrentPtr;
		csexstr generationEnd;
		Vec2i generationTokenStartPos;
		Vec2i generationCursorPos;
		Vec2i generationPrevCursorPos;
		CSExpression* generationNode;
		csexstr startOfToken; 
		CSExpression* original;

		typedef void (STATE_CALL CSParserTree::*FN_STATE)();

		FN_STATE generationState;

		void AddToken(csexstr start, csexstr end);
		void AddStringToken(csexstr start, int32 length);
		void UpdateCursor(SEXCHAR c);
		void OpenInnerExpression();
		void CloseInnerExpression();

		void STATE_CALL ReadingStringLiteral();
		void STATE_CALL ReadingLineComment();
		void STATE_CALL ReadingParagraphComment();
		void STATE_CALL ReadingToken();
		void STATE_CALL ReadingWhitespace();

	public:
		CSParserTree(CSexParser& _parser, ISourceCode& _sourceCode, const Vec2i& start, const Vec2i& end, int _transformDepth, CSExpression* _original):
			refcount(1),
			parser(_parser),
			sourceCode(_sourceCode),
			transformDepth(_transformDepth),
			original(_original),
			symbols(_sourceCode.SourceLength())
		{
			sourceCode.AddRef();
			root = new CSExpression(*this, start, 0, NULL);
			root->SetEnd(end, _sourceCode.SourceLength()-1);
		}

		~CSParserTree()
		{
			delete root;
			sourceCode.Release();
		}

		CSymbols& Symbols() { return symbols; }
		void Generate();

		virtual const int TransformDepth() const	 { return transformDepth; }
		virtual ISParser& Parser();
		virtual ISExpression& Root()				{	return *root;		}
		virtual const ISExpression& Root() const	{	return *root;		}
		virtual ISExpressionBuilder* BuilderRoot()  {	return root;		}
		virtual const ISourceCode& Source()	const	{	return sourceCode;	}		
		virtual refcount_t AddRef()					{	return ++refcount;	}
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

	ISExpressionBuilder* CSExpression::CreateTransform() 
	{
		Release(transform);
		transform = NULL;
		transform = ConstructTransform(tree, start, end, this);
		return transform->BuilderRoot();
	}

	const sexstring CSExpression::String() const
	{
		return this->Symbol;
	}

	ISExpression* CSExpression::GetTransform() const { return transform == NULL ? NULL : &transform->Root(); }

	const int CSExpression::TransformDepth() const { return tree.TransformDepth(); }

	sexstring AddSymbol(CSParserTree& tree, csexstr data, int dataLen)
	{
		sexstring s = tree.Symbols().AddSymbol(data, dataLen);
		return s;
	}

	void CSExpression::AddAtomic(csexstr sourceSegmentOrigin, int32 sourceSegmentLength, const Vec2i& start, const Vec2i& end, size_t startOffset, size_t endOffset)
	{
		new CSExpression(tree, start, end, startOffset, endOffset, this, sourceSegmentOrigin, sourceSegmentLength, false);
	}

	void CSExpression::AddEscapedString(csexstr sourceSegmentOrigin, int32 sourceSegmentLength, const Vec2i& start, const Vec2i& end, size_t startOffset, size_t endOffset)
	{
		new CSExpression(tree, start, end, startOffset, endOffset, this, sourceSegmentOrigin, sourceSegmentLength, true);
	}

	const ISParserTree& CSExpression::Tree() const
	{
		return tree;
	}

	void CSParserTree::AddToken(csexstr start, csexstr end)
	{
		generationNode->AddAtomic(start, (int32)(end - start), generationTokenStartPos, generationPrevCursorPos, start - sourceCode.SourceStart(), end - sourceCode.SourceStart() - 1);
	}

	void CSParserTree::AddStringToken(csexstr start, int32 length)
	{
		Vec2i theEnd = generationPrevCursorPos;
		theEnd.x++;
		generationNode->AddEscapedString(start, length, generationTokenStartPos, theEnd, start - sourceCode.SourceStart(), start - sourceCode.SourceStart() + (size_t) length);
	}

	void CSParserTree::Generate()
	{
		generationLength = sourceCode.SourceLength();
		generationCurrentPtr = sourceCode.SourceStart();
		generationEnd = generationCurrentPtr + generationLength;
		generationPrevCursorPos = generationCursorPos = sourceCode.Origin();
		generationState = &CSParserTree::ReadingWhitespace;
		generationNode = root;

		while(generationState != NULL)
		{
			(this->*generationState)();
		}
		
		root->SetEnd(generationCursorPos, generationLength-1);

		if (generationNode != root)
		{
			generationNode->SetEnd(generationCursorPos, generationLength-1);
			SEXCHAR specimen[64];
			GetSpecimen(specimen, *generationNode);
			throw ParseException(sourceCode.Origin(), generationPrevCursorPos, sourceCode.Name(), SEXTEXT("Syntax error: missing close parenthesis character: ')'"), specimen, NULL);
		}	
	}

	void CSParserTree::UpdateCursor(SEXCHAR c)
	{
		generationPrevCursorPos = generationCursorPos;

		switch(c)
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

		while(generationCurrentPtr < generationEnd)
		{
			SEXCHAR c = *generationCurrentPtr++;
			UpdateCursor(c);
			
			if(c == ESCAPECHAR) 
			{
					if (generationCurrentPtr == generationEnd)
					{
						SEXCHAR specimen[64];
						GetSpecimen(specimen, *generationNode);
						throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), SEXTEXT("Literal string escape character missing trailing string"), specimen, NULL);
					}

					SEXCHAR c = *generationCurrentPtr++;
					UpdateCursor(c);

					SEXCHAR finalChar;

					// N.B from MAT: I decided not to code octal escape sequences, as base-8 is practically deprecated in every sphere

					if (c == (SEXCHAR)'x')
					{
						if (generationCurrentPtr + sizeof(SEXCHAR) >= generationEnd)
						{
							SEXCHAR specimen[64];
							GetSpecimen(specimen, *generationNode);
							throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), SEXTEXT("Literal string escape character missing trailing hexes"), specimen, NULL);
						}

						if (!TryParseSexHex(finalChar, generationCurrentPtr))
						{
							SEXCHAR specimen[64];
							GetSpecimen(specimen, *generationNode);
							throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), SEXTEXT("Literal string escape character. Unrecognized escape sequence"), specimen, NULL);
						}

						generationCursorPos.x += 2 * sizeof(SEXCHAR);
						generationCurrentPtr += 2 * sizeof(SEXCHAR);
					}
					else
					{
						if (!ParseEscapeCharacter(finalChar, c))
						{
							SEXCHAR specimen[64];
							GetSpecimen(specimen, *generationNode);
							throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), SEXTEXT("Unrecognized escape sequence in literal string."), specimen, NULL);
						}
					}
					
					literalStringBuilder.push_back(finalChar);
			}
			else if (c == '"')
			{
				size_t len = literalStringBuilder.size();
				if (len > INT_MAX)
				{
					SEXCHAR specimen[64];
					GetSpecimen(specimen, *generationNode);
					throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), SEXTEXT("Literal string was too long (> INT_MAX)"), specimen, NULL);
				}
				AddStringToken(len == 0 ? SEXTEXT("") : &literalStringBuilder[0], (int32) literalStringBuilder.size());
				generationState = &CSParserTree::ReadingWhitespace;
				return;
			}
			else
			{
				literalStringBuilder.push_back(c);
			}
		}

		SEXCHAR specimen[64];
		GetSpecimen(specimen, *generationNode);
		throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), SEXTEXT("Literal string not closed with a double quote character"), specimen, NULL);
	}

	void CSParserTree::OpenInnerExpression()
	{
		generationNode = new CSExpression(*this, generationPrevCursorPos, generationCurrentPtr - sourceCode.SourceStart() - 1, generationNode);
	}

	void CSParserTree::CloseInnerExpression()
	{
		generationNode->SetEnd(generationCursorPos, generationCurrentPtr - sourceCode.SourceStart()-1);

		CSExpression* parent = generationNode->ConcreteParent();
		if (parent == NULL)
		{
			SEXCHAR specimen[64];
			GetSpecimen(specimen, *generationNode);
			throw ParseException(sourceCode.Origin(), generationPrevCursorPos, sourceCode.Name(), SEXTEXT("Too many close parenthesis characters: ')'"), specimen, NULL);
		}
		
		generationNode = parent;
	}

	void STATE_CALL CSParserTree::ReadingLineComment()
	{
		while(generationCurrentPtr < generationEnd)
		{
			SEXCHAR c = *generationCurrentPtr++;
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
		while(generationCurrentPtr < generationEnd)
		{
			SEXCHAR c = *generationCurrentPtr++;
			UpdateCursor(c);

			if (c == '*')
			{
				if (generationCurrentPtr < generationEnd)
				{
					SEXCHAR c = *generationCurrentPtr++;
					UpdateCursor(c);

					if (c == '/')
					{
						generationState = &CSParserTree::ReadingWhitespace;
						return;
					}
				}
			}
		}

		SEXCHAR specimen[64];
		GetSpecimen(specimen, *generationNode);
		throw ParseException(generationTokenStartPos, generationPrevCursorPos, sourceCode.Name(), SEXTEXT("Paragraph-style comment /* ... */ not closed with '*/'"), specimen, NULL);
	}

	void STATE_CALL CSParserTree::ReadingToken()
	{
		while(generationCurrentPtr < generationEnd)
		{
			SEXCHAR c = *generationCurrentPtr++;
			UpdateCursor(c);
			switch(c)
			{
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				AddToken(this->startOfToken, generationCurrentPtr-1);
				generationState = &CSParserTree::ReadingWhitespace;
				return;
			case '(':	
				AddToken(this->startOfToken, generationCurrentPtr-1);
				OpenInnerExpression();				
				generationState = &CSParserTree::ReadingWhitespace;
				return;
			case ')':
				AddToken(this->startOfToken, generationCurrentPtr-1);
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
		while(generationCurrentPtr < generationEnd)
		{
			SEXCHAR c = *generationCurrentPtr++;
			UpdateCursor(c);
			switch(c)
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
					startOfToken = generationCurrentPtr-1;
					generationState = &CSParserTree::ReadingToken;
				}
				return;
			default:				
				generationTokenStartPos = generationPrevCursorPos;
				startOfToken = generationCurrentPtr-1;
				generationState = &CSParserTree::ReadingToken;
				return;

			}
		}

		generationState = NULL;
	}

	bool IsUnicode(const char* buffer, long len)
	{
		return len > 2 && buffer[0] == 0 || buffer[1] == 0;
	}
	
	class CSexParser: public ISParser
	{
	private:
		refcount_t refcount;

	public:
		CSexParser(): refcount(1)
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
			catch(IException&)
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

		virtual ISourceCode* DuplicateSourceBuffer(csexstr buffer, int segmentLength, const Vec2i& origin, csexstr name)
		{
			if (segmentLength < 0)
			{
				size_t computedSegmentLength = StringLength(buffer);
				if (computedSegmentLength > INT_MAX-1)
				{
					csexstr specimen = SEXTEXT("!!!Bad source buffer!!!");
               throw ParseException(Vec2i{ 0,0 }, Vec2i{ 0,0 }, name, SEXTEXT("The string length of the input buffer was greater than INT_MAX"), specimen, NULL);
				}
				segmentLength = (int32) computedSegmentLength;
			}
			return new CSourceCodeCopy(buffer, segmentLength, origin, name);
		}		

		virtual ISourceCode* ProxySourceBuffer(csexstr bufferRef, int segmentLength, const Vec2i& origin, csexstr nameRef)
		{
			if (segmentLength < 0)
			{
				size_t computedSegmentLength = StringLength(bufferRef);
				if (computedSegmentLength > INT_MAX)
				{
					csexstr specimen = SEXTEXT("!!!Bad source buffer!!!");
               throw ParseException(Vec2i{ 0,0 }, Vec2i{ 0,0 }, nameRef, SEXTEXT("The string length of the input buffer was greater than INT_MAX"), specimen, NULL);
				}
				segmentLength = (int32) computedSegmentLength;
			}
			return new CSourceCodeProxy(bufferRef, segmentLength, origin, nameRef);
		}

		virtual ISourceCode* LoadSource(csexstr filename, const Vec2i& origin)
		{
			long len;
			FileReader reader(filename, len);

			char* buffer = (char*) _malloca(len+1);		
			size_t nBytes = fread(buffer, 1, len, reader.fp);
			buffer[len] = 0;

			ISourceCode* src = LoadSource(filename, origin, buffer, len);

			_freea(buffer);

			return src;
		}

		virtual ISourceCode* LoadSource(csexstr moduleName, const Vec2i& origin, const char* buffer, long len)
		{
			ISourceCode* src;

			if (IsUnicode(buffer, len))
			{
				if (sizeof(SEXCHAR) != 1)
				{
					src = DuplicateSourceBuffer((csexstr)buffer, len >> 1, origin, moduleName);
				}
				else
				{
					SEXCHAR* sexBuffer = (SEXCHAR*)_malloca(len << 1);
					for (int i = 0; i < (len << 1); ++i)
					{
						wchar_t c = buffer[i];
						sexBuffer[i] = c > 127 ? '?' : c;
					}
					sexBuffer[len] = 0;

					src = DuplicateSourceBuffer(sexBuffer, len >> 1, origin, moduleName);
				}
			}
			else
			{
				if (sizeof(SEXCHAR) == 1)
				{
					src = DuplicateSourceBuffer((csexstr)buffer, len, origin, moduleName);
				}
				else
				{
					SEXCHAR* sexBuffer = (SEXCHAR*)_malloca((len << 1) + sizeof(SEXCHAR));
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

	int TransformDepth(CSParserTree& tree)
	{
		return tree.TransformDepth();
	}

	CSParserTree* ConstructTransform(CSParserTree& prototype, const Vec2i& start, const Vec2i& end, CSExpression* original)
	{
		CSParserTree* tree = new CSParserTree((CSexParser&) prototype.Parser(), (ISourceCode&) prototype.Source(), start, end, prototype.TransformDepth() + 1, original);
		return tree;
	}

	const ISExpression* GetOriginal(CSParserTree& tree)
	{
		return tree.GetOriginal();
	}
}

namespace
{
	struct EscapeSequence
	{
		char text[8];
		size_t len;
	};

	static EscapeSequence* mapWcharToSequence = nullptr;

	void ClearEscapeMap()
	{
		delete[] mapWcharToSequence;
		mapWcharToSequence = nullptr;
	}

	void InitEscapeSequences()
	{
		if (mapWcharToSequence == nullptr)
		{
			mapWcharToSequence = new EscapeSequence[65536];
			atexit(ClearEscapeMap);

			for (uint32 i = 0; i < 65536; ++i)
			{
				sprintf_s(mapWcharToSequence[i].text, "&x%x", i);
				mapWcharToSequence[i].len = 6;
			}

			for (uint32 i = 32; i <= 255; ++i)
			{
				sprintf_s(mapWcharToSequence[i].text, "%c", i);
				mapWcharToSequence[i].len = 1;
			}

			sprintf_s(mapWcharToSequence[L'\r'].text, "&r");
			mapWcharToSequence[L'\r'].len = 2;

			sprintf_s(mapWcharToSequence[L'\n'].text, "&n");
			mapWcharToSequence[L'\n'].len = 2;

			sprintf_s(mapWcharToSequence[L'\t'].text, "&t");
			mapWcharToSequence[L'\t'].len = 2;

			sprintf_s(mapWcharToSequence[L'\"'].text, "&q");
			mapWcharToSequence[L'\"'].len = 2;

			sprintf_s(mapWcharToSequence[L'&'].text, "&&");
			mapWcharToSequence[L'&'].len = 2;
		}
	}
}

namespace
{
	void ThrowBadArg(const wchar_t* format, ...)
	{
		struct : public IException
		{
			wchar_t msg[256];
			int32 errorCode;

			virtual const wchar_t* Message() const
			{
				return msg;
			}

			virtual int32 ErrorCode() const
			{
				return errorCode;
			}
		} ex;

		va_list args;
		va_start(args, format);

		SafeVFormat(ex.msg, _TRUNCATE, format, args);

		ex.errorCode = 0;

		throw ex;
	}
}

namespace Sexy
{
	namespace Sex
	{
		using namespace Rococo::IO;

		bool IsToken(cr_sex s, const wchar_t* text)
		{
			return IsAtomic(s) && AreEqual(s.String(), text);
		}

		void EscapeScriptStringToAnsi(IBinaryWriter& writer, const wchar_t* text)
		{
			if (*text == 0) return;

			InitEscapeSequences();

			size_t segmentLength = 0;

			for (const wchar_t* s = text; *s != 0; s++)
			{
				segmentLength += mapWcharToSequence[*s].len;
			}

			if (segmentLength > 0xFFFFFFFFull)
			{
				ThrowBadArg(L"EscapeScriptStringToAnsi -> string length too long");
			}

			uint8* segment = (uint8*)_malloca(segmentLength);

			uint8* writePos = segment;
			for (const wchar_t* s = text; *s != 0; s++)
			{
				memcpy(writePos, mapWcharToSequence[*s].text, mapWcharToSequence[*s].len);
				writePos += mapWcharToSequence[*s].len;
			}

			writer.Write(segment, (uint32) segmentLength);

			_freea(segment);
		}

		void EscapeScriptStringToUnicode(IUnicode16Writer& writer, const wchar_t* text)
		{
			if (*text == 0) return;

			InitEscapeSequences();

			size_t segmentLength = 1;

			for (const wchar_t* s = text; *s != 0; s++)
			{
				segmentLength += mapWcharToSequence[*s].len;
			}

			wchar_t* segment = (wchar_t*)_malloca(sizeof(wchar_t) * segmentLength);

			wchar_t* writePos = segment;
			for (const wchar_t* s = text; *s != 0; s++)
			{
				SecureFormat(writePos, segment + segmentLength - writePos, L"%S", mapWcharToSequence[*s].text);
				writePos += mapWcharToSequence[*s].len;
			}

			*writePos = 0;

			writer.Append(L"%s", segment);

			_freea(segment);
		}
	}
}

/////////////////////// Entry point ///////////////////////////
SEXY_SPARSER_API Sexy::Sex::ISParser* Sexy_CreateSexParser()
{
	return new CSexParser();
}