#include "REA/MemoryHeap.hpp"

#include <SplitEngine/ErrorHandler.hpp>
#include <iterator>

namespace REA
{
	MemoryHeap::MemoryHeap(size_t totalSize) :
		_totalSize(totalSize),
		_nextId(0)
	{
		_freeBlocks.insert({ 0, totalSize });
	}

	size_t MemoryHeap::Allocate(size_t size)
	{
		for (auto it = _freeBlocks.begin(); it != _freeBlocks.end(); ++it)
		{
			if (it->second >= size)
			{
				size_t offset = it->first;
				size_t blockSize = it->second;
				_freeBlocks.erase(it);
				if (blockSize > size) {
					_freeBlocks.insert({ offset + size, blockSize - size });
				}
				_allocations[_nextId] = { offset, size };
				return _nextId++;
			}
		}

		ErrorHandler::ThrowRuntimeError("HeapAllocator: Not enough memory to allocate.");
		return static_cast<size_t>(-1);
	}

	MemoryHeap::Allocation MemoryHeap::GetAllocationInfo(size_t id) const
	{
		if (!_allocations.contains(id)) {
			ErrorHandler::ThrowRuntimeError("HeapAllocator: Invalid allocation id.");
		}
		return _allocations.at(id);
	}

	void MemoryHeap::Deallocate(size_t id)
	{
		auto it = _allocations.find(id);
		if (it == _allocations.end()) {
			ErrorHandler::ThrowRuntimeError("HeapAllocator: Invalid allocation id.");
		}

		Allocation alloc = it->second;
		_allocations.erase(it);
		_freeBlocks.insert({ alloc.Offset, alloc.Size });
		CoalesceFreeBlocks();
	}

	void MemoryHeap::CoalesceFreeBlocks()
	{
		auto it = _freeBlocks.begin();
		while (it != _freeBlocks.end())
		{
			auto next = std::next(it);
			if (next != _freeBlocks.end() && it->first + it->second == next->first)
			{
				// Coalesce current block with the next block
				size_t newSize = it->second + next->second;
				size_t newOffset = it->first;

				// Erase both current and next blocks
				it = _freeBlocks.erase(it);
				next = _freeBlocks.erase(next);

				// Insert the coalesced block and reset the iterator
				it = _freeBlocks.insert({ newOffset, newSize }).first;
			}
			else {
				++it;
			}
		}
	}
}
