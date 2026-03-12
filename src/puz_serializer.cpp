#include "puz_serializer.h"

#include <QFile>

#include <cctype>
#include <cstdint>
#include <cstring>

// ─────────────────────────────────────────────────────────────────────────────
//  Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

static uint16_t puzCksum(const uint8_t* data, size_t len, uint16_t cksum = 0)
{
    for (size_t i = 0; i < len; ++i) {
        if (cksum & 1) cksum = static_cast<uint16_t>((cksum >> 1) | 0x8000u);
        else           cksum = static_cast<uint16_t>(cksum >> 1);
        cksum = static_cast<uint16_t>(cksum + data[i]);
    }
    return cksum;
}

static uint16_t cksumStr(const std::string& s, uint16_t c)
{
    return puzCksum(reinterpret_cast<const uint8_t*>(s.data()), s.size(), c);
}

// Read a null-terminated string from raw bytes starting at pos.
// Advances pos past the null terminator.
static std::string readNulStr(const QByteArray& buf, int& pos)
{
    std::string result;
    while (pos < buf.size() && buf[pos] != '\0')
        result += buf[pos++];
    ++pos; // skip null terminator
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Export
// ─────────────────────────────────────────────────────────────────────────────

QString PuzSerializer::exportToFile(const Grid&        grid,
                                     const QString&     path,
                                     const std::string& defaultClue)
{
    const int rows = grid.getRows();
    const int cols = grid.getCols();

    // ── Solution and player-state strings ────────────────────────
    std::string solution(rows * cols, ' ');
    std::string playerState(rows * cols, ' ');
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int  idx = r * cols + c;
            char v   = static_cast<char>(grid.getValue(r, c));
            if (v == '#') {
                solution[idx]    = '.';
                playerState[idx] = '.';
            } else {
                solution[idx]    = (v == '_') ? 'A' : static_cast<char>(std::toupper((unsigned char)v));
                playerState[idx] = '-';
            }
        }
    }

    // ── Clue list (reading order: across then down per numbered cell) ─
    std::vector<std::string> clueList;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (grid.getValue(r, c) == '#')
                continue;
            bool startsAcross = (c == 0 || grid.getValue(r, c - 1) == '#') &&
                                (c + 1 < cols && grid.getValue(r, c + 1) != '#');
            bool startsDown   = (r == 0 || grid.getValue(r - 1, c) == '#') &&
                                (r + 1 < rows && grid.getValue(r + 1, c) != '#');
            if (startsAcross) clueList.push_back(defaultClue);
            if (startsDown)   clueList.push_back(defaultClue);
        }
    }

    const auto numClues = static_cast<uint16_t>(clueList.size());

    // ── CIB ──────────────────────────────────────────────────────
    uint8_t cib[8] = {};
    cib[0] = static_cast<uint8_t>(cols);
    cib[1] = static_cast<uint8_t>(rows);
    cib[2] = numClues & 0xFF;
    cib[3] = (numClues >> 8) & 0xFF;
    cib[4] = 0x01; cib[5] = 0x00;
    cib[6] = 0x00; cib[7] = 0x00;

    // ── Checksums ─────────────────────────────────────────────────
    uint16_t c_cib  = puzCksum(cib, 8);
    uint16_t c_sol  = puzCksum(reinterpret_cast<const uint8_t*>(solution.data()), solution.size());
    uint16_t c_grid = puzCksum(reinterpret_cast<const uint8_t*>(playerState.data()), playerState.size());
    uint16_t c_part = 0;
    for (const auto& cl : clueList)
        c_part = cksumStr(cl, c_part);

    uint16_t overall = c_cib;
    overall = cksumStr(solution, overall);
    overall = cksumStr(playerState, overall);
    for (const auto& cl : clueList)
        overall = cksumStr(cl, overall);

    // ── Header ────────────────────────────────────────────────────
    uint8_t header[52] = {};
    header[0x00] = overall & 0xFF;
    header[0x01] = (overall >> 8) & 0xFF;
    std::memcpy(header + 0x02, "ACROSS&DOWN\0", 12);
    header[0x0E] = c_cib & 0xFF;
    header[0x0F] = (c_cib >> 8) & 0xFF;
    header[0x10] = static_cast<uint8_t>('I' ^ (c_cib  & 0xFF));
    header[0x11] = static_cast<uint8_t>('C' ^ (c_sol  & 0xFF));
    header[0x12] = static_cast<uint8_t>('H' ^ (c_grid & 0xFF));
    header[0x13] = static_cast<uint8_t>('E' ^ (c_part & 0xFF));
    header[0x14] = static_cast<uint8_t>('A' ^ ((c_cib  >> 8) & 0xFF));
    header[0x15] = static_cast<uint8_t>('T' ^ ((c_sol  >> 8) & 0xFF));
    header[0x16] = static_cast<uint8_t>('E' ^ ((c_grid >> 8) & 0xFF));
    header[0x17] = static_cast<uint8_t>('D' ^ ((c_part >> 8) & 0xFF));
    std::memcpy(header + 0x18, "1.3\0", 4);
    std::memcpy(header + 0x2C, cib, 8);

    // ── Write ─────────────────────────────────────────────────────
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return QStringLiteral("Cannot open file for writing:\n") + path;

    auto writeNul = [&](const std::string& s) {
        file.write(s.data(), static_cast<qint64>(s.size()));
        file.write("\0", 1);
    };

    file.write(reinterpret_cast<const char*>(header), 52);
    file.write(solution.data(),    static_cast<qint64>(solution.size()));
    file.write(playerState.data(), static_cast<qint64>(playerState.size()));
    writeNul(""); // title
    writeNul(""); // author
    writeNul(""); // copyright
    for (const auto& cl : clueList)
        writeNul(cl);
    writeNul(""); // notes
    file.close();

    return {};
}

// ─────────────────────────────────────────────────────────────────────────────
//  Import
// ─────────────────────────────────────────────────────────────────────────────

PuzData PuzSerializer::importFromFile(const QString& path)
{
    PuzData data;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        data.errorMessage = QStringLiteral("Cannot open file:\n") + path;
        return data;
    }
    const QByteArray buf = file.readAll();
    file.close();

    // Minimum header size is 52 bytes
    if (buf.size() < 52) {
        data.errorMessage = QStringLiteral("File too small to be a valid .puz file.");
        return data;
    }

    // Verify magic
    if (std::memcmp(buf.constData() + 0x02, "ACROSS&DOWN", 11) != 0) {
        data.errorMessage = QStringLiteral("Not a valid .puz file (bad magic).");
        return data;
    }

    data.cols = static_cast<uint8_t>(buf[0x2C]);
    data.rows = static_cast<uint8_t>(buf[0x2D]);

    const int cellCount = data.rows * data.cols;
    const int solOffset = 0x34;
    const int stateOffset = solOffset + cellCount;
    const int stringsOffset = stateOffset + cellCount;

    if (buf.size() < stringsOffset) {
        data.errorMessage = QStringLiteral("File too small (truncated grid data).");
        return data;
    }

    // Solution
    data.solution.resize(data.rows);
    for (int r = 0; r < data.rows; ++r) {
        data.solution[r].resize(data.cols);
        for (int c = 0; c < data.cols; ++c)
            data.solution[r][c] = buf[solOffset + r * data.cols + c];
    }

    // Text strings
    int pos = stringsOffset;
    data.title     = readNulStr(buf, pos);
    data.author    = readNulStr(buf, pos);
    data.copyright = readNulStr(buf, pos);

    const uint16_t numClues =
        static_cast<uint16_t>(static_cast<uint8_t>(buf[0x2E]) |
                              (static_cast<uint8_t>(buf[0x2F]) << 8));
    data.clues.reserve(numClues);
    for (uint16_t i = 0; i < numClues && pos < buf.size(); ++i)
        data.clues.push_back(readNulStr(buf, pos));

    if (pos < buf.size())
        data.notes = readNulStr(buf, pos);

    return data;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Conversion helper
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> PuzSerializer::toGridLines(const PuzData& data)
{
    std::vector<std::string> lines;
    lines.reserve(data.rows);
    for (int r = 0; r < data.rows; ++r) {
        std::string line;
        line.reserve(data.cols);
        for (int c = 0; c < data.cols; ++c) {
            char ch = data.solution[r][c];
            if (ch == '.')
                line += '#';
            else if (ch == '-' || ch == ' ')
                line += '.'; // empty white cell
            else
                line += static_cast<char>(std::toupper((unsigned char)ch));
        }
        lines.push_back(std::move(line));
    }
    return lines;
}
