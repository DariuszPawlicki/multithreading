#include <mutex>
#include <thread>
#include <iostream>



//class A
//{
//public:
//	int x;
//	std::string name;
//
//	A(int x, const std::string& name) : x(x), name(name) {};
//
//	void swap(A& rhs) {
//		std::lock_guard<std::mutex> lock{ m };
//		std::cout << "Locked First: " << name << std::endl;
//
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//
//		std::lock_guard<std::mutex> lock2{ rhs.m };
//		std::cout << "Locked Second: " << rhs.name << std::endl;
//
//		int tmp = x;
//		x = rhs.x;
//		rhs.x = tmp;
//	}
//
//private:
//	std::mutex m{};
//};


class A
{
public:
	int x;
	std::string name;

	A(int x, const std::string& name) : x(x), name(name) {};

	void swap(A& rhs) {
		std::lock(m, rhs.m);
		std::lock_guard<std::mutex> lock{ m, std::adopt_lock };
		std::cout << "Locked First: " << name << std::endl;

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		std::lock_guard<std::mutex> lock2{ rhs.m, std::adopt_lock };
		std::cout << "Locked Second: " << rhs.name << std::endl;

		int tmp = x;
		x = rhs.x;
		rhs.x = tmp;
	}

private:
	std::mutex m{};
};


int main()
{
	A a1{ 12, "a1" };
	A a2{ 23, "a2" };

	std::jthread t{ &A::swap, &a1, std::ref(a2) };

	a2.swap(a1);

	std::cout << a1.x << std::endl;
	std::cout << a2.x << std::endl;
}