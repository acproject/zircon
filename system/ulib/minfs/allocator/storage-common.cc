// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "allocator.h"
#include "storage.h"

namespace minfs {

namespace {

blk_t BitmapBlocksForSizeImpl(size_t size) {
  return (static_cast<blk_t>(size) + kMinfsBlockBits - 1) / kMinfsBlockBits;
}

}  // namespace

uint32_t AllocatorStorage::PoolBlocks() const { return BitmapBlocksForSizeImpl(PoolTotal()); }

void PersistentStorage::Load(fs::ReadTxn* read_transaction, ReadData data) {
  read_transaction->Enqueue(data, 0, metadata_.MetadataStartBlock(), PoolBlocks());
}

void PersistentStorage::PersistRange(PendingWork* transaction, WriteData data, size_t index,
                                     size_t count) {
  ZX_DEBUG_ASSERT(transaction != nullptr);
  // Determine the blocks containing the first and last indices.
  blk_t first_rel_block = static_cast<blk_t>(index / kMinfsBlockBits);
  blk_t last_rel_block = static_cast<blk_t>((index + count - 1) / kMinfsBlockBits);

  // Calculate number of blocks based on the first and last blocks touched.
  blk_t block_count = last_rel_block - first_rel_block + 1;
  blk_t abs_block = metadata_.MetadataStartBlock() + first_rel_block;

  fs::Operation op = {
    .type = fs::OperationType::kWrite,
    .vmo_offset = first_rel_block,
    .dev_offset = abs_block,
    .length = block_count,
  };

  transaction->EnqueueMetadata(data, std::move(op));
}

void PersistentStorage::PersistAllocate(PendingWork* write_transaction, size_t count) {
  ZX_DEBUG_ASSERT(write_transaction != nullptr);
  metadata_.PoolAllocate(static_cast<blk_t>(count));
  sb_->Write(write_transaction, UpdateBackupSuperblock::kUpdate);
}

void PersistentStorage::PersistRelease(PendingWork* write_transaction, size_t count) {
  ZX_DEBUG_ASSERT(write_transaction != nullptr);
  metadata_.PoolRelease(static_cast<blk_t>(count));
  sb_->Write(write_transaction, UpdateBackupSuperblock::kUpdate);
}

// Static.
blk_t PersistentStorage::BitmapBlocksForSize(size_t size) { return BitmapBlocksForSizeImpl(size); }

}  // namespace minfs
