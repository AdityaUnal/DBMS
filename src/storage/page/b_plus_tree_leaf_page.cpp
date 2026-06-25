//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree_leaf_page.cpp
//
// Identification: src/storage/page/b_plus_tree_leaf_page.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <algorithm>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include "common/config.h"
#include "common/exception.h"
#include "common/macros.h"
#include "common/rid.h"
#include "gtest/gtest.h"
#include "storage/page/b_plus_tree_page.h"
#include "storage/page/b_plus_tree_leaf_page.h"

namespace bustub {

/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 ***************
 **************************************************************/

/**
 * @brief Init method after creating a new leaf page
 *
 * After creating a new leaf page from buffer pool, must call initialize method to set default values,
 * including set page type, set current size to zero, set page id/parent id, set
 * next page id and set max size.
 *
 * @param max_size Max size of the leaf node
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(int max_size) { 
  SetPageType(IndexPageType::LEAF_PAGE);
  SetSize(0);
  SetMaxSize(max_size);
  next_page_id_ = INVALID_PAGE_ID;
  num_tombstones_ = 0;
  key_array_[0] = KeyType();

}

/**
 * @brief Helper function for fetching tombstones of a page.
 * @return The last `NumTombs` keys with pending deletes in this page in order of recency (oldest at front).
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetTombstones() const -> std::vector<KeyType> {
  std::vector<KeyType> res;
  res.reserve(num_tombstones_);
  for(auto i:tombstones_){
    res.push_back(key_array_[i]);
  }
  return res;
}

/**
 * Helper methods to set/get next page id
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const -> page_id_t { 
  return next_page_id_;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id) {
  next_page_id_ = next_page_id;
}

/*
 * Helper method to find and return the key associated with input "index" (a.k.a
 * array offset)
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const -> KeyType { 
  return key_array_[index];
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::IsKeyPresent(const KeyType &key, const KeyComparator &comparator) const -> bool {
  for (int i = 0; i < GetSize(); i++) {
    if (comparator(key_array_[i], key) == 0) {
      return true;
    }
  }
  return false;
}
FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::ValueAt(int index) const -> ValueType {
  return rid_array_[index];
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::InsertKeyValue(const KeyType &key, const ValueType &value, KeyComparator &comparator) {
  int n = GetSize();
  BUSTUB_ASSERT(n < GetMaxSize(), "No space in leaf to insert key-value pair.");
  std::vector<std::pair<KeyType,ValueType>> temp(n + 1);
  for(int i = 0;i < n;i +=1){
    temp[i] = std::make_pair(key_array_[i], rid_array_[i]);
  }
  temp[n] = std::make_pair(key,value);
  std::sort(temp.begin(), temp.end(),
            [&](const auto &a, const auto &b) { return comparator(a.first, b.first) < 0; });
  for(int i = 0; i <= n; i+=1){
    key_array_[i] = temp[i].first;
    rid_array_[i] = temp[i].second;
  }
  ChangeSizeBy(1);  
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::RemoveKeyValue(const KeyType &key, KeyComparator &comparator) {
  int n = GetSize();
  BUSTUB_ASSERT(n < GetMaxSize(), "No space in leaf to insert key-value pair.");
  std::vector<std::pair<KeyType,ValueType>> temp(n + 1);
  for(int i = 0;i < n;i +=1){
    if(comparator(key_array_[i],key)){
      ChangeSizeBy(-1);  
      temp[i] = std::make_pair(key_array_[i], rid_array_[i]);
    }
  }
  // temp[n] = std::make_pair(key,value);
  std::sort(temp.begin(), temp.end(),
            [&](const auto &a, const auto &b) { return comparator(a.first, b.first) < 0; });
  for(int i = 0; i <= n; i+=1){
    key_array_[i] = temp[i].first;
    rid_array_[i] = temp[i].second;
  }
}
FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::IndexOfKey(const KeyType &key,KeyComparator &comparator) const -> int {
  int n = GetSize();
  for(int i = 0;i < n;i +=1){
    if(comparator(key_array_[i],key) == 0){
      return i;
    }
  }
  // temp[n] = std::make_pair(key,value);
  return -1;  
}

template class BPlusTreeLeafPage<GenericKey<4>, RID, GenericComparator<4>>;

template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>, 3>;
template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>, 2>;
template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>, 1>;
template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>, -1>;

template class BPlusTreeLeafPage<GenericKey<16>, RID, GenericComparator<16>>;

template class BPlusTreeLeafPage<GenericKey<32>, RID, GenericComparator<32>>;

template class BPlusTreeLeafPage<GenericKey<64>, RID, GenericComparator<64>>;
}  // namespace bustub
