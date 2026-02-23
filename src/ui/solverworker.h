#pragma once

#include <QObject>
#include <QVector>

#include "grid.h"
#include "dict.h"

// ──────────────────────────────────────────────────────────────
// Worker that runs Grid::solve() on a separate thread and emits
// a snapshot of the current cell values periodically so the UI
// can be updated without coupling the solver to Qt directly.
// ──────────────────────────────────────────────────────────────
class SolverWorker : public QObject {
    Q_OBJECT
public:
    explicit SolverWorker(Grid* grid, const Dict* dict, QObject* parent = nullptr);

signals:
    // Emitted when the solver finishes (success=true means a solution was found)
    void finished(bool success);

public slots:
    void run();

private:
    Grid*       _grid;
    const Dict* _dict;
};
