#include "integer_linalg.hpp"

IntegerMatrix findTransform(IntegerVector a, IntegerVector x, IntegerVector b, IntegerVector y){
	// returns matrix that transforms a to x and b to y
	int d = IntegerDet(a.x, b.x, a.y, b.y);
	IntegerMatrix m;
	m.m11 = IntegerDet(x.x, y.x, a.y, b.y) / d;
	m.m12 = IntegerDet(a.x, b.x, x.x, y.x) / d;
	m.m21 = IntegerDet(x.y, y.y, a.y, b.y) / d;
	m.m22 = IntegerDet(a.x, b.x, x.y, y.y) / d;
	return m;
};

