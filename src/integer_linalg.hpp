#pragma once

// integer lattice linear algebra helpers
struct IntegerVector {
	int x;
	int y;
	IntegerVector(){
		this->x = 0;
		this->y = 0;
	}
	IntegerVector(int x, int y){
		this->x = x;
		this->y = y;
	}
	// overload + and - operators
	IntegerVector operator+(const IntegerVector& c) const {
		return {x + c.x, y + c.y};
	}
	IntegerVector operator-(const IntegerVector& c) const {
		return {x - c.x, y - c.y};
	}
	// overload * operator -> scalar product
	int operator*(const IntegerVector& c) const {
		return x * c.x + y * c.y;
	}
	bool operator==(const IntegerVector& c) const {
		return x == c.x && y == c.y;
	}
	bool operator!=(const IntegerVector& c) const {
		return x != c.x || y != c.y;
	}
	IntegerVector operator*(int c) const {
		return {x * c, y * c};
	}
};
inline int IntegerDet(IntegerVector a, IntegerVector b){
	return a.x * b.y - a.y * b.x;
}
inline int IntegerDet(int a11, int a12, int a21, int a22){
	return a11*a22 - a12*a21;
};
struct IntegerMatrix{
	int m11, m12, m21, m22;
	// overload * operator
	IntegerVector operator*(const IntegerVector& c) const {
		IntegerVector d;
		d.x = m11 * c.x + m12 * c.y;
		d.y = m21 * c.x + m22 * c.y;
		return d;
	}
};
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

