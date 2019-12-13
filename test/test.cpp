#define GTF_HEADER_ONLY
#include "../iutest/include/iutest.hpp"
#include "../src/system/task.h"

#include <vector>

using namespace GTF;

static std::vector<int> veve;

template<typename T, class B>
class CTekitou : public B
{
public:
    CTekitou(T init) : hogehoge(init)
    {

    }
    ~CTekitou()
    {}

    T hogehoge;
    unsigned int GetID() const override { return hogehoge; }
    int GetDrawPriority() const override { return 1; }
    void Draw() override { veve.push_back(hogehoge); }
};

template<typename T, class B, typename... Arg>
class CTekitou2 : public B
{
public:
	CTekitou2(T init, Arg&&... args) : B(forward<Arg>(args)...), hogehoge(init)
	{

	}
	~CTekitou2()
	{}

	T hogehoge;

	bool Execute(double /* e */) override{ veve.push_back(hogehoge); return true; }
	unsigned int GetID()const override{ return hogehoge; }
};

template<typename T, typename... Arg>
class CExTaskSelfDestruct : public CExclusiveTaskBase
{
public:
	CExTaskSelfDestruct(T init, Arg&&... args) : CExclusiveTaskBase(forward<Arg>(args)...), hogehoge(init)
	{

	}
	~CExTaskSelfDestruct()
	{}

	T hogehoge;

	bool Execute(double /* e */) override{ veve.push_back(hogehoge); return false; }
	unsigned int GetID()const override{ return hogehoge; }
};

IUTEST(GTFTest, TestMethod1)
{
    CTaskManager task;
    auto ptr = task.AddNewTask< CTekitou<int, CTaskBase> >(1);
    IUTEST_ASSERT_EQ((void*)task.FindTask(ptr->GetID()).lock().get(), (void*)ptr.get());
    IUTEST_ASSERT_EQ((void*)task.FindTask<CTaskBase>(ptr->GetID()).get(), (void*)ptr.get());
}

IUTEST(GTFTest, TestMethod2)
{
    CTaskManager task;
    auto ptr = task.AddTask(new CTekitou<int, CBackgroundTaskBase>(1)).lock();
    IUTEST_ASSERT_EQ((void*)(task.FindTask<CBackgroundTaskBase>(ptr->GetID())).get(), (void*)ptr.get());
    auto ptr2 = task.AddTask(static_cast<CTaskBase*>(new CTekitou<int, CBackgroundTaskBase>(1))).lock();
    IUTEST_ASSERT_NE((void*)task.FindTask(ptr2->GetID()).lock().get(), (void*)ptr2.get());
    IUTEST_ASSERT_EQ((void*)task.FindBGTask(ptr2->GetID()).lock().get(), (void*)ptr2.get());
}
IUTEST(GTFTest, TestMethod3)
{
    CTaskManager task;
    auto ptr = task.AddTask(new CTekitou<int, CExclusiveTaskBase>(1)).lock();
    auto ptr2 = task.AddTask(static_cast<CTaskBase*>(new CTekitou<int, CExclusiveTaskBase>(1))).lock();
    IUTEST_ASSERT_NE((void*)task.FindTask(ptr2->GetID()).lock().get(), (void*)ptr2.get());
}
IUTEST(GTFTest, RunOrder1)
{
    CTaskManager task;
    veve.clear();
    auto ptr = task.AddNewTask< CTekitou2<int, CExclusiveTaskBase> >(1);
    auto ptr2 = task.AddNewTask< CTekitou2<int, CTaskBase> >(2);
    //modify veve
    task.Execute(0);
    IUTEST_ASSERT_EQ(2, veve[0]);
}
IUTEST(GTFTest, RunOrder2)
{
    static CTaskManager task;
    class ct : public CTekitou2 < int, CExclusiveTaskBase >
    {
    public:
        ct(int init) : CTekitou2 < int, CExclusiveTaskBase >(init) {}
        void Initialize()
        {
            task.AddTask(new CTekitou2<int, CTaskBase>(2));
        }
    };

    veve.clear();
    auto ptr = task.AddNewTask<ct>(1);
    task.Execute(0);
    task.Execute(1);
    IUTEST_ASSERT_EQ(1, veve[0]);
    IUTEST_ASSERT_EQ(2, veve[1]);
}

IUTEST(GTFTest, Draw1)
{
    static CTaskManager task;
    class ct2 : public CTekitou2 < int, CExclusiveTaskBase >
    {
    public:
        ct2(int init) : CTekitou2 < int, CExclusiveTaskBase >(init)
        {

        }
        void Initialize()
        {
            task.AddNewTask< CTekitou<int, CTaskBase> >(hogehoge + 1);
        }
    };

    for (int i = 1; i < 257; i++)
    {
        task.AddNewTask<ct2>(i * 2);
        task.Execute(0);
    }
    veve.clear();
    task.RemoveTaskByID(1);
    for (int i = 0; i < 4; i++)
        task.Draw();

    IUTEST_ASSERT_EQ(513, veve[0]);
}
IUTEST(GTFTest, Draw2)
{
    static CTaskManager task;
    class ct2 : public CTekitou2 < int, CExclusiveTaskBase >
    {
    public:
        ct2(int init) : CTekitou2 < int, CExclusiveTaskBase >(init)
        {

        }
        void Initialize()
        {
            task.AddNewTask< CTekitou<int, CBackgroundTaskBase> >(hogehoge + 1);
        }
    };

    for (int i = 1; i < 5; i++)
    {
        task.AddNewTask<ct2>(i * 2);
        task.Execute(0);
    }
    veve.clear();
    task.RemoveTaskByID(1);
    for (int i = 0; i < 4; i++)
        task.Draw();

    IUTEST_ASSERT_EQ(3, veve[0]);
}
IUTEST(GTFTest, DrawFallthrough)
{
    static CTaskManager task;
    class ct2 : public CTekitou2 < int, CExclusiveTaskBase, bool >
    {
    public:
        ct2(int init) : CTekitou2 < int, CExclusiveTaskBase, bool >(init, true)
        {

        }
        void Initialize()
        {
            task.AddNewTask< CTekitou<int, CTaskBase> >(hogehoge + 1);
        }
    };

    for (int i = 1; i < 5; i++)
    {
        task.AddNewTask<ct2>(i * 2);
        task.Execute(0);
    }
    veve.clear();
    task.RemoveTaskByID(1);
    for (int i = 0; i < 4; i++)
        task.Draw();

    IUTEST_ASSERT_EQ(3, veve[0]);
}

IUTEST(GTFTest, TaskDependency)
{
    static CTaskManager task;
    class ct : public CTekitou2 < int, CExclusiveTaskBase >
    {
    public:
        ct(int init) : CTekitou2 < int, CExclusiveTaskBase >(init)
        {

        }
        void Initialize()
        {
            task.AddNewTask< CTekitou2<int, CTaskBase> >(hogehoge + 20);
        }
        virtual bool Inactivate(unsigned int /* nextTaskID */){ return true; }//!< 他の排他タスクが開始したときに呼ばれる
    };

    veve.clear();
    auto ptr = task.AddNewTask<ct>(1);
    task.Execute(0);
    auto ptr2 = task.AddNewTask<ct>(3);
    task.Execute(1);
    IUTEST_ASSERT_EQ(1, veve[0]);
    IUTEST_ASSERT_EQ(1 + 20, veve[1]);
    task.Execute(2);
    IUTEST_ASSERT_EQ(3, veve[2]);
    IUTEST_ASSERT_EQ(3 + 20, veve[3]);
    task.RevertExclusiveTaskByID(1);
    task.Execute(3);
    IUTEST_ASSERT_EQ(1, veve[4]);
    IUTEST_ASSERT_EQ(1 + 20, veve[5]);
    ptr2 = task.AddNewTask<ct>(4);
    task.Execute(4);
    task.RemoveTaskByID(21);
    task.Execute(5);
    IUTEST_ASSERT_EQ(4, veve[8]);
    IUTEST_ASSERT_EQ(4 + 20, veve[9]);
}
IUTEST(GTFTest, ReuseContainer)
{
    CTaskManager task;
    auto ptr = task.AddTask(new CTekitou<int, CExclusiveTaskBase>(1)).lock();
    auto ptr2 = task.AddTask(static_cast<CTaskBase*>(new CTekitou<int, CExclusiveTaskBase>(1))).lock();
    IUTEST_ASSERT_NE((void*)task.FindTask(ptr2->GetID()).lock().get(), (void*)ptr2.get());

    task.Destroy();
    task.Execute(0);
    IUTEST_ASSERT_EQ((void*)task.GetTopExclusiveTask().lock().get(), (void*)nullptr);

    auto ptr3 = task.AddTask(new CTekitou<int, CExclusiveTaskBase>(1)).lock();
    auto ptr4 = task.AddTask(static_cast<CTaskBase*>(new CTekitou<int, CExclusiveTaskBase>(1))).lock();
    task.Execute(0);
    IUTEST_ASSERT_NE((void*)task.FindTask(ptr4->GetID()).lock().get(), (void*)ptr4.get());
}
IUTEST(GTFTest, ExTaskSelfDeestruct)
{
    CTaskManager task;
    veve.clear();
    auto ptr = task.AddNewTask< CTekitou2<int, CExclusiveTaskBase> >(1);
    task.Execute(0);
    auto ptr2 = task.AddNewTask< CExTaskSelfDestruct<int> >(2);
    task.Execute(0);
    task.Execute(0);
    IUTEST_ASSERT_EQ(1, veve[0]);
    IUTEST_ASSERT_EQ(2, veve[1]);
    IUTEST_ASSERT_EQ(1, veve[2]);
}
int main(int argc, char** argv)
{
    IUTEST_INIT(&argc, argv);
    return IUTEST_RUN_ALL_TESTS();
}
