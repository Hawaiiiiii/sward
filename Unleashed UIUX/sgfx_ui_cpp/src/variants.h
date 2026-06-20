#pragma once
#include <string>
#include <vector>
#include <map>

// Model / powertrain / trim coverage from cars/model_variants.json: per model, which
// powertrains (ICE / PHEV / HEV / FCEV / BEV) exist and how many trim variants each has.
// A read-only variant-coverage view. Path: viewer3d.json "bmw_git_root".
namespace variants {

struct Model {
    std::string name;                       // PINT, G65, NA0, ...
    std::map<std::string, int> powertrains; // ICE->21, BEV->4, ...
    int totalTrims = 0;
};

struct Catalog {
    bool configured = false;
    bool rootExists = false;                // model_variants.json present
    std::string root;
    std::vector<Model> models;
    std::vector<std::string> powertrains;   // union across models, canonical order
    int totalVariants = 0;
};

Catalog Scan();   // reads viewer3d.json "bmw_git_root"; cars/model_variants.json

} // namespace variants
