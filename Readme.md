Code implementation for the paper *GPU-Accelerated Rendering of Vector Strokes with Piecewise Quadratic Approximation* accepted by conference *CAD/Graphics 2025* and journal *Graphical Models*.

Microsoft Visual Studio 2022 build:

```
mkdir build
cd build
cmake -G "Visual Studio 17 2022" ..
```

Main algorithms are implemented on shaders. For instance:

- Algorithm 1 Rendering of the Approximate Stroke Region: *quadratic.geom* for triangles generation; *quadratic.frag* for implicit-equation based rendering.

- Algorithm 2 Curvature-Guided Adaptive Subdivision for Quadratic Curves: *quadratic.tesc* for segment number calculation, *quadratic.tese* for computation of where to subdivide;

- Algorithm 3 Newton-based Arc-Length Parameter Computation: *dash_parallel.tese*;

- Algorithm 4 Texture Filling for Approximated Stroke Region: quadratic_texture.frag for Newton iteration on solving the texture coordinate.