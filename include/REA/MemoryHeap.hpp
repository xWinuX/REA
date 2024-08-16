#pragma once

#include <set>
#include <unordered_map>
#include <vector>
#include <SplitEngine/ErrorHandler.hpp>


using namespace SplitEngine;

namespace REA
{
	class MemoryHeap
	{
		public:
			struct Allocation
			{
				uint32_t Offset;
				uint32_t Size;
			};

			explicit MemoryHeap(uint32_t totalSize);

			uint32_t Allocate(uint32_t size);

			[[nodiscard]] Allocation GetAllocationInfo(uint32_t id) const;

			void Deallocate(uint32_t id);

		private:
			uint32_t                                 _totalSize;
			uint32_t                                 _nextId;
			std::unordered_map<uint32_t, Allocation> _allocations{};
			std::set<std::pair<uint32_t, uint32_t>>  _freeBlocks{};

			void CoalesceFreeBlocks();
	};
}
