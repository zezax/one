// Main public implementation for RED - Regular Expression DFA
// See 'apigen' in scripts directory for repetitive code generation

#include "Red.h"

#include <limits>

#include "Parser.h"
#include "Compile.h"
#include "Matcher.h"
#include "Util.h"

namespace zezax::red {

using std::numeric_limits;
using std::string;
using std::string_view;
using std::vector;

Red::Red(const char *regex) {
  Parser p;
  p.add(regex, 1, 0);
  program_ = compile(p);
}


Red::Red(const string &regex) {
  Parser p;
  p.add(regex, 1, 0);
  program_ = compile(p);
}


Red::Red(string_view regex) {
  Parser p;
  p.add(regex, 1, 0);
  program_ = compile(p);
}


Red::Red(string_view regex, Flags flags) {
  Parser p;
  p.add(regex, 1, flags);
  program_ = compile(p);
}


Red::Red(Parser &parser) { // for power users
  program_ = compile(parser);
}


Red::Red(string &&prog) : program_(std::move(prog)) {}


Red::Red(const CopyTag &, const string &prog) : program_(gCopyTag, prog) {}


Red::Red(const CopyTag &, string_view prog) : program_(gCopyTag, prog) {}


Red::Red(const CopyTag &, const void *prog, size_t len)
  : program_(gCopyTag, string_view(static_cast<const char *>(prog), len)) {}


Red::Red(const DeleteTag &, const void *prog, size_t len)
  : program_(gDeleteTag, prog, len) {}


Red::Red(const FreeTag &, const void *prog, size_t len)
  : program_(gFreeTag, prog, len) {}


Red::Red(const UnownedTag &, string_view prog)
  : program_(gUnownedTag, prog) {}


Red::Red(const UnownedTag &, const void *prog, size_t len)
  : program_(gUnownedTag, string_view(static_cast<const char *>(prog), len)) {}


Red::Red(const PathTag &, const char *path) {
  string buf = readFileToString(path);
  program_ = Executable(std::move(buf));
}


void Red::save(const char *path) const {
  string_view sv = program_.serialized();
  writeStringToFile(sv, path);
}


size_t Red::collect(string_view text, vector<Outcome> &out) {
  out.clear();
  const char *beg = text.data();
  const char *end = beg + text.size();
  for (const char *ptr = beg; ptr < end; ) {
    Outcome oc = search<styLast, false>(program_, ptr, end - ptr);
    if (!oc)
      break;
    size_t off = ptr - beg;
    out.emplace_back(Outcome{oc.result_, off + oc.start_, off + oc.end_});
    ptr += oc.end_;
  }
  return out.size();
}


size_t Red::collect(const char *text, vector<Outcome> &out) {
  out.clear();
  for (const char *ptr = text; *ptr; ) {
    Outcome oc = search<styLast, false>(program_, ptr);
    if (!oc)
      break;
    size_t off = ptr - text;
    out.emplace_back(Outcome{oc.result_, off + oc.start_, off + oc.end_});
    ptr += oc.end_;
  }
  return out.size();
}

///////////////////////////////////////////////////////////////////////////////
//
// ORTHOGONAL API
//
///////////////////////////////////////////////////////////////////////////////

Outcome Red::matchInstant(string_view text) const {
  return match<styInstant, true>(program_, text);
}


Outcome Red::matchInstant(const char *text) const {
  return match<styInstant, true>(program_, text);
}


Outcome Red::matchFirst(string_view text) const {
  return match<styFirst, true>(program_, text);
}


Outcome Red::matchFirst(const char *text) const {
  return match<styFirst, true>(program_, text);
}


Outcome Red::matchTangent(string_view text) const {
  return match<styTangent, true>(program_, text);
}


Outcome Red::matchTangent(const char *text) const {
  return match<styTangent, true>(program_, text);
}


Outcome Red::matchLast(string_view text) const {
  return match<styLast, true>(program_, text);
}


Outcome Red::matchLast(const char *text) const {
  return match<styLast, true>(program_, text);
}


Outcome Red::matchFull(string_view text) const {
  return match<styFull, true>(program_, text);
}


Outcome Red::matchFull(const char *text) const {
  return match<styFull, true>(program_, text);
}


Outcome Red::searchInstant(string_view text) const {
  return search<styInstant, true>(program_, text);
}


Outcome Red::searchInstant(const char *text) const {
  return search<styInstant, true>(program_, text);
}


Outcome Red::searchFirst(string_view text) const {
  return search<styFirst, true>(program_, text);
}


Outcome Red::searchFirst(const char *text) const {
  return search<styFirst, true>(program_, text);
}


Outcome Red::searchTangent(string_view text) const {
  return search<styTangent, true>(program_, text);
}


Outcome Red::searchTangent(const char *text) const {
  return search<styTangent, true>(program_, text);
}


Outcome Red::searchLast(string_view text) const {
  return search<styLast, true>(program_, text);
}


Outcome Red::searchLast(const char *text) const {
  return search<styLast, true>(program_, text);
}


Outcome Red::searchFull(string_view text) const {
  return search<styFull, true>(program_, text);
}


Outcome Red::searchFull(const char *text) const {
  return search<styFull, true>(program_, text);
}


size_t Red::replaceOneInstant(string_view text,
                              string_view repl,
                              string &out) const {
  return red::replace<styInstant, true>(program_, text, repl, out, 1);
}


size_t Red::replaceOneInstant(const char *text,
                              string_view repl,
                              string &out) const {
  return red::replace<styInstant, true>(program_, text, repl, out, 1);
}


size_t Red::replaceOneFirst(string_view text,
                            string_view repl,
                            string &out) const {
  return red::replace<styFirst, true>(program_, text, repl, out, 1);
}


size_t Red::replaceOneFirst(const char *text,
                            string_view repl,
                            string &out) const {
  return red::replace<styFirst, true>(program_, text, repl, out, 1);
}


size_t Red::replaceOneTangent(string_view text,
                              string_view repl,
                              string &out) const {
  return red::replace<styTangent, true>(program_, text, repl, out, 1);
}


size_t Red::replaceOneTangent(const char *text,
                              string_view repl,
                              string &out) const {
  return red::replace<styTangent, true>(program_, text, repl, out, 1);
}


size_t Red::replaceOneLast(string_view text,
                           string_view repl,
                           string &out) const {
  return red::replace<styLast, true>(program_, text, repl, out, 1);
}


size_t Red::replaceOneLast(const char *text,
                           string_view repl,
                           string &out) const {
  return red::replace<styLast, true>(program_, text, repl, out, 1);
}


size_t Red::replaceOneFull(string_view text,
                           string_view repl,
                           string &out) const {
  return red::replace<styFull, true>(program_, text, repl, out, 1);
}


size_t Red::replaceOneFull(const char *text,
                           string_view repl,
                           string &out) const {
  return red::replace<styFull, true>(program_, text, repl, out, 1);
}


size_t Red::replaceAllInstant(string_view text,
                              string_view repl,
                              string &out) const {
  return red::replace<styInstant, true>(program_, text, repl, out,
                                        numeric_limits<size_t>::max());
}


size_t Red::replaceAllInstant(const char *text,
                              string_view repl,
                              string &out) const {
  return red::replace<styInstant, true>(program_, text, repl, out,
                                        numeric_limits<size_t>::max());
}


size_t Red::replaceAllFirst(string_view text,
                            string_view repl,
                            string &out) const {
  return red::replace<styFirst, true>(program_, text, repl, out,
                                      numeric_limits<size_t>::max());
}


size_t Red::replaceAllFirst(const char *text,
                            string_view repl,
                            string &out) const {
  return red::replace<styFirst, true>(program_, text, repl, out,
                                      numeric_limits<size_t>::max());
}


size_t Red::replaceAllTangent(string_view text,
                              string_view repl,
                              string &out) const {
  return red::replace<styTangent, true>(program_, text, repl, out,
                                        numeric_limits<size_t>::max());
}


size_t Red::replaceAllTangent(const char *text,
                              string_view repl,
                              string &out) const {
  return red::replace<styTangent, true>(program_, text, repl, out,
                                        numeric_limits<size_t>::max());
}


size_t Red::replaceAllLast(string_view text,
                           string_view repl,
                           string &out) const {
  return red::replace<styLast, true>(program_, text, repl, out,
                                     numeric_limits<size_t>::max());
}


size_t Red::replaceAllLast(const char *text,
                           string_view repl,
                           string &out) const {
  return red::replace<styLast, true>(program_, text, repl, out,
                                     numeric_limits<size_t>::max());
}


size_t Red::replaceAllFull(string_view text,
                           string_view repl,
                           string &out) const {
  return red::replace<styFull, true>(program_, text, repl, out,
                                     numeric_limits<size_t>::max());
}


size_t Red::replaceAllFull(const char *text,
                           string_view repl,
                           string &out) const {
  return red::replace<styFull, true>(program_, text, repl, out,
                                     numeric_limits<size_t>::max());
}


/* static */ Outcome Red::matchInstant(string_view text, const Red &re) {
  return match<styInstant, true>(re.program_, text);
}


/* static */ Outcome Red::matchInstant(const char *text, const Red &re) {
  return match<styInstant, true>(re.program_, text);
}


/* static */ Outcome Red::matchFirst(string_view text, const Red &re) {
  return match<styFirst, true>(re.program_, text);
}


/* static */ Outcome Red::matchFirst(const char *text, const Red &re) {
  return match<styFirst, true>(re.program_, text);
}


/* static */ Outcome Red::matchTangent(string_view text, const Red &re) {
  return match<styTangent, true>(re.program_, text);
}


/* static */ Outcome Red::matchTangent(const char *text, const Red &re) {
  return match<styTangent, true>(re.program_, text);
}


/* static */ Outcome Red::matchLast(string_view text, const Red &re) {
  return match<styLast, true>(re.program_, text);
}


/* static */ Outcome Red::matchLast(const char *text, const Red &re) {
  return match<styLast, true>(re.program_, text);
}


/* static */ Outcome Red::matchFull(string_view text, const Red &re) {
  return match<styFull, true>(re.program_, text);
}


/* static */ Outcome Red::matchFull(const char *text, const Red &re) {
  return match<styFull, true>(re.program_, text);
}


/* static */ Outcome Red::searchInstant(string_view text, const Red &re) {
  return search<styInstant, true>(re.program_, text);
}


/* static */ Outcome Red::searchInstant(const char *text, const Red &re) {
  return search<styInstant, true>(re.program_, text);
}


/* static */ Outcome Red::searchFirst(string_view text, const Red &re) {
  return search<styFirst, true>(re.program_, text);
}


/* static */ Outcome Red::searchFirst(const char *text, const Red &re) {
  return search<styFirst, true>(re.program_, text);
}


/* static */ Outcome Red::searchTangent(string_view text, const Red &re) {
  return search<styTangent, true>(re.program_, text);
}


/* static */ Outcome Red::searchTangent(const char *text, const Red &re) {
  return search<styTangent, true>(re.program_, text);
}


/* static */ Outcome Red::searchLast(string_view text, const Red &re) {
  return search<styLast, true>(re.program_, text);
}


/* static */ Outcome Red::searchLast(const char *text, const Red &re) {
  return search<styLast, true>(re.program_, text);
}


/* static */ Outcome Red::searchFull(string_view text, const Red &re) {
  return search<styFull, true>(re.program_, text);
}


/* static */ Outcome Red::searchFull(const char *text, const Red &re) {
  return search<styFull, true>(re.program_, text);
}


/* static */ size_t Red::replaceOneInstant(string_view text,
                                           const Red &re,
                                           string_view repl,
                                           string &out) {
  return red::replace<styInstant, true>(re.program_, text, repl, out, 1);
}


/* static */ size_t Red::replaceOneInstant(const char *text,
                                           const Red &re,
                                           string_view repl,
                                           string &out) {
  return red::replace<styInstant, true>(re.program_, text, repl, out, 1);
}


/* static */ size_t Red::replaceOneFirst(string_view text,
                                         const Red &re,
                                         string_view repl,
                                         string &out) {
  return red::replace<styFirst, true>(re.program_, text, repl, out, 1);
}


/* static */ size_t Red::replaceOneFirst(const char *text,
                                         const Red &re,
                                         string_view repl,
                                         string &out) {
  return red::replace<styFirst, true>(re.program_, text, repl, out, 1);
}


/* static */ size_t Red::replaceOneTangent(string_view text,
                                           const Red &re,
                                           string_view repl,
                                           string &out) {
  return red::replace<styTangent, true>(re.program_, text, repl, out, 1);
}


/* static */ size_t Red::replaceOneTangent(const char *text,
                                           const Red &re,
                                           string_view repl,
                                           string &out) {
  return red::replace<styTangent, true>(re.program_, text, repl, out, 1);
}


/* static */ size_t Red::replaceOneLast(string_view text,
                                        const Red &re,
                                        string_view repl,
                                        string &out) {
  return red::replace<styLast, true>(re.program_, text, repl, out, 1);
}


/* static */ size_t Red::replaceOneLast(const char *text,
                                        const Red &re,
                                        string_view repl,
                                        string &out) {
  return red::replace<styLast, true>(re.program_, text, repl, out, 1);
}


/* static */ size_t Red::replaceOneFull(string_view text,
                                        const Red &re,
                                        string_view repl,
                                        string &out) {
  return red::replace<styFull, true>(re.program_, text, repl, out, 1);
}


/* static */ size_t Red::replaceOneFull(const char *text,
                                        const Red &re,
                                        string_view repl,
                                        string &out) {
  return red::replace<styFull, true>(re.program_, text, repl, out, 1);
}


/* static */ size_t Red::replaceAllInstant(string_view text,
                                           const Red &re,
                                           string_view repl,
                                           string &out) {
  return red::replace<styInstant, true>(re.program_, text, repl, out,
                                        numeric_limits<size_t>::max());
}


/* static */ size_t Red::replaceAllInstant(const char *text,
                                           const Red &re,
                                           string_view repl,
                                           string &out) {
  return red::replace<styInstant, true>(re.program_, text, repl, out,
                                        numeric_limits<size_t>::max());
}


/* static */ size_t Red::replaceAllFirst(string_view text,
                                         const Red &re,
                                         string_view repl,
                                         string &out) {
  return red::replace<styFirst, true>(re.program_, text, repl, out,
                                      numeric_limits<size_t>::max());
}


/* static */ size_t Red::replaceAllFirst(const char *text,
                                         const Red &re,
                                         string_view repl,
                                         string &out) {
  return red::replace<styFirst, true>(re.program_, text, repl, out,
                                      numeric_limits<size_t>::max());
}


/* static */ size_t Red::replaceAllTangent(string_view text,
                                           const Red &re,
                                           string_view repl,
                                           string &out) {
  return red::replace<styTangent, true>(re.program_, text, repl, out,
                                        numeric_limits<size_t>::max());
}


/* static */ size_t Red::replaceAllTangent(const char *text,
                                           const Red &re,
                                           string_view repl,
                                           string &out) {
  return red::replace<styTangent, true>(re.program_, text, repl, out,
                                        numeric_limits<size_t>::max());
}


/* static */ size_t Red::replaceAllLast(string_view text,
                                        const Red &re,
                                        string_view repl,
                                        string &out) {
  return red::replace<styLast, true>(re.program_, text, repl, out,
                                     numeric_limits<size_t>::max());
}


/* static */ size_t Red::replaceAllLast(const char *text,
                                        const Red &re,
                                        string_view repl,
                                        string &out) {
  return red::replace<styLast, true>(re.program_, text, repl, out,
                                     numeric_limits<size_t>::max());
}


/* static */ size_t Red::replaceAllFull(string_view text,
                                        const Red &re,
                                        string_view repl,
                                        string &out) {
  return red::replace<styFull, true>(re.program_, text, repl, out,
                                     numeric_limits<size_t>::max());
}


/* static */ size_t Red::replaceAllFull(const char *text,
                                        const Red &re,
                                        string_view repl,
                                        string &out) {
  return red::replace<styFull, true>(re.program_, text, repl, out,
                                     numeric_limits<size_t>::max());
}

///////////////////////////////////////////////////////////////////////////////
//
// COMPATIBILITY
//
///////////////////////////////////////////////////////////////////////////////

/* static */ Outcome Red::fullMatch(string_view text, const Red &re) {
  return match<styFull, true>(re.program_, text);
}


/* static */ Outcome Red::partialMatch(string_view text, const Red &re) {
  return search<styTangent, true>(re.program_, text);
}


/* static */ Result Red::consume(string_view &text, const Red &re) {
  Outcome oc = match<styTangent, true>(re.program_, text);
  if (oc)
    text.remove_prefix(oc.end_);
  return oc.result_;
}


/* static */ Result Red::findAndConsume(string_view &text, const Red &re) {
  Outcome oc = search<styTangent, true>(re.program_, text);
  if (oc)
    text.remove_prefix(oc.end_);
  return oc.result_;
}


/* static */ size_t Red::replace(string      *text,
                                 const Red   &re,
                                 string_view  repl) {
  string out;
  size_t n = red::replace<styTangent, true>(re.program_, *text, repl, out, 1);
  if (n)
    *text = out;
  return n;
}


/* static */ size_t Red::globalReplace(string      *text,
                                       const Red   &re,
                                       string_view  repl) {
  string out;
  size_t n = red::replace<styTangent, true>(re.program_, *text, repl, out,
                                            numeric_limits<size_t>::max());
  if (n)
    *text = out;
  return n;
}


// like RE2::Set::Match()
size_t Red::allMatches(string_view text, vector<Outcome> *out) {
  return matchAll(program_, text, *out);
}

} // namespace zezax::red
