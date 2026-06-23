#include "colors.inc"
#include "textures.inc"

camera {
   location <0.0  1.5  -2.0>
   direction <0.0 0.0  1.0>
   up  <0.0  1.0  0.0>
   right <1.33333 0.0 0.0>
   look_at <0 0 0>
}

object { light_source { <0 30 10> color White  }}

object { sphere { <0 0 0> 1000 } texture { color DimGray } }

// A glass marble (large sphere) with two small air bubbles carved out
// of it (small spheres subtracted via difference), as a CSG showcase.
object {
   difference {
      sphere { <0 0 0> 1 }
      sphere { <-0.3 0.2 -0.1> 0.25 }
      sphere { <0.25 -0.3 0.2> 0.18 }
   }
   texture { Glass3 reflection 0.25 }
   bounded_by { sphere { <0 0 0> 1.1 } }
}


object { plane { <0 1 0> -1 }
   texture {
      DMFDarkOak
      scale <0.75 0.75 1>
      translate <10 -0.45 10>
      rotate <5.0 30 0.25>
      ambient 0.05
      diffuse 0.5
      reflection 0.35
      specular 0.9
      roughness 0.005
   }
}
