***4 implementations of Deferred shading***
---

### Method 1. Forward Shading
1 pass of rasterization
- 100 lights of random positions and colors

### Method 2. Simple Deferred Shading
2 passes of rasterization
- geometry pass which produces position, normal, albeo textures with depth testing
- lighting pass for shading without depth testing

### Method 3. Deferred Shading optimization with light attenuation range
By using actual effective range of each lights, shading stage is reduced.

### Method 4. Deferred Shading optimization using stencil buffer
Considering the parallelism of GPU, each light executes the fragment shader only for affected pixels.
- Reference : <https://www.ogldev.org/www/tutorial37/tutorial37.html>
