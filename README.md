This is a simple generic C11 hash library.

I have come across quite a lot of these and they are usually very complicated or not very performant, this tries to be as simple as possible whilst still remaining fairly performant.


To see an example of how to use the library look at the profiling.c file.

These are some performance metrics which should of course always be taken with a grain of salt

I did my testing with reserved maps, but you should know that spiking behaviour on rehashing is true for all hashmaps and impacts insertion values by about ~20%. So do the mental maths on that, I just don't like waiting.

This is the ouput of running bench, the plotting and such uses numpy, pandas and matplotlib which are fairly standard.

These results were gotten on a machine using a AMD Ryzen 5 5600G and 40GB DDR4 3200 MHz. You will need about 24 GB because of the preprocessing on the large key values for ska takes up a lot of ram.

## Group `nosuffix`: 1024 byte key / 1032 byte value

### Operation: Insert

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 1310.38 | 1010.18 |
| ska | 601.63 | 225.89 |
| swiss | 1576.61 | 1600.70 |

### Operation: Lookup

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 993.43 | 214.97 |
| ska | 853.60 | 175.93 |
| swiss | 614.00 | 130.79 |

### Operation: Delete

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 968.98 | 328.13 |
| ska | 791.43 | 207.18 |
| swiss | 500.36 | 183.18 |

## Group `8`: 8 byte key / 8 byte value

### Operation: Insert

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 111.31 | 82.53 |
| ska | 124.15 | 60.22 |
| swiss | 71.12 | 46.52 |

### Operation: Lookup

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 298.09 | 125.05 |
| ska | 165.45 | 76.53 |
| swiss | 178.81 | 85.91 |

### Operation: Delete

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 275.53 | 135.87 |
| ska | 176.67 | 79.73 |
| swiss | 144.70 | 91.90 |

## Group `r`: random 1024 byte key / 1032 byte value

### Operation: Insert

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 151.24 | 5.24 |
| ska | 149.04 | 6.05 |
| swiss | 90.33 | 4.32 |

### Operation: Lookup

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 607.47 | 139.04 |
| ska | 702.63 | 143.09 |
| swiss | 376.48 | 107.42 |

### Operation: Delete

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 606.22 | 138.47 |
| ska | 705.16 | 143.53 |
| swiss | 372.73 | 107.50 |

## Group `r8`: random 8 byte key / 8 byte value

### Operation: Insert

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 21.98 | 3.97 |
| ska | 21.24 | 3.25 |
| swiss | 30.06 | 3.46 |

### Operation: Lookup

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 200.37 | 84.16 |
| ska | 231.44 | 76.81 |
| swiss | 133.00 | 67.56 |

### Operation: Delete

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 199.59 | 79.86 |
| ska | 232.35 | 79.31 |
| swiss | 128.70 | 64.24 |
