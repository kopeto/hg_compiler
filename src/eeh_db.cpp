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

static QString eehStripMarkup(const QString& text) {
    QString r = text;
    r.replace(QRegularExpression(R"(_b_(.*?)_/b_)"), R"(\1)");
    r.replace(QRegularExpression(R"(_i_(.*?)_/i_)"), R"(\1)");
    r.replace(QRegularExpression(R"(_u_.*?_/u_)"), QString());
    r.replace(QRegularExpression(R"(_gorria_.*?_/gorria_)"), QString());
    r.replace(QRegularExpression(R"(_oharra_\(.*?\)_/oharra_)"), QString());
    r.replace(QRegularExpression(R"(<w>(.*?)</w>)"), QString::fromUtf8("\u00AB\\1\u00BB"));
    r.replace(QRegularExpression(R"(<span[^>]*>.*?</span>)"), QString());
    r.replace("&quot;", "\"");
    r.replace(QRegularExpression(R"(<[^>]+>)"), QString());
    r.replace(QRegularExpression(R"(\s{2,})"), " ");
    return r.trimmed();
}

static const QLatin1StringView
    kCatAlts("iz|adj|adlag|adond|adb|det|izenb|interj|lot|prep|part|zenbtz|esap|izenlag|adizkig|jas|sin");

static QStringList eehExtractDefinitions(const QString& bistan) {
    const QString clean = eehStripMarkup(bistan);

    const QRegularExpression splitRe(QString(R"((?<!\d)(?=\b\d+\s+(?:%1)\b))").arg(kCatAlts),
                                     QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression catRe(QString(R"(^\s*\d+\s+(?:%1)\s+)").arg(kCatAlts),
                                   QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression ikRe(R"(\s*\bik\b.*$)", QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression trailRe(R"([\s.;,]+$)");

    QStringList result;
    for (QString part : clean.split(splitRe, Qt::SkipEmptyParts)) {
        part.replace(catRe, QString());
        part = part.trimmed();
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
              "WHERE sarreraH = :word COLLATE NOCASE "
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
static QStringList eehExtractExamples(const QString& raw) {
    QString clean = eehStripMarkup(raw);
    if (clean.isEmpty())
        return {};
    // Split on semicolons, newlines or HTML <br> tags
    const QRegularExpression re(R"([;\n\r]+|<br\s*/?>)", QRegularExpression::CaseInsensitiveOption);
    QStringList              parts = clean.split(re, Qt::SkipEmptyParts);
    for (QString& p : parts)
        p = p.trimmed();
    return parts;
}

bool EehDb::lookupWithExamples(const QString& word, QStringList& defs, QStringList& examples) const {
    defs.clear();
    examples.clear();
    if (!_db.isOpen())
        return false;

    QSqlQuery q(_db);
    q.prepare("SELECT bistan, adibideak, etc_adibideak FROM eeh "
              "WHERE sarreraH = :word COLLATE NOCASE "
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
