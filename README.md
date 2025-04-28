This is a simple generic C11 hash [stb](https://github.com/nothings/stb) style single header library.

I have come across quite a lot of these and they are usually very complicated or not very performant, this tries to be as simple as possible whilst still remaining fairly performant.

These are some performance metrics which should of course always be taken with a grain of salt, and another test tried to be made as similar as possible using [abseil's](https://github.com/abseil/abseil-cpp) flat_hash_map implementation.

This:
Average Insert time per op: 975.050000 ns
Average Lookup time per op: 693.130000 ns
Average Iterate time per op: 606.040000 ns
Average Delete time per op: 665.710000 ns
Memory usage:
  SwissMap struct:       56 bytes
  control array:         262152 bytes
  keys array:            2097152 bytes
  vals array:            4194304 bytes
  total:                 6553664 bytes
Overhead (struct+ctrl): 4.00% of total

Abseil:
Insert avg: 2559.48 ns
Lookup avg: 575.484 ns
Iterate avg: 73.6445 ns
Delete avg: 812.926 ns

To see an example of how to use the library look at the profiling.c file.
