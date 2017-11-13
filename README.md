# GTFramework
Goluah Task Framework ver1.00

「Goluah!」から流用したゲーム開発向け汎用タスクシステム

タスクのリスト式管理・検索・優先度つきの描画などが高速に行えます。

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

* CTaskBase 通常タスク（下記の排他タスクに依存して（子タスクとして）振る舞う。　親排他タスクが実行中の時のみ実行される。　シーン中のオブジェクトなど。）
* CExclusiveTaskBase 排他タスク（他の排他タスクと同時に実行されない。　スタック可能。　シーン遷移などに。）
* CBackGroundTaskBase 常駐タスク（タスク階層に依存せずに常時実行されるタスク）

これらの使い分けの詳細については，下記のリファレンスをご参照ください。

タスクが実行されるとExecuteメソッドが実行され、falseを返すとそのタスクは破棄されます。

GetIDメソッドは、タスクに個別のIDを付けたいときに使えます（0にすると未設定扱いとなりますのでご注意ください。）

### 初期化・実行
    using namespace GTF;
    
    CTaskManager taskManager;
    
    taskManager.AddNewTask<CNewTask>();

CTaskManagerクラスをインスタンス化するとタスクを管理できるようになります。

AddNewTaskメソッドにテンプレート引数としてクラスをわたすと、タスクが自動で生成されます。
括弧の中に引数を記述すると、タスクのコンストラクタ引数として初期化時に渡すことが出来ます。
    
    taskManager.AddNewTask<CNewArgumentTask>(12, 2, "String");

タスクをすべて実行するにはExecuteメソッドを使います。

    taskManager.Execute(0);

### 検索
    auto p = taskManager.FindTask<CNewTask>(12);

FindTaskメソッドを使用すると、指定したIDのタスクのスマートポインタが手に入ります。
テンプレート引数としてタスクのクラス型を指定すると、動的キャストを行い、指定された型のshared_ptrを返します。
ただし排他タスクの検索は出来ません。

### 描画(優先度付き)
    #include "task.h"
    
    using namespace GTF;
    
    class CNewTask : CTaskBase
    {
        virtual void Draw() override					// Draw実行時の処理
        {
            // do something
        }
        
        virtual int GetDrawPriority() const override
        {
            return 0;	// 描画の優先度。数値の大きいものから先に処理される。-1で無効。
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
