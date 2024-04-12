#include "kernel/cost.h"

USING_YOSYS_NAMESPACE

int CellCosts::get(RTLIL::Module *mod)
{
    if (mod->attributes.count(ID(cost)))
        return mod->attributes.at(ID(cost)).as_int();

    if (mod_cost_cache_.count(mod->name))
        return mod_cost_cache_.at(mod->name);

    int module_cost = 1;
    for (auto c : mod->cells())
        module_cost += get(c);

    mod_cost_cache_[mod->name] = module_cost;
    return module_cost;
}

static int y_coef(RTLIL::IdString type)
{
    if (type == ID('$bweqx') ||
        type == ID('$nex') ||
        type == ID('$eqx') ||
        type == ID('$and') ||
        type == ID('$or') ||
        type == ID('$xor') ||
        type == ID('$xnor') ||
        type == ID('$not'))
        return 1;
    else if (
        type == ID('$neg')
    )
        return 3;
    return 0;
}

static int sum_coef(RTLIL::IdString type)
{
    if (type == ID('$reduce_and') ||
        type == ID('$reduce_or') ||
        type == ID('$reduce_xor') ||
        type == ID('$reduce_xnor') ||
        type == ID('$reduce_bool'))
        return 1;

    return 0;
}

int CellCosts::get(RTLIL::Cell *cell)
{
    if (gate_type_cost().count(cell->type))
        return gate_type_cost().at(cell->type);

    if (design_ && design_->module(cell->type) && cell->parameters.empty())
    {
        return get(design_->module(cell->type));
    } else if (RTLIL::builtin_ff_cell_types().count(cell->type)) {
        log_assert(cell->hasPort(ID::Q) && "Weird flip flop");
        return cell->getParam(ID::WIDTH).as_int();
    } else if (y_coef(cell->type)) {
        // linear with Y_WIDTH
        log_assert(cell->hasParam(ID::Y_WIDTH) && "No Y port");
        int width = cell->getParam(ID::Y_WIDTH).as_int();
        return width * y_coef(cell->type);
    } else if (sum_coef(cell->type)) {
        // linear with sum of port widths
        int sum = 0;

        if (cell->hasParam(ID::A_WIDTH))
            sum += cell->getParam(ID::A_WIDTH).as_int();
        if (cell->hasParam(ID::B_WIDTH))
            sum += cell->getParam(ID::B_WIDTH).as_int();
        if (cell->hasParam(ID::S_WIDTH))
            sum += cell->getParam(ID::S_WIDTH).as_int();
        if (cell->hasParam(ID::Y_WIDTH))
            sum += cell->getParam(ID::Y_WIDTH).as_int();
        if (cell->hasParam(ID::WIDTH))
            sum += cell->getParam(ID::WIDTH).as_int();


        // TODO CTRL_IN $fsm ?
        // TODO CTRL_OUT $fsm ?

        return sum * sum_coef(cell->type);
    } else if (
        cell->type == ID('$overwrite_tag') ||
        cell->type == ID('$set_tag') ||
        cell->type == ID('$original_tag') ||
        cell->type == ID('$get_tag') ||
        cell->type == ID('$check') ||
        cell->type == ID('$equiv') ||
        cell->type == ID('$initstate') ||
        cell->type == ID('$assert') ||
        cell->type == ID('$assume') ||
        cell->type == ID('$live') ||
        cell->type == ID('$cover') ||
        cell->type == ID('$allseq') ||
        cell->type == ID('$allconst') ||
        cell->type == ID('$anyseq') ||
        cell->type == ID('$anyinit') ||
        cell->type == ID('$anyconst') ||
        cell->type == ID('$fair') ||
        cell->type == ID('$scopeinfo') ||
        cell->type == ID('$print') ||
        cell->type == ID('$concat') ||
        cell->type == ID('$slice') ||
        cell->type == ID('$pos')
    ) {
        return 0;
    }

    log_warning("Can't determine cost of %s cell (%d parameters).\n", log_id(cell->type), GetSize(cell->parameters));
    return 1;
}


