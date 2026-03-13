#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// HgConfig — lightweight persistent configuration backed by an INI file.
//
// File location: same directory as the executable, named "hg_config.ini".
// On first run the file does not exist and all values are empty / default.
// Call save() whenever a value changes so the next session picks it up.
// ─────────────────────────────────────────────────────────────────────────────

#include <QString>

struct HgConfig {
    // ── Stored values ─────────────────────────────────────────────────────────
    QString serverHost; // host only, e.g. "myserver.com"
    QString apiKey;
    QString author;
    QString dictPath; // absolute path to last used dictionary (empty = default)

    // ── Load / save ───────────────────────────────────────────────────────────

    /// Load from disk.  Silent no-op if the file does not exist.
    void load();

    /// Persist current values to disk.
    void save() const;

    /// Returns the absolute path to the config file.
    static QString configFilePath();
};
