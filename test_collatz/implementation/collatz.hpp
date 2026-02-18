#ifndef MODULE_COLLATZ_HPP
#define MODULE_COLLATZ_HPP

//////////////
// PREAMBLE //
//////////////

#include "cuttlesim.hpp"

//////////////
// TYPEDEFS //
//////////////




namespace prims {
#ifndef SIM_MINIMAL
#endif

}

////////////////////
// IMPLEMENTATION //
////////////////////

template <typename extfuns_t> class module_collatz {
public:
struct state_t {
bits<16> r0;

#ifndef SIM_MINIMAL
void dump(std::ostream& os = std::cout) const {
os << "r0 = " << r0 << std::endl;
}

static _unused void vcd_header(std::ostream& os) {
#ifndef SIM_VCD_SCOPES
#define SIM_VCD_SCOPES { "TOP", "collatz" }
#endif
cuttlesim::vcd::begin_header(os, SIM_VCD_SCOPES);
cuttlesim::vcd::var(os, "r0", 16);
cuttlesim::vcd::end_header(os, SIM_VCD_SCOPES);
}

void vcd_dumpvars(std::uint_fast64_t cycle_id, _unused std::ostream& os, const state_t& previous, const bool force) const {
os << '#' << cycle_id << std::endl;
cuttlesim::vcd::dumpvar(os, "r0", r0, previous.r0, force);
}

std::uint_fast64_t vcd_readvars(_unused std::istream& is) {
std::string var{}, val{};
std::uint_fast64_t cycle_id = std::numeric_limits<std::uint_fast64_t>::max();
cuttlesim::vcd::read_header(is);
while (cuttlesim::vcd::readvar(is, cycle_id, var, val)) {
if (var == "r0") {
r0 = prims::unpack<bits<16>>(bits<16>::of_str(val));
}
}
return cycle_id;
}
#endif
};

using snapshot_t = cuttlesim::snapshot_t<state_t>;

protected:
struct rwset_t {
};

struct log_t {
rwset_t rwset;
state_t state;

const state_t& snapshot() const {
return state;
}
explicit log_t(const state_t& init) : rwset{}, state(init) {
}
};

log_t log;
log_t Log;
extfuns_t extfuns;
cuttlesim::sim_metadata meta;

#ifndef SIM_MINIMAL
std::default_random_engine rng{};
std::uniform_int_distribution<int> uniform{0, 1};
#endif

#define RULE_NAME divide
DEF_RESET(divide) {
log.state = Log.state;
}

DEF_COMMIT(divide) {
Log.state = log.state;
}

DEF_RULE(divide) {
bits<16> v = READ0_FAST(r0);
bits<1> odd = v[4'0000_b];
if (~(odd)) {
WRITE0_FAST(r0, (v >> 1'1_b));
}
else {
FAIL_FAST();
}

COMMIT();
}
#undef RULE_NAME

#define RULE_NAME multiply
DEF_RESET(multiply) {
log.state = Log.state;
}

DEF_COMMIT(multiply) {
Log.state = log.state;
}

DECL_FN(times_three_0, bits<16>)
DEF_FN(times_three_0, bits<16> &_ret, bits<16> bs) {
_ret = ((bs << 1'1_b) + bs);
return true;
}
DEF_RULE(multiply) {
bits<16> v = READ1_FAST(r0);
bits<1> odd = v[4'0000_b];
if (odd) {
bits<16> _v0 = CALL_FN(times_three_0, v);
WRITE1_FAST(r0, (_v0 + 16'1_b));
}
else {
FAIL_FAST();
}

COMMIT();
}
#undef RULE_NAME

public:
void finish(cuttlesim::exit_info exit_config, int exit_code) {
meta.finished = true;
meta.exit_config = exit_config;
meta.exit_code = exit_code;
}

bool finished() {
return meta.finished;
}

snapshot_t snapshot() const {
return snapshot_t(Log.snapshot(), meta);
}

static constexpr state_t initial_state() {
state_t init {
.r0 = 0x16'12_x,
};
return init;
}

explicit module_collatz(const state_t init = initial_state()) : log(init), Log(init), extfuns{}, meta{} {
#ifndef SIM_MINIMAL
rng.seed(cuttlesim::random_seed);
#endif
}

_virtual void strobe() const {}

void cycle() {
meta.cycle_id++;
log.rwset = Log.rwset = rwset_t{};
rule_divide();
rule_multiply();
strobe();
}

_flatten module_collatz& run(std::uint_fast64_t ncycles) {
for (std::uint_fast64_t cycle_id = 0;
                cycle_id < ncycles && !meta.finished;
                cycle_id++) {
cycle();
}
return *this;
}

#ifndef SIM_MINIMAL
typedef bool (module_collatz::*rule_ptr)();
void cycle_randomized() {
meta.cycle_id++;
log.rwset = Log.rwset = rwset_t{};
static constexpr rule_ptr rules[2] {
&module_collatz::rule_divide,
&module_collatz::rule_multiply};
(this->*rules[uniform(rng)])();
strobe();
}

_flatten module_collatz& run_randomized(std::uint_fast64_t ncycles) {
for (std::uint_fast64_t cycle_id = 0;
                cycle_id < ncycles && !meta.finished;
                cycle_id++) {
cycle_randomized();
}
return *this;
}

_flatten module_collatz& trace(std::string fname, std::uint_fast64_t ncycles) {
std::ofstream vcd(fname);
state_t::vcd_header(vcd);
state_t latest = Log.snapshot();
latest.vcd_dumpvars(meta.cycle_id, vcd, latest, true);
for (std::uint_fast64_t cycle_id = 0;
                cycle_id < ncycles && !meta.finished;
                cycle_id++) {
cycle();
state_t current = Log.snapshot();
current.vcd_dumpvars(meta.cycle_id, vcd, latest, false);
latest = current;
}
return *this;
}

_flatten module_collatz& trace_randomized(std::string fname, std::uint_fast64_t ncycles) {
std::ofstream vcd(fname);
state_t::vcd_header(vcd);
state_t latest = Log.snapshot();
latest.vcd_dumpvars(meta.cycle_id, vcd, latest, true);
for (std::uint_fast64_t cycle_id = 0;
                cycle_id < ncycles && !meta.finished;
                cycle_id++) {
cycle_randomized();
state_t current = Log.snapshot();
current.vcd_dumpvars(meta.cycle_id, vcd, latest, false);
latest = current;
}
return *this;
}
#endif
};

#endif
