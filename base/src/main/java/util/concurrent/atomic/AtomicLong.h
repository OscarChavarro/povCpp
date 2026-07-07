#ifndef __ATOMICLONG__
#define __ATOMICLONG__

#include <atomic>
namespace java {

class AtomicLong {
private:
    std::atomic<long long> value_;

public:
    AtomicLong() : value_(0) {}
    explicit AtomicLong(long long initialValue) : value_(initialValue) {}

    long long get() const { return value_.load(); }
    void set(long long value) { value_.store(value); }
    long long incrementAndGet() { return value_.fetch_add(1) + 1; }
};

}

#endif
