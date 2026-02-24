#pragma once

#include "dict.h"
#include "grid.h"

#include <QObject>
#include <QVector>
#include <atomic>

// ──────────────────────────────────────────────────────────────
// Worker that runs Grid::solve() on a separate thread and emits
// a snapshot of the current cell values periodically so the UI
// can be updated without coupling the solver to Qt directly.
// ──────────────────────────────────────────────────────────────
class SolverWorker : public QObject {
    Q_OBJECT
public:
    explicit SolverWorker(Grid* grid, const Dict* dict, QObject* parent = nullptr);

    // Call this from any thread to ask the solver to abort.
    // Grid::solve() checks this flag at each recursive call and returns false immediately.
    void requestCancel() { _cancel.store(true, std::memory_order_relaxed); }

signals:
    void finished(bool success);

public slots:
    void run();

private:
    Grid*             _grid;
    const Dict*       _dict;
    std::atomic<bool> _cancel{false};
};
