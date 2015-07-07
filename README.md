# GTFramework
Goluah Task Framework ver0.99b

「Goluah!」から流用したゲーム開発向け汎用タスクシステム

## 導入方法
src/systemフォルダ以下をプロジェクト内にコピーして使用してください。

ライブラリを作るMakefile等は今のところありません。

## 簡単な使い方
### タスク用クラスの定義
タスク用の基礎クラスを継承することで、GTFrameworkで管理することの出来るタスククラスを生成することが出来ます。

    #include "task.h"
    
    using namespace GTF;
    
    class CNewTask : CTaskBase
    {
        virtual bool Execute(double elapsedTime) override					// 実行時の処理
        {
            // do something
            return true;
        }
        
        virtual unsigned int GetID() const override
        {
            return 12;
        }
    };
    
GTFrameworkには3種類の基礎クラスがあります。

* CTaskBase 通常タスク
* CExclusiveTaskBase 排他タスク
* CBackGroundTaskBase 常駐タスク

これらの使い分けの詳細については，下記のリファレンスをご参照ください。

タスクが実行されるとExecuteメソッドが実行され、falseを返すとそのタスクは破棄されます。

GetIDメソッドは、タスクに個別のIDを付けたいときに使えます（0にすると未設定となりますのでご注意ください。）

### 初期化・実行
    using namespace GTF;
    
    CTaskManager taskManager;
    
    taskManager.AddTask(new CNewTask());

CTaskManagerクラスをインスタンス化するとタスクを管理できるようになります。
AddTaskメソッドにnew生成したタスクをわたすと自動で生成～破棄まで管理してくれます。

タスクをすべて実行するにはExecuteメソッドを使います。

    taskManager.Execute(0);

### 検索
    auto p = taskManager.FindTask<CNewTask>(12);
    auto pbg = taskManager.FindBGTask<CNewBGTask>(33);

FindTaskメソッドを使用すると、指定したIDの通常タスクのスマートポインタが手に入ります。
常駐タスクと、メソッドが分かれてることに注意してください（常駐タスクのはFindBGTask）。

排他タスクの検索は出来ません。

### 描画(優先度付き)
    #include "task.h"
    
    using namespace GTF;
    
    class CNewTask : CTaskBase
    {
        virtual void Draw() override					// Draw実行時の処理
        {
            // do something
            return true;
        }
        
        virtual unsigned int GetDrawPriority() const override
        {
            return 0;	// 描画の優先度。数値の大きいものから先に処理される。-1で無効
        }
    };
    
Drawメソッドを使うには、GetDrawPriorityメソッドをオーバーライドして、
あらかじめ優先度を定義しておく必要があります。

描画は(別に描画処理でなくてもいいのですが)優先度の数値が大きい順に処理され、-1のものは処理されません。

すべてのタスクのDrawメソッドを実行するには、CTaskManagerクラスのDrawメソッドを使います。

    taskManager.Draw();

## リファレンス：
http://at-sushi.github.io/GTFramework/

詳しいことはこちらをご参照ください。
