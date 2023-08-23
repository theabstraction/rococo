#pragma once
#include <rococo.debugging.h>
#include <unordered_map>
#include <vector>
#include <algorithm>

using namespace Rococo;
using namespace Rococo::Debugging;

namespace
{
	std::unordered_map<uint64, int> mapAddressToUsage;
}

void ClearPerformanceMap()
{
	std::vector<std::pair<int, uint64>> perfVector;
	for (auto& i : mapAddressToUsage)
	{
		perfVector.push_back(std::make_pair(i.second, i.first));
	}

	std::sort(perfVector.begin(), perfVector.end(), [](const std::pair<int, uint64>& a, const std::pair<int, uint64>& b)
		{
			return a.first < b.first;
		}
	);

	printf("\nFunction profile. Call count vs caller\n");

	for (auto& i : perfVector)
	{
		char desc[256];
		StackFrame::Address addr;
		addr.segment = 0;
		addr.offset = i.second;
		Debugging::FormatStackFrame(desc, sizeof desc, addr);
		printf("%4.4d - %s\n", i.first, desc);
	}
}

void ProfileThis()
{
// The greater the depth the slower the function, but the deeper down the call stack function addresses are recorded
	enum {TRACE_DEPTH = 4};
	StackFrame::Address caller = FormatStackFrame(nullptr, 0, TRACE_DEPTH);

	if (mapAddressToUsage.empty())
	{
		atexit(ClearPerformanceMap);
	}

	auto i = mapAddressToUsage.insert(std::make_pair(caller.offset, 1));
	if (!i.second)
	{
		// duplicate
		i.first->second++;
	}
}