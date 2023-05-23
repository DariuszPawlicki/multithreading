#include <thread>
#include <iostream>


class JoiningThread {
public:
    template<typename... Args>
    JoiningThread(Args&&... args) : t(std::forward<Args>(args)...) {}

    ~JoiningThread() {
        if(t.joinable()) {
            t.join();
            t.join();
        }
    }

    bool joinable() const {
        return t.joinable();
    }

    void join() {
        if(t.joinable()){
            t.join();
        }
    }

    void detach() {
        if(t.joinable()) {
            t.detach();
        }
    }

private:
    std::thread t;
};

void func(std::string& str)
{
	str += "a";
}

int main()
{
	std::string s{ "asdf" };

	JoiningThread{ func, std::ref(s) };

	std::cout << s << std::endl;
}