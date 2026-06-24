#include "java/util/Collections.h"
namespace java {

template <class T> void
Collections::swap(ArrayList<T> &a, ArrayList<T> &b) {
    ArrayList<T> temp(static_cast<ArrayList<T>&&>(a));
    a = static_cast<ArrayList<T>&&>(b);
    b = static_cast<ArrayList<T>&&>(temp);
}

}
