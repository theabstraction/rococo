#pragma once

#include <rococo.types.h>

namespace Rococo
{
	namespace Ninja
	{
		struct SourceCode
		{
			const Vec2i topLeft;
			const fstring text;
			const fstring name;
		};

		enum ExpressionType
		{
			ExpressionType_Null,
			ExpressionType_Atomic,
			ExpressionType_StringLiteral,
			ExpressionType_Compound
		};

		ROCOCOAPI IExpression
		{
			virtual IExpression* Root() = 0;
			virtual IExpression* Parent() = 0;
			virtual IExpression* begin() = 0;
			virtual IExpression* end() = 0;
			virtual IExpression* operator[] (size_t index) = 0;
			virtual size_t Count() const = 0;
			virtual fstring RawText() const = 0;
			virtual ExpressionType Type() const = 0;
			virtual Vec2i SourceStart() const = 0;
			virtual Vec2i SourceEnd() const = 0;
		};

		ROCOCOAPI IExpressionException: IException
		{
			virtual IExpression* Source() = 0;
		};

		ROCOCOAPI ISourceWriter
		{
			virtual void Write(cstr filename, fstring text) = 0;
		};

		ROCOCOAPI ISourceData
		{
			virtual char* CreateBuffer(size_t nBytes) = 0;
		};

		ROCOCOAPI ISourceLoader
		{
			virtual void Load(cstr filename, ISourceData& sourceData) = 0;
		};

		ROCOCOAPI ICompiler
		{
			virtual void Compile(cstr filename) = 0;
		};

		struct FactorySpec
		{
		};

		struct CompileSpec
		{

		};

		ROCOCOAPI IFactory
		{
			virtual void AddSourceCode(cstr filename) = 0;
			virtual IExpression& ParseSources(ISourceLoader& loader) = 0;
			virtual void GenerateTargetCode(ISourceWriter& writer, CompileSpec& spec) = 0;
			virtual void CompileTargetCode(ICompiler& compiler) = 0;
			virtual void Free() = 0;
		};

		IFactory* CreateFactory(FactorySpec& spec);
	}
}