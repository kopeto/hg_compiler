#include "eeh_db.h"

#include <QDebug>
#include <QRegularExpression>

EehDb::~EehDb() {
    close();
}

bool EehDb::open(const QString& path) {
    if (QSqlDatabase::contains(kConnectionName))
        QSqlDatabase::removeDatabase(kConnectionName);

    _db = QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
    _db.setDatabaseName(path);

    if (!_db.open()) {
        _lastError = _db.lastError().text();
        qWarning() << "EehDb: could not open database:" << _lastError;
        return false;
    }

    // Optional: tune SQLite for read-heavy workload
    QSqlQuery q(_db);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA cache_size=-8000"); // ~8 MB cache

    return true;
}

void EehDb::close() {
    if (_db.isOpen())
        _db.close();
}

bool EehDb::isOpen() const {
    return _db.isOpen();
}

QStringList EehDb::wordsByLength(int length) const {
    QStringList results;
    if (!_db.isOpen())
        return results;

    QSqlQuery q(_db);
    q.prepare("SELECT sarreraH FROM eeh "
              "WHERE nagusia_da = 1 AND LENGTH(sarreraH) = :len "
              "ORDER BY sarreraH");
    q.bindValue(":len", length);

    if (!q.exec()) {
        _lastError = q.lastError().text();
        qWarning() << "EehDb::wordsByLength error:" << _lastError;
        return results;
    }

    while (q.next())
        results << q.value(0).toString().toUpper();

    return results;
}

QStringList EehDb::wordsByPattern(const QString& likePattern) const {
    QStringList results;
    if (!_db.isOpen())
        return results;

    QSqlQuery q(_db);
    q.prepare("SELECT sarreraH FROM eeh "
              "WHERE nagusia_da = 1 AND sarreraH LIKE :pat "
              "ORDER BY sarreraH");
    q.bindValue(":pat", likePattern);

    if (!q.exec()) {
        _lastError = q.lastError().text();
        qWarning() << "EehDb::wordsByPattern error:" << _lastError;
        return results;
    }

    while (q.next())
        results << q.value(0).toString().toUpper();

    return results;
}

bool EehDb::contains(const QString& word) const {
    if (!_db.isOpen())
        return false;

    QSqlQuery q(_db);
    q.prepare("SELECT 1 FROM eeh WHERE nagusia_da = 1 AND sarreraH = :word LIMIT 1");
    q.bindValue(":word", word.toLower());

    if (!q.exec() || !q.next())
        return false;
    return q.value(0).toInt() == 1;
}

// ---------------------------------------------------------------------------
// Internal helpers (markup stripping + definition extraction)
// ---------------------------------------------------------------------------

// Replaces all common named HTML entities with their Unicode equivalents.
static void resolveHtmlEntities(QString& r) {
    // Basic
    r.replace("&amp;", "&");
    r.replace("&lt;", "<");
    r.replace("&gt;", ">");
    r.replace("&quot;", "\"");
    r.replace("&apos;", "'");
    r.replace("&nbsp;", " ");
    // Punctuation / typography
    r.replace("&middot;", QString::fromUtf8("\u00B7")); // ·
    r.replace("&bull;", QString::fromUtf8("\u2022"));   // •
    r.replace("&hellip;", QString::fromUtf8("\u2026")); // …
    r.replace("&ndash;", QString::fromUtf8("\u2013"));  // –
    r.replace("&mdash;", QString::fromUtf8("\u2014"));  // —
    r.replace("&laquo;", QString::fromUtf8("\u00AB"));  // «
    r.replace("&raquo;", QString::fromUtf8("\u00BB"));  // »
    r.replace("&lsquo;", QString::fromUtf8("\u2018"));  // '
    r.replace("&rsquo;", QString::fromUtf8("\u2019"));  // '
    r.replace("&ldquo;", QString::fromUtf8("\u201C"));  // "
    r.replace("&rdquo;", QString::fromUtf8("\u201D"));  // "
    r.replace("&sbquo;", QString::fromUtf8("\u201A"));  // ‚
    r.replace("&bdquo;", QString::fromUtf8("\u201E"));  // „
    // Currency / symbols
    r.replace("&euro;", QString::fromUtf8("\u20AC"));   // €
    r.replace("&pound;", QString::fromUtf8("\u00A3"));  // £
    r.replace("&cent;", QString::fromUtf8("\u00A2"));   // ¢
    r.replace("&yen;", QString::fromUtf8("\u00A5"));    // ¥
    r.replace("&copy;", QString::fromUtf8("\u00A9"));   // ©
    r.replace("&reg;", QString::fromUtf8("\u00AE"));    // ®
    r.replace("&trade;", QString::fromUtf8("\u2122"));  // ™
    r.replace("&deg;", QString::fromUtf8("\u00B0"));    // °
    r.replace("&micro;", QString::fromUtf8("\u00B5"));  // µ
    r.replace("&para;", QString::fromUtf8("\u00B6"));   // ¶
    r.replace("&sect;", QString::fromUtf8("\u00A7"));   // §
    r.replace("&dagger;", QString::fromUtf8("\u2020")); // †
    r.replace("&Dagger;", QString::fromUtf8("\u2021")); // ‡
    r.replace("&permil;", QString::fromUtf8("\u2030")); // ‰
    r.replace("&prime;", QString::fromUtf8("\u2032"));  // ′
    r.replace("&Prime;", QString::fromUtf8("\u2033"));  // ″
    r.replace("&frasl;", QString::fromUtf8("\u2044"));  // ⁄
    // Maths
    r.replace("&minus;", QString::fromUtf8("\u2212"));  // −
    r.replace("&times;", QString::fromUtf8("\u00D7"));  // ×
    r.replace("&divide;", QString::fromUtf8("\u00F7")); // ÷
    r.replace("&plusmn;", QString::fromUtf8("\u00B1")); // ±
    r.replace("&frac12;", QString::fromUtf8("\u00BD")); // ½
    r.replace("&frac14;", QString::fromUtf8("\u00BC")); // ¼
    r.replace("&frac34;", QString::fromUtf8("\u00BE")); // ¾
    r.replace("&sup1;", QString::fromUtf8("\u00B9"));   // ¹
    r.replace("&sup2;", QString::fromUtf8("\u00B2"));   // ²
    r.replace("&sup3;", QString::fromUtf8("\u00B3"));   // ³
    r.replace("&infin;", QString::fromUtf8("\u221E"));  // ∞
    r.replace("&ne;", QString::fromUtf8("\u2260"));     // ≠
    r.replace("&le;", QString::fromUtf8("\u2264"));     // ≤
    r.replace("&ge;", QString::fromUtf8("\u2265"));     // ≥
    // Arrows
    r.replace("&larr;", QString::fromUtf8("\u2190")); // ←
    r.replace("&rarr;", QString::fromUtf8("\u2192")); // →
    r.replace("&uarr;", QString::fromUtf8("\u2191")); // ↑
    r.replace("&darr;", QString::fromUtf8("\u2193")); // ↓
    r.replace("&harr;", QString::fromUtf8("\u2194")); // ↔
    // Latin accented letters (common in Basque/Spanish context)
    r.replace("&ntilde;", QString::fromUtf8("\u00F1")); // ñ
    r.replace("&Ntilde;", QString::fromUtf8("\u00D1")); // Ñ
    r.replace("&aacute;", QString::fromUtf8("\u00E1")); // á
    r.replace("&eacute;", QString::fromUtf8("\u00E9")); // é
    r.replace("&iacute;", QString::fromUtf8("\u00ED")); // í
    r.replace("&oacute;", QString::fromUtf8("\u00F3")); // ó
    r.replace("&uacute;", QString::fromUtf8("\u00FA")); // ú
    r.replace("&Aacute;", QString::fromUtf8("\u00C1")); // Á
    r.replace("&Eacute;", QString::fromUtf8("\u00C9")); // É
    r.replace("&Iacute;", QString::fromUtf8("\u00CD")); // Í
    r.replace("&Oacute;", QString::fromUtf8("\u00D3")); // Ó
    r.replace("&Uacute;", QString::fromUtf8("\u00DA")); // Ú
    r.replace("&uuml;", QString::fromUtf8("\u00FC"));   // ü
    r.replace("&Uuml;", QString::fromUtf8("\u00DC"));   // Ü
    r.replace("&auml;", QString::fromUtf8("\u00E4"));   // ä
    r.replace("&ouml;", QString::fromUtf8("\u00F6"));   // ö
    r.replace("&agrave;", QString::fromUtf8("\u00E0")); // à
    r.replace("&egrave;", QString::fromUtf8("\u00E8")); // è
    r.replace("&acirc;", QString::fromUtf8("\u00E2"));  // â
    r.replace("&ecirc;", QString::fromUtf8("\u00EA"));  // ê
    r.replace("&icirc;", QString::fromUtf8("\u00EE"));  // î
    r.replace("&ocirc;", QString::fromUtf8("\u00F4"));  // ô
    r.replace("&ucirc;", QString::fromUtf8("\u00FB"));  // û
    r.replace("&ccedil;", QString::fromUtf8("\u00E7")); // ç
    r.replace("&Ccedil;", QString::fromUtf8("\u00C7")); // Ç
    r.replace("&szlig;", QString::fromUtf8("\u00DF"));  // ß
    r.replace("&aring;", QString::fromUtf8("\u00E5"));  // å
    r.replace("&aelig;", QString::fromUtf8("\u00E6"));  // æ
    r.replace("&oslash;", QString::fromUtf8("\u00F8")); // ø
    // Numeric references (decimal &#NNN; and hex &#xNNN;)
    QRegularExpression              numRef(R"(&#(\d+);)");
    QRegularExpressionMatchIterator it = numRef.globalMatch(r);
    // collect replacements to avoid position drift
    struct Repl {
        qsizetype start;
        qsizetype len;
        QString   ch;
    };
    QList<Repl> repls;
    while (it.hasNext()) {
        auto m = it.next();
        repls.prepend({m.capturedStart(), m.capturedLength(), QString(QChar(m.captured(1).toUInt()))});
    }
    for (const auto& rep : repls)
        r.replace(rep.start, rep.len, rep.ch);
    QRegularExpression hexRef(R"(&#x([0-9A-Fa-f]+);)");
    it = hexRef.globalMatch(r);
    repls.clear();
    while (it.hasNext()) {
        auto m = it.next();
        repls.prepend({m.capturedStart(), m.capturedLength(), QString(QChar(m.captured(1).toUInt(nullptr, 16)))});
    }
    for (const auto& rep : repls)
        r.replace(rep.start, rep.len, rep.ch);
}

static QString eehStripMarkup(const QString& text) {
    QString r = text;
    r.replace(QRegularExpression(R"(_b_(.*?)_/b_)"), R"(\1)");
    r.replace(QRegularExpression(R"(_i_(.*?)_/i_)"), R"(\1)");
    r.replace(QRegularExpression(R"(_u_.*?_/u_)"), QString());
    r.replace(QRegularExpression(R"(_gorria_.*?_/gorria_)"), QString());
    r.replace(QRegularExpression(R"(_oharra_\(.*?\)_/oharra_)"), QString());
    r.replace(QRegularExpression(R"(<w>(.*?)</w>)"), QString::fromUtf8("\u00AB\\1\u00BB")); // «word»
    r.replace(QRegularExpression(R"(<span[^>]*>(.*?)</span>)"), "(\\1)");
    r.replace(QRegularExpression(R"(<[^>]+>)"), QString());
    resolveHtmlEntities(r);
    r.replace(QRegularExpression(R"(\s{2,})"), " ");
    return r.trimmed();
}

static const QLatin1StringView
    kCatAlts("iz|adj|adlag|adond|adb|det|izenb|interj|lot|prep|part|zenbtz|esap|izenlag|adizkig|jas|sin");

// Returns a list of HTML-formatted definition strings.
// Each entry is "<i>category</i> definition text" with:
//  - headword stripped (already shown as title)
//  - sense numbers stripped
//  - grammatical category wrapped in <i>
//  - cross-references (ik ...) stripped
static QStringList eehExtractDefinitions(const QString& bistan) {
    QString r = bistan;

    // --- Phase 1: strip markup that should never appear in output ---
    // Remove note blocks (may contain nested markup like _gorria_)
    r.replace(QRegularExpression(R"(_oharra2_\(.*?\)_/oharra2_)"), QString());
    r.replace(QRegularExpression(R"(_oharra_\(.*?\)_/oharra_)"), QString());
    // Strip underline blocks used for cross-reference targets
    r.replace(QRegularExpression(R"(_u_.*?_/u_)"), QString());
    // Strip frequency counters
    r.replace(QRegularExpression(R"(_gorria_.*?_/gorria_)"), QString());
    // Convert <w>word</w> to guillemets before any HTML stripping
    r.replace(QRegularExpression(R"(<w>(.*?)</w>)"), QString::fromUtf8("\u00AB\\1\u00BB"));
    // Wrap <span> content in parentheses
    r.replace(QRegularExpression(R"(<span[^>]*>(.*?)</span>)"), "(\\1)");
    // Strip remaining HTML tags
    r.replace(QRegularExpression(R"(<br\s*/?>)", QRegularExpression::CaseInsensitiveOption), QString());
    r.replace(QRegularExpression(R"(<[^>]+>)"), QString());
    resolveHtmlEntities(r);

    // --- Phase 2: process remaining EEH custom markup ---
    // Remove the leading headword: first _b_..._/b_ at the very start
    r.replace(QRegularExpression(R"(^\s*_b_.*?_/b_\s*)"), QString());
    // Remove bold sense numbers: _b_1_/b_, _b_2_/b_, etc.
    r.replace(QRegularExpression(R"(_b_(\d+)_/b_\s*)"), QString());
    // Convert italic markup → HTML <i> (grammatical categories use _i_..._/i_)
    r.replace(QRegularExpression(R"(_i_(.*?)_/i_)"), QString::fromUtf8("<i>\\1</i>"));
    // Strip remaining bold markers, keeping their content
    r.replace(QRegularExpression(R"(_b_(.*?)_/b_)"), R"(\1)");
    r = r.simplified().trimmed();

    // --- Phase 3: split into individual senses and clean each one ---
    // Each sense starts with <i>category</i>; split before every <i>
    const QRegularExpression splitRe(R"(\s*(?=<i>))");
    const QRegularExpression ikRe(R"(\s*\bik\b.*$)", QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression trailRe(R"([\s.;,]+$)");

    QStringList result;
    for (QString part : r.split(splitRe, Qt::SkipEmptyParts)) {
        part.replace(ikRe, QString());
        part = part.trimmed();
        part.replace(trailRe, QString());
        part = part.trimmed();
        if (!part.isEmpty())
            result.append(part);
    }
    return result;
}

QStringList EehDb::lookup(const QString& word) const {
    QStringList allDefs;
    if (!_db.isOpen())
        return allDefs;

    QSqlQuery q(_db);
    q.prepare("SELECT bistan FROM eeh "
              "WHERE sarreraH = :word "
              "ORDER BY nagusia_da DESC, ord ASC");
    q.bindValue(":word", word.toLower());

    if (!q.exec()) {
        _lastError = q.lastError().text();
        qWarning() << "EehDb::lookup error:" << _lastError;
        return allDefs;
    }

    while (q.next()) {
        const QString bistan = q.value(0).toString();
        if (!bistan.isEmpty())
            allDefs.append(eehExtractDefinitions(bistan));
    }
    return allDefs;
}

// Extracts example strings from an EEH `adibideak`-style field.
// adibideak separates examples with "-:-"; etc_adibideak uses "\r\n".
// <span>...</span> is already stripped (including inner text) by eehStripMarkup.
static QStringList eehExtractExamples(const QString& raw) {
    QString clean = eehStripMarkup(raw);
    if (clean.isEmpty())
        return {};
    // "-:-" is the primary separator in adibideak; real or literal "\n" separate in etc_adibideak.
    const QRegularExpression re(R"(\s*-:-\s*|\\n|[\r\n]+)", QRegularExpression::CaseInsensitiveOption);
    QStringList              parts = clean.split(re, Qt::SkipEmptyParts);
    for (QString& p : parts)
        p = p.trimmed();
    parts.removeAll(QString());
    return parts;
}

bool EehDb::lookupWithExamples(const QString& word, QStringList& defs, QStringList& examples) const {
    defs.clear();
    examples.clear();
    if (!_db.isOpen())
        return false;

    QSqlQuery q(_db);
    q.prepare("SELECT bistan, adibideak, etc_adibideak FROM eeh "
              "WHERE sarreraH = :word "
              "ORDER BY nagusia_da DESC, ord ASC");
    q.bindValue(":word", word.toLower());

    if (!q.exec()) {
        _lastError = q.lastError().text();
        qWarning() << "EehDb::lookupWithExamples error:" << _lastError;
        return false;
    }

    bool any = false;
    while (q.next()) {
        any                  = true;
        const QString bistan = q.value(0).toString();
        const QString adib   = q.value(1).toString();
        const QString etcad  = q.value(2).toString();
        if (!bistan.isEmpty())
            defs.append(eehExtractDefinitions(bistan));
        if (!adib.isEmpty())
            examples.append(eehExtractExamples(adib));
        if (!etcad.isEmpty())
            examples.append(eehExtractExamples(etcad));
    }
    return any;
}
