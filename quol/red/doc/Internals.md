# Red Internals

## Map of the Code

### Directories

- `include` - headers (declarations, inlines, and templates)
- `lib` - C++ source files
- `test` - unit tests for above
- `tools` - debugging aids and benchmarks
- `scripts` - miscellaneous helpers (bash & python)
- `doc` - documentation beyond this README

File naming:

- `Foo.h` - header for Foo that lives in `include`
- `Foo.cpp` - code for Foo that lives in `lib`
- `foo.cpp` - unit tests for Foo that live in `test`

### Classes

Stand-alone code:

- `Fnv` - Fowler Noll Vo hashing
- `DefaultMap` - hash table that returns the default value for missing keys
- `SparseVec` - sparse vector as sorted array of pairs
- `BitSet` - generic bit-vector set

Building blocks:

- `Types` - declarations of common types
- `Consts` - commonly used constant values
- `Compile` - wrapper convert regex into final DFA
- `Outcome` - struct returned by match and search functions
- `Except` - exceptions thrown by library
- `Util` - useful low-level code
- `Debug` - string serialization of library classes

Heavy lifting:

- `Scanner` - regular expression tokenizer
- `Parser` - creates NFA from regular expression
- `Nfa` - object encapsulation of NFA
- `Dfa` - object representing DFA
- `Powerset` - converter from NFA to DFA
- `Minimizer` - DFA minimization
- `Serializer` - creates efficient representation for execution
- `Executable` - container for serialized representation
- `Proxy` - templates for accessing various input and DFA formats
- `Matcher` - template implementations of check, match, search, replace
- `Red` - mainstream API

## Algorithms in Play

- Recursive descent parsing
- Rabin-Scott powerset construction
- End marks (end symbols) to differentiate accept states
- David Gries DFA minimization
- Successive partitioning to yield equivalent character sets

## apigen

The script `apigen` is used to generate much of the repetitive code
related to the orthogonal naming scheme.  It generates rough code for
`Red.h` and `Red.cpp`, and tests in `red.cpp`.
