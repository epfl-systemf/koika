/*! Default driver for Kôika programs compiled to C++ using Cuttlesim !*/
#include "collatz.hpp"

struct extfuns {};
class simulator final : public module_collatz<extfuns> {};

#ifdef SIM_MINIMAL
template simulator::snapshot_t cuttlesim::init_and_run<simulator>(unsigned long long int);
#else
int main(int argc, char **argv) { return cuttlesim::main<simulator>(argc, argv); }
#endif
