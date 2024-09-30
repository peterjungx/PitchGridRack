#include <iomanip>

#include "pitchgrid.hpp"
#include "exquis.hpp"
#include "continuedFraction.hpp"

struct ExquisScaleMapper {
	RegularScale scale = RegularScale({2,5},1);
	ExquisVector exquis_base = {5,5}, exquis_interval1 = {8,7}, exquis_interval2={7,6};
	ScaleVector scale_interval1 = {2,5}, scale_interval2 = {1,3};
	IntegerMatrix transform_e2s;
	IntegerMatrix transform_s2e;

	IntegerMatrix rot_plus_60;
	IntegerMatrix rot_minus_60;
	IntegerMatrix horizontal_flip;
	IntegerMatrix vertical_flip;

	ExquisScaleMapper(){
		calcTransforms();
		rot_plus_60 = findTransform({1,1}, {1,0}, {0,1}, {1,1}); // Exquis Lattice rotation by 60 degrees
		rot_minus_60 = findTransform({1,0}, {1,1}, {1,1}, {0,1}); // Exquis Lattice rotation by -60 degrees
		vertical_flip = findTransform({0,1},{0,1},{2,1},{-2,-1});
		horizontal_flip = findTransform({0,1},{0,-1},{2,1},{2,1});
	}
	void calcTransforms(){
		transform_e2s = findTransform(exquis_interval1 - exquis_base, scale_interval1, exquis_interval2 - exquis_base, scale_interval2);
		transform_s2e = findTransform(scale_interval1, exquis_interval1 - exquis_base, scale_interval2, exquis_interval2 - exquis_base);


	}
	ScaleVector exquis2scale(ExquisVector c){
		return transform_e2s * c;
	}
	ExquisVector scale2exquis(ScaleVector c){
		return transform_s2e * c;
	}
	void shiftX(int amount){
		exquis_base.x += amount;
		exquis_interval1.x += amount;
		exquis_interval2.x += amount;
	}
	void shiftY(int amount){
		exquis_base.y += amount;
		exquis_interval1.y += amount;
		exquis_interval2.y += amount;
	}
	void moveBaseTo(ExquisVector new_base){
		ExquisVector shift = new_base - exquis_base;
		exquis_base = new_base;
		exquis_interval1 += shift;
		exquis_interval2 += shift;
	}
	void skewX(int amount){
		int x_shift = amount * (exquis_interval1.y - exquis_base.y);
		exquis_interval1.x += x_shift;
		
		x_shift = amount * (exquis_interval2.y - exquis_base.y);
		exquis_interval2.x += x_shift;

		calcTransforms();
	}
	void skewY(int amount){
		int y_shift = amount * (exquis_interval1.x - exquis_base.x);
		exquis_interval1.y += y_shift;
		
		y_shift = amount * (exquis_interval2.x - exquis_base.x);
		exquis_interval2.y += y_shift;

		calcTransforms();
	}
	void rotatePlus60(){
		exquis_interval1 = exquis_base + rot_plus_60 * (exquis_interval1 - exquis_base);
		exquis_interval2 = exquis_base + rot_plus_60 * (exquis_interval2 - exquis_base);
		calcTransforms();
	}
	void rotateMinus60(){
		exquis_interval1 = exquis_base + rot_minus_60 * (exquis_interval1 - exquis_base);
		exquis_interval2 = exquis_base + rot_minus_60 * (exquis_interval2 - exquis_base);
		calcTransforms();
	}
	void flipHorizontally(){
		exquis_interval1 = exquis_base + horizontal_flip * (exquis_interval1 - exquis_base);
		exquis_interval2 = exquis_base + horizontal_flip * (exquis_interval2 - exquis_base);
		calcTransforms();
	}
	void flipVertically(){
		exquis_interval1 = exquis_base + vertical_flip * (exquis_interval1 - exquis_base);
		exquis_interval2 = exquis_base + vertical_flip * (exquis_interval2 - exquis_base);
		calcTransforms();
	}

};

ScaleVector ZERO_VECTOR = {0,0};

template <typename T>
T sin2pi_pade_05_5_4(T x) {
	x -= 0.5f;
	return (T(-6.283185307) * x + T(33.19863968) * simd::pow(x, 3) - T(32.44191367) * simd::pow(x, 5))
	       / (1 + T(1.296008659) * simd::pow(x, 2) + T(0.7028072946) * simd::pow(x, 4));
}


float posfmod(float x, float y){
	return fmod(x+100*y, y);
}

struct PitchGridExquis: Exquis {

	bool tuningIntervalSelectionModeOn = false;
	bool tuningConstantNoteSelected = false;

	bool arrangeModeOn = false;
	bool scaleSelectModeOn = false;


	ExquisVector scaleSelectModeBaseNode = {0,2};
	
	ScaleVector tuningModeRetuneInterval = {0,0};
	ScaleVector tuningModeConstantInterval = {0,0};

	bool tuningModeOn = false;
	ExquisScaleMapper scaleMapper;
	ConsistentTuning* tuning;

	bool didManualRetune = false;

	enum ColorScheme {
		COLORSCHEME_SCALE_MONOCHROME = 0,
		COLORSCHEME_SCALE_COLOR_CIRCLE,
		NUM_COLORSCHEMES
	};
	ColorScheme colorScheme = COLORSCHEME_SCALE_MONOCHROME;

	PitchGridExquis(){
		Exquis();
		scaleMapper = ExquisScaleMapper();
		scaleMapper.scale.mode=5;
		showAllOctavesLayer();
	}

	void showAllOctavesLayer(){
		for (ExquisNote& note : notes){
			note.scaleCoord = scaleMapper.exquis2scale(note.coord - scaleMapper.exquis_base);
			note.scaleSeqNr = scaleMapper.scale.coordToScaleNoteSeqNr(note.scaleCoord);

			//r = r - floor(r);
			float r;
			switch(colorScheme){
				case COLORSCHEME_SCALE_MONOCHROME:
					if (note.scaleSeqNr != -1){
						note.color = note.scaleSeqNr == 0 ? XQ_COLOR_EXQUIS_BLUE : XQ_COLOR_EXQUIS_YELLOW;
						r = note.scaleCoord * scaleMapper.scale.scale_class;
						r /= scaleMapper.scale.scale_class * scaleMapper.scale.scale_class;
						r = posfmod(r+.001f, 1.f);
						note.brightness = note.scaleSeqNr == -1? 0.f : pow(8.f, - r);
						if (note.scaleSeqNr == 0 && note.scaleCoord != ZERO_VECTOR){
							note.brightness = .5f;
						}
					}else{
						note.color = XQ_COLOR_BLACK;
						note.brightness = 0.f;
					}
					break;

				case COLORSCHEME_SCALE_COLOR_CIRCLE:
					if (note.scaleSeqNr != -1){
						note.brightness = 1.f;
						r = note.scaleCoord * scaleMapper.scale.scale_class;
						r /= scaleMapper.scale.scale_class * scaleMapper.scale.scale_class;
						note.color = {
							(uint8_t)(63+63*sin2pi_pade_05_5_4(posfmod(r+.25f, 1.f))),
							(uint8_t)(32+32*sin2pi_pade_05_5_4(posfmod(r+.5f, 1.f))),
							(uint8_t)(63+63*sin2pi_pade_05_5_4(posfmod(r+.75f, 1.f)))
						};
					}else{
						note.color = XQ_COLOR_BLACK;
						note.brightness = 0.f;
					}
					break;
				
			}
		}
		needsNoteDisplayUpdate = true;
	}	


	void showMainLayer(){
		showAllOctavesLayer();
	}

	void showSingleOctaveLayer(){
		showAllOctavesLayer();
		for (ExquisNote& note : notes){
			if (note.scaleCoord.x < 0 || note.scaleCoord.y < 0 || note.scaleCoord.x > scaleMapper.scale.scale_class.x || note.scaleCoord.y > scaleMapper.scale.scale_class.y){
				note.color = XQ_COLOR_BLACK;
				note.brightness = 0.f;
			}else{
				if (note.color==XQ_COLOR_BLACK){
					note.color = XQ_COLOR_EXQUIS_BLUE;
					note.brightness = .2f;
				}
			}
		}
		needsNoteDisplayUpdate=true;
	}

	ScaleVector scaleClassForNote(ExquisNote* note){
		ScaleVector scaleClass = ScaleVector(note->coord-scaleSelectModeBaseNode);
		scaleClass = {scaleClass.y, scaleClass.x};
		return scaleClass;
	}

	void showScaleClassSelectLayer(){
		for (ExquisNote& note : notes){

			ScaleVector scaleClass = scaleClassForNote(&note);

			if (scaleClass.x > 0 && scaleClass.y > 0){
				// scale class selection area
				if (scaleMapper.scale.isCoprimeScaleVector(scaleClass)){
					note.brightness = .7f;
					if (scaleClass == scaleMapper.scale.scale_class){
						note.color = XQ_COLOR_EXQUIS_BLUE;
					}else if (scaleClass == ScaleVector(2,5)){
						note.color = Color(127,45,0);
					}else {
						note.color = XQ_COLOR_EXQUIS_YELLOW;
					}
					if (scaleClass == scaleMapper.scale.scale_class){
						selectedScaleNote.startWithNote(&note);
					}
				}else{
					note.color = XQ_COLOR_BLACK;
					note.brightness = 0.f;
				}
				
			}else if (scaleClass.x < 0){
				note.color = XQ_COLOR_BLACK;
				note.brightness = 0.f;
			}else if (scaleClass.y < 0){
				note.color = XQ_COLOR_BLACK;
				note.brightness = 0.f;
			}else if (scaleClass == ZERO_VECTOR){
				// origin
				note.color = XQ_COLOR_EXQUIS_BLUE;
				note.brightness = 1.f;
			}else{
				// axes
				note.color = XQ_COLOR_EXQUIS_BLUE;
				note.brightness = .2f;
			}
				
		}
		needsNoteDisplayUpdate=true;
	}

	void stopTuningMode(){
		tuningModeOn = false;
		tuningConstantNoteSelected = false;
		tuningConstantNote.stop();
		tuningRetuneNote.stop();
	}

	void enterTuningIntervalSelectionMode(){
		stopTuningMode();
		showSingleOctaveLayer();
		tuningIntervalSelectionModeOn = true;
	}
	void exitTuningIntervalSelectionMode(){
		if (tuningModeOn){
			if (!tuningConstantNoteSelected){
				if (tuningModeRetuneInterval != ZERO_VECTOR && tuningModeRetuneInterval != scaleMapper.scale.scale_class){
					tuningConstantNoteSelected = true;
					tuningModeConstantInterval = scaleMapper.scale.scale_class;
					// find note for interval of equivalence (octave)
					ExquisNote* note = NULL;
					for (ExquisNote& n: notes){
						if (n.scaleCoord == scaleMapper.scale.scale_class){
							note = &n;
							break;
						}
					}
					if (note != NULL){
						tuningConstantNote.startWithNote(note);
					}
				}
			}
		}
		showMainLayer();
		tuningIntervalSelectionModeOn = false;
	}
	void enterArrangeMode(){
		stopTuningMode();
		showSingleOctaveLayer();
		arrangeModeOn = true;
	}
	void exitArrangeMode(){
		showMainLayer();
		arrangeModeOn = false;
	}
	void enterScaleSelectMode(){
		stopTuningMode();
		showScaleClassSelectLayer();
		scaleSelectModeOn = true;
	}
	void exitScaleSelectMode(){
		showMainLayer();
		selectedScaleNote.stop();
		scaleSelectModeOn = false;
	}


	void retuneIntervalByAmount(float amount){
		
		if (tuningModeRetuneInterval == ZERO_VECTOR){
			// change offset
			tuning->setOffset(tuning->Offset() + 0.001*amount);
		}else if (tuningModeRetuneInterval == scaleMapper.scale.scale_class && !tuningConstantNoteSelected){
			// proportional stretch tuning
			float f1 = tuning->vecToFreqRatioNoOffset( scaleMapper.scale.scale_class );
			ScaleVector v2 = tuning->V1() == scaleMapper.scale.scale_class ? tuning->V2() : tuning->V1();
			float f2 = tuning->V1() == scaleMapper.scale.scale_class ? tuning->F2() : tuning->F1();

			tuning->setParams(scaleMapper.scale.scale_class, f1*pow(2.f, amount/1200.f), v2, f2*pow(2.f, amount/1200.f));

		}else{
			float retune_f = tuning->vecToFreqRatioNoOffset(tuningModeRetuneInterval);
			float constant_f = tuning->vecToFreqRatioNoOffset(tuningModeConstantInterval);

			tuning->setParams(tuningModeRetuneInterval, retune_f*pow(2.f, amount/1200.f), tuningModeConstantInterval, constant_f);
		}

		didManualRetune = true;
	}

	void justifyTuning(){
		if (tuningModeOn && tuningModeRetuneInterval != ZERO_VECTOR){
			// tune selected note to a close just interval while keeping the other note constant
			float f = tuning->vecToFreqRatioNoOffset( tuningModeRetuneInterval );
			Fraction approx = closestRational(f, 5*scaleMapper.scale.n);
			INFO("retune from %f to  %d/%d (%f)", f, approx.numerator, approx.denominator, approx.toFloat());

			if (!tuningConstantNoteSelected){
				ScaleVector v2 = tuning->V1() == tuningModeRetuneInterval ? tuning->V2() : tuning->V1();
				float f2 = tuning->V1() == tuningModeRetuneInterval ? tuning->F2() : tuning->F1();
				tuning->setParams(tuningModeRetuneInterval, approx.toFloat(), v2, f2);
			}else{
				tuning->setParams(tuningModeRetuneInterval, approx.toFloat(), tuningModeConstantInterval, tuning->vecToFreqRatioNoOffset(tuningModeConstantInterval));
			}

			

			didManualRetune = true;

		}
	}
	void setTuning(){
		if (tuningModeOn && tuningConstantNoteSelected){
			// tune selected note to a close just interval while keeping the other note constant
			float f = tuning->vecToFreqRatioNoOffset( tuningModeRetuneInterval );
			tuning->setParams(tuningModeRetuneInterval, f, tuningModeConstantInterval, tuning->vecToFreqRatioNoOffset(tuningModeConstantInterval));

			didManualRetune = true;

		}
	}


	std::string contFracDisplay(float f){
		INFO("contFracDisplay %f", f);
		Fraction approx = closestRational(f, 5*scaleMapper.scale.n);
		std::stringstream ss;
		ss << approx.numerator << "/" << approx.denominator;
		float error_ct = 1200*log2(f / approx.toFloat());
		if (fabs(error_ct)>0.1){
			if (error_ct > 0) ss << "+";
			ss << std::fixed << std::setprecision(1) << error_ct << "ct";
		}
		return ss.str();
	}


	void processMidiMessage(midi::Message msg) override {
		// control sysex messages 
		if (msg.bytes.size() == 8 && msg.bytes[0] == 0xf0 && msg.bytes[1] == 0x00 && msg.bytes[2] == 0x21 && msg.bytes[3] == 0x7e && msg.bytes[7] == 0xf7) {
			uint8_t messageType = msg.bytes[4];
			uint8_t controllerId = msg.bytes[5];
			uint8_t value = msg.bytes[6];
			switch (messageType){
				case 0x08: // button press/release
					switch (controllerId){
						case 1: // tuning button
							if (value == 1){
								enterTuningIntervalSelectionMode();
							}else{
								exitTuningIntervalSelectionMode();
							}
							break;
						case 3: // arrange button
							if (value == 1){
								enterArrangeMode();
							}else{
								exitArrangeMode();
							}
							break;
						case 4: // scale select button
							if (value == 1){
								enterScaleSelectMode();
							}else{
								exitScaleSelectMode();
							}
							break;
						case 6: // octave down
							if (value == 1){
								// apply offset equal to octave to tuning
								float octave_freq_ratio = tuning->vecToFreqRatioNoOffset(scaleMapper.scale.scale_class);
								float new_offset = tuning->Offset() - log2(octave_freq_ratio);
								tuning->setOffset(new_offset);
							}
							break;
						case 7: // octave up
							if (value == 1){
								// apply offset equal to octave to tuning
								float octave_freq_ratio = tuning->vecToFreqRatioNoOffset(scaleMapper.scale.scale_class);
								float new_offset = tuning->Offset() + log2(octave_freq_ratio);
								tuning->setOffset(new_offset);
							}
							break;
						case 10: // controller 1 press
							if (arrangeModeOn){
								if (value == 1){
									scaleMapper.flipHorizontally();
									showSingleOctaveLayer();
								}
							} else if (tuningModeOn){
								if (value == 1){
									justifyTuning();
								}
							}							
							break;
						case 11: // controller 2 press
							if (arrangeModeOn){
								if (value == 1){
									scaleMapper.flipVertically();
									showSingleOctaveLayer();
								}
							} else if (tuningModeOn){
								if (value == 1){
									setTuning();
								}
							}							
							break;
						case 12: // controller 3 press

							break;
						case 13: // controller 4 press
							// change color scheme
							if (!scaleSelectModeOn){
								if (value == 1){
									colorScheme = (ColorScheme)((colorScheme + 1) % NUM_COLORSCHEMES);
									if (arrangeModeOn || tuningIntervalSelectionModeOn){
										showSingleOctaveLayer();
									}else{
										showAllOctavesLayer();
									}
								}
							}
							break;
					}
					break;
				case 0x0a: // rotary decrement
					switch(controllerId){
						case 0:
							if (arrangeModeOn){
								//scaleMapper.shiftY(-1);
								scaleMapper.rotatePlus60();
								showSingleOctaveLayer();
							}else if (tuningModeOn){
								retuneIntervalByAmount(-10.0*value);
							}
							break;
						case 1:
							if (arrangeModeOn){
								//scaleMapper.shiftX(1);
								scaleMapper.skewY(-1);
								showSingleOctaveLayer();
							}else if (tuningModeOn){
								retuneIntervalByAmount(-0.1*value);
							}
							break;
						case 2:
							break;
						case 3:
							if (!scaleSelectModeOn){
								scaleMapper.scale.mode = scaleMapper.scale.mode >0 ? scaleMapper.scale.mode - 1 : 0;
								if (arrangeModeOn || tuningIntervalSelectionModeOn){
									showSingleOctaveLayer();
								}else{
									showAllOctavesLayer();
								}
							}
							break;
					}
					break;
				case 0x0b: // rotary increment
					switch(controllerId){
						case 0:
							if (arrangeModeOn){
								//scaleMapper.shiftY(1);
								scaleMapper.rotateMinus60();
								showSingleOctaveLayer();
							}else if (tuningModeOn){
								retuneIntervalByAmount(10.0*value);
							}
							break;
						case 1:
							if (arrangeModeOn){
								//scaleMapper.shiftX(-1);
								scaleMapper.skewY(1);
								showSingleOctaveLayer();
							}else if (tuningModeOn){
								retuneIntervalByAmount(0.1*value);
							}
							break;
						case 2:
							break;
						case 3:
							if (!scaleSelectModeOn){
								scaleMapper.scale.mode = scaleMapper.scale.mode < scaleMapper.scale.n-1 ? scaleMapper.scale.mode + 1 : scaleMapper.scale.n-1;
								if (arrangeModeOn || tuningIntervalSelectionModeOn){
									showSingleOctaveLayer();
								}else{
									showAllOctavesLayer();
								}
							}
							break;
					}
					break;
				default:
					break;
			}
		}
		// react to button presses in tuning mode
		if (tuningIntervalSelectionModeOn){
			// note on: select interval to tune
			if (msg.getStatus()==0x9){
				
				uint8_t noteId = msg.getNote();
				
				ExquisNote* note = getNoteByMidinote(noteId);
				IntegerVector selectedInterval = note->scaleCoord;
				if (selectedInterval == ZERO_VECTOR){
					tuningConstantNoteSelected = false;
					tuningConstantNote.stop();
					tuningModeRetuneInterval = selectedInterval;
					tuningRetuneNote.startWithNote(note);
					tuningModeOn = true;
				} else if (note->scaleSeqNr>=0 && selectedInterval.x >= 0 && selectedInterval.y >= 0 && selectedInterval.x <= scaleMapper.scale.scale_class.x && selectedInterval.y <= scaleMapper.scale.scale_class.y){
					if (tuningConstantNoteSelected){
						tuningConstantNoteSelected = false;
						tuningConstantNote.stop();
						tuningModeOn = false;
						tuningRetuneNote.stop();
					}
					if (tuningModeOn){
						if (tuningModeRetuneInterval != ZERO_VECTOR){
							// check that selected intervals are not linearly dependent
							if (tuningModeRetuneInterval.x * selectedInterval.y - tuningModeRetuneInterval.y * selectedInterval.x != 0){
								tuningConstantNoteSelected = true;
								tuningModeConstantInterval = selectedInterval;
								tuningConstantNote.startWithNote(note);
							}
						}else{
							tuningModeRetuneInterval = selectedInterval;
							tuningRetuneNote.startWithNote(note);
						}
					}else{
						tuningConstantNoteSelected = false;
						tuningConstantNote.stop();
						tuningModeRetuneInterval = selectedInterval;
						tuningRetuneNote.startWithNote(note);
						tuningModeOn = true;
					}
				} // otherwise ignore
			}
		} else if (scaleSelectModeOn){
			// react to button presses in scale select mode
			// note on: select interval to tune
			if (msg.getStatus()==0x9){
				
				uint8_t noteId = msg.getNote();
				
				ExquisNote* note = getNoteByMidinote(noteId);
				ScaleVector scaleClass = scaleClassForNote(note);
				if (scaleClass.x > 0 && scaleClass.y > 0 && scaleMapper.scale.isCoprimeScaleVector(scaleClass)){
					scaleMapper.scale.setScaleClass(scaleClass);
					selectedScaleNote.startWithNote(note);
				}
				showScaleClassSelectLayer();
			}
		} else if (arrangeModeOn){
			// react to button presses in arrange mode
			// note on: select new base note
			if (msg.getStatus()==0x9){
				
				uint8_t noteId = msg.getNote();
				
				ExquisNote* note = getNoteByMidinote(noteId);
				scaleMapper.moveBaseTo(note->coord);
				showSingleOctaveLayer();
			}
		}
		if (msg.getStatus()==0x9){
			// note on
			uint8_t noteId = msg.getNote();
			ExquisNote* note = getNoteByMidinote(noteId);
			note->playing = true;
		}else if (msg.getStatus()==0x8){
			// note off
			uint8_t noteId = msg.getNote();
			ExquisNote* note = getNoteByMidinote(noteId);
			note->playing = false;
		}
		
	}


};

