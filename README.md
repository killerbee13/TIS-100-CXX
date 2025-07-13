# TIS-100-CXX
A TIS-100 simulator and save validator.

This is a validator for saves for the game
[TIS-100](https://zachtronics.com/tis-100/), designed to be used for the 
[community-run leaderboard](https://www.reddit.com/r/tis100/wiki/index). Unlike
other simulators, TIS-100-CXX includes definitions for the base game's levels
and can generate suitable random tests for them. This simulator aims for exact
parity with the game in both execution and test generation.

## Building on Linux:

Ensure you have a C++23 compliant compiler (clang 19+, gcc 14+), `cmake`, and
`ccmake`.

1. clone repository
2. update submodules: `git submodule update --init --recursive`
3. run `ccmake -B "path/to/some/build/dir" -S .`
4. customize the `TIS_ENABLE_*` flags if desired
5. run `cmake --build "path/to/some/build/dir"`

If `TIS_ENABLE_LUA` is selected, one needs the lua dev library installed in the
system,
for Debian derivatives use:
```sh
apt install libluajit-5.1-dev
```

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
  * `LUAJIT_LIB` - Set this to the path of `libluajit-5.1.dll.a` built in the previous
    section, e.g. `C:/Users/username/source/repos/luajit/src/libluajit-5.1.dll.a`
* Before running `TIS-100-CXX.exe`, copy `lua51.dll` into the directory
  containing the built `TIS-100-CXX.exe`

## Run instructions:

Invocation:
`TIS-100-CXX [options] path/to/solution1.txt path/to/solution2.txt ...`  
The special value `-` can be used to read a solution from STDIN.
The level is autodeduced from the filename prefix, if this is not possible
use an option to give it explicitely.

By default, it will run the simulation and if it passes, it will print
"validation successful" and a score, in the format 
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
on a validation, including a cheated solution, `1` on a validation failure,
and `2` on an exception.

For options `--limit`, `--total-limit`, `--random`, `--seed`, `--seeds`,
and `--T30-size`, integer arguments can be specified with a scale suffix,
either K, M, or B (case-insensitive) for thousand, million, or billion
respectively.

The most useful options are:
- `-l segment` is the name of a segment as it appears in game, either the
  numeric ID or the human-readable name (case-sensitive). For example, `"00150"`
  and `"SELF-TEST DIAGNOSTIC"` are equivalent. There is a complete list of these
  in the `--help` output, but you will most likely find it easier to get them
  from the game or from the solution's filename.
- `-L` and `--custom-spec`: in alternative to `-l`, give the path of
  a Lua custom spec in the format used by the game, the sim will evaluate it
  the same way the game would.
- `--limit N`: set the timeout limit for the simulation. Default `100500`
  (enough for BUSY_LOOP with a little slack).
- `--seeds L..H`: a comma-separated list of integer ranges, such as `0..99`.
  Ranges are inclusive on both sides. Can also specify an individual integer,
  meaning a range of just that integer. Can be specified multiple times, which
  is equivalent to concatenating the arguments with a comma.
- `-r N` and `--seed S`: an alternate way to specify random tests, cannot be
  combined with `--seeds`. If `-r` is specified by itself, a starting seed will
  be selected at random. In any case, a contiguous range of N seeds starting at
  S will be used for random tests, except for EXPOSURE MASK VIEWER which may
  skip some seeds.
- `--loglevel LEVEL`, `--debug`, `--trace`, `--info`: set the amount of
  information logged to stderr. The default log level is "notice", which
  corresponds to only important information. "info" includes information that
  may be useful but is not always important, and notably the amount of
  information logged at level "info" is bounded. "trace" includes a printout
  of the board state at each cycle. "debug" includes a full trace of the
  execution in the log and will often produce multiple MB of data.
- `-j N`: run random tests with N worker threads. With `-j 0`, the number of
  hardware threads is detected and used.
- `-q`, `--quiet`: reduce the amount of human-readable text printed around the
  information. May be specified twice to remove almost all supplemental text.
  
Other options:
- `--T21-size N` and `--T30-size M`: override the default size limits on
  instructions in any particular T21 node and values in any particular T30 node
  respectively.
- `--cheat-rate C`: change the threshold between /c and /h to any proportion in
  the range 0-1. The default value is .05, or 5%.
- `-k N`, `--limit-multiplier N`: change the scale factor for dynamic timeouts
  for random tests (will not exceed --limit). Higher values are more lenient
  toward slow random tests. The default value is 5, meaning that if a solution
  takes 100 cycles to pass the slowest fixed test, it will time out after 500
  cycles on random tests (even when that is less than --limit).  
- `-c`, `--color`: force color for important info even when redirecting output
- `-C`, `--log-color`: force color for logs even when redirecting stderr
- `-S`, `--stats`: run all requested random tests and report the pass rate at
  the end. Without this flag, the sim will quit as soon as it can label a
  solution /c (that is, at least 5% of requested tests passed and at least one
  failed).
- `--fixed 0`: disable fixed tests, run only random tests. This affects scoring,
  as normally random tests do not contribute to scoring except for /c and /h
  flags, but with this flag, the reported score will be the worst observed
  score.
- `--dry-run`: Mainly useful for debugging the command-line parser and initial
  setup. Checks the command line as normal and quits before running any tests.

## Additional features:

Contrary to its documentation, TIS-100 clamps input values in test cases to the
range [-99, 999]. The simulator allows the full documented range [-999, 999].

TIS-100-CXX supports custom levels with any rectangular size, using the new Lua
function `get_layout_ext()`, which is expected to return an array of arrays of
`TILE_*` values of any rectangular size (i.e. every row must have the same
number of columns).

The parser accepts some things the game does not, namely:
- When the `--T21-size` argument is specified, nodes may have more than 15
  instructions
- multiple labels on one line
- Directional port names (LEFT, RIGHT, UP, DOWN) may be abbreviated to their
  first letters
- Lines have no length limit

Otherwise, the simulator is intended to exactly simulate the game, including any
restrictions it has.

## Support scripts:

There are two shell scripts distributed with the sim, mainly intended for
testing the sim itself, `test_saves_single.sh` and `test_saves_lb.sh`, both of
which require [fish](https://fishshell.com/) to run. `test_saves_single.sh`
can be used to consume an entire folder of solutions, such as the saves folder
used by the official game, and simulate each of them. `test_saves_lb.sh` can be
used to consume the hierarchical folder structure used by the leaderboard.

In addition to all options accepted by the sim itself, these scripts both
accept options:
- `-d`, which identifies the top-level folder to consume (required);
- `-s`, `-w`, and `-f`, which identify files to write reports to, for success
  (score and flags correct), wrong scores, and failed validations (including
  wrong scores if `-w` is not separately specified), respectively;
- `-a`, which identifies a file to receive a report of each score
  (like -s and -f pointing to the same file, but fully independent of those
  flags);
- `-n`, which is an abbreviation of `--fixed 0`;
- `-i`, which prompts for input after each test.

Note that fish's argparse does not always accept spaces between flags
and option values.
