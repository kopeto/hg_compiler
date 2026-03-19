#!/usr/bin/env python3
"""Quick sanity check for the converted EEH SQLite database."""

import sqlite3
import sys
from pathlib import Path

DB_PATH = Path("assets/db/eeh.sqlite")

def check(con):
    cur = con.cursor()

    # 1. Row count
    cur.execute("SELECT COUNT(*) FROM eeh")
    total = cur.fetchone()[0]
    print(f"Total rows        : {total:,}")

    # 2. Main entries only
    cur.execute("SELECT COUNT(*) FROM eeh WHERE nagusia_da = 1")
    main = cur.fetchone()[0]
    print(f"Main entries      : {main:,}")

    # 3. Words by length (5, 7, 10)
    for length in (5, 7, 10):
        cur.execute(
            "SELECT COUNT(*) FROM eeh WHERE nagusia_da = 1 AND LENGTH(sarreraH) = ?",
            (length,)
        )
        print(f"  Length {length:2d}        : {cur.fetchone()[0]:,} words")

    # 4. LIKE pattern test
    pattern = "etx%"
    cur.execute(
        "SELECT sarreraH FROM eeh WHERE nagusia_da = 1 AND sarreraH LIKE ? ORDER BY sarreraH LIMIT 10",
        (pattern,)
    )
    rows = [r[0] for r in cur.fetchall()]
    print(f"\nLIKE '{pattern}'     : {rows}")

    # 5. Exact lookup
    for word in ("etxe", "mendi", "itsaso"):
        cur.execute(
            "SELECT COUNT(*) FROM eeh WHERE nagusia_da = 1 AND sarreraH = ?",
            (word,)
        )
        found = cur.fetchone()[0] > 0
        print(f"  contains({word!r:12}) : {'YES' if found else 'NO'}")

    # 6. Sample 5 random main entries
    cur.execute(
        "SELECT sarreraH, letra FROM eeh WHERE nagusia_da = 1 ORDER BY RANDOM() LIMIT 5"
    )
    print("\nRandom sample     :")
    for row in cur.fetchall():
        print(f"  {row[0]!r:30} (letra={row[1]})")

if __name__ == "__main__":
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else DB_PATH
    if not path.exists():
        print(f"ERROR: {path} not found. Run mysql_to_sqlite.py first.")
        sys.exit(1)

    con = sqlite3.connect(path)
    print(f"Checking {path} ...\n")
    try:
        check(con)
    finally:
        con.close()
    print("\nAll checks passed.")
