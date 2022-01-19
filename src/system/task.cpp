

/*============================================================================

    タスク管理(?)

==============================================================================*/

#include <cassert>
#include <algorithm>
#include <typeinfo>
#include "task.h"

namespace gtf
{
    using namespace std;

    TaskManager::TaskManager()
    {
        // ダミーデータ挿入
        const auto it = tasks.emplace(tasks.end(), make_shared<TaskBase>());
        ex_stack.emplace_back(exNext, it);
    }

    void TaskManager::Destroy()
    {
        //バックグラウンドタスクTerminate
        for(auto&& ib : bg_tasks) ib->Terminate();
        bg_tasks.clear();

        //排他タスク・通常タスクTerminate
        while (ex_stack.size() != 0 && ex_stack.back().value){
            CleanupPartialSubTasks(ex_stack.back().SubTaskStartPos);
            ex_stack.back().value->Terminate();
            ex_stack.pop_back();
        }
        exNext = nullptr;
    }

    TaskManager::TaskPtr TaskManager::AddTaskGuaranteed(TaskBase *newTask)
    {
        assert(newTask);
        assert(dynamic_cast<ExclusiveTaskBase*>(newTask) == nullptr);
        assert(dynamic_cast<BackgroundTaskBase*>(newTask) == nullptr);

        if (newTask->GetID() != 0){
            RemoveTaskByID(newTask->GetID());
        }

        //通常タスクとしてAdd
        tasks.emplace_back(newTask);
        auto pnew = tasks.back();
        newTask->Initialize();
        if (newTask->GetID() != 0)
            indices[newTask->GetID()] = pnew;
        if (pnew->GetDrawPriority() >= 0)
            ex_stack.back().drawList.emplace(pnew->GetDrawPriority(), pnew);
        return pnew;
    }

    TaskManager::ExTaskPtr TaskManager::AddTask(ExclusiveTaskBase *newTask)
    {
        //排他タスクとしてAdd
        //Execute中かもしれないので、ポインタ保存のみ
        if (exNext){
            auto t1 = *exNext;
            auto t2 = *newTask;
            OutputLog("■ALERT■ 排他タスクが2つ以上Addされた : %s / %s",
                typeid(t1).name(), typeid(t2).name());
        }
        exNext = shared_ptr<ExclusiveTaskBase>(newTask);

        return exNext;
    }

    TaskManager::BgTaskPtr TaskManager::AddTask(BackgroundTaskBase *newTask)
    {
        if (newTask->GetID() != 0){
            RemoveTaskByID(newTask->GetID());
        }

        bg_tasks.emplace_back(newTask);

        auto pbgt = bg_tasks.back();

        //常駐タスクとしてAdd
        pbgt->Initialize();
        if (newTask->GetID() != 0)
            bg_indices[newTask->GetID()] = pbgt;
        if (pbgt->GetDrawPriority() >= 0)
            drawListBG.emplace(pbgt->GetDrawPriority(), pbgt);
        return pbgt;
    }

    void TaskManager::Execute(double elapsedTime)
    {
#ifdef ARRAYBOUNDARY_DEBUG
        if(!AfxCheckMemory()){
            OutputLog("AfxCheckMemory() failed");
            return;
        }
#endif

        //排他タスク、topのみExecute
        assert(ex_stack.size() != 0);
        shared_ptr<ExclusiveTaskBase> exTsk = ex_stack.back().value;

        // 新しいタスクがある場合
        if (exNext){
            //現在排他タスクのInactivate
            assert(ex_stack.size() != 0);
            if (exTsk && !exTsk->Inactivate(exNext->GetID())){
                //通常タスクを全て破棄する
                CleanupPartialSubTasks(ex_stack.back().SubTaskStartPos);

                exTsk->Terminate();
                ex_stack.pop_back();
            }

            //AddされたタスクをInitializeして突っ込む
            const auto it = tasks.emplace(tasks.end(), make_shared<TaskBase>());				// ダミータスク挿入
            ex_stack.emplace_back(move(exNext), it);
            auto pnew = ex_stack.back().value;
            if (pnew->IsFallthroughDraw()) {
                assert(ex_stack.size() >= 2);
                ex_stack.back().drawList = (ex_stack.rbegin() + 1)->drawList;					// 一つ下の階層のdrawListをコピー
            }
            pnew->Initialize();
            if (pnew->GetDrawPriority() >= 0)
                ex_stack.back().drawList.emplace(pnew->GetDrawPriority(), pnew);

            exNext = nullptr;
            exTsk = move(pnew);
        }

        if (exTsk)
        {
            bool ex_ret = true;
#ifdef _CATCH_WHILE_EXEC
            try{
#endif
                ex_ret = exTsk->Execute(elapsedTime);
#ifdef _CATCH_WHILE_EXEC
            }catch(...){
                if (ex_stack.back() == NULL)OutputLog("catch while execute3 : NULL", SYSLOG_ERROR);
                else OutputLog("catch while execute3 : %X %s",ex_stack.back(),typeid(*ex_stack.back()).name());
            }
#endif

            if (!ex_ret)
            {
                if (!exNext){
                    //現在排他タスクの変更

#ifdef _CATCH_WHILE_EXEC
                    try{
#endif

                        //通常タスクを全て破棄する
                        CleanupPartialSubTasks(ex_stack.back().SubTaskStartPos);

#ifdef _CATCH_WHILE_EXEC
                    }catch(...){
                        if ((*i) == NULL)OutputLog("catch while terminate1 : NULL", SYSLOG_ERROR);
                        else OutputLog("catch while terminate1 : %X %s", (*i), typeid(*(*i)).name());
                    }
#endif

#ifdef _CATCH_WHILE_EXEC
                    try{
#endif

                        //現在排他タスクの破棄
                        unsigned int prvID = exTsk->GetID();
                        exTsk->Terminate();
                        exTsk = nullptr;
                        ex_stack.pop_back();

#ifdef _CATCH_WHILE_EXEC
                    }catch(...){
                        if (exTsk == NULL)OutputLog("catch while terminate2 : NULL", SYSLOG_ERROR);
                        else OutputLog("catch while terminate : %X %s", exTsk, typeid(*exTsk).name());
                    }
#endif


#ifdef _CATCH_WHILE_EXEC
                    try{
#endif

                        //次の排他タスクをActivateする
                        assert(ex_stack.size() != 0);
                        exTsk = ex_stack.back().value;
                        if (exTsk)
                            exTsk->Activate(prvID);

#ifdef _CATCH_WHILE_EXEC
                    }catch(...){
                        if (exTsk == NULL)OutputLog("catch while activate : NULL", SYSLOG_ERROR);
                        else OutputLog("catch while activate : %X %s", exTsk, typeid(*exTsk).name());
                    }
#endif


                    return;
                }
            }
        }

        //通常タスクExecute
        assert(!ex_stack.empty());
        taskExecute(tasks, ex_stack.back().SubTaskStartPos, tasks.end(), elapsedTime);

        //常駐タスクExecute
        taskExecute(bg_tasks, bg_tasks.begin(), bg_tasks.end(), elapsedTime);
    }


    void TaskManager::Draw()
    {
        assert(ex_stack.size() != 0);

        //Drawリストを取得
        auto iv = ex_stack.back().drawList.begin();
        const auto iedv = ex_stack.back().drawList.end();
        auto ivBG = drawListBG.begin();
        const auto iedvBG = drawListBG.end();
        auto DrawAndProceed = [](DrawPriorityMap::iterator& iv, DrawPriorityMap& drawList)
        {
                    auto is = iv->second.lock();

                    if (is)
                    {
                        is->Draw();
                        ++iv;
                    }
                    else
                        drawList.erase(iv++);
        };

        //描画
        while (iv != iedv)
        {
#ifdef _CATCH_WHILE_RENDER
            try{
#endif
                while (ivBG != iedvBG && ivBG->first <= iv->first)
                    DrawAndProceed(ivBG, drawListBG);
                DrawAndProceed(iv, ex_stack.back().drawList);
#ifdef _CATCH_WHILE_RENDER
            }catch(...){
                OutputLog("catch while draw : %X %s", *iv, typeid(*(*iv).lock()).name());
            }
#endif
        }

        // 書き残した常駐タスク処理
        while (ivBG != iedvBG)
            DrawAndProceed(ivBG, drawListBG);
    }

    void TaskManager::RemoveTaskByID(unsigned int id)
    {
        // ID比較用関数
        const auto is_equal_to_id = [id](const auto& i){
            return id == i->GetID();
        };

        //通常タスクをチェック
        if (indices.count(id) != 0)
        {
            // 該当するIDのタスクがあったら削除
            const auto it = std::find_if(
                    tasks.begin(),
                    tasks.end(), 
                    is_equal_to_id);

            if (it != tasks.end()) {
                (*it)->Terminate();
                tasks.erase(it);
            }
        }

        //バックグラウンドタスクTerminate
        if (bg_indices.count(id) != 0)
        {
            // 該当するIDのタスクがあったら削除
            const auto it = std::find_if(
                    bg_tasks.begin(),
                    bg_tasks.end(), 
                    is_equal_to_id);

            if (it != bg_tasks.end()) {
                (*it)->Terminate();
                bg_tasks.erase(it);
            }
        }
    }


    //指定IDの排他タスクまでTerminate/popする
    void TaskManager::RevertExclusiveTaskByID(unsigned int id)
    {
        bool act = false;
        unsigned int previd = 0;

        assert(ex_stack.size() != 0);
        while (ex_stack.back().value){
            const shared_ptr<ExclusiveTaskBase>& task = ex_stack.back().value;
            if (task->GetID() == id){
                if (act){
                    task->Activate(previd);
                }
                return;
            }
            else{
                previd = task->GetID();
                act = true;
                CleanupPartialSubTasks(ex_stack.back().SubTaskStartPos);
                task->Terminate();
                ex_stack.pop_back();
                assert(ex_stack.size() != 0);
            }
        }
    }

    //通常タスクを一部だけ破棄する
    void TaskManager::CleanupPartialSubTasks(TaskList::iterator it_task)
    {
        for (TaskList::iterator i = it_task; i != tasks.end(); ++i){
            (*i)->Terminate();
        }
        tasks.erase(it_task, tasks.end());
    }


    //デバッグ・タスク一覧表示
    void TaskManager::DebugOutputTaskList()
    {
        OutputLog("\n\n■TaskManager::DebugOutputTaskList() - start");

        OutputLog("□通常タスク一覧□");
        //通常タスク
        for(auto&& i : tasks) OutputLog(typeid(i).name());

        OutputLog("□常駐タスク一覧□");
        //バックグラウンドタスク
        for(auto&& ib : bg_tasks) OutputLog(typeid(ib).name());

        //排他タスク
        OutputLog("\n");
        OutputLog("□現在のタスク：");
        if (ex_stack.empty())
            OutputLog("なし");
        else {
            auto s_top_v = *ex_stack.back().value;
            OutputLog(typeid(s_top_v).name());
        }

        OutputLog("\n\n■TaskManager::DebugOutputTaskList() - end\n\n");
    }
}
