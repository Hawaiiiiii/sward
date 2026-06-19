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
};

struct Roster {
    bool configured = false;
    bool rootExists = false;
    std::string root;
    std::vector<Car> cars;
    int exportedCount = 0;
    int withPersp = 0;
    std::vector<std::string> brands;   // distinct, in roster order
};

Roster Scan();   // reads viewer3d.json "bmw_git_root"; empty if unset

} // namespace fleet
