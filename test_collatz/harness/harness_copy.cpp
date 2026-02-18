#include "../implementation/collatz.hpp"
#include <vector>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <fstream>
#include <string>
#include <limits>
#include <random>

struct extfuns {};
using simulator = module_collatz<extfuns>;
using snapshot_t = simulator::snapshot_t;
using state_t = simulator::state_t;


int main(int argc, char **argv) { // entry point of fuzzer
    if (argc != 2) return 1; 


    const char *input_path = argv[1]; // input harness file path 
    FILE *f = fopen(input_path, "rb"); // read in binary mode

    if (!f) {
        fprintf(stderr, "Error: unable to open %s\n", input_path);
        return 1;
    }

    std::vector<uint8_t> buf; 
    fseek(f, 0, SEEK_END); // move to end of file
    long file_size = ftell(f); // get file size
    fseek(f, 0, SEEK_SET); // move back to start of file

    if (file_size > 0){
        buf.resize(file_size); // resize buffer to fit file size
        fread(buf.data(), 1, file_size, f); // read file into buffer
    } else {
        fprintf(stderr, "Error: unable to read from %s\n", input_path);
        return 1;
    }
    fclose(f); // close file

    //  implementation under test specific code
    uint16_t r0 = 0;
    if (buf.size() != 2) {
        fprintf(stderr, "Error: input file must be exactly 2 bytes\n");
        return 1;
    }
    r0 = uint16_t(buf[0]) | (uint16_t(buf[1]) << 8); // convert 2 bytes to uint16_t, little endian
    
    uint64_t ncycles = 100; // default number of cycles for now
    
    state_t st = simulator::initial_state();
    st.r0 = prims::bits<16>::mk(r0); 

      simulator sim(st);

      auto init_snap = sim.snapshot();
      init_snap.state.dump(std::cout);


   // step one cycle at a time and print
    for (uint64_t i = 0; i < ncycles; ++i) {
#ifdef SIM_MINIMAL
        sim.run(1);  
        auto snap = sim.snapshot();                         // if run returns snapshot
        snap.state.dump(std::cout);  
#else
        sim.run(1);                                       // if run doesn't return snapshot
        auto snap = sim.snapshot();                          // if run returns snapshot
        snap.state.dump(std::cout); 
#endif
    }


    return 0;

}
