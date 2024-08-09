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
				size_t Offset;
				size_t Size;
			};

			explicit MemoryHeap(size_t totalSize);

			size_t Allocate(size_t size);

			[[nodiscard]] Allocation GetAllocationInfo(size_t id) const;

			void Deallocate(size_t id);

		private:
			size_t                                 _totalSize;
			size_t                                 _nextId;
			std::unordered_map<size_t, Allocation> _allocations {};
			std::set<std::pair<size_t, size_t>>    _freeBlocks {};

			void CoalesceFreeBlocks();
	};
}
