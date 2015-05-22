#include "stdafx.h"
#include "CppUnitTest.h"
#include "../../../src/system/task.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace GTF;
#include <vector>

static std::vector<int> veve;
template<typename T, class B>
class CTekitou2 : public B
{
public:
	CTekitou2(T init) : hogehoge(init)
	{

	}
	~CTekitou2()
	{}

	T hogehoge;

	bool Execute(unsigned int e){ veve.push_back(hogehoge); return true; }
	unsigned int GetID()const{ return hogehoge; }
};

namespace GTFTest
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
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
			unsigned int GetID()const{ return 1; }
		};

		TEST_METHOD(TestMethod1)
		{
			// TODO: テスト コードをここに挿入します
			CTaskManager task;

			auto ptr = task.AddTask(new CTekitou<int, CTaskBase>(1)).lock();
			Assert::AreEqual((void*)task.FindTask(ptr->GetID()).lock().get(), (void*)ptr.get());
		}

		TEST_METHOD(TestMethod2)
		{
			// TODO: テスト コードをここに挿入します
			CTaskManager task;

			auto ptr = task.AddTask(new CTekitou<int, CBackgroundTaskBase>(1)).lock();
			Assert::AreNotEqual((void*)(task.FindTask<CBackgroundTaskBase>(ptr->GetID())).get(), (void*)ptr.get());
			Assert::AreEqual((void*)(task.FindBGTask<CBackgroundTaskBase>(ptr->GetID())).get(), (void*)ptr.get());
			auto ptr2 = task.AddTask(static_cast<CTaskBase*>(new CTekitou<int, CBackgroundTaskBase>(1))).lock();
			Assert::AreNotEqual((void*)task.FindTask(ptr2->GetID()).lock().get(), (void*)ptr2.get());
			Assert::AreEqual((void*)task.FindBGTask(ptr2->GetID()).lock().get(), (void*)ptr2.get());
		}

		TEST_METHOD(TestMethod3)
		{
			// TODO: テスト コードをここに挿入します
			CTaskManager task;

			auto ptr = task.AddTask(new CTekitou<int, CExclusiveTaskBase>(1)).lock();
			Assert::AreNotEqual((void*)(task.FindTask<CExclusiveTaskBase>(ptr->GetID())).get(), (void*)ptr.get());
			auto ptr2 = task.AddTask(static_cast<CTaskBase*>(new CTekitou<int, CExclusiveTaskBase>(1))).lock();
			Assert::AreNotEqual((void*)task.FindTask(ptr2->GetID()).lock().get(), (void*)ptr2.get());
		}

		TEST_METHOD(実行順序)
		{
			// TODO: テスト コードをここに挿入します
			CTaskManager task;

			veve.clear();
			auto ptr = task.AddTask(new CTekitou2<int, CExclusiveTaskBase>(1));
			auto ptr2 = task.AddTask(new CTekitou2<int, CTaskBase>(2));
			task.Execute(0);
			Assert::AreEqual(2, veve[0]);
		}

		TEST_METHOD(実行順序2)
		{
			// TODO: テスト コードをここに挿入します
			static CTaskManager task;
			class ct : public CTekitou2 < int, CExclusiveTaskBase >
			{
			public:
				ct(int init) : CTekitou2 < int, CExclusiveTaskBase >(init)
				{

				}
				void Initialize()
				{
					task.AddTask(new CTekitou2<int, CTaskBase>(2));
				}
			};

			veve.clear();
			auto ptr = task.AddTask(new ct(1));
			task.Execute(0);
			task.Execute(1);
			Assert::AreEqual(1, veve[0]);
			Assert::AreEqual(2, veve[1]);
		}

		TEST_METHOD(TestMethod4)
		{
			// TODO: テスト コードをここに挿入します
			CTaskManager task;

			for (int i = 0; i < 256;i++)
			{
				task.AddTask(new CTekitou<int, CExclusiveTaskBase>(1));
				task.AddTask(static_cast<CTaskBase*>(new CTekitou<int, CExclusiveTaskBase>(1)));
			}
			for (int i = 0; i < 256;i++)
				task.Draw();
		}

		TEST_METHOD(タスクの依存関係)
		{
			// TODO: テスト コードをここに挿入します
			static CTaskManager task;
			class ct : public CTekitou2 < int, CExclusiveTaskBase >
			{
			public:
				ct(int init) : CTekitou2 < int, CExclusiveTaskBase >(init)
				{

				}
				void Initialize()
				{
					task.AddTask(new CTekitou2<int, CTaskBase>(hogehoge+20));
				}
				virtual bool Inactivate(unsigned int nextTaskID){ return true; }//!< 他の排他タスクが開始したときに呼ばれる
			};

			veve.clear();
			auto ptr = task.AddTask(new ct(1));
			task.Execute(0);
			auto ptr2 = task.AddTask(new ct(3));
			task.Execute(1);
			Assert::AreEqual(1, veve[0]);
			Assert::AreEqual(1+20, veve[1]);
			task.Execute(2);
			Assert::AreEqual(3, veve[2]);
			Assert::AreEqual(3+20, veve[3]);
			task.RevertExclusiveTaskByID(1);
			task.Execute(3);
			Assert::AreEqual(1, veve[4]);
			Assert::AreEqual(1+20, veve[5]);
			ptr2 = task.AddTask(new ct(4));
			task.Execute(4);
			task.RemoveTaskByID(21);
			task.Execute(5);
			Assert::AreEqual(4, veve[8]);
			Assert::AreEqual(4+20, veve[9]);

		}

	};
}