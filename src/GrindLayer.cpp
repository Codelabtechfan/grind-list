#include "GrindLayer.hpp"
#include "Storage.hpp"
#include <fmt/format.h>
using namespace geode::prelude;

static constexpr int PER_PAGE = 5;
static constexpr float THUMB_W = 60.f, THUMB_H = 34.f;
static std::unordered_map<int, Ref<CCTexture2D>> s_thumbs;
static std::unordered_set<int> s_missing;

static std::string thumbBase() {
    if (auto lt = Loader::get()->getLoadedMod("cdc.level_thumbnails")) {
        auto v = lt->getSettingValue<std::string>("level-thumbnails-api");
        while (!v.empty() && v.back() == '/') v.pop_back();
        if (!v.empty()) return v;
    }
    return "https://levelthumbs.prevter.me";
}

static void setThumb(CCNode* slot, CCTexture2D* tex) {
    slot->removeAllChildren();
    auto spr = CCSprite::createWithTexture(tex);
    if (!spr) return;
    auto sz = spr->getContentSize();
    float sc = std::min(THUMB_W / sz.width, THUMB_H / sz.height);
    spr->setScale(sc);
    spr->setPosition({THUMB_W / 2, THUMB_H / 2});
    slot->addChild(spr);
}

void GrindLayer::loadThumb(int id, CCNode* slot) {
    if (auto it = s_thumbs.find(id); it != s_thumbs.end()) { setThumb(slot, it->second); return; }
    if (s_missing.count(id)) return;
    if (m_thumbTasks.count(id)) return;
    auto holder = std::make_unique<async::TaskHolder<web::WebResponse>>();
    auto self = this; // holder is owned by this layer, so the task is cancelled if it goes away
    holder->spawn(web::WebRequest().get(fmt::format("{}/thumbnail/{}/small", thumbBase(), id)),
        [self, id](web::WebResponse res) {
            if (!res.ok()) { s_missing.insert(id); return; }
            auto data = res.data();
            auto img = new CCImage();
            if (!img->initWithImageData(data.data(), data.size())) { delete img; s_missing.insert(id); return; }
            auto tex = new CCTexture2D();
            if (!tex->initWithImage(img)) { delete img; tex->release(); s_missing.insert(id); return; }
            delete img;
            s_thumbs[id] = tex;
            tex->release();
            self->refresh();
        });
    m_thumbTasks[id] = std::move(holder);
}


GrindLayer* GrindLayer::create() {
    auto ret = new GrindLayer();
    if (ret->init()) { ret->autorelease(); return ret; }
    delete ret;
    return nullptr;
}

CCScene* GrindLayer::scene() {
    auto s = CCScene::create();
    s->addChild(GrindLayer::create());
    return s;
}

bool GrindLayer::init() {
    if (!CCLayer::init()) return false;
    auto win = CCDirector::get()->getWinSize();

    auto bg = CCSprite::create("GJ_gradientBG.png");
    bg->setAnchorPoint({0, 0});
    bg->setScaleX(win.width / bg->getContentSize().width);
    bg->setScaleY(win.height / bg->getContentSize().height);
    bg->setColor({0, 102, 255});
    this->addChild(bg, -2);

    auto title = CCLabelBMFont::create("Level Grinding", "bigFont.fnt");
    title->setPosition({win.width / 2, win.height - 22});
    title->setScale(0.8f);
    this->addChild(title);

    auto panel = CCScale9Sprite::create("GJ_square01.png");
    panel->setContentSize({380, 230});
    panel->setPosition({win.width / 2, win.height / 2 - 12});
    this->addChild(panel, -1);

    auto menu = CCMenu::create();
    menu->setPosition({0, 0});
    this->addChild(menu, 5);

    auto backSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    auto back = CCMenuItemSpriteExtra::create(backSpr, this, menu_selector(GrindLayer::onBack));
    back->setPosition({25, win.height - 25});
    menu->addChild(back);

    auto addSpr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
    auto add = CCMenuItemSpriteExtra::create(addSpr, this, menu_selector(GrindLayer::onAdd));
    add->setPosition({win.width - 30, 30});
    menu->addChild(add);

    auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    auto prev = CCMenuItemSpriteExtra::create(prevSpr, this, menu_selector(GrindLayer::onPrev));
    prev->setPosition({win.width / 2 - 215, win.height / 2 - 12});
    menu->addChild(prev);
    auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    nextSpr->setFlipX(true);
    auto next = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(GrindLayer::onNext));
    next->setPosition({win.width / 2 + 215, win.height / 2 - 12});
    menu->addChild(next);

    m_pageLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_pageLabel->setScale(0.5f);
    m_pageLabel->setPosition({win.width / 2, 12});
    this->addChild(m_pageLabel);

    m_listNode = CCNode::create();
    this->addChild(m_listNode, 2);
    m_listMenu = CCMenu::create();
    m_listMenu->setPosition({0, 0});
    this->addChild(m_listMenu, 3);

    this->setKeypadEnabled(true);
    return true;
}

void GrindLayer::onEnter() {
    CCLayer::onEnter();
    grind::addMode = false;
    refresh();
}

void GrindLayer::refresh() {
    m_listNode->removeAllChildren();
    m_listMenu->removeAllChildren();
    auto win = CCDirector::get()->getWinSize();
    auto list = grind::all();
    int pages = std::max(1, ((int)list.size() + PER_PAGE - 1) / PER_PAGE);
    m_page = std::clamp(m_page, 0, pages - 1);
    m_pageLabel->setString(fmt::format("Page {}/{}  -  {} levels", m_page + 1, pages, list.size()).c_str());

    if (list.empty()) {
        auto lbl = CCLabelBMFont::create("No levels yet!\nPress + to add levels you are grinding.", "goldFont.fnt");
        lbl->setAlignment(kCCTextAlignmentCenter);
        lbl->setScale(0.55f);
        lbl->setPosition({win.width / 2, win.height / 2 - 12});
        m_listNode->addChild(lbl);
        return;
    }

    float top = win.height / 2 - 12 + 115 - 24;
    for (int i = 0; i < PER_PAGE; i++) {
        int idx = m_page * PER_PAGE + i;
        if (idx >= (int)list.size()) break;
        auto& e = list[idx];
        float y = top - i * 44;
        float left = win.width / 2 - 180;

        auto row = CCScale9Sprite::create("square02b_001.png");
        row->setContentSize({360, 40});
        row->setColor({0, 0, 0});
        row->setOpacity(60);
        row->setPosition({win.width / 2, y});
        m_listNode->addChild(row);

        auto slot = CCLayerColor::create({0, 0, 0, 120}, THUMB_W, THUMB_H);
        slot->setPosition({left + 4, y - THUMB_H / 2});
        m_listNode->addChild(slot);
        auto ph = CCLabelBMFont::create(s_missing.count(e.id) ? "No image" : "...", "chatFont.fnt");
        ph->setScale(0.45f);
        ph->setPosition({THUMB_W / 2, THUMB_H / 2});
        slot->addChild(ph);
        loadThumb(e.id, slot);
        left += 64;

        auto face = GJDifficultySprite::create(e.face, GJDifficultyName::Short);
        face->setScale(0.5f);
        face->setPosition({left + 15, y + 2});
        m_listNode->addChild(face);

        auto name = CCLabelBMFont::create(e.name.c_str(), "bigFont.fnt");
        name->setAnchorPoint({0, 0.5f});
        name->limitLabelWidth(110, 0.5f, 0.1f);
        name->setPosition({left + 32, y + 10});
        m_listNode->addChild(name);

        auto creator = CCLabelBMFont::create(fmt::format("by {}", e.creator).c_str(), "goldFont.fnt");
        creator->setAnchorPoint({0, 0.5f});
        creator->limitLabelWidth(60, 0.4f, 0.1f);
        creator->setPosition({left + 32 + name->getScaledContentSize().width + 5, y + 10});
        m_listNode->addChild(creator);

        auto stats = CCLabelBMFont::create(
            fmt::format("Normal {}%  Practice {}%  Att {}", e.normal, e.practice, e.attempts).c_str(), "chatFont.fnt");
        stats->setAnchorPoint({0, 0.5f});
        stats->setScale(0.6f);
        stats->setPosition({left + 32, y - 9});
        m_listNode->addChild(stats);

        auto viewSpr = ButtonSprite::create("View", "bigFont.fnt", "GJ_button_01.png", 0.6f);
        viewSpr->setScale(0.6f);
        auto view = CCMenuItemSpriteExtra::create(viewSpr, this, menu_selector(GrindLayer::onView));
        view->setTag(e.id);
        view->setPosition({win.width / 2 + 100, y});
        m_listMenu->addChild(view);

        auto delSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
        delSpr->setScale(0.6f);
        auto del = CCMenuItemSpriteExtra::create(delSpr, this, menu_selector(GrindLayer::onRemove));
        del->setTag(e.id);
        del->setPosition({win.width / 2 + 138, y});
        m_listMenu->addChild(del);

        auto mk = [&](float rot, SEL_MenuHandler sel, float dy, float sc) {
            auto s = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
            s->setRotation(rot);
            s->setScale(sc);
            auto b = CCMenuItemSpriteExtra::create(s, this, sel);
            b->setTag(e.id);
            b->setPosition({win.width / 2 + 166, y + dy});
            m_listMenu->addChild(b);
        };
        if (idx > 0) mk(90.f, menu_selector(GrindLayer::onUp), 10.f, 0.32f);
        if (idx < (int)list.size() - 1) mk(-90.f, menu_selector(GrindLayer::onDown), -10.f, 0.32f);
        if (idx > 0) {
            auto ts = ButtonSprite::create("Top", "goldFont.fnt", "GJ_button_04.png", 0.5f);
            ts->setScale(0.45f);
            auto tb = CCMenuItemSpriteExtra::create(ts, this, menu_selector(GrindLayer::onTop));
            tb->setTag(e.id);
            tb->setPosition({win.width / 2 + 119, y + 15});
            m_listMenu->addChild(tb);
        }
    }
}

void GrindLayer::onView(CCObject* sender) {
    int id = static_cast<CCNode*>(sender)->getTag();
    for (auto& e : grind::all()) {
        if (e.id != id) continue;
        auto level = grind::levelFor(e);
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, LevelInfoLayer::scene(level, false)));
        return;
    }
}

void GrindLayer::onRemove(CCObject* sender) {
    int id = static_cast<CCNode*>(sender)->getTag();
    Ref<GrindLayer> self = this;
    geode::createQuickPopup("Remove level", "Remove this level from your <cg>grind list</c>?", "Cancel", "Remove",
        [self, id](FLAlertLayer*, bool yes) {
            if (!yes) return;
            grind::remove(id);
            self->refresh();
        });
}

void GrindLayer::onAdd(CCObject*) {
    grind::addMode = true;
    CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, LevelSearchLayer::scene(0)));
    Notification::create("Search for a level, then press + to add it", NotificationIcon::Info)->show();
}

void GrindLayer::onUp(CCObject* s) { grind::move(static_cast<CCNode*>(s)->getTag(), -1); refresh(); }
void GrindLayer::onDown(CCObject* s) { grind::move(static_cast<CCNode*>(s)->getTag(), 1); refresh(); }
void GrindLayer::onTop(CCObject* s) { grind::move(static_cast<CCNode*>(s)->getTag(), -100000); m_page = 0; refresh(); }
void GrindLayer::onPrev(CCObject*) { m_page--; refresh(); }
void GrindLayer::onNext(CCObject*) { m_page++; refresh(); }

void GrindLayer::onBack(CCObject*) {
    grind::addMode = false;
    CCDirector::get()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}
void GrindLayer::keyBackClicked() { onBack(nullptr); }
