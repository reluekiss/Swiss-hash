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
| boost | 1353.78 | 1009.30 |
| ska | 593.02 | 212.14 |
| swiss | 1609.57 | 1615.63 |

### Operation: Lookup

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 974.41 | 215.79 |
| ska | 855.14 | 172.57 |
| swiss | 692.37 | 145.65 |

### Operation: Delete

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 921.31 | 315.95 |
| ska | 792.62 | 203.88 |
| swiss | 560.28 | 194.91 |

## Group `8`: 8 byte key / 8 byte value

### Operation: Insert

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 107.52 | 81.78 |
| ska | 116.47 | 50.67 |
| swiss | 74.89 | 49.82 |

### Operation: Lookup

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 298.75 | 122.53 |
| ska | 165.73 | 78.75 |
| swiss | 180.59 | 86.12 |

### Operation: Delete

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 275.25 | 132.49 |
| ska | 175.30 | 80.35 |
| swiss | 133.53 | 84.00 |

## Group `r`: random 1024 byte key / 1032 byte value

### Operation: Insert

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 151.50 | 5.56 |
| ska | 148.85 | 5.69 |
| swiss | 93.87 | 8.04 |

### Operation: Lookup

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 595.95 | 135.01 |
| ska | 706.69 | 140.20 |
| swiss | 1760.33 | 1180.95 |

### Operation: Delete

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 597.83 | 140.18 |
| ska | 708.62 | 139.55 |
| swiss | 427.55 | 112.18 |

## Group `r8`: random 8 byte key / 8 byte value

### Operation: Insert

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 22.01 | 3.98 |
| ska | 21.22 | 3.23 |
| swiss | 30.55 | 3.06 |

### Operation: Lookup

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 199.13 | 86.49 |
| ska | 226.83 | 75.56 |
| swiss | 273.42 | 88.97 |

### Operation: Delete

| Impl | Mean (ns) | Std (ns) |
| --- | --- | --- |
| boost | 199.90 | 87.20 |
| ska | 227.91 | 77.26 |
| swiss | 147.15 | 72.73 |
