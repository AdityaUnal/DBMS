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
#include <optional>
#include <utility>
#include <vector>
#include "buffer/traced_buffer_pool_manager.h"
#include "common/config.h"
#include "common/macros.h"
#include "common/rid.h"
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
  WritePageGuard header_guard = bpm_->WritePage(header_page_id_);
  auto header_page = header_guard.AsMut<BPlusTreeHeaderPage>();
  header_page->root_page_id_ = INVALID_PAGE_ID;
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
  Context ctx;
  
  // ctx.header_page_ = bpm_->WritePage(header_page_id_).AsMut<BPlusTreeHeaderPage>();
  page_id_t root_page_id;
  {
    ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
    auto header_page = guard.As<BPlusTreeHeaderPage>();
    root_page_id = header_page->root_page_id_;
  }

  switch (root_page_id) {
    case INVALID_PAGE_ID:
    {
      return false;
    }
    default:
    {
      ctx.root_page_id_ = root_page_id;
      get_path_read_guards(ctx,key);
      auto leaf_page_guard = std::move(ctx.read_set_.front());
      // auto leaf_page_id = leaf_page_guard.GetPageId();
      auto leaf_page = leaf_page_guard.As<BPlusTreePage>();
      auto leaf_read_page = leaf_page_guard.As<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();

      int n = leaf_page->GetSize();
      
      for(int i = 0;i < n;i +=1){
        if(comparator_(key,leaf_read_page->KeyAt(i)) == 0){
          return true;
        }
      }
      break;
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
void BPLUSTREE_TYPE::get_path_read_guards(Context& ctx,const KeyType &key){
  page_id_t root_page_id = ctx.root_page_id_;
  auto curr_page_guard = bpm_->ReadPage(root_page_id);
  auto curr_page = curr_page_guard.As<BPlusTreePage>(); 
  ctx.read_set_.push_front(std::move(curr_page_guard));
  page_id_t curr_page_id;  
  
  while(!curr_page->IsLeafPage()){
    auto page_size = curr_page->GetSize();
    auto curr_internal_page =
        ctx.read_set_.front().As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
        // auto it = std::upper_bound(curr_internal_page->KeyAt(1),curr_internal_page->KeyAt(internal_max_size_),key,comparator_);
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
    ctx.read_set_.push_front(std::move(curr_page_guard));
  }
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::get_path_write_guards(Context& ctx,const KeyType &key){
  page_id_t root_page_id = ctx.root_page_id_;
  auto curr_page_guard = bpm_->WritePage(root_page_id);
  auto curr_page = curr_page_guard.As<BPlusTreePage>(); 
  ctx.write_set_.push_front(std::move(curr_page_guard));
  page_id_t curr_page_id;  
  
  while(!curr_page->IsLeafPage()){
    auto page_size = curr_page->GetSize();
    auto curr_internal_page =
        ctx.write_set_.front().AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
        // auto it = std::upper_bound(curr_internal_page->KeyAt(1),curr_internal_page->KeyAt(internal_max_size_),key,comparator_);
        for(int i = 0;i < page_size;i +=1){
          if(i != page_size - 1){
            if(comparator_(curr_internal_page->KeyAt(i + 1), key ) >= 0){
          curr_page_id = curr_internal_page->ValueAt(i);
          break;
        }
      }
      curr_page_id = curr_internal_page->ValueAt(i);
    }
    curr_page_guard = bpm_->WritePage(curr_page_id);
    curr_page = curr_page_guard.As<BPlusTreePage>();
    ctx.write_set_.push_front(std::move(curr_page_guard));
  }
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::AddAndSplitLeaf(WritePageGuard& page_guard,WritePageGuard& parent_page_guard,const KeyType &key,const ValueType &value) 
-> std::pair<KeyType,page_id_t>{
  auto curr_leaf_page = page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
  auto curr_page = page_guard.AsMut<BPlusTreePage>();
  int n = curr_page->GetSize();
  std::vector<std::pair<KeyType,ValueType>> temp(n + 1);
  for(int i = 0;i < n;i +=1){
    temp[i] = std::make_pair(curr_leaf_page->KeyAt(i), curr_leaf_page->ValueAt(i));
  }
  temp[n] = std::make_pair(key, value);
  std::sort(temp.begin(), temp.end(),
            [&](const auto &a, const auto &b) { return comparator_(a.first, b.first) < 0; });
  
  page_id_t new_leaf_page_id = bpm_->NewPage();
  WritePageGuard new_leaf_page_guard = bpm_->WritePage(new_leaf_page_id);
  auto new_leaf_page = new_leaf_page_guard.AsMut<BPlusTreeLeafPage<KeyType,RID,KeyComparator>>();

  new_leaf_page->Init(leaf_max_size_);
  curr_leaf_page->Init(leaf_max_size_);

  for(int i = 0;i <= n;i +=1){
    if(i < n/2){
      new_leaf_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
    }
    else{
      curr_leaf_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
    }
  }

  auto parent_page = parent_page_guard.AsMut<BPlusTreePage>();
  auto parent_internal_page = parent_page_guard.AsMut<BPlusTreeInternalPage<KeyType,page_id_t,KeyComparator>>();
  for(int i = 1;i < parent_page->GetSize();i +=1){
    if(parent_internal_page->ValueAt(i) == page_guard.GetPageId()){
      parent_internal_page->SetKeyAt(i,temp[n/2].first);
    }
  }
  return std::make_pair(temp[0].first,new_leaf_page_id);
  //find index in parent page and set the key as temp[n/2]
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::AddAndSplitInternal(WritePageGuard& page_guard,WritePageGuard& parent_page_guard,const KeyType &key,const page_id_t &value) 
-> std::pair<KeyType,page_id_t>{
  auto curr_internal_page = page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  auto curr_page = page_guard.AsMut<BPlusTreePage>();
  int n = curr_page->GetSize();
  std::vector<std::pair<KeyType,page_id_t>> temp(n + 1);
  temp[0] = {KeyType(), curr_internal_page->ValueAt(0)};  // dummy key, real child ptr
  for (int i = 1; i < n; i++) {
    temp[i] = {curr_internal_page->KeyAt(i), curr_internal_page->ValueAt(i)};
  }
  temp[n] = std::make_pair(key, value);
  std::sort(temp.begin(), temp.end(),
            [&](const auto &a, const auto &b) { return comparator_(a.first, b.first) < 0; });
  
  page_id_t new_internal_page_id = bpm_->NewPage();
  WritePageGuard new_internal_page_guard = bpm_->WritePage(new_internal_page_id);
  auto new_internal_page = new_internal_page_guard.AsMut<BPlusTreeInternalPage<KeyType,page_id_t,KeyComparator>>();
  curr_internal_page->Init(internal_max_size_);

  for(int i = 0;i <= n;i +=1){
    if(i < n/2){
      if(i!=0){
        new_internal_page->SetKeyAt(i,temp[i].first);
      }
      new_internal_page->SetValueAt(i,temp[i].second);
    }
    else{
      curr_internal_page->SetKeyAt(i - (n/2),temp[i].first);
      curr_internal_page->SetValueAt(i - (n/2),temp[i].second);
    }
  }
  //Add if parent_page_guard is header
  auto header_page_guard = std::move(parent_page_guard);
  if(parent_page_guard.GetPageId() == header_page_id_){
    /*
    * Write a new root
    * keep it as an internal page
    * change the root in header
    * 0th value will be the new one
    */
    auto new_root_id = bpm_->NewPage();
    auto header_page = header_page_guard.AsMut<BPlusTreeHeaderPage>();
    header_page->root_page_id_ = new_root_id;
    parent_page_guard = bpm_->WritePage(new_root_id);
    auto temp_internal_page = parent_page_guard.AsMut<BPlusTreeInternalPage<KeyType,page_id_t,KeyComparator>>();
    temp_internal_page->Init(internal_max_size_);
  }
  auto parent_page = parent_page_guard.AsMut<BPlusTreePage>();
  auto parent_internal_page = parent_page_guard.AsMut<BPlusTreeInternalPage<KeyType,page_id_t,KeyComparator>>();
  for(int i = 1;i < parent_page->GetSize();i +=1){
    if(parent_internal_page->ValueAt(i) == page_guard.GetPageId()){
      parent_internal_page->SetKeyAt(i,temp[n/2].first);
    }
  }
  return std::make_pair(temp[0].first,new_internal_page_id);
  //find index in parent page and set the key as temp[n/2]
}


FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::pessimist_latch_required(WritePageGuard& page_guard,std::vector<KeyType> &keys,std::vector<RID> &Rids) -> bool{
  
  auto curr_write_page = page_guard.AsMut<BPlusTreePage>();
  int n = keys.size();
  int write_n = curr_write_page->GetSize();
  
  if(n != write_n){
    return true;
  }
  
  auto curr_write_leaf_page = page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
  for(int i = 0;i < n;i +=1){
    const RID curr_rid = curr_write_leaf_page->ValueAt(i);
    if (comparator_(curr_write_leaf_page->KeyAt(i), keys[i]) != 0 ||
        curr_rid.GetPageId() != Rids[i].GetPageId() ||
        curr_rid.GetSlotNum() != Rids[i].GetSlotNum()) {
      return true;
    }
  }
  return false;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  std::map<page_id_t,int> mp;
  // Declaration of context instance. Using the Context is not necessary but advised.
  Context ctx;
  
    // ctx.header_page_ = bpm_->WritePage(header_page_id_).AsMut<BPlusTreeHeaderPage>();
  page_id_t root_page_id;
  {
    ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
    auto header_page = guard.As<BPlusTreeHeaderPage>();
    root_page_id = header_page->root_page_id_;
  }

  switch (root_page_id) {
    case INVALID_PAGE_ID:
    {
      WritePageGuard write_header_guard = bpm_->WritePage(header_page_id_);
      auto header_page_guard = write_header_guard.AsMut<BPlusTreeHeaderPage>();
      
      page_id_t root_page_id = bpm_->NewPage(); 
      
      auto root_page_guard = bpm_->WritePage(root_page_id);
      
      auto root_leaf_page = root_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
      root_leaf_page->Init(leaf_max_size_);

      root_leaf_page->InsertKeyValue(key,value,comparator_);
      
      header_page_guard->root_page_id_ = root_page_id;
      ctx.root_page_id_ = root_page_id;
      return true;
    }
    default:
    {
      ctx.header_page_ = bpm_->WritePage(header_page_id_);
      ctx.root_page_id_ = root_page_id;
      get_path_read_guards(ctx,key);
      auto leaf_page_guard = std::move(ctx.read_set_.front());
      auto leaf_page_id = leaf_page_guard.GetPageId();
      ctx.read_set_.pop_front();
      
      auto leaf_page = leaf_page_guard.As<BPlusTreePage>();
      auto leaf_read_page = leaf_page_guard.As<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();

      int n = leaf_page->GetSize();
      bool found = false;
      
      std::vector<KeyType> curr_keys;
      std::vector<RID> curr_Rids;
      for(int i = 0;i < n;i +=1){
        curr_keys.push_back(leaf_read_page->KeyAt(i));
        curr_Rids.push_back(leaf_read_page->ValueAt(i));
        if(comparator_(key,leaf_read_page->KeyAt(i)) == 0){
          found = true;
        }
      }

      if(found){
        return false;
      }

      leaf_page_guard.Drop();
      if(leaf_page->GetSize() == leaf_page->GetMaxSize()){
        break;
      }
      auto leaf_write_page_guard = bpm_->WritePage(leaf_page_id);
      
      if(pessimist_latch_required(leaf_write_page_guard,curr_keys,curr_Rids)){
        break;
      }
      auto curr_write_leaf_page = leaf_write_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
      
      if(curr_write_leaf_page->IndexOfKey(key,comparator_)!=-1){
        return false;
      }

      if(leaf_page->GetSize() == leaf_page->GetMaxSize()){
        leaf_write_page_guard.Drop();
        break;
      }

      curr_write_leaf_page->InsertKeyValue(key,value,comparator_);
      return true;
    }
  }
  ctx.read_set_.clear();
  get_path_write_guards(ctx, key);
  // ctx.header_page_ = bpm_->WritePage(header_page_id_);
  KeyType new_key;
  page_id_t new_value;
  //fix the value bug
  /*
  * Move the write guard from front of write set.
  * If it is a leaf page
  ** See if key is already present
  ** See if there is space
  ** If No spave and and split and get the new key and value
  */
  while(!ctx.write_set_.empty()){
    WritePageGuard curr_page_guard = std::move(ctx.write_set_.front());
    ctx.write_set_.pop_front();
    auto curr_page = curr_page_guard.AsMut<BPlusTreePage>();
    if(curr_page->IsLeafPage()){
      auto write_leaf_page = curr_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
      if(write_leaf_page->IndexOfKey(key,comparator_) != -1){
        return false;
      }
      if(curr_page->GetSize() < curr_page->GetMaxSize()){
        write_leaf_page->InsertKeyValue(key,value,comparator_);
        return true;
      }
      else{
        // WritePageGuard parent_page_guard;
        if(ctx.write_set_.empty()){
          auto new_root_id = bpm_->NewPage();
          auto new_root_page = bpm_->WritePage(new_root_id);
          auto new_root_internal_page = new_root_page.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
          new_root_internal_page->Init(internal_max_size_);
          new_root_internal_page->SetValueAt(0,curr_page_guard.GetPageId());
          auto header_page = ctx.header_page_->AsMut<BPlusTreeHeaderPage>();
          header_page->root_page_id_ = new_root_id;
          ctx.write_set_.push_front(std::move(new_root_page));
        }
        std::tie(new_key,new_value)  = AddAndSplitLeaf(curr_page_guard,ctx.write_set_.front(),key,value);
      }
    }
    else{
      int n = curr_page->GetSize();
      if(curr_page->GetSize() < curr_page->GetMaxSize()){
        std::vector<std::pair<KeyType,page_id_t>> temp(n + 1);
        auto write_internal_page = curr_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
        temp[0] = {KeyType(), write_internal_page->ValueAt(0)};  // dummy key, real child ptr
        for (int i = 1; i < n; i++) {
          temp[i] = {write_internal_page->KeyAt(i), write_internal_page->ValueAt(i)};
        }
        temp[n] = std::make_pair(new_key,new_value);
        /*See if first value is empty*/
        std::sort(temp.begin(), temp.end(),
                  [&](const auto &a, const auto &b) { return comparator_(a.first, b.first) < 0; });

        
        for(int i = 0;i <= n;i +=1){
          if(i!=0){
            write_internal_page->SetKeyAt(i,temp[i].first);
            write_internal_page->SetValueAt(i,temp[i].second);
          }
          else{
            write_internal_page->SetValueAt(i,temp[i].second);
          }
        }
        return true;
      }
      else{
        if(ctx.write_set_.empty()){
          auto new_root_id = bpm_->NewPage();
          auto new_root_page = bpm_->WritePage(new_root_id);
          auto new_root_internal_page = new_root_page.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
          new_root_internal_page->Init(internal_max_size_);
          new_root_internal_page->SetValueAt(0,curr_page_guard.GetPageId());
          auto header_page = ctx.header_page_->AsMut<BPlusTreeHeaderPage>();
          header_page->root_page_id_ = new_root_id;
          ctx.write_set_.push_front(std::move(new_root_page));  
        }
        std::tie(new_key,new_value)  = AddAndSplitInternal(curr_page_guard,ctx.write_set_.front(),new_key,new_value);
  
        // write_guard.Drop();

      }
  }
}
return false;
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
      ctx.read_set_.push_front(std::move(curr_page_guard));
      
      while(!curr_page->IsLeafPage()){
        auto page_size = curr_page->GetSize();
        auto curr_internal_page =
            ctx.read_set_.front().As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
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
        ctx.read_set_.push_front(std::move(curr_page_guard));
      }
    }
  }
  // auto leaf_page_id = ctx.read_set_.front().GetPageId();
  // auto leaf_page_guard = bpm_->WritePage(leaf_page_id);
  // ctx.write_set_.push_front(leaf_page_guard);
  // ctx.read_set_.pop_front();
  // auto leaf_page = leaf_page_guard.AsMut<BPlusTreePage>();
  // leaf_page->ChangeSizeBy(-1);
  KeyType new_key;
  while(!ctx.read_set_.empty()){
    auto curr_page_id = ctx.read_set_.front().GetPageId();
    WritePageGuard curr_write_page_guard = bpm_->WritePage(curr_page_id);
    ctx.write_set_.push_front(std::move(curr_write_page_guard));
    ctx.read_set_.pop_front();
    auto &write_guard = ctx.write_set_.front();
    auto curr_page = write_guard.AsMut<BPlusTreePage>();
    if(curr_page->IsLeafPage()){
      auto curr_leaf_page = write_guard.AsMut<BPlusTreeLeafPage<KeyType,RID,KeyComparator>>();
      curr_leaf_page->RemoveKeyValue(key,comparator_);

      if(curr_page->GetSize() < curr_page->GetMinSize()){
        auto parent_id = ctx.read_set_.front().GetPageId();
        auto parent_page_guard = bpm_->WritePage(parent_id);
        auto parent_page = parent_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
        ctx.read_set_.pop_front();
      
        int prev_size = INT_MAX, next_size = INT_MAX;
        WritePageGuard prev_page_guard;
        WritePageGuard next_page_guard;
        BPlusTreePage* prev_page;
        BPlusTreePage* next_page;

        if(mp[parent_id]!=0){
          int prev_idx = mp[prev_size];
          int prev_page_id = parent_page->ValueAt(prev_idx);
          prev_page_guard = bpm_->WritePage(prev_page_id);
          prev_page = prev_page_guard.AsMut<BPlusTreePage>();
          prev_size = prev_page->GetSize();
        }

        if(mp[parent_id]!=parent_page->GetSize() - 1){
          int next_idx = mp[next_size];
          int next_page_id = parent_page->ValueAt(next_idx);
          next_page_guard = bpm_->WritePage(next_page_id);
          next_page = next_page_guard.AsMut<BPlusTreePage>();
          next_size = next_page->GetSize();
        }

        if(prev_size < next_size and prev_size > prev_page->GetMinSize()){
          auto prev_leaf_page = prev_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
          std::vector<std::pair<KeyType,RID>> temp(prev_size + curr_leaf_page->GetSize());
          for(int i = 0;i < prev_size;i +=1){
            temp[i] = std::make_pair(prev_leaf_page->KeyAt(i),prev_leaf_page->ValueAt(i));
          }
          for(int i = 0;i < curr_page->GetSize();i +=1){
            temp[i + prev_size] = std::make_pair(curr_leaf_page->KeyAt(i),curr_leaf_page->ValueAt(i));
          }
          std::sort(temp.begin(), temp.end(),
                  [&](const auto &a, const auto &b) { return comparator_(a.first, b.first) < 0; });
          int n = temp.size();
          
          auto new_prev_page_id = bpm_->NewPage();
          auto new_prev_page_guard = bpm_->WritePage(new_prev_page_id);
          auto new_prev_write_page = new_prev_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
          
          auto new_leaf_page_id = bpm_->NewPage();
          auto new_leaf_page_guard = bpm_->WritePage(new_leaf_page_id);
          auto new_leaf_write_page = new_leaf_page_guard.AsMut<BPlusTreeLeafPage<KeyType,RID,KeyComparator>>();
          
          
          for(int i = 0;i < n;i +=1){
            if(i <= n/2){
              new_prev_write_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
            }
            else{
              new_leaf_write_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
            }
          } 
          parent_page->SetValueAt(mp[parent_id] - 1,new_prev_page_id);
          parent_page->SetKeyAt(mp[parent_id],temp[n/2].first);
          parent_page->SetValueAt(mp[parent_id],new_leaf_page_id); 
          prev_page_guard.Drop();
          write_guard.Drop();
        }
        else if(next_size != INT_MAX and next_size > next_page->GetMinSize()){
          auto next_leaf_page = prev_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
          std::vector<std::pair<KeyType,RID>> temp(prev_size + curr_leaf_page->GetSize());
          for(int i = 0;i < prev_size;i +=1){
            temp[i] = std::make_pair(next_leaf_page->KeyAt(i),next_leaf_page->ValueAt(i));
          }
          for(int i = 0;i < curr_page->GetSize();i +=1){
            temp[i + prev_size] = std::make_pair(curr_leaf_page->KeyAt(i),curr_leaf_page->ValueAt(i));
          }
          std::sort(temp.begin(), temp.end(),
                  [&](const auto &a, const auto &b) { return comparator_(a.first, b.first) < 0; });
          int n = temp.size();
          
          auto new_next_page_id = bpm_->NewPage();
          auto new_next_page_guard = bpm_->WritePage(new_next_page_id);
          auto new_next_write_page = new_next_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
          
          auto new_leaf_page_id = bpm_->NewPage();
          auto new_leaf_page_guard = bpm_->WritePage(new_leaf_page_id);
          auto new_leaf_write_page = new_leaf_page_guard.AsMut<BPlusTreeLeafPage<KeyType,RID,KeyComparator>>();
          
          
          for(int i = 0;i < n;i +=1){
            if(i <= n/2){
              new_leaf_write_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
            }
            else{
              new_next_write_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
            }
          } 
          parent_page->SetKeyAt(mp[parent_id],temp[n/2].first);
          parent_page->SetValueAt(mp[parent_id],new_leaf_page_id); 
          parent_page->SetValueAt(mp[parent_id] + 1,new_next_page_id);
          prev_page_guard.Drop();
          write_guard.Drop();
        }
        else{
          if(prev_size < next_size ){
            auto prev_leaf_page = prev_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
            for(int i = 0;i < curr_page->GetSize();i +=1){
              prev_leaf_page->InsertKeyValue(curr_leaf_page->KeyAt(i), curr_leaf_page->ValueAt(i), comparator_);
            }
            new_key = curr_leaf_page->KeyAt(mp[curr_page_id]);
          }
          else if(next_size!=INT_MAX){
            auto next_leaf_page = next_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
            for(int i = 0;i < curr_page->GetSize();i +=1){
              next_leaf_page->InsertKeyValue(curr_leaf_page->KeyAt(i), curr_leaf_page->ValueAt(i), comparator_);
            }
            new_key = curr_leaf_page->KeyAt(mp[curr_page_id]);
          }
        }
      }
    }
    else{
        
        auto curr_internal_page = write_guard.AsMut<BPlusTreeInternalPage<KeyType,page_id_t,KeyComparator>>();
        curr_internal_page->RemoveKeyValue(new_key,comparator_);
    
        if(curr_page->GetSize() < curr_page->GetMinSize()){
          auto parent_id = ctx.read_set_.front().GetPageId();
          auto parent_page_guard = bpm_->WritePage(parent_id);
          auto parent_page = parent_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
          ctx.read_set_.pop_front();
        
          int prev_size = INT_MAX, next_size = INT_MAX;
          WritePageGuard prev_page_guard;
          WritePageGuard next_page_guard;
          BPlusTreePage* prev_page;
          BPlusTreePage* next_page;
    
          if(mp[parent_id]!=0){
            int prev_idx = mp[prev_size];
            int prev_page_id = parent_page->ValueAt(prev_idx);
            prev_page_guard = bpm_->WritePage(prev_page_id);
            prev_page = prev_page_guard.AsMut<BPlusTreePage>();
            prev_size = prev_page->GetSize();
          }
    
          if(mp[parent_id]!=parent_page->GetSize() - 1){
            int next_idx = mp[next_size];
            int next_page_id = parent_page->ValueAt(next_idx);
            next_page_guard = bpm_->WritePage(next_page_id);
            next_page = next_page_guard.AsMut<BPlusTreePage>();
            next_size = next_page->GetSize();
          }
    
          if(prev_size < next_size and prev_size > prev_page->GetMinSize()){
            auto prev_internal_page = prev_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
            std::vector<std::pair<KeyType,page_id_t>> temp(prev_size + curr_page->GetSize());
            for(int i = 0;i < prev_size;i +=1){
              temp[i] = std::make_pair(prev_internal_page->KeyAt(i),prev_internal_page->ValueAt(i));
            }
            for(int i = 0;i < curr_page->GetSize();i +=1){
              temp[i + prev_size] = std::make_pair(curr_internal_page->KeyAt(i),curr_internal_page->ValueAt(i));
            }
            std::sort(temp.begin(), temp.end(),
                  [&](const auto &a, const auto &b) { return comparator_(a.first, b.first) < 0; });
            int n = temp.size();
            
            auto new_prev_page_id = bpm_->NewPage();
            auto new_prev_page_guard = bpm_->WritePage(new_prev_page_id);
            auto new_prev_write_page = new_prev_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
            
            auto new_internal_page_id = bpm_->NewPage();
            auto new_internal_page_guard = bpm_->WritePage(new_internal_page_id);
            auto new_internal_write_page = new_internal_page_guard.AsMut<BPlusTreeInternalPage<KeyType,page_id_t,KeyComparator>>();
            
            
            for(int i = 0;i < n;i +=1){
              if(i <= n/2){
                new_prev_write_page->SetKeyAt(i,temp[i].first);
                new_prev_write_page->SetValueAt(i,temp[i].second);
              }
              else{
                new_internal_write_page->SetKeyAt(i - n/2 - 1,temp[i].first);
                new_internal_write_page->SetValueAt(i - n/2 - 1,temp[i].second);
              }
            } 
            parent_page->SetValueAt(mp[parent_id] - 1,new_prev_page_id);
            parent_page->SetKeyAt(mp[parent_id],temp[n/2].first);
            parent_page->SetValueAt(mp[parent_id],new_internal_page_id); 
            prev_page_guard.Drop();
            write_guard.Drop();
          }
          else if(next_size != INT_MAX and next_size > next_page->GetMinSize()){
            auto next_internal_page = prev_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
            std::vector<std::pair<KeyType,page_id_t>> temp(prev_size + curr_internal_page->GetSize());
            for(int i = 0;i < prev_size;i +=1){
              temp[i] = std::make_pair(next_internal_page->KeyAt(i),next_internal_page->ValueAt(i));
            }
            for(int i = 0;i < curr_page->GetSize();i +=1){
              temp[i + prev_size] = std::make_pair(curr_internal_page->KeyAt(i),curr_internal_page->ValueAt(i));
            }
            std::sort(temp.begin(), temp.end(),
                  [&](const auto &a, const auto &b) { return comparator_(a.first, b.first) < 0; });
            int n = temp.size();
            
            auto new_next_page_id = bpm_->NewPage();
            auto new_next_page_guard = bpm_->WritePage(new_next_page_id);
            auto new_next_write_page = new_next_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
            
            auto new_internal_page_id = bpm_->NewPage();
            auto new_internal_page_guard = bpm_->WritePage(new_internal_page_id);
            auto new_internal_write_page = new_internal_page_guard.AsMut<BPlusTreeInternalPage<KeyType,page_id_t,KeyComparator>>();
            
            
            for(int i = 0;i < n;i +=1){
              if(i <= n/2){
                new_internal_write_page->SetKeyAt(i,temp[i].first);
                new_internal_write_page->SetValueAt(i,temp[i].second);
              }
              else{
                new_next_write_page->SetKeyAt(i - n/2 - 1,temp[i].first);
                new_next_write_page->SetValueAt(i - n/2 - 1,temp[i].second);
                // new_next_write_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
        
              }
            } 
            parent_page->SetKeyAt(mp[parent_id],temp[n/2].first);
            parent_page->SetValueAt(mp[parent_id],new_internal_page_id); 
            parent_page->SetValueAt(mp[parent_id] + 1,new_next_page_id);
            prev_page_guard.Drop();
            write_guard.Drop();
          }
          else{
            if(prev_size < next_size ){
              auto prev_internal_page = prev_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
              for(int i = 0;i < curr_page->GetSize();i +=1){
                prev_internal_page->SetKeyAt(i + prev_size,curr_internal_page->KeyAt(i));
                prev_internal_page->SetValueAt(i + prev_size,curr_internal_page->ValueAt(i));
              }
              // curr_internal_page->SetKeyAt(mp[curr_page_id] + 1,temp[0].first);
              new_key = curr_internal_page->KeyAt(mp[curr_page_id]);
            }
            else if(next_size!=INT_MAX){
              auto next_internal_page = next_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
              int n  = next_size + curr_page->GetSize();
              std::vector<std::pair<KeyType,page_id_t>> temp(n);

              for(int i = 0;i < curr_page->GetSize();i +=1){
                temp.push_back(std::make_pair(curr_internal_page->KeyAt(i),curr_internal_page->ValueAt(i))) ;
              }
              for(int i = 0;i < next_size;i +=1){
                temp.push_back(std::make_pair(next_internal_page->KeyAt(i),next_internal_page->ValueAt(i))) ;
              }
              std::sort(temp.begin(), temp.end(),
                  [&](const auto &a, const auto &b) { return comparator_(a.first, b.first) < 0; });
              for(int i = 0;i < n;i +=1){
                next_internal_page->SetKeyAt(i,temp[i].first);
                next_internal_page->SetValueAt(i,temp[i].second);
              }
              curr_internal_page->SetKeyAt(mp[curr_page_id] + 1,temp[0].first);
              new_key = curr_internal_page->KeyAt(mp[curr_page_id]);
            }
          }
        return;
      }
    }

  }
}
  // if(leaf_page->GetSize() < leaf_page->GetMinSize() and !ctx.IsRootPage(leaf_page_id)){
    
    
  //   if(next_size > prev_size){
  //     int prev_idx = mp[prev_size];
  //     int prev_page_id = parent_page->ValueAt(prev_idx);
  //     auto prev_page_guard = bpm_->WritePage(prev_page_id);
  //     auto prev_page = prev_page_guard.AsMut<BPlusTreePage>();
  //     auto prev_internal_page = prev_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
  //     if(prev_size > prev_page->GetMinSize()){
  //     }
  //   }
  //   else if(next_size!=INT_MAX and next_size > next_page->GetMinSize()){
      
  //     auto next_leaf_page = next_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
  //     std::vector<std::pair<KeyType,RID>> temp(prev_size + leaf_page->GetSize());
  //     for(int i = 0;i < prev_size;i +=1){
  //       temp[i] = std::make_pair(next_leaf_page->KeyAt(i),next_leaf_page->ValueAt(i));
  //     }
  //     for(int i = 0;i < leaf_page->GetSize();i +=1){
  //       temp[i + prev_size] = std::make_pair(next_leaf_page->KeyAt(i),next_leaf_page->ValueAt(i));
  //     }
  //     std::sort(temp.begin(),temp.end());
  //     int n = temp.size();
  //     next_page_guard.Drop();
  //     leaf_page_guard.Drop();

  //     auto new_next_page_id = bpm_->NewPage();
  //     auto new_next_page_guard = bpm_->WritePage(new_next_page_id);
  //     auto new_next_page = new_next_page_guard.AsMut<BPlusTreePage>();
  //     auto new_next_write_page = new_next_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
      
  //     auto new_leaf_page_id = bpm_->NewPage();
  //     auto new_leaf_page_guard = bpm_->WritePage(new_leaf_page_id);
  //     auto new_leaf_page = new_leaf_page_guard.AsMut<BPlusTreePage>();
  //     auto new_leaf_write_page = new_leaf_page_guard.AsMut<BPlusTreeLeafPage<KeyType,RID,KeyComparator>>();
      
      
  //     for(int i = 0;i < n;i +=1){
  //       if(i <= n/2){
  //         new_next_write_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
  //         new_next_page->ChangeSizeBy(1);
  //       }
  //       else{
  //         new_leaf_write_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
  //         new_leaf_page->ChangeSizeBy(1);
  //       }
  //     } 
  //     parent_page->SetValueAt(mp[new_parent_id] - 1,new_next_page_id);
  //     parent_page->SetKeyAt(mp[new_parent_id],temp[n/2].first);
  //     parent_page->SetValueAt(mp[new_parent_id],new_leaf_page_id);

  //     //Get Values from both
  //     //sort and add
  //   }
  //   else{
      
  //     //Add to jiska size chotta h adn update the key
  //   }


  // while(ctx.read_set_.size() > 0){
  //   // new_key  = 
  //   auto curr_page_id = ctx.read_set_.front().GetPageId();
  //   auto curr_page_guard = bpm_->WritePage(curr_page_id);
  //   auto curr_page  = curr_page_guard.AsMut<BPlusTreePage>();
  //   auto curr_write_page = curr_page_guard.AsMut<BPlusTreeInternalPage<KeyType, typename ValueType, typename KeyComparator>>()
  //   if(curr_page)
  // }


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
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t {
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  return guard.As<BPlusTreeHeaderPage>()->root_page_id_;
}

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
