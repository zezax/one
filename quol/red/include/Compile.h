/* Compile.h - compilation conveniences header

   These convenience functions start with a populated Parser and yield
   a DFA encapsulated either as Executable or as serialized bytes.
   They handle powerset conversion, DFA minimization and serialization
   into the efficient executable format.

   There is no need to call finalize() on the parser.  Any Budget or
   CompStats pointers given to the parser will be propagated through
   the subsequent compilation stages.

   Usage is like:

   Parser p;
   p.add("foo.*bar", 1, 0);
   Executable exec = compile(p);
   Result res = check(exec, "foolsbar", styFull);
 */

#pragma once

#include "Parser.h"
#include "Serializer.h"
#include "Executable.h"

namespace zezax::red {

Executable compile(Parser &rp, Format fmt = fmtDirectAuto);

std::string compileToSerialized(Parser &rp, Format fmt = fmtDirectAuto);

} // namespace zezax::red
