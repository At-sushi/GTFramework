/*!
*	@file
*	@brief タスク(?)管理・定義
*/
#pragma once
#include <vector>
#include <string>
#include <map>
#include <list>
#include <unordered_map>
#include <stack>
#include <memory>
#include <functional>
#include <type_traits>



/*!
*	@defgroup Tasks
*	@brief タスク
*
*	CTaskBaseを継承したクラスは、メインループから呼ばれる更新・描画処理関数を持っています。
*	システムはこのクラスのリストを持っています。
*	タイトル・キャラセレ・試合 などのゲームの状態の変更は、
*	これらタスククラスの切り替えによって行われます。
*/

namespace GTF
{
    using namespace std;

    /*! 
    *	@ingroup Tasks
    *	@brief	基本タスク
    *
    *	・Executeでfalseを返すと破棄される
    *	・親の排他タスクが変更されたとき、破棄される
    */
    class CTaskBase
    {
    public:
        virtual ~CTaskBase(){}
        virtual void Initialize(){}							//!< ExecuteまたはDrawがコールされる前に1度だけコールされる
        virtual bool Execute(double elapsedTime)
                            {return(true);}					//!< 毎フレームコールされる
        virtual void Terminate(){}							//!< タスクのリストから外されるときにコールされる（その直後、deleteされる）
        virtual void Draw(){}								//!< 描画時にコールされる
        virtual unsigned int GetID() const { return 0; }	//!< 0以外を返すようにした場合、マネージャに同じIDを持つタスクがAddされたとき破棄される
        virtual int GetDrawPriority() const { return -1; }	//!< 描画プライオリティ。低いほど手前に（後順に）描画。マイナスならば表示しない
    };


    /*! 
    *	@ingroup Tasks
    *	@brief 排他タスク? =ゲームのシーンと考えてください。
    *
    *	・他の排他タスクと一緒には動作(Execute)しない
    *	・他の排他タスクが追加された場合、Inactivateがコールされ、そこでfalseを返すと
    *		破棄される。trueを返すとExecute、WndMessageがコールされない状態になり、
    *		新規の排他タスクが全て破棄されたときにActivateが呼ばれ、処理が再開する。
    *	・通常タスクとの親子関係を持つ。
    *	・AddTask実行後、一度Executeが実行されるまで追加が保留される。その後に追加された通常タスクは子タスクとなる。
    */
    class CExclusiveTaskBase : public CTaskBase
    {
    public:
        virtual ~CExclusiveTaskBase(){}
        virtual void Activate(unsigned int prvTaskID){}				//!< Executeが再開されるときに呼ばれる
        virtual bool Inactivate(unsigned int nextTaskID){return true;}//!< 他の排他タスクが開始したときに呼ばれる
    
        virtual int GetDrawPriority() const override {return 0;}				//!< 描画プライオリティ取得メソッド
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
        CBackgroundTaskBase(){m_isEnabled=true;}

        bool IsEnabled() const {return m_isEnabled;}
        void Enable(){m_isEnabled = true;}
        void Disable(){m_isEnabled = false;}

    protected:
        bool m_isEnabled;
    };




    /*!
    *	@ingroup System
    *	@brief タスク管理クラス
    *
    *	タスク継承クラスのリストを管理し、描画、更新、ウィンドウメッセージ等の配信を行う。
    *
    *	実行中に例外が起こったとき、どのクラスが例外を起こしたのかをログに吐き出す。
    *	その際に実行時型情報からクラス名を取得しているので、コンパイルの際には
    *	実行時型情報(RTTIと表記される場合もある)をONにすること。
    */

    class CTaskManager
    {
    public:
        CTaskManager();
        ~CTaskManager(){Destroy();}

        using TaskPtr = weak_ptr<CTaskBase>;
        using ExTaskPtr = weak_ptr<CExclusiveTaskBase>;
        using BgTaskPtr = weak_ptr<CBackgroundTaskBase>;

        void Destroy();
            
        //! 追加したタスクはCTaskManager内部で自動的に破棄されるので、呼び出し側でdeleteしないこと。
        TaskPtr AddTask(CTaskBase *newTask);		        //!< タスク追加
        ExTaskPtr AddTask(CExclusiveTaskBase *newTask);     //!< 排他タスク追加
        BgTaskPtr AddTask(CBackgroundTaskBase *newTask);    //!< 常駐タスク追加
        void RemoveTaskByID(unsigned int id);				//!< 指定IDを持つタスクの除去　※注：Exclusiveタスクはチェックしない
        void RevertExclusiveTaskByID(unsigned int id);		//!< 指定IDの排他タスクまでTerminate/popする

        //! 最上位にあるエクスクルーシブタスクをゲト
        ExTaskPtr GetTopExclusiveTask() const
        {
            return ex_stack.top().value;
        }

        //!指定IDの通常タスク取得
        TaskPtr FindTask(unsigned int id) const
        {
            return indices.at(id);
        }

        //!指定IDの常駐タスク取得
        BgTaskPtr FindBGTask(unsigned int id) const
        {
           return bg_indices.at(id);
        }

        //! 任意のクラス型のタスクを取得（通常・常駐兼用）
        template<class T> shared_ptr<T> FindTask(unsigned int id) const
        {
            return dynamic_pointer_cast<T>(FindTask_impl<T>(id).lock());
        }

        void Execute(double elapsedTime);					//!< 各タスクのExecute関数をコールする
        void Draw();										//!< 各タスクをプライオリティ順に描画する

        //!< 排他タスクが全部なくなっちゃったかどうか
        bool ExEmpty() const    {
            return ex_stack.empty();
        }

        //デバッグ
        void DebugOutputTaskList();							//!< 現在リストに保持されているクラスのクラス名をデバッグ出力する

    protected:
        using TaskList = list<shared_ptr<CTaskBase>>;
        using BgTaskList = list<shared_ptr<CBackgroundTaskBase>>;
        using DrawPriorityMap = multimap<int, TaskPtr, greater<int>>;

        struct ExTaskInfo {
            const shared_ptr<CExclusiveTaskBase> value;		//!< 排他タスクのポインタ
            const TaskList::iterator SubTaskStartPos;		//!< 依存する通常タスクの開始地点
            DrawPriorityMap drawList;						//!< 描画順ソート用コンテナ

            ExTaskInfo(shared_ptr<CExclusiveTaskBase>& source, TaskList::iterator startPos)
                : value(source), SubTaskStartPos(startPos)
            {
            }
        };
        using ExTaskStack = stack<ExTaskInfo>;

        //! 通常タスクを全てTerminate , deleteする
        void CleanupAllSubTasks()    {
            CleanupPartialSubTasks(tasks.begin());
        }

        void CleanupPartialSubTasks(TaskList::iterator it_task);	//!< 一部の通常タスクをTerminate , deleteする

        //! ログ出力
        void OutputLog(string s, ...)
        {
            // Not Implemented
        }

    private:
        template<class T, class = typename enable_if_t<!is_base_of<CBackgroundTaskBase, T>::value>>
            TaskPtr FindTask_impl(unsigned int id) const
        {
            return FindTask(id);
        }
        template<class T, class = typename enable_if_t<is_base_of<CBackgroundTaskBase, T>::value>>
            BgTaskPtr FindTask_impl(unsigned int id) const
        {
            return FindBGTask(id);
        }

        //! タスクExecute
        template<class T, typename I = T::iterator, class QI = deque<I>, typename I_QI = QI::iterator>
            void taskExecute(T& tasks, I i, I ied, double elapsedTime)
        {
            QI deleteList;

            for (; i != ied; ++i){
#ifdef _CATCH_WHILE_EXEC
                try{
#endif
                    if ((*i)->Execute(elapsedTime) == false)
                    {
                        deleteList.push_back(i);
                    }
#ifdef _CATCH_WHILE_EXEC
                }
                catch (...){
                    if (*i == nullptr)OutputLog("catch while execute1 : NULL", SYSLOG_ERROR);
                    else OutputLog("catch while execute1 : %X , %s", *i, typeid(**i).name());
                    break;
                }
#endif
            }

            //タスクでfalseを返したものを消す
            I_QI idl = deleteList.begin();
            const I_QI idl_ed = deleteList.end();

            for (; idl != idl_ed; ++idl){
                i = *idl;
                (*i)->Terminate();
                tasks.erase(i);
            }
        }

        TaskList tasks;								//!< 現在動作ちゅうのタスクリスト
        BgTaskList bg_tasks;						//!< 常駐タスクリスト
        ExTaskStack ex_stack;						//!< 排他タスクのスタック。topしか実行しない

        shared_ptr<CExclusiveTaskBase> exNext;		//!< 現在フレームでAddされた排他タスク
        DrawPriorityMap drawListBG;					//!< 描画順ソート用コンテナ（常駐タスク）
        unordered_map<unsigned int, TaskPtr> indices;
        unordered_map<unsigned int, BgTaskPtr> bg_indices;
    };


}
