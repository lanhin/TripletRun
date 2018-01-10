# TripletRun
A dataflow runtime simulator.

------

## Make
```bash
make
```

To clean the compiling folder:
```bash
make clean
```

## Run
```bash
./triplet
```

## Input files

### 1. `graph.json`

[`graph.json`](graph.json) is the json file that defines the computing graph. Refer to it as an example.

### 2. `cluster.json`

[`cluster.json`](cluster.json) is the json file that defines the cluster. Refer to it as an example.

## Todo list
1. ~~Record every device's busy time and output the results.~~
2. ~~A better way to deal with the data transmission between devices; for memory access, it can overlap with calculation, but network transmission is not.~~
3. Get some good graphs (synthesized and real ones) as benchmarks.
4. More scheduling methods (configurable and self-defined), like FCFS, SJF, RR
5. Data reside in the device memory until all the node's successors have read that data. This will affect the later nodes' scheduling and the runtime needs to record how many successors a node has.
6. ~~Distinguish the communication time and computing time of a task processing.~~
7. Add communication cost for every edge and process logic in execution.

## Develope Notes
(2017-11-13) Now the simulator can really run and produce the processing time of the input graph. There're still many bugs to fix and many features to add.
