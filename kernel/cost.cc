#include "kernel/cost.h"

USING_YOSYS_NAMESPACE

CellCosts::CellCosts(CellCosts::CostKind kind, RTLIL::Design *design) : kind(kind), design(design) {
}


