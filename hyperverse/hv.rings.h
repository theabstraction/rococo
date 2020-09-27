#pragma once

namespace HV
{
	// If [array] represents a ring, then GetRingElement(i...) returns the ith element using modular arithmetic
	template<class T>
	T GetRingElement(size_t index, const T* array, size_t capacity)
	{
		return array[index % capacity];
	}
}