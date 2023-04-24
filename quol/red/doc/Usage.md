# Red Usage

The simplest usage looks like:
```
Outcome oc = Red::matchFull("0123456789", "[0-9]+");
```
This is achieved by implicit construction of a `Red` object from a string.
More straightforward would look like:
```
Red re("[0-9]+");
Outcome oc = Red::matchFull("0123456789", re);
```
This has the advantages of making the compilation step explicit, and
allowing the `Red` object to be reused.  For those who prefer a more
object-oriented approach, the code can look like:
```
Red re("[0-9]+");
Outcome oc = re.matchFull("0123456789");
```
The `Outcome` object has details about what matched where.
Here's an example that loads two regexes into a DFA and then
performs a sliding-window match.
```
Parser p;
p.add("[0-9]+", 1, 0);
p.add("[a-z]+", 2, 0);
Red re(p);
string input = "...foo123...";
Outcome oc = re.searchTangent(s);
string match = input.substr(oc.start_, oc.end_ - oc.start_);
cout << oc.result_ << ':' << match << endl;
```
The output should be:
```
2:foo
```
For more demanding situations, the interfaces in `Matcher.h` are recommended:
```
Red re("<[0-9A-Za-z]+>");
Result rs = scan<stdFirst, false>(re.getExec(), text);
```

## Multi-Pattern

Say one wanted to identify news-worthy cities.
One could load the names of all the cities into a DFA
and then scan every available news article using the DFA.
```
Parser p;
p.add("tokyo",     1, fIgnoreCase);
p.add("shanghai",  2, fIgnoreCase);
p.add("sao paulo", 3, fIgnoreCase);
...
p.add("ashgabat",  10000, fIgnoreCase);
Red re(p);
vector<Outcome> ocs;
for (string_view text : articles)
  if (re.collect(text, ocs) > 0)
    doSomeProcessing(ocs);
```
There are some considerations here.
First, note that the DFA is built with the raw names,
without any leading `.*` or `fLooseStart` flags.
Second, note that the result values are sequential,
avoiding gaps and keeping the numbers as low as possible.
Note also the use of the special API call `collect()`
which is designed for this purpose.
It moves through the input once doing `styLast` matches,
appending all non-overlapping matches to the array.
This is useful to avoid matching "York" when "New York" is seen.

Conversely, if the specific match doesn't matter, using `1` for all
the result values is recommended.

## Orthogonal Naming

There are many ways to process text via a regex.  To avoid the confusion of
ad-hoc naming, such functions are named according to a scheme:

*VERB-QUANTIFIER-STYLE*

*VERB* is inspired by Python usage:

- `match` - process the regex from the start of the text
- `search` - try the regex at successive positions until a match is found
- `replace` - rewrite text based on regex matches

*QUANTIFIER* is just for `replace`:

- `One` - replace just first occurrence
- `All` - replace all occurrences

*STYLE* can be:

- `Instant` - stop processing as soon as an accepting state is reached
- `First` - as instant, but continue as long as the result is the same
- `Tangent` - as instant, but continue while still accepting
- `Last` - process until end, returning last accepting state reached
- `Full` - process entire text against regex, return based on final state

These API functions return `Outcome` structs, and take either
`std::string_view` or C-style `char *` inputs.  They all enable
the leader optimization for fixed prefixes.  More choices and
documentation are availabile in `Matcher.h`.  Specifically, if the
style is known only at run-time, look there.  Also, high-performance
options that return a scalar Result are available, as are ways to
disable the leader optimization.  Let benchmarks be your guide.

Examples:

- `matchFull()` - what `RE2` calls `FullMatch()`
- `matchTangent()` - a less-greedy prefix match
- `matchLast()` - a more conventional prefix match
- `searchInstant()` - appropriate for grep
- `searchTangent()` - what `RE2` calls `PartialMatch()`
- `replaceOneTangent()` - like `RE2` `Replace()`
- `replaceAllLast()` - global longest-match replacement

## Regular Expression Syntax

- Any ordinary character is a regex that matches itself
- `.` matches any one character
- <i>x</i><i>y</i> matches *x* followed by *y* where *x* and *y* are regexes
- *x*`|`*y* matches either *x* or *y*
- `(`*abc*`)` indicates grouping, making a single entity
- *x*`?` means zero or one occurrence of *x*
- *x*`*` means zero or more occurrences of *x*
- *x*`+` means one or more occurrences of *x*
- *x*`{`*n*`}` means exactly *n* occurrences of *x*
- *x*`{`*n*`,`*m*`}` means between *n* and *m* occurrences of *x*
- *x*`{`*n*`,}` means at least *n* occurrences of *x*
- *x*`{,`*m*`}` means zero to *m* occurrences of *x*
- *x*`{,}` means zero or more occurrences of *x*
- `[`...`]` is a character class; **see below**
- `\d` matches any one decimal digit
- `\D` matches any character except a decimal digit
- `\s` matches any whitespace character
- `\S` matches any non-whitespace character
- `\w` matches any alphanumeric character or underscore
- `\W` matches any character except alphanumeric or underscore
- `\0` matches a zero byte (NUL = ASCII 0)
- `\a` matches bell (BEL = ASCII 7)
- `\b` matches backspace (BS = ASCII 8)
- `\n` matches newline (LF = ASCII 10)
- `\r` matches carriage return (CR = ASCII 13)
- `\t` matches tab (HT = ASCII 9)
- `\v` matches vertical tab (VT = ASCII 11)
- `\\` matches backslash (\ = ASCII 92)
- `\x`<i>p</i>*q* matches hexadecimal ASCII character 0x<i>p</i>*q*
- `\i` makes the regular expression case-insensitive
- `^` and `$` are ordinary characters

### Character Classes

Within square brackets, the following syntax is used to decribe a set
or class of characters.  If the first character after the `[` is a
caret (`^`) then the sense of the character class is inverted.

- *a* matches the character *a*
- *abc* matches any of the characters *a*, *b* or *c*
- *a*`-`*b* matches characters in the range from *a* to *b*
- *a*`-`*b*<i>c</i>`-`*d* matches the range from *a* to *b* or *c* to *d*

If a dash comes at the beginning or end of the class specification
(after any caret), the dash character is treated as a normal character.
Similarly, to provide a right-square-bracket (`]`) character,
include it right after the left-square-bracket (or the caret, if present).

For example, `\W` is equivalent to `[^0-9A-Z_a-z]`.
Any of the following backslash-escapes is valid in a character class
and has the meaning described above:
`\0` `\\` `\a` `\b` `\d` `\D` `\n` `\r` `\s` `\S` `\t` `\v` `\w` `\W` `\x`.

## The Outcome Structure

An `Outcome` has three fields:

- `result_` - zero if no match, otherwise,
the `Result` value passed to `Parser::add()` or `addAuto()`
- `start_` - the index at which DFA processing exited the initial state.
This is will be the start of the match for a specific but common case:
a DFA with a single-regex starting with `.*`
followed by something non-optional.
This field may prove useful for other specific DFAs.
- `end_` - the index of the character after the end of the match.
This is a reliable value in all cases.

If `result_` is zero, `start_` and `end_` should come back zero, too.

If the position infomation isn't useful, the `check()` and `scan()` functions
in `Matcher.h` act like `match()` and `search()` but return just a `Result`.
They run faster by avoiding position accounting.

## addAuto()

In the `Parser` class, in addition to `add()`,
there is an `addAuto()` method.  The idea behind this is to simulate
grep-style regular expressions.
The rules of `addAuto()` are simple and fallible:

- `fLooseStart` and `fLooseEnd` are enabled by default
- Leading and trailing `.*` are removed
- Leading `^` disables `fLooseStart` and is removed
- Trailing `$` disables `fLooseEnd` and is removed

Note that this hueristic fails for a regex like `^a|b$`.
Also, `^` and `$` are not special anywhere else.

## The Null Regex

In theory, an empty or null regular expression matches any and all inputs.
This is because a regular expression matches a set of languages and the empty
set is a subset of all languages.

Practically, though, it's less problematic if the empty regex matches the
empty string.  This keeps the invariant that, neglecting special characters,
the regex S exclusively matches the string S.

Parsing, too, becomes clearer.  In the regex `ab`, is there an empty regex
between `a` and `b`?  Do we want `(a|)` or `()` to match all inputs?
What about an extra bar, as in `a|b||c`?

## Safety

To avoid denial-of-service (DoS) attacks, don't compile user-provided input
unless the user is very trustworthy.

It's actually very easy to use up inordinate amounts of CPU and memory
in the process of converting the initial NFA built by the parser into
the DFA used for matching.  This regex `.{,10000}a` takes
5GB of memory and 3 hours of CPU to compile on a 2.6GHz i7.
