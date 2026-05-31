// :bustub-keep-private:
//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// arc_replacer.cpp
//
// Identification: src/buffer/arc_replacer.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/arc_replacer.h"
#include <optional>
#include "common/config.h"

namespace bustub {

/**
 *
 * TODO(P1): Add implementation
 *
 * @brief a new ArcReplacer, with lists initialized to be empty and target size to 0
 * @param num_frames the maximum number of frames the ArcReplacer will be required to cache
 */
ArcReplacer::ArcReplacer(size_t num_frames) : replacer_size_(num_frames) {
    curr_size_ = 0;
    mru_target_size_ = 0;
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Performs the Replace operation as described by the writeup
 * that evicts from either mfu_ or mru_ into its corresponding ghost list
 * according to balancing policy.
 *
 * If you wish to refer to the original ARC paper, please note that there are
 * two changes in our implementation:
 * 1. When the size of mru_ equals the target size, we don't check
 * the last access as the paper did when deciding which list to evict from.
 * This is fine since the original decision is stated to be arbitrary.
 * 2. Entries that are not evictable are skipped. If all entries from the desired side
 * (mru_ / mfu_) are pinned, we instead try victimize the other side (mfu_ / mru_),
 * and move it to its corresponding ghost list (mfu_ghost_ / mru_ghost_).
 *
 * @return frame id of the evicted frame, or std::nullopt if cannot evict
 */
// arc_replacer.cpp
auto ArcReplacer::Evict() -> std::optional<frame_id_t> {
    std::scoped_lock<std::mutex> lock(latch_);
    return EvictInternal();
  }

/**
* TODO(P1): Add implementation
*
* @brief Record access to a frame, adjusting ARC bookkeeping accordingly
* by bring the accessed page to the front of mfu_ if it exists in any of the lists
* or the front of mru_ if it does not.
*
* Performs the operations EXCEPT REPLACE described in original paper, which is
* handled by `Evict()`.
*
* Consider the following four cases, handle accordingly:
* 1. Access hits mru_ or mfu_
* 2/3. Access hits mru_ghost_ / mfu_ghost_
* 4. Access misses all the lists
*
* This routine performs all changes to the four lists as preperation
* for `Evict()` to simply find and evict a victim into ghost lists.
*
* Note that frame_id is used as identifier for alive pages and
* page_id is used as identifier for the ghost pages, since page_id is
* the unique identifier to the page after it's dead.
* Using page_id for alive pages should be the same since it's one to one mapping,
* but using frame_id is slightly more intuitive.
*
 * @param frame_id id of frame that received a new access.
 * @param page_id id of page that is mapped to the frame.
 * @param access_type type of access that was received. This parameter is only needed for
 * leaderboard tests.
 */
 void ArcReplacer::RecordAccess(frame_id_t frame_id, page_id_t page_id, [[maybe_unused]] AccessType access_type) {
    std::scoped_lock<std::mutex> lock(latch_) ;  
    if(alive_map_.find(frame_id) != alive_map_.end()){
        if(alive_map_[frame_id]->arc_status_ == ArcStatus::MRU){
            mru_.remove(frame_id);
        }
        else{
            mfu_.remove(frame_id);
        }
        mfu_.push_front(frame_id);
        alive_map_[frame_id]->arc_status_ = ArcStatus::MFU;
        // SetEvictableInternal(frame_id, false);
    }
    else if(ghost_map_.find(page_id) != ghost_map_.end()){    
        if(mfu_.size() + mru_.size() == replacer_size_){
            std::optional<frame_id_t> evicted_frame_id = EvictInternal();
            BUSTUB_ENSURE(evicted_frame_id != std::nullopt, "No evictable page found");
        }
        if(ghost_map_[page_id]->arc_status_ == ArcStatus::MRU_GHOST){
            mru_target_size_ = std::min(replacer_size_,mru_target_size_ + 1);
            mru_ghost_.remove(page_id);
        }
        else{
            if (mru_target_size_ > 0) {
                mru_target_size_--;
            }
            mfu_ghost_.remove(page_id);
        }
        alive_map_[frame_id] = ghost_map_[page_id];
        ghost_map_.erase(page_id);
        alive_map_[frame_id]->frame_id_ = frame_id;
        alive_map_[frame_id]->page_id_ = page_id;
        
        
        alive_map_[frame_id]->arc_status_ = ArcStatus::MFU;
        mfu_.push_front(frame_id);
        alive_map_[frame_id]->evictable_ = false;
        // SetEvictableInternal(frame_id, false);
    }
    else{
        if(mru_.size() + mru_ghost_.size() == replacer_size_){
            page_id_t evicted_page_from_mru = mru_ghost_.back();
            ghost_map_.erase(evicted_page_from_mru);
            mru_ghost_.pop_back();
        }
        if(mfu_.size() + mru_.size() == replacer_size_){
            std::optional<frame_id_t> evicted_frame_id = EvictInternal();
            BUSTUB_ENSURE(evicted_frame_id != std::nullopt, "No evictable page found");
        }
        alive_map_[frame_id] = std::make_shared<FrameStatus>(page_id, frame_id, false, ArcStatus::MRU);
        mru_.push_front(frame_id);
    }
}

/**
* TODO(P1): Add implementation
*
* @brief Toggle whether a frame is evictable or non-evictable. This function also
* controls replacer's size. Note that size is equal to number of evictable entries.
*
 * If a frame was previously evictable and is to be set to non-evictable, then size should
 * decrement. If a frame was previously non-evictable and is to be set to evictable,
 * then size should increment.
 *
 * If frame id is invalid, throw an exception or abort the process.
 *
 * For other scenarios, this function should terminate without modifying anything.
 *
 * @param frame_id id of frame whose 'evictable' status will be modified
 * @param set_evictable whether the given frame is evictable or not
 */
 void ArcReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
    std::scoped_lock<std::mutex> lock(latch_) ;  
    SetEvictableInternal(frame_id, set_evictable);
}
    
 void ArcReplacer::SetEvictableInternal(frame_id_t frame_id, bool set_evictable) {  
    auto it = alive_map_.find(frame_id) ;
    BUSTUB_ENSURE(it != alive_map_.end(), "invalid frame_id");
    bool old = alive_map_[frame_id]->evictable_;
    if (old == set_evictable) {
        return;
    }
    alive_map_[frame_id]->evictable_ = set_evictable;
    int delta = set_evictable ? 1 : -1;
    curr_size_ = curr_size_ + delta; 
   
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Remove an evictable frame from replacer.
 * This function should also decrement replacer's size if removal is successful.
 *
 * Note that this is different from evicting a frame, which always remove the frame
 * decided by the ARC algorithm.
 *
 * If Remove is called on a non-evictable frame, throw an exception or abort the
 * process.
 *
 * If specified frame is not found, directly return from this function.
 *
 * @param frame_id id of frame to be removed
 */
 void ArcReplacer::Remove(frame_id_t frame_id) {
    std::scoped_lock<std::mutex> lock(latch_) ;  
    if(alive_map_.find(frame_id) != alive_map_.end()){
        if(alive_map_[frame_id]->evictable_){
            if(alive_map_[frame_id]->arc_status_ == ArcStatus::MRU){
                frame_id_t evicted_frame_id = alive_map_[frame_id]->frame_id_;
                page_id_t evicted_page_id = alive_map_[frame_id]->page_id_;
                if(replacer_size_ > mru_target_size_ && mru_ghost_.size() == replacer_size_ - mru_target_size_){
                    page_id_t evicted_page_from_mru = mru_ghost_.back();
                    ghost_map_.erase(evicted_page_from_mru);
                    mru_ghost_.pop_back();
                }
                ghost_map_[evicted_page_id] = alive_map_[evicted_frame_id];
                ghost_map_[evicted_page_id]->arc_status_ = ArcStatus::MRU_GHOST;
                mru_ghost_.push_front(evicted_page_id);
                mru_.remove(evicted_frame_id);
                alive_map_.erase(evicted_frame_id);
                curr_size_ --;
                return ;
            }
            else{
                frame_id_t evicted_frame_id = alive_map_[frame_id]->frame_id_;
                page_id_t evicted_page_id = alive_map_[frame_id]->page_id_;
                if(mru_target_size_ > 0 && mfu_ghost_.size() == mru_target_size_){
                    page_id_t evicted_page_from_mfu = mfu_ghost_.back();
                    ghost_map_.erase(evicted_page_from_mfu);
                    mfu_ghost_.pop_back();
                }
                ghost_map_[evicted_page_id] = alive_map_[evicted_frame_id];
                ghost_map_[evicted_page_id]->arc_status_ = ArcStatus::MFU_GHOST;
                mfu_ghost_.push_front(evicted_page_id);
                mfu_.remove(evicted_frame_id);
                alive_map_.erase(evicted_frame_id);
                curr_size_--;
                return;
            }
        }
        BUSTUB_ENSURE(false, "No evictable page found");
    }
}
auto ArcReplacer::EvictInternal() -> std::optional<frame_id_t> {
    if (curr_size_ == 0) {
      return std::nullopt;
    }
  
    auto evict_from = [&](std::list<frame_id_t> &live_list,
                          std::list<page_id_t> &ghost_list,
                          size_t ghost_limit,
                          ArcStatus ghost_status) -> std::optional<frame_id_t> {
      for (auto it = live_list.rbegin(); it != live_list.rend(); ++it) {
        const frame_id_t evicted_fid = *it;
        if(alive_map_[evicted_fid]->evictable_ == false){
            continue;
        }
        const frame_id_t evicted_pid = alive_map_[evicted_fid]->page_id_;
        if (ghost_limit > 0 && ghost_list.size() == ghost_limit) {
          const page_id_t old = ghost_list.back();
          ghost_list.pop_back();
          ghost_map_.erase(old);
        }
  
        ghost_list.push_front(evicted_pid);
        
        ghost_map_[evicted_pid] = alive_map_[evicted_fid];
        ghost_map_[evicted_pid]->arc_status_ = ghost_status;
        live_list.remove(evicted_fid);
        alive_map_.erase(evicted_fid);
        --curr_size_;
        return evicted_fid;
      }
      return std::nullopt;
    };
  
    // Preferred side first, fallback to the other side.
    if (mru_.size() < mru_target_size_) {
        if (auto evicted_fid = evict_from(mfu_, mfu_ghost_, mru_target_size_, ArcStatus::MFU_GHOST);
        evicted_fid.has_value()) {
      return evicted_fid;
    }
    }
    
    
    return evict_from(mru_, mru_ghost_, replacer_size_ - mru_target_size_, ArcStatus::MRU_GHOST);
  }
  
  /**
  
   * @brief Return if the selected frame is in alive map.
   *
   * @return size_t
   */
  auto ArcReplacer::InAliveMap(frame_id_t frame_id) -> bool{
    std::scoped_lock<std::mutex> lock(latch_) ;  
    return alive_map_.find(frame_id)!=alive_map_.end();
}
/**
* TODO(P1): Add implementation
*
 * @brief Return replacer's size, which tracks the number of evictable frames.
 *
 * @return size_t
 */
auto ArcReplacer::Size() -> size_t { 
    std::scoped_lock<std::mutex> lock(latch_) ;  
    return curr_size_; 
}


}  // namespace bustub
