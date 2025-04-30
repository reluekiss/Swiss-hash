This is a simple generic C11 hash [stb](https://github.com/nothings/stb) style single header library.

I have come across quite a lot of these and they are usually very complicated or not very performant, this tries to be as simple as possible whilst still remaining fairly performant.

These are some performance metrics which should of course always be taken with a grain of salt

To see an example of how to use the library look at the profiling.c file.

Metric          | Value
----------------|----------------
Keys/Values     | 100,000
SwissMap struct | 56 bytes
control array   | 131,080 bytes
keys array      | 1,048,576 bytes
vals array      | 2,097,152 bytes
total           | 3,276,864 bytes
Overhead (struct+ctrl) | 4.00 %

| Operation | SwissMap (ns) | [Abseil](https://github.com/abseil/abseil-cpp) (ns) | [Boost](https://github.com/boostorg/boost) (ns) | [stb_ds](https://github.com/nothings/stb/blob/master/stb_ds.h) (ns) | [CC](https://github.com/JacksonAllan/CC) (ns) | [ska_flat_map](https://github.com/skarupke/flat_hash_map/blob/master/flat_hash_map.hpp) (ns) |
|---------|--------|---------|---------|-----------|------------|---------|
| Insert  | 876.86 | 2559.48 | 423.893 | 976.69    | 957.21     | 445.834 |
| Lookup  | 663.13 | 575.484 | 91.8559 | 821.06    | 720.19     | 64.223  |
| Iterate | 589.76 | 73.6445 | 30.3343 | 27,985.10 | 363,763.89 | 9.54452 |
| Delete  | 643.72 | 812.926 | 146.817 | 1,030.30  | 778.00     | 71.8897 |


For stb and CC I think I may have done the iteration tests wrong but I can't seem to verify how/why.
