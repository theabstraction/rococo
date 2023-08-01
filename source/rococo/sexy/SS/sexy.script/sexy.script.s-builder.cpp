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

#include "sexy.script.stdafx.h"
#include "sexy.s-parser.h"

#include <sexy.vector.h>

using namespace Rococo;
using namespace Rococo::Sex;
using namespace Rococo::Compiler;
using namespace Rococo::Script;

namespace ANON
{
	struct ExpressionBuilder;
	const ISParserTree& ToTree(ExpressionBuilder& builder);

	template<EXPRESSION_TYPE type> struct LeafExpression: public ISExpression
	{
		ExpressionBuilder* parent;
		mutable sexstring_header text;

		void Free() override
		{
			Throw(0, "LeafExpressions should be deleted by their parent.");
		}

		const Vec2i Start() const override
		{
			return { 0,0 };
		}

		const Vec2i End() const override
		{
			return { 1,1 };
		}

		EXPRESSION_TYPE Type() const override
		{
			return type;
		}

		const sexstring String() const override
		{
			return &text;
		}

		cstr c_str() const override
		{
			return String()->Buffer;
		}

		const ISParserTree& Tree() const override
		{
			return ToTree(*parent);
		}

		int NumberOfElements() const override
		{
			return 0;
		}

		const ISExpression& GetElement(int index) const override
		{
			Throw(0, "Expression has no children");
		}

		int GetIndexOf(cr_sex) const override
		{
			return -1;
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
			return Eq(token, text.Buffer);
		}
	};
	
	void Free(ExpressionBuilder* eb);

	struct ChildElement
	{
		ISExpression* s;
		bool isForeign;
	};

	struct ExpressionBuilder: public ISExpressionBuilder
	{
		TSexyVector<ChildElement> children;
		ExpressionBuilder* parent;

		ExpressionBuilder(ExpressionBuilder* _parent): parent(_parent)
		{

		}

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS

		void Free() override
		{
			delete this;
		}

		~ExpressionBuilder()
		{
			for (auto& child : children)
			{
				if (!child.isForeign)
				{
					switch (child.s->Type())
					{
					case EXPRESSION_TYPE_ATOMIC:
					case EXPRESSION_TYPE_STRING_LITERAL:
						Rococo::Memory::FreeSexyUnknownMemory((char*)child.s);
						break;
					default:
						child.s->Free();
						break;
					}
				}
			}
		}

		const Vec2i Start() const override
		{
			return { 0,0 };
		}

		const Vec2i End() const override
		{
			return { 1,1 };
		}

		EXPRESSION_TYPE Type() const override
		{
			return children.size() != 0 ? EXPRESSION_TYPE_COMPOUND : EXPRESSION_TYPE_NULL;
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
			return parent->Tree();
		}

		int NumberOfElements() const override
		{
			return (int)children.size();
		}

		const ISExpression& GetElement(int index) const override
		{
			return *children[index].s;
		}

		int GetIndexOf(cr_sex s) const override
		{
			for (int i = 0; i < NumberOfElements(); ++i)
			{
				cr_sex child = GetElement(i);
				if (&child == &s)
				{
					return i;
				}
			}

			return -1;
		}

		const ISExpression* Parent() const override
		{
			return parent;
		}

		const ISExpression* GetOriginal() const override
		{
			return parent->GetOriginal();
		}

		bool operator == (const char* token) const override
		{
			return token && this->c_str() != nullptr && Eq(token, this->c_str());
		}

		ISExpressionBuilder* AddChild() override
		{
			auto* eb = new ExpressionBuilder(this);
			eb->parent = this;
			children.push_back({ eb, false });
			return eb;
		}

		void AddRef(cr_sex s) override
		{
			children.push_back({ const_cast<ISExpression*>(&s), true });
		}

		ISExpressionBuilder* InsertChildAfter(int index) override
		{
			if ((size_t)index >= children.size())
			{
				Throw(*this, "%s: bad index %d. Child count is %llu", __FUNCTION__, index, children.size());
			}

			auto i = children.begin();

			int insertAtIndex = index + 1;
			if (insertAtIndex >= children.size())
			{
				i = children.end();
			}
			else
			{
				std::advance(i, index + 1);
			}

			auto* eb = new ExpressionBuilder(this);
			eb->parent = this;

			children.insert(i, { eb, false });

			return eb;
		}

		template<class Leaf> void AddLeaf(cstr text)
		{
			size_t len = strlen(text);
			char* buffer = (char*) Rococo::Memory::AllocateSexyMemory(sizeof(Leaf) + strlen(text) + ((8 - (len % 0x7LL)) % 0x7LL));
			auto* leaf = new (buffer) Leaf();
			leaf->parent = this;
			leaf->text.Length = (int32)len;
			memcpy(leaf->text.Buffer, text, len);
			leaf->text.Buffer[len] = 0;
			children.push_back({ leaf, false });
		}

		void AddAtomic(cstr text) override
		{
			AddLeaf<LeafExpression<EXPRESSION_TYPE_ATOMIC>>(text);
		}

		void AddStringLiteral(cstr text) override
		{
			AddLeaf<LeafExpression<EXPRESSION_TYPE_STRING_LITERAL>>(text);
		}
	};

	void Free(ExpressionBuilder* eb)
	{
		delete eb;
	}

	const ISParserTree& ToTree(ExpressionBuilder& builder)
	{
		return builder.Tree();
	}

	struct RootExpression: public ExpressionBuilder, public ISParserTree, public ISParser, public ISourceCode
	{
		refcount_t ref = 1;
		char name[64];
		cr_sex original;

		RootExpression(cr_sex _original): ExpressionBuilder(nullptr), original(_original)
		{
			SafeFormat(name, sizeof(name), "['] %s", _original.Tree().Source().Name());
		}

		const ISParserTree& Tree() const override
		{
			return *this;
		}

		const ISExpression* Parent() const override
		{
			return nullptr;
		}

		const ISExpression* GetOriginal() const override
		{
			return &original;
		}

		refcount_t AddRef() override
		{
			return ++ref;
		}

		refcount_t Release() override
		{
			ref--;
			if (ref == 0)
			{
				delete this;
				return 0;
			}
			else
			{
				return ref;
			}
		}

		ISExpression& Root() override { return *this; }
		cr_sex Root() const override { return *this; }

		const Vec2i& Origin() const override
		{
			static Vec2i origin{ 1,1 };
			return origin;
		}

		ISParser& Parser() override
		{
			return *this;
		}

		cstr SourceStart() const override
		{
			return "";
		}

		const int SourceLength() const override
		{
			return 0;
		}

		cstr Name() const override
		{
			return name;
		}

		const ISourceCode& Source() const override
		{
			return *this;
		}

		const IPackage* Package() const override
		{
			return nullptr;
		}

		ISParserTree* CreateTree(ISourceCode&) override
		{
			Throw(0, "Not implemented on Expression Builder");
		}

		ISourceCode* DuplicateSourceBuffer(cstr, int, const Vec2i&, const char*) override
		{
			Throw(0, "Not implemented on Expression Builder");
		}

		ISourceCode* ProxySourceBuffer(cstr, int, const Vec2i&, cstr, IPackage*) override
		{
			Throw(0, "Not implemented on Expression Builder");
		}

		ISourceCode* LoadSource(const wchar_t*, const Vec2i&) override
		{
			Throw(0, "Not implemented on Expression Builder");
		}

		ISourceCode* LoadSource(const wchar_t*, const Vec2i&, const char*, long) override
		{
			Throw(0, "Not implemented on Expression Builder");
		}
	};

	struct ExpressionTransform : public IExpressionTransform
	{
		cr_sex originator;

		Auto<RootExpression> root;

		DEFINE_SEXY_ALLOCATORS_FOR_CLASS;

		ExpressionTransform(cr_sex _originator) :
			originator(_originator),
			root(new RootExpression(_originator))
		{
		}

		ISExpressionBuilder& Root() override
		{
			ISExpressionBuilder& builder = static_cast<ISExpressionBuilder&>(*root);
			return builder;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo
{
	namespace Sex
	{
		IExpressionTransform* CreateExpressionTransform(cr_sex src)
		{
			return new ANON::ExpressionTransform(src);
		}
	} // Sex
} // Rococo