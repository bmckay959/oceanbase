/**
 * Copyright (c) 2023 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */
#ifndef STORAGE_MULTI_DATA_SOURCE_MDS_UNIT_H
#define STORAGE_MULTI_DATA_SOURCE_MDS_UNIT_H

#include "lib/lock/ob_small_spin_lock.h"
#include "lib/ob_errno.h"
#include "lib/utility/utility.h"
#include "mds_row.h"
#include "share/scn.h"
#include "storage/multi_data_source/mds_node.h"
#include "storage/multi_data_source/runtime_utility/list_helper.h"
#include "storage/multi_data_source/runtime_utility/common_define.h"
#include "storage/multi_data_source/compile_utility/mds_dummy_key.h"
#include "storage/multi_data_source/runtime_utility/mds_factory.h"
#include <type_traits>
#include <utility>
#include "mds_table_base.h"

namespace oceanbase
{
namespace storage
{
namespace mds
{
template <typename K, typename V>
using Row = MdsRow<K, V>;
template <typename K, typename V>
struct KvPair : public ListNode<KvPair<K, V>>
{
  KvPair() = default;
  bool operator<(const KvPair &rhs) const { return k_ < rhs.k_; }
  bool operator==(const KvPair &rhs) const { return k_ == rhs.k_; }
  TO_STRING_KV(K_(k), K_(v));
  K k_;
  V v_;
};
// multi row defination
template <typename K, typename V>
class MdsUnit final : public MdsUnitBase<K, V>
{
public:
  typedef K key_type;
  typedef V value_type;
public:// iterator defination
  template <typename ValueType>
  struct IteratorBase;
  template <typename ValueType>
  struct NormalIterator;
  template <typename ValueType>
  struct ReverseIterator;
  using iterator = NormalIterator<KvPair<K, Row<K, V>>>;
  using const_iterator = NormalIterator<const KvPair<K, Row<K, V>>>;
  using reverse_iterator = ReverseIterator<KvPair<K, Row<K, V>>>;
  using const_reverse_iterator = ReverseIterator<const KvPair<K, Row<K, V>>>;
  iterator begin();
  iterator end();
  const_iterator cbegin();
  const_iterator cend();
  reverse_iterator rbegin();
  reverse_iterator rend();
  const_reverse_iterator crbegin();
  const_reverse_iterator crend();
public:
  MdsUnit();
  ~MdsUnit();
public:
  template <typename Value>
  int set(MdsTableBase *p_mds_table,
          const K &key,
          Value &&value,
          MdsCtx &ctx,
          const int64_t lock_timeout_us,
          const bool is_for_remove = false);
  template <typename Value>
  int replay(MdsTableBase *p_mds_table,
             const K &key,
             Value &&value,
             MdsCtx &ctx,
             const share::SCN scn,
             const bool is_for_remove = false);
  template <typename OP>
  int get_snapshot(const K &key,
                   OP &&read_op,
                   const share::SCN snapshot,
                   const int64_t read_seq,
                   const int64_t timeout_us) const;
  template <typename OP>
  int get_by_writer(const K &key,
                    OP &&op,
                    const MdsWriter &writer,
                    const share::SCN snapshot,
                    const int64_t read_seq,
                    const int64_t timeout_us) const;
  template <typename OP>
  int get_latest(const K &key, OP &&op, const int64_t read_seq) const;
  template <typename DUMP_OP>
  int scan_KV_row(DUMP_OP &&op,
                  share::SCN &flush_scn,
                  const uint8_t mds_table_id,
                  const uint8_t mds_unit_id,
                  const bool for_flush) const;
  template <typename OP>
  int for_each_node_on_row(OP &&op) const;
  template <typename OP>
  int for_each_row(OP &&op);
  void lock() const { lock_.wrlock(); }
  void unlock() const { lock_.unlock(); }
  int fill_virtual_info(ObIArray<MdsNodeInfoForVirtualTable> &mds_node_info_array, const int64_t unit_id) const;
  TO_STRING_KV(K_(multi_row_list));
public:
  template <int N>
  void report_event_(const char (&event_str)[N],
                     const K &key,
                     const char *file = __builtin_FILE(),
                     const uint32_t line = __builtin_LINE(),
                     const char *function_name = __builtin_FUNCTION()) const;
  const Row<K, V> *get_row_from_list_(const K &key) const;
  int insert_empty_kv_to_list_(const K &key, Row<K, V> *&row, MdsTableBase *p_mds_table);
  SortedList<KvPair<K, Row<K, V>>, SORT_TYPE::ASC> multi_row_list_;
  mutable MdsLock lock_;
};

// signal row defination
template <typename V>
class MdsUnit<DummyKey, V> final : public MdsUnitBase<DummyKey, V>
{
public:
  typedef DummyKey key_type;
  typedef V value_type;
public:// iterator defination
  template <typename ValueType>
  struct IteratorBase;
  template <typename ValueType>
  struct NormalIterator;
  template <typename ValueType>
  struct ReverseIterator;
  using iterator = NormalIterator<KvPair<DummyKey, Row<DummyKey, V>>>;
  using const_iterator = NormalIterator<const KvPair<DummyKey, Row<DummyKey, V>>>;
  using reverse_iterator = ReverseIterator<KvPair<DummyKey, Row<DummyKey, V>>>;
  using const_reverse_iterator = ReverseIterator<const KvPair<DummyKey, Row<DummyKey, V>>>;
  iterator begin();
  iterator end();
  const_iterator cbegin();
  const_iterator cend();
  reverse_iterator rbegin();
  reverse_iterator rend();
  const_reverse_iterator crbegin();
  const_reverse_iterator crend();
public:
  MdsUnit();
  template <typename Value>
  int set(MdsTableBase *p_mds_table, Value &&value, MdsCtx &ctx, const int64_t lock_timeout_us);
  template <typename Value>
  int replay(MdsTableBase *p_mds_table, Value &&value, MdsCtx &ctx, const share::SCN scn);
  template <typename OP>
  int get_snapshot(OP &&read_op,
                   const share::SCN snapshot,
                   const int64_t read_seq,
                   const int64_t timeout_us) const;
  template <typename OP>
  int get_by_writer(OP &&op,
                    const MdsWriter &writer,
                    const share::SCN snapshot,
                    const int64_t read_seq,
                    const int64_t timeout_us) const;
  template <typename OP>
  int get_latest(OP &&op, const int64_t read_seqs) const;
  template <typename DUMP_OP>
  int scan_KV_row(DUMP_OP &&op,
                  share::SCN &flush_scn,
                  const uint8_t mds_table_id,
                  const uint8_t mds_unit_id,
                  const bool for_flush) const;
  template <typename OP>
  int for_each_node_on_row(OP &&op) const;
  template <typename OP>
  int for_each_row(OP &&op) const;
  void lock() const { lock_.wrlock(); }
  void unlock() const { lock_.unlock(); }
  int fill_virtual_info(ObIArray<MdsNodeInfoForVirtualTable> &mds_node_info_array, const int64_t unit_id) const;
  TO_STRING_KV(K_(single_row));
public:
  KvPair<DummyKey, Row<DummyKey, V>> single_row_;
  mutable MdsLock lock_;
};
}
}
}

#ifndef STORAGE_MULTI_DATA_SOURCE_MDS_UNIT_H_IPP
#define STORAGE_MULTI_DATA_SOURCE_MDS_UNIT_H_IPP
#include "mds_unit.ipp"
#endif

#endif