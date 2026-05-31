//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// count_min_sketch.cpp
//
// Identification: src/primer/count_min_sketch.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/count_min_sketch.h"

#include <stdexcept>
#include <string>

namespace bustub {

/**
 * Constructor for the count-min sketch.
 *
 * @param width The width of the sketch matrix.
 * @param depth The depth of the sketch matrix.
 * @throws std::invalid_argument if width or depth are zero.
 */
template <typename KeyType>
CountMinSketch<KeyType>::CountMinSketch(uint32_t width, uint32_t depth) : 
width_(width), depth_(depth),algo_space_(depth_,std::vector<uint32_t>(width_,0)) {
  /** @TODO(student) Implement this function! */
  if(width <= 0 or depth <= 0){
    throw std::invalid_argument("Depth and width should greater than 0");
  }
  /** @spring2026 PLEASE DO NOT MODIFY THE FOLLOWING */
  // Initialize seeded hash functions
  hash_functions_.reserve(depth_);
  for (size_t i = 0; i < depth_; i++) {
    hash_functions_.push_back(this->HashFunction(i));
  }
}

template <typename KeyType>
CountMinSketch<KeyType>::CountMinSketch(CountMinSketch &&other) noexcept : 
width_(other.width_), depth_(other.depth_), hash_functions_(other.hash_functions_), algo_space_(other.algo_space_) {
  /** @TODO(student) Implement this function! */
  

}

template <typename KeyType>
auto CountMinSketch<KeyType>::operator=(CountMinSketch &&other) noexcept -> CountMinSketch & {
  /** @TODO(student) Implement this function! */
  return *this;
}

template <typename KeyType>
void CountMinSketch<KeyType>::Insert(const KeyType &item) {
  /** @TODO(student) Implement this function! */
}

template <typename KeyType>
void CountMinSketch<KeyType>::Merge(const CountMinSketch<KeyType> &other) {
  if (width_ != other.width_ || depth_ != other.depth_) {
    throw std::invalid_argument("Incompatible CountMinSketch dimensions for merge.");
  }
  /** @TODO(student) Implement this function! */
  for (uint32_t i = 0; i < depth_; i += 1) {
    for (uint32_t j = 0; j < width_; j += 1) {
      algo_space_[i][j] += other.algo_space_[i][j];
    }
  }
}

template <typename KeyType>
auto CountMinSketch<KeyType>::Count(const KeyType &item) const -> uint32_t {
  return 0;
}

template <typename KeyType>
void CountMinSketch<KeyType>::Clear() {
  /** @TODO(student) Implement this function! */
}

template <typename KeyType>
auto CountMinSketch<KeyType>::TopK(uint16_t k, const std::vector<KeyType> &candidates)
    -> std::vector<std::pair<KeyType, uint32_t>> {
  /** @TODO(student) Implement this function! */
  return {};
}

// Explicit instantiations for all types used in tests
template class CountMinSketch<std::string>;
template class CountMinSketch<int64_t>;  // For int64_t tests
template class CountMinSketch<int>;      // This covers both int and int32_t
}  // namespace bustub
