# JSON Tracer
A DynamoRIO client to generate trace of a program using DynamoRIO, outputting
in JSON

## Dependencies
This requires an AMD64 machine running Linux with core utilities installed.
Further dependencies are fetched automatically when building the client.

## How to use
To build the client, run `./build.sh` in the root directory of this repository.
This will fetch the required dependencies and build the client.

To run the client to generate a trace of `ls` using arguments ARGS, run
`./run.sh ARGS -- ls`. Note that this requires building first.

To perform the provided tests on the client, run `./test.sh`.

## Arguments
The client must be provided a command to trace after its other options. An
option for the output format must be given:
  - `--output_interleaved` outputs a trace of all threads interleaved
  - `--output_separated` outputs separate trace files for each thread
  
Then, a command must be given, separated with ` -- `. For example, to trace
`ls -a` with an interleaved output, run:
  `./run.sh --output_interleaved -- ls -a`

Other options may be used for filtering entries to output in the trace.
These allow us to include or exclude any entries corresponding to code in
any dynamically loaded module, in any source code file, or using any specific
instruction. By default, all entries are included in the trace.

To indicate a module, use `--module` followed by the path of the path. Likewise,
for a source file use `--file` or for an instruction use `--instr` followed by
the instruction's name. Instead of a path or instruction name, `--all` may be
used to indicate any match.

To include the following indicated options, use `--include`. To exclude the
following indicated options, use `--exclude`.

Note that later options take precedence over earlier options.

For example, to only include `call` or `ret` instructions when tracing `ls`,
run: `./run.sh --exclude --instruction --all --include --instr call --instr ret -- ls`

To only include the file at source path `src/main.c` when tracing `ls`, run:
`./run.sh --exclude --file --all --include --file src/main.c -- ls`
