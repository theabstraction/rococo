// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.types.h>

namespace Rococo::Validators
{
	enum class EValidationPurpose
	{
		Construction
	};

	template<class VALUE_TYPE>
	struct IValueValidator
	{
		virtual void ThrowIfBad(VALUE_TYPE value, EValidationPurpose purpose) const = 0;
	};

	template<class VALUE_TYPE>
	struct IValueFormatter
	{
		virtual void Format(char* buffer, size_t capacity, VALUE_TYPE value) const = 0;
	};

	ROCOCO_API IValueValidator<int32>& AllInt32sAreValid();
	ROCOCO_API IValueValidator<int64>& AllInt64sAreValid();
	ROCOCO_API IValueValidator<uint32>& AllUInt32sAreValid();
	ROCOCO_API IValueValidator<uint64>& AllUInt64sAreValid();
	ROCOCO_API IValueValidator<bool>& AllBoolsAreValid();
	ROCOCO_API IValueValidator<float>& AllFloatsAreValid();
	ROCOCO_API IValueValidator<double>& AllDoublesAreValid();

	ROCOCO_API IValueFormatter<int32>& Int32Decimals();
	ROCOCO_API IValueFormatter<int64>& Int64Decimals();
	ROCOCO_API IValueFormatter<uint32>& Uint32Decimals();
	ROCOCO_API IValueFormatter<uint64>& Uint64Decimals();
	ROCOCO_API IValueFormatter<bool>& BoolFormatter();
	ROCOCO_API IValueFormatter<float>& FloatDecimals();
	ROCOCO_API IValueFormatter<double>& DoubleDecimals();
}