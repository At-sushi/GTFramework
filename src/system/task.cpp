

/*============================================================================

    タスク管理(?)

==============================================================================*/

#include <vector>
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
            delete (*i);
        }
        tasks.clear();

        //バックグラウンドタスクTerminate
        i = bg_tasks.begin();
        ied = bg_tasks.end();
        for (; i != ied; i++){
            (*i)->Terminate();
            delete (*i);
        }
        bg_tasks.clear();

        //排他タスクTerminate
        while (ex_stack.size() != 0){
            ex_stack.top()->Terminate();
            delete(ex_stack.top());
            ex_stack.pop();
        }
    }


    CTaskBase* CTaskManager::AddTask(CTaskBase *newTask)
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
        tasks.push_back(newTask);
        newTask->Initialize();
        return newTask;
    }

    CExclusiveTaskBase* CTaskManager::AddTask(CExclusiveTaskBase *newTask)
    {
        if (newTask->GetID() != 0){
            RemoveTaskByID(newTask->GetID());
        }

        CExclusiveTaskBase *pext = newTask;

        //排他タスクとしてAdd
        //Execute中かもしれないので、ポインタ保存のみ
        if (exNext){
            OutputLog("■ALERT■ 排他タスクが2つ以上Addされた : %s / %s",
                typeid(*exNext).name(), typeid(*newTask).name());
        }
        exNext = pext;

        return pext;
    }

    CBackgroundTaskBase* CTaskManager::AddTask(CBackgroundTaskBase *newTask)
    {
        if (newTask->GetID() != 0){
            RemoveTaskByID(newTask->GetID());
        }

        CBackgroundTaskBase *pbgt = newTask;

        //常駐タスクとしてAdd
        bg_tasks.push_back(pbgt);
        pbgt->Initialize();
        return pbgt;
    }

    void CTaskManager::Execute(unsigned int time)
    {
        TaskList::iterator i, ied;
        std::list<TaskList::iterator> deleteList;
        std::list<TaskList::iterator>::iterator idl, idl_ed;
        CTaskBase *delTgt;

#ifdef ARRAYBOUNDARY_DEBUG
        if(!AfxCheckMemory()){
            OutputLog("AfxCheckMemory() failed");
            return;
        }
#endif

        CExclusiveTaskBase *exTsk;

        bool ex_ret;

        //排他タスク、topのみExecute
        if (ex_stack.size() != 0){
            exTsk = ex_stack.top();
#ifdef _CATCH_WHILE_EXEC
            try{
#endif
                ex_ret = true;
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
                        i = tasks.begin();
                        ied = tasks.end();
                        for (; i != ied; i++){
                            delTgt = (*i);
                            delTgt->Terminate();
                            delete delTgt;
                        }
                        tasks.clear();

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
                        delete exTsk;
                        exTsk = NULL;
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
                        exTsk = ex_stack.top();
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
        i = tasks.begin();
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
                delTgt = *i;
                delTgt->Terminate();
                delete delTgt;
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
                delTgt = *i;
                delTgt->Terminate();
                delete delTgt;
                bg_tasks.erase(i);
            }
            deleteList.clear();
        }

        // 新しいタスクがある場合
        if (exNext){
            //通常タスクを全て破棄する
            CleanupAllSubTasks();

            //現在排他タスクのInactivate
            if (ex_stack.size() != 0){
                exTsk = ex_stack.top();
                if (!exTsk->Inactivate(exNext->GetID())){
                    exTsk->Terminate();
                    delete exTsk;
                    ex_stack.pop();
                }
            }

            //AddされたタスクをInitializeして突っ込む
            ex_stack.push(exNext);
            exNext->Initialize();

            exNext = NULL;
        }
    }


    void CTaskManager::Draw()
    {
        std::vector<CTaskBase*> tmplist;
        TaskList::iterator i, ied;

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
            if (ex_stack.top()->GetDrawPriority() >= 0){
                tmplist.push_back(ex_stack.top());
            }
        }

        std::sort(tmplist.begin(), tmplist.end(), CTaskBase::CompByDrawPriority);//描画プライオリティ順にソート

        //描画
        std::vector<CTaskBase*>::iterator iv = tmplist.begin();
        std::vector<CTaskBase*>::iterator iedv = tmplist.end();
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

    }

    void CTaskManager::RemoveTaskByID(unsigned int id)
    {
        TaskList::iterator i, ied;

        //通常タスクをチェック
        if (!tasks.empty())
        {
            i = tasks.begin();
            ied = tasks.end();
            for (; i != ied; i++){
                if (id == (*i)->GetID()){
                    (*i)->Terminate();
                    delete (*i);
                    tasks.erase(i);
                    return;
                }
            }
        }

        //バックグラウンドタスクTerminate
        if (!bg_tasks.empty())
        {
            i = bg_tasks.begin();
            ied = bg_tasks.end();
            for (; i != ied; i++){
                if (id == (*i)->GetID()){
                    (*i)->Terminate();
                    delete (*i);
                    bg_tasks.erase(i);
                    return;
                }
            }
        }
    }


    //最上位にあるエクスクルーシブタスクをゲト
    CExclusiveTaskBase* CTaskManager::GetTopExclusiveTask()
    {
        if (ex_stack.size() == 0)return nullptr;
        return ex_stack.top();
    }

    //指定IDの排他タスクまでTerminate/popする
    void CTaskManager::ReturnExclusiveTaskByID(unsigned int id)
    {
        bool act = false;
        unsigned int previd = 0;

        while (ex_stack.size() != 0){
            CExclusiveTaskBase *task = ex_stack.top();
            if (task->GetID() == id){
                if (act){
                    task->Activate(previd);
                }
                return;
            }
            else{
                previd = task->GetID();
                act = true;
                CleanupAllSubTasks();
                task->Terminate();
                delete task;
                ex_stack.pop();
            }
        }
    }

    //通常タスクを全て破棄する
    void CTaskManager::CleanupAllSubTasks()
    {
        CTaskBase *delTgt;
        TaskList::iterator i, ied;

        i = tasks.begin();
        ied = tasks.end();
        for (; i != ied; i++){
            delTgt = (*i);
            delTgt->Terminate();
            delete delTgt;
        }
        tasks.clear();
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
            OutputLog(typeid(*ex_stack.top()).name());


        OutputLog("\n\n■CTaskManager::DebugOutputTaskList() - end\n\n");
    }


    //指定IDの常駐タスク取得
    CBackgroundTaskBase* CTaskManager::FindBGTask(unsigned int id)
    {
        TaskList::iterator i, ied;

        i = bg_tasks.begin();
        ied = bg_tasks.end();
        for (; i != ied; i++){
            if ((*i)->GetID() == id){//ハケーソ
                return dynamic_cast<CBackgroundTaskBase*>(*i);
            }
        }
        return NULL;
    }

    //指定IDの通常タスク取得
    CTaskBase* CTaskManager::FindTask(unsigned int id)
    {
        TaskList::iterator i, ied;

        i = tasks.begin();
        ied = tasks.end();
        for (; i != ied; i++){
            if ((*i)->GetID() == id){//ハケーソ
                return (*i);
            }
        }
        return NULL;
    }

    //全部なくなっちゃったら、やばいっしょ
    bool CTaskManager::ExEmpty()
    {
        return (ex_stack.size() == 0) ? true : false;
    }

}
