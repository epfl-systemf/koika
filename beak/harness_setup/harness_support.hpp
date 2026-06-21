

#include <cstdio>
#include <queue>
#include <type_traits>

#define HARNESS_DECL_INPUT_CHANNEL(name, arg_width, ret_width) \
  using name##_stored_t = ret_width; \
  static input_channel_fifo<name##_stored_t> name##_chan;

#define HARNESS_DECL_OUTPUT_CHANNEL(name, arg_width, ret_width) \
  using name##_stored_t = arg_width; \
  static output_channel_fifo<name##_stored_t> name##_chan;

#define HARNESS_CLEAR_INPUT_CHANNEL(name, arg_width, ret_width) \
  name##_chan.clear();

#define HARNESS_CLEAR_OUTPUT_CHANNEL(name, arg_width, ret_width) \
  name##_chan.clear();

#define HARNESS_DEF_INPUT_CHANNEL(cls, name, arg_width, ret_width) \
  input_channel_fifo<typename cls::name##_stored_t> cls::name##_chan;

#define HARNESS_DEF_OUTPUT_CHANNEL(cls, name, arg_width, ret_width) \
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
    if (q.empty()) {
      return StoredT{}; 
    }
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
