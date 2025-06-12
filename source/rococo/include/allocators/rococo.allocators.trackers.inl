#pragma once

#include <rococo.debugging.h>
#include "rococo.allocators.inl"
#include <algorithm>


namespace Rococo::Memory
{
	template<class KEY, class VALUE, class HASH_, class EQUAL_>
	using TAllocatorInternalMap = std::unordered_map<KEY, VALUE, HASH_, EQUAL_, RococoUtilsAllocator<std::pair<const KEY, VALUE>>>;
	// Monitors memory allocations very closely and attempts to provide detailed leak info. For specific leak debugging
	// it is suggested one copies and pastes the code into the suspected moduleand tweak for the particular issues of the leak.
	template<class T>
	class DeepTrackingAllocator
	{
	public:
		size_t totalFreed = 0;
		AllocatorMetrics stats;

		DeepTrackingAllocator()
		{
			Rococo::Debugging::Log("Constructed deep tracking allocator: %s\n", __FUNCSIG__);
		}

		~DeepTrackingAllocator()
		{
			Rococo::Debugging::Log("Destructed deep tracking allocator: %s\n", __FUNCSIG__);
		}

		struct TrackingString
		{
			// char msg[256]; // For some types of leak analysis it may be useful to add extra tracking data. The requirements are leak specific and too broad to define in a general API
			int count;
		};

		struct BigTrackingAtom : TrackingAtom
		{
			Debugging::StackFrame::Address address;
		};

		using BufferAddress = size_t;
		using BufferLength = size_t;
		using AllocationCount = size_t;

		// Note, we use a dedicated allocator for the allocator's own maps, to avoid an infinite recursive loop that would otherwise occur.
		using TTrackingMap = TAllocatorInternalMap<BufferAddress, BigTrackingAtom, hash_size_t, size_t_equal_to>;
		using TWatchMap = TAllocatorInternalMap<BufferAddress, TrackingString, hash_size_t, size_t_equal_to>;
		using TMetricsMap = TAllocatorInternalMap<BufferLength, AllocationCount, hash_size_t, size_t_equal_to>;

		TTrackingMap tracking;
		TWatchMap stackWatch;
		TMetricsMap metrics;
		size_t countWatched = 0;

		void AddMetrics(BufferLength nBytes)
		{
			auto i = metrics.find(nBytes);
			if (i == metrics.end())
			{
				metrics.insert(std::make_pair(nBytes, 1));
			}
			else
			{
				i->second++;
			}
		}

		void ShowMetrics()
		{
			auto* alloc_printf = Rococo::Debugging::Log;

			struct Metric
			{
				size_t allocSize;
				size_t allocCount;
				size_t product;
			};

			struct MetricSortByProduct
			{
				bool operator()(const Metric& a, const Metric& b) const
				{
					return a.product < b.product;
				}
			} byProduct;

			UNUSED(byProduct);

			struct MetricSortByAllocSize
			{
				bool operator()(const Metric& a, const Metric& b) const
				{
					return a.allocSize < b.allocSize;
				}
			} byAllocSize;

			std::vector<Metric, RococoUtilsAllocator<Metric>> metricArray;
			for (auto i : metrics)
			{
				Metric m;
				m.allocSize = i.first;
				m.allocCount = i.second;
				m.product = m.allocSize * m.allocCount;
				metricArray.push_back(m);
			}

			std::sort(metricArray.begin(), metricArray.end(), byAllocSize);

			alloc_printf("Metrics:\n");

			for (auto& m : metricArray)
			{
				alloc_printf(" %8llu nBytes x %8llu = %8llu\n", m.allocSize, m.allocCount, m.product);
			}
		}

		enum 
		{ 
			// Defines the allocation size, in bytes, that triggers deep tracking. Typically used to trace a leak of known size
			ALLOCATION_SIZE_WATCHED = 64
		};

		void AddProblemTracker(size_t nBytes)
		{
			if (nBytes != ALLOCATION_SIZE_WATCHED)
			{
				return;
			}

			countWatched++;

			enum
			{
				// The callstack depth here determines the depth of the current callstack from where we extract the PC address used to generate the 
				// function name and line number in the leak log
				CALLSTACK_DEPTH = 7 
			};

			/*
			TrackingString ts;

			// Stick your bespoke leak analysis code in here. Every leak requires its own especial love

			auto address = Debugging::FormatStackFrame(ts.msg, sizeof(ts.msg), CALLSTACK_DEPTH);
			Rococo::Debugging::Log(" Problem -> %s\n", ts.msg);

			/* if (strstr(ts.msg, "NastyBugString") != nullptr)
			{
				TrackingString helper;
				FormatStackFrame(helper.msg, sizeof(helper.msg), DEPTH + 1);
				Rococo::Debugging::Log(" NastyBugString -> %s\n", helper.msg);

			}
			*/

			auto address = Debugging::FormatStackFrame(nullptr, 0, CALLSTACK_DEPTH);
			auto i = stackWatch.find(address.offset);
			if (i == stackWatch.end())
			{
				stackWatch.insert(std::make_pair((const size_t)address.offset, TrackingString { 0 }));
			}
			else
			{
				i->second.count++;
			}
		}

		void* ModuleAllocate(std::size_t nBytes)
		{
			stats.totalAllocationSize += nBytes;
			stats.totalAllocations++;

			if constexpr (ALLOCATION_SIZE_WATCHED != 0)
			{
				AddProblemTracker(nBytes);
			}

			auto* data = moduleAllocatorFunctions.Allocate(nBytes);
			
			// We could add a test for logFlags.LogLeaks here, but the use of this template implies the developer is looking at leaks anyway, so skip the test
			AddTrackingData(data, nBytes, true);

			if (moduleLogFlags.LogDetailedMetrics)
			{
				AddMetrics(nBytes);
			}

			return data;
		}

		void AddTrackingData(void* buffer, size_t nBytes, bool addPCAddresses)
		{
			enum { TRACK_DEPTH = 8 };

			Debugging::StackFrame::Address address = addPCAddresses ? Debugging::FormatStackFrame(nullptr, 0, TRACK_DEPTH) : Debugging::StackFrame::Address{ 0, 0 };

			auto i = tracking.find((BufferAddress)buffer);
			if (i == tracking.end())
			{
				tracking.insert(std::make_pair((const BufferAddress)buffer, BigTrackingAtom{ nBytes, false, 0, address }));
			}
			else
			{
				BigTrackingAtom& atom = i->second;
				atom.address = address;
				atom.bufferLength = nBytes;
				atom.reuseCount++;
				atom.wasFreed = false;
			}
		}

		void ModuleFree(void* buffer)
		{
			stats.totalFrees++;

			if (buffer)
			{
				// If you have a crash here on exit, it probably means your DeepTrackingAllocator was defined after your global IAllocator instance
				auto i = tracking.find((BufferAddress) buffer);
				if (i != tracking.end())
				{
					TrackingAtom& atom = i->second;

					if (!atom.wasFreed)
					{
						totalFreed += atom.bufferLength;
						atom.wasFreed = true;
					}
				}
				else
				{
					// This element should not have been freed with this allocator
					OS::TripDebugger();
				}

				stats.usefulFrees++;
				moduleAllocatorFunctions.Free(buffer);
			}
			else
			{
				stats.blankFrees++;
			}
		}

		void Log(cstr name, cstr intro)
		{
			auto* allocator_printf = Rococo::Debugging::Log;
			Rococo::Memory::Log(stats, name, intro, allocator_printf);

			TAllocatorInternalMap<BufferLength, AllocationCount, hash_size_t, size_t_equal_to> leakMapSizeToCount;

			char buffer[256];
			for (auto i : tracking)
			{
				BigTrackingAtom& atom = i.second;
				if (!atom.wasFreed)
				{
					std::pair<const BufferLength, AllocationCount> newItem(atom.bufferLength, 1);
					auto j = leakMapSizeToCount.insert(newItem);
					if (!j.second)
					{
						j.first->second++;
					}

					Debugging::FormatStackFrame(buffer, sizeof(buffer), atom.address);
					allocator_printf("Leak at %p: %s\n", i.first, buffer);
				}
			}
			
			if (moduleLogFlags.LogDetailedMetrics)
			{
				ShowMetrics();
			}

			if (!leakMapSizeToCount.empty())
			{
				allocator_printf(" Leaks detected: (%llu bytes)\n", stats.totalAllocationSize - totalFreed);
				for (auto i : leakMapSizeToCount)
				{
					allocator_printf("%9llu bytes x %-9llu = %llu bytes\n", i.first, i.second, i.first * i.second);
				}
			}
			else
			{
				allocator_printf("No leaks detected. Keep up the good programming work!\n\n");
			}

			if constexpr (ALLOCATION_SIZE_WATCHED != 0)
			{
				allocator_printf("%llu byte allocation sizes are being watched. Total: %llu\n\n", ALLOCATION_SIZE_WATCHED, countWatched);

				for (auto& i : stackWatch)
				{
					TrackingString& ts = i.second;
					allocator_printf("%lld\n", ts.count);
				}
			}

			allocator_printf("\n\n");
		}

		using TDefaultMonitor = AllocatorMonitor<T>;
	};
} // Rococo::Memory