#include "pitchgrid.hpp"

int IntegerGCD(int a, int b){
	while (b != 0){
		int t = b;
		b = a % b;
		a = t;
	}
	return a;
}
