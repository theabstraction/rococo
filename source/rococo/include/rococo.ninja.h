// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
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

		ROCOCO_INTERFACE IExpression
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

		ROCOCO_INTERFACE IExpressionException: IException
		{
			virtual IExpression* Source() = 0;
		};

		ROCOCO_INTERFACE ISourceWriter
		{
			virtual void Write(cstr filename, fstring text) = 0;
		};

		ROCOCO_INTERFACE ISourceData
		{
			virtual char* CreateBuffer(size_t nBytes) = 0;
		};

		ROCOCO_INTERFACE ISourceLoader
		{
			virtual void Load(cstr filename, ISourceData& sourceData) = 0;
		};

		ROCOCO_INTERFACE ICompiler
		{
			virtual void Compile(cstr filename) = 0;
		};

		struct FactorySpec
		{
		};

		struct CompileSpec
		{

		};

		ROCOCO_INTERFACE IFactory
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