#ifndef FRACTAL_COMPLEX_H_
#define FRACTAL_COMPLEX_H_

typedef struct
{
    double x;
    double y;
} complex_t;

complex_t complex_t_mul(complex_t a, complex_t b);

complex_t complex_t_mul(complex_t a, complex_t b)
{
    complex_t res;
	res.x = a.x * b.x - a.y * b.y;
	res.y = a.x * b.y + a.y * b.x;
	return res;
}

complex_t complex_t_add( complex_t a,  complex_t b);
complex_t complex_t_add( complex_t a,  complex_t b)
{
	 complex_t res;

	res.x = a.x + b.x;
	res.y = a.y + b.y;
	return res;
}

double complex_t_modul( complex_t z);
double complex_t_modul( complex_t z)
{
    double  res;

	res = sqrt(z.x * z.x + z.y * z.y);
	return res;
}

double equation(int x, double x1, double y1, double x2, double y2);
double equation(int x, double x1, double y1, double x2, double y2)
{
    double a = (y1 - y2) / (x1 - x2);
    double b = (x1 * y2 - y1 * x2) / (x1 - x2);
    double y = a * x + b;

	return y;
}

#endif /* FRACTAL_COMPLEX_H_ */
