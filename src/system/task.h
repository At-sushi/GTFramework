/*!
*	@file
*	@brief タスク(?)管理・定義
*/
#pragma once
#include <string>
#include <list>
#include <stack>
#include <wtypes.h>



/*!
*	@defgroup Tasks
*	@brief タスク? =ゲームのシーンと考えてください。
*
*	CTaskBaseを継承したクラスは、メインループから呼ばれる更新・描画処理関数を持っています。
*	システムはこのクラスのリストを持っています。
*	タイトル・キャラセレ・試合 などのゲームの状態の変更は、
*	これらタスククラスの切り替えによって行われます。
*/

/*! 
*	@ingroup Tasks
*	@brief	基本タスク
*
*	・ExecuteでFALSEを返すと破棄される
*	・排他タスクが変更されたとき、破棄される
*/
class CTaskBase
{
public:
	virtual ~CTaskBase(){}
	virtual void Initialize(){}					//!< ExecuteまたはDrawがコールされる前に1度だけコールされる
	virtual BOOL Execute(DWORD time)
						{return(TRUE);}			//!< 毎フレームコールされる
	virtual void Terminate(){}					//!< タスクのリストから外されるときにコールされる（その直後、deleteされる）
	virtual void Draw(){}						//!< 描画時にコールされる
	virtual void WndMessage(					//!< メインウインドウのメッセージと同じモノを受け取ることができる
					HWND hWnd,
					UINT msg, 
					WPARAM wparam, 
					LPARAM lparam){}
	virtual DWORD GetID(){return 0;}			//!< 0以外を返すようにした場合、マネージャに同じIDを持つタスクがAddされたとき破棄される
	virtual int GetDrawPriority(){return -1;}	//!< 描画プライオリティ。低いほど手前に（後に）描画。マイナスならば表示しない

	static bool CompByDrawPriority(CTaskBase* arg1,CTaskBase* arg2)	//!< 描画プライオリティでソートするための比較演算
		{return arg1->GetDrawPriority() > arg2->GetDrawPriority() ;}
};


/*! 
*	@ingroup Tasks
*	@brief 排他タスク
*
*	・他の排他タスクと一緒には動作(Execute)しない
*	・他の排他タスクが追加された場合、Inactivateがコールされ、そこでFALSEを返すと
*		破棄される。TRUEを返すとExecute、WndMessageがコールされない状態になり、
*		新規の排他タスクが全て破棄されたときにActivateが呼ばれ、処理が再開する。
*/
class CExclusiveTaskBase : public CTaskBase
{
public:
	virtual ~CExclusiveTaskBase(){}
	virtual void Activate(DWORD prvTaskID){}				//!< Executeが再開されるときに呼ばれる
	virtual BOOL Inactivate(DWORD nextTaskID){return FALSE;}//!< 他の排他タスクが開始したときに呼ばれる
	
	virtual int GetDrawPriority(){return 0;}				//!< 描画プライオリティ取得メソッド
};



/*!
*	@ingroup Tasks
*	@brief 常駐タスク
*
*	・基本タスクと違い、排他タスクが変更されても破棄されない
*	・Enabledでないときには Execute , WndMessage をコールしない
*/
class CBackgroundTaskBase : public CTaskBase
{
public:
	virtual ~CBackgroundTaskBase(){}
	CBackgroundTaskBase(){m_isEnabled=TRUE;}

	BOOL IsEnabled(){return m_isEnabled;}
	void Enable(){m_isEnabled = TRUE;}
	void Disable(){m_isEnabled = FALSE;}

protected:
	BOOL m_isEnabled;
};




/*!
*	@ingroup System
*	@brief タスク管理クラス
*
*	CSystemの内部に保持される。CSystem以外からはこれにアクセスする必要はないはず
*	タスク継承クラスのリストを管理し、描画、更新、ウィンドウメッセージ等の配信を行う。
*
*	実行中に例外が起こったとき、どのクラスが例外を起こしたのかをログに吐き出す。
*	その際に実行時型情報からクラス名を取得しているので、コンパイルの際には
*	実行時型情報(RTTIと表記される場合もある)をONにすること。
*/

typedef std::list<CTaskBase*> TaskList;
typedef std::stack<CExclusiveTaskBase*> ExTaskStack;

class CTaskManager
{
public:
	CTaskManager();
	~CTaskManager(){Destroy();}

	void Destroy();

	void AddTask(CTaskBase *newTask);			//!< タスク追加
	void RemoveTaskByID(DWORD id);				//!< 指定IDを持つタスクの除去　※注：Exclusiveタスクはチェックしない
	void ReturnExclusiveTaskByID(DWORD id);		//!< 指定IDの排他タスクまでTerminate/popする
	CExclusiveTaskBase* GetTopExclusiveTask();	//!< 最上位にあるエクスクルーシブタスクをゲト
	CBackgroundTaskBase* FindBGTask(DWORD id);	//!< 指定IDをもつ常駐タスクゲット
	CTaskBase* FindTask(DWORD id);				//!< 指定IDをもつ通常タスクゲット

	void Execute(DWORD time);					//!< CSystemからコールされ、各タスクの同関数をコールする
	void Draw();								//!< CSystemからコールされ、各タスクをプライオリティ順に描画する
	void WndMessage(							//!< CSystemからコールされ、各タスクの同関数をコールする
			HWND hWnd,
			UINT msg,
			WPARAM wparam,
			LPARAM lparam);
	BOOL ExEmpty();								//!< 排他タスクが全部なくなっちゃったかどうか

	//デバッグ
	void DebugOutputTaskList();					//!< 現在リストに保持されているクラスのクラス名をデバッグ出力する

protected:
	void CleanupAllSubTasks();					//!< 通常タスクを全てTerminate , deleteする
	void SortTask(TaskList *ptgt);				//!< タスクを描画プライオリティ順に並べる

	void OutputLog(std::string s, ...)			//!< ログ出力
	{
		// Not Implemented
	}

private:
	TaskList tasks;								//!< 現在動作ちゅうのタスクリスト
	TaskList bg_tasks;							//!< 常駐タスクリスト
	ExTaskStack ex_stack;						//!< 排他タスクのスタック。topしか実行しない

	CExclusiveTaskBase* exNext;					//!< 現在フレームでAddされた排他タスク
};


