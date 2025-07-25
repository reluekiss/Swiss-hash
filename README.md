This is a simple generic C11 hash library.

I have come across quite a lot of these and they are usually very complicated or not very performant, this tries to be as simple as possible whilst still remaining fairly performant.

These are some performance metrics which should of course always be taken with a grain of salt

To see an example of how to use the library look at the profiling.c file.

I did my testing with reserved maps, but you should know that spiking behaviour on rehashing is true for all hashmaps and impacts insertion values by about ~20%. So do the mental maths on that, I just don't like waiting.

1024 byte key, 1024 + 8 byte value, with reserved maps. 1,000,000 entries
| Operation | swissmap (ns/op) | [Boost](https://github.com/boostorg/boost) (ns/op) | [ska_flat_map](https://github.com/skarupke/flat_hash_map/blob/master/flat_hash_map.hpp) (ns/op) |
|---------|--------:|---------:|---------:|
| Insert  | 3850.94 | 1437.51 | 563.905 |
| Lookup  | 374.92 | 852.56 | 723.097 |
| Iterate | 10.63783e+07 | 2.05731e+07 | 2.09321e+07 |
| Delete  | 337.08 | 750.926 | 600.135 |

8 byte int key, 8 byte int value, with reserved maps. 1,000,000 entries
| Operation | swissmap (ns/op) | [Boost](https://github.com/boostorg/boost) (ns/op) | [ska_flat_map](https://github.com/skarupke/flat_hash_map/blob/master/flat_hash_map.hpp) (ns/op) |
|-----------|--------:|---------:|---------:|
| Insert    | 126.19  |  107.86   | 38.54    |
| Lookup    | 82.09   |  42.97    |26.29    |
| Iterate   | 5.128352e+06 | 5.83025e+06 | 1.69832e+07 |
| Delete    | 56.58   |  144.45   |25.66    |

Memory Footprint (1 000 000 entries)
All numbers in MiB (1 MiB = 1024 KiB)

| Map                  | Total (MiB) | Used (MiB) | Overhead (MiB) | Overhead % |
|----------------------|------------:|-----------:|---------------:|-----------:|
| swissmap             |      4 106  |     720    |        3 386   |     82.5%  |
| boost::unordered_map |      3 082  |     720    |        2 362   |     76.6%  |
| ska::flat_hash_map   |      4 106  |     720    |        3 386   |     82.4%  |

| Map                  | Total (MiB) | Used (MiB) | Overhead (MiB) | Overhead % |
|----------------------|------------:|-----------:|---------------:|-----------:|
| SwissMap             |         34  |     5.6    |         28.4   |     83.5%  |
| boost::unordered_map |       25.5  |     5.6    |         19.9   |     78.0%  |
| ska::flat_hash_map   |         34  |     5.6    |         28.4   |     83.5%  |
