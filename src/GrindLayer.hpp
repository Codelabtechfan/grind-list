#pragma once
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
class GrindLayer : public cocos2d::CCLayer {
    cocos2d::CCMenu* m_listMenu = nullptr;
    cocos2d::CCNode* m_listNode = nullptr;
    cocos2d::CCLabelBMFont* m_pageLabel = nullptr;
    int m_page = 0;
    bool init() override;
    void keyBackClicked() override;
    void onEnter() override;
    void onBack(cocos2d::CCObject*);
    void onAdd(cocos2d::CCObject*);
    void onView(cocos2d::CCObject*);
    void onRemove(cocos2d::CCObject*);
    void onPrev(cocos2d::CCObject*);
    void onNext(cocos2d::CCObject*);
    void onUp(cocos2d::CCObject*);
    void onDown(cocos2d::CCObject*);
    void onTop(cocos2d::CCObject*);
    void loadThumb(int id, cocos2d::CCNode* slot);
    std::unordered_map<int, std::unique_ptr<geode::async::TaskHolder<geode::utils::web::WebResponse>>> m_thumbTasks;
    void refresh();
public:
    static GrindLayer* create();
    static cocos2d::CCScene* scene();
};
