#!/usr/bin/env python3
"""
Convert a MySQL/phpMyAdmin SQL dump to a SQLite database.
Usage: python tools/mysql_to_sqlite.py assets/db/EEH-23-01-26.sql assets/db/eeh.sqlite
"""

import re
import sqlite3
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# CREATE TABLE transformer: parses column defs one-by-one (avoids regex on
# the full multi-line SQL which is fragile).
# ---------------------------------------------------------------------------

_TYPE_RE = re.compile(
    r'\b(TINY|SMALL|MEDIUM|BIG)?INT\s*\(\d+\)'
    r'|\bINT\b'
    r'|\bVARCHAR\s*\(\d+\)'
    r'|\b(MEDIUM|LONG)?TEXT\b'
    r'|\bDATE\b',
    re.IGNORECASE,
)

def _map_type(m: re.Match) -> str:
    s = m.group(0).upper()
    if 'INT' in s:
        return 'INTEGER'
    if 'TEXT' in s or 'VARCHAR' in s:
        return 'TEXT'
    if s == 'DATE':
        return 'TEXT'
    return s


def _clean_col_def(defn: str) -> str:
    """Clean a single column definition for SQLite."""
    defn = defn.replace('`', '"')
    defn = re.sub(r'\s+AUTO_INCREMENT\b', '', defn, flags=re.IGNORECASE)
    defn = re.sub(r'\s+COLLATE\s+\S+', '', defn, flags=re.IGNORECASE)
    defn = re.sub(r'\s+CHARACTER\s+SET\s+\S+', '', defn, flags=re.IGNORECASE)
    defn = re.sub(r"\bDEFAULT\s+'0000-00-00'", 'DEFAULT NULL', defn, flags=re.IGNORECASE)
    defn = _TYPE_RE.sub(_map_type, defn)
    return defn.strip()


def _split_top_level(body: str) -> list[str]:
    """Split CREATE TABLE body by commas at depth 0 (ignores commas inside parens)."""
    parts, cur, depth = [], [], 0
    for ch in body:
        if ch == '(':
            depth += 1
            cur.append(ch)
        elif ch == ')':
            depth -= 1
            cur.append(ch)
        elif ch == ',' and depth == 0:
            s = ''.join(cur).strip()
            if s:
                parts.append(s)
            cur = []
        else:
            cur.append(ch)
    s = ''.join(cur).strip()
    if s:
        parts.append(s)
    return parts


def transform_create_table(sql: str) -> str:
    """Convert a MySQL CREATE TABLE statement to SQLite-compatible SQL."""
    # Extract table name
    m = re.search(r'CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?`?(\w+)`?',
                  sql, re.IGNORECASE)
    if not m:
        return sql
    table = m.group(1)

    # Extract the body between the first ( and its matching )
    start = sql.index('(')
    depth, end = 0, start
    for i, ch in enumerate(sql[start:], start):
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
            if depth == 0:
                end = i
                break
    body = sql[start + 1:end]

    sqlite_defs = []
    for defn in _split_top_level(body):
        upper = defn.strip().upper()
        # Skip KEYs and INDEXes (not PRIMARY KEY)
        if re.match(r'^\s*(UNIQUE\s+KEY|KEY|FULLTEXT\s+KEY|INDEX)\s+', defn, re.IGNORECASE):
            continue
        if upper.startswith('PRIMARY KEY'):
            sqlite_defs.append(defn.replace('`', '"').strip())
            continue
        sqlite_defs.append(_clean_col_def(defn))

    body_str = ',\n  '.join(sqlite_defs)
    return f'CREATE TABLE IF NOT EXISTS "{table}" (\n  {body_str}\n)'


# ---------------------------------------------------------------------------
# Statement splitter: handles --, /* */, '' escapes, backslash escapes
# ---------------------------------------------------------------------------

def iter_statements(content: str):
    """Yield SQL statements split on ; (respects string literals and comments)."""
    buf = []
    in_string = False
    string_char = ''
    i, n = 0, len(content)

    while i < n:
        ch = content[i]

        if in_string:
            buf.append(ch)
            if ch == '\\' and string_char == "'":
                # MySQL backslash escape inside string
                if i + 1 < n:
                    i += 1
                    buf.append(content[i])
            elif ch == string_char:
                # Check for doubled-quote escape: '' or ""
                if i + 1 < n and content[i + 1] == string_char:
                    i += 1
                    buf.append(content[i])
                else:
                    in_string = False
            i += 1
            continue

        # Outside a string:
        if ch in ("'", '"', '`'):
            in_string = True
            string_char = ch
            buf.append(ch)
        elif ch == '-' and i + 1 < n and content[i + 1] == '-':
            # Line comment: skip to EOL
            while i < n and content[i] != '\n':
                i += 1
            continue
        elif ch == '/' and i + 1 < n and content[i + 1] == '*':
            # Block comment: skip to */
            i += 2
            while i < n - 1:
                if content[i] == '*' and content[i + 1] == '/':
                    i += 2
                    break
                i += 1
            continue
        elif ch == ';':
            stmt = ''.join(buf).strip()
            # Skip MySQL-only housekeeping statements
            if stmt and not re.match(
                r'^(SET\b|LOCK\s+TABLES|UNLOCK\s+TABLES)', stmt, re.IGNORECASE
            ):
                yield stmt
            buf = []
        else:
            buf.append(ch)
        i += 1


# ---------------------------------------------------------------------------
# Main conversion
# ---------------------------------------------------------------------------

def convert(input_path: str, output_path: str) -> None:
    src = Path(input_path)
    dst = Path(output_path)

    if not src.exists():
        print(f"ERROR: input file not found: {src}", file=sys.stderr)
        sys.exit(1)

    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        dst.unlink()

    print(f"Reading {src} ...")
    with open(src, encoding='utf-8', errors='replace') as f:
        content = f.read()

    con = sqlite3.connect(dst)
    cur = con.cursor()
    cur.execute("PRAGMA journal_mode=WAL")
    cur.execute("PRAGMA synchronous=NORMAL")

    errors = ok = 0
    cur.execute("BEGIN")

    for i, raw_sql in enumerate(iter_statements(content)):
        if i > 0 and i % 5000 == 0:
            con.execute("COMMIT")
            con.execute("BEGIN")
            print(f"  {i:,} statements processed ...")

        if re.match(r'\s*CREATE\s+TABLE', raw_sql, re.IGNORECASE):
            sql = transform_create_table(raw_sql)
        elif re.match(r'\s*(DROP\s+TABLE|INSERT\s+INTO)', raw_sql, re.IGNORECASE):
            sql = raw_sql.replace('`', '"')
        else:
            continue  # skip everything else

        try:
            cur.execute(sql)
            ok += 1
        except sqlite3.Error as e:
            errors += 1
            if errors <= 10:
                short = sql[:120].replace('\n', ' ')
                print(f"  WARN #{i}: {e} | {short}", file=sys.stderr)
            elif errors == 11:
                print("  (further warnings suppressed)", file=sys.stderr)

    con.execute("COMMIT")

    # Useful indexes for crossword lookups
    print("Creating indexes...")
    for ddl in [
        'CREATE INDEX IF NOT EXISTS idx_sarreraH ON eeh(sarreraH)',
        'CREATE INDEX IF NOT EXISTS idx_nagusia_da ON eeh(nagusia_da)',
        'CREATE INDEX IF NOT EXISTS idx_letra ON eeh(letra)',
    ]:
        try:
            cur.execute(ddl)
        except sqlite3.Error:
            pass
    con.commit()
    con.close()

    size_mb = dst.stat().st_size / (1024 * 1024)
    print(f"\nDone! {ok:,} statements OK, {errors} skipped.")
    print(f"Output: {dst} ({size_mb:.1f} MB)")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: python {sys.argv[0]} <input.sql> <output.sqlite>")
        sys.exit(1)
    convert(sys.argv[1], sys.argv[2])
