# TIS-100-CXX
A TIS-100 simulator and save validator.

This is a validator for saves for the game
[TIS-100](https://zachtronics.com/tis-100/), designed to be used for the 
[community-run leaderboard](https://www.reddit.com/r/tis100/wiki/index). Unlike
other simulators, TIS-100-CXX includes definitions for the base game's levels
and can generate suitable random tests for them. This simulator aims for exact
parity with the game in both execution and test generation and can run custom
Lua puzzles.

## Building on Linux:

Ensure you have a C++23 compliant compiler (clang 19+, gcc 14+), `cmake`, and
`ccmake`.

1. clone repository
2. update submodules: `git submodule update --init --recursive`
3. run `ccmake -B "path/to/some/build/dir" -S .`
4. customize the `TIS_ENABLE_*` flags if desired
5. run `cmake --build "path/to/some/build/dir"`

If `TIS_ENABLE_LUA` is selected, one needs the lua dev library installed in the
system. For example, for Debian derivatives use `apt install libluajit-5.1-dev`
to install the packaged LuaJIT runtime. Your distro may vary.

Otherwise TIS-100-CXX has only header-only dependencies managed in submodules,
so no further management is needed beyond the above steps.

## Building on Windows (thanks gtw123):

### Install MSYS2
* Install MSYS2 by following the steps on https://www.msys2.org/ to download and
  run the installer.
* Once installed it will automatically launch the MSYS2 UCRT64 environment.
* Install packages:
  * `pacman -S mingw-w64-ucrt-x86_64-toolchain`
  * `pacman -S mingw-w64-ucrt-x86_64-cmake`
    * (This might be automatically installed by the next one? I haven't tried)
  * `pacman -S mingw-w64-ucrt-x86_64-ccmake`
  * `pacman -S git`
    * (This is required to avoid "WARNING Cannot determine rolling release
	  version from git log" when building LuaJIT. If you already have Git for
	  Windows installed you may be able to configure MSYS2 to use that instead.)

### Build LuaJIT
* `git clone https://luajit.org/git/luajit.git`
* `cd luajit`
* `mingw32-make`
* This will create `lua51.dll` and `libluajit-5.1.dll.a` in the `src` directory
  of this repo.

### Build TIS-100-CXX
* Follow the instructions under [Building on Linux](#building-on-linux).
  However, when configuring CMake you'll need to set the location of LuaJIT:
  * `LUAJIT_INCLUDE_DIR` - Set this to the `src` directory of the LuaJIT repo,
    e.g. `C:/Users/username/source/repos/luajit/src`
  * `LUAJIT_LIB` - Set this to the path of `libluajit-5.1.dll.a` built in the
    previous section, e.g.
	`C:/Users/username/source/repos/luajit/src/libluajit-5.1.dll.a`
* Before running `TIS-100-CXX.exe`, copy `lua51.dll` into the directory
  containing the built `TIS-100-CXX.exe`

## Run instructions:

Invocation:
`TIS-100-CXX [options] path/to/solution1.txt path/to/solution2.txt ...`  
The special value `-` can be used to read a solution from STDIN.
The level is autodeduced from the filename prefix, if this is not possible
use an option to give it explicitely.

By default, it will run the simulation and if it passes all fixed tests, it will
print "validation successful" and a score, in the format 
`cycles/nodes/instructions/flags`, where flags may be `a` for achievement,
`c` for cheat, i.e. a solution which does not pass all random tests,
and `h` for a solution that passes < 5% of the random tests
(meaning that it made no real effort to pass any). "Passing" for a random test
means doing it in less than 5x as many cycles as the slowest fixed test (see
`--limit-multiplier` below).

If it does not pass, it will instead print the inputs and outputs
(expected and received) for the failed test, then
"validation failed" with some more information, and finally a score in which
the cycles count is replaced by `-` to denote a failure. The return value is `0`
on a validation, including a cheated solution, `1` on a validation failure
(fixed test failure), and `2` on an exception.

For options `--limit`, `--total-limit`, `--random`, `--seed`, `--seeds`,
and `--T30-size`, integer arguments can be specified with a scale suffix,
either K, M, or B (case-insensitive) for thousand, million, or billion
respectively.

The most useful options are:
- `-L` and `--custom-spec`: Give the path of a Lua custom spec in the same
  format used by the game, the sim will evaluate it the same way the game would.
- `-F` and `--custom-spec-folder`: Give the path of a folder containing Lua
  custom specs. If the sim is asked to sim a file called `SPEC<value>.*`,
  it will search for a Lua spec file called `<value>.lua` in the given folder
  and use it as custom spec if found.
- `--seeds L..H`: a comma-separated list of integer ranges, such as `0..99`.
  Ranges are inclusive on both sides. Can also specify an individual integer,
  meaning a range of just that integer. Can be specified multiple times, which
  is equivalent to concatenating the arguments with a comma.
- `-r N` and `--seed S`: an alternate way to specify random tests, cannot be
  combined with `--seeds`. If `-r` is specified by itself, a starting seed will
  be selected at random. In any case, a contiguous range of N seeds starting at
  S will be used for random tests, except for EXPOSURE MASK VIEWER which may
  skip some seeds (skipped seeds are not compensated for by expanding the
  range).
- `--limit N`: set the timeout limit for the simulation. Default `150000`
  (enough for BUSY_LOOP and almost everything to have been submitted to the
  leaderboard).
- `--total-limit N`: set the maximum number of cycles to evaluate cumulatively
  across all random tests before stopping the simulation and returning a score.
- `--loglevel LEVEL`, `--info`, `--trace`, `--debug`: set the amount of
  information logged to stderr. The default log level is "notice", which
  corresponds to only important information. "info" includes information that
  may be useful but is not always important, including the first random seed to
  fail, and notably the amount of information logged at level "info" is bounded.
  "trace" includes a printout of the board state at each cycle, and may produce
  megabytes of data for moderately long simulations. "debug" includes a full
  trace of the execution in the log, and outputs approximately twice as much as
  "trace". "debug" logging may be disabled at build time for performance.
- `-j N`: run random tests with N worker threads. With `-j 0`, the number of
  hardware threads is detected and used. Note that using multiple threads is not
  allowed with log levels higher than "info" to avoid interleaved output.
- `-q`, `--quiet`: reduce the amount of human-readable text printed around the
  information (does not affect logging). May be specified twice to remove almost
  all supplemental text, printing just the filename (if multiple solves), its
  score, and detailed stats (when requested).
  
Other options:
- `-l segment` is the name of a segment/level as it appears in game, either the
  numeric ID or the human-readable name (case-sensitive). For example, `"00150"`
  and `"SELF-TEST DIAGNOSTIC"` are equivalent. Usually, the sim can determine it
  automatically from the filename prefix, but this argument is sometimes useful,
  such as when the solution comes from stdin. There is a complete list of these
  in the `--help` output, but you will most likely find it easier to get them
  from the game or from the solution's filename. When this is not used, each
  filename in the arguments is checked individually for its prefix, so they can
  be for different levels.
- `--T21-size N` and `--T30-size M`: override the default size limits on
  instructions in any particular T21 node and values in any particular T30 node
  respectively.
  - `--permissive`: allow parser extensions (see below).
- `--cheat-rate C`: change the threshold between /c and /h to any proportion in
  the range 0-1. The default value is .05, or 5%.
- `-k N`, `--limit-multiplier N`: change the scale factor for dynamic timeouts
  for random tests (will not exceed `--limit`). Higher values are more lenient
  toward slow random tests. The default value is `5`, meaning that if a solution
  takes 100 cycles to pass the slowest fixed test, it will time out after 500
  cycles on random tests (even when that is less than `--limit`). This is on by
  default to minimize certain kinds of leaderboard cheese.
- `-c`, `--color`: force color for important info even when redirecting output.
- `-C`, `--log-color`: force color for logs even when redirecting stderr.
- `-S`, `--stats`: run all requested random tests and report the pass rate at
  the end on the score line. Without this flag, the sim will quit as soon as it
  can label a solution /c (that is, at least 5% (cf. `--cheat-rate`) of
  requested tests passed and at least one failed).
- `--no-fixed`: disable fixed tests, run only random tests. This affects
  scoring, as normally random tests do not contribute to scoring except for /c
  and /h flags, but with this flag, the reported score will be the worst
  observed score.
- `--dry-run`: Mainly useful for debugging the command-line parser and initial
  setup. Checks the command line as normal, and that all referenced files exist,
  and quits without running any tests.

## Additional features/intentional discrepancies:

Contrary to its documentation, TIS-100 clamps input values in test cases to the
range [-99, 999]. The simulator allows the full documented range [-999, 999].
When the `--T21-size` argument is specified, nodes may have more than 15
instructions. When `--T30-size` is specified, the capacity of T30 (stack memory)
nodes can be altered.

When `--permissive` is specified on the command line, the simulator allows
the following extensions to the assembly language that do not increase the power of
the language, mostly for convenience when editing files outside of the game.
- Blank/comment-only lines don't count toward the line count limit.
- multiple labels on one line (the game allows multiple labels on one
  instruction, but they must have newlines between them).
- Directional port names (LEFT, RIGHT, UP, DOWN) may be abbreviated to their
  first letters.
- Lines have no length limit.

TIS-100-CXX supports custom levels with any rectangular size, using the new Lua
function `get_layout_ext()`, which is expected to return an array of arrays of
`TILE_*` values of any rectangular size (i.e. every row must have the same
number of columns. If you want empty spaces, fill them with `TILE_DAMAGED` as
normal).

Otherwise, the simulator is intended to exactly simulate the game, including any
restrictions it has.

## Support scripts:

There are two shell scripts distributed with the sim, mainly intended for
testing the sim itself, `test_saves_single.sh` and `test_saves_lb.sh`, both of
which require [fish](https://fishshell.com/) to run. `test_saves_single.sh`
can be used to consume an entire folder of solutions, such as the saves folder
used by the official game, and simulate each of them. `test_saves_lb.sh` can be
used to consume the hierarchical folder structure used by the leaderboard.

Note that TIS-100-CXX can simulate multiple solutions passed on the command line
in a single invocation, reducing some of the need for these scripts.

In addition to all options accepted by the sim itself, these scripts both
accept options:
- `-d`, which identifies the top-level folder to consume (required);
- `-s`, `-w`, and `-f`, which identify files to write reports to, for success
  (score and flags correct), wrong scores, and failed validations (including
  wrong scores if `-w` is not separately specified), respectively;
- `-a`, which identifies a file to receive a report of each score
  (like `-s` and `-f` pointing to the same file, but fully independent of those
  flags);
- `-n`, which is an abbreviation of `--no-fixed`;
- `-i`, which prompts for input after each test.

Note that fish's argparse does not always accept spaces between flags
and option values.
