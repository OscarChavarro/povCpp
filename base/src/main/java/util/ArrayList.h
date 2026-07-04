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

        // Defined inline (not in ArrayList.txx) so every translation unit
        // that includes this header sees the full body and can inline the
        // capacity-0 no-allocation path at the call site, rather than
        // requiring cross-TU LTO to fold an out-of-line call. This is the
        // path RayWithSegments::RayWithSegments(LocalIntersectionClone, ...)
        // takes tens of millions of times per render (Plan 12 Phase 2) - it
        // was measured as the single hottest self-time leaf post-LTO because
        // LTO's own inlining heuristics did not reach across every call
        // site. The growth/copy paths (add/reserve/operator=/...) stay in
        // ArrayList.txx: they are not on this hot path and keeping them
        // out-of-line avoids code bloat from a heavier inline body.
        void init()
        {
            if (maxSize > 0) {
                Data = new T[maxSize];
                if (!Data) {
                    maxSize = 0;
                }
            } else {
                Data = nullptr;
            }
            currentSize = 0;
        }

    public:
        ArrayList();
        explicit ArrayList(long i)
        {
            currentSize = 0;
            increaseChunk = i;
            maxSize = increaseChunk;
            init();
        }
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
