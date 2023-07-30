# JSON Tracer
A DynamoRIO client to generate trace of a program using DynamoRIO, outputting
in JSON

## Dependencies
To build the client, DynamoRIO and `libdwarf` are required. Importantly,
`libdwarf` must be compiled as position independent.

## Example Sum Program
This repository comes with an example program in `sum_program/` to test the
tracer.

The example program is given a number of threads and some file names,
summarising the files using that many threads. Each file is a CSV file with
columns for:
    - tag (unsigned integer)
    - value (float)

Any lines not in this format are ignored.

This program returns the mean value for each tag over all files.

To execute the example program, build it using `make` and run `sum`. For
example, to execute it using two threads over the provided CSV files, run in
the `sum_program/` directory:

```
make
./sum 2 table*
```
