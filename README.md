# TIS-100-CXX
A TIS-100 simulator and save validator.

This is a validator for saves for the game
[TIS-100](https://zachtronics.com/tis-100/), designed to be used for the 
community-run leaderboard. Unlike other simulators, TIS-100-CXX includes
definitions for the base game's levels and can generate suitable random tests
for them. This simulator aims for exact parity with the game in both execution
and test generation.

## Build instructions:

Ensure you have a C++23 compliant compiler (clang 19+, gcc 14+).

1. clone repository
2. update submodules: `git submodule update --init --recursive`
2. run `cmake -B "path/to/some/build/dir" -S .`
3. run `cmake --build "path/to/some/build/dir"`

TIS-100-CXX only has two dependencies, which are header-only and will be found
and used automatically, so no further management is needed beyond the above
steps.

## Run instructions:

Invocation:
`TIS-100-CXX [options] path/to/solution.txt segment`
`segment` is the name of a segment as it appears in game, either the numeric ID
or the human-readable name (case-sensitive). For example, `"00150"` and
`"SELF-TEST DIAGNOSTIC"` are equivalent. There is a complete list of these in
the `--help` output, but you will most likely find it easier to get these from
the game or from the solution's filename. 

By default, it will run the simulation and if it passes, it will print
"validation successful" and a score, in the format 
`cycles/nodes/instructions/flags`, where flags may be `a` for achievement,
`c` for cheat, i.e. a solution which does not pass all random tests,
and `h` for a solution that passes < 5% of the random tests
(meaning that it made no real effort to pass any).

If it does not pass, it will instead print the inputs and outputs
(expected and received) for the failed test, then
"validation failed" with some more information, and finally a score in which
the cycles count is replaced by `-` to denote a failure. The return value is `0`
on a validation, including a cheated solution, `1` on a validation failure,
and `2` on an exception.

The most useful options are:
- `--limit N`: set the timeout limit for the simulation. Default 100500 (enough
  for BUSY_LOOP with a little slack)
- `--seeds L..H`: a comma-separated list of integer ranges, such as 0..99.
  Ranges are inclusive on both sides. Can also specify an individual integer,
  meaning a range of just that integer. Can be specified multiple times, which
  is equivalent to concatenating the arguments with a comma.
- `-r N` and `--seed S`: an alternate way to specify random tests, cannot be
  combined with `--seeds`. If `-r` is specified by itself, a starting seed will
  be selected at random. In any case, a contiguous range of N seeds starting at
  S will be used for random tests, except for EXPOSURE MASK VIEWER which may
  skip some seeds.
- `--fixed 0`: Disable fixed tests, run only random tests. This affects scoring,
  as normally random tests do not contribute to scoring except for /c and /z
  flags, but with this flag, the reported score will be the worst observed
  score.
- `--loglevel LEVEL`, `--debug`, `--info`: set the amount of information logged
  to stderr. The default log level is "notice", which corresponds to only
  important information. "info" includes information that may be useful but is
  not always important, and notably the amount of information logged at level
  "info" is bounded. "debug" includes a full trace of the execution in the
  log and will often produce multiple MB of data.
- `-q`, `--quiet`: reduce the amount of human-readable text printed around the
  information. May be specified twice to remove almost all supplemental text.
  
Other options:
- `--T21_size N` and `--T30_size M`: override the default size limits on
  instructions in any particular T21 node and values in any particular T30 node
  respectively.
- `-c`, `--color`: force color for important info even when redirecting output
- `-C`, `--log-color`: force color for logs even when redirecting stderr
- `-S`, `--stats`: run all requested random tests and report the pass rate at
  the end. Without this flag, the sim will quit as soon as it can label a
  solution /c (that is, one pass and one failure).
- `-L ID`, `--level ID`: As an alternative to specifying the segment as
  described above, the numeric ID (0..50) can be used instead.
- `--dry-run`: Mainly useful for debugging the command-line parser and initial
  setup.
