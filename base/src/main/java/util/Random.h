#ifndef __JAVA_UTIL_RANDOM__
#define __JAVA_UTIL_RANDOM__

#include "java/lang/System.h"

namespace java {

/**
Emulation of `java.util.Random`: the same 48-bit linear congruential
generator, so a given seed produces the same sequence as in the Java port.
*/
class Random {
private:
    // Unsigned arithmetic wraps as Java long arithmetic does
    static const unsigned long long MULTIPLIER = 0x5DEECE66DULL;
    static const unsigned long long ADDEND = 0xBULL;
    static const unsigned long long MASK = (1ULL << 48) - 1;

    unsigned long long seed;

    static unsigned long long initialScramble(long long value)
    {
        return ((unsigned long long)value ^ MULTIPLIER) & MASK;
    }

    int next(int bits)
    {
        seed = (seed * MULTIPLIER + ADDEND) & MASK;
        return (int)(long long)(seed >> (48 - bits));
    }

public:
    Random()
        : seed(initialScramble((long long)(
              (unsigned long long)java::System::nanoTime() ^
              (8682522807148012ULL * 181783497276652981ULL))))
    {
    }

    explicit Random(long long seedValue) : seed(initialScramble(seedValue))
    {
    }

    void setSeed(long long seedValue)
    {
        seed = initialScramble(seedValue);
    }

    int nextInt()
    {
        return next(32);
    }

    /**
    @param bound upper bound (exclusive), must be positive
    @return uniformly distributed value in [0, bound)
    */
    int nextInt(int bound)
    {
        if ( bound <= 0 ) {
            return 0;
        }
        if ( (bound & -bound) == bound ) {
            return (int)((bound * (long long)next(31)) >> 31);
        }
        int bits;
        int value;
        do {
            bits = next(31);
            value = bits % bound;
        } while ( bits - value + (bound - 1) < 0 );
        return value;
    }

    /**
    @return uniformly distributed value in [0, 1)
    */
    double nextDouble()
    {
        return (double)(((long long)next(26) << 27) + next(27)) *
            (1.0 / (double)(1LL << 53));
    }

    bool nextBoolean()
    {
        return next(1) != 0;
    }
};

}

#endif
