#pragma once

#include <rococo.types.h>
#include <vector>
#include <rococo.strings.h>

namespace Rococo::Strings
{
	struct CharBuilder : ICharBuilder
	{
		std::vector<char>& buffer;

		CharBuilder(std::vector<char>& _buffer) : buffer(_buffer)
		{

		}

		void Clear() override
		{
			buffer.clear();
		}

		size_t Size() const override
		{
			return buffer.size();
		}

		void Resize(size_t capacity) override
		{
			buffer.resize(capacity);
		}

		char* WriteBuffer() override
		{
			return buffer.data();
		}

		cstr c_str() const override
		{
			return buffer.data();
		}
	};
}