#ifndef __JumpingPenguin__GameLayer__
#define __JumpingPenguin__GameLayer__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "cocostudio/CocoStudio.h"
#include <random>

class GameLayer : public cocos2d::Layer
{
protected:
    //タグ
    enum Tag
    {
        T_BG = 1,
        T_Block,
        T_Penguin,
    };
    
    //Zオーダー
    enum ZOrder
    {
        Z_BG = 1,
        Z_Penguin,
        Z_Block,
        Z_GameStartLayer,
        Z_GameOverLayer,
    };
    
    //ゲームの状態
    enum class State
    {
        GameStart,
        Gaming,
        GameOver,
    };
    
    std::mt19937 _mt; //乱数生成エンジン
    std::uniform_real_distribution<double> _height; //乱数の分布
    
    State _state; //ゲームの状態
    float _totalTime; //ゲーム時間
    float _nextBlockTime; //次のブロックが出るまでの時間
    cocostudio::Armature* _penguin; //ペンギン
    float _jumpingTime; //ジャンプを開始した時間
    float _jumpPointY; //ジャンプを開始した位置
    
    bool contactBlock(); //ブロック衝突チェック
    
public:
    static cocos2d::Scene* createScene(); //シーン生成
    virtual bool init(); //初期化
    CREATE_FUNC(GameLayer); //create関数生成マクロ
    virtual void onEnter(); //レイヤー表示時処理
    virtual void update(float dt); //update関数（毎フレーム処理）
    
    //マルチタップイベント
    void onTouchesBegan(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event *event);
    
    //戻るボタンタップイベント
    void backCallback(cocos2d::Ref* sender, cocos2d::ui::TouchEventType type);
};

#endif /* defined(__JumpingPenguin__GameLayer__) */
