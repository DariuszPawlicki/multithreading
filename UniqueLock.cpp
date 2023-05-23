#include <mutex>


template<typename Mutex>
class UniqueLock {
public:
    explicit UniqueLock(Mutex& mutex) : mutex(&mutex) {
        lock();
    }

    UniqueLock(UniqueLock&& other) noexcept = default;

    UniqueLock& operator=(const UniqueLock& other) = delete;
    UniqueLock(const UniqueLock& other) = delete;

    ~UniqueLock() {
        if(owns_mutex) {
            unlock();
        }
    }

    void lock() {
        if (mutex) {
            mutex->lock();
            owns_mutex = true;
        }
    }

    void unlock() {
        if(mutex) {
            mutex->unlock();
            owns_mutex = false;
        }
    }

    Mutex* release() {
        Mutex* tmp = mutex;
        mutex = nullptr;
        owns_mutex = false;
        return tmp;
    }

private:
    Mutex* mutex;
    bool owns_mutex {false};
};