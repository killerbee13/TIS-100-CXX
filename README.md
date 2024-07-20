# TIS-100-CXX
A TIS-100 simulator and save validator

This is a validator for saves for the game
[TIS-100](https://zachtronics.com/tis-100/), designed to be used for the 
community-run leaderboard. Unlike other simulators, TIS-100-CXX includes
definitions for the base game's levels and can generate suitable random tests
for them.

Build instructions:

1. Download [kblib](https://github.com/killerbee13/kblib)
2. run `cmake -S . -DEXTRA_INCLUDES="path/to/kblib"`
3. run `cmake --build .`

Note that kblib contains a subfolder also called kblib. The path above refers to
the outer folder, not the inner one.

Run instructions:

The command-line API documented here is currently extremely basic and ad-hoc,
and will be replaced soon.

If run with no
arguments, TIS-100-CXX will run a very basic self-test. With the single argument
`generate`, TIS-100-CXX will write layout and randomized input files compatible
with [Phlarx' tis simulator](https://github.com/Phlarx/tis) for all of TIS-100's
main and second campaign's levels.

Otherwise, the first argument
identifies a level by either name or id (for example, `00150` or
`SELF-TEST DIAGNOSTIC` both refer to the first level of the main campaign)
followed by a path to a TIS-100 source file, followed optionally by the word
`random`, which will cause TIS-100-CXX to use a randomly generated test case.
Otherwise, it will use a hardcoded test case copied from the game. In any case,
it will print whether the validation was successful or not, followed by either
the score, or the reason it was not validated and how many cycles it took to
fail.

