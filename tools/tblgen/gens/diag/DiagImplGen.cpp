// Custom TableGen backend for generating diagnostic implementations.
// Reads Diagnostics.td and emits Diagnostics.cpp
#include "DiagImplGen.hpp"

DiagImplGen::DiagImplGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: DiagGen(os, records, genName, "lbc", {}) {
}

auto DiagImplGen::run() -> bool {
    // TODO: implement diagnostic cpp generation
    return false;
}
