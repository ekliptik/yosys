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
	// clang-format off
    if (// equality
        type == ID('$bweqx') ||
        type == ID('$nex') ||
        type == ID('$eqx') ||
        // basic logic
        type == ID('$and') ||
        type == ID('$or') ||
        type == ID('$xor') ||
        type == ID('$xnor') ||
        type == ID('$not') ||
        // mux
        type == ID('$bwmux') ||
        type == ID('$mux') ||
        type == ID('$demux') ||
        // others
        type == ID('$tribuf')) {
        return 1;
    } else if (type == ID('$neg')) {
        return 4;
    } else if (type == ID('$fa')) {
        return 5;
    } else if (// multi-bit adders
        type == ID('$add') ||
        type == ID('$sub') ||
        type == ID('$alu')) {
        return 8;
    } else if (// left shift
        type == ID('$shl') ||
        type == ID('$sshl')) {
        return 10;
    }
	// clang-format on
	return 0;
}

static int max_inp_coef(RTLIL::IdString type)
{
	// clang-format off
    if (// binop reduce
        type == ID('$reduce_and') ||
        type == ID('$reduce_or') ||
        type == ID('$reduce_xor') ||
        type == ID('$reduce_xnor') ||
        type == ID('$reduce_bool') ||
        // others
        type == ID('$logic_not') ||
        type == ID('$bmux')) {
        return 1;
    } else if (// equality
        type == ID('$eq') ||
        type == ID('$ne') ||
        // logic
        type == ID('$logic_and') ||
        type == ID('$logic_or')) {
        return 2;
    } else if (type == ID('$lcu')) {
        return 5;
    } else if (// comparison
        type == ID('$lt') ||
        type == ID('$le') ||
        type == ID('$ge') ||
        type == ID('$gt') ||
        // others
        type == ID('$sop')) {
        return 6;
	}
	// clang-format on
	return 0;
}
static int sum_coef(RTLIL::IdString type)
{
	// clang-format off
    if (// right shift
        type == ID('$shr') ||
        type == ID('$sshr')) {
        return 4;
    } else if (// shift
        type == ID('$shift') ||
        type == ID('$shiftx')) {
        return 8;
	}
    // clang-format on

	return 0;
}

static bool is_free(RTLIL::IdString type)
{
	// clang-format off
    return (// tags
        type == ID('$overwrite_tag') ||
        type == ID('$set_tag') ||
        type == ID('$original_tag') ||
        type == ID('$get_tag') ||
        // formal
        type == ID('$check') ||
        type == ID('$equiv') ||
        type == ID('$initstate') ||
        type == ID('$assert') ||
        type == ID('$assume') ||
        type == ID('$live') ||
        type == ID('$cover') ||
        type == ID('$allseq') ||
        type == ID('$allconst') ||
        type == ID('$anyseq') ||
        type == ID('$anyinit') ||
        type == ID('$anyconst') ||
        type == ID('$fair') ||
        // utilities
        type == ID('$scopeinfo') ||
        type == ID('$print') ||
        // real but free
        type == ID('$concat') ||
        type == ID('$slice') ||
        type == ID('$pos') ||
        // specify
        type == ID('$specrule') ||
        type == ID('$specify2') ||
        type == ID('$specify3'));
	// clang-format on
}

static const RTLIL::IdString port_width_params[] = {
  ID::WIDTH, ID::A_WIDTH, ID::B_WIDTH, ID::S_WIDTH, ID::Y_WIDTH,
};

// Ditto but without Y_WIDTH
// No neat solution available: https://stackoverflow.com/questions/3154170/combine-two-constant-strings-or-arrays-into-one-constant-string-or-array-at
static const RTLIL::IdString input_width_params[] = {
  ID::WIDTH,
  ID::A_WIDTH,
  ID::B_WIDTH,
  ID::S_WIDTH,
};

int CellCosts::get(RTLIL::Cell *cell)
{

	if (gate_type_cost().count(cell->type))
		return gate_type_cost().at(cell->type);

	if (design_ && design_->module(cell->type) && cell->parameters.empty()) {
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
		for (auto param : port_width_params)
			if (cell->hasParam(param))
				sum += cell->getParam(param).as_int();

		return sum * sum_coef(cell->type);
	} else if (max_inp_coef(cell->type)) {
		// linear with largest input width
		int max = 0;
		for (auto param : input_width_params)
			if (cell->hasParam(param))
				max = std::max(max, cell->getParam(param).as_int());

		return max;
	} else if (is_free(cell->type)) {
		return 0;
	}
	// TODO: $fsm $mem.*
	// ignored: $pow

	log_warning("Can't determine cost of %s cell (%d parameters).\n", log_id(cell->type), GetSize(cell->parameters));
	return 1;
}
