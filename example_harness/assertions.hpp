#ifndef _ASSERTIONS_HPP
#define _ASSERTIONS_HPP


template <typename snapshot_t>
inline bool assert_fn(const snapshot_t& snap) {
  return true;
}

template <typename snapshot_t>
inline bool assert_final(const snapshot_t& snap) {
   return true; 
}


#endif