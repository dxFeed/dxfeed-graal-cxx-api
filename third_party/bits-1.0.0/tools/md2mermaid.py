# python
# Usage: python md2mermaid.py <input.md>
# Output file: <input_basename>.mermaid.md (placed next to input)

import os
import re
import sys
from math import floor, ceil


def split_row(line):
    return [c.strip() for c in line.strip().strip('|').split('|')]


def num(s):
    return float(s.replace(',', '').strip())


def kind(header_last):
    d = 'l.shift' if '<<' in header_last else ('r.shift' if '>>' in header_last else 'shift')
    m = re.search(r'\((u?int(32|64)_t)\)', header_last)
    t = (m.group(1) if m else 'uint32_t')
    t = {'uint32_t': 'u32', 'uint64_t': 'u64', 'int32_t': 'i32', 'int64_t': 'i64'}.get(t, t)
    return d, t


def variant(label_raw):
    s = label_raw.strip()
    if len(s) >= 2 and s[0] == '`' and s[-1] == '`': s = s[1:-1]
    for v in ('builtin', 'sal', 'sar', 'shl', 'shr'):
        if s.startswith(v): return v
    return s


def label(d, t, v, idx):
    return f'{d}, {t}, {v}, {idx}' if v == 'builtin' and idx > 0 and t.startswith('u') else f'{d}, {t}, {v}'


def mermaid(title, axis_title, xs, ys, yrange=None):
    xa = '[' + ', '.join(f'"{x}"' for x in xs) + ']'
    ba = '[' + ', '.join(f'{v:.2f}' for v in ys) + ']'
    y_line = f'y-axis "{axis_title}" {yrange[0]} --> {yrange[1]}' if yrange else f'y-axis "{axis_title}"'
    return '\n'.join((
        '```mermaid',
        '---',
        'config:',
        '    xyChart:',
        '        showDataLabel: true',
        '        width: 1200',
        '    yAxis:',
        '        showLabel: false',
        '        showAxisLine: false',
        '---',
        'xychart-beta horizontal',
        f'title "{title}"',
        y_line,
        f'x-axis {xa}',
        f'bar {ba}',
        '```',
    ))


def main():
    if len(sys.argv) < 2:
        print('Usage: python md2mermaid.py <input.md>')
        sys.exit(1)

    input_filename = sys.argv[1]
    base, _ = os.path.splitext(input_filename)
    output_filename = base + '.mermaid.md'

    lines = open(input_filename, encoding='utf-8').read().splitlines()
    x_data, mop, ns = [], [], []
    i, n = 0, len(lines)
    builtin_idx = 2

    while i + 3 < n:
        if '| relative' not in lines[i] or '---' not in lines[i + 1] or '|' not in lines[i + 1]:
            i += 1
            continue

        hcols = split_row(lines[i])
        if len(hcols) < 6: i += 1; continue
        d, t = kind(hcols[-1])

        r1 = i + 2
        while r1 < n and not lines[r1].strip(): r1 += 1
        r2 = r1 + 1
        while r2 < n and not lines[r2].strip(): r2 += 1
        if r2 >= n: break

        c1, c2 = split_row(lines[r1]), split_row(lines[r2])
        if len(c1) < 6 or len(c2) < 6: i += 1; continue

        v1, v2 = variant(c1[-1]), variant(c2[-1])

        if d == 'l.shift' and t == 'u32' and v1 == 'builtin':
            if builtin_idx == 1:
                builtin_idx = 2
            else:
                builtin_idx = 1

        x_data += [label(d, t, v1, builtin_idx), label(d, t, v2, 0)]
        ns += [num(c1[1]), num(c2[1])]
        mop += [num(c1[2]) / 1e6, num(c2[2]) / 1e6]

        i = r2 + 1

    with open(output_filename, 'w', encoding='utf-8') as f:
        max_mop = ceil(max(mop))
        div_max_mop, _ = divmod(max_mop, 1000)
        max_mop = (div_max_mop + 1) * 1000

        f.write(mermaid('Mop/s (higher is better)', 'Mop/s', x_data, mop, [0, max_mop]))
        f.write('\n\n')
        f.write(mermaid('ns/op (lower is better)', 'ns/op', x_data, ns, [0, 1]))
        f.write('\n')


if __name__ == '__main__':
    main()
