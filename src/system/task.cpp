

/*============================================================================

    タスク管理(?)

==============================================================================*/

#include <cassert>
#include <algorithm>
#include <typeinfo>
#include "task.h"

namespace GTF
{

    CTaskManager::CTaskManager()
    {
        exNext = nullptr;
    }

    void CTaskManager::Destroy()
    {
        TaskList::iterator i, ied;

        //通常タスクTerminate
        i = tasks.begin();
        ied = tasks.end();
        for (; i != ied; i++){
            (*i)->Terminate();
        }
        tasks.clear();

        //バックグラウンドタスクTerminate
        i = bg_tasks.begin();
        ied = bg_tasks.end();
        for (; i != ied; i++){
            (*i)->Terminate();
        }
        bg_tasks.clear();

        //排他タスクTerminate
        while (ex_stack.size() != 0){
            ex_stack.top().value->Terminate();
            ex_stack.pop();
        }
    }


    CTaskManager::TaskPtr CTaskManager::AddTask(CTaskBase *newTask)
    {
        if (newTask->GetID() != 0){
            RemoveTaskByID(newTask->GetID());
        }

        CBackgroundTaskBase *pbgt = dynamic_cast<CBackgroundTaskBase*>(newTask);
        if (pbgt){
            //常駐タスクとしてAdd
            return AddTask(pbgt);
        }

        CExclusiveTaskBase *pext = dynamic_cast<CExclusiveTaskBase*>(newTask);
        if (pext){
            //排他タスクとしてAdd
            return AddTask(pext);
        }

        //通常タスクとしてAdd
        tasks.emplace_back(newTask);
        auto& pnew = tasks.back();
        newTask->Initialize();
        if (newTask->GetID() != 0)
            indices[newTask->GetID()] = pnew;
        return pnew;
    }

    CTaskManager::ExTaskPtr CTaskManager::AddTask(CExclusiveTaskBase *newTask)
    {
        if (newTask->GetID() != 0){
            RemoveTaskByID(newTask->GetID());
        }

        //排他タスクとしてAdd
        //Execute中かもしれないので、ポインタ保存のみ
        if (exNext){
            OutputLog("■ALERT■ 排他タスクが2つ以上Addされた : %s / %s",
                typeid(*exNext).name(), typeid(*newTask).name());
        }
        exNext = std::shared_ptr<CExclusiveTaskBase>(newTask);

        return exNext;
    }

    CTaskManager::BgTaskPtr CTaskManager::AddTask(CBackgroundTaskBase *newTask)
    {
        if (newTask->GetID() != 0){
            RemoveTaskByID(newTask->GetID());
        }

        bg_tasks.emplace_back(newTask);
        // 暫定的な型変換
        auto& pbgt = std::static_pointer_cast<CBackgroundTaskBase>(bg_tasks.back());
        assert(std::dynamic_pointer_cast<CBackgroundTaskBase>(pbgt).get());

        //常駐タスクとしてAdd
        pbgt->Initialize();
        if (newTask->GetID() != 0)
            bg_indices[newTask->GetID()] = pbgt;
        return pbgt;
    }

    void CTaskManager::Execute(unsigned int time)
    {
        TaskList::iterator i, ied;
        std::list<TaskList::iterator> deleteList;
        std::list<TaskList::iterator>::iterator idl, idl_ed;

#ifdef ARRAYBOUNDARY_DEBUG
        if(!AfxCheckMemory()){
            OutputLog("AfxCheckMemory() failed");
            return;
        }
#endif

        //排他タスク、topのみExecute
        if (ex_stack.size() != 0){
            std::shared_ptr<CExclusiveTaskBase> exTsk = ex_stack.top().value;
            bool ex_ret = true;
#ifdef _CATCH_WHILE_EXEC
            try{
#endif
                ex_ret = exTsk->Execute(time);
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

                    unsigned int prvID;

#ifdef _CATCH_WHILE_EXEC
                    try{
#endif

                        //現在排他タスクの破棄
                        prvID = exTsk->GetID();
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
                        if (ex_stack.size() == 0)return;
                        exTsk = ex_stack.top().value;
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
        i = ex_stack.empty() ? tasks.begin() : ex_stack.top().SubTaskStartPos;
        ied = tasks.end();
        for (; i != ied; i++){
#ifdef _CATCH_WHILE_EXEC
            try{
#endif
                if ((*i)->Execute(time) == false)
                {
                    deleteList.push_back(i);
                }
#ifdef _CATCH_WHILE_EXEC
            }catch(...){
                if(*i==NULL)OutputLog("catch while execute1 : NULL",SYSLOG_ERROR);
                else OutputLog("catch while execute1 : %X , %s",*i,typeid(**i).name());
                break;
            }
#endif
        }
        //通常タスクでfalseを返したものを消す
        if (deleteList.size() != 0){
            idl = deleteList.begin();
            idl_ed = deleteList.end();
            for (; idl != idl_ed; idl++){
                i = *idl;
                (*i)->Terminate();
                tasks.erase(i);
            }
            deleteList.clear();
        }

        //常駐タスクExecute
        i = bg_tasks.begin();
        ied = bg_tasks.end();
        for (; i != ied; i++)
        {
#ifdef _CATCH_WHILE_EXEC
            try{
#endif
                if ((*i)->Execute(time) == false){
                    deleteList.push_back(i);
                }
#ifdef _CATCH_WHILE_EXEC
            }catch(...){
                if(*i==NULL)OutputLog("catch while execute2 : NULL",SYSLOG_ERROR);
                else OutputLog("catch while execute2 : %X %s",*i,typeid(**i).name());
            }
#endif
        }
        //常駐タスクでfalseを返したものを消す
        if (deleteList.size() != 0){
            idl = deleteList.begin();
            idl_ed = deleteList.end();
            for (; idl != idl_ed; idl++){
                i = *idl;
                (*i)->Terminate();
                bg_tasks.erase(i);
            }
            deleteList.clear();
        }

        // 新しいタスクがある場合
        if (exNext){
            //現在排他タスクのInactivate
            if (ex_stack.size() != 0){
                auto& exTsk = ex_stack.top().value;
                if (!exTsk->Inactivate(exNext->GetID())){
                    //通常タスクを全て破棄する
                    CleanupPartialSubTasks(ex_stack.top().SubTaskStartPos);

                    exTsk->Terminate();
                    ex_stack.pop();
                }
            }

            //AddされたタスクをInitializeして突っ込む
            i = tasks.emplace(tasks.end(), new CTaskBase());				// ダミータスク挿入
            ex_stack.emplace(std::move(exNext), i);
            ex_stack.top().value->Initialize();

            exNext = nullptr;
        }
    }


    void CTaskManager::Draw()
    {
        TaskList::iterator i, ied;

        assert(tmplist.empty());
        tmplist.reserve(tasks.size());

        //通常タスクDraw
        i = tasks.begin();
        ied = tasks.end();
        for (; i != ied; i++){
            if ((*i)->GetDrawPriority() >= 0){
                tmplist.push_back(*i);
            }
        }

        //バックグラウンドタスクDraw
        i = bg_tasks.begin();
        ied = bg_tasks.end();
        for (; i != ied; i++){
            if ((*i)->GetDrawPriority() >= 0){
                tmplist.push_back(*i);
            }
        }

        //排他タスクDraw
        if (ex_stack.size() != 0){
            if (ex_stack.top().value->GetDrawPriority() >= 0){
                tmplist.push_back(ex_stack.top().value);
            }
        }

        std::sort(tmplist.begin(), tmplist.end(), CompByDrawPriority);//描画プライオリティ順にソート

        //描画
        auto iv = tmplist.begin();
        auto iedv = tmplist.end();
        for (; iv != iedv; iv++)
        {
#ifdef _CATCH_WHILE_RENDER
            try{
#endif
                (*iv)->Draw();
#ifdef _CATCH_WHILE_RENDER
            }catch(...){
                OutputLog("catch while draw : %X %s", *iv, typeid(**iv).name());
            }
#endif
        }

        // 一時リスト破棄
        tmplist.clear();
    }

    void CTaskManager::RemoveTaskByID(unsigned int id)
    {
        TaskList::iterator i, ied;

        //通常タスクをチェック
        if (indices.find(id) != indices.end())
        {
            i = tasks.begin();
            ied = tasks.end();
            for (; i != ied; i++){
                if (id == (*i)->GetID()){
                    (*i)->Terminate();
                    tasks.erase(i);
                    return;
                }
            }
        }

        //バックグラウンドタスクTerminate
        if (bg_indices.find(id) != bg_indices.end())
        {
            i = bg_tasks.begin();
            ied = bg_tasks.end();
            for (; i != ied; i++){
                if (id == (*i)->GetID()){
                    (*i)->Terminate();
                    bg_tasks.erase(i);
                    return;
                }
            }
        }
    }


    //最上位にあるエクスクルーシブタスクをゲト
    CTaskManager::ExTaskPtr CTaskManager::GetTopExclusiveTask()
    {
        if (ex_stack.size() == 0)return ExTaskPtr();
        return ex_stack.top().value;
    }

    //指定IDの排他タスクまでTerminate/popする
    void CTaskManager::ReturnExclusiveTaskByID(unsigned int id)
    {
        bool act = false;
        unsigned int previd = 0;

        while (ex_stack.size() != 0){
            const std::shared_ptr<CExclusiveTaskBase>& task = ex_stack.top().value;
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
            }
        }
    }

    //通常タスクを全て破棄する
    void CTaskManager::CleanupAllSubTasks()
    {
        CleanupPartialSubTasks(tasks.begin());
    }

    //通常タスクを一部だけ破棄する
    void CTaskManager::CleanupPartialSubTasks(TaskList::iterator it_task)
    {
        TaskList::iterator i, ied;

        i = it_task;
        ied = tasks.end();
        for (; i != ied; i++){
            std::shared_ptr<CTaskBase>& delTgt = (*i);
            delTgt->Terminate();
        }
        tasks.erase(it_task, ied);
    }


    //デバッグ・タスク一覧表示
    void CTaskManager::DebugOutputTaskList()
    {
        OutputLog("\n\n■CTaskManager::DebugOutputTaskList() - start");

        TaskList::iterator i, ied;

        OutputLog("□通常タスク一覧□");
        //通常タスク
        i = tasks.begin();
        ied = tasks.end();
        for (; i != ied; i++){
            OutputLog(typeid(**i).name());
        }

        OutputLog("□常駐タスク一覧□");
        //バックグラウンドタスク
        i = bg_tasks.begin();
        ied = bg_tasks.end();
        for (; i != ied; i++){
            OutputLog(typeid(**i).name());
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

    //全部なくなっちゃったら、やばいっしょ
    bool CTaskManager::ExEmpty()
    {
        return (ex_stack.size() == 0) ? true : false;
    }

}
