// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include <rocksdb/compaction_filter.h>
#include <rocksdb/db.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/options.h>

class SplayMerge : public rocksdb::MergeOperator {
public:
    virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                             MergeOperationOutput* merge_out) const override {
        merge_out->new_value.clear();
        if (merge_in.existing_value != nullptr) {
            merge_out->new_value.assign(merge_in.existing_value->data(),
                                        merge_in.existing_value->size());
        }
        for (const rocksdb::Slice& m : merge_in.operand_list) {
            merge_out->new_value.assign(m.data(), m.size());
        }
        return true;
    }

    const char* Name() const override { return "SplayMerge"; }
};

class SplayFilter : public rocksdb::CompactionFilter {
public:
    bool Filter(int level, const rocksdb::Slice& key,
                const rocksdb::Slice& existing_value, std::string* new_value,
                bool* value_changed) const override {
        // return true to remove, false to keep
        const SplayTag* prev_tag =
            reinterpret_cast<const SplayTag*>(existing_value.data());
        if (prev_tag->splayed) {
            // always delete splayed items
            return true;
        }
        if (!prev_tag->merged) {
            // update merged tag
            *new_value =
                std::string(existing_value.data(), existing_value.size());
            SplayTag* tag = reinterpret_cast<SplayTag*>(&new_value[0]);
            tag->merged = 1;
            *value_changed = true;
        }
        return false;
    }

    bool FilterMergeOperand(int level, const rocksdb::Slice& key,
                            const rocksdb::Slice& existing_value) const override {
        assert(false && "FilterMergeOperand used");
        return false;
    }

    const char* Name() const override { return "SplayFilter"; }
};
