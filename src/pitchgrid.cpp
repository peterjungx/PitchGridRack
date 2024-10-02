#include "pitchgrid.hpp"

int IntegerGCD(int a, int b){
	while (b != 0){
		int t = b;
		b = a % b;
		a = t;
	}
	return a;
}
//int inverseModulo(int a, int m) {
//	a = a % m;
//	for (int x = 1; x < m; x++){
//		if ((a * x) % m == 1){
//			return x;
//		}
//	}
//	return 1;
//}

int inverseModulo(int a, int m) {
    int m0 = m, x = 1, y = 0, temp;
    while (a > 1) {
        int q = a / m;
        temp = m;
        m = a % m;
        a = temp;
        temp = y;
        y = x - q * y;
        x = temp;
    }
    return (x + m0) % m0;
}