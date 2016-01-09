#include "GameLayer.h"

USING_NS_CC;
using namespace ui;
using namespace cocostudio;

#define WINSIZE Director::getInstance()->getWinSize() //画面サイズ

#define INIT_BLOCK_TIME 3.0 //ブロックが最初に出現する時間[s]
#define INTERVAL_BLOCK_TIME 1.5 //ブロックが出現するタイミング[s]
#define MOVING_TIME 3 //ブロックが消えるまでの時間
#define JUMP_V0 5 //ジャンプの初速度
#define GRAVITY 9.8 //重力
#define POSITION_RATE 70 //ゲーム倍率

//背景のJSON
#define BACKGROUND_JSON "JumpingPenguinUI/Background.ExportJson"

//ポップアップのJSON
#define POPUP_JSON "JumpingPenguinUI/Popup.ExportJson"

//ブロックのJSON
#define BLOCK_JSON "JumpingPenguinUI/Block.ExportJson"

//ペンギンのJSON
#define CHARACTER_JSON "JumpingPenguinCharacter/JumpingPenguinCharacter.ExportJson"

//シーン生成
Scene* GameLayer::createScene()
{
    auto scene = Scene::create(); //シーンを生成する
    auto layer = GameLayer::create(); //GameLayerクラスのレイヤーを生成する
    scene->addChild(layer); //シーンに対してレイヤーを追加する
    
    return scene; //シーンを返す
}

//初期化
bool GameLayer::init()
{
    if (!Layer::init())
        return false;
    
    //乱数の初期化
    std::random_device rd;
    _mt = std::mt19937(rd());
    _height = std::uniform_real_distribution<double>(-200.0, 200.0);
    
    //タップイベントの取得
    auto listener = EventListenerTouchAllAtOnce::create();
    listener->onTouchesBegan = CC_CALLBACK_2(GameLayer::onTouchesBegan, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    
    // Backgroundレイヤーの取得
    auto widget = GUIReader::getInstance()->widgetFromJsonFile(BACKGROUND_JSON);
    auto layout = dynamic_cast<Layout*>(widget);
    layout->setPosition(Point((WINSIZE - layout->getContentSize()) / 2));
    addChild(layout, Z_BG, T_BG);
    
    //キャラクターデータの取得
    ArmatureDataManager::getInstance()->addArmatureFileInfo(CHARACTER_JSON);
    
    return true;
}

//レイヤー表示時処理
void GameLayer::onEnter()
{
    Layer::onEnter();
    
    //スタート時のキャラクター表示
    _penguin = Armature::create("JumpingPenguinCharacter");
    _penguin->setPosition(Point(WINSIZE * 0.4));
    _penguin->getAnimation()->play("GameStart");
    addChild(_penguin, Z_Penguin);
    
    //Popupレイヤーの表示
    auto widget = GUIReader::getInstance()->widgetFromJsonFile(POPUP_JSON);
    auto popup = dynamic_cast<Layout*>(widget);
    popup->setPosition(Point((WINSIZE - popup->getContentSize()) / 2));
    addChild(popup, Z_GameStartLayer);
    
    //スタート画面フレームイン
    ActionManagerEx::getInstance()->playActionByName("Popup.ExportJson", "GameStart_In");
    _state = State::GameStart;
    
    //最初に戻るボタンの取得
    auto layer = dynamic_cast<Layout*>(popup->getChildByName("GameOver"));
    auto backButton = dynamic_cast<Button*>(layer->getChildByName("Back"));
    backButton->addTouchEventListener(this, toucheventselector(GameLayer::backCallback));
}

//update関数（毎フレーム処理）
void GameLayer::update(float dt)
{
    //ゲーム時間の積算
    _totalTime += dt;
    
    if (_penguin->getPositionY() < 0 ||
        contactBlock())
    {
        //画面から消えようとする または ブロックと衝突するとゲームオーバー
        
        //update関数を停止
        unscheduleUpdate();
        
        //全ブロックの動きを停止する
        for (auto node : getChildren())
        {
            if (node->getTag() == T_Block)
                node->stopAllActions();
        }
        
        //ゲームオーバー画面フレームイン
        ActionManagerEx::getInstance()->playActionByName("Popup.ExportJson", "GameOver_In");
        _state = State::GameOver;
    }
    else
    {
        //ペンギンの位置を変更
        float time = _totalTime - _jumpingTime;
        float newY = (JUMP_V0 * time - GRAVITY * time * time / 2) * POSITION_RATE + _jumpPointY;
        _penguin->setPositionY(newY);
        
        //次のブロックの表示
        if (_totalTime > _nextBlockTime)
        {
            //Blockレイヤーの取得
            auto widget = GUIReader::getInstance()->widgetFromJsonFile(BLOCK_JSON);
            auto blockLayout = dynamic_cast<Layout*>(widget);
            blockLayout->setAnchorPoint(Point(0, 0.5));
            blockLayout->setPosition(Point(WINSIZE.width, WINSIZE.height * 0.5 + _height(_mt)));
            addChild(blockLayout, Z_Block, T_Block);
            
            //ブロックを移動させる
            auto move = MoveBy::create(MOVING_TIME, Point((blockLayout->getContentSize().width + WINSIZE.width) * -1, 0));
            auto remove = RemoveSelf::create();
            auto seq = Sequence::create(move, remove, nullptr);
            blockLayout->runAction(seq);
            
            //次のブロックの時間を設定
            _nextBlockTime += INTERVAL_BLOCK_TIME;
        }
    }
}

void GameLayer::onTouchesBegan(const std::vector<Touch*>& touches, Event *event)
{
    if (_state == State::GameStart)
    {
        // スタート画面フレームアウト
        ActionManagerEx::getInstance()->playActionByName("Popup.ExportJson", "GameStart_Out");
        _state = State::Gaming;
        //時間の初期化
        _totalTime = 0;
        _jumpingTime = 0;
        _nextBlockTime = INIT_BLOCK_TIME;
        
        //ペンギンの位置を保持
        _jumpPointY = _penguin->getPositionY();
        
        //update関数を毎フレーム呼び出す
        scheduleUpdate();
    }
    else if (_state == State::Gaming)
    {
        //ペンギンの位置を保持
        _jumpPointY = _penguin->getPositionY();
        
        //ジャンプ開始時間を保持
        _jumpingTime = _totalTime;
        
        //ペンギンのジャンプアニメーションを行う
        _penguin->getAnimation()->play("Gaming");
    }
}

//ブロック衝突チェック
bool GameLayer::contactBlock()
{
    //あたり判定
    auto penguinRect = Rect(_penguin->getPositionX() - _penguin->getContentSize().width / 2,
                            _penguin->getPositionY() - _penguin->getContentSize().height / 2,
                            _penguin->getContentSize().width,
                            _penguin->getContentSize().height);
    
    //ブロックとの衝突チェック
    for (auto node : getChildren())
    {
        if (node->getTag() == T_Block)
        {
            //ブロックの取得
            auto block = (Layout*)node;
            
            auto lowerBlock = block->getChildByName("LowerBlock");
            auto upperBlock = block->getChildByName("UpperBlock");
            
            //下のブロックのRectを取得
            auto lowerBlockPoint = lowerBlock->convertToWorldSpace(getPosition());
            auto lowerBlockSize = lowerBlock->getContentSize();
            auto lowerBlockRect = Rect(lowerBlockPoint.x,
                                       lowerBlockPoint.y,
                                       lowerBlockSize.width,
                                       lowerBlockSize.height);
            
            //上のブロックのRectを取得
            auto upperBlockPoint = upperBlock->convertToWorldSpace(getPosition());
            auto upperBlockSize = upperBlock->getContentSize();
            auto upperBlockRect = Rect(upperBlockPoint.x,
                                       upperBlockPoint.y,
                                       upperBlockSize.width,
                                       upperBlockSize.height);
            
            //ペンギンとの衝突チェック
            if (penguinRect.intersectsRect(lowerBlockRect) ||
                penguinRect.intersectsRect(upperBlockRect))
            {
                //ペンギンがブロックと衝突するとゲームオーバー
                return true;
            }
        }
    }
    
    return false;
}

//戻るボタンタップイベント
void GameLayer::backCallback(cocos2d::Ref* sender, cocos2d::ui::TouchEventType type)
{
    if (type == TOUCH_EVENT_BEGAN)
    {
        //全ブロックを削除する
        while (true)
        {
            auto block = getChildByTag(T_Block);
            if (block)
                block->removeFromParent();
            else
                break;
        }
        
        // キャラクターを初期位置へ移動
        _penguin->setPosition(Point(WINSIZE * 0.4));
        _penguin->getAnimation()->play("GameStart");
        
        ActionManagerEx::getInstance()->playActionByName("Popup.ExportJson", "GameOver_Out");
        _state = State::GameStart;
    }
}
