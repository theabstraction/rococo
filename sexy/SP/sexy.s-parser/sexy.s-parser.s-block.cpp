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

#include "sexy.s-parser.stdafx.h"

#define _CRT_SECURE_NO_WARNINGS

#include <rococo.api.h>
#include <rococo.strings.h>
#include <rococo.os.h>

#include <stdio.h>
#include <memory>

#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#include <errno.h>
#endif

#include <new>
#include <rococo.stl.allocators.h>

namespace Rococo
{
	void ThrowIllFormedSExpression(int32 errorCode, cstr format, ...);
}

namespace Anon
{
	using namespace Rococo;
	using namespace Rococo::Sex;

	ROCOCOAPI ITokenBuilder
	{
		virtual void AddChar(char c) = 0;
	};

	ROCOCOAPI ISExpressionLinkBuilder : ISExpression
	{
		virtual void AddSibling(ISExpressionLinkBuilder* sibling) = 0;
		virtual ISExpressionLinkBuilder* GetNextSibling() = 0;
	};

	ROCOCOAPI ICompoundSExpression : ISExpressionLinkBuilder
	{
		virtual void AddChild(ISExpressionLinkBuilder* child) = 0;
		virtual ISExpressionLinkBuilder* GetFirstChild() = 0;
		virtual void SetArrayStart(ISExpression** pArray) = 0;
		virtual void SetOffsets(int32 startOffset, int32 endOffset) = 0;
	};

	ROCOCOAPI IExpressionBuilder
	{
		virtual void AddAtomic(cstr begin, cstr end, ICompoundSExpression* parent) = 0;
		virtual void AddComment(cstr begin, cstr end) = 0;
		virtual ICompoundSExpression* AddCompound(cstr beginPoint, int depth, ICompoundSExpression* parent) = 0;
		virtual void FinishCompound(cstr beginPoint, cstr endPoint, int depth, ICompoundSExpression* compound) = 0;
		virtual cstr ParseString(cstr sStart, cstr sEnd, ICompoundSExpression* parent) = 0;
	};

	bool IsBlank(char c)
	{
		switch (c)
		{
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			return true;
		default:
			return false;
		}
	}

	int32 HexCharToValue(int32 i)
	{
		if (i >= '0' && i <= '9')
		{
			return i - '0';
		}

		if (i >= 'A' && i <= 'F')
		{
			return i - 'A' + 10;
		}

		if (i >= 'a' && i <= 'f')
		{
			return i - 'a' + 10;
		}

		Throw(0, "Bad hex value");
		return -1;
	}

	char FromHexToChar(int32 high16, int32 low16)
	{
		int32 h = HexCharToValue(high16);
		int32 l = HexCharToValue(low16);
		return (char)((h << 4) + l);
	}

	template<char ESCAPE_CHAR> cstr ParseSexyString(cstr sStart, cstr sEnd, ITokenBuilder& tokenBuilder)
	{
		cstr  p = sStart;

		while (true)
		{
			// Not in an escape sequence
			while (p != sEnd)
			{
				char c = *p++;

				if (c == ESCAPE_CHAR)
				{
					break;
				}
				else if (c == '"')
				{
					tokenBuilder.AddChar(0);
					return p;
				}
				else
				{
					tokenBuilder.AddChar(c);
				}
			}

			if (p >= sEnd)
			{
				Throw(0, "Unexpected end of string literal");
			}

			switch (*p)
			{
			case ESCAPE_CHAR:
			case 'e':
				tokenBuilder.AddChar(ESCAPE_CHAR);
				break;
			case 'a':
				tokenBuilder.AddChar('\a');
				p++;
				break;
			case 'b':
				tokenBuilder.AddChar('\b');
				p++;
				break;
			case 'f':
				tokenBuilder.AddChar('\f');
				p++;
				break;
			case 'r':
				tokenBuilder.AddChar('\r');
				p++;
				break;
			case 'n':
				tokenBuilder.AddChar('\n');
				p++;
				break;
			case 't':
				tokenBuilder.AddChar('\t');
				p++;
				break;
			case 'v':
				tokenBuilder.AddChar('\v');
				p++;
				break;
			case '\'':
				tokenBuilder.AddChar('\'');
				p++;
				break;
			case '\"':
				tokenBuilder.AddChar('\"');
				p++;
				break;
			case 'x':
			{
				p++;
				char high16 = *p++;
				char low16 = *p++;

				tokenBuilder.AddChar(FromHexToChar(high16, low16));

				if (p >= sEnd)
				{
					Throw(0, "unexpected end of escape sequence");
				}
			}
			break;
			}
		}
	}


	struct TestExpressionBuilder : public IExpressionBuilder
	{
		enum { MAX_ATOMIC_TOKEN_SIZE = 32768 };
		enum { MAX_STRING_SIZE = 32768 };
		enum { ESCAPE_CHAR = '&' };

		int depth = 0;

		void AddTabs(int depth)
		{
			for (int i = 0; i < depth; ++i)
			{
				printf("\t");
			}
		}

		void AddAtomic(cstr begin, cstr end, ICompoundSExpression* parent) override
		{
			size_t diff = end - begin;
			if (diff > (MAX_ATOMIC_TOKEN_SIZE - 1))
			{
				Throw(0, "Token inside of expression was greater than %d characters length", MAX_ATOMIC_TOKEN_SIZE - 1);
			}

			char buffer[MAX_ATOMIC_TOKEN_SIZE];
			memcpy(buffer, begin, diff);
			buffer[diff] = 0;

			AddTabs(depth);
			printf("%s\n", buffer);
		}

		void AddComment(cstr begin, cstr end) override
		{

		}

		ICompoundSExpression* AddCompound(cstr beginPoint, int depth, ICompoundSExpression* parent) override
		{
			this->depth = depth;
			return nullptr;
		}

		void FinishCompound(cstr beginPoint, cstr endPoint, int depth, ICompoundSExpression* compound) override
		{

		}

		cstr ParseString(cstr sStart, cstr sEnd, ICompoundSExpression* parent) override
		{
			struct : ITokenBuilder
			{
				char buffer[MAX_ATOMIC_TOKEN_SIZE];

				char* pWritePos = buffer;
				char* pEnd = buffer + MAX_ATOMIC_TOKEN_SIZE;

				void AddChar(char c) override
				{
					*pWritePos++ = c;
					if (pWritePos == pEnd)
					{
						Throw(0, "Maximum string literal size");
					}
				}
			} addToBuffer;
			cstr next = ParseSexyString<'&'>(sStart, sEnd, addToBuffer);

			AddTabs(depth);
			printf("'%s'\n", addToBuffer.buffer);

			return next;
		}
	};

	Vec2i OffsetToPos(int32 offset, cstr start, cstr toOffsetPoint, Vec2i origin)
	{
		if (offset < 0)
		{
			Throw(0, "ExpressionTree::OffsetToPos: Bad offset");
			return { 0,0 };
		}

		Vec2i pos = origin;

		for (auto* p = start; p < toOffsetPoint; ++p)
		{
			switch (*p)
			{
			case '\r':
				break;
			case '\n':
				pos.y++;
				pos.x = origin.x;
				break;
			default:
				pos.x++;
				break;
			}
		}

		return pos;
	}

	struct SParser
	{
		cstr sExpression;
		cstr end;
		size_t len;
		cstr name;

		cstr p;

		IExpressionBuilder& builder;

		SParser(cstr sNull_terminated_Expression, IExpressionBuilder& _builder) : sExpression(sNull_terminated_Expression), builder(_builder)
		{
			if (sNull_terminated_Expression == nullptr) Throw(0, "Parser::Parse - null expression forbidden");
			len = rlen(sExpression);
			end = sExpression + len;
		}

		SParser(cstr _sExpression, size_t length, IExpressionBuilder& _builder) : sExpression(_sExpression), builder(_builder)
		{
			if (_sExpression == nullptr) ThrowIllFormedSExpression(0, "Parser::Parse - null expression forbidden");
			len = length;
			end = sExpression + len;
		}

		void Parse(ICompoundSExpression* root)
		{
			cstr next = sExpression;
			
			try
			{
				next = ParseCompound(sExpression, 0, root);

				if (next != end)
				{
					ThrowIllFormedSExpression((int)(end - next), "Parser::Parse failed: too many close parenthesis characters");
				}

				builder.FinishCompound(sExpression, end, 0, root);
			}
			catch (IException& ex)
			{
				char specimen[64];
				SafeFormat(specimen, sizeof(specimen), "");
				Vec2i pos = OffsetToPos(ex.ErrorCode(), sExpression, next, { 1,1 });
				throw ParseException(pos, pos, name, ex.Message(), specimen, nullptr);
			}
		}

		cstr ParseAtomic(cstr sAtomic, ICompoundSExpression* parent)
		{
			cstr next = sAtomic;

			while (true)
			{
				char c = *next;

				if (next == end)
				{
					builder.AddAtomic(sAtomic, next, parent);
					return end;
				}

				if (IsBlank(c))
				{
					builder.AddAtomic(sAtomic, next, parent);
					return next + 1;
				}

				switch (c)
				{
				case '(':
					builder.AddAtomic(sAtomic, next, parent);
					return next;
				case ')':
					builder.AddAtomic(sAtomic, next, parent);
					return next;
				default:
					next++;
					break;
				}
			}
		}

		cstr ParseString(cstr sString, ICompoundSExpression* parent)
		{
			try
			{
				return builder.ParseString(sString, end, parent);
			}
			catch (IException& ex)
			{
				Throw((int)(sString - sExpression), "%s", ex.Message());
				return sString;
			}
		}

		cstr ParseCompound(cstr sCompound, int depth, ICompoundSExpression* parent)
		{
			cstr next = sCompound;

			while (true)
			{
				if (next == end) break;

				next = ParseBlankspace(next);

				if (next == end) break;

				char c = *next++;

				switch (c)
				{
				case '(':
					{
						auto* child = builder.AddCompound(sCompound, depth + 1, parent);
						next = ParseCompound(next, depth + 1, child);
					}
					break;
				case ')':
					builder.FinishCompound(sCompound, next - 1, depth, parent);
					return next;
				case '"':
					next = ParseString(next, parent);
					break;
				case '/':
					if (*next == '/')
					{
						next = ParseLineComment(next + 1);
					}
					else if (*next == '*')
					{
						next = ParseBlockComment(next + 1);
					}
					else
					{
						next = ParseAtomic(next - 1, parent);
					}
					break;
				default:
					next = ParseAtomic(next - 1, parent);
					break;
				}
			}

			if (depth != 0)
			{
				ThrowIllFormedSExpression((int)(next - sExpression), "Missing close parenthesis character before end of s-expression");
			}

			return next;
		}

		cstr ParseBlankspace(cstr start)
		{
			auto* p = start;
			while (p != end && IsBlank(*p))
			{
				++p;
			}

			return p;
		}

		cstr ParseLineComment(cstr start)
		{
			auto* p = start;
			while (p != end)
			{
				switch (*p)
				{
				case '\r':
				case '\n':
					builder.AddComment(start, p);
					return p + 1;
				default:
					break;
				}
				++p;
			}

			return p;
		}

		cstr ParseBlockComment(cstr start)
		{
			auto* p = start;
			while (p != end)
			{
				switch (*p)
				{
				case '*':
					if (p[1] == '/')
					{
						builder.AddComment(start, p);
						return p + 2;
					}
				default:
					break;
				}
				++p;
			}

			return p;
		}
	};

	struct CompoundExpression;
	struct RootExpression;

	cr_sex  GetISExpression(const CompoundExpression& compound);

	ISExpression& GetISExpression(RootExpression& root);
	cr_sex GetISExpression(const RootExpression& root);

	struct ExpressionTree : Rococo::Sex::ISParserTree
	{
		char* block = nullptr;
		ISExpression** arraySets;
		RootExpression* root = nullptr;
		ISourceCode* sourceCode = nullptr;
		ISParser* sParser = nullptr;
		IAllocator& allocator;
		refcount_t refcount = 1;

		size_t aIndex = 0;

		ExpressionTree(IAllocator& _allocator) : allocator(_allocator)
		{

		}

		~ExpressionTree()
		{
			sParser->Release();
			if (sourceCode) sourceCode->Release();
			if (block)
			{
				allocator.FreeData(block);
			}
		}

		Vec2i OffsetToPos(int32 offset)
		{
			auto* end = sourceCode->SourceStart() + sourceCode->SourceLength();
			auto* finalPos = min(end, sourceCode->SourceStart() + offset);

			Vec2i origin = sourceCode->Origin();

			return Anon::OffsetToPos(offset, sourceCode->SourceStart(), finalPos, sourceCode->Origin());
		}

		void CovertListsToArrays(ICompoundSExpression& cs);

		ISExpression& Root() override
		{
			return GetISExpression(*root);
		}

		const ISExpression& Root() const override
		{
			return GetISExpression(*root);
		}

		ISParser& Parser() override
		{
			return *sParser;
		}

		const ISourceCode& Source() const override
		{
			return *sourceCode;
		}

		refcount_t AddRef() override
		{
			return ++refcount;
		}

		refcount_t Release() override
		{
			refcount--;
			if (refcount != 0)
			{
				return refcount;
			}
			else
			{
				this->~ExpressionTree();
				return 0;
			}
		}
	};

#pragma pack(push,1)
	struct CodeOffsets
	{
		int32 startOffset;
		int32 endOffset;
	};
#pragma pack(pop)

	struct AtomicExpression : ISExpressionLinkBuilder
	{
		ISExpression* parent = nullptr;
		ISExpressionLinkBuilder* next = nullptr;
		CodeOffsets offsets;
		sexstring_header header;

		AtomicExpression(ptrdiff_t startOffset, ptrdiff_t endOffset) :
			offsets{ (int32)startOffset, (int32)endOffset }
		{
		}

		void Free() override
		{
			Throw(0, "%s: S_Block expressions do not support method Free.", __func__);
		}

		void AddSibling(ISExpressionLinkBuilder* sibling) override
		{
			if (next == nullptr)
			{
				next = sibling;
				return;
			}

			ISExpressionLinkBuilder* last = this;
			for (auto i = next; i != nullptr; i = i->GetNextSibling())
			{
				last = i;
			}

			last->AddSibling(sibling);
		}

		int GetIndexOf(cr_sex s) const override
		{
			return -1;
		}

		ISExpressionLinkBuilder* GetNextSibling() override
		{
			return next;
		}

		const Vec2i Start() const override
		{
			return ((ExpressionTree&)Tree()).OffsetToPos(offsets.startOffset);
		}

		const Vec2i End() const override
		{
			return ((ExpressionTree&)Tree()).OffsetToPos(offsets.endOffset);;
		}

		EXPRESSION_TYPE Type() const override
		{
			return EXPRESSION_TYPE_ATOMIC;
		}

		const sexstring String() const override
		{
			auto* s = &const_cast<AtomicExpression*>(this)->header;
			return s;
		}

		const ISParserTree& Tree() const override
		{
			return parent->Tree();
		}

		int NumberOfElements() const override
		{
			return 0;
		}

		const ISExpression& GetElement(int index) const override
		{
			Throw(0, "Atomic node has no elements");
			return *this;
		}

		const ISExpression* Parent() const override
		{
			return parent;
		}

		const ISExpression* GetOriginal() const override
		{
			return nullptr;
		}

		bool operator == (const char* token) const override
		{
			return Eq(token, header.Buffer);
		}
	};

	struct LiteralExpression : ISExpressionLinkBuilder
	{
		ISExpression* parent = nullptr;
		ISExpressionLinkBuilder* next = nullptr;
		CodeOffsets offsets;
		sexstring_header header;

		LiteralExpression(ptrdiff_t startOffset, ptrdiff_t endOffset) : offsets { (int32)startOffset, (int32)endOffset }
		{
		}

		void Free() override
		{
			Throw(0, "%s: S_Block expressions do not support method Free.", __func__);
		}

		void AddSibling(ISExpressionLinkBuilder* sibling) override
		{
			if (next == nullptr)
			{
				next = sibling;
				return;
			}

			ISExpressionLinkBuilder* last = this;
			for (auto i = next; i != nullptr; i = i->GetNextSibling())
			{
				last = i;
			}

			last->AddSibling(sibling);
		}

		int GetIndexOf(cr_sex s) const override
		{
			return -1;
		}

		ISExpressionLinkBuilder* GetNextSibling() override
		{
			return next;
		}

		const Vec2i Start() const override
		{
			return ((ExpressionTree&)Tree()).OffsetToPos(offsets.startOffset);
		}

		const Vec2i End() const override
		{
			return ((ExpressionTree&)Tree()).OffsetToPos(offsets.endOffset);
		}

		EXPRESSION_TYPE Type() const override
		{
			return EXPRESSION_TYPE_STRING_LITERAL;
		}

		const sexstring String() const override
		{
			auto* s = &const_cast<LiteralExpression*>(this)->header;
			return s;
		}

		const ISParserTree& Tree() const override
		{
			return parent->Tree();
		}

		int NumberOfElements() const override
		{
			return 0;
		}

		const ISExpression& GetElement(int index) const override
		{
			Throw(0, "String literal has no elements");
			return *this;
		}

		const ISExpression* Parent() const override
		{
			return parent;
		}

		const ISExpression* GetOriginal() const override
		{
			return nullptr;
		}

		bool operator == (const char* token) const override
		{
			return Eq(token, header.Buffer);
		}
	};

	union LinkOrArray
	{
		// When the expression is first created, the children are added as a linked list
		// Once the list is complete, the list is converted to an array.
		ISExpressionLinkBuilder* firstChild;
		ISExpression** pArray;

		LinkOrArray() : firstChild(nullptr) {}
	};

	struct RootExpression : ICompoundSExpression
	{
		ExpressionTree* tree = nullptr;
		LinkOrArray children;
		int32 numberOfChildren = 0;
		ISExpressionLinkBuilder* lastChild = nullptr;

		void Free() override
		{
			Throw(0, "%s: S_Block expressions do not support method Free.", __func__);
		}

		int GetIndexOf(cr_sex s) const override
		{
			for (int i = 0; i < numberOfChildren; ++i)
			{
				cr_sex child = GetElement(i);
				if (&child == &s)
				{
					return i;
				}
			}

			return -1;
		}

		void SetOffsets(int32 startOffset, int32 endOffset) override
		{
		}

		void SetArrayStart(ISExpression** pArray) override
		{
			children.pArray = pArray;
		}

		void AddSibling(ISExpressionLinkBuilder* sibling) override
		{
			Throw(0, "Root cannnot have siblings");
		}

		ISExpressionLinkBuilder* GetNextSibling() override
		{
			return nullptr;
		}

		void AddChild(ISExpressionLinkBuilder* child) override
		{
			numberOfChildren++;
			if (children.firstChild == nullptr)
			{
				children.firstChild = child;
			}
			else
			{
				lastChild->AddSibling(child);
			}

			lastChild = child;
		}

		ISExpressionLinkBuilder* GetFirstChild() override
		{
			return children.firstChild;
		}

		const Vec2i Start() const override
		{
			return tree->sourceCode->Origin();
		}

		const Vec2i End() const override
		{
			return tree->OffsetToPos(tree->sourceCode->SourceLength());
		}

		EXPRESSION_TYPE Type() const override
		{
			return EXPRESSION_TYPE_COMPOUND;
		}

		const sexstring String() const override
		{
			return nullptr;
		}

		const ISParserTree& Tree() const override
		{
			return *tree;
		}

		int NumberOfElements() const override
		{
			return numberOfChildren;
		}

		const ISExpression& GetElement(int index) const override
		{
#ifdef _DEBUG
			if (index < 0 || index >= numberOfChildren) Rococo::Throw(0, "CompoundExpression.GetElement(index): index %d of %d out of range", index, numberOfChildren);
#endif
			return *children.pArray[index];
		}

		const ISExpression* Parent() const override
		{
			return nullptr;
		}

		const ISExpression* GetOriginal() const override
		{
			return nullptr;
		}

		bool operator == (const char* token) const override
		{
			return token == nullptr;
		}
	};

	ISExpression& GetISExpression(RootExpression& root) { return root; }
	cr_sex GetISExpression(const RootExpression& root) { return root; }

	struct CompoundExpression : ICompoundSExpression
	{
		ISExpression* parent = nullptr;
		ISExpressionLinkBuilder* nextSibling; // used in the generation phase, when expression form a linked list

		CodeOffsets offsets;
		LinkOrArray children;
		int32 numberOfChildren;

		void Free() override
		{
			Throw(0, "%s: S_Block expressions do not support method Free.", __func__);
		}

		void SetOffsets(int32 start, int32 end) override
		{
			offsets = { start, end };
		}

		void SetArrayStart(ISExpression** pArray) override
		{
			children.pArray = pArray;
		}

		void AddChild(ISExpressionLinkBuilder* child) override
		{
			numberOfChildren++;

			if (children.firstChild == nullptr)
			{
				children.firstChild = child;
			}
			else
			{
				children.firstChild->AddSibling(child);
			}
		}

		void AddSibling(ISExpressionLinkBuilder* sibling) override
		{
			if (nextSibling == nullptr)
			{
				nextSibling = sibling;
				return;
			}

			ISExpressionLinkBuilder* last = this;
			for (auto i = nextSibling; i != nullptr; i = i->GetNextSibling())
			{
				last = i;
			}

			last->AddSibling(sibling);
		}

		ISExpressionLinkBuilder* GetFirstChild() override
		{
			return children.firstChild;
		}

		int GetIndexOf(cr_sex s) const override
		{
			for (int i = 0; i < numberOfChildren; ++i)
			{
				cr_sex child = GetElement(i);
				if (&child == &s)
				{
					return i;
				}
			}

			return -1;
		}

		ISExpressionLinkBuilder* GetNextSibling() override
		{
			return nextSibling;
		}

		const Vec2i Start() const override
		{
			return ((ExpressionTree&)Tree()).OffsetToPos(offsets.startOffset);
		}

		const Vec2i End() const override
		{
			return ((ExpressionTree&)Tree()).OffsetToPos(offsets.endOffset);
		}

		EXPRESSION_TYPE Type() const override
		{
			return numberOfChildren == 0 ? EXPRESSION_TYPE_NULL : EXPRESSION_TYPE_COMPOUND;
		}

		const sexstring String() const override
		{
			Rococo::Sex::Throw(*this, "Compound node has no string value");
			return nullptr;
		}

		const ISParserTree& Tree() const override
		{
			return parent->Tree();
		}

		int NumberOfElements() const override
		{
			return numberOfChildren;
		}

		const ISExpression& GetElement(int index) const override
		{
#ifdef _DEBUG
			if (index < 0 || index >= numberOfChildren) Rococo::Sex::Throw(*this, "CompoundExpression.GetElement(index): index %d of %d out of range", index, numberOfChildren);
#endif
			return *children.pArray[index];
		}

		const ISExpression* Parent() const override
		{
			return parent;
		}

		const ISExpression* GetOriginal() const override
		{
			return nullptr;
		}

		bool operator == (const char* token) const override
		{
			return false;
		}
	};

	void ExpressionTree::CovertListsToArrays(ICompoundSExpression& cs)
	{
		auto* a = arraySets + aIndex;
		auto* startOfArray = a;
		for (auto i = cs.GetFirstChild(); i != nullptr; i = i->GetNextSibling())
		{
			*a++ = i;
		}
		cs.SetArrayStart(startOfArray);
		aIndex += cs.NumberOfElements();

		for (int32 j = 0; j < cs.NumberOfElements(); ++j)
		{
			auto& child = cs[j];
			if (child.Type() == EXPRESSION_TYPE_COMPOUND)
			{
				auto& c = (CompoundExpression&)child;
				CovertListsToArrays(c);
			}
		}
	}

	cr_sex  GetISExpression(const CompoundExpression& compound) { return compound; }

	struct SCostEvaluator : public IExpressionBuilder
	{
		size_t totalAtomicCost = 0; // Number of bytes, including terminating nul characters in all the atomic strings
		size_t totalCommentCost = 0; // Number of bytes, including terminating nul characters in all the comment strings
		size_t totalStringCost = 0; // Number of bytes, including terminating nul characters in all the literal strings
		size_t largestAtomicSize = 0;
		size_t largestCommentSize = 0;
		size_t largestStringSize = 0;
		size_t maxStringSize; // Max bytes in a literal string before an exception is thrown
		size_t maxAtomicSize; // Max bytes in an atomic string before an exception is thrown
		size_t branchCount = 0; // Number of compound expression (branches)
		size_t atomicCount = 0; // Number of atomic expressions 
		size_t stringCount = 0; // Number of literal strings

		SCostEvaluator(size_t _maxStringSize, size_t _maxAtomicSize) :
			maxStringSize(_maxStringSize), maxAtomicSize(_maxAtomicSize)
		{
			size_t absMax = 128_kilobytes;
			if (maxStringSize < 8 || maxStringSize > absMax)
			{
				Throw(0, "Max string size limit exceeded. Current limit is %llu bytes", absMax);
			}
		}

		void AddAtomic(cstr begin, cstr end, ICompoundSExpression* parent) override
		{
			size_t nBytes = end - begin + 1;

			if (nBytes >= maxStringSize)
			{
				Throw(0, "Error parsing s expression. Atomic token had greater than > %llu characters", maxAtomicSize);
			}

			size_t padding = (8 - (nBytes & 0x07LL)) & 0x07LL;

			largestAtomicSize = max(largestAtomicSize, nBytes);
			totalAtomicCost += nBytes + padding;

			atomicCount++;
		}

		void AddComment(cstr begin, cstr end) override
		{
			size_t nBytes = end - begin + 1;
			largestCommentSize = max(largestCommentSize, nBytes);
			totalCommentCost += nBytes;
		}

		ICompoundSExpression* AddCompound(cstr beginPoint, int depth, ICompoundSExpression* parent) override
		{
			enum { MAX_S_DEPTH = 128 };
			if (depth > MAX_S_DEPTH)
			{
				Throw(0, "Maximum compound expression depth of %u was reached.", MAX_S_DEPTH);
			}

			branchCount++;
			return nullptr;
		}

		void FinishCompound(cstr beginPoint, cstr endPoint, int depth, ICompoundSExpression* compound) override
		{

		}

		cstr ParseString(cstr sStart, cstr sEnd, ICompoundSExpression* parent) override
		{
			struct ANON : ITokenBuilder
			{
				size_t writeCount = 0;

				ANON()
				{
				}

				void AddChar(char c) override
				{
					writeCount++;
				}
			} x;

			cstr next = ParseSexyString<'&'>(sStart, sEnd, x);

			largestStringSize = max(largestStringSize, x.writeCount);
			totalStringCost += x.writeCount;

			size_t padding = (8 - (x.writeCount & 0x07LL)) & 0x07LL;

			totalStringCost += padding;

			stringCount++;

			return next;
		}

	};

	static void FreeAllocator(IAllocatorSupervisor *allocator)
	{
		if (allocator) allocator->Free();
	}

	// SBlockAllocator - an expression tree builder, the intent of which is to consolidate all allocations 
	//    from the heap into a single allocation, and so massively reduce memory management costs.
	//    we also want to maximize likelihood of two neighbouring atomic tokens being in the same cache line
	//    so that atomic & literal expressions are allocated consecutively and in same cache line as their string data
	// Most strings are small and we are aiming for < 64 bytes per cache line, so maybe 2-3 tokens per cache line
	struct SBlockAllocator : IExpressionBuilder
	{
		char* block;
		char* writePos;

		cstr sourceStart;

		CompoundExpression* nextFreeCompoundSlot;

		size_t totalSize;
		CompoundExpression* compoundArray;
		ISExpression** arraySets;

		enum { NO_MANS_LAND = 128 };

		const SCostEvaluator& ce;
		IAllocator& allocator;

		SBlockAllocator(const SCostEvaluator& _ce, IAllocator& _allocator, cstr _sourceStart) :
			ce(_ce), allocator(_allocator), sourceStart(_sourceStart)
		{
			/* Block
			   Concrete atomic + literal expressions
			   Concrete compound expressions
			   Expression pointer array block
			   Expression Tree
			   Root Expression
			*/
			size_t sizeofAtomicExpression = sizeof(AtomicExpression);
			totalSize = ce.atomicCount * sizeofAtomicExpression;
			totalSize += ce.totalAtomicCost;

			totalSize += ce.stringCount * sizeof(LiteralExpression);
			totalSize += ce.totalStringCost;

			size_t toCompounds = totalSize;

			totalSize += ce.branchCount * sizeof(CompoundExpression);

			size_t toArrayEntries = totalSize;
			totalSize += (ce.branchCount + ce.atomicCount + ce.stringCount) * sizeof(ISExpression*);

			totalSize += sizeof(ExpressionTree) + sizeof(RootExpression);

			block = (char*)allocator.Allocate(totalSize + NO_MANS_LAND);

#ifdef _DEBUG
			memset(block, 0xFE, NO_MANS_LAND + totalSize);
#endif

			writePos = block;

			compoundArray = (CompoundExpression*)(block + toCompounds);
			nextFreeCompoundSlot = compoundArray;

			arraySets = (ISExpression**)(block + toArrayEntries);

			auto* treeMemory = block + totalSize - sizeof(ExpressionTree) - sizeof(RootExpression);
			auto* tree = new (treeMemory) ExpressionTree(allocator);
			auto* root = new (treeMemory + sizeof(ExpressionTree)) RootExpression();
			root->tree = tree;
			tree->root = root;
		}

		ICompoundSExpression* Root()
		{
			auto* root = block + totalSize - sizeof(RootExpression);
			return (RootExpression*)root;
		}

		void ValidateMemory()
		{
#ifdef _DEBUG
			cstr nomansLand = block + totalSize;
			cstr endOfNoMandsLand = block + totalSize + NO_MANS_LAND;

			for (cstr c = nomansLand; c < endOfNoMandsLand; ++c)
			{
				if (*c != (char)0xFE)
				{
					Throw(0, "SBlockAllocator overwrite detected");
				}
			}
#endif
		}

		ExpressionTree* Detach()
		{
			auto* treeMemory = block + totalSize - sizeof(ExpressionTree) - sizeof(RootExpression);
			auto* tree = (ExpressionTree*)treeMemory;
			tree->block = block;
			block = nullptr;
			return tree;
		}

		~SBlockAllocator()
		{
			if (block)
			{
				ValidateMemory();
				allocator.FreeData(block);
			}
		}

		void AddAtomic(cstr begin, cstr end, ICompoundSExpression* parent) override
		{
			size_t nBytes = end - begin;

			size_t sizeofAtomic = sizeof(AtomicExpression) + nBytes + 1;
			size_t padding = (8 - (sizeofAtomic & 0x07LL)) & 0x07LL;
			sizeofAtomic += padding;

			if (nBytes > 0x7FFFFFFFLL)
			{
				Throw(0, "String exceeded absolute maximum size");
			}

#ifdef _DEBUG

			char* nextWritePos = sizeofAtomic + writePos;
			if (nextWritePos > (char*)compoundArray)
			{
				Throw(0, "Atomic token would overwrite block allocator inner bounds");
			}
#endif

			auto* atomic = new (writePos) AtomicExpression(begin - sourceStart, end - sourceStart);
			atomic->parent = parent;
			parent->AddChild(atomic);

			char* buffer = atomic->header.Buffer;
			memcpy(buffer, begin, nBytes);
			buffer[nBytes] = 0;
			atomic->header.Length = (int32)nBytes;

			writePos += sizeofAtomic;

		}

		void AddComment(cstr begin, cstr end) override
		{
			size_t nBytes = end - begin + 1;
		}

		ICompoundSExpression* AddCompound(cstr beginPoint, int depth, ICompoundSExpression* parent) override
		{
			auto* n = nextFreeCompoundSlot;
			new (n) CompoundExpression();
			n->parent = parent;
			parent->AddChild(n);
			nextFreeCompoundSlot++;
			return n;
		}

		void FinishCompound(cstr beginPoint, cstr endPoint, int depth, ICompoundSExpression* parent) override
		{
			parent->SetOffsets((int32)(beginPoint - sourceStart), (int32)(endPoint - sourceStart));
		}

		cstr ParseString(cstr sStart, cstr sEnd, ICompoundSExpression* parent) override
		{
			char* buffer = (char*)alloca(ce.maxAtomicSize);

			struct ANON : ITokenBuilder
			{
				char* buffer;
				char* writePos;

				ANON(char* _buffer) : buffer(_buffer), writePos(_buffer)
				{
				}

				void AddChar(char c) override
				{
					*writePos++ = c;
				}
			} x(buffer);

			cstr next = ParseSexyString<'&'>(sStart, sEnd, x);

			size_t nBytes = x.writePos - buffer;
#ifdef _DEBUG
			if (nBytes >= ce.maxStringSize)
			{
				Throw(0, "SBlockAllocator::ParseString. String overflow. Max %d characters reached.\n'%64.64s...'", ce.maxStringSize, buffer);
			}
#endif

			size_t sizeofLiteral = sizeof(LiteralExpression) + nBytes;
			size_t padding = (8 - (sizeofLiteral & 0x07LL)) & 0x07LL;
			sizeofLiteral += padding;

#ifdef _DEBUG
			if (sizeofLiteral + writePos > (char*)compoundArray)
			{
				Throw(0, "Literal token would overwrite block allocator inner bounds");
			}
#endif
			auto* literal = new (writePos) LiteralExpression(sStart - sourceStart, sEnd - sourceStart);
			literal->parent = parent;
			parent->AddChild(literal);

			char* persistentBuffer = literal->header.Buffer;
			memcpy(persistentBuffer, buffer, nBytes);
			literal->header.Length = (int32)nBytes - 1;
			writePos += sizeofLiteral;

			return next;
		}
	};

	struct SourceCode : public ISourceCode
	{
		char* block;
		Vec2i origin;
		char* buffer;
		int srcLength;
		char* name;
		bool proxied;
		refcount_t refcount = 1;
		IAllocator& allocator;

		SourceCode(IAllocator& _allocator) : allocator(_allocator) {}

		const Vec2i& Origin() const override { return origin; }
		cstr SourceStart() const override { return buffer; }
		const int SourceLength() const override { return srcLength; }
		cstr Name() const override { return name; }
		refcount_t AddRef()  override { return ++refcount; }
		refcount_t Release()  override
		{
			refcount--;
			if (refcount == 0)
			{
				allocator.FreeData(block);
				return 0;
			}
			else
			{
				return refcount;
			}
		}
	};

	struct SParser_2_0 : public ISParser
	{
		refcount_t refcount = 1;
		IAllocator& allocator;
		size_t maxStringLength;

		SParser_2_0(IAllocator& _allocator, size_t _maxStringLength) :
			maxStringLength(_maxStringLength),
			allocator(_allocator)
		{
		}

		~SParser_2_0()
		{
		}

		ISParserTree* CreateTree(ISourceCode& sourceCode) override
		{
			SCostEvaluator ce(maxStringLength, maxStringLength);

			SParser parser(sourceCode.SourceStart(), sourceCode.SourceLength(), ce);
			parser.name = sourceCode.Name();
			parser.Parse(nullptr);

			SBlockAllocator sba(ce, allocator, sourceCode.SourceStart());
			{
				SParser parser(sourceCode.SourceStart(), sba);
				parser.name = sourceCode.Name();
				parser.Parse(sba.Root());
			}

			auto* tree = sba.Detach();
			tree->sParser = this;
			tree->arraySets = sba.arraySets;
			AddRef();
			tree->sourceCode = &sourceCode;
			sourceCode.AddRef();
			tree->CovertListsToArrays(*tree->root);
			return tree;
		}

		ISourceCode* DuplicateSourceBuffer(cstr buffer, int segmentLength, const Vec2i& origin, const char* name) override
		{
			if (buffer == nullptr || name == nullptr)
			{
				Rococo::Throw(0, "ISParserTree::DuplicateSourceBuffer (%s) Bad argument", name);
			}

			if (segmentLength < 0)
			{
				size_t len = strlen(buffer);
				if (len > 0x7FFFFELL)
				{
					Rococo::Throw(0, "ISParserTree::DuplicateSourceBuffer - source code %s must be shorter than 2 gigabytes.", name);
				}

				segmentLength = (int)len;
			}

			// We create the source file object, a buffer and a name buffer in one allocation call
			// This cuts down allocation and deallocation count from 6 to 2

			size_t blockSize = sizeof(SourceCode) + segmentLength + 1 + strlen(name) + 1;
			char* block = (char*)allocator.Allocate(blockSize);
			auto* src = new (block) SourceCode(allocator);
			src->allocator = allocator;
			src->block = block;
			src->buffer = block + sizeof(SourceCode);
			memcpy(src->buffer, buffer, segmentLength);
			src->buffer[segmentLength] = 0;
			src->name = src->buffer + segmentLength + 1;
			src->srcLength = segmentLength;
			src->origin = origin;
			memcpy(src->name, name, strlen(name) + 1);
			return src;
		}

		ISourceCode* ProxySourceBuffer(cstr bufferRef, int segmentLength, const Vec2i& origin, cstr nameRef) override
		{
			if (bufferRef == nullptr || nameRef == nullptr)
			{
				Rococo::Throw(0, "ISParserTree::ProxySourceBuffer (%s) Bad argument", nameRef);
			}

			if (segmentLength < 0)
			{
				size_t len = strlen(bufferRef);
				if (len > 0x7FFFFELL)
				{
					Rococo::Throw(0, "ISParserTree::ProxySourceBuffer - source code %s must be shorter than 2 gigabytes.", nameRef);
				}

				segmentLength = (int)len;
			}

			if (bufferRef[segmentLength] != 0)
			{
				Throw(0, "%s: buffer must be null at position %d", nameRef, segmentLength);
			}

			size_t blockSize = sizeof(SourceCode) + strlen(nameRef) + 1;
			char* block = (char*)allocator.Allocate(blockSize);
			auto* src = new (block) SourceCode(allocator);
			src->allocator = allocator;
			src->buffer = const_cast<char*>(bufferRef);
			src->name = block + sizeof(SourceCode);
			memcpy_s(src->name, strlen(nameRef) + 1, nameRef, strlen(nameRef) + 1);
			src->srcLength = segmentLength;
			src->origin = origin;
			src->block = block;
			return src;
		}

		ISourceCode* LoadSource(const wchar_t* u16filename, const Vec2i& origin) override
		{
			if (u16filename == nullptr || *u16filename == 0) Rococo::Throw(0, "ISParser::LoadSource: Blank filename");

			U8FilePath filename;
			Assign(filename, u16filename);

			try
			{
				if (!Rococo::OS::IsFileExistant(u16filename))
				{
					Rococo::Throw(0, "%s: file does not exist", __FUNCTION__);
				}

				enum { MAX_FILELEN = 0x7FFFFFFELL }; // +1 byte for the nul char gives int32.max, which is our hard limit
				
#ifdef _WIN32
				FILE* fp = _wfopen(u16filename, L"rb");
#else
				U8FilePath u8filename;
				Format(u8filename, "%S", u16filename);
				
				FILE* fp = fopen(u8filename, "rb");
#endif
				if (fp == nullptr)
				{
					int err = errno;
					Rococo::Throw(0, "Could not open file %ls - %s", u16filename, strerror(err));
				}

				struct FPANON
				{
					FILE* fp;

					~FPANON()
					{
						fclose(fp);
					}
				} fpCloser{ fp };

				fseek(fp, 0, SEEK_END);

				long filelen = ftell(fp);

				fseek(fp, 0, SEEK_SET);

				size_t blockSize = sizeof(SourceCode) + filelen + 1 + strlen(filename) + 1;
				char* block = (char*)allocator.Allocate(blockSize);
				auto* src = new (block) SourceCode(allocator);
				src->allocator = allocator;
				src->buffer = block + sizeof(SourceCode);

				size_t lenRead = fread(src->buffer, 1, filelen, fp);
				if (lenRead != filelen)
				{
					int err = errno;
					allocator.FreeData(block);
					Rococo::Throw(0, "Could not read all file data - %s", strerror(err));
				}

				src->buffer[filelen] = 0;
				src->name = src->buffer + filelen + 1;
				src->srcLength = (int32)filelen;
				src->origin = origin;
				src->block = block;

				memcpy(src->name, filename, strlen(filename) + 1);

				return src;
			}
			catch (IException& ex)
			{
				Rococo::Throw(ex.ErrorCode(), "ISParser::LoadSource( %s ,...) failed - %s", filename.buf, ex.Message());
				return nullptr;
			}
		}

		ISourceCode* LoadSource(const wchar_t* u16filename, const Vec2i& origin, const char* buffer, long len) override
		{
			U8FilePath filename;
			Assign(filename, u16filename);
			return DuplicateSourceBuffer(buffer, len, origin, filename);
		}

		refcount_t AddRef() override
		{
			return ++refcount;
		}

		refcount_t Release() override
		{
			refcount--;
			if (refcount == 0)
			{
				auto& allocator = this->allocator;
				this->~SParser_2_0();	
				allocator.FreeData(this);
				return 0;
			}
			else
			{
				return refcount;
			}
		}
	};
} // Anon

namespace Rococo
{
	namespace Sex
	{
		ISParser* CreateSParser_2_0(IAllocator& allocator, size_t maxStringLength)
		{
			enum { ABS_MAX_STRING_LENGTH = 0x7FFFFFFFLL };
			if (maxStringLength < 8 || maxStringLength > ABS_MAX_STRING_LENGTH)
			{
				Rococo::Throw(0, "CreateSParser: %llu must not be less than 8 nor more than %llu. Recommended size is 32768", maxStringLength, ABS_MAX_STRING_LENGTH);
			}

			auto* buffer = allocator.Allocate(sizeof Anon::SParser_2_0);
			return new (buffer) Anon::SParser_2_0(allocator, maxStringLength);
		}

		void TestBlockAllocator(cstr sExpression)
		{
			Anon::SCostEvaluator ce(32768, 32768);
			{
				Anon::SParser parser(sExpression, ce);
				parser.name = "test";
				parser.Parse(nullptr);

				printf("total string cost: %llu\n", (uint64) ce.totalStringCost);
				printf("total atomic cost: %llu\n", (uint64) ce.totalAtomicCost);
				printf("total comment cost: %llu\n", (uint64) ce.totalCommentCost);
				printf("largest string size: %llu\n", (uint64) ce.largestStringSize);
				printf("larget atomic size: %llu\n", (uint64) ce.largestAtomicSize);
				printf("larget comment size: %llu\n", (uint64) ce.totalCommentCost);
				printf("atomic count: %llu\n", (uint64) ce.atomicCount);
				printf("literal count: %llu\n", (uint64) ce.stringCount);
				printf("compound count: %llu\n", (uint64) ce.branchCount);
			}

			AutoFree<IAllocatorSupervisor> allocator(Rococo::Memory::CreateBlockAllocator(16, 0));

			Anon::SBlockAllocator sba(ce, *allocator, sExpression);
			Anon::SParser parser(sExpression, sba);
			parser.name = "test";
			parser.Parse(nullptr);

			ptrdiff_t dp = sba.writePos - (char*)sba.compoundArray;
			if (dp != 0)
			{
				Rococo::Throw(0, "SBlockAllocator: Difference between string write position and start of compound element array was non-zero");
			}
		}
	} // Sex
} // Rococo

SEXY_SPARSER_API Rococo::Sex::ISParser* Sexy_CreateSexParser_2_0(Rococo::IAllocator& allocator, size_t maxStringLength)
{
	return Rococo::Sex::CreateSParser_2_0(allocator, maxStringLength);
}

