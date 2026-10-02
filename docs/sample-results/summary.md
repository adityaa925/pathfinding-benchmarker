
### Grid obstacle density (4-connected)

**expanded** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar manhattan [binary] |   astar zero [binary] |       bfs |   dijkstra [binary] |
|--------------:|---------------------------:|---------------------------:|----------------------:|----------:|--------------------:|
|          0    |                  2.611e+05 |               1023         |             2.621e+05 | 2.621e+05 |           2.621e+05 |
|          0.05 |                  2.477e+05 |               7273         |             2.49e+05  | 2.49e+05  |           2.49e+05  |
|          0.1  |                  2.334e+05 |                  1.117e+04 |             2.358e+05 | 2.358e+05 |           2.358e+05 |
|          0.15 |                  2.179e+05 |                  1.679e+04 |             2.225e+05 | 2.225e+05 |           2.225e+05 |
|          0.2  |                  2.015e+05 |                  1.813e+04 |             2.093e+05 | 2.093e+05 |           2.093e+05 |
|          0.25 |                  1.81e+05  |                  1.085e+04 |             1.955e+05 | 1.955e+05 |           1.955e+05 |
|          0.3  |                  1.576e+05 |                  2.971e+04 |             1.803e+05 | 1.803e+05 |           1.803e+05 |
|          0.35 |                  1.333e+05 |                  6.548e+04 |             1.61e+05  | 1.61e+05  |           1.61e+05  |
|          0.4  |                  7.704e+04 |                  7.109e+04 |             9.122e+04 | 9.121e+04 |           9.122e+04 |

**time_ms** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar manhattan [binary] |   astar zero [binary] |   bfs |   dijkstra [binary] |
|--------------:|---------------------------:|---------------------------:|----------------------:|------:|--------------------:|
|          0    |                     34.95  |                     0.4102 |                 32.51 | 5.722 |               29.78 |
|          0.05 |                     28.68  |                     1.236  |                 25.17 | 4.413 |               22.34 |
|          0.1  |                     31.08  |                     2.201  |                 29.89 | 4.83  |               28.71 |
|          0.15 |                     30.14  |                     3.152  |                 30.73 | 6.394 |               27.9  |
|          0.2  |                     27.61  |                     3.426  |                 27.26 | 6.534 |               29    |
|          0.25 |                     23.35  |                     1.929  |                 25.01 | 7.051 |               25.34 |
|          0.3  |                     21.73  |                     5.699  |                 23.97 | 6.978 |               23.14 |
|          0.35 |                     18.47  |                    11.85   |                 22.42 | 7.411 |               24.07 |
|          0.4  |                      8.822 |                     9.956  |                 10.46 | 3.945 |               10.52 |

**alloc_MiB** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar manhattan [binary] |   astar zero [binary] |   bfs |   dijkstra [binary] |
|--------------:|---------------------------:|---------------------------:|----------------------:|------:|--------------------:|
|          0    |                      3.035 |                      3.035 |                 3.018 | 4.5   |               3.018 |
|          0.05 |                      3.035 |                      3.281 |                 3.021 | 4.5   |               3.021 |
|          0.1  |                      3.035 |                      3.45  |                 3.018 | 4.5   |               3.018 |
|          0.15 |                      3.035 |                      3.506 |                 3.018 | 4.5   |               3.018 |
|          0.2  |                      3.035 |                      3.562 |                 3.018 | 4.5   |               3.018 |
|          0.25 |                      3.035 |                      3.253 |                 3.018 | 4.5   |               3.018 |
|          0.3  |                      3.035 |                      3.309 |                 3.018 | 4.5   |               3.018 |
|          0.35 |                      3.035 |                      3.127 |                 3.018 | 4.5   |               3.018 |
|          0.4  |                      3.014 |                      3.026 |                 3.008 | 3.675 |               3.008 |


### Grid side length (20% obstacles)

**expanded** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar manhattan [binary] |   astar zero [binary] |          bfs |   dijkstra [binary] |
|--------------:|---------------------------:|---------------------------:|----------------------:|-------------:|--------------------:|
|            64 |               2870         |                419         |          3243         | 3243         |        3243         |
|           128 |                  1.22e+04  |               1039         |             1.307e+04 |    1.307e+04 |           1.307e+04 |
|           256 |                  4.959e+04 |               4042         |             5.238e+04 |    5.238e+04 |           5.238e+04 |
|           512 |                  2.011e+05 |                  1.724e+04 |             2.093e+05 |    2.093e+05 |           2.093e+05 |
|          1024 |                  8.093e+05 |                  6.517e+04 |             8.372e+05 |    8.372e+05 |           8.372e+05 |

**time_ms** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar manhattan [binary] |   astar zero [binary] |      bfs |   dijkstra [binary] |
|--------------:|---------------------------:|---------------------------:|----------------------:|---------:|--------------------:|
|            64 |                     0.2059 |                    0.03381 |                0.2093 |  0.04968 |              0.2033 |
|           128 |                     1.283  |                    0.129   |                1.168  |  0.2969  |              1.201  |
|           256 |                     6.565  |                    0.7159  |                6.223  |  1.483   |              6.038  |
|           512 |                    26.95   |                    3.398   |               30.26   |  6.188   |             27.61   |
|          1024 |                   129.2    |                   13.41    |              119.1    | 34.7     |            119.8    |

**alloc_MiB** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar manhattan [binary] |   astar zero [binary] |      bfs |   dijkstra [binary] |
|--------------:|---------------------------:|---------------------------:|----------------------:|---------:|--------------------:|
|            64 |                    0.05127 |                    0.06094 |               0.04907 |  0.07031 |             0.04907 |
|           128 |                    0.1963  |                    0.2191  |               0.1919  |  0.2812  |             0.1919  |
|           256 |                    0.7676  |                    0.8906  |               0.7588  |  1.125   |             0.7588  |
|           512 |                    3.035   |                    3.562   |               3.018   |  4.5     |             3.018   |
|          1024 |                   12.07    |                   14.03    |              12.04    | 18       |            12.04    |


### Sparse random graphs, SSSP

**time_ms** (mean over instances)

|   param_value |     bfs |   dijkstra [binary] |
|--------------:|--------:|--------------------:|
|         1e+04 |  0.2831 |               2.224 |
|         3e+04 |  0.991  |               8.352 |
|         1e+05 |  5.356  |              39.47  |
|         3e+05 | 20.83   |             150.2   |
|         1e+06 | 90.31   |             781.9   |

**alloc_MiB** (mean over instances)

|   param_value |     bfs |   dijkstra [binary] |
|--------------:|--------:|--------------------:|
|         1e+04 |  0.2082 |              0.3957 |
|         3e+04 |  0.5308 |              1.356  |
|         1e+05 |  1.894  |              3.394  |
|         3e+05 |  6.433  |             12.43   |
|         1e+06 | 17.44   |             47.44   |

**expanded** (mean over instances)

|   param_value |   bfs |   dijkstra [binary] |
|--------------:|------:|--------------------:|
|         1e+04 | 1e+04 |               1e+04 |
|         3e+04 | 3e+04 |               3e+04 |
|         1e+05 | 1e+05 |               1e+05 |
|         3e+05 | 3e+05 |               3e+05 |
|         1e+06 | 1e+06 |               1e+06 |


### Heap comparison: Dijkstra SSSP on random graphs (param = average degree)

**time_ms** (mean over instances)

|   param_value |   dijkstra [binary] |   dijkstra [indexed_binary] |   dijkstra [quaternary] |   dijkstra [std_pq] |
|--------------:|--------------------:|----------------------------:|------------------------:|--------------------:|
|             2 |               61.5  |                       56.82 |                   55.75 |               44.48 |
|             4 |               96.63 |                       78.32 |                   89.84 |               78    |
|             8 |              137.4  |                      100.1  |                  126.7  |              119.6  |
|            16 |              194.6  |                      128.6  |                  194.6  |              189.1  |

**generated** (mean over instances)

|   param_value |   dijkstra [binary] |   dijkstra [indexed_binary] |   dijkstra [quaternary] |   dijkstra [std_pq] |
|--------------:|--------------------:|----------------------------:|------------------------:|--------------------:|
|             2 |           2e+05     |                   2e+05     |               2e+05     |           2e+05     |
|             4 |           2.609e+05 |                   2.609e+05 |               2.609e+05 |           2.609e+05 |
|             8 |           3.552e+05 |                   3.552e+05 |               3.552e+05 |           3.552e+05 |
|            16 |           4.701e+05 |                   4.701e+05 |               4.701e+05 |           4.701e+05 |

**stale_pops** (mean over instances)

|   param_value |   dijkstra [binary] |   dijkstra [indexed_binary] |   dijkstra [quaternary] |   dijkstra [std_pq] |
|--------------:|--------------------:|----------------------------:|------------------------:|--------------------:|
|             2 |           0.4       |                           0 |               0.4       |           0.4       |
|             4 |           6.092e+04 |                           0 |               6.092e+04 |           6.092e+04 |
|             8 |           1.552e+05 |                           0 |               1.552e+05 |           1.552e+05 |
|            16 |           2.701e+05 |                           0 |               2.701e+05 |           2.701e+05 |

**peak_open** (mean over instances)

|   param_value |   dijkstra [binary] |   dijkstra [indexed_binary] |   dijkstra [quaternary] |   dijkstra [std_pq] |
|--------------:|--------------------:|----------------------------:|------------------------:|--------------------:|
|             2 |           2.022e+04 |                   2.022e+04 |               2.022e+04 |           2.022e+04 |
|             4 |           1.095e+05 |                   8.271e+04 |               1.095e+05 |           1.095e+05 |
|             8 |           2.107e+05 |                   1.235e+05 |               2.107e+05 |           2.107e+05 |
|            16 |           3.289e+05 |                   1.53e+05  |               3.289e+05 |           3.289e+05 |

**open_MiB** (mean over instances)

|   param_value |   dijkstra [binary] |   dijkstra [indexed_binary] |   dijkstra [quaternary] |   dijkstra [std_pq] |
|--------------:|--------------------:|----------------------------:|------------------------:|--------------------:|
|             2 |                0.75 |                       1.513 |                    0.75 |                0.75 |
|             4 |                3    |                       3.763 |                    3    |                3    |
|             8 |                6    |                       3.763 |                    6    |                6    |
|            16 |               12    |                       6.763 |                   12    |               12    |


### Heap comparison: 8-connected grid (param = side length)

**time_ms** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar euclidean [indexed_binary] |   astar euclidean [quaternary] |   astar euclidean [std_pq] |   astar octile [binary] |   astar octile [indexed_binary] |   astar octile [quaternary] |   astar octile [std_pq] |   dijkstra [binary] |   dijkstra [indexed_binary] |   dijkstra [quaternary] |   dijkstra [std_pq] |
|--------------:|---------------------------:|-----------------------------------:|-------------------------------:|---------------------------:|------------------------:|--------------------------------:|----------------------------:|------------------------:|--------------------:|----------------------------:|------------------------:|--------------------:|
|           768 |                       56.6 |                              44.63 |                          55.69 |                      55.56 |                   49.15 |                           35.93 |                       49.57 |                   45.98 |               77.66 |                       75.24 |                   81.78 |               77.82 |

**expanded** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar euclidean [indexed_binary] |   astar euclidean [quaternary] |   astar euclidean [std_pq] |   astar octile [binary] |   astar octile [indexed_binary] |   astar octile [quaternary] |   astar octile [std_pq] |   dijkstra [binary] |   dijkstra [indexed_binary] |   dijkstra [quaternary] |   dijkstra [std_pq] |
|--------------:|---------------------------:|-----------------------------------:|-------------------------------:|---------------------------:|------------------------:|--------------------------------:|----------------------------:|------------------------:|--------------------:|----------------------------:|------------------------:|--------------------:|
|           768 |                  2.515e+05 |                          2.515e+05 |                      2.515e+05 |                  2.515e+05 |               1.756e+05 |                       1.756e+05 |                   1.756e+05 |               1.756e+05 |           4.708e+05 |                   4.708e+05 |               4.708e+05 |           4.708e+05 |

**generated** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar euclidean [indexed_binary] |   astar euclidean [quaternary] |   astar euclidean [std_pq] |   astar octile [binary] |   astar octile [indexed_binary] |   astar octile [quaternary] |   astar octile [std_pq] |   dijkstra [binary] |   dijkstra [indexed_binary] |   dijkstra [quaternary] |   dijkstra [std_pq] |
|--------------:|---------------------------:|-----------------------------------:|-------------------------------:|---------------------------:|------------------------:|--------------------------------:|----------------------------:|------------------------:|--------------------:|----------------------------:|------------------------:|--------------------:|
|           768 |                  3.416e+05 |                          3.416e+05 |                      3.416e+05 |                  3.416e+05 |               2.572e+05 |                       2.572e+05 |                   2.572e+05 |               2.572e+05 |           4.792e+05 |                   4.792e+05 |               4.792e+05 |           4.792e+05 |

**stale_pops** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar euclidean [indexed_binary] |   astar euclidean [quaternary] |   astar euclidean [std_pq] |   astar octile [binary] |   astar octile [indexed_binary] |   astar octile [quaternary] |   astar octile [std_pq] |   dijkstra [binary] |   dijkstra [indexed_binary] |   dijkstra [quaternary] |   dijkstra [std_pq] |
|--------------:|---------------------------:|-----------------------------------:|-------------------------------:|---------------------------:|------------------------:|--------------------------------:|----------------------------:|------------------------:|--------------------:|----------------------------:|------------------------:|--------------------:|
|           768 |                  8.747e+04 |                                  0 |                      8.747e+04 |                  8.747e+04 |               7.827e+04 |                               0 |                   7.827e+04 |               7.827e+04 |                8360 |                           0 |                    8360 |                8360 |

**peak_open** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar euclidean [indexed_binary] |   astar euclidean [quaternary] |   astar euclidean [std_pq] |   astar octile [binary] |   astar octile [indexed_binary] |   astar octile [quaternary] |   astar octile [std_pq] |   dijkstra [binary] |   dijkstra [indexed_binary] |   dijkstra [quaternary] |   dijkstra [std_pq] |
|--------------:|---------------------------:|-----------------------------------:|-------------------------------:|---------------------------:|------------------------:|--------------------------------:|----------------------------:|------------------------:|--------------------:|----------------------------:|------------------------:|--------------------:|
|           768 |                       3098 |                               1858 |                           3098 |                       3098 |                    3615 |                            1824 |                        3615 |                    3615 |                1097 |                        1080 |                    1097 |                1097 |

**open_MiB** (mean over instances)

|   param_value |   astar euclidean [binary] |   astar euclidean [indexed_binary] |   astar euclidean [quaternary] |   astar euclidean [std_pq] |   astar octile [binary] |   astar octile [indexed_binary] |   astar octile [quaternary] |   astar octile [std_pq] |   dijkstra [binary] |   dijkstra [indexed_binary] |   dijkstra [quaternary] |   dijkstra [std_pq] |
|--------------:|---------------------------:|-----------------------------------:|-------------------------------:|---------------------------:|------------------------:|--------------------------------:|----------------------------:|------------------------:|--------------------:|----------------------------:|------------------------:|--------------------:|
|           768 |                    0.09375 |                              2.297 |                        0.09375 |                    0.09375 |                 0.09375 |                           2.297 |                     0.09375 |                 0.09375 |             0.04688 |                       2.297 |                 0.04688 |             0.04688 |
