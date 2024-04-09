/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "kernel/yosys.h"
// #include "kernel/sigtools.h"
#include "kernel/cost.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct CellEstimatePass : public Pass {
	CellEstimatePass() : Pass("cellestimate", "TODO") {} // TODO
	void help() override
	{
		//   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
		// log("\n");
		// log("    zinit [options] [selection]\n");
		// log("\n");
		// log("Add inverters as needed to make all FFs zero-initialized.\n");
		// log("\n");
		// log("    -all\n");
		// log("        also add zero initialization to uninitialized FFs\n");
		// log("\n");
		// TODO
	}
	void execute(std::vector<std::string> args, RTLIL::Design *design) override
	{
		bool cmos_cost = false;

		log_header(design, "Executing cell estimate pass.\n");

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++) {
			if (args[argidx] == "-cmos") {
				cmos_cost = true;
				continue;
			}
			break;
		}
		extra_args(args, argidx, design);

        int cells_known_cost = 0;
        int cell_count = 0;
		for (auto module : design->selected_modules()) {

			int module_cost = 0;
			// TODO should we really respect cell selection?
			for (auto cell : module->selected_cells()) {
                cell_count++;

				auto &cell_cost = cmos_cost ? CellCosts::cmos_gate_cost() : CellCosts::default_gate_cost();
                bool has_known_cost = (bool)cell_cost.count(cell->type);
                if (has_known_cost)
                    cells_known_cost++;

				int cost = has_known_cost ? cell_cost.at(cell->type) : 0;
				log_debug("Cost for cell %s (%s): %d\n", log_id(cell), log_id(cell->type), cost);
				module_cost += cost;
			}
			log("Cost estimate for module %s: %d\n", log_id(module->name), module_cost);
			module->attributes[ID::cost] = module_cost;
		}
        float known_percent = (static_cast<double>(cells_known_cost) / cell_count) * 100.0;
		if (!isnan(known_percent))
			log("Cost estimates known for %.0f%% cells\n", known_percent);
	}
} ZinitPass;

PRIVATE_NAMESPACE_END
