#include "Storage.hpp"
using namespace geode::prelude;

static int faceOf(GJGameLevel* l) {
    if (l->m_autoLevel) return -1;
    if (l->m_demon.value() > 0) {
        switch (l->m_demonDifficulty) {
            case 3: return 7; case 4: return 8; case 5: return 9; case 6: return 10; default: return 6;
        }
    }
    if (l->m_ratings > 0) return l->m_ratingsSum / l->m_ratings;
    return 0;
}

static GrindEntry fromLevel(GJGameLevel* l) {
    GrindEntry e;
    e.id = l->m_levelID.value();
    e.name = std::string(l->m_levelName);
    e.creator = std::string(l->m_creatorName);
    e.face = faceOf(l);
    e.stars = l->m_stars.value();
    e.normal = l->m_normalPercent.value();
    e.practice = l->m_practicePercent;
    e.attempts = l->m_attempts.value();
    return e;
}

static matjson::Value toJson(GrindEntry const& e) {
    return matjson::makeObject({
        {"id", e.id}, {"name", e.name}, {"creator", e.creator}, {"face", e.face},
        {"stars", e.stars}, {"normal", e.normal}, {"practice", e.practice}, {"attempts", e.attempts},
    });
}

static std::vector<GrindEntry> loadRaw() {
    std::vector<GrindEntry> out;
    auto arr = Mod::get()->getSavedValue<matjson::Value>("grind");
    if (!arr.isArray()) return out;
    for (auto& v : arr) {
        GrindEntry e;
        e.id = (int)v["id"].asInt().unwrapOr(0);
        if (e.id <= 0) continue;
        e.name = v["name"].asString().unwrapOr("Unknown");
        e.creator = v["creator"].asString().unwrapOr("");
        e.face = (int)v["face"].asInt().unwrapOr(0);
        e.stars = (int)v["stars"].asInt().unwrapOr(0);
        e.normal = (int)v["normal"].asInt().unwrapOr(0);
        e.practice = (int)v["practice"].asInt().unwrapOr(0);
        e.attempts = (int)v["attempts"].asInt().unwrapOr(0);
        out.push_back(e);
    }
    return out;
}

static void saveRaw(std::vector<GrindEntry> const& list) {
    auto arr = matjson::Value::array();
    for (auto& e : list) arr.push(toJson(e));
    Mod::get()->setSavedValue("grind", arr);
    (void)Mod::get()->saveData();
}

std::vector<GrindEntry> grind::all() {
    auto list = loadRaw();
    bool changed = false;
    for (auto& e : list) {
        if (auto l = GameLevelManager::get()->getSavedLevel(e.id)) {
            auto f = fromLevel(l);
            if (f.name.empty()) f.name = e.name;
            if (f.creator.empty()) f.creator = e.creator;
            if (f.attempts != e.attempts || f.normal != e.normal || f.practice != e.practice || f.face != e.face) changed = true;
            e = f;
        }
    }
    if (changed) saveRaw(list);
    return list;
}

bool grind::has(int id) {
    for (auto& e : loadRaw()) if (e.id == id) return true;
    return false;
}

void grind::add(GJGameLevel* level) {
    if (!isOnline(level) || has(level->m_levelID.value())) return;
    auto list = loadRaw();
    list.insert(list.begin(), fromLevel(level));
    saveRaw(list);
}

void grind::remove(int id) {
    auto list = loadRaw();
    std::erase_if(list, [id](GrindEntry const& e) { return e.id == id; });
    saveRaw(list);
}

void grind::updateFrom(GJGameLevel* level) {
    if (!level) return;
    auto list = loadRaw();
    for (auto& e : list) {
        if (e.id == level->m_levelID.value()) {
            auto f = fromLevel(level);
            if (f.name.empty()) f.name = e.name;
            if (f.creator.empty()) f.creator = e.creator;
            e = f;
            saveRaw(list);
            return;
        }
    }
}

bool grind::isOnline(GJGameLevel* level) {
    return level && level->m_levelID.value() > 0
        && level->m_levelType != GJLevelType::Main && level->m_levelType != GJLevelType::Editor;
}

GJGameLevel* grind::levelFor(GrindEntry const& e) {
    if (auto l = GameLevelManager::get()->getSavedLevel(e.id)) return l;
    auto l = GJGameLevel::create();
    l->m_levelID = e.id;
    l->m_levelName = e.name;
    l->m_creatorName = e.creator;
    return l;
}
