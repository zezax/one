# RED - Regular Expression DFA

**Red** is a C++ regular expression engine based on deterministic finite
automata (DFAs).  DFAs are extremely fast pattern-matchers
(linear in size of input).
**Red** excels at matching from among large sets of regexes.

The world already has `RE2`.  Why do we need **Red**?

1. **Red** can report where each of many patterns matched.
`RE2::Set` can only tell which of them did.

1. **Red** compiled patterns are read-only, concurrent, and lock-free.
`RE2` locks the program every time it's used.
This can make a 12-27x difference in performance.

1. **Red** doesn't try to protect you from yourself.  It will compile
expressions for which `RE2` complains "pattern too large - compile failed".

1. **Red** can load/save DFAs as files.

1. **Red** multi-pattern matching is 10x as fast as `RE2::Set`.

**Red** is a pure byte-oriented DFA.

`RE2` has a lot of capture-and-extract functionality,
but since DFA's don't support that, **Red** doesn't either.

## Documentation

- [Usage](doc/Usage.md)
- [Performance](doc/Performance.md)
- [Internals](doc/Internals.md)
