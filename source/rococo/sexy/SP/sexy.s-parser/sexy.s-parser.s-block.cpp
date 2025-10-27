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

#include "sexy.s-parser.stdafx.h"

#define _CRT_SECURE_NO_WARNINGS

#include <rococo.api.h>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <rococo.io.h>

#include <memory>

#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#include <errno.h>
#endif

#include <new>
#include <rococo.sexy.allocators.h>
#include <sexy.unordered_map.h>
#include <rococo.functional.h>

using namespace Rococo::Strings;

namespace Rococo::Sex
{
	IExpressionTransform* CreateExpressionTransform(cr_sex s);
}

namespace Rococo
{
	ROCOCO_API void ThrowIllFormedSExpression(int32 displacement, cstr format, ...);
}

namespace Anon
{
	using namespace Rococo;
	using namespace Rococo::Sex;

	ROCOCO_INTERFACE ITokenBuilder
	{
		virtual void AddChar(char c) = 0;
	};

	ROCOCO_INTERFACE ISExpressionLinkBuilder : ISExpression
	{
		virtual void AddSibling(ISExpressionLinkBuilder* sibling) = 0;
		virtual ISExpressionLinkBuilder* GetNextSibling() = 0;
	};

	ROCOCO_INTERFACE ICompoundSExpression : ISExpressionLinkBuilder
	{
		virtual void AddChild(ISExpressionLinkBuilder* child) = 0;
		virtual ISExpressionLinkBuilder* GetFirstChild() = 0;
		virtual void SetArrayStart(ISExpression** pArray) = 0;
		virtual void SetOffsets(int32 startOffset, int32 endOffset) = 0;
		virtual void TransformChild(IExpressionTransform& transform, cr_sex sCompound) const = 0;
	};

	ROCOCO_INTERFACE IExpressionBuilder
	{
		virtual void AddAtomic(cstr begin, cstr end, ICompoundSExpression* parent) = 0;
		virtual void AddComment(cstr begin, cstr end) = 0;
		virtual ICompoundSExpression* AddCompound(cstr beginPoint, int depth, ICompoundSExpression* parent) = 0;
		virtual void FinishCompound(cstr beginPoint, cstr endPoint, int depth, ICompoundSExpression* compound) = 0;
		virtual cstr ParseString(cstr sStart, cstr sEnd, ICompoundSExpression* parent) = 0;
	};

	bool IsBlank(char c)
	{
		return c <= 32;
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
				p++;
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

		void AddTabs(int nTabs)
		{
			for (int i = 0; i < nTabs; ++i)
			{
				printf("\t");
			}
		}

		void AddAtomic(cstr begin, cstr end, ICompoundSExpression* parent) override
		{
			UNUSED(parent);
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
			UNUSED(begin);
			UNUSED(end);
		}

		ICompoundSExpression* AddCompound(cstr beginPoint, int _depth, ICompoundSExpression* parent) override
		{
			UNUSED(beginPoint);
			UNUSED(parent);
			this->depth = _depth;
			return nullptr;
		}

		void FinishCompound(cstr beginPoint, cstr endPoint, int _depth, ICompoundSExpression* compound) override
		{
			UNUSED(beginPoint);
			UNUSED(endPoint);
			UNUSED(_depth);
			UNUSED(compound);
		}

		cstr ParseString(cstr sStart, cstr sEnd, ICompoundSExpression* parent) override
		{
			UNUSED(parent);
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
					ThrowIllFormedSExpression((int)(end - next), "%s: Parser::Parse failed: too many close parenthesis characters", name);
				}

				builder.FinishCompound(sExpression, end, 0, root);
			}
			catch (IException& ex)
			{
				char specimen[64];
				SafeFormat(specimen, sizeof(specimen), "");
				int offset = ex.ErrorCode();
				Vec2i pos = OffsetToPos(0, sExpression, next + offset, { 1,1 });
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
					if (depth == 0)
					{
						ThrowIllFormedSExpression((int)(next - sExpression), "%s: Unexpected close parenthesis at the root expression", name);
					}
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
				ThrowIllFormedSExpression((int)(next - sExpression), "%s: Missing close parenthesis character before end of s-expression", name);
			}

			return next;
		}

		cstr ParseBlankspace(cstr start)
		{
			auto* s = start;
			while (s != end && IsBlank(*s))
			{
				++s;
			}

			return s;
		}

		cstr ParseLineComment(cstr start)
		{
			auto* s = start;
			while (s != end)
			{
				switch (*s)
				{
				case '\r':
				case '\n':
					builder.AddComment(start, s);
					return s + 1;
				default:
					break;
				}
				++s;
			}

			return s;
		}

		cstr ParseBlockComment(cstr start)
		{
			auto* s = start;
			while (s != end)
			{
				switch (*s)
				{
				case '*':
					if (s[1] == '/')
					{
						builder.AddComment(start, s);
						return s + 2;
					}
				default:
					break;
				}
				++s;
			}

			return s;
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
		ISExpression** arraySets = nullptr;
		RootExpression* root = nullptr;
		ISourceCode* sourceCode = nullptr;
		ISParser* sParser = nullptr;
		IAllocator& allocator;
		refcount_t refcount = 1;
		mutable IExpressionTransform* transform = nullptr;
		size_t aIndex = 0;

		std::unordered_map<const ISExpression*, std::vector<HString>>* mapExpressionPtrToCommentBlock = nullptr;
		mutable std::unordered_map<const ISExpression*, IExpressionTransform*>* transforms = nullptr;

		ExpressionTree(IAllocator& _allocator) : allocator(_allocator)
		{

		}

		virtual ~ExpressionTree()
		{
			delete transforms;
			delete mapExpressionPtrToCommentBlock;
			sParser->Release();
			if (sourceCode) sourceCode->Release();
			if (block)
			{
				allocator.FreeData(block);
			}
		}

		void MakeTransforms() const
		{
		}

		IExpressionTransform* Transform(cr_sex s) const
		{
			if (!transforms)
			{
				transforms = new std::unordered_map<const ISExpression*, IExpressionTransform*>();
			}

			auto i = transforms->find(&s);
			if (i != transforms->end())
			{
				Throw(s, "Transform for expression already exists");
			}

			IExpressionTransform* newTransform = CreateExpressionTransform(s);

			auto binding = std::make_pair(&s, newTransform);
			i = transforms->insert(binding).first;
			return i->second;
		}

		void ReleaseTransform(cr_sex s) const
		{
			if (transforms)
			{
				auto i = transforms->find(&s);
				if (i != transforms->end())
				{
					auto* t = i->second;
					t->Free();
					transforms->erase(i);
				}
			}
		}

		Vec2i OffsetToPos(int32 offset)
		{
			auto* end = sourceCode->SourceStart() + sourceCode->SourceLength();
			auto* finalPos = min(end, sourceCode->SourceStart() + offset);

			Vec2i origin = sourceCode->Origin();

			return Anon::OffsetToPos(offset, sourceCode->SourceStart(), finalPos, sourceCode->Origin());
		}

		void ConvertListsToArrays(ICompoundSExpression& cs);

		ISExpression& Root() override
		{
			return GetISExpression(*root);
		}

		const ISExpression& Root() const override
		{
			return transform ? transform->Root() : GetISExpression(*root);
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

		size_t EnumerateComments(cr_sex s, Rococo::Function<void(cstr)> onBlockItem) const override
		{
			if (mapExpressionPtrToCommentBlock == nullptr)
			{
				return 0;
			}

			auto i = mapExpressionPtrToCommentBlock->find(&s);

			if (i == mapExpressionPtrToCommentBlock->end())
			{
				return 0;
			}

			for (auto& element : i->second)
			{
				onBlockItem(element);
			}

			return i->second.size();
		}

		void TransformRoot(IExpressionTransform& _transform, const ICompoundSExpression& _root) const
		{
			UNUSED(_root);
			this->transform = &_transform;
		}
	};

#pragma pack(push,1)
	struct CodeOffsets
	{
		int32 startOffset;
		int32 endOffset;
	};
#pragma pack(pop)

	IExpressionTransform& TransformExpression(ICompoundSExpression* parent, cr_sex s, const ExpressionTree& tree)
	{
		if (!parent)
		{
			Throw(s, "not supported - Compound Expression had no parent");
		}

		auto* transform = tree.Transform(s);

		try
		{
			parent->TransformChild(*transform, s);
		}
		catch (...)
		{
			tree.ReleaseTransform(s);
			throw;
		}

		return *transform;
	}

	struct AtomicExpression : ISExpressionLinkBuilder
	{
		ICompoundSExpression* parent = nullptr;
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
			UNUSED(s);
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

		cstr c_str() const override
		{
			return String()->Buffer;
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
			UNUSED(index);
			Throw(0, "Atomic node has no elements");
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

		IExpressionTransform& TransformThis() const override
		{
			if (!parent)
			{
				Throw(*this, "not supported - Compound Expression had no parent");
			}

			auto& tree = static_cast<const ExpressionTree&>(Tree());

			auto* transform = tree.Transform(*this);

			try
			{
				parent->TransformChild(*transform, *this);
			}
			catch (...)
			{
				tree.ReleaseTransform(*this);
				throw;
			}

			return *transform;
		}
	};

	struct LiteralExpression : ISExpressionLinkBuilder
	{
		ICompoundSExpression* parent = nullptr;
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
			UNUSED(s);
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

		cstr c_str() const override
		{
			return String()->Buffer;
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
			UNUSED(index);
			Throw(0, "String literal has no elements");
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

		IExpressionTransform& TransformThis() const override
		{
			return TransformExpression(parent, *this, static_cast<const ExpressionTree&>(Tree()));
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
			UNUSED(startOffset);
			UNUSED(endOffset);
		}

		void SetArrayStart(ISExpression** pArray) override
		{
			children.pArray = pArray;
		}

		void AddSibling(ISExpressionLinkBuilder* sibling) override
		{
			UNUSED(sibling);
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

		cstr c_str() const override
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
			if (index < 0 || index >= numberOfChildren) Rococo::Sex::Throw(*this, "CompoundExpression.GetElement(index): index %d of %d out of range", index, numberOfChildren);
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

		void TransformChild(IExpressionTransform& transform, cr_sex sCompound) const override
		{
			int index = GetIndexOf(sCompound);
			if (index < 0)	Throw(*this, "sCompound was not a child of the parent");

			children.pArray[index] = &transform.Root();
		}

		IExpressionTransform& TransformThis() const override
		{
			if (!tree)
			{
				Throw(*this, "not supported - Root Expression had no tree");
			}

			auto& t = static_cast<const ExpressionTree&>(Tree());

			auto* transform = t.Transform(*this);

			try
			{
				t.TransformRoot(*transform, *this);
			}
			catch (...)
			{
				t.ReleaseTransform(*this);
				throw;
			}

			return *transform;
		}
	};

	ISExpression& GetISExpression(RootExpression& root) { return root; }
	cr_sex GetISExpression(const RootExpression& root) { return root; }

	struct ChildTransformArray
	{

	};

	struct CompoundExpression : ICompoundSExpression
	{
		ICompoundSExpression* parent = nullptr;
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
		}

		cstr c_str() const override
		{
			return String()->Buffer;
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
			UNUSED(token);
			return false;
		}

		IExpressionTransform& TransformThis() const override
		{
			return TransformExpression(parent, *this, static_cast<const ExpressionTree&>(Tree()));
		}

		void TransformChild(IExpressionTransform& transform, cr_sex sCompound) const override
		{
			int index = GetIndexOf(sCompound);
			if (index < 0)	Throw(*this, "sCompound was not a child of the parent");

			children.pArray[index] = &transform.Root();
		}
	};

	void ExpressionTree::ConvertListsToArrays(ICompoundSExpression& cs)
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
				ConvertListsToArrays(c);
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
			UNUSED(parent);
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
			UNUSED(beginPoint);
			UNUSED(parent);
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
			UNUSED(beginPoint);
			UNUSED(endPoint);
			UNUSED(depth);
			UNUSED(compound);
			UNUSED(beginPoint);
		}

		cstr ParseString(cstr sStart, cstr sEnd, ICompoundSExpression* parent) override
		{
			UNUSED(parent);
			struct ANON : ITokenBuilder
			{
				size_t writeCount = 0;

				ANON()
				{
				}

				void AddChar(char c) override
				{
					UNUSED(c);
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

#ifdef _DEBUG
		enum { NO_MANS_LAND = 128 };
#else	
		enum { NO_MANS_LAND = 0 };
#endif

		const SCostEvaluator& ce;
		IAllocator& allocator;

		std::vector<char> commentBuffer;

		SBlockAllocator(const SCostEvaluator& _ce, IAllocator& _allocator, cstr _sourceStart, bool addCommentMap) :
			sourceStart(_sourceStart), ce(_ce), allocator(_allocator)
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

			if (addCommentMap)
			{
				// !mapExpressionPtrToComment.empty() => store comments. This is used by code generators to pass comments onto generated code
				tree->mapExpressionPtrToCommentBlock = new std::unordered_map<const ISExpression*, std::vector<HString>>();
			}
		}

		ExpressionTree* Tree()
		{
			return (ExpressionTree*) (block + totalSize - sizeof(ExpressionTree) - sizeof(RootExpression));
		}

		ICompoundSExpression* Root()
		{
			auto* root = block + totalSize - sizeof(RootExpression);
			auto* r = (RootExpression*)root;
			return r;
		}

		void ValidateMemory()
		{
#ifdef _DEBUG
			cstr nomansLand = block + totalSize;
			cstr endOfNoMandsLand = block + totalSize + NO_MANS_LAND;

			for (cstr c = nomansLand; c < endOfNoMandsLand; ++c)
			{
				if ((uint8) *c != (unsigned char)0xFE)
				{
					Throw(0, "SBlockAllocator overwrite detected");
				}
			}
#endif
		}

		ExpressionTree* Detach()
		{
			auto* treeMemory = Tree();
			auto* tree = (ExpressionTree*)treeMemory;
			tree->block = block;
			block = nullptr;
			return tree;
		}

		virtual ~SBlockAllocator()
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

			auto* tree = Tree();

			if (tree->mapExpressionPtrToCommentBlock)
			{
				commentBuffer.resize(nBytes);

				Substring s{ begin, end };
				s.CopyWithTruncate(commentBuffer.data(), nBytes);

				ISExpression* ptr = nextFreeCompoundSlot;
				auto i = tree->mapExpressionPtrToCommentBlock->insert(std::make_pair(ptr, std::vector<HString>()));
				i.first->second.push_back(HString(commentBuffer.data()));
			}
		}

		ICompoundSExpression* AddCompound(cstr beginPoint, int depth, ICompoundSExpression* parent) override
		{
			UNUSED(depth);
			UNUSED(beginPoint);
			auto* n = nextFreeCompoundSlot;
			new (n) CompoundExpression();
			n->parent = parent;
			parent->AddChild(n);
			nextFreeCompoundSlot++;
			return n;
		}

		void FinishCompound(cstr beginPoint, cstr endPoint, int depth, ICompoundSExpression* parent) override
		{
			UNUSED(depth);
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
			literal->offsets.endOffset = literal->offsets.startOffset + max(((int32)nBytes) - 1, 0);
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
		IPackage* package;

		SourceCode(IAllocator& _allocator, IPackage* _package) : allocator(_allocator), package(_package) {}

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
		const IPackage* Package() const { return package; }
	};

	struct SParser_2_0 : public ISParser
	{
		refcount_t refcount = 1;
		IAllocator& allocator;
		size_t maxStringLength;
		bool mapComments = false;

		SParser_2_0(IAllocator& _allocator, size_t _maxStringLength) :
			allocator(_allocator),
			maxStringLength(_maxStringLength)
		{
		}

		virtual ~SParser_2_0()
		{
		}

		ISParserTree* CreateTree(ISourceCode& sourceCode) override
		{
			SCostEvaluator ce(maxStringLength, maxStringLength);

			SParser parser(sourceCode.SourceStart(), sourceCode.SourceLength(), ce);
			parser.name = sourceCode.Name();
			parser.Parse(nullptr);

			SBlockAllocator sba(ce, allocator, sourceCode.SourceStart(), mapComments);
			{
				SParser parser2(sourceCode.SourceStart(), sba);
				parser2.name = sourceCode.Name();
				parser2.Parse(sba.Root());
			}

			auto* tree = sba.Detach();
			tree->sParser = this;
			tree->arraySets = sba.arraySets;
			AddRef();
			tree->sourceCode = &sourceCode;
			sourceCode.AddRef();
			tree->ConvertListsToArrays(*tree->root);
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
			auto* src = new (block) SourceCode(allocator, nullptr);
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

		void MapComments() override
		{
			mapComments = true;
		}

		ISourceCode* ProxySourceBuffer(cstr bufferRef, int segmentLength, const Vec2i& origin, cstr nameRef, IPackage* package) override
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
			auto* src = new (block) SourceCode(allocator, package);
			src->allocator = allocator;
			src->buffer = const_cast<char*>(bufferRef);
			src->name = block + sizeof(SourceCode);
			memcpy_s(src->name, strlen(nameRef) + 1, nameRef, strlen(nameRef) + 1);
			src->srcLength = segmentLength;
			src->origin = origin;
			src->block = block;
			return src;
		}

		ISourceCode* LoadSource(crwstr u16filename, const Vec2i& origin) override
		{
			if (u16filename == nullptr || *u16filename == 0) Rococo::Throw(0, "ISParser::LoadSource: Blank filename");

			U8FilePath filename;
			Assign(filename, u16filename);

			try
			{
				struct Loader : IO::IBinaryFileLoader
				{
					IAllocator& allocator;
					cstr filename;
					char* block = nullptr;
					char* fileBuffer = nullptr;
					size_t fileLength = 0;

					uint8* LockWriter(size_t length) override
					{
						fileLength = length;
						size_t blocksize = sizeof(SourceCode) + length + 1 + strlen(filename) + 1;
						block = (char*)allocator.Allocate(blocksize);
						fileBuffer = block + sizeof(SourceCode);
						fileBuffer[length] = 0;
						return (uint8*)fileBuffer;
					}

					void Unlock() override
					{

					}

					Loader(IAllocator& _allocator, cstr _filename) : allocator(_allocator), filename(_filename)
					{

					}
				} loader(allocator, filename);

				IO::LoadBinaryFile(loader, u16filename, 512_megabytes);

				char* block = loader.block;

				auto* src = new (block) SourceCode(allocator, nullptr);

				src->allocator = allocator;
				src->buffer = loader.fileBuffer;
				src->name = src->buffer + loader.fileLength + 1;
				src->srcLength = (int32)loader.fileLength;
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

		ISourceCode* LoadSource(crwstr u16filename, const Vec2i& origin, const char* buffer, long len) override
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
				auto& refAllocator = this->allocator;
				this->~SParser_2_0();	
				refAllocator.FreeData(this);
				return 0;
			}
			else
			{
				return refcount;
			}
		}
	};
} // Anon

namespace Rococo::Sex
{
	SEXY_SPARSER_API ISParser* CreateSexParser_2_0(IAllocator& allocator, size_t maxStringLength)
	{
		enum { ABS_MAX_STRING_LENGTH = 0x7FFFFFFFLL };
		if (maxStringLength < 8 || maxStringLength > ABS_MAX_STRING_LENGTH)
		{
			Rococo::Throw(0, "CreateSParser: %llu must not be less than 8 nor more than %llu. Recommended size is 32768", maxStringLength, ABS_MAX_STRING_LENGTH);
		}

		auto* buffer = allocator.Allocate(sizeof(Anon::SParser_2_0));
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

		AutoFree<IAllocatorSupervisor> allocator(Rococo::Memory::CreateBlockAllocator(16, 0, "s-block-parser"));

		Anon::SBlockAllocator sba(ce, *allocator, sExpression, false);
		Anon::SParser parser(sExpression, sba);
		parser.name = "test";
		parser.Parse(nullptr);

		ptrdiff_t dp = sba.writePos - (char*)sba.compoundArray;
		if (dp != 0)
		{
			Rococo::Throw(0, "SBlockAllocator: Difference between string write position and start of compound element array was non-zero");
		}
	}
} 


