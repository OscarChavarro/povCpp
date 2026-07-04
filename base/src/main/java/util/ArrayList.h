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
