#!/usr/bin/env python3
"""
Lookup a word in the EEH (Euskal Hiztegia) SQLite database.
Usage: python tools/query.py <hitza>
"""

import re
import sqlite3
import sys
from pathlib import Path

DB_PATH = Path("assets/db/eeh.sqlite")


def strip_markup(text: str) -> str:
    """Remove the custom markup tags used in the bistan field."""
    # _b_word_/b_  → bold
    text = re.sub(r'_b_(.*?)_/b_', r'\1', text)
    # _i_word_/i_  → italic
    text = re.sub(r'_i_(.*?)_/i_', r'\1', text)
    # _u_word_/u_  → underline / cross-reference → strip entirely
    text = re.sub(r'_u_(.*?)_/u_', '', text)
    # _gorria_number_/gorria_  → frequency count → strip entirely
    text = re.sub(r'_gorria_.*?_/gorria_', '', text)
    # _oharra_(...)_/oharra_  → note → strip entirely
    text = re.sub(r'_oharra_\(.*?\)_/oharra_', '', text)
    # <w>word</w>  → example word
    text = re.sub(r'<w>(.*?)</w>', r'«\1»', text)
    # <span ...>...</span>  → strip (source references)
    text = re.sub(r'<span[^>]*>.*?</span>', '', text)
    # &quot; → "
    text = text.replace('&quot;', '"')
    # Remove remaining HTML tags
    text = re.sub(r'<[^>]+>', '', text)
    # Collapse multiple spaces/newlines
    text = re.sub(r'\s{2,}', ' ', text).strip()
    return text


# Grammatical category markers used in the EEH
_CAT_RE = re.compile(
    r'^\s*\d+\s+'                          # leading number  "1 "
    r'(?:iz|adj|adlag|adond|adb|det|izenb|'
    r'interj|lot|prep|part|zenbtz|esap|'
    r'izenlag|adizkig|jas|sin)\s+',        # category abbrev + space
    re.IGNORECASE
)


def extract_definitions(bistan: str) -> list[str]:
    """Return a list of clean definition strings from the bistan field."""
    clean = strip_markup(bistan)
    # Split on numbered entries: " 1 ", " 2 ", etc. at word boundary
    parts = re.split(r'(?<!\d)(?=\b\d+\s+(?:iz|adj|adlag|adond|adb|det|izenb|interj|lot|prep|part|zenbtz|esap|izenlag|adizkig|jas|sin)\b)', clean)
    result = []
    for part in parts:
        part = _CAT_RE.sub('', part).strip()
        # Remove trailing "ik word." cross-references
        part = re.sub(r'\s*\bik\b.*$', '', part, flags=re.IGNORECASE).strip()
        part = re.sub(r'[\s.;,]+$', '', part).strip()
        if part:
            result.append(part)
    return result


def lookup(word: str, con: sqlite3.Connection) -> None:
    cur = con.cursor()

    # Fetch main entry + sub-entries ordered by ord
    cur.execute("""
        SELECT bistan
        FROM eeh
        WHERE sarreraH = ? COLLATE NOCASE
        ORDER BY nagusia_da DESC, ord ASC
    """, (word.lower(),))

    rows = cur.fetchall()

    if not rows:
        print("Ez da hitza topatu.")
        return

    all_defs = []
    for row in rows:
        (bistan,) = row
        if bistan:
            all_defs.extend(extract_definitions(bistan))

    if len(all_defs) > 1:
        for i, d in enumerate(all_defs, 1):
            print(f"{i}. {d}")
    
    else:
        print("Ez da hitza topatu.")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Erabilera: python {sys.argv[0]} <hitza>")
        sys.exit(1)

    word = " ".join(sys.argv[1:])

    db = Path(sys.argv[0]).parent.parent / "assets/db/eeh.sqlite"
    if not db.exists():
        db = DB_PATH
    if not db.exists():
        print(f"ERROR: ez da aurkitu: {db}")
        sys.exit(1)

    con = sqlite3.connect(db)
    try:
        lookup(word, con)
    finally:
        con.close()
