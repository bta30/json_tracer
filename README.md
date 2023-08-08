# JSON Tracer
A DynamoRIO client to generate trace of a program using DynamoRIO, outputting
in JSON

## Dependencies
This requires an AMD64 machine running Linux with core utilities installed.
Further dependencies are fetched automatically when building the client.

## How to use
To build the client, run `./build.sh` in the root directory of this repository.
This will fetch the required dependencies and build the client.

To run the client to generate a trace of `ls ARGS` using options OPTS, run
`./run.sh OPTS -- ls ARGS`. Note that this requires building first.

To perform the provided tests on the client, run `./test.sh`.

## Debugging Information in Trace Output
The trace output includes all relevant debugging information retrievable.
Importantly, this requires the given executable to be compiled to include
maximal DWARF 4 debugging information and without optimisation.

## Tracer Options

### Output Options
An option for the output format must be given, otherwise no output will be
created:
  - `--output_interleaved` outputs a trace of all threads interleaved in one
file (keeping the correct ordering of executed instructions across threads)
  - `--output_separated` outputs separate trace files for each thread
  
Then, a program must be given with its arguments, separated with ` -- `. For
example, to trace `ls -a` with an interleaved output, run:

```./run.sh --output_interleaved -- ls -a```

By default, the trace outputs in unique files at relative paths
`[PREFIX].xxxx.log`, where `[PREFIX]` is a prefix passed to the client and
`xxxx` is a four-digit numeral incremented to output at a unique file.

By default, `[PREFIX]` is `trace`, so we get outputs `trace.0000.log`,
`trace.0001.log`, etc. To change `[PREFIX]`, pass the client the option
`--output_prefix [PREFIX]`. For example, to trace `ls -a` with an
interleaved output in a file `ls_trace.xxxx.log`, run:

```./run.sh --output_interleaved --output_prefix ls_trace -- ls -a```

### Filtering Options
Other options may be used for filtering which entries to output in the trace.
These allow us to include or exclude any entries corresponding to code in
any dynamically loaded module, in any source code file, or using any specific
instruction.

By default, entries for every instruction in every module is included in the
trace output.

First, we use `--include` to indicate that further options indicate entries to
include in the trace output, and we use `--exclude` similarly to exclude entries.

To indicate a module, use `--module` followed by the path of the module. Likewise,
for a source file use `--file` or for an instruction use `--instr` followed by
the instruction's name. Instead of a path or instruction name, `--all` may be
used to indicate any match.

For example, `--include --module /usr/lib/library.so` includes entries in the
module at `/usr/lib/library.so`, `--exclude --file /home/user/project/main.c`
excludes entries corresponding to source file `/home/user/project/main.c`, and
to only include entries corresponding to only the instructions `call` and `ret`,
use `--exclude --instr --all --include --instr call --instr ret`.

Note that later options take precedence over earlier options. With this, filtering
by source file depends on having debug information available - without debugging
information, filtering by a source file may not successfully filter entries for
instructions in that file.

For example, to only include `call` or `ret` instructions when tracing `ls -a`,
run:

```./run.sh --exclude --instruction --all --include --instr call --instr ret -- ls```

To only include the file at source path `src/main.c` when tracing `ls -a`, run:

```./run.sh --exclude --file --all --include --file src/main.c -- ls -a```

To only include entries in module `build/module.so` but excluding entries corresponding
to source files `src/main.c` and `src/file.c` when generating a trace of `progname args`, run:

```./run.sh --exclude --module --all --include --module build/module.so --exclude --file src/main.c -- file src/file.c -- progname args```

## JSON Output Format
The output of the trace is an array of records corresponding to one instruction
executed each, in order of execution. These records contain the following fields:
  - `time`   - a string of the real-world time this instruction was executed
  - `tid`    - an integer of the id of the thread executing this instructio
  - `pc`     - a string containing the hexadecimal value of the program counter at this instruction
  - `opcode` - a record containing information of the operator of the instruction, containing fields:
    - `name` - a string containing the name of the operator (e.g. `mov`)
    - `value` - an integer of the value of this opcode (e.g. `56`)
  - `lineInfo` (optional) - a record containing information on which line in the program source code this instruction forms part of
(this may not be included for entries where no debugging information for such source lines exists)
    - `file` - a string containing the path of the source code file the instruction corresponds to
    - `line` - an integer containing the line of the source code file the instruction corresponds to
  - `srcs`   - an array of operand records corresponding to the source operands of the instruction
  - `dsts`   - an array of operand records corresponding to the destination operands of the instruction

Operand records contain the following fields:
  - `type` - a string containing the type of operand, containing one of the following values:
    - `immedInt`   - an immediate integer
    - `immedFloat` - an immediate floating point number
    - `pc`         - a program counter value (e.g. for function calls)
    - `reg`        - a register
    - `memRef`     - a memory reference
  - `size` - the size of the operand, in bytes

Depending on the type of the operand, there may be other fields in its operand record.

If the type is `immedInt`:
  - `value` - an integer containing the value of the immediate integer
    
If the type is `immedFloat`:
  - `value` - a float containing the value of the immediate float
    
If the type is `pc`:
  - `value` - a string of the hexadecimal value of this program counter operand
  - `debugInfo` (optional) - an array of debugging information records for the address of this operand   
  
If the type is `reg`:
  - `register` - a record containing the following fields of information for the register:
    - `name` - a string of the name of the register (e.g. `rax`, `eax` or `xmm0`)
    - `value` - a string of the hexadecimal value of the register (note that this value may
correspond to a number greater than 64 bits long for some special registers)
    - `debugInfo` (optional)  - an array of debugging information records for an address stored in the register
    
If the type is `memRef`:
- `reference` - a record of information on the memory reference, containing fields:
  - `type` - a string for the type of the memory reference, containing one of the following values:
    
    - `absAddr` - an absolute address
    - `pcRelAddr` - a program counter-relative address
    - `baseDisp` - a base register + displacement indirect address
      
  - `isFar` - a boolean value containing `true` is this is a far address, or `false` is this is a near address
  - `addr`  - a string containing the hexadecimal absolute value of this address when executing this instruction
  - `value` (optional) - a string containing the hexadecimal 64-bit value stored at this address
  - `debugInfo` (optional) - an array of debugging information records for the address of the memory reference
(this is given if the instruction specifically accesses this value)

  If the `type` is specifically `baseDisp`, the `reference` record also contains the following fields:
    - `base`  - a record containing information on the base register, with fields:
      - `name`  - a string containing the name of the base register
      - `value` - a string containing the hexadecimal value of the base register
    - `index` - a record containing information on the index register, with fields:
      - `name`  - a string containing the name of the index register
      - `value` - a string containing the hexadecimal value of the index register
    - `scale` - an integer containing the scale factor of the index register
    - `disp` - an integer containing the displacement of the address

Debugging information for operands (i.e. the corresponding function/variable
information as in the source code) is given as an array of one or more records,
as there may be more than one match for multiple variables or such sharing a
memory location (e.g. in a union). These records contain the following fields:
  - `type` - the type of the debugging information, containing one of the following values:
    - `function` - indicating a matched function
    - `variable` - indicating a matched variable
  - `name` - the name of the function/variable matches
  - `address` - the base address of the function or variable (i.e. its lowest program counter or address)

If the `type` is `variable` then the following fields will also be present:
  - `isLocal` - a boolean containing `true` if this is a local variable, or `false` if this is a static/global variable
  - `valueType` - a record for information on the type of the variable, containing fields:
    - `name` - a string containing the name of the base type (e.g. `int`)
    - `compound` - an array of strings of any compounds on this type, in order of closest applied to the base type to least, each string of one of the following values:
      - `const` - a constant value (`const` in C)
      - `pointer` - a pointer to a value (`*` in C)
      - `lvalueReference` - an lvalue reference of a value (`&` in C++)
      - `rvalueReference` - an rvalue reference of a value (`&&` in C++)
      - `restrict` - a restricted value (`restrict` in C)
      - `volatile` - a volatile value (`volatile` in C)
      - `array`  - an array of values (`[]` in C)
    
    For example, a volatile pointer to a constant integer may correspond to `valueType`: `{ "name": "int",  "compound": [ "const", "pointer", "volatile" ] }`

## Scripts
This client comes with some scripts with some Python scripts for performing
simple analyses on traces. These are found in the `scripts/` directory.

### Human Readable Output
This script, `human_readable_output.py`, converts the contents of a given trace
to a more human readable output. For example, to convert a trace
`trace.0000.log`, run:
```python scripts/human_readable_output.py trace.0000.log```

Each entry becomes a line of tab-separated values for time, thread ID,
source file (if available), source file line (if available), opcode name
and operands (with debug information if available).

### Eraser
This script, `eraser_lockset.py`, performs a modified version of the Eraser
algorithm, returning the possible locks associated with certain resources for
a given trace file. For multi-threaded applications, it is recommended to
generate interleaved traces for this. For example, to get locks for resources
in `trace.0000.log`, run:
```python scripts/eraser_lockset.py trace.0000.log```

These resources are global variables that are accessed under other global
variables that are locks (detected as having type `std::mutex`). This are
only global variables for which debugging information is found.

It returns output of one line for each global variable with a list of each
possible mutex. For example, if global variable `a` is locked with lock `b`,
then there will be line `a, ['b']`, or this list may contain other locks.
