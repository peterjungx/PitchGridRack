#include <rack.hpp>
using namespace rack;

#include "integer_linalg.hpp"

typedef IntegerVector ScaleVector;

class ConsistentTuning {
	ScaleVector v1, v2;
	float f1, log2f1;
	float f2, log2f2;
	float det;
public:
	ConsistentTuning(ScaleVector v1, float f1, ScaleVector v2, float f2) {
		this->setParams(v1, f1, v2, f2);
	};
	void setParams(ScaleVector v1, float f1, ScaleVector v2, float f2) {
		this->v1 = v1;
		this->v2 = v2;
		this->f1 = f1;
		this->f2 = f2;
		this->det = IntegerDet(v1, v2);
		assert(this->det != 0);
		this->log2f1 = log2(f1);
		this->log2f2 = log2(f2);
	};
	float vecToFreqRatio(ScaleVector v){
		float z1 = IntegerDet(v, this->v2) / this->det;
		float z2 = IntegerDet(this->v1, v) / this->det;
		float f = pow(this->f1, z1) * pow(this->f2, z2);
		return f;
	};
	float vecToVoltage(ScaleVector v){
		float z1 = IntegerDet(v, this->v2) / this->det;
		float z2 = IntegerDet(this->v1, v) / this->det;
		float voltage = z1 * this->log2f1 + z2 * this->log2f2;
		return voltage;
	};
};


struct RegularScale {
	// a regular scale 

	ScaleVector scale_class = {1,1};
	int mode = 1; // 1=major
	int n = 2;
	RegularScale(ScaleVector scale_class, int mode){
		setScaleClass(scale_class);
		this->mode = mode;
		
	}
	void setScaleClass(ScaleVector scale_class){
		this->scale_class = scale_class;
		n = scale_class.x + scale_class.y;
	}
	ScaleVector scaleNoteSeqNrToCoord(int seqNr){
		int x = (int)(scale_class.x * seqNr - 1.f * mode / n - .5f);
		int y = (int)(scale_class.y * seqNr + 1.f * mode / n + .5f);
		return {x, y};
	}
	int coordToScaleNoteSeqNr(ScaleVector c){
		int d = c.x*scale_class.y - c.y*scale_class.x  + mode;
		return d<0? -1 : d>=n? -1 : (d+ n-mode ) % n;
	}
};