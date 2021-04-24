[![Build Status](https://travis-ci.org/At-sushi/GTFramework.svg?branch=master)](https://travis-ci.org/At-sushi/GTFramework)
[![Coverage Status](https://coveralls.io/repos/github/At-sushi/GTFramework/badge.svg?branch=master)](https://coveralls.io/github/At-sushi/GTFramework?branch=master)
[![MIT License](http://img.shields.io/badge/license-MIT-blue.svg?style=flat)](./LICENSE)
# GTFramework
Goluah Task Framework ver1.50

「Goluah!」から流用したゲーム開発向け汎用タスクシステム

タスクのリスト式管理・検索・優先度つきの描画などが高速に行えます。

## 導入方法
src/systemフォルダ以下をプロジェクト内にコピーして使用してください。

ライブラリを作るMakefile等は今のところありません。

## 簡単な使い方
### タスク用クラスの定義
タスク用の基礎クラスを継承することで、GTFrameworkで管理することの出来るタスククラスを生成することが出来ます。

```cpp
    #include "task.h"
    
    using namespace gtf;
    
    class NewTask : TaskBase
    {
        virtual void Initialize() override						// 初期化時の処理
        {
            // タスク追加処理など
        }
        
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
```

GTFrameworkには3種類の基礎クラスがあります。

* `gtf::TaskBase` 通常タスク（下記の排他タスクに依存して（子タスクとして）振る舞う。　親排他タスクが実行中の時のみ実行される。　シーン中のオブジェクトなど。）
* `gtf::ExclusiveTaskBase` 排他タスク（他の排他タスクと同時に実行されない。　スタック可能。　シーン遷移などに。）
* `gtf::BackGroundTaskBase` 常駐タスク（タスク階層に依存せずに常時実行されるタスク）

これらの使い分けの詳細については，下記のリファレンスをご参照ください。

タスクが初期化され、実行可能な状態になると`Initialize`メソッドが実行されます。
排他タスクの場合、生成直後にはタスクがアクティブにならないため、子タスクの生成処理などはこの関数の内部で行うようにしてください。

タスクが実行されると`Execute`メソッドが実行され、`false`を返すとそのタスクは破棄されます。

`GetID`メソッドは、タスクに個別のIDを付けたいときに使えます（`0`にすると未設定扱いとなりますのでご注意ください。）

### 初期化・実行

```cpp
    using namespace gtf;
    
    TaskManager taskManager;
    
    taskManager.AddNewTask<NewTask>();
```

`gtf::TaskManager`クラスをインスタンス化するとタスクを管理できるようになります。

`gtf::TaskManager::AddNewTask`メソッドにテンプレート引数としてクラスをわたすと、タスクが自動で生成されます。
括弧の中に引数を記述すると、タスクのコンストラクタ引数として初期化時に渡すことが出来ます。
    
```cpp
    taskManager.AddNewTask<NewArgumentTask>(12, 2, "String");
```

タスクをすべて実行するには`gtf::TaskManager::Execute`メソッドを使います。

```cpp
    taskManager.Execute(0);
```

### 検索

```cpp
    auto p = taskManager.FindTask<NewTask>(12);
```

`gtf::TaskManager::FindTask`メソッドを使用すると、指定したIDのタスクのスマートポインタが手に入ります。
テンプレート引数としてタスクのクラス型を指定すると、動的キャストを行い、指定された型の`shared_ptr`を返します。
ただし排他タスクの検索は出来ません。

### 描画(優先度付き)

```cpp
    #include "task.h"
    
    using namespace gtf;
    
    class NewTask : TaskBase
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
```

`Draw`メソッドを使うには、`GetDrawPriority`メソッドをオーバーライドして、
あらかじめ優先度を定義しておく必要があります。

描画は(別に描画処理でなくてもいいのですが)優先度の数値が大きい順に処理され、`-1`のものは処理されません。

すべてのタスクの`Draw`メソッドを実行するには、`gtf::TaskManager`クラスの`Draw`メソッドを使います。

```cpp
    taskManager.Draw();
```

排他タスクの生成時、コンストラクタ引数`fallthroughDraw`に`true`を指定して生成すると、Draw処理のフォールスルーが行われるようになります。
この状態で`Draw`処理がコールされると、一つ下の階層のタスクも含めて描画することが出来ます。
```cpp
    class NewExTask : CExcusiveTaskBase
    {
        iNewExTask() : CExcusiveTaskBase(true)

        {
            // do something
            return true;
        }
    };
```


## リファレンス：
http://at-sushi.github.io/GTFramework/

詳しいことはこちらをご参照ください。
