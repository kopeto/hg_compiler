#include "solverworker.h"

SolverWorker::SolverWorker(Grid* grid, const Dict* dict, QObject* parent)
    : QObject(parent), _grid(grid), _dict(dict) {}

void SolverWorker::run()
{
    bool ok = _grid->solve(*_dict);
    emit finished(ok);
}
