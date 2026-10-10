#include "GrindLayer.hpp"
#include "Storage.hpp"
#include <fmt/format.h>
using namespace geode::prelude;

static constexpr int PER_PAGE = 5;

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

        auto face = GJDifficultySprite::create(e.face, GJDifficultyName::Short);
        face->setScale(0.6f);
        face->setPosition({left + 22, y + 2});
        m_listNode->addChild(face);

        auto name = CCLabelBMFont::create(e.name.c_str(), "bigFont.fnt");
        name->setAnchorPoint({0, 0.5f});
        name->limitLabelWidth(170, 0.5f, 0.1f);
        name->setPosition({left + 45, y + 10});
        m_listNode->addChild(name);

        auto creator = CCLabelBMFont::create(fmt::format("by {}", e.creator).c_str(), "goldFont.fnt");
        creator->setAnchorPoint({0, 0.5f});
        creator->limitLabelWidth(120, 0.4f, 0.1f);
        creator->setPosition({left + 45 + name->getScaledContentSize().width + 5, y + 10});
        m_listNode->addChild(creator);

        auto stats = CCLabelBMFont::create(
            fmt::format("Normal {}%   Practice {}%   Attempts {}", e.normal, e.practice, e.attempts).c_str(), "chatFont.fnt");
        stats->setAnchorPoint({0, 0.5f});
        stats->setScale(0.6f);
        stats->setPosition({left + 45, y - 9});
        m_listNode->addChild(stats);

        auto viewSpr = ButtonSprite::create("View", "bigFont.fnt", "GJ_button_01.png", 0.6f);
        viewSpr->setScale(0.6f);
        auto view = CCMenuItemSpriteExtra::create(viewSpr, this, menu_selector(GrindLayer::onView));
        view->setTag(e.id);
        view->setPosition({win.width / 2 + 120, y});
        m_listMenu->addChild(view);

        auto delSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
        delSpr->setScale(0.6f);
        auto del = CCMenuItemSpriteExtra::create(delSpr, this, menu_selector(GrindLayer::onRemove));
        del->setTag(e.id);
        del->setPosition({win.width / 2 + 163, y});
        m_listMenu->addChild(del);
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

void GrindLayer::onPrev(CCObject*) { m_page--; refresh(); }
void GrindLayer::onNext(CCObject*) { m_page++; refresh(); }

void GrindLayer::onBack(CCObject*) {
    grind::addMode = false;
    CCDirector::get()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}
void GrindLayer::keyBackClicked() { onBack(nullptr); }
