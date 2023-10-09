#define ROCOCO_API __declspec(dllexport)
#include <rococo.types.h>
#include <vector>
#include <algorithm>
#include <random>
#include <rococo.functional.h>

namespace Rococo::Sequences
{
	ROCOCO_API void ShuffleInt32Indices(int lengthOfArray, Rococo::Function<void(int index, int count)> lambda)
	{
		std::vector<int> perms;
		perms.resize(lengthOfArray);
		for (int i = 0; i < lengthOfArray; i++)
		{
			perms[i] = i;
		}

		std::random_device rd;
		std::mt19937 g(rd());

		std::shuffle(perms.begin(), perms.end(), g);

		int count = 0;
		for (auto i : perms)
		{
			lambda.Invoke(i, count++);
		}
	}
}