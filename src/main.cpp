#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelCell.hpp>
#include "GrindLayer.hpp"
#include "Storage.hpp"
using namespace geode::prelude;

class $modify(GLMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto menu = this->getChildByID("main-menu");
        if (!menu) return true;
        auto icon = menu->getChildByID("icon-kit-button");
        auto play = menu->getChildByID("play-button");
        auto editor = menu->getChildByID("editor-button");

        auto spr = CCSprite::create("grindBtn.png"_spr);
        if (!spr) return true;
        float target = editor ? editor->getContentSize().height : 90.f;
        spr->setScale(target / spr->getContentSize().height);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(GLMenuLayer::onGrind));
        btn->setID("grind-button"_spr);

        if (icon && play && editor) {
            float gap = editor->getPositionX() - play->getPositionX();
            float shift = gap * 0.5f + 0.f;
            // shift the three buttons left, then place the new one where the gap continues
            icon->setPositionX(icon->getPositionX() - shift);
            play->setPositionX(play->getPositionX() - shift);
            editor->setPositionX(editor->getPositionX() - shift);
            btn->setPosition({editor->getPositionX() + gap * 0.85f, editor->getPositionY()});
            menu->addChild(btn);
        } else {
            menu->addChild(btn);
            menu->updateLayout();
        }
        return true;
    }

    void onGrind(CCObject*) {
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, GrindLayer::scene()));
    }
};

static CCSprite* toggleSprite(bool in) {
    return CircleButtonSprite::createWithSpriteFrameName(in ? "edit_delBtn_001.png" : "edit_addCBtn_001.png",
        1.f, in ? CircleBaseColor::Pink : CircleBaseColor::Green, CircleBaseSize::Small);
}

class $modify(GLLevelInfoLayer, LevelInfoLayer) {
    struct Fields { CCMenuItemSpriteExtra* btn = nullptr; };

    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        if (!grind::isOnline(level)) return true;
        grind::updateFrom(level);
        bool in = grind::has(level->m_levelID.value());
        auto btn = CCMenuItemSpriteExtra::create(toggleSprite(in), this, menu_selector(GLLevelInfoLayer::onGrindToggle));
        btn->setID("grind-toggle"_spr);
        m_fields->btn = btn;
        if (auto side = this->getChildByID("left-side-menu")) {
            side->addChild(btn);
            side->updateLayout();
        } else {
            auto win = CCDirector::get()->getWinSize();
            auto m = CCMenu::create();
            m->setPosition({30, win.height / 2 - 60});
            m->addChild(btn);
            this->addChild(m, 10);
        }
        if (grind::addMode && !in) {
            Notification::create("Press the green + to add this level to your grind list", NotificationIcon::Info)->show();
        }
        return true;
    }

    void levelDownloadFinished(GJGameLevel* level) {
        LevelInfoLayer::levelDownloadFinished(level);
        grind::updateFrom(m_level);
    }

    void onGrindToggle(CCObject*) {
        if (!m_level) return;
        int id = m_level->m_levelID.value();
        bool in = grind::has(id);
        if (in) grind::remove(id); else grind::add(m_level);
        in = !in;
        m_fields->btn->setNormalImage(toggleSprite(in));
        Notification::create(in ? "Added to grind list" : "Removed from grind list",
            in ? NotificationIcon::Success : NotificationIcon::Info)->show();
    }
};

class $modify(GLLevelCell, LevelCell) {
    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);
        if (!grind::addMode || !grind::isOnline(level)) return;
        auto menu = m_mainLayer ? m_mainLayer->getChildByID("main-menu") : nullptr;
        if (!menu) return;
        bool in = grind::has(level->m_levelID.value());
        auto spr = toggleSprite(in);
        spr->setScale(0.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(GLLevelCell::onGrindCell));
        btn->setID("grind-cell-toggle"_spr);
        auto view = menu->getChildByID("view-button");
        btn->setPosition(view ? ccp(view->getPositionX() - 55.f, view->getPositionY()) : ccp(250.f, -25.f));
        menu->addChild(btn);
    }

    void onGrindCell(CCObject* sender) {
        if (!m_level) return;
        int id = m_level->m_levelID.value();
        bool in = grind::has(id);
        if (in) grind::remove(id); else grind::add(m_level);
        in = !in;
        auto spr = toggleSprite(in);
        spr->setScale(0.6f);
        static_cast<CCMenuItemSpriteExtra*>(sender)->setNormalImage(spr);
        Notification::create(in ? "Added to grind list" : "Removed from grind list",
            in ? NotificationIcon::Success : NotificationIcon::Info)->show();
    }
};
