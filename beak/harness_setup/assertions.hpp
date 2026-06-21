#ifndef _ASSERTIONS_HPP
#define _ASSERTIONS_HPP


template <typename sim_t>
inline bool assert_fn(const sim_t& sim) {
return true; 
}


template <typename sim_t>
inline bool assert_final(const sim_t& sim) {

  return true;
}

#endif