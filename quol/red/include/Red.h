/* Red.h - main public header for RED - Regular Expression DFA

   API Usage:
   ----------

   The simplest usage would look like:

     Outcome oc = Red::matchFull("0123456789", "[0-9]+");

   This is achieved by implicit construction of a Red object from a string.
   More straightforward would look like:

     Red re("[0-9]+");
     Outcome oc = Red::matchFull("0123456789", re);

   This has the advantages of making the compilation step separate, and
   allowing the Red object to be reused.  For those who prefer a more
   object-oriented approach, the code can look like:

     Red re("[0-9]+");
     Outcome oc = re.matchFull("0123456789");

   Most calls to Red functions may throw RedExcept.

   Orthogonal Naming:
   ------------------

   There are many ways to process text via a regex.  To avoid the confusion of
   ad-hoc naming, such functions are named according to a scheme:

   <VERB><?QUANTIFIER?><STYLE>

   VERB is inspired by Python usage:

   - match:   process the regex from the start of the text
   - search:  try the regex at successive positions until a match is found
   - replace: rewrite text based on regex matches

   QUANTIFIER is just for 'replace':

   - One: replace just first occurrence
   - All: replace all occurrences

   STYLE can be:

   - Instant: stop processing as soon as an accepting state is reached
   - First:   as instant, but continue as long as the result is the same
   - Tangent: as instant, but continue while still accepting
   - Last:    process until end, returning last accepting state reached
   - Full:    process entire text against regex, return based on final state

   These API functions return Outcome structs, and take either
   std::string_view or C-style char pointer inputs.  They all enable
   the leader optimization for fixed prefixes.  More choices and
   documentation are availabile in Matcher.h.  Specifically, if the
   style is known only at run-time, look there.  Also, high-performance
   options that return a scalar Result are available, as are ways to
   disable the leader optimization.  Let benchmarks be your guide.

   Examples:

   matchFull()         - what RE2 calls FullMatch()
   matchTangent()      - a less-greedy prefix match
   matchLast()         - a more conventional prefix match
   searchInstant()     - appropriate for grep
   searchTangent()     - what RE2 calls PartialMatch()
   replaceOneTangent() - like RE2 Replace()
   replaceAllLast()    - global longest-match replacement
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Outcome.h"
#include "Executable.h"

namespace zezax::red {

class Parser;

class Red {
public:
  Red() = delete;
  Red(const Red &) = delete;
  Red &operator=(const Red &) = delete;

  Red(Red &&other) = default;
  Red &operator=(Red &&rhs) = default;

  // these are meant for implicit conversion...
  Red(const char *regex);
  Red(const std::string &regex);
  Red(std::string_view regex);
  Red(std::string_view regex, Flags flags);
  Red(Parser &parser);

  // these accept serialized compiled regexes
  Red(std::string &&prog);
  Red(const CopyTag &, const std::string &prog);
  Red(const CopyTag &, std::string_view prog);
  Red(const CopyTag &, const void *prog, size_t len);
  Red(const DeleteTag &, const void *prog, size_t len);
  Red(const FreeTag &, const void *prog, size_t len);
  Red(const UnownedTag &, std::string_view prog);
  Red(const UnownedTag &, const void *prog, size_t len);
  Red(const PathTag &, const char *path);

  void save(const char *path) const;
  std::string_view serialized() const { return program_.serialized(); }

  // reports all non-overlapping matches in text, in order
  size_t collect(std::string_view text, std::vector<Outcome> &out);
  size_t collect(const char *text, std::vector<Outcome> &out);

  // orthogonal api...
  Outcome matchInstant(std::string_view text) const;
  Outcome matchInstant(const char *text) const;
  Outcome matchFirst(std::string_view text) const;
  Outcome matchFirst(const char *text) const;
  Outcome matchTangent(std::string_view text) const;
  Outcome matchTangent(const char *text) const;
  Outcome matchLast(std::string_view text) const;
  Outcome matchLast(const char *text) const;
  Outcome matchFull(std::string_view text) const;
  Outcome matchFull(const char *text) const;
  Outcome searchInstant(std::string_view text) const;
  Outcome searchInstant(const char *text) const;
  Outcome searchFirst(std::string_view text) const;
  Outcome searchFirst(const char *text) const;
  Outcome searchTangent(std::string_view text) const;
  Outcome searchTangent(const char *text) const;
  Outcome searchLast(std::string_view text) const;
  Outcome searchLast(const char *text) const;
  Outcome searchFull(std::string_view text) const;
  Outcome searchFull(const char *text) const;
  size_t replaceOneInstant(std::string_view text, std::string_view repl,
                           std::string &out) const;
  size_t replaceOneInstant(const char *text, std::string_view repl,
                           std::string &out) const;
  size_t replaceOneFirst(std::string_view text, std::string_view repl,
                         std::string &out) const;
  size_t replaceOneFirst(const char *text, std::string_view repl,
                         std::string &out) const;
  size_t replaceOneTangent(std::string_view text, std::string_view repl,
                           std::string &out) const;
  size_t replaceOneTangent(const char *text, std::string_view repl,
                           std::string &out) const;
  size_t replaceOneLast(std::string_view text, std::string_view repl,
                        std::string &out) const;
  size_t replaceOneLast(const char *text, std::string_view repl,
                        std::string &out) const;
  size_t replaceOneFull(std::string_view text, std::string_view repl,
                        std::string &out) const;
  size_t replaceOneFull(const char *text, std::string_view repl,
                        std::string &out) const;
  size_t replaceAllInstant(std::string_view text, std::string_view repl,
                           std::string &out) const;
  size_t replaceAllInstant(const char *text, std::string_view repl,
                           std::string &out) const;
  size_t replaceAllFirst(std::string_view text, std::string_view repl,
                         std::string &out) const;
  size_t replaceAllFirst(const char *text, std::string_view repl,
                         std::string &out) const;
  size_t replaceAllTangent(std::string_view text, std::string_view repl,
                           std::string &out) const;
  size_t replaceAllTangent(const char *text, std::string_view repl,
                           std::string &out) const;
  size_t replaceAllLast(std::string_view text, std::string_view repl,
                        std::string &out) const;
  size_t replaceAllLast(const char *text, std::string_view repl,
                        std::string &out) const;
  size_t replaceAllFull(std::string_view text, std::string_view repl,
                        std::string &out) const;
  size_t replaceAllFull(const char *text, std::string_view repl,
                        std::string &out) const;

  static Outcome matchInstant(std::string_view text, const Red &re);
  static Outcome matchInstant(const char *text, const Red &re);
  static Outcome matchFirst(std::string_view text, const Red &re);
  static Outcome matchFirst(const char *text, const Red &re);
  static Outcome matchTangent(std::string_view text, const Red &re);
  static Outcome matchTangent(const char *text, const Red &re);
  static Outcome matchLast(std::string_view text, const Red &re);
  static Outcome matchLast(const char *text, const Red &re);
  static Outcome matchFull(std::string_view text, const Red &re);
  static Outcome matchFull(const char *text, const Red &re);
  static Outcome searchInstant(std::string_view text, const Red &re);
  static Outcome searchInstant(const char *text, const Red &re);
  static Outcome searchFirst(std::string_view text, const Red &re);
  static Outcome searchFirst(const char *text, const Red &re);
  static Outcome searchTangent(std::string_view text, const Red &re);
  static Outcome searchTangent(const char *text, const Red &re);
  static Outcome searchLast(std::string_view text, const Red &re);
  static Outcome searchLast(const char *text, const Red &re);
  static Outcome searchFull(std::string_view text, const Red &re);
  static Outcome searchFull(const char *text, const Red &re);
  static size_t replaceOneInstant(std::string_view text, const Red &re,
                                  std::string_view repl, std::string &out);
  static size_t replaceOneInstant(const char *text, const Red &re,
                                  std::string_view repl, std::string &out);
  static size_t replaceOneFirst(std::string_view text, const Red &re,
                                std::string_view repl, std::string &out);
  static size_t replaceOneFirst(const char *text, const Red &re,
                                std::string_view repl, std::string &out);
  static size_t replaceOneTangent(std::string_view text, const Red &re,
                                  std::string_view repl, std::string &out);
  static size_t replaceOneTangent(const char *text, const Red &re,
                                  std::string_view repl, std::string &out);
  static size_t replaceOneLast(std::string_view text, const Red &re,
                               std::string_view repl, std::string &out);
  static size_t replaceOneLast(const char *text, const Red &re,
                               std::string_view repl, std::string &out);
  static size_t replaceOneFull(std::string_view text, const Red &re,
                               std::string_view repl, std::string &out);
  static size_t replaceOneFull(const char *text, const Red &re,
                               std::string_view repl, std::string &out);
  static size_t replaceAllInstant(std::string_view text, const Red &re,
                                  std::string_view repl, std::string &out);
  static size_t replaceAllInstant(const char *text, const Red &re,
                                  std::string_view repl, std::string &out);
  static size_t replaceAllFirst(std::string_view text, const Red &re,
                                std::string_view repl, std::string &out);
  static size_t replaceAllFirst(const char *text, const Red &re,
                                std::string_view repl, std::string &out);
  static size_t replaceAllTangent(std::string_view text, const Red &re,
                                  std::string_view repl, std::string &out);
  static size_t replaceAllTangent(const char *text, const Red &re,
                                  std::string_view repl, std::string &out);
  static size_t replaceAllLast(std::string_view text, const Red &re,
                               std::string_view repl, std::string &out);
  static size_t replaceAllLast(const char *text, const Red &re,
                               std::string_view repl, std::string &out);
  static size_t replaceAllFull(std::string_view text, const Red &re,
                               std::string_view repl, std::string &out);
  static size_t replaceAllFull(const char *text, const Red &re,
                               std::string_view repl, std::string &out);

  // RE2 style...
  static Outcome fullMatch(std::string_view text, const Red &re);
  static Outcome partialMatch(std::string_view text, const Red &re);
  static Result consume(std::string_view &text, const Red &re);
  static Result findAndConsume(std::string_view &text, const Red &re);
  static size_t replace(std::string      *text,
                        const Red        &re,
                        std::string_view  repl);
  static size_t globalReplace(std::string      *text,
                              const Red        &re,
                              std::string_view  repl);

  // sort of like RE2::Set::Match()...
  size_t allMatches(std::string_view text, std::vector<Outcome> *out);

  // to access more powerful interfaces in Matcher.h
  const Executable &getExec() const { return program_; }

private:
  Executable program_;
};

} // namespace zezax::red
