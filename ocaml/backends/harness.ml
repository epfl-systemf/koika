(*! Harness backend !*)

open Common 
open Cpp


let write_harness_cpp (target_dpath : string) (modname : string) : unit =
  let fpath = Filename.concat target_dpath "harness.cpp" in
  let buf = Buffer.create 256 in
  Buffer.add_string buf (Printf.sprintf "// harness for module: %s\n" modname);
  with_output_to_file fpath Buffer.output_buffer buf

let main target_dpath (modname : string) (cpp_out : cpp_output_t) =
  write_harness_cpp target_dpath modname