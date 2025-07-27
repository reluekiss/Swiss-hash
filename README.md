This is a simple generic C11 hash library.

I have come across quite a lot of these and they are usually very complicated or not very performant, this tries to be as simple as possible whilst still remaining fairly performant.


To see an example of how to use the library look at the profiling.c file.

These are some performance metrics which should of course always be taken with a grain of salt

I did my testing with reserved maps, but you should know that spiking behaviour on rehashing is true for all hashmaps and impacts insertion values by about ~20%. So do the mental maths on that, I just don't like waiting.

This is the ouput of running bench, the plotting and such uses numpy, pandas and matplotlib which are fairly standard.

These results were gotten on a machine using a AMD Ryzen 5 5600G and 40GB DDR4 3200 MHz. You will need about 24 GB because of the preprocessing on the large key values for ska takes up a lot of ram.

Group 'nosuffix': 1024 byte key / 1032 byte value

  Operation: Insert
 Impl Mean(ns) Std(ns)
boost  1301.52 1011.63
  ska   590.76  210.96
swiss  1609.49 1636.63

  Operation: Lookup
 Impl Mean(ns) Std(ns)
boost   985.47  215.92
  ska   847.72  172.73
swiss   625.94  130.01

  Operation: Delete
 Impl Mean(ns) Std(ns)
boost   963.82  327.74
  ska   789.40  203.34
swiss   490.18  177.60

Group '8': 8 byte key / 8 byte value

  Operation: Insert
 Impl Mean(ns) Std(ns)
boost   111.44   86.27
  ska   115.70   53.43
swiss    75.67   49.46

  Operation: Lookup
 Impl Mean(ns) Std(ns)
boost   294.25  127.55
  ska   159.68   72.98
swiss   179.91   88.41

  Operation: Delete
 Impl Mean(ns) Std(ns)
boost   272.18  137.35
  ska   172.90   84.18
swiss   141.14   87.33

Group 'r': random 1024 byte key / 1032 byte value

  Operation: Insert
 Impl Mean(ns) Std(ns)
boost   151.55    4.88
  ska   148.60    6.07
swiss    94.33    8.64

  Operation: Lookup
 Impl Mean(ns) Std(ns)
boost   605.40  139.28
  ska   706.64  137.93
swiss  1768.38 1166.62

  Operation: Delete
 Impl Mean(ns) Std(ns)
boost   604.93  140.86
  ska   711.49  141.94
swiss   434.26  117.39

Group 'r8': random 8 byte key / 8 byte value

  Operation: Insert
 Impl Mean(ns) Std(ns)
boost    21.88    3.88
  ska    21.22    3.26
swiss    30.53    3.16

  Operation: Lookup
 Impl Mean(ns) Std(ns)
boost   198.61   85.52
  ska   230.34   79.58
swiss   270.94   87.66

  Operation: Delete
 Impl Mean(ns) Std(ns)
boost   198.64   84.71
  ska   229.93   80.20
swiss   142.21   63.99
