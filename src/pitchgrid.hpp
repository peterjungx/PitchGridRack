#pragma once
#include <rack.hpp>
using namespace rack;

#include "integer_linalg.hpp"

typedef IntegerVector ScaleVector;

class ConsistentTuning {
	ScaleVector v1, v2;
	float f1, log2f1;
	float f2, log2f2;
	float det;
	float offset = 0.f;
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
		//this->offset = 0.f;
	};
	float vecToFreqRatio(ScaleVector v){
		float z1 = IntegerDet(v, this->v2) / this->det;
		float z2 = IntegerDet(this->v1, v) / this->det;
		float f = pow(this->f1, z1) * pow(this->f2, z2) * pow(2.f, offset);
		return f;
	};
	float vecToFreqRatioNoOffset(ScaleVector v){
		float z1 = IntegerDet(v, this->v2) / this->det;
		float z2 = IntegerDet(this->v1, v) / this->det;
		float f = pow(this->f1, z1) * pow(this->f2, z2);
		return f;
	};
	float vecToVoltage(ScaleVector v){
		float z1 = IntegerDet(v, this->v2) / this->det;
		float z2 = IntegerDet(this->v1, v) / this->det;
		float voltage = z1 * this->log2f1 + z2 * this->log2f2 + offset;
		return voltage;
	};
	float vecToVoltageNoOffset(ScaleVector v){
		float z1 = IntegerDet(v, this->v2) / this->det;
		float z2 = IntegerDet(this->v1, v) / this->det;
		float voltage = z1 * this->log2f1 + z2 * this->log2f2;
		return voltage;
	};
	ScaleVector V1(){
		return v1;
	};
	ScaleVector V2(){
		return v2;
	};
	float F1(){
		return f1;
	};
	float F2(){
		return f2;
	};
	float Log2F1(){
		return log2f1;
	};
	float Log2F2(){
		return log2f2;
	};
	float Offset(){
		return offset;
	};
	float OffsetAsStandardFreq(){
		return 440.0 * pow(2.f, -9./12.) * pow(2.f, offset); // 0 Volt = C4
	};
	void setOffset(float offset){
		//std::lock_guard<std::mutex> guard(consistent_tuning_offset_mutex);
		this->offset = offset;
	};
};

int IntegerGCD(int a, int b);
int inverseModulo(int a, int b);


struct RegularScale {
	// a regular scale 

	ScaleVector scale_class = {1,1};
	int mode = 1; // 1=major
	int n = 2;
	int inverse_of_x = 1;
	RegularScale(ScaleVector scale_class, int mode){
		setScaleClass(scale_class);
		this->mode = mode;
		
	}
	void setScaleClass(ScaleVector scale_class){
		this->scale_class = scale_class;
		n = scale_class.x + scale_class.y;
		if (mode>=n){
			mode = n-1;
		}
		inverse_of_x = inverseModulo(scale_class.x, n);
	}
	ScaleVector scaleNoteSeqNrToCoord(int seqNr){
		int x = (int)(scale_class.x * seqNr - 1.f * (n-(mode+1)) / n - .5f);
		int y = (int)(scale_class.y * seqNr + 1.f * (n-(mode+1)) / n + .5f);
		return {x, y};
	}
	int coordToScaleNoteSeqNr(ScaleVector c){
		int d = c.x * scale_class.y - c.y * scale_class.x + (n-(mode+1)) ;
		return d<0 ? -1 : d>=n ? -1 : (d + mode + 1) % n;
	}


	bool isCoprimeScaleVector(ScaleVector v){
		return IntegerGCD(v.x, v.y) == 1;
	}

	std::string canonicalNameForCoord(ScaleVector c){
		int d = -(c.x * scale_class.y - c.y * scale_class.x);
		int diatonic_note = (inverse_of_x*d+100*n)%n+1;
		//int diatonic_note = coordToScaleNoteSeqNr(c) + 1;
		int accidentals = (d+1+100*n)/n-100;


		std::string result = "";
		while(accidentals>0){
			//result += "#";
			result += "\u266F";
			accidentals--;
		}
		while(accidentals<0){
			result += "\u266D";
			accidentals++;
		}
		result += std::to_string(diatonic_note);
		return result;
	}


};

/*

struct RegularSubScale {
	RegularScale base_scale = RegularScale({2,5}, 2);
	std::vector<ScaleVector> scale_note_coords;

	RegularSubScale(ScaleVector scale_class, int mode){
		base_scale.setScaleClass(scale_class);
		base_scale.mode = mode;
		setScaleNoteCoords();
	}
	RegularSubScale(ScaleVector scale_class, int mode, std::vector<ScaleVector> scale_note_offsets){
		base_scale.setScaleClass(scale_class);
		base_scale.mode = mode;
		ScaleVector last = {0,0};
		for (ScaleVector c : scale_note_offsets){
			last = last + c;
			scale_note_coords.push_back(last);
		}
	}


	
	void setScaleNoteCoords(){
		// empty the vector
		scale_note_coords.clear();
		for (int i=1; i<=base_scale.n; i++){
			scale_note_coords.push_back(base_scale.scaleNoteSeqNrToCoord(i));
		}
	}

	std::vector<ScaleVector> getScaleNoteOffsets(){
		std::vector<ScaleVector> offsets;
		ScaleVector last = {0,0};
		for (ScaleVector c : scale_note_coords){
			offsets.push_back(c - last);
			last = c;
		}
		return offsets;
	}

};
*/



