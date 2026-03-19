

#include <cstdio>
#include <queue>
#include <type_traits>

#define HARNESS_DECL_INPUT_CHANNEL(name, width, storage_t) \
  using name##_stored_t = storage_t; \
  static input_channel_fifo<name##_stored_t> name##_chan;

#define HARNESS_DECL_OUTPUT_CHANNEL(name, width, storage_t) \
  using name##_stored_t = storage_t; \
  static output_channel_fifo<name##_stored_t> name##_chan;

#define HARNESS_CLEAR_INPUT_CHANNEL(name, width, storage_t) \
  name##_chan.clear();

#define HARNESS_CLEAR_OUTPUT_CHANNEL(name, width, storage_t) \
  name##_chan.clear();

#define HARNESS_DEF_INPUT_CHANNEL(cls, name, width, storage_t) \
  input_channel_fifo<typename cls::name##_stored_t> cls::name##_chan;

#define HARNESS_DEF_OUTPUT_CHANNEL(cls, name, width, storage_t) \
  output_channel_fifo<typename cls::name##_stored_t> cls::name##_chan;

template <typename StoredT>
struct input_channel_fifo {
  std::queue<StoredT> q;
  std::vector<StoredT> q_assert;
  

  void clear() {
    q = {};
    q_assert = {};
  }

  void push(const StoredT& v) {
    q.push(v);
    q_assert.push_back(v);
  }

  bool empty() const {
    return q.empty();
  }

  StoredT pop_or_zero() {
    // if (q.empty()) {
    //   return None; 
    // }
    StoredT v = q.front();
    q.pop();
    return v;
  }
};

template <typename StoredT>
struct output_channel_fifo {
  std::queue<StoredT> q;
  std::vector<StoredT> q_assert;

  void clear() {
    q = {};
    q_assert = {};
  }

  void push(const StoredT& v) {
    q.push(v);
    q_assert.push_back(v);
  }

  bool empty() const {
    return q.empty();
  }

  bool pop(StoredT& v) {
    if (q.empty()) return false;
    v = q.front();
    q.pop();
    return true;
  }
};
