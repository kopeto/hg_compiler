#pragma once

#include <QString>

// ═══════════════════════════════════════════════════════════════
//  HG::Styles — centralised Qt stylesheet strings for the app.
//  Include this header wherever a setStyleSheet() call is needed.
// ═══════════════════════════════════════════════════════════════

namespace HG::Styles {

// ── Colours ──────────────────────────────────────────────────
inline constexpr auto kMuted        = "color: #555;";
inline constexpr auto kSmallMuted   = "color: #555; font-size: 11px;";
inline constexpr auto kSmallLabel   = "font-size: 11px;";
inline constexpr auto kSmallBold    = "font-size: 11px; font-weight: bold; margin-top: 6px;";
inline constexpr auto kSectionLabel = "font-weight: bold; margin-top: 4px;";

// ── Grid area ────────────────────────────────────────────────
inline constexpr auto kGridArea = "background: #e8e8e8;";

// ── Buttons ──────────────────────────────────────────────────
inline constexpr auto kPushButton = "QPushButton {"
                                    "  background: #dcdcdc;"
                                    "  color: #222;"
                                    "  border: 1px solid #aaa;"
                                    "  border-radius: 4px;"
                                    "  padding: 4px 12px;"
                                    "  font-size: 12px;"
                                    "}"
                                    "QPushButton:hover   { background: #c8c8c8; border-color: #888; }"
                                    "QPushButton:pressed { background: #b0b0b0; }"
                                    "QPushButton:disabled { color: #999; background: #ebebeb; }";

// ── Word list ────────────────────────────────────────────────
inline constexpr auto kWordList = "font-family: monospace; font-size: 12px;";

// ── Symmetry checkbox ──────────────────────────────────────────
inline constexpr auto kSymmetryCheck = "QCheckBox { font-size: 12px; color: #222; spacing: 5px; }"
                                       "QCheckBox::indicator { width: 14px; height: 14px;"
                                       "  border: 1px solid #aaa; border-radius: 2px; background: #fff; }"
                                       "QCheckBox::indicator:checked {"
                                       "  background: #555; border-color: #333;"
                                       "  image: url(none); }"
                                       "QCheckBox::indicator:hover { border-color: #777; background: #f0f0f0; }"
                                       "QCheckBox::indicator:checked:hover { background: #444; }"
                                       "QCheckBox:disabled { color: #999; }"
                                       "QCheckBox::indicator:disabled { background: #e0e0e0; border-color: #ccc; }";

// ── Clue table (used in export / upload dialogs) ─────────────
inline constexpr auto kClueTable = "QTableWidget { border: 1px solid #c0c0c0; border-radius: 4px; }"
                                   "QTableWidget::item { padding: 4px 8px; }"
                                   "QHeaderView::section { background: #f0f0f0; font-weight: bold;"
                                   "                       padding: 4px 8px; border: none;"
                                   "                       border-bottom: 1px solid #c0c0c0; }";

// ── Missing-clue highlight colour ────────────────────────────
inline constexpr auto kMissingClueColor = "#ffcccc";

} // namespace HG::Styles
