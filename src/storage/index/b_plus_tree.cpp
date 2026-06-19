//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree.cpp
//
// Identification: src/storage/index/b_plus_tree.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/index/b_plus_tree.h"
#include <algorithm>
#include <climits>
#include <cstddef>
#include<map>
#include <cstdint>
#include <vector>
#include "buffer/traced_buffer_pool_manager.h"
#include "common/config.h"
#include "storage/index/b_plus_tree_debug.h"
#include "storage/page/b_plus_tree_header_page.h"
#include "storage/page/b_plus_tree_internal_page.h"
#include "storage/page/b_plus_tree_leaf_page.h"
#include "storage/page/b_plus_tree_page.h"
#include "storage/page/page_guard.h"

namespace bustub {

FULL_INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(std::string name, page_id_t header_page_id, BufferPoolManager *buffer_pool_manager,
                          const KeyComparator &comparator, int leaf_max_size, int internal_max_size)
    : bpm_(std::make_shared<TracedBufferPoolManager>(buffer_pool_manager)),
      index_name_(std::move(name)),
      comparator_(std::move(comparator)),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size),
      header_page_id_(header_page_id) {
  WritePageGuard guard = bpm_->WritePage(header_page_id_);
  auto root_page = guard.AsMut<BPlusTreeHeaderPage>();
  root_page->root_page_id_ = INVALID_PAGE_ID;
}

/**
 * @brief Helper function to decide whether current b+tree is empty
 * @return Returns true if this B+ tree has no keys and values.
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool {
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  auto root_page = guard.As<BPlusTreeHeaderPage>();
  auto root_page_id = root_page->root_page_id_;
  return root_page_id == INVALID_PAGE_ID;
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/**
 * @brief Return the only value that associated with input key
 *
 * This method is used for point query
 *
 * @param key input key
 * @param[out] result vector that stores the only value that associated with input key, if the value exists
 * @return : true means key exists
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetValue(const KeyType &key, std::vector<ValueType> *result) -> bool {
  if(IsEmpty()){
    return false;
  }
  //Get the root 
  page_id_t root_page_id;
  {
    ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
    auto header_page = guard.As<BPlusTreeHeaderPage>();
    root_page_id = header_page->root_page_id_;
  }
  auto curr_page_guard = bpm_->ReadPage(root_page_id);
  auto curr_page = curr_page_guard.As<BPlusTreePage>(); 
  while(!curr_page->IsLeafPage()){
    auto page_size = curr_page->GetSize();
    auto curr_internal_page = curr_page_guard.As<BPlusTreeInternalPage<KeyType, page_id_t,KeyComparator>>();
    // auto it = std::upper_bound(curr_internal_page->KeyAt(1),curr_internal_page->KeyAt(internal_max_size_),key,comparator_);
    page_id_t curr_page_id;  
    for(int i = 0;i < page_size;i +=1){
      if(i != page_size - 1){
        if(comparator_(curr_internal_page->KeyAt(i + 1), key ) >= 0){
          curr_page_id = curr_internal_page->ValueAt(i);
          break;
        }
      }
      curr_page_id = curr_internal_page->ValueAt(i);
    }
    curr_page_guard = bpm_->ReadPage(curr_page_id);
    curr_page = curr_page_guard.As<BPlusTreePage>();
  }
  //Write helper function for page_guard
  auto leaf_page_size = curr_page->GetSize();
  auto leaf_page = curr_page_guard.As<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
  for(int i = 0;i < leaf_page_size;i +=1){
    if(comparator_(leaf_page->KeyAt(i),key)==0){
      result->push_back(leaf_page->ValueAt(i));
      return true;
    }
  }
  return false;
}


//   page_id_t new_parent_id = page_id;
//   WritePageGuard write_parent_page_guard = bpm_->WritePage(new_parent_id);
//   auto parent_page_guard = write_parent_page_guard.AsMut<BPlusTreePage>();
//   parent_page_guard->SetPageType(INTERNAL)
//   int n = parent_page_guard->GetMaxSize();
//   std::vector<std::pair<KeyType,ValueType>> temp(n + 1);
  

//   page_id_t prev_page_id = bpm_->NewPage(); 
//   WritePageGuard write_prev_page_guard = bpm_->WritePage(prev_page_id);
//   auto prev_page_guard = write_prev_page_guard.AsMut<BPlusTreePage>();

//   page_id_t next_page_id = bpm_->NewPage(); 
//   WritePageGuard write_next_page_guard = bpm_->WritePage(next_page_id);
//   auto next_page_guard = write_next_page_guard.AsMut<BPlusTreePage>();
  
// }

/*****************************************************************************
* INSERTION
*****************************************************************************/
 /**
 * @brief Insert constant key & value pair into b+ tree
 *
 * if current tree is empty, start new tree, update root page id and insert
 * entry; otherwise, insert into leaf page.
 *
 * @param key the key to insert
 * @param value the value associated with key
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false; otherwise, return true.
 */


FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  if(IsEmpty()){
    /*TO DO*/
    return true;
  }
  std::map<page_id_t,int> mp;
  // Declaration of context instance. Using the Context is not necessary but advised.
  Context ctx;
  
    // ctx.header_page_ = bpm_->WritePage(header_page_id_).AsMut<BPlusTreeHeaderPage>();
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  auto header_page = guard.As<BPlusTreeHeaderPage>();
  switch (header_page->root_page_id_) {
    case INVALID_PAGE_ID:
    {
      WritePageGuard write_header_guard = bpm_->WritePage(header_page_id_);
      auto header_page_guard = write_header_guard.AsMut<BPlusTreeHeaderPage>();
      
      page_id_t root_page_id = bpm_->NewPage(); 
      
      auto root_page_guard = bpm_->WritePage(root_page_id);
      
      auto root_page = root_page_guard.AsMut<BPlusTreePage>();
      root_page->SetPageType(IndexPageType::LEAF_PAGE);
      
      auto root_leaf_page = root_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
      root_leaf_page->InsertKeyValue(key,value,comparator_);
      
      header_page_guard->root_page_id_ = root_page_id;
      
      return true;
    }
    default:
    {
      auto root_page_id = header_page->root_page_id_;
      ctx.root_page_id_ = root_page_id;
      auto curr_page_guard = bpm_->ReadPage(root_page_id);
      auto curr_page = curr_page_guard.As<BPlusTreePage>(); 
      ctx.read_set_.push_front(curr_page_guard);
      
      while(!curr_page->IsLeafPage()){
        auto page_size = curr_page->GetSize();
        auto curr_internal_page = curr_page_guard.As<BPlusTreeInternalPage<KeyType, page_id_t,KeyComparator>>();
        // auto it = std::upper_bound(curr_internal_page->KeyAt(1),curr_internal_page->KeyAt(internal_max_size_),key,comparator_);
        page_id_t curr_page_id;  
        for(int i = 0;i < page_size;i +=1){
          if(i != page_size - 1){
            if(comparator_(curr_internal_page->KeyAt(i + 1), key ) >= 0){
              curr_page_id = curr_internal_page->ValueAt(i);
              mp[curr_page_id] = i;
              break;
            }
          }
          curr_page_id = curr_internal_page->ValueAt(i);
          mp[curr_page_id] = i;
        }
        curr_page_guard = bpm_->ReadPage(curr_page_id);
        curr_page = curr_page_guard.As<BPlusTreePage>();
        ctx.read_set_.push_front(curr_page_guard);
      }
      if(curr_page->IsLeafPage()){
        auto read_curr_page = curr_page_guard.As<BPlusTreeLeafPage<KeyType, RID, KeyComparator>();
        int n = curr_page->GetSize();
        for(int i = 0;i < n;i +=1){
          if(comparator_(key,read_curr_page->KeyAt(i)) == 0){
            return false;
          }
        }
      }
    }
  }
  
  KeyType new_key;
  page_id_t new_value;
  while(!ctx.read_set_.empty()){
    auto curr_page_id = ctx.read_set_.front().GetPageId();
    auto curr_write_page_guard = bpm_->WritePage(curr_page_id);
    ctx.write_set_.push_front(curr_write_page_guard);
    ctx.read_set_.pop_front();
    auto write_page = curr_write_page_guard.AsMut<BPlusTreePage>();
    if(write_page->IsLeafPage()){
      if(write_page->GetSize() < write_page->GetMaxSize()){
        auto write_leaf_page = curr_write_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
        write_leaf_page->InsertKeyValue(key,value,comparator_);
        return true;
      }
      else{
        int n = write_page->GetSize();
        auto write_leaf_page = curr_write_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
        std::vector<std::pair<KeyType,RID>> temp(n + 1);
        for(int i = 0;i < n;i +=1){
          temp[i] = std::make_pair(write_leaf_page->KeyAt(i), write_leaf_page->ValueAt(i));
        }
        temp[n] = std::make_pair(key, value);
        std::sort(temp.begin(),temp.end());
        page_id_t prev_page_id = bpm_->NewPage(); 
        WritePageGuard write_prev_page_guard = bpm_->WritePage(prev_page_id);
        auto prev_page_guard = write_prev_page_guard.AsMut<BPlusTreePage>();
        prev_page_guard->SetPageType(IndexPageType::LEAF_PAGE);
        
        page_id_t next_page_id = bpm_->NewPage(); 
        WritePageGuard write_next_page_guard = bpm_->WritePage(next_page_id);
        auto next_page_guard = write_next_page_guard.AsMut<BPlusTreePage>();
        next_page_guard->SetPageType(IndexPageType::LEAF_PAGE);
        
        for(int i = 0;i < n;i +=1){
          if(i <= n/2){
            prev_page_guard->ChangeSizeBy(1);
            auto prev_page_guard_write = write_prev_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>(); 
            prev_page_guard_write->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
          }
          else{
            next_page_guard->ChangeSizeBy(1);
            auto next_page_guard_write = write_next_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>(); 
            next_page_guard_write->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
          }
        }

        if(ctx.IsRootPage(curr_page_id)){
          curr_write_page_guard.Drop();
          WritePageGuard write_header_guard = bpm_->WritePage(header_page_id_);
          auto header_page_guard = write_header_guard.AsMut<BPlusTreeHeaderPage>();
          
          page_id_t new_root_page_id = bpm_->NewPage(); 
          
          auto root_page_guard = bpm_->WritePage(new_root_page_id);
          
          auto root_page = root_page_guard.AsMut<BPlusTreePage>();
          root_page->SetPageType(IndexPageType::INTERNAL_PAGE);
          
          auto root_internal_page = root_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
          
          root_internal_page->SetValueAt(0,prev_page_id);
          // root_internal_page->SetKeyAt(0,KeyType());
          root_internal_page->SetKeyAt(1,temp[n/2].first);
          root_internal_page->SetValueAt(1,next_page_id);
          
          header_page_guard->root_page_id_ = new_root_page_id;
          ctx.root_page_id_ = new_root_page_id;
          return true;
        }
        auto curr_internal_page =  curr_write_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
        if(mp[curr_page_id]!=0){
          curr_internal_page->SetKeyAt(mp[curr_page_id],key);
          
        }
        curr_internal_page->SetValueAt(mp[curr_page_id],new_value);
        new_key = key;
        new_value = next_page_id;
      }
    }
    else{
      int n = write_page->GetSize();
      if(write_page->GetSize() < write_page->GetMaxSize()){
        std::vector<std::pair<KeyType,page_id_t>> temp(n + 1);
        auto write_internal_page = curr_write_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
        for(int i = 0;i < n;i +=1){
          temp[i] = std::make_pair(write_internal_page->KeyAt(i), write_internal_page->ValueAt(i));
        }
        temp[n] = std::make_pair(key,new_value);
        /*See if first value is empty*/
        std::sort(temp.begin(),temp.end());

        
        for(int i = 0;i <= n;i +=1){
          if(i!=0){
            write_internal_page->SetKeyAt(i - 1,temp[i].first);
            write_internal_page->SetValueAt(i,temp[i].second);
          }
          else{
            write_internal_page->SetValueAt(i,temp[i].second);
          }
        }
      }
      else{
        auto write_internal_page = curr_write_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
        std::vector<std::pair<KeyType,page_id_t>> temp(n + 1);
        for(int i = 0;i < n;i +=1){
          temp[i] = std::make_pair(write_internal_page->KeyAt(i), write_internal_page->ValueAt(i));
        }
        temp[n] = std::make_pair(key,new_value);
        std::sort(temp.begin(),temp.end());
        page_id_t prev_page_id = bpm_->NewPage(); 
        WritePageGuard write_prev_page_guard = bpm_->WritePage(prev_page_id);
        auto prev_page_guard = write_prev_page_guard.AsMut<BPlusTreePage>();
  
  
        page_id_t next_page_id = bpm_->NewPage(); 
        WritePageGuard write_next_page_guard = bpm_->WritePage(next_page_id);
        auto next_page_guard = write_next_page_guard.AsMut<BPlusTreePage>();
        
        for(int i = 0;i < n;i +=1){
          if(i <= n/2){
            prev_page_guard->ChangeSizeBy(1);
            auto prev_page_guard_write = write_prev_page_guard.AsMut<BPlusTreeLeafPage<KeyType, page_id_t, KeyComparator>>(); 
            prev_page_guard_write->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
          }
          else{
            next_page_guard->ChangeSizeBy(1);
            auto next_page_guard_write = write_next_page_guard.AsMut<BPlusTreeLeafPage<KeyType, page_id_t, KeyComparator>>(); 
            next_page_guard_write->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
          }
        }
        auto curr_internal_page =  curr_write_page_guard.AsMut<BPlusTreeInternalPage<KeyType, RID,KeyComparator>>();
        if(mp[curr_page_id]!=0){
          curr_internal_page->SetKeyAt(mp[curr_page_id],key);
          
        }
        curr_internal_page->SetValueAt(mp[curr_page_id],value);
        
        new_key = temp[n/2].first;
        new_value = temp[n/2].second;
        // curr_write_page_guard.Drop();

      }
    }
  return false;
  }
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/**
 * @brief Delete key & value pair associated with input key
 * If current tree is empty, return immediately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 *
 * @param key input key
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key) {
  if(IsEmpty()){
    return;
  }
  Context ctx;
  std::map<page_id_t,int> mp;
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  auto header_page = guard.As<BPlusTreeHeaderPage>();
  switch (header_page->root_page_id_) {
    case INVALID_PAGE_ID:
    {
      return;
    }
    default:
    {
      auto root_page_id = header_page->root_page_id_;
      ctx.root_page_id_ = root_page_id;
      auto curr_page_guard = bpm_->ReadPage(root_page_id);
      auto curr_page = curr_page_guard.As<BPlusTreePage>(); 
      ctx.read_set_.push_front(curr_page_guard);
      
      while(!curr_page->IsLeafPage()){
        auto page_size = curr_page->GetSize();
        auto curr_internal_page = curr_page_guard.As<BPlusTreeInternalPage<KeyType, page_id_t,KeyComparator>>();
        // auto it = std::upper_bound(curr_internal_page->KeyAt(1),curr_internal_page->KeyAt(internal_max_size_),key,comparator_);
        page_id_t curr_page_id;  
        for(int i = 0;i < page_size;i +=1){
          if(i != page_size - 1){
            if(comparator_(curr_internal_page->KeyAt(i + 1), key ) >= 0){
              curr_page_id = curr_internal_page->ValueAt(i);
              mp[curr_page_id] = i;
              break;
            }
          }
          curr_page_id = curr_internal_page->ValueAt(i);
          mp[curr_page_id] = i;
        }
        curr_page_guard = bpm_->ReadPage(curr_page_id);
        curr_page = curr_page_guard.As<BPlusTreePage>();
        ctx.read_set_.push_front(curr_page_guard);
      }
    }
  }
  auto leaf_page_id = ctx.read_set_.front().GetPageId();
  auto leaf_page_guard = bpm_->WritePage(leaf_page_id);
  ctx.write_set_.push_front(leaf_page_guard);
  ctx.read_set_.pop_front();
  auto leaf_page = leaf_page_guard.AsMut<BPlusTreePage>();
  auto leaf_write_page = leaf_page_guard.AsMut<BPlusTreeLeafPage<KeyType,RID,KeyComparator>>();
  leaf_write_page->RemoveKeyValue(key,comparator_);
  if(leaf_page->GetSize() < leaf_page->GetMinSize()){
    // new_key  = 
  }
}

/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/**
 * @brief Input parameter is void, find the leftmost leaf page first, then construct
 * index iterator
 *
 * You may want to implement this while implementing Task #3.
 *
 * @return : index iterator
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/**
 * @brief Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/**
 * @brief Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/**
 * @return Page id of the root of this tree
 *
 * You may want to implement this while implementing Task #3.
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t { UNIMPLEMENTED("TODO(P2): Add implementation."); }

template class BPlusTree<GenericKey<4>, RID, GenericComparator<4>>;

template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>, 3>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>, 2>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>, 1>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>, -1>;

template class BPlusTree<GenericKey<16>, RID, GenericComparator<16>>;

template class BPlusTree<GenericKey<32>, RID, GenericComparator<32>>;

template class BPlusTree<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
