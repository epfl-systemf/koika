#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#define BEAK_MODE_INIT            1
#define BEAK_MODE_INPUTS          2
#define BEAK_MODE_INIT_AND_INPUTS 3

#ifndef BEAK_FUZZ_MODE
#error "BEAK_FUZZ_MODE must be defined: BEAK_MODE_INIT, BEAK_MODE_INPUTS, or BEAK_MODE_INIT_AND_INPUTS"
#endif

#if BEAK_FUZZ_MODE != BEAK_MODE_INIT && \
    BEAK_FUZZ_MODE != BEAK_MODE_INPUTS && \
    BEAK_FUZZ_MODE != BEAK_MODE_INIT_AND_INPUTS
#error "Invalid BEAK_FUZZ_MODE"
#endif

#ifndef BEAK_MAX_CYCLES
#define BEAK_MAX_CYCLES 0
#endif

namespace beak_mode {

struct plan {
  std::size_t cycles = 0;
  std::size_t used_bytes = 0;
};

template <std::size_t InitBytes, std::size_t InputBytesPerCycle>
[[nodiscard]] inline bool prepare_input(std::vector<std::uint8_t>& buf,
                                        plan& p) {
  p = {};

#if BEAK_FUZZ_MODE == BEAK_MODE_INIT

  static_assert(
      InitBytes > 0,
      "BEAK_MODE_INIT selected, but no registers/configuration bytes are visible."
  );

  if (buf.size() < InitBytes) {
    return false;
  }

  // Init is read once from the first InitBytes bytes.
  // The total input length still controls how many cycles are simulated.
  p.cycles = buf.size() / InitBytes;

#elif BEAK_FUZZ_MODE == BEAK_MODE_INPUTS

  static_assert(
      InputBytesPerCycle > 0,
      "BEAK_MODE_INPUTS selected, but no input bytes are visible."
  );

  if (buf.size() < InputBytesPerCycle) {
    return false;
  }

  p.cycles = buf.size() / InputBytesPerCycle;

#elif BEAK_FUZZ_MODE == BEAK_MODE_INIT_AND_INPUTS

  static_assert(
      InitBytes > 0,
      "BEAK_MODE_INIT_AND_INPUTS selected, but no registers/configuration bytes are visible."
  );

  static_assert(
      InputBytesPerCycle > 0,
      "BEAK_MODE_INIT_AND_INPUTS selected, but no input bytes are visible."
  );

  if (buf.size() < InitBytes + InputBytesPerCycle) {
    return false;
  }

  // Layout:
  // [ init bytes ][ cycle 0 input bytes ][ cycle 1 input bytes ] ...
  p.cycles = (buf.size() - InitBytes) / InputBytesPerCycle;

#endif

  if (p.cycles == 0) {
    return false;
  }

#if BEAK_MAX_CYCLES > 0
  if (p.cycles > BEAK_MAX_CYCLES) {
    p.cycles = BEAK_MAX_CYCLES;
  }
#endif

#if BEAK_FUZZ_MODE == BEAK_MODE_INIT
  p.used_bytes = p.cycles * InitBytes;
#elif BEAK_FUZZ_MODE == BEAK_MODE_INPUTS
  p.used_bytes = p.cycles * InputBytesPerCycle;
#elif BEAK_FUZZ_MODE == BEAK_MODE_INIT_AND_INPUTS
  p.used_bytes = InitBytes + p.cycles * InputBytesPerCycle;
#endif

  buf.resize(p.used_bytes);
  return true;
}

} // namespace beak_mode

#if BEAK_FUZZ_MODE == BEAK_MODE_INIT

#define BEAK_APPLY_FUZZ_MODE(NS, STATE, BUF, PLAN) \
  do {                                             \
    NS::set_init((STATE), (BUF));                  \
  } while (0)

#elif BEAK_FUZZ_MODE == BEAK_MODE_INPUTS

#define BEAK_APPLY_FUZZ_MODE(NS, STATE, BUF, PLAN) \
  do {                                             \
    NS::set_inputs((BUF), (PLAN).cycles, 0);          \
  } while (0)

#elif BEAK_FUZZ_MODE == BEAK_MODE_INIT_AND_INPUTS

#define BEAK_APPLY_FUZZ_MODE(NS, STATE, BUF, PLAN) \
  do {                                             \
    NS::set_init_and_inputs((STATE), (BUF), (PLAN).cycles, InitBytes); \
  } while (0)

#endif