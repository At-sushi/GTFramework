#include "stdafx.h"
#include "CppUnitTest.h"
#include "../../../src/system/task.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace GTF;

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
		};

		TEST_METHOD(TestMethod1)
		{
			// TODO: テスト コードをここに挿入します
			CTaskManager task;

			auto ptr = task.AddTask(new CTekitou<int, CTaskBase>(1));
			Assert::AreEqual((void*)task.FindTask(ptr->GetID()), (void*)ptr);
		}

		TEST_METHOD(TestMethod2)
		{
			// TODO: テスト コードをここに挿入します
			CTaskManager task;

			auto ptr = task.AddTask(new CTekitou<int, CBackgroundTaskBase>(1));
			Assert::AreNotEqual((void*)task.FindTask(ptr->GetID()), (void*)ptr);
			auto ptr2 = task.AddTask(static_cast<CTaskBase*>(new CTekitou<int, CBackgroundTaskBase>(1)));
			Assert::AreNotEqual((void*)task.FindTask(ptr2->GetID()), (void*)ptr2);
		}

	};
}