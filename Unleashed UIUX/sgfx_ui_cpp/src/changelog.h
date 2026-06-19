#pragma once
#include <string>
#include <vector>

// Per-car delivery state from each car's CHANGELOG.md: the latest version + date and how
// many change bullets that version carries. A read-only "what version is each car at, and
// when" view (the changelog is the delivery record). Path: viewer3d.json "bmw_git_root".
namespace changelog {

struct Entry {
    std::string brand;     // BMW, MINI, RR
    std::string id;        // G65_EVO, ...
    std::string version;   // "3.1.0" (or "" if none parsed)
    std::string date;      // "2026-05-13"
    int changes = 0;       // bullet lines in the latest version section
};

struct Log {
    bool configured = false;
    bool rootExists = false;
    std::string root;
    std::vector<Entry> entries;
};

Log Scan();   // reads viewer3d.json "bmw_git_root"; each car's CHANGELOG.md, latest version

} // namespace changelog
