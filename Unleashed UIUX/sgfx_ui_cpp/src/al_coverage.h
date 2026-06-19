#pragma once
#include <string>
#include <vector>

// Ambient Layer screenshot-test coverage, read from the AL assets repo's per-scene
// tests/{expected,actuals,diff} folders (Confluence "How to screenshot test AL"). A
// read-only readiness slice: it reports what a test run left on disk, it runs nothing.
// The operator-local repo path comes from viewer3d.json ("al_assets_root").
namespace al {

struct Cell {
    std::string brand;     // BMW, BMW_M, ALPINA, RR_Parallax, RR_Prismatic, RR_EternalBeauty
    std::string screen;    // CID, FPD, PADI, PHUD, FPK
    int  expected = 0;     // baseline screenshots present
    int  diffs    = 0;     // regression diffs present (>0 = a visual change since baseline)
    bool actuals  = false; // a run has produced actuals
    std::string status;    // "pass" | "diff" | "notrun" | "nobaseline"
};

struct Coverage {
    bool configured = false;   // al_assets_root set in viewer3d.json
    bool rootExists = false;   // the assets/ tree is present at that path
    std::string root;
    std::vector<Cell> cells;
    int pass = 0, diff = 0, notrun = 0, nobaseline = 0;
};

Coverage Scan();   // reads viewer3d.json "al_assets_root"; an empty/unconfigured Coverage if unset

} // namespace al
