#include <mutex>
#include <thread>
#include <iostream>


class Data
{
public:
	size_t value{ 0 };

	void write()
	{
		while (value <= 1000000)
			++value;
	}

	void exit()
	{
		if (value % 2 == 0) {
            std::cout << value << " is divisible by 2" << std::endl;
		}
		else {
            std::cout << value << " is divisible by 2" << std::endl;
		}
	}
};


//class Data
//{
//public:
//	size_t value{ 0 };
//	std::mutex m{};
//
//	void write()
//	{
//		std::scoped_lock lk{ m };
//
//		while (value <= 1000000)
//			++value;
//	}
//
//	void exit()
//	{
//		std::scoped_lock lk{ m };
//
//		if (value % 2 == 0) {
//			std::cout << value << " is divisible by 2" << std::endl;
//		}
//		else {
//			std::cout << value << " isn't divisible by 2" << std::endl;
//		}
//	}
//};


int main()
{
	Data d{};

	std::jthread t1{ &Data::write, &d };

	while (d.value <= 1000000)
		d.exit();

	return 0;
}