#ifndef __LOOKUPTABLESINE__
#define __LOOKUPTABLESINE__

/**
Precomputed sin() values over one full period [0, 2*pi), sampled at SIZE
equally spaced points. Lets callers replace a sin(2*pi*fraction) evaluation
with a single array lookup.
*/
class LookUpTableSine {
  private:
    static constexpr int SIZE = 1000;
    double table[SIZE];

  public:
    LookUpTableSine();
    LookUpTableSine(int numberOfApproximationDecimals);
    double eval(double fraction) const;
};

/**
Returns sin(2*pi*fraction) for fraction in [0, 1), via table lookup.
*/
inline double LookUpTableSine::eval(double fraction) const
{
    return table[(int)(fraction * SIZE)];
}

#endif
