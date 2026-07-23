#!/usr/bin/env python3
"""PlantUML クラス図 → Markdown 表 変換ツール

使い方:
    python plantuml_to_table.py diagram.puml
    python plantuml_to_table.py --java diagram.puml   # フィールドをJavaスタイル（型名 変数名）でパース
    cat diagram.puml | python plantuml_to_table.py
"""

import argparse
import re
import sys


MODIFIER_MAP = {
    '+': 'public',
    '-': 'private',
    '#': 'protected',
    '~': 'internal',
}


def parse_member(line, java_style=False):
    sym = line[0] if line and line[0] in MODIFIER_MAP else None
    if sym is None:
        return None

    rest = re.sub(r'\{.*?\}', '', line[1:]).strip()

    method_match = re.match(r'(\w+)\s*\((.*?)\)(?:\s*:\s*(.+))?', rest)
    if method_match:
        name, params, return_type = method_match.groups()
        return {
            'kind': 'method',
            'name': name,
            'modifier': MODIFIER_MAP[sym],
            'params': params.strip() if params.strip() else '-',
            'return_type': (return_type or '').strip() or '-',
        }

    if java_style:
        # 型名 変数名 の形式: +int hp
        java_match = re.match(r'([\w<>\[\]]+)\s+(\w+)$', rest)
        if java_match:
            field_type, name = java_match.groups()
            return {
                'kind': 'field',
                'name': name,
                'modifier': MODIFIER_MAP[sym],
                'type': field_type,
            }

    # デフォルト: 変数名 : 型名 の形式: +hp : int
    field_match = re.match(r'(\w+)\s*(?::\s*(.+))?', rest)
    if field_match:
        name, field_type = field_match.groups()
        return {
            'kind': 'field',
            'name': name,
            'modifier': MODIFIER_MAP[sym],
            'type': (field_type or '').strip() or '-',
        }

    return None


def parse_plantuml(text, java_style=False):
    classes = {}
    current_class = None
    brace_depth = 0

    for line in text.splitlines():
        line = line.strip()

        if not line or line.startswith("'"):
            continue

        class_match = re.match(r'(?:abstract\s+)?class\s+(\w+)', line)
        if class_match:
            current_class = class_match.group(1)
            classes[current_class] = {'methods': [], 'fields': []}
            brace_depth = 1 if '{' in line else 0
            continue

        if current_class is None:
            continue

        if line == '{':
            brace_depth += 1
            continue

        if line == '}':
            brace_depth -= 1
            if brace_depth <= 0:
                current_class = None
            continue

        if line and line[0] in MODIFIER_MAP:
            member = parse_member(line, java_style=java_style)
            if member:
                key = 'methods' if member['kind'] == 'method' else 'fields'
                classes[current_class][key].append(member)

    return classes


def to_markdown(classes):
    lines = []

    for class_name, members in classes.items():
        lines.append(f'**`{class_name}`**\n')

        if members['methods']:
            lines.append('| メソッド名 | 修飾子 | 引数 | 戻り値 | 役割 |')
            lines.append('| :--- | :--- | :--- | :--- | :--- |')
            for m in members['methods']:
                lines.append(f'| {m["name"]} | {m["modifier"]} | {m["params"]} | {m["return_type"]} |  |')

        if members['fields']:
            if members['methods']:
                lines.append('')
            lines.append('| フィールド名 | 修飾子 | 型 | 役割 |')
            lines.append('| :--- | :--- | :--- | :--- |')
            for f in members['fields']:
                lines.append(f'| {f["name"]} | {f["modifier"]} | {f["type"]} |  |')

        lines.append('')

    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description='PlantUML クラス図 → Markdown 表 変換ツール')
    parser.add_argument('file', nargs='?', help='入力ファイル（省略時は標準入力）')
    parser.add_argument('--java', action='store_true',
                        help='フィールドを Javaスタイル（型名 変数名）でパースする')
    args = parser.parse_args()

    if args.file:
        try:
            with open(args.file, encoding='utf-8') as f:
                text = f.read()
        except FileNotFoundError:
            print(f'Error: file not found: {args.file}', file=sys.stderr)
            sys.exit(1)
    else:
        text = sys.stdin.read()

    classes = parse_plantuml(text, java_style=args.java)
    if not classes:
        print('クラスが見つかりませんでした。', file=sys.stderr)
        sys.exit(1)

    print(to_markdown(classes))


if __name__ == '__main__':
    main()
