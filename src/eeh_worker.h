#pragma once

#include "eeh_db.h"

#include <QObject>
#include <QString>
#include <QStringList>

class EehWorker : public QObject {
    Q_OBJECT
public:
    explicit EehWorker(QObject* parent = nullptr) : QObject(parent) {}
    ~EehWorker() { _db.close(); }

public slots:
    // Initialize/open the DB in the worker thread
    void init(const QString& dbPath) { _db.open(dbPath); }

    // Perform lookup in worker thread (non-blocking for main thread)
    void lookup(const QString& word) {
        QStringList defs, examples;
        bool        found = _db.lookupWithExamples(word, defs, examples);
        Q_EMIT lookupDone(word, found, defs, examples);
    }

signals:
    void lookupDone(const QString& word, bool found, const QStringList& defs, const QStringList& examples);

private:
    EehDb _db;
};
