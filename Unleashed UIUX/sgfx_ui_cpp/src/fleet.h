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
    long long ramsesBytes = 0;   // export/exported.ramses size (the delivery's "Ramses Size")
    long long logicBytes  = 0;   // export/exported.rlogic size ("Logic Size")
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

// per-test detail for one car: each baseline test (export/tests/expected/<name>.png) and
// whether the latest run differs from it (a diff/<name>_*.png exists). Differing first.
struct TestResult { std::string name; bool differs = false; };
std::vector<TestResult> CarTests(const std::string& id);

// one-shot car hand-off: the Fleet screen sets the picked car on Enter, the 3D view
// consumes it on entry (then it clears, so the next entry uses the active profile).
void SetSelected(const std::string& id);
std::string TakeSelected();

} // namespace fleet
