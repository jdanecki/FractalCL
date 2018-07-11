# Implemented fractals

* julia
```
j_x = z_x * z_x - z_y * z_y + c_x;
j_y = 2 * z_x * z_y + c_y;
```
![julia](julia.png)

* julia
```
j_x = z_x * z_x * z_x - 3 * z_x * z_y * z_y + c_x;
j_y = 3 * z_x * z_x * z_y - z_y * z_y * z_y + c_y;
```
![julia3](julia3.png)

* mandelbrot
```
j_x = z_x * z_x - z_y * z_y + c_x;
j_y = 2 * z_x * z_y + c_y;
```
![mandelbrot](mandelbrot.png)

* burning ship
```
j_x = z_x * z_x - z_y * z_y + c_x;
j_y = 2 * fabs(z_x * z_y) + c_y;
```
![burning_ship](burning_ship.png)

* zoomed burning ship

![burning_ship](burning_ship1.png)

* modified zoomed burning ship
```
j_x = fabs(z_x * z_x - z_y * z_y) + c_x;
j_y = 2 * fabs(z_x * z_y) + c_y;
```
![burning_ship](burning_ship2.png)

* generalized celtic
```
j_x = fabs(z_x * z_x - z_y * z_y) + c_x;
j_y = 2 * z_x * z_y + c_y;
```
![generalized_celtic](generalized_celtic.png)

* tricorn
```
j_x = z_x * z_x - z_y * z_y + c_x;
j_y = -2 * z_x * z_y + c_y;
```
![tricorn](tricorn.png)

* dragon
```
random select next point
x1 = -0.3 * xc - 1.0;
y1 = -0.3 * yc + 0.1;
or 
x1 = 0.76 * xc - 0.4 * yc;
y1 = 0.4 * xc + 0.76 * yc;

```
![dragon](dragon.png)


