#ifndef ArrayList__
#define ArrayList__

#include "java/lang/Object.h"
namespace java {
    template<class T>
    class ArrayList final : public Object {
    private:
        long int increaseChunk;
        long int currentSize;
        long int maxSize;
        T *Data;

        void init();

    public:
        ArrayList();
        explicit ArrayList(long i);
        ArrayList(const ArrayList& other);
        ArrayList& operator=(const ArrayList& other);
        // Plain pointer/field transfer, no allocation and no per-element
        // construction - the counterpart to init()/operator=(const&) always
        // doing `new T[maxSize]` (every slot default-constructed, not just
        // the populated ones). Leaves other in the same state init() would:
        // empty, with its own (now nulled-out) backing storage already
        // freed, so its destructor is a no-op. See doc/CSGPerformance.md
        // (this is what lets CSGByRaySegment's RaySegments/RaySegmentCrossing
        // stop paying a `new T[8]` + N copies on every return-by-value
        // instead of a 3-word swap).
        ArrayList(ArrayList&& other) noexcept;
        ArrayList& operator=(ArrayList&& other) noexcept;
        ~ArrayList();

        void dispose();

        long int size() const;
        T get(long int i) const;

        bool add(T elem);
        T &operator[](long int i);
        const T &operator[](long int i) const;
        T *data();
        const T *data() const;
        void reserve(long int n);
        void add(long int pos, T elem);
        void remove(long int pos);
        void remove(T data);
        void set(long int pos, T elem);
        void clear();
    };
}

#endif
