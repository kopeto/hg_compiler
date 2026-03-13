#include "hg_config.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

// ─────────────────────────────────────────────────────────────────────────────

QString HgConfig::configFilePath() {
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("hg_config.ini");
}

void HgConfig::load() {
    QSettings s(configFilePath(), QSettings::IniFormat);
    serverHost = s.value("upload/serverHost").toString();
    apiKey     = s.value("upload/apiKey").toString();
    author     = s.value("crossword/author").toString();
    dictPath   = s.value("dict/path").toString();
}

void HgConfig::save() const {
    QSettings s(configFilePath(), QSettings::IniFormat);
    s.setValue("upload/serverHost", serverHost);
    s.setValue("upload/apiKey", apiKey);
    s.setValue("crossword/author", author);
    s.setValue("dict/path", dictPath);
    s.sync();
}
