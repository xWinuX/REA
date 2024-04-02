#pragma once
#include <cstdint>

namespace REA
{
	template<typename T>
	class BitSet
	{
		public:
			BitSet() = default;

			explicit BitSet(T bits) { Set(bits); }

			inline void Set(const T bits) { _mask |= bits; }

			inline void Unset(const T bits) { _mask &= ~bits; }

			inline bool Has(const T bits) const { return (_mask & bits) == bits; }

			/**
			 * Checks if this bitset matches given bitset EXACTLY
			 */
			[[nodiscard]] inline bool Matches(const BitSet<T>& other) const { return _mask == other._mask; }

			/**
			 * Determines whether this DynamicBitSet fuzzily matches another DynamicBitSet.
			 * Fuzzy matching requires that each set bit in this DynamicBitSet is also set in the given DynamicBitSet,
			 * without concern for additional bits set in the given bitset.
			 * Comment written by ChatGPT because I'm to dumb to explain this concisely
			 */
			[[nodiscard]] inline bool FuzzyMatches(const BitSet<T>& other) const { return (_mask & other._mask) == _mask; }

			[[nodiscard]] T GetMask() const { return _mask; }

		private:
			T _mask = 0;
	};
}
