/**
 * query.cpp – EEH (Euskal Hiztegia) SQLite word lookup tool.
 * Qt6 equivalent of tools/query.py.
 *
 * Usage: eeh_query <hitza>
 */

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariant>

static QTextStream out(stdout);
static QTextStream err(stderr);

// ---------------------------------------------------------------------------
// Markup stripping
// ---------------------------------------------------------------------------

static QString stripMarkup(const QString& text) {
    QString r = text;

    // _b_word_/b_  → keep text (bold)
    r.replace(QRegularExpression(R"(_b_(.*?)_/b_)"), R"(\1)");
    // _i_word_/i_  → keep text (italic)
    r.replace(QRegularExpression(R"(_i_(.*?)_/i_)"), R"(\1)");
    // _u_word_/u_  → strip entirely (cross-reference)
    r.replace(QRegularExpression(R"(_u_.*?_/u_)"), QString());
    // _gorria_…_/gorria_  → strip (frequency count)
    r.replace(QRegularExpression(R"(_gorria_.*?_/gorria_)"), QString());
    // _oharra_(…)_/oharra_  → strip (note)
    r.replace(QRegularExpression(R"(_oharra_\(.*?\)_/oharra_)"), QString());
    // <w>word</w>  → «word»
    r.replace(QRegularExpression(R"(<w>(.*?)</w>)"), QString::fromUtf8("«\\1»"));
    // <span …>…</span>  → strip (source references)
    r.replace(QRegularExpression(R"(<span[^>]*>.*?</span>)"), QString());
    // &quot; → "
    r.replace("&quot;", "\"");
    // Remove remaining HTML tags
    r.replace(QRegularExpression(R"(<[^>]+>)"), QString());
    // Collapse multiple whitespace characters
    r.replace(QRegularExpression(R"(\s{2,})"), " ");

    return r.trimmed();
}

// ---------------------------------------------------------------------------
// Definition extraction
// ---------------------------------------------------------------------------

// Grammatical category abbreviations used by EEH
static const QLatin1StringView
    CAT_ALTS("iz|adj|adlag|adond|adb|det|izenb|interj|lot|prep|part|zenbtz|esap|izenlag|adizkig|jas|sin");

static QStringList extractDefinitions(const QString& bistan) {
    const QString clean = stripMarkup(bistan);

    // Split on numbered entries: "1 iz ", "2 adj ", …
    const QRegularExpression splitRe(QString(R"((?<!\d)(?=\b\d+\s+(?:%1)\b))").arg(CAT_ALTS),
                                     QRegularExpression::CaseInsensitiveOption);

    const QRegularExpression catRe(QString(R"(^\s*\d+\s+(?:%1)\s+)").arg(CAT_ALTS),
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

// ---------------------------------------------------------------------------
// Database lookup
// ---------------------------------------------------------------------------

static void lookup(const QString& word, QSqlDatabase& db) {
    QSqlQuery q(db);
    q.prepare("SELECT bistan FROM eeh "
              "WHERE sarreraH = ? COLLATE NOCASE "
              "ORDER BY nagusia_da DESC, ord ASC");
    q.addBindValue(word.toLower());

    if (!q.exec()) {
        err << "Query error: " << q.lastError().text() << Qt::endl;
        return;
    }

    QStringList allDefs;
    while (q.next()) {
        const QString bistan = q.value(0).toString();
        if (!bistan.isEmpty())
            allDefs.append(extractDefinitions(bistan));
    }

    if (allDefs.size() > 1) {
        for (int i = 0; i < allDefs.size(); ++i)
            out << (i + 1) << ". " << allDefs.at(i) << Qt::endl;
    } else {
        out << "Ez da hitza topatu." << Qt::endl;
    }
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    const QStringList args = app.arguments().mid(1); // strip program name
    if (args.isEmpty()) {
        err << "Erabilera: " << argv[0] << " <hitza>" << Qt::endl;
        return 1;
    }

    const QString word = args.join(" ");

    // Walk up from the executable directory until we find assets/db/eeh.sqlite,
    // then fall back to cwd-relative path.
    QString dbPath;
    {
        QDir d(QCoreApplication::applicationDirPath());
        for (int i = 0; i < 6; ++i) {
            QString candidate = d.filePath("assets/db/eeh.sqlite");
            if (QFileInfo::exists(candidate)) {
                dbPath = candidate;
                break;
            }
            if (!d.cdUp())
                break;
        }
    }
    if (dbPath.isEmpty())
        dbPath = "assets/db/eeh.sqlite";

    if (!QFileInfo::exists(dbPath)) {
        err << "ERROR: ez da aurkitu: " << dbPath << Qt::endl;
        return 1;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        err << "ERROR: ezin da ireki DB: " << db.lastError().text() << Qt::endl;
        return 1;
    }

    lookup(word, db);

    db.close();
    return 0;
}
