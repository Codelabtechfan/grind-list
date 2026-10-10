#pragma once
#include <Geode/Geode.hpp>
struct GrindEntry {
    int id = 0;
    std::string name, creator;
    int face = 0, stars = 0, normal = 0, practice = 0, attempts = 0;
    bool epicFeatured = false;
};
namespace grind {
    std::vector<GrindEntry> all();
    bool has(int id);
    void add(GJGameLevel* level);
    void remove(int id);
    void updateFrom(GJGameLevel* level); // refresh cached data if in list
    bool isOnline(GJGameLevel* level);
    GJGameLevel* levelFor(GrindEntry const& e);
    inline bool addMode = false;
}
