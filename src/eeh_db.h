#pragma once

#include <QString>
#include <QStringList>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

// EehDb — thin wrapper around the EEH SQLite database.
//
// Usage:
//   EehDb db;
//   if (!db.open("assets/db/eeh.sqlite")) { /* handle error */ }
//
//   // Get all main headwords of a given length
//   QStringList words = db.wordsByLength(5);
//
//   // Get headwords matching a LIKE pattern (use '_' for single char wildcard)
//   QStringList words = db.wordsByPattern("_ERR_");

class EehDb {
public:
    EehDb() = default;
    ~EehDb();

    // Opens the SQLite database at the given path.
    // Returns true on success.
    bool open(const QString& path);

    void close();

    bool isOpen() const;

    // Returns all main-entry headwords (sarreraH) of the given length.
    // Only entries where nagusia_da = 1 are returned.
    QStringList wordsByLength(int length) const;

    // Returns headwords matching a SQL LIKE pattern.
    // Use '_' for a single wildcard character, '%' for multiple.
    // Example: wordsByPattern("_ERRE_") → all 6-letter words with ERRE in pos 2-5.
    QStringList wordsByPattern(const QString& likePattern) const;

    // Returns true if the word exists as a main headword.
    bool contains(const QString& word) const;

    // Returns a list of human-readable definition strings for the given word,
    // stripping all EEH markup tags.  Returns an empty list if not found.
    QStringList lookup(const QString& word) const;

    // Like `lookup`, but also returns example strings (from `adibideak`/`etc_adibideak`).
    // Fills `defs` and `examples`. Returns true if any row was found.
    bool lookupWithExamples(const QString& word, QStringList& defs, QStringList& examples) const;

    QString lastError() const { return _lastError; }

private:
    QSqlDatabase    _db;
    mutable QString _lastError;

    static constexpr const char* kConnectionName = "EehDbConnection";
};
