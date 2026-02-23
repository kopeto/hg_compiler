#include "solverworker.h"

SolverWorker::SolverWorker(Grid* grid, const Dict* dict, QObject* parent)
    : QObject(parent), _grid(grid), _dict(dict) {}

void SolverWorker::run()
{
    bool ok = _grid->solve(*_dict, &_cancel);
    // Always emit finished — MainWindow uses _paused/_solving flags to decide
    // what to show. If we don't emit, the thread is never cleaned up.
    emit finished(ok);
}
