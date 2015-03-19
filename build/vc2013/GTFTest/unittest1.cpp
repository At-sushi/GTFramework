#include "stdafx.h"
#include "CppUnitTest.h"
#include "../../../src/system/task.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace GTF;

//template<typename T> static std::wstring ToString<T>(const T* t)                  { RETURN_WIDE_STRING(t); }
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

	};
}