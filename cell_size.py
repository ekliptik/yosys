import subprocess, functools
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

r = functools.partial(subprocess.check_output, text=True)

cells = [
    # "$not",
    # "$pos",
    # "$neg",
    # "$and",
    # "$or",
    # "$xor",
    # "$xnor",
    # "$reduce_and",
    # "$reduce_or",
    # "$reduce_xor",
    # "$reduce_xnor",
    # "$reduce_bool",
    # "$shl",
    # "$shr",
    # "$sshl",
    # "$sshr",
    # "$shift",
    # "$shiftx",
    # "$lt",
    # "$le",
    # "$eq",
    # "$ne",
    # "$ge",
    # "$gt",
    # "$add",
    # "$sub",
    # "$mul",

    # "$div",
    # "$mod",
    # "$divfloor",
    # "$modfloor",
    # "$logic_not",
    # "$logic_and",
    # "$logic_or",
    # "$mux",
    # "$bmux",
    # "$demux",

    # "$lut",

    # "$sop",
    # "$alu",
    # "$lcu",
    # "$macc",
    # "$fa",

    # "$pow",
]
# cells = ["$add"]
out_dir = Path('sizes')
test_count = 100

for cell in cells:
    print(cell)
    cell_name = cell[1:]
    s = r(f'./yosys -p "test_cell -noeval -nosat -n {test_count} \{cell}"', shell=True)
    gate_counts = []
    port_widths = []
    y_widths = []
    inp_widths = []

    def push_port(l, line):
        words = line.split()
        if 'width' in line:
            assert(len(words) == 6)
            l.append(int(words[2]))
        else:
            assert len(words) == 4, words
            l.append(1)

    for line in s.splitlines():
        if "Number of cells" in line:
            gate_counts.append(int(line.split()[-1]))
        elif 'wire' in line and ('output' in line or 'input' in line):
            if line.endswith('\Y'):
                push_port(y_widths, line)
            elif 'input' in line:
                push_port(inp_widths, line)
            push_port(port_widths, line)

    result_count = len(gate_counts)
    assert result_count == test_count
    assert len(port_widths) % result_count == 0
    assert len(inp_widths) % result_count == 0
    ports_per_cell = len(port_widths) // result_count
    inputs_per_cell = len(inp_widths) // result_count

    port_size_sums = [sum(port_widths[i:i+ports_per_cell]) for i in range(0, len(port_widths), ports_per_cell)]
    max_inp_widths = [max(inp_widths[i:i+inputs_per_cell]) for i in range(0, len(inp_widths), inputs_per_cell)]

    def out(x, y, suffix):
        if not len(x) == len(y):
            print(f"skipping weird {cell} {suffix}")
            return

        with open(Path('sizes') / f'{cell_name}.{suffix}.csv', 'w') as f:
            for width, gates in zip(x, y):
                f.write(f"{width},{gates}\n")

        unique_pairs, counts = np.unique((x, y), axis=1, return_counts=True)

        area = counts*10
        c = np.sqrt(area)
        plt.clf()
        plt.scatter(unique_pairs[0], unique_pairs[1], s=area, alpha=0.5, c=c)
        plt.ylim(bottom=0)
        plt.xlabel(f'{suffix} port width')
        plt.ylabel('Gates')
        plt.title(cell)
        plt.savefig(out_dir / f'{cell_name}.{suffix}.png')

    out(y_widths, gate_counts, 'y')
    out(port_size_sums, gate_counts, 'sum')
    out(max_inp_widths, gate_counts, 'max_inp')
