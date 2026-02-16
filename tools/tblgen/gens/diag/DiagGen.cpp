// Custom TableGen backend for generating diagnostic definitions.
// Reads Diagnostics.td and emits Diagnostics.hpp
#include "DiagGen.hpp"

DiagGen::DiagGen(
    raw_ostream& os,
    const RecordKeeper& records,
    StringRef generator,
    StringRef ns,
    std::vector<StringRef> includes
)
: GeneratorBase(os, records, generator, ns, std::move(includes)) {
}

auto DiagGen::run() -> bool {
    // TODO: implement diagnostic header generation
    return false;
}
