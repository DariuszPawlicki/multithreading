#include <atomic>


class SpinLock
{
public:

	SpinLock() = default;

	void lock()
	{
		while (flag.test_and_set())
		{

		}
	}

	void unlock()
	{
		flag.clear();
	}

private:

	std::atomic_flag flag = ATOMIC_FLAG_INIT;
};