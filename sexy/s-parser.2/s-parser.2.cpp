#include <rococo.api.h>
#include <rococo.strings.h>

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <memory>
#include <malloc.h>

#include <rococo.libs.inl>

#include <sexy.types.h>

#include <new>

#include <filesystem>

#include <sexy.s-parser.h>

// Experimental s-parser. Motivation - be able to iterate through an s source file using functions without allocating heap mempry

namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;

	ROCOCOAPI ITokenBuilder
	{
		virtual void AddChar(char c) = 0;
	};

	ROCOCOAPI ISExpressionLinkBuilder: ISExpression
	{
		virtual void AddLink(ISExpressionLinkBuilder* child) = 0;
		virtual ISExpressionLinkBuilder* GetLinkTail() = 0;
		virtual ISExpressionLinkBuilder* GetNextLink() = 0;
		virtual ISExpressionLinkBuilder* GetLinkHead() = 0;
	};

	ROCOCOAPI ICompoundSExpression : ISExpressionLinkBuilder
	{
		virtual void AddChild(ISExpressionLinkBuilder* child) = 0;
		virtual void SetArrayStart(ISExpression** pArray) = 0;
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

	struct SParser
	{
		cstr sExpression;
		cstr end;
		size_t len;

		cstr p;

		IExpressionBuilder& builder;

		SParser(cstr _sExpression, IExpressionBuilder& _builder) : sExpression(_sExpression), builder(_builder)
		{
			if (_sExpression == nullptr) Throw(0, "Parser::Parse - null expression forbidden");
			len = rlen(sExpression);
			end = sExpression + len;
		}

		void Parse(ICompoundSExpression* root)
		{
			cstr next = ParseCompound(sExpression, 0, root);

			if (next != end)
			{
				Throw(0, "Parser::Parse failed: too many close parenthesis characters");
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
			return builder.ParseString(sString, end, parent);
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
						auto* child = builder.AddCompound(sCompound, depth, parent);
						next = ParseCompound(next, depth + 1, child);
					}
					break;
				case ')':
					builder.FinishCompound(sCompound, next, depth, parent);
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
				Throw(0, "Missing close parenthesis character before end of s-expression");
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
		std::shared_ptr<IAllocatorSupervisor> allocator;
		refcount_t refcount = 1;

		size_t aIndex = 0;

		ExpressionTree(std::shared_ptr<IAllocatorSupervisor> _allocator): allocator(_allocator)
		{

		}

		~ExpressionTree()
		{
			sParser->Release();

			if (block)
			{
				allocator->FreeData(block);
			}
		}

		void CovertListsToArrays(ICompoundSExpression& cs)
		{
			auto* a = arraySets + aIndex;
			auto* startOfArray = a;
			for (auto i = cs.GetLinkHead(); i != nullptr; i = i->GetNextLink())
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
					CovertListsToArrays((ICompoundSExpression&)child);
				}
			}
		}

		ISExpression& Root()
		{
			return GetISExpression(*root);
		}

		const ISExpression& Root() const
		{
			return GetISExpression(*root);
		}

		ISParser& Parser()
		{
			return *sParser;
		}

		const ISourceCode& Source() const
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

	union PosOrNext
	{
		Vec2i pos;
		ISExpressionLinkBuilder* next;
	};

	struct AtomicExpression: ISExpressionLinkBuilder
	{
		ISExpression* parent = nullptr;
		PosOrNext startPos;
		Vec2i endPos;
		sexstring_header header;

		void AddLink(ISExpressionLinkBuilder* child) override
		{
			if (startPos.next == nullptr)
			{
				startPos.next = child;
			}
			else
			{
				auto i = GetLinkTail();
				i->AddLink(child);
			}
		}

		ISExpressionLinkBuilder* GetLinkHead() override
		{
			return startPos.next;
		}

		ISExpressionLinkBuilder* GetLinkTail() override
		{
			if (startPos.next == nullptr) return this;
			else
			{
				return startPos.next->GetLinkTail();
			}
		}

		ISExpressionLinkBuilder* GetNextLink() override
		{
			return startPos.next;
		}

		const Vec2i& Start() const override
		{
			return startPos.pos;
		}

		const Vec2i& End() const override
		{
			return endPos;
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

		ISExpressionBuilder* CreateTransform() override
		{
			Throw(0, "Cannot transform atomic nodes");
			return nullptr;
		}

		const ISExpression* GetTransform() const override
		{
			return nullptr;
		}

		const ISExpression* GetOriginal() const override
		{
			return this;
		}

		const int TransformDepth() const override
		{
			return 0;
		}

		bool operator == (const char* token) const override
		{
			return Eq(token, header.Buffer);
		}
	};

	struct LiteralExpression : ISExpressionLinkBuilder
	{
		ISExpression* parent = nullptr;
		PosOrNext startPos;
		Vec2i endPos;
		sexstring_header header;

		void AddLink(ISExpressionLinkBuilder* child) override
		{
			if (startPos.next == nullptr)
			{
				startPos.next = child;
			}
			else
			{
				auto i = GetLinkTail();
				i->AddLink(child);
			}
		}

		ISExpressionLinkBuilder* GetLinkHead() override
		{
			return startPos.next;
		}

		ISExpressionLinkBuilder* GetLinkTail() override
		{
			if (startPos.next == nullptr) return this;
			else
			{
				return startPos.next->GetLinkTail();
			}
		}

		ISExpressionLinkBuilder* GetNextLink() override
		{
			return startPos.next;
		}

		const Vec2i& Start() const override
		{
			return startPos.pos;
		}

		const Vec2i& End() const override
		{
			return endPos;
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

		ISExpressionBuilder* CreateTransform() override
		{
			Throw(0, "Cannot transform string literal nodes");
			return nullptr;
		}

		const ISExpression* GetTransform() const override
		{
			return nullptr;
		}

		const ISExpression* GetOriginal() const override
		{
			return this;
		}

		const int TransformDepth() const override
		{
			return 0;
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
		ISExpressionLinkBuilder* firstLink;
		ISExpression** pArray;

		LinkOrArray() : firstLink(nullptr) {}
	};

	struct RootExpression : ICompoundSExpression
	{
		ExpressionTree* tree = nullptr;
		Vec2i startPos;
		Vec2i endPos;
		LinkOrArray children;
		int32 numberOfChildren = 0;

		void SetArrayStart(ISExpression** pArray) override
		{
			children.pArray = pArray;
		}

		void AddLink(ISExpressionLinkBuilder* child) override
		{
			numberOfChildren++;
			if (children.firstLink == nullptr)
			{
				children.firstLink = child;
			}
			else
			{
				auto i = GetLinkTail();
				i->AddLink(child);
			}
		}

		ISExpressionLinkBuilder* GetLinkHead() override
		{
			return children.firstLink;
		}

		ISExpressionLinkBuilder* GetLinkTail() override
		{
			if (children.firstLink == nullptr) return this;
			else
			{
				return children.firstLink->GetLinkTail();
			}
		}

		ISExpressionLinkBuilder* GetNextLink() override
		{
			return children.firstLink->GetLinkHead();
		}

		const Vec2i& Start() const override
		{
			return startPos;
		}

		const Vec2i& End() const override
		{
			return endPos;
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
			if (index < 0 || index >= numberOfChildren) Throw(0, "CompoundExpression.GetElement(index): index %d of %d out of range", index, numberOfChildren);
#endif
			return *children.pArray[index];
		}

		const ISExpression* Parent() const override
		{
			return nullptr;
		}

		ISExpressionBuilder* CreateTransform() override
		{
			Throw(0, "Cannot transform atomic nodes");
			return nullptr;
		}

		const ISExpression* GetTransform() const override
		{
			return nullptr;
		}

		const ISExpression* GetOriginal() const override
		{
			return this;
		}

		const int TransformDepth() const override
		{
			return 0;
		}

		bool operator == (const char* token) const override
		{
			return token == nullptr;
		}

		// N.B the child's parent pointer has been set to this instance already
		void AddChild(ISExpressionLinkBuilder* child) override
		{
			AddLink(child);
		}
	};

	ISExpression& GetISExpression(RootExpression& root) { return root; }
	cr_sex GetISExpression(const RootExpression& root) { return root; }

	struct CompoundExpression : ICompoundSExpression
	{
		ISExpression* parent = nullptr;
		Vec2i startPos;
		Vec2i endPos = { 0,0 };
		LinkOrArray children;
		int32 numberOfChildren;

		void SetArrayStart(ISExpression** pArray)
		{
			children.pArray = pArray;
		}

		void AddLink(ISExpressionLinkBuilder* child) override
		{
			numberOfChildren++;

			if (children.firstLink == nullptr)
			{
				children.firstLink = child;
			}
			else
			{
				auto i = GetLinkTail();
				i->AddLink(child);
			}
		}

		ISExpressionLinkBuilder* GetLinkHead() override
		{
			return children.firstLink;
		}

		ISExpressionLinkBuilder* GetNextLink() override
		{
			return children.firstLink->GetLinkHead();
		}

		ISExpressionLinkBuilder* GetLinkTail() override
		{
			if (children.firstLink == nullptr) return this;
			else
			{
				return children.firstLink->GetLinkTail();
			}
		}

		const Vec2i& Start() const override
		{
			return startPos;
		}

		const Vec2i& End() const override
		{
			return endPos;
		}

		EXPRESSION_TYPE Type() const override
		{
			return EXPRESSION_TYPE_COMPOUND;
		}

		const sexstring String() const override
		{
			Throw(0, "Compound node has no string value");
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
			if (index < 0 || index >= numberOfChildren) Throw(0, "CompoundExpression.GetElement(index): index %d of %d out of range", index, numberOfChildren);
#endif
			return *children.pArray[index];
		}

		const ISExpression* Parent() const override
		{
			return parent;
		}

		ISExpressionBuilder* CreateTransform() override
		{
			Throw(0, "Cannot transform atomic nodes");
			return nullptr;
		}

		const ISExpression* GetTransform() const override
		{
			return nullptr;
		}

		const ISExpression* GetOriginal() const override
		{
			return this;
		}

		const int TransformDepth() const override
		{
			return 0;
		}

		bool operator == (const char* token) const override
		{
			return false;
		}

		// N.B the child's parent pointer has been set to this instance already
		void AddChild(ISExpressionLinkBuilder* child) override
		{
			AddLink(child);
		}
	};

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
		size_t maxAtomicSize; // Max bytes in a literal string before an exception is thrown
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
	struct SBlockAllocator: IExpressionBuilder
	{
		char* block;
		char* writePos;

		CompoundExpression* nextFreeCompoundSlot;

		size_t totalSize;
		CompoundExpression* compoundArray;
		ISExpression** arraySets;

		enum { NO_MANS_LAND = 128 };

		const SCostEvaluator& ce;
		std::shared_ptr<IAllocatorSupervisor> allocator;

		SBlockAllocator(const SCostEvaluator& _ce, std::shared_ptr<IAllocatorSupervisor> _allocator): ce(_ce), allocator(_allocator)
		{	
			size_t sizeofAtomicExpression = sizeof(AtomicExpression);
			totalSize = ce.atomicCount * sizeofAtomicExpression;
			totalSize += ce.totalAtomicCost;

			totalSize += ce.stringCount * sizeof(LiteralExpression);
			totalSize += ce.totalStringCost;

			size_t toCompounds = totalSize;

			totalSize += ce.branchCount * sizeof(CompoundExpression);

			totalSize += (ce.branchCount + ce.atomicCount + ce.stringCount) * sizeof(ISExpression*);

			totalSize += sizeof(ExpressionTree) + sizeof(RootExpression);

			block = (char*) allocator->Allocate(totalSize + NO_MANS_LAND);

#ifdef _DEBUG
			memset(block, 0xFE, NO_MANS_LAND + totalSize);
#endif

			writePos = block;

			compoundArray = (CompoundExpression*) (block + toCompounds);
			nextFreeCompoundSlot = compoundArray;

			arraySets = (ISExpression**) ( block + toCompounds + ce.branchCount * sizeof(CompoundExpression) );

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
				if (*c != (char) 0xFE)
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
				allocator->FreeData(block);
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
			if (nextWritePos >  (char*) compoundArray)
			{
				Throw(0, "Atomic token would overwrite block allocator inner bounds");
			}
#endif

			auto* atomic = new (writePos) AtomicExpression();
			atomic->parent = parent;
			parent->AddChild(atomic);

			char* buffer = atomic->header.Buffer;
			memcpy(buffer, begin, nBytes);
			buffer[nBytes] = 0;
			atomic->header.Length = (int32) nBytes;

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
		}

		cstr ParseString(cstr sStart, cstr sEnd, ICompoundSExpression* parent) override
		{
			char* buffer = (char*)alloca(ce.maxAtomicSize);

			struct ANON : ITokenBuilder
			{
				char* buffer;
				char* writePos;

				ANON(char* _buffer): buffer(_buffer), writePos(_buffer)
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
			if (nBytes >= ce.maxAtomicSize)
			{
				Throw(0, "Stack corruption SBlockAllocator::ParseString");
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
			auto* literal = new (writePos) LiteralExpression();
			literal->parent = parent;
			parent->AddChild(literal);

			char* persistentBuffer = literal->header.Buffer;
			memcpy(persistentBuffer, buffer, nBytes);
			literal->header.Length = (int32)nBytes;
			writePos += sizeofLiteral;

			return next;
		}
	};

	void Test(cstr sExpression)
	{
		SCostEvaluator ce(32768,32768);
		{
			SParser parser(sExpression, ce);
			parser.Parse(nullptr);

			printf("total string cost: %llu\n", ce.totalStringCost);
			printf("total atomic cost: %llu\n", ce.totalAtomicCost);
			printf("total comment cost: %llu\n", ce.totalCommentCost);
			printf("largest string size: %llu\n", ce.largestStringSize);
			printf("larget atomic size: %llu\n", ce.largestAtomicSize);
			printf("larget comment size: %llu\n", ce.totalCommentCost);
			printf("atomic count: %llu\n", ce.atomicCount);
			printf("literal count: %llu\n", ce.stringCount);
			printf("compound count: %llu\n", ce.branchCount);
		}

		std::shared_ptr<IAllocatorSupervisor> allocator (Rococo::Memory::CreateBlockAllocator(16, 1024), FreeAllocator);

		SBlockAllocator sba(ce, allocator);
		SParser parser(sExpression, sba);
		parser.Parse(nullptr);

		ptrdiff_t dp = sba.writePos - (char*) sba.compoundArray;
		if (dp != 0)
		{
			Throw(0, "SBlockAllocator: Difference between string write position and start of compound element array was non-zero");
		}
	}

	ISParser* CreateSParser(size_t maxStringLength)
	{
		enum { ABS_MAX_STRING_LENGTH = 0x7FFFFFFFLL };
		if (maxStringLength < 8 || maxStringLength > ABS_MAX_STRING_LENGTH)
		{
			Throw(0, "CreateSParser: %llu must not be less than 8 nor more than %llu. Recommended size is 32768", maxStringLength, ABS_MAX_STRING_LENGTH);
		}

		struct SourceCode : public ISourceCode
		{
			char* block;
			Vec2i origin;
			char* buffer;
			int srcLength;
			char* name;
			bool proxied;
			refcount_t refcount = 1;
			std::shared_ptr<IAllocator> allocator;

			const Vec2i& Origin() const override { return origin; }
			cstr SourceStart() const override { return buffer; }
			const int SourceLength() const override { return srcLength; }
			cstr Name() const override { return name; }
			refcount_t AddRef() { return ++refcount;  }
			refcount_t Release()
			{
				refcount--;
				if (refcount == 0)
				{
					allocator->FreeData(block);
					return 0;
				}
				else
				{
					return refcount;
				}
			}
		};

		struct ANON: public ISParser
		{
			refcount_t refcount = 1;
			std::shared_ptr <IAllocatorSupervisor> allocator;
			size_t maxStringLength;

			ANON(size_t _maxStringLength) :
				maxStringLength(_maxStringLength),
				allocator(Rococo::Memory::CreateBlockAllocator(16, 1024), FreeAllocator)
			{
			}

			~ANON()
			{
			}

			ISParserTree* CreateTree(ISourceCode& sourceCode) override
			{
				SCostEvaluator ce(maxStringLength, maxStringLength);
				{
					SParser parser(sourceCode.SourceStart(), ce);
					parser.Parse(nullptr);
				}

				SBlockAllocator sba(ce, allocator);
				SParser parser(sourceCode.SourceStart(), sba);
				parser.Parse(sba.Root());

				auto* tree = sba.Detach();
				tree->sParser = this;
				tree->arraySets = sba.arraySets;
				AddRef();
				tree->sourceCode = &sourceCode;
				tree->CovertListsToArrays(*tree->root);
				return tree;
			}

			ISourceCode* DuplicateSourceBuffer(cstr buffer, int segmentLength, const Vec2i& origin, cstr name) override
			{
				if (buffer == nullptr || segmentLength < 0 || name == nullptr)
				{
					Throw(0, "ISParserTree::DuplicateSourceBuffer Bad argument");
				}

				// We create the source file object, a buffer and a name buffer in one allocation call
				// This cuts down allocation and deallocation count from 6 to 2

				size_t blockSize = sizeof(SourceCode) + segmentLength + 1 + strlen(name) + 1;
				char* block = (char*) allocator->Allocate(blockSize);
				auto* src = new (block) SourceCode();
				src->allocator = allocator;
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
				size_t blockSize = sizeof(SourceCode);
				char* block = (char*)allocator->Allocate(blockSize);
				auto* src = new (block) SourceCode();
				src->allocator = allocator;
				src->buffer = const_cast<char*>(bufferRef);
				src->name = const_cast<char*>(nameRef);
				src->srcLength = segmentLength;
				src->origin = origin;
				return src;
			}

			ISourceCode* LoadSource(cstr filename, const Vec2i& origin) override
			{
				if (filename == nullptr || *filename == 0) Throw(0, "ISParser::LoadSource: Blank filename");

				using namespace std::experimental::filesystem;

				path srcPath(filename);

				try
				{
					if (!exists(srcPath))
					{
						Throw(0, "file does not exist");
					}
					auto filelen = file_size(srcPath);
					if (filelen == -1)
					{
						Throw(0, "Could not get file size");
					}

					enum { MAX_FILELEN = 0x7FFFFFFELL }; // +1 byte for the nul char gives int32.max, which is our hard limit

					if (filelen >= MAX_FILELEN)
					{
						Throw(0, "File length for source code greater than maximum file size: %llu bytes", MAX_FILELEN);
					}

					FILE* fp = fopen(filename, "rb");
					if (fp == nullptr)
					{
						int err = errno;
						Throw(0, "Could not open file - %s", strerror(err));
					}

					size_t blockSize = sizeof(SourceCode) + filelen + 1 + strlen(filename) + 1;
					char* block = (char*)allocator->Allocate(blockSize);
					auto* src = new (block) SourceCode();
					src->allocator = allocator;
					src->buffer = block + sizeof(SourceCode);

					size_t lenRead = fread(src->buffer, 1, filelen, fp);
					if (lenRead != filelen)
					{
						int err = errno;
						allocator->FreeData(block);
						Throw(0, "Could not read all file data - %s", strerror(err));
					}

					src->buffer[filelen] = 0;
					src->name = src->buffer + filelen + 1;
					src->srcLength = (int32) filelen;
					src->origin = origin;

					memcpy(src->name, filename, strlen(filename) + 1);

					return src;
				}
				catch (IException& ex)
				{
					Throw(ex.ErrorCode(), "ISParser::LoadSource( %s ,...) failed - %s", filename, ex.Message());
					return nullptr;
				}
			}

			ISourceCode* LoadSource(cstr moduleName, const Vec2i& origin, const char* buffer, long len) override
			{
				return DuplicateSourceBuffer(buffer, len, origin, moduleName);
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
					delete this;
					return 0;
				}
				else
				{
					return refcount;
				}
			}
		};

		return new ANON(maxStringLength);
	}
}

using namespace Rococo::Sex;

void validate(bool condition, cstr function, int line)
{
	if (!condition) Throw(0, "Condition false: %s %d", function, line);
}

#define VALIDATE(x) validate(x, __FUNCTION__, __LINE__);

int main()
{
	try
	{
	//	T1();
	//	T2();
	//	T3();
	//	Test("\"&x20abcdef\" a b cd (qcumber)");

		ISParser* sparser = CreateSParser(32768);

		auto t1 = "\"&x20abcdef\" a b cd (qcumber)"_fstring;
		ISourceCode* src = sparser->DuplicateSourceBuffer(t1.buffer, t1.length, Vec2i{ 1,1 }, "t1");
		VALIDATE(Eq(src->Name(), "t1"));
		VALIDATE(src->SourceLength() == t1.length);
		VALIDATE(src->SourceStart() != t1.buffer);

		auto* tree = sparser->CreateTree(*src);

		VALIDATE(tree->Root().NumberOfElements() == 5);
		VALIDATE(Eq(tree->Root()[0].String()->Buffer, " abcdef"));
		VALIDATE(Eq(tree->Root()[1].String()->Buffer, "a"));
		VALIDATE(Eq(tree->Root()[2].String()->Buffer, "b"));
		VALIDATE(Eq(tree->Root()[3].String()->Buffer, "cd"));
		auto& q = tree->Root()[4];
		VALIDATE(q.NumberOfElements() == 1);
		VALIDATE(Eq(q[0].String()->Buffer, "qcumber"));

		VALIDATE(&tree->Source() == src);
		VALIDATE(0 == tree->Release());

		sparser->Release();
		VALIDATE(0 == src->Release());
		return 0;
	}
	catch (IException& ex)
	{
		printf("Exception code %d:\n %s\n", ex.ErrorCode(), ex.Message());
		return ex.ErrorCode();
	}
}

