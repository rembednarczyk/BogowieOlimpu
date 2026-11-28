#!/usr/bin/env python3
"""Guards for the simulation core (docs/PRAKTYKI.md ch. 2, 3, 6).

Checks core/ sources for:
  - forbidden includes: Godot headers (core must not know the engine);
  - non-determinism: std::rand/srand, <random>, <chrono>, time(), clock(), unordered containers
    (iteration order is not portable), float/double in simulation state headers;
  - size limits: file <= 500 lines, function <= 120 lines (brace-depth heuristic);
  - tests (docs/PRAKTYKI.md ch. 4): a bug or gap id goes at the end of the name in parentheses, never as a
    prefix; the shared helpers of core/tests/helpers.h are not defined again in a test file.
Exit code 1 on any violation. Usage: python3 tools/checks/check_core.py [repo_root]
"""
import pathlib
import re
import sys

MAX_FILE_LINES = 500
MAX_FUNCTION_LINES = 120

FORBIDDEN = [
    (re.compile(r'#\s*include\s*[<"](godot|godot_cpp)'), "Godot header in core"),
    (re.compile(r'\bstd::rand\b|\bsrand\s*\(|(?<![\w:.])rand\s*\('), "std::rand / srand / rand"),
    (re.compile(r'#\s*include\s*<random>'), "<random> (use bo/core/rng.h)"),
    (re.compile(r'#\s*include\s*<chrono>|\bstd::chrono\b'), "<chrono> / wall-clock time"),
    (re.compile(r'(?<![\w:.])time\s*\(|\bclock\s*\('), "time() / clock()"),
    (re.compile(r'\bunordered_(map|set|multimap|multiset)\b'), "unordered container (non-portable order)"),
]
STATE_HEADERS = {"state.h", "time.h", "rng.h", "command.h", "event.h", "fixed.h"}
FLOAT_RE = re.compile(r'\b(float|double)\b')
ID_PREFIX_RE = re.compile(r'TEST_CASE\("\s*[BLKOR]-\d+')
SHARED_HELPERS_RE = re.compile(
    r'^(?:const\s+)?(?:std::string|Building&)\s+(at|rejection|refusal|read_text)\s*\(', re.M)


def strip_comments_and_strings(text: str) -> str:
    text = re.sub(r'//[^\n]*', '', text)
    text = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), text, flags=re.S)
    return re.sub(r'"(\\.|[^"\\])*"', '""', text)


def function_lengths(code: str):
    """Yields (start_line, length) of brace blocks opened at depth 0 or inside a namespace."""
    depth = 0
    starts = []
    ns_depths = set()
    lines = code.split('\n')
    for lineno, line in enumerate(lines, 1):
        for ch in line:
            if ch == '{':
                is_ns = re.search(r'\bnamespace\b[^{]*$', line[: line.index('{')]) is not None
                if is_ns:
                    ns_depths.add(depth)
                elif depth == 0 or (depth - 1) in ns_depths and all(d in ns_depths for d in range(depth)):
                    starts.append((depth, lineno))
                depth += 1
            elif ch == '}':
                depth -= 1
                if starts and starts[-1][0] == depth:
                    _, start = starts.pop()
                    yield start, lineno - start + 1
                ns_depths.discard(depth)


def main() -> int:
    root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.')
    errors = []
    for path in sorted((root / 'core').rglob('*')):
        if path.suffix not in {'.h', '.hpp', '.cpp'}:
            continue
        rel = path.relative_to(root)
        raw = path.read_text(encoding='utf-8')
        code = strip_comments_and_strings(raw)
        if raw.count('\n') > MAX_FILE_LINES:
            errors.append(f"{rel}: {raw.count(chr(10))} lines > {MAX_FILE_LINES}")
        is_test = 'tests' in path.parts
        for regex, why in FORBIDDEN:
            if is_test and why.startswith(('time()', '<chrono>')):
                continue
            for m in regex.finditer(code):
                line = code[: m.start()].count('\n') + 1
                errors.append(f"{rel}:{line}: {why}")
        if is_test and path.name != 'helpers.h':
            for m in ID_PREFIX_RE.finditer(raw):
                errors.append(f"{rel}:{raw[: m.start()].count(chr(10)) + 1}: id as a test name prefix (put it last, in parentheses)")
            for m in SHARED_HELPERS_RE.finditer(code):
                errors.append(f"{rel}:{code[: m.start()].count(chr(10)) + 1}: {m.group(1)}() again (use core/tests/helpers.h)")
        if path.name in STATE_HEADERS:
            for m in FLOAT_RE.finditer(code):
                line = code[: m.start()].count('\n') + 1
                errors.append(f"{rel}:{line}: float/double in simulation state (use bo/core/fixed.h)")
        if not is_test:
            for start, length in function_lengths(code):
                if length > MAX_FUNCTION_LINES:
                    errors.append(f"{rel}:{start}: block of {length} lines > {MAX_FUNCTION_LINES}")
    for e in errors:
        print(e)
    print(f"check_core: {len(errors)} violation(s)")
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
