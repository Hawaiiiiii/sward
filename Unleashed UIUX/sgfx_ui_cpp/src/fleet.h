#pragma once
#include <string>
#include <vector>

// Fleet readiness across the 3D-car models repo: per car, is there an export
// (export/exported.ramses) and how many QA camera-perspective sets are defined
// (perspectives_*.json). A read-only "can I render / screenshot-test this car?" view,
// read straight from the repo. Path is operator-local (viewer3d.json "bmw_git_root").
namespace fleet {

struct Car {
    std::string brand;       // BMW, MINI, RR, IPN, UCAP
    std::string id;          // G65_EVO, F70, ...
    bool exported = false;   // export/exported.ramses present
    int  perspSets = 0;      // perspectives_*.json count (QA camera configs)
    // screenshot tests (export/tests/{expected,actuals,diff})
    int  testExpected = 0;   // baseline screenshots
    int  testDiff     = 0;   // regression diffs (>0 = a visual change)
    bool testActuals  = false;
    std::string testStatus;  // "pass" | "diff" | "notrun" | "none"
};

struct Roster {
    bool configured = false;
    bool rootExists = false;
    std::string root;
    std::vector<Car> cars;
    int exportedCount = 0;
    int withPersp = 0;
    int tested = 0, testPass = 0, testDiff = 0, testNotrun = 0;
    std::vector<std::string> brands;   // distinct, in roster order
};

Roster Scan();   // reads viewer3d.json "bmw_git_root"; empty if unset

// one-shot car hand-off: the Fleet screen sets the picked car on Enter, the 3D view
// consumes it on entry (then it clears, so the next entry uses the active profile).
void SetSelected(const std::string& id);
std::string TakeSelected();

} // namespace fleet
