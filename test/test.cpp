#define GTF_HEADER_ONLY
#include "../iutest/include/iutest.hpp"
#include "../src/system/task.h"

#include <vector>

using namespace gtf;

IUTEST_MAKE_PEEP(std::weak_ptr<BackgroundTaskBase> (TaskManager::*)(unsigned int) const, TaskManager, FindBGTask);

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
	CTekitou2(T init, Arg&&... args) : B(std::forward<Arg>(args)...), hogehoge(init)
	{

	}
	~CTekitou2()
	{}

	T hogehoge;

	bool Execute(double /* e */) override{ veve.push_back(hogehoge); return true; }
	unsigned int GetID()const override{ return hogehoge; }
};

template<typename T, typename... Arg>
class CExTaskSelfDestruct : public ExclusiveTaskBase
{
public:
	CExTaskSelfDestruct(T init, Arg&&... args) : ExclusiveTaskBase(std::forward<Arg>(args)...), hogehoge(init)
	{

	}
	~CExTaskSelfDestruct()
	{}

	T hogehoge;

	bool Execute(double /* e */) override{ veve.push_back(hogehoge); return false; }
	unsigned int GetID()const override{ return hogehoge; }
};

IUTEST(gtfTest, TestMethod1)
{
    TaskManager task;
    auto ptr = task.AddNewTask< CTekitou<int, TaskBase> >(1);
    IUTEST_ASSERT_EQ((void*)task.FindTask<TaskBase>(ptr->GetID()).get(), (void*)ptr.get());
}

IUTEST(gtfTest, TestMethod2)
{
    TaskManager task;
    auto ptr = task.AddNewTask< CTekitou2<int, BackgroundTaskBase> >(1);
    IUTEST_ASSERT_EQ((void*)(task.FindTask<BackgroundTaskBase>(ptr->GetID())).get(), (void*)ptr.get());
}
IUTEST(gtfTest, TestMethod3)
{
    TaskManager task;
    auto ptr = task.AddNewTask< CTekitou2<int, ExclusiveTaskBase> >(1);
    IUTEST_ASSERT_NE((void*)task.FindTask<TaskBase>(ptr->GetID()).get(), (void*)ptr.get());
}
IUTEST(gtfTest, RunOrder1)
{
    TaskManager task;
    veve.clear();
    auto ptr = task.AddNewTask< CTekitou2<int, ExclusiveTaskBase> >(1);
    auto ptr2 = task.AddNewTask< CTekitou2<int, TaskBase> >(2);
    //modify veve
    task.Execute(0);
    task.Execute(1);
    IUTEST_ASSERT_EQ(1, veve[0]);
    IUTEST_ASSERT_EQ(1, veve[1]);
}
IUTEST(gtfTest, RunOrder2)
{
    static TaskManager task;
    class ct : public CTekitou2 < int, ExclusiveTaskBase >
    {
    public:
        ct(int init) : CTekitou2 < int, ExclusiveTaskBase >(init) {}
        void Initialize()
        {
            task.AddNewTask< CTekitou2<int, TaskBase> >(2);
        }
    };

    veve.clear();
    auto ptr = task.AddNewTask<ct>(1);
    task.Execute(0);
    task.Execute(1);
    IUTEST_ASSERT_EQ(1, veve[0]);
    IUTEST_ASSERT_EQ(2, veve[1]);
}

IUTEST(gtfTest, Draw1)
{
    static TaskManager task;
    class ct2 : public CTekitou2 < int, ExclusiveTaskBase >
    {
    public:
        ct2(int init) : CTekitou2 < int, ExclusiveTaskBase >(init)
        {

        }
        void Initialize()
        {
            task.AddNewTask< CTekitou<int, TaskBase> >(hogehoge + 1);
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
IUTEST(gtfTest, Draw2)
{
    static TaskManager task;
    class ct2 : public CTekitou2 < int, ExclusiveTaskBase >
    {
    public:
        ct2(int init) : CTekitou2 < int, ExclusiveTaskBase >(init)
        {

        }
        void Initialize()
        {
            task.AddNewTask< CTekitou<int, BackgroundTaskBase> >(hogehoge + 1);
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
IUTEST(gtfTest, DrawFallthrough)
{
    static TaskManager task;
    class ct2 : public CTekitou2 < int, ExclusiveTaskBase, bool >
    {
    public:
        ct2(int init) : CTekitou2 < int, ExclusiveTaskBase, bool >(init, true)
        {

        }
        void Initialize()
        {
            task.AddNewTask< CTekitou<int, TaskBase> >(hogehoge + 1);
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

IUTEST(gtfTest, TaskDependency)
{
    static TaskManager task;
    class ct : public CTekitou2 < int, ExclusiveTaskBase >
    {
    public:
        ct(int init) : CTekitou2 < int, ExclusiveTaskBase >(init)
        {

        }
        void Initialize()
        {
            task.AddNewTask< CTekitou2<int, TaskBase> >(hogehoge + 20);
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
IUTEST(gtfTest, ReuseContainer)
{
    TaskManager task;
    auto ptr = task.AddNewTask< CTekitou2<int, ExclusiveTaskBase> >(1);
    IUTEST_ASSERT_NE((void*)task.FindTask<TaskBase>(ptr->GetID()).get(), (void*)ptr.get());

    task.Destroy();
    task.Execute(0);
    IUTEST_ASSERT_EQ((void*)task.GetTopExclusiveTask().lock().get(), (void*)nullptr);

    auto ptr3 = task.AddNewTask< CTekitou2<int, ExclusiveTaskBase> >(1);
    task.Execute(0);
    IUTEST_ASSERT_NE((void*)task.FindTask<TaskBase>(ptr3->GetID()).get(), (void*)ptr3.get());
}
IUTEST(gtfTest, ExTaskSelfDeestruct)
{
    TaskManager task;
    veve.clear();
    auto ptr = task.AddNewTask< CTekitou2<int, ExclusiveTaskBase> >(1);
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
