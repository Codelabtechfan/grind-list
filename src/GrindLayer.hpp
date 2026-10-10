#pragma once
#include <Geode/Geode.hpp>
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
    void refresh();
public:
    static GrindLayer* create();
    static cocos2d::CCScene* scene();
};
