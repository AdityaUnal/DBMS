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
          result->push_back(leaf_read_page->ValueAt(i));
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
            if(comparator_(curr_internal_page->KeyAt(i + 1), key ) > 0){
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
            if(comparator_(curr_internal_page->KeyAt(i + 1), key ) > 0){
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
      curr_leaf_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
    }
    else{
      new_leaf_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
    }
  }
  curr_leaf_page->SetNextPageId(new_leaf_page_id);
  return std::make_pair(temp[n/2].first,new_leaf_page_id);
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
  auto new_page = new_internal_page_guard.AsMut<BPlusTreePage>();
  
  curr_internal_page->Init(internal_max_size_);
  new_internal_page->Init(internal_max_size_);

  curr_page->SetSize(n/2);
  new_page->SetSize(n + 1 - n/2);

  for(int i = 0;i <= n;i +=1){
    if(i < n/2){
      if(i != 0){
        curr_internal_page->SetKeyAt(i,temp[i].first);
      }
      curr_internal_page->SetValueAt(i,temp[i].second);
    }
    else{
      if(i - (n/2) != 0){
        new_internal_page->SetKeyAt(i - (n/2),temp[i].first);
      }
      new_internal_page->SetValueAt(i - (n/2),temp[i].second);
    }
  }
  //Add if parent_page_guard is header

  return std::make_pair(temp[n/2].first,new_internal_page_id);
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
  
    ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
    auto header_page = guard.As<BPlusTreeHeaderPage>();
    root_page_id = header_page->root_page_id_;
  

  switch (root_page_id) {
    case INVALID_PAGE_ID:
    {
      guard.Drop();
      WritePageGuard write_header_guard = bpm_->WritePage(header_page_id_);
      auto header_page_guard = write_header_guard.AsMut<BPlusTreeHeaderPage>();
      
      root_page_id = bpm_->NewPage(); 
      
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
      // ctx.header_page_ = bpm_->WritePage(header_page_id_);
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
      int leaf_size = leaf_page->GetSize();
      int leaf_max_size = leaf_page->GetMaxSize();
      leaf_page_guard.Drop();
      if(leaf_size == leaf_max_size){
        break;
      }
      auto leaf_write_page_guard = bpm_->WritePage(leaf_page_id);
      
      if(pessimist_latch_required(leaf_write_page_guard,curr_keys,curr_Rids)){
        leaf_write_page_guard.Drop();
        break;
      }
      auto curr_write_leaf_page = leaf_write_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
      
      if(curr_write_leaf_page->IndexOfKey(key,comparator_)!=-1){
        leaf_write_page_guard.Drop();
        return false;
      }

      // if(leaf_page->GetSize() == leaf_page->GetMaxSize()){
      //   leaf_write_page_guard.Drop();
      //   break;
      // }

      curr_write_leaf_page->InsertKeyValue(key,value,comparator_);
      return true;
    }
  }
  ctx.read_set_.clear();
  guard.Drop();
  ctx.header_page_ = bpm_->WritePage(header_page_id_);
  auto header_guard = ctx.header_page_->AsMut<BPlusTreeHeaderPage>();
  ctx.root_page_id_ = header_guard->root_page_id_;
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
          auto new_root_page_guard = bpm_->WritePage(new_root_id);
          auto new_root_internal_page = new_root_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
          auto new_root_page = new_root_page_guard.AsMut<BPlusTreePage>();

          new_root_internal_page->Init(internal_max_size_);
          new_root_page->SetSize(1);
          new_root_internal_page->SetValueAt(0,curr_page_guard.GetPageId());
          auto header_page = ctx.header_page_->AsMut<BPlusTreeHeaderPage>();
          header_page->root_page_id_ = new_root_id;
          
          ctx.root_page_id_ = new_root_id;
          ctx.write_set_.push_front(std::move(new_root_page_guard));
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
        curr_page->SetSize(n + 1);
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
          auto new_root_page_guard = bpm_->WritePage(new_root_id);
          auto new_root_internal_page = new_root_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
          auto new_root_page = new_root_page_guard.AsMut<BPlusTreePage>();
          new_root_internal_page->Init(internal_max_size_);
          new_root_internal_page->SetValueAt(0,curr_page_guard.GetPageId());
          new_root_page->SetSize(1);
          auto header_page = ctx.header_page_->AsMut<BPlusTreeHeaderPage>();
          header_page->root_page_id_ = new_root_id;
          ctx.write_set_.push_front(std::move(new_root_page_guard));  
        }
        std::tie(new_key,new_value)  = AddAndSplitInternal(curr_page_guard,ctx.write_set_.front(),new_key,new_value);
  
        // write_guard.Drop();

      }
  }
}
return false;
}


FULL_INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RedistributeLeaf(WritePageGuard& left_page_guard,WritePageGuard& right_page_guard,WritePageGuard& parent_page_guard,int& left_idx,int& right_idx){
  auto left_page = left_page_guard.AsMut<BPlusTreePage>();
  auto left_leaf_page = left_page_guard.AsMut<BPlusTreeLeafPage<KeyType,RID,KeyComparator>>();

  auto right_page = right_page_guard.AsMut<BPlusTreePage>();
  auto right_leaf_page = right_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();

  int n = left_page->GetSize() + right_page->GetSize();
  std::vector<std::pair<KeyType,RID>> temp(n);

  for(int i = 0;i < left_page->GetSize();i +=1){
    temp[i] = std::make_pair(left_leaf_page->KeyAt(i),left_leaf_page->ValueAt(i));
  }
  for(int i = 0;i < right_page->GetSize();i +=1){
    temp[i + left_page->GetSize()] = std::make_pair(right_leaf_page->KeyAt(i),right_leaf_page->ValueAt(i));
  }

  left_leaf_page->Init(leaf_max_size_);
  right_leaf_page->Init(leaf_max_size_);

  for(int i = 0;i < n;i +=1){
    if(i < n/2){
      left_leaf_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
    }
    else{
      left_leaf_page->InsertKeyValue(temp[i].first,temp[i].second,comparator_);
    }
  }
  auto parent_internal_page = parent_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  parent_internal_page->SetKeyAt(left_idx,temp[0].first); 
  parent_internal_page->SetKeyAt(right_idx,temp[n/2].first); 
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RedistributeInternal(WritePageGuard& left_page_guard,WritePageGuard& right_page_guard,WritePageGuard& parent_page_guard,int& left_idx,int& right_idx){
  auto left_page = left_page_guard.AsMut<BPlusTreePage>();
  auto left_internal_page = left_page_guard.AsMut<BPlusTreeInternalPage<KeyType,RID,KeyComparator>>();

  auto right_page = right_page_guard.AsMut<BPlusTreePage>();
  auto right_internal_page = right_page_guard.AsMut<BPlusTreeInternalPage<KeyType, RID, KeyComparator>>();

  int n = left_page->GetSize() + right_page->GetSize();
  std::vector<std::pair<KeyType,RID>> temp(n);

  for(int i = 0;i < left_page->GetSize();i +=1){
    temp[i] = std::make_pair(left_internal_page->KeyAt(i),left_internal_page->ValueAt(i));
  }
  for(int i = 0;i < right_page->GetSize();i +=1){
    temp[i + left_page->GetSize()] = std::make_pair(right_internal_page->KeyAt(i),right_internal_page->ValueAt(i));
  }

  left_internal_page->Init(internal_max_size_);
  right_internal_page->Init(internal_max_size_);
  
  left_internal_page->SetSize(n/2);
  right_internal_page->SetSize(n - n/2);


  for(int i = 0;i < n;i +=1){
    if(i < n/2){
      if(i!=0){
        left_internal_page->SetKeyAt(i,temp[i].first);
      }
      left_internal_page->SetValueAt(i,temp[i].second);
    }
    else{
      if(i!=n/2){
        right_internal_page->SetKeyAt(i,temp[i].first);
      }
      right_internal_page->SetValueAt(i,temp[i].second);
    }
  }
  auto parent_internal_page = parent_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  parent_internal_page->SetKeyAt(left_idx,temp[0].first); 
  parent_internal_page->SetKeyAt(right_idx,temp[n/2].first); 
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::DeleteAndMergeLeaf(WritePageGuard& page_guard,WritePageGuard& parent_page_guard,const KeyType &key) -> page_id_t{
  auto curr_page = page_guard.AsMut<BPlusTreePage>();
  auto curr_leaf_page = page_guard.AsMut<BPlusTreeLeafPage<KeyType,RID,KeyComparator>>();
  page_id_t curr_page_id = page_guard.GetPageId();

  auto parent_page = page_guard.AsMut<BPlusTreePage>();
  auto parent_internal_page = page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  
  int idx = -1;
  
  int parent_size = parent_page->GetSize();
  // long long next_size = INT_MAX,prev_size = INT_MAX;
  for(int i = 0;i < parent_size;i +=1){
    // if(comparator_(parent_internal_page->KeyAt(i),key)==0){
    if(page_guard.GetPageId() == parent_internal_page->ValueAt(i)){
      if(i != parent_size -1){
        WritePageGuard next_page_guard = bpm_->WritePage(parent_internal_page->ValueAt(i + 1));
        auto next_page = next_page_guard.AsMut<BPlusTreePage>();
        if(next_page->GetSize() + curr_page->GetSize() < curr_page->GetMaxSize()){
          idx = i + 1;
          break;
        }
      }
      else if(i != 0){
        WritePageGuard prev_page_guard = bpm_->WritePage(parent_internal_page->ValueAt(i - 1));
        auto prev_page = prev_page_guard.AsMut<BPlusTreePage>();
        if(prev_page->GetSize() + curr_page->GetSize() < curr_page->GetMaxSize()){
          idx = i - 1;
          break;
        }
      }
      else{
        WritePageGuard other_page_guard;
        if(i == 0){
          if(parent_size == 1){
            page_guard.Drop();
            return curr_page_id;
          }
          other_page_guard = bpm_->WritePage(parent_internal_page->ValueAt(i + 1));
        }
        else{
          other_page_guard = bpm_->WritePage(parent_internal_page->ValueAt(i - 1));
        }
        curr_leaf_page->RemoveKeyValue(key,comparator_);
        RedistributeLeaf(page_guard, other_page_guard, parent_page_guard);
      }
    }
  }
  page_id_t remove_value = parent_internal_page->ValueAt(idx);
  
  
  auto remove_page_guard = bpm_->WritePage(remove_value);
  auto remove_page = remove_page_guard.AsMut<BPlusTreePage>();
  auto remove_leaf_page = remove_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
  
  // std::vector<std::pair<KeyType,page_id_t>> temp;
  // bool found = false;
  curr_leaf_page->RemoveKeyValue(key,comparator_);
  
  for(int i = 0;i < remove_page->GetSize();i +=1){
    curr_leaf_page->InsertKeyValue(remove_leaf_page->KeyAt(i),remove_leaf_page->ValueAt(i),comparator_);
  }

  return remove_value;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::DeleteAndMergeInternal(WritePageGuard& page_guard,WritePageGuard& parent_page_guard ,const KeyType &key,const page_id_t &Value) -> page_id_t{
  auto curr_page = page_guard.AsMut<BPlusTreePage>();
  auto curr_internal_page = page_guard.AsMut<BPlusTreeInternalPage<KeyType,page_id_t,KeyComparator>>();
  page_id_t curr_page_id = page_guard.GetPageId();

  auto parent_page = page_guard.AsMut<BPlusTreePage>();
  auto parent_internal_page = page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  
  int idx = -1;
  
  int parent_size = parent_page->GetSize();
  // long long next_size = INT_MAX,prev_size = INT_MAX;
  for(int i = 0;i < parent_size;i +=1){
    // if(comparator_(parent_internal_page->KeyAt(i),key)==0){
    if(page_guard.GetPageId() == parent_internal_page->ValueAt(i)){
      if(i != parent_size -1){
        WritePageGuard next_page_guard = bpm_->WritePage(parent_internal_page->ValueAt(i + 1));
        auto next_page = next_page_guard.AsMut<BPlusTreePage>();
        if(next_page->GetSize() + curr_page->GetSize() < curr_page->GetMaxSize()){
          idx = i + 1;
          break;
        }
      }
      else if(i != 0){
        WritePageGuard prev_page_guard = bpm_->WritePage(parent_internal_page->ValueAt(i - 1));
        auto prev_page = prev_page_guard.AsMut<BPlusTreePage>();
        if(prev_page->GetSize() + curr_page->GetSize() < curr_page->GetMaxSize()){
          idx = i - 1;
          break;
        }
      }
      else{
        WritePageGuard other_page_guard;
        if(i == 0){
          if(parent_size == 1){
            page_guard.Drop();
            return curr_page_id;
          }
          other_page_guard = bpm_->WritePage(parent_internal_page->ValueAt(i + 1));
        }
        else{
          other_page_guard = bpm_->WritePage(parent_internal_page->ValueAt(i - 1));
        }
        RedistributeLeaf(page_guard, other_page_guard, parent_page_guard);
        return INVALID_PAGE_ID;
      }
    }
  }
  page_id_t remove_value = parent_internal_page->ValueAt(idx);
  
  
  auto remove_page_guard = bpm_->WritePage(remove_value);
  auto remove_page = remove_page_guard.AsMut<BPlusTreePage>();
  auto remove_internal_page = remove_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  
  // std::vector<std::pair<KeyType,page_id_t>> temp;
  // bool found = false;
  int n = curr_page->GetSize();
  std::vector<std::pair<KeyType,page_id_t>> temp(n - 1);
  for(int i = 0;i < n;i +=1){
    if(Value != curr_internal_page->ValueAt(i)){
      temp.push_back(std::make_pair(curr_internal_page->KeyAt(i), curr_internal_page->ValueAt(i)));
    }
  }
  curr_internal_page->Init(internal_max_size_);
  int removed_page_n = remove_page->GetSize();
  curr_page->SetSize(n + removed_page_n);
  for(int i = 0;i < n - 1;i +=1){
    if(i!=0){
      curr_internal_page->SetKeyAt(i,temp[i].first);
    }
    curr_internal_page->SetValueAt(i,temp[i].second);
  }
  for(int i = 0;i < removed_page_n;i +=1){
    curr_internal_page->SetKeyAt(n + i,remove_internal_page->KeyAt(i));
    curr_internal_page->SetValueAt(n + i,remove_internal_page->ValueAt(i));
    // curr_leaf_page->InsertKeyValue(remove_leaf_page->KeyAt(i),remove_leaf_page->ValueAt(i),comparator_);
  }

  return remove_value;
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
  page_id_t root_page_id;
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  auto header_page = guard.As<BPlusTreeHeaderPage>();
  root_page_id = header_page->root_page_id_;
  ctx.root_page_id_ = root_page_id;
  
  get_path_read_guards(ctx,key);
  page_id_t leaf_page_id = ctx.read_set_.front().GetPageId();
  ctx.read_set_.pop_front();
  
  auto leaf_page_guard = bpm_->WritePage(leaf_page_id);
  auto leaf_page = leaf_page_guard.As<BPlusTreePage>();
  auto write_leaf_page = leaf_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
  
  if(!write_leaf_page->IsKeyPresent(key,comparator_)){
    return;
  }
  /*
  * Do a root check here
  */
  if(leaf_page->GetSize() > leaf_page->GetMinSize() ){
    if(leaf_page_id == ctx.root_page_id_){
      if(leaf_page->GetSize() == 1){
        leaf_page_guard.Drop();
        guard.Drop();
        WritePageGuard header_guard = bpm_->WritePage(header_page_id_);
        auto header_page = header_guard.AsMut<BPlusTreeHeaderPage>();
        if(header_page->root_page_id_ == root_page_id){
          header_page->root_page_id_ = INVALID_PAGE_ID;
          return;
        }
      }
    }
    write_leaf_page->RemoveKeyValue(key,comparator_);
    return;
  }
  ctx.read_set_.clear();
  guard.Drop();
  ctx.header_page_ = bpm_->WritePage(header_page_id_);
  
  auto header_guard = ctx.header_page_->AsMut<BPlusTreeHeaderPage>();
  ctx.root_page_id_ = header_guard->root_page_id_;
  get_path_write_guards(ctx, key);

  KeyType remove_key;
  page_id_t remove_value;

  while(!ctx.write_set_.empty()){
    WritePageGuard curr_page_guard = std::move(ctx.write_set_.front());
    ctx.write_set_.pop_front();
    /*
    * Do a root check here
    */
    auto curr_page = curr_page_guard.AsMut<BPlusTreePage>();
    page_id_t curr_page_id = curr_page_guard.GetPageId();
    if(curr_page->IsLeafPage()){
      auto write_leaf_page = curr_page_guard.AsMut<BPlusTreeLeafPage<KeyType, RID, KeyComparator>>();
      if(!write_leaf_page->IsKeyPresent(key,comparator_)){
        return ;
      }
      if(curr_page_id == root_page_id){
        write_leaf_page->RemoveKeyValue(key,comparator_);
        if(curr_page->GetSize() == 0){
          curr_page_guard.Drop();
          header_guard->root_page_id_ = INVALID_PAGE_ID;
          return;
        }
      }
      if(curr_page->GetSize() > curr_page->GetMinSize() ){
        write_leaf_page->RemoveKeyValue(key,comparator_);
        return;
      }
      else{
        remove_value = DeleteAndMergeLeaf(curr_page_guard, ctx.write_set_.front(),key);
        if(remove_value == INVALID_PAGE_ID){
          return ;
        }
      }
    }
    else{
      int n = curr_page->GetSize();
      if(curr_page_id == root_page_id){
        auto curr_internal_page = curr_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t,KeyComparator>>();
        // write_leaf_page->RemoveKeyValue(key,comparator_);
        std::vector<std::pair<KeyType,page_id_t>> temp(n - 1);
        for(int i = 0;i < n;i +=1){
          if(curr_internal_page->ValueAt(i) != remove_value){
            temp.push_back(std::make_pair(curr_internal_page->KeyAt(i), curr_internal_page->ValueAt(i)));
          }
          else{
            curr_page->ChangeSizeBy(-1);
          }
        }
        std::sort(temp.begin(),temp.end());
        for(int i = 0;i < n - 1;i +=1){
          if(i!=0){
            curr_internal_page->SetKeyAt(i,temp[i].first);
          }
          curr_internal_page->SetValueAt(i,temp[i].second);
        }
        if(curr_page->GetSize() == 0){
          curr_page_guard.Drop();
          header_guard->root_page_id_ = INVALID_PAGE_ID;
        }
        return;
      }
      if(curr_page->GetSize() > curr_page->GetMinSize()){
        auto curr_internal_page = curr_page_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t,KeyComparator>>();
        std::vector<std::pair<KeyType,page_id_t>> temp(n - 1);
        for(int i = 0;i < n;i +=1){
          if(curr_internal_page->ValueAt(i) != remove_value){
            temp.push_back(std::make_pair(curr_internal_page->KeyAt(i), curr_internal_page->ValueAt(i)));
          }
          else{
            curr_page->ChangeSizeBy(-1);
          }
        }
        std::sort(temp.begin(),temp.end());
        for(int i = 0;i < n - 1;i +=1){
          if(i!=0){
            curr_internal_page->SetKeyAt(i,temp[i].first);
          }
          curr_internal_page->SetValueAt(i,temp[i].second);
        }
        return;
      }
      else{
        remove_value = DeleteAndMergeInternal(curr_page_guard, ctx.write_set_.front(),remove_key,remove_value);
        if(remove_value == INVALID_PAGE_ID){
          return;
        }
      }
    }
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
