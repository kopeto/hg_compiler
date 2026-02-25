#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Runtime asset path resolution.
//
// The installed layout (both the NSIS installer and the `deploy/` folder
// produced by windeployqt) places files like this:
//
//   <root>/bin/HitzGurutzatuak.exe   ← QCoreApplication::applicationDirPath()
//   <root>/assets/grids/...
//   <root>/assets/words/...
//
// So assets are always at  <exeDir>/../assets/.
//
// During development (running straight from the build tree) the runtime path
// won't exist and we fall back to the compile-time HG_ASSETS_PATH macro which
// points to the source tree.
// ─────────────────────────────────────────────────────────────────────────────

#include <QCoreApplication>
#include <QDir>
#include <QString>

namespace HG {

/// Returns the resolved assets base directory (always ends with '/').
inline QString assetsPath() {
    // Prefer the layout-relative path (installed / deploy)
    QString runtimeAssets = QDir(QCoreApplication::applicationDirPath() + "/../assets").absolutePath();
    if (QDir(runtimeAssets).exists())
        return runtimeAssets + "/";

    // Fallback: compile-time source-tree path (raw build, unit tests)
    return QString(HG_ASSETS_PATH);
}

/// Default grid path resolved at runtime.
inline QString defaultGridPath() {
    return assetsPath() + "grids/basic_6x6.grid";
}

/// Default dictionary path resolved at runtime.
inline QString defaultDictPath() {
    return assetsPath() + "words/garbi_r_gabe.txt";
}

} // namespace HG
