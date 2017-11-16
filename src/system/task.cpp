

/*============================================================================

    タスク管理(?)

==============================================================================*/

#include <cassert>
#include <algorithm>
#include <typeinfo>
#include "task.h"

namespace GTF
{
    using namespace std;

    CTaskManager::CTaskManager()
    {
        // ダミーデータ挿入
        const auto it = tasks.emplace(tasks.end(), make_shared<CTaskBase>());
        ex_stack.emplace(exNext, it);
    }

    void CTaskManager::Destroy()
    {
        //通常タスクTerminate
        auto i = tasks.begin();
        const auto ied = tasks.end();
        for (; i != ied; ++i){
            (*i)->Terminate();
        }
        tasks.clear();

        //バックグラウンドタスクTerminate
        auto ib = bg_tasks.begin();
        const auto ibed = bg_tasks.end();
        for (; ib != ibed; ++ib){
            (*ib)->Terminate();
        }
        bg_tasks.clear();

        //排他タスクTerminate
        while (ex_stack.size() != 0 && ex_stack.top().value){
            ex_stack.top().value->Terminate();
            ex_stack.pop();
        }
    }


    CTaskManager::TaskPtr CTaskManager::AddTask(CTaskBase *newTask)
    {
        assert(newTask);

        CExclusiveTaskBase *pext = dynamic_cast<CExclusiveTaskBase*>(newTask);
        if (pext){
            //排他タスクとしてAdd
            return AddTask(pext);
        }

        CBackgroundTaskBase *pbgt = dynamic_cast<CBackgroundTaskBase*>(newTask);
        if (pbgt){
            //常駐タスクとしてAdd
            return AddTask(pbgt);
        }

        //通常タスクとしてAdd
        return AddTaskGuaranteed(newTask);
    }

    CTaskManager::TaskPtr CTaskManager::AddTaskGuaranteed(CTaskBase *newTask)
    {
        assert(newTask);
        assert(dynamic_cast<CExclusiveTaskBase*>(newTask) == nullptr);
        assert(dynamic_cast<CBackgroundTaskBase*>(newTask) == nullptr);

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
            ex_stack.top().drawList.emplace(pnew->GetDrawPriority(), pnew);
        return pnew;
    }

    CTaskManager::ExTaskPtr CTaskManager::AddTask(CExclusiveTaskBase *newTask)
    {
        //排他タスクとしてAdd
        //Execute中かもしれないので、ポインタ保存のみ
        if (exNext){
            OutputLog("■ALERT■ 排他タスクが2つ以上Addされた : %s / %s",
                typeid(*exNext).name(), typeid(*newTask).name());
        }
        exNext = shared_ptr<CExclusiveTaskBase>(newTask);

        return exNext;
    }

    CTaskManager::BgTaskPtr CTaskManager::AddTask(CBackgroundTaskBase *newTask)
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

    void CTaskManager::Execute(double elapsedTime)
    {
#ifdef ARRAYBOUNDARY_DEBUG
        if(!AfxCheckMemory()){
            OutputLog("AfxCheckMemory() failed");
            return;
        }
#endif

        //排他タスク、topのみExecute
        assert(ex_stack.size() != 0);
        shared_ptr<CExclusiveTaskBase> exTsk = ex_stack.top().value;

        if (exTsk)
        {
            bool ex_ret = true;
#ifdef _CATCH_WHILE_EXEC
            try{
#endif
                ex_ret = exTsk->Execute(elapsedTime);
#ifdef _CATCH_WHILE_EXEC
            }catch(...){
                if (ex_stack.top() == NULL)OutputLog("catch while execute3 : NULL", SYSLOG_ERROR);
                else OutputLog("catch while execute3 : %X %s",ex_stack.top(),typeid(*ex_stack.top()).name());
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
                        CleanupPartialSubTasks(ex_stack.top().SubTaskStartPos);

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
                        ex_stack.pop();

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
                        exTsk = ex_stack.top().value;
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
        taskExecute(tasks, ex_stack.top().SubTaskStartPos, tasks.end(), elapsedTime);

        //常駐タスクExecute
        taskExecute(bg_tasks, bg_tasks.begin(), bg_tasks.end(), elapsedTime);

        // 新しいタスクがある場合
        if (exNext){
            //現在排他タスクのInactivate
            assert(ex_stack.size() != 0);
            auto& exTsk = ex_stack.top().value;
            if (exTsk && !exTsk->Inactivate(exNext->GetID())){
                //通常タスクを全て破棄する
                CleanupPartialSubTasks(ex_stack.top().SubTaskStartPos);

                exTsk->Terminate();
                ex_stack.pop();
            }

            //AddされたタスクをInitializeして突っ込む
            const auto it = tasks.emplace(tasks.end(), make_shared<CTaskBase>());				// ダミータスク挿入
            ex_stack.emplace(move(exNext), it);
            ex_stack.top().value->Initialize();

            exNext = nullptr;
        }
    }


    void CTaskManager::Draw()
    {
         shared_ptr<CExclusiveTaskBase> pex = nullptr;

        //排他タスクを取得
        assert(ex_stack.size() != 0);
        if (ex_stack.top().value && ex_stack.top().value->GetDrawPriority() >= 0){
            pex = ex_stack.top().value;
        }

        auto iv = ex_stack.top().drawList.begin();
        auto iedv = pex ? ex_stack.top().drawList.upper_bound(pex->GetDrawPriority()) : ex_stack.top().drawList.end();
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
        auto DrawAll = [&]()		// 描画関数
        {
            while (iv != iedv)
            {
#ifdef _CATCH_WHILE_RENDER
                try{
#endif
                    while (ivBG != iedvBG && ivBG->first <= iv->first)
                        DrawAndProceed(ivBG, drawListBG);
                    DrawAndProceed(iv, ex_stack.top().drawList);
#ifdef _CATCH_WHILE_RENDER
                }catch(...){
                    OutputLog("catch while draw : %X %s", *iv, typeid(*(*iv).lock()).name());
                }
#endif
            }
        };
        //描画
        DrawAll();

        // 排他タスクDraw
        if (pex)
            pex->Draw();

        //描画
        assert(iv == iedv);
        iedv = ex_stack.top().drawList.end();
        DrawAll();

        // 書き残した常駐タスク処理
        while (ivBG != iedvBG)
            DrawAndProceed(ivBG, drawListBG);
    }

    void CTaskManager::RemoveTaskByID(unsigned int id)
    {
        //通常タスクをチェック
        if (indices.count(id) != 0)
        {
            auto i = tasks.begin();
            const auto ied = tasks.end();
            for (; i != ied; ++i){
                if (id == (*i)->GetID()){
                    (*i)->Terminate();
                    tasks.erase(i);
                    return;
                }
            }
        }

        //バックグラウンドタスクTerminate
        if (bg_indices.count(id) != 0)
        {
            auto i = bg_tasks.begin();
            const auto ied = bg_tasks.end();
            for (; i != ied; ++i){
                if (id == (*i)->GetID()){
                    (*i)->Terminate();
                    bg_tasks.erase(i);
                    return;
                }
            }
        }
    }


    //指定IDの排他タスクまでTerminate/popする
    void CTaskManager::RevertExclusiveTaskByID(unsigned int id)
    {
        bool act = false;
        unsigned int previd = 0;

        assert(ex_stack.size() != 0);
        while (ex_stack.top().value){
            const shared_ptr<CExclusiveTaskBase>& task = ex_stack.top().value;
            if (task->GetID() == id){
                if (act){
                    task->Activate(previd);
                }
                return;
            }
            else{
                previd = task->GetID();
                act = true;
                CleanupPartialSubTasks(ex_stack.top().SubTaskStartPos);
                task->Terminate();
                ex_stack.pop();
                assert(ex_stack.size() != 0);
            }
        }
    }

    //通常タスクを一部だけ破棄する
    void CTaskManager::CleanupPartialSubTasks(TaskList::iterator it_task)
    {
        TaskList::iterator i = it_task, ied = tasks.end();

        for (; i != ied; ++i){
            shared_ptr<CTaskBase>& delTgt = (*i);
            delTgt->Terminate();
        }
        tasks.erase(it_task, ied);
    }


    //デバッグ・タスク一覧表示
    void CTaskManager::DebugOutputTaskList()
    {
        OutputLog("\n\n■CTaskManager::DebugOutputTaskList() - start");

        OutputLog("□通常タスク一覧□");
        //通常タスク
        auto i = tasks.begin();
        const auto ied = tasks.end();
        for (; i != ied; ++i){
            OutputLog(typeid(**i).name());
        }

        OutputLog("□常駐タスク一覧□");
        //バックグラウンドタスク
        auto ib = bg_tasks.begin();
        const auto ibed = bg_tasks.end();
        for (; ib != ibed; ++ib){
            OutputLog(typeid(**ib).name());
        }

        //排他タスク
        OutputLog("\n");
        OutputLog("□現在のタスク：");
        if (ex_stack.empty())
            OutputLog("なし");
        else
            OutputLog(typeid(*ex_stack.top().value).name());


        OutputLog("\n\n■CTaskManager::DebugOutputTaskList() - end\n\n");
    }
}
