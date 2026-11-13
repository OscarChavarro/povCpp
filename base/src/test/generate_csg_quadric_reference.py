#!/usr/bin/env python3
"""Generate exact homogeneous quadric transformations using rational arithmetic."""
from fractions import Fraction
from pathlib import Path
import random

rng = random.Random(0x514D)
lines = ['// Generated independently with Fraction; include inside a test.']
for case in range(50):
    while True:
        a = [[rng.randint(-3, 3) for _ in range(3)] for _ in range(3)]
        cof = [[a[(i+1)%3][(j+1)%3]*a[(i+2)%3][(j+2)%3] -
                a[(i+1)%3][(j+2)%3]*a[(i+2)%3][(j+1)%3]
                for j in range(3)] for i in range(3)]
        det = sum(a[0][j]*cof[0][j] for j in range(3))
        if det:
            break
    t = [rng.randint(-3, 3) for _ in range(3)]
    q = [rng.randint(-4, 4) for _ in range(10)]
    m = [[Fraction(0) for _ in range(4)] for _ in range(4)]
    for i in range(3):
        for j in range(3):
            m[i][j] = Fraction(cof[j][i])
        m[i][3] = -sum(m[i][j] * t[j] for j in range(3))
    m[3][3] = Fraction(det)
    local = [[Fraction(0) for _ in range(4)] for _ in range(4)]
    for i in range(3):
        local[i][i] = Fraction(q[i])
    local[3][3] = Fraction(q[9])
    pairs = [(0,1),(0,2),(1,2),(0,3),(1,3),(2,3)]
    for k, (i, j) in enumerate(pairs):
        local[i][j] = local[j][i] = Fraction(q[3+k], 2)
    world = [[sum(m[k][i]*local[k][l]*m[l][j] for k in range(4) for l in range(4))
              for j in range(4)] for i in range(4)]
    expected = [world[0][0],world[1][1],world[2][2]] + [2*world[i][j] for i,j in pairs] + [world[3][3]]
    assert all(x.denominator == 1 for x in expected)
    matrix = 'Matrix4x4d::identityMatrix()' + ''.join(
        f'.withVal({i},{j},{a[i][j]})' for i in range(3) for j in range(3)) + ''.join(
        f'.withVal({i},3,{t[i]})' for i in range(3))
    values = ','.join(str(x) for x in q)
    reference = ','.join(str(int(x)) for x in expected)
    lines.append('{const double local[10]={' + values + '},expected[10]={' + reference +
                 '}; double result[10]={}; ASSERT_TRUE(CsgExactQuadric::world(local,' + matrix +
                 ',result)); for(int i=0;i<10;++i) EXPECT_DOUBLE_EQ(expected[i],result[i]);}')
Path(__file__).with_name('csg_quadric_reference.inc').write_text('\n'.join(lines) + '\n')
