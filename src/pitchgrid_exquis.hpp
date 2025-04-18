#include <iomanip>

#include "pitchgrid.hpp"
#include "exquis.hpp"
#include "continuedFraction.hpp"

#include "hsluv.h"


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
	ConsistentTuning* tuning = NULL;

	bool needsRetune = false;

	std::string lastNotePlayedLabel = "";
	std::string lastNotePlayedNameLabel = "";


	enum ColorScheme {
		COLORSCHEME_SCALE_MONOCHROME = 0,
		COLORSCHEME_SCALE_COLOR_CIRCLE,
		NUM_COLORSCHEMES
	};
	ColorScheme colorScheme = COLORSCHEME_SCALE_COLOR_CIRCLE;

	PitchGridExquis(){
		Exquis();
		scaleMapper = ExquisScaleMapper();
		scaleMapper.scale.mode=5;
		showAllOctavesLayer();
	}

	void initialize(){
		sendEnterDevModeMessage();
		setNoteMidinoteValues();
		setNoteColors();
		resetTuningButton();
		resetArrangeButton();
		resetScaleSelectionButton();
		resetOctaveUpButton();
		resetOctaveDownButton();
	}	

	void showAllOctavesLayer(){
		if (!tuning){
			return;
		}
		for (ExquisNote& note : notes){

			note.scaleCoord = scaleMapper.exquis2scale(note.coord - scaleMapper.exquis_base);
			note.scaleSeqNr = scaleMapper.scale.coordToScaleNoteSeqNr(note.scaleCoord);
			float octave_fr = tuning ? tuning->vecToVoltageNoOffset(scaleMapper.scale.scale_system) - tuning->vecToVoltageNoOffset(ZERO_VECTOR) : 1.f;
			switch(colorScheme){
				case COLORSCHEME_SCALE_MONOCHROME:
					if (note.scaleSeqNr != -1){
						note.color = note.scaleSeqNr == 0 ? XQ_COLOR_EXQUIS_BLUE : XQ_COLOR_EXQUIS_YELLOW;
						float r = note.scaleCoord * scaleMapper.scale.scale_system;
						r /= scaleMapper.scale.scale_system * scaleMapper.scale.scale_system;
						r = posfmod(r+.001f, 1.f);
						note.brightness = note.scaleSeqNr == -1? 0.f : pow(6.f, - r);
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
						double r, g, b;
						float h;
						if (tuning){
							h = (tuning->vecToVoltageNoOffset(note.scaleCoord)) / octave_fr;
						}else{
							h = (float)(note.scaleCoord * scaleMapper.scale.scale_system) / (scaleMapper.scale.scale_system * scaleMapper.scale.scale_system);
						}
						h = 360.f * posfmod(h+.106f, 1.f);

						hsluv2rgb(
							(double)h,
							100.f,
							note.scaleCoord == ZERO_VECTOR ? 70.f: 30.f,
							&r, &g, &b
						);
						note.color = Color(127*r, 127*g, 127*b);

					}else{
						note.color = XQ_COLOR_BLACK;
						note.brightness = 0.f;
					}
					break;
				default:
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
			if (note.scaleCoord.x < 0 || note.scaleCoord.y < 0 || note.scaleCoord.x > scaleMapper.scale.scale_system.x || note.scaleCoord.y > scaleMapper.scale.scale_system.y){
				note.color = XQ_COLOR_BLACK;
				note.brightness = 0.f;
			}
			//else{
			//	if (note.color==XQ_COLOR_BLACK){
			//		note.color = XQ_COLOR_WHITE;
			//		note.brightness = .15f;
			//	}
			//}
		}
		needsNoteDisplayUpdate=true;
	}

	void updateKeyDisplay(){
		if (arrangeModeOn || tuningIntervalSelectionModeOn){
			showSingleOctaveLayer();
		}else{
			showAllOctavesLayer();
		}
	}

	ScaleVector scaleSystemForNote(ExquisNote* note){
		ScaleVector scaleSystem = ScaleVector(note->coord-scaleSelectModeBaseNode);
		scaleSystem = {scaleSystem.y, scaleSystem.x};
		return scaleSystem;
	}

	void showScaleSystemSelectLayer(){
		for (ExquisNote& note : notes){

			ScaleVector scaleSystem = scaleSystemForNote(&note);

			if (scaleSystem.x > 0 && scaleSystem.y > 0){
				// scale system selection area
				if (scaleMapper.scale.isCoprimeScaleVector(scaleSystem)){
					note.brightness = .7f;
					if (scaleSystem == scaleMapper.scale.scale_system){
						note.color = XQ_COLOR_EXQUIS_BLUE;
					}else if (scaleSystem == ScaleVector(2,5)){
						note.color = Color(127,45,0);
					}else {
						note.color = XQ_COLOR_EXQUIS_YELLOW;
					}
					if (scaleSystem == scaleMapper.scale.scale_system){
						selectedScaleNote.startWithNote(&note);
						// display needs update
					}
				}else{
					note.color = XQ_COLOR_BLACK;
					note.brightness = 0.f;
				}
				
			}else if (scaleSystem.x < 0){
				note.color = XQ_COLOR_BLACK;
				note.brightness = 0.f;
			}else if (scaleSystem.y < 0){
				note.color = XQ_COLOR_BLACK;
				note.brightness = 0.f;
			}else if (scaleSystem == ZERO_VECTOR){
				// origin
				note.color = XQ_COLOR_EXQUIS_BLUE;
				note.brightness = 1.f;
			}else{
				// axes
				note.color = XQ_COLOR_WHITE;
				note.brightness = .15f;
			}
				
		}
		needsNoteDisplayUpdate=true;
	}

	void stopTuningMode(){
		tuningModeOn = false;
		tuningConstantNoteSelected = false;
		tuningConstantNote.stop();
		tuningRetuneNote.stop();
		resetTuningButton();
	}

	void enterTuningIntervalSelectionMode(){
		stopTuningMode();
		showSingleOctaveLayer();
		activateTuningButton();
		tuningIntervalSelectionModeOn = true;
	}
	void exitTuningIntervalSelectionMode(){
		if (tuningModeOn){
			if (!tuningConstantNoteSelected){
				if (tuningModeRetuneInterval != ZERO_VECTOR && tuningModeRetuneInterval != scaleMapper.scale.scale_system){
					tuningConstantNoteSelected = true;
					tuningModeConstantInterval = scaleMapper.scale.scale_system;
					// find note for interval of equivalence (octave)
					ExquisNote* note = NULL;
					for (ExquisNote& n: notes){
						if (n.scaleCoord == scaleMapper.scale.scale_system){
							note = &n;
							break;
						}
					}
					if (note != NULL){
						tuningConstantNote.startWithNote(note);
						needsNoteDisplayUpdate=true;
					}
				}
			}
		}
		showMainLayer();
		resetTuningButton();
		tuningIntervalSelectionModeOn = false;
	}
	void enterArrangeMode(){
		stopTuningMode();
		showSingleOctaveLayer();
		activateArrangeButton();
		arrangeModeOn = true;
	}
	void exitArrangeMode(){
		showMainLayer();
		resetArrangeButton();
		arrangeModeOn = false;
	}
	void enterScaleSelectMode(){
		stopTuningMode();
		showScaleSystemSelectLayer();
		activateScaleSelectionButton();
		scaleSelectModeOn = true;
	}
	void exitScaleSelectMode(){
		showMainLayer();
		selectedScaleNote.stop();
		resetScaleSelectionButton();
		scaleSelectModeOn = false;
	}


	void activateTuningButton(){
		sendSetColorMessage(101, XQ_COLOR_EXQUIS_BLUE);
	}
	void resetTuningButton(){
		sendSetColorMessage(101, XQ_COLOR_EXQUIS_YELLOW_DIMMED);
	}
	void activateArrangeButton(){
		sendSetColorMessage(103, XQ_COLOR_EXQUIS_BLUE);
	}
	void resetArrangeButton(){
		sendSetColorMessage(103, XQ_COLOR_EXQUIS_YELLOW_DIMMED);
	}
	void activateScaleSelectionButton(){
		sendSetColorMessage(104, XQ_COLOR_EXQUIS_BLUE);
	}
	void resetScaleSelectionButton(){
		sendSetColorMessage(104, XQ_COLOR_EXQUIS_YELLOW_DIMMED);
	}
	void activateOctaveDownButton(){
		sendSetColorMessage(106, XQ_COLOR_EXQUIS_BLUE);
	}
	void resetOctaveDownButton(){
		sendSetColorMessage(106, XQ_COLOR_EXQUIS_YELLOW_DIMMED);
	}
	void activateOctaveUpButton(){
		sendSetColorMessage(107, XQ_COLOR_EXQUIS_BLUE);
	}
	void resetOctaveUpButton(){
		sendSetColorMessage(107, XQ_COLOR_EXQUIS_YELLOW_DIMMED);
	}


	void retuneIntervalByAmount(float amount){
		
		if (tuningModeRetuneInterval == ZERO_VECTOR){
			// change offset
			tuning->setOffset(tuning->Offset() + 0.001*amount);
		}else if (tuningModeRetuneInterval == scaleMapper.scale.scale_system && !tuningConstantNoteSelected){
			// proportional stretch tuning
			float logf1 = tuning->vecToVoltageNoOffset( scaleMapper.scale.scale_system );
			ScaleVector v2 = tuning->V1() == scaleMapper.scale.scale_system ? tuning->V2() : tuning->V1();
			float logf2 = tuning->V1() == scaleMapper.scale.scale_system ? tuning->Log2F2() : tuning->Log2F1();

			float new_logf1 = logf1 + amount/1200.f;
			float new_logf2 = logf2 + logf2/logf1 * amount/1200.f;

			tuning->setParams(scaleMapper.scale.scale_system, pow(2.f, new_logf1), v2, pow(2.f, new_logf2));

		}else{
			float retune_f = tuning->vecToFreqRatioNoOffset(tuningModeRetuneInterval);
			float constant_f = tuning->vecToFreqRatioNoOffset(tuningModeConstantInterval);

			tuning->setParams(tuningModeRetuneInterval, retune_f*pow(2.f, amount/1200.f), tuningModeConstantInterval, constant_f);
		}

		needsRetune = true;
		updateKeyDisplay();

	}

	void justifyTuning(){
		// TODO: check crash
		if (tuningModeOn && tuningModeRetuneInterval != ZERO_VECTOR){
			// tune selected note to a close just interval while keeping the other note constant
			float f = tuning->vecToFreqRatioNoOffset( tuningModeRetuneInterval );
			Fraction approx = closestRational(f, 5*scaleMapper.scale.n);

			if (!tuningConstantNoteSelected){
				ScaleVector v2 = tuning->V1() == tuningModeRetuneInterval ? tuning->V2() : tuning->V1();
				float f2 = tuning->V1() == tuningModeRetuneInterval ? tuning->F2() : tuning->F1();
				tuning->setParams(tuningModeRetuneInterval, approx.toFloat(), v2, f2);
			}else{
				tuning->setParams(tuningModeRetuneInterval, approx.toFloat(), tuningModeConstantInterval, tuning->vecToFreqRatioNoOffset(tuningModeConstantInterval));
			}
			needsRetune = true;

		}
	}
	void setTuning(){
		if (tuningModeOn && tuningModeRetuneInterval != ZERO_VECTOR){
			// set tuning base note to selected note
			float f = tuning->vecToFreqRatioNoOffset( tuningModeRetuneInterval );

			if (!tuningConstantNoteSelected){
				ScaleVector v2 = tuning->V1() == tuningModeRetuneInterval ? tuning->V2() : tuning->V1();
				float f2 = tuning->V1() == tuningModeRetuneInterval ? tuning->F2() : tuning->F1();
				tuning->setParams(tuningModeRetuneInterval, f, v2, f2);
			}else{
				tuning->setParams(tuningModeRetuneInterval, f, tuningModeConstantInterval, tuning->vecToFreqRatioNoOffset(tuningModeConstantInterval));
			}
			needsRetune = true;

		}
	}


	std::string contFracDisplay(float f){
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
		// react to cc messages on channel 16 (0xbf)
		if (msg.bytes.size() == 3 && msg.bytes[0] == 0xbf) {
			uint8_t controllerId = msg.bytes[1];
			uint8_t value = msg.bytes[2];
			
			switch (controllerId){
				// button press/release


				case 0x65: // tuning button
					if (value == 0x7F){
						enterTuningIntervalSelectionMode();
					}else{
						exitTuningIntervalSelectionMode();
					}
					break;
				case 0x67: // arrange button
					if (value == 0x7F){
						enterArrangeMode();
					}else{
						exitArrangeMode();
					}
					break;
				case 0x68: // scale select button
					if (value == 0x7F){
						enterScaleSelectMode();
					}else{
						exitScaleSelectMode();
					}
					break;
				case 0x6A: // octave down
					if (value == 0x7F){
						// apply offset equal to octave to tuning
						float octave_freq_ratio = tuning->vecToFreqRatioNoOffset(scaleMapper.scale.scale_system);
						float new_offset = tuning->Offset() - log2(octave_freq_ratio);
						tuning->setOffset(new_offset);
						activateOctaveDownButton();
						needsRetune = true;
					}else{
						resetOctaveDownButton();
					}
					break;
				case 0x6B: // octave up
					if (value == 0x7F){
						// apply offset equal to octave to tuning
						float octave_freq_ratio = tuning->vecToFreqRatioNoOffset(scaleMapper.scale.scale_system);
						float new_offset = tuning->Offset() + log2(octave_freq_ratio);
						tuning->setOffset(new_offset);
						activateOctaveUpButton();
						needsRetune = true;
					}else{
						resetOctaveUpButton();
					}
					break;
				case 0x72: // controller 1 press
					if (arrangeModeOn){
						if (value == 0x7F){
							scaleMapper.flipHorizontally();
							showSingleOctaveLayer();
							needsRetune = true;
						}
					} else if (tuningModeOn){
						if (value == 0x7F){
							justifyTuning();
						}
					}							
					break;
				case 0x73: // controller 2 press
					if (arrangeModeOn){
						if (value == 0x7F){
							scaleMapper.flipVertically();
							showSingleOctaveLayer();
							needsRetune = true;
						}
					} else if (tuningModeOn){
						if (value == 0x7F){
							setTuning();
						}
					}							
					break;
				case 0x74: // controller 3 press
					// change color scheme
					if (!scaleSelectModeOn){
						if (value == 0x7F){
							colorScheme = (ColorScheme)((colorScheme + 1) % NUM_COLORSCHEMES);
							updateKeyDisplay();
						}
					}
					break;
				case 0x75: // controller 4 press

					break;


				case 0x6E: // knob 1
					if (value < 0x40){
						// decrement
						if (arrangeModeOn){
							//scaleMapper.shiftY(-1);
							scaleMapper.rotatePlus60();
							showSingleOctaveLayer();
							needsRetune = true;
						}else if (tuningModeOn){
							retuneIntervalByAmount(-10.0*value/127);
						}
					}else{
						// increment
						if (arrangeModeOn){
							//scaleMapper.shiftY(1);
							scaleMapper.rotateMinus60();
							showSingleOctaveLayer();
							needsRetune = true;
						}else if (tuningModeOn){
							retuneIntervalByAmount(10.0*value/127);
						}
					}
					break;
				case 0x6F: // knob 2
					if (value < 0x40){
						if (arrangeModeOn){
							//scaleMapper.shiftX(1);
							scaleMapper.skewY(-1);
							showSingleOctaveLayer();
							needsRetune = true;
						}else if (tuningModeOn){
							retuneIntervalByAmount(-0.1*value/127);
						}
					}else{
						if (arrangeModeOn){
							//scaleMapper.shiftX(-1);
							scaleMapper.skewY(1);
							showSingleOctaveLayer();
							needsRetune = true;
						}else if (tuningModeOn){
							retuneIntervalByAmount(0.1*value/127);
						}
					}
					break;
				case 0x70: // knob 3
					break;
				case 0x71: // knob 4
					if (value < 0x40){
						if (!scaleSelectModeOn){
							scaleMapper.scale.mode = scaleMapper.scale.mode >0 ? scaleMapper.scale.mode - 1 : 0;
							updateKeyDisplay();
							needsRetune = true;
						}
					}else{
						if (!scaleSelectModeOn){
							scaleMapper.scale.mode = scaleMapper.scale.mode < scaleMapper.scale.n-1 ? scaleMapper.scale.mode + 1 : scaleMapper.scale.n-1;
							updateKeyDisplay();
							needsRetune = true;
						}
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
					needsNoteDisplayUpdate=true;
					tuningModeOn = true;
				} else if (note->scaleSeqNr>=0 && selectedInterval.x >= 0 && selectedInterval.y >= 0 && selectedInterval.x <= scaleMapper.scale.scale_system.x && selectedInterval.y <= scaleMapper.scale.scale_system.y){
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
					needsNoteDisplayUpdate=true;
				} // otherwise ignore
			}
		} else if (scaleSelectModeOn){
			// react to button presses in scale select mode
			// note on: select interval to tune
			if (msg.getStatus()==0x9){
				
				uint8_t noteId = msg.getNote();
				
				ExquisNote* note = getNoteByMidinote(noteId);
				ScaleVector scaleSystem = scaleSystemForNote(note);
				if (scaleSystem.x > 0 && scaleSystem.y > 0 && scaleMapper.scale.isCoprimeScaleVector(scaleSystem)){
					scaleMapper.scale.setScaleSystem(scaleSystem);
					selectedScaleNote.startWithNote(note);
					needsNoteDisplayUpdate=true;
					needsRetune = true;
				}
				showScaleSystemSelectLayer();
			}
		} else if (arrangeModeOn){
			// react to button presses in arrange mode
			// note on: select new base note
			if (msg.getStatus()==0x9){
				
				uint8_t noteId = msg.getNote();
				
				ExquisNote* note = getNoteByMidinote(noteId);
				scaleMapper.moveBaseTo(note->coord);
				showSingleOctaveLayer();
				needsRetune = true;
			}
		}
		if (msg.getStatus()==0x9){
			// note on
			uint8_t noteId = msg.getNote();
			ExquisNote* note = getNoteByMidinote(noteId);
			note->playing = true;

			if (tuning){
				lastNotePlayedNameLabel = scaleMapper.scale.canonicalNameForCoord(note->scaleCoord, tuning) + " (" + std::to_string(note->scaleCoord.y) + "," +  std::to_string(note->scaleCoord.x) + ")";
				std::stringstream ss1;
				float note_fr = tuning->vecToFreqRatioNoOffset(note->scaleCoord);
				ss1 << std::fixed << std::setprecision(1) << 1200*log2(note_fr) << "ct"
					<< " (" <<  contFracDisplay(note_fr) << ")";
				lastNotePlayedLabel = ss1.str();
			}
		}else if (msg.getStatus()==0x8){
			// note off
			uint8_t noteId = msg.getNote();
			ExquisNote* note = getNoteByMidinote(noteId);
			note->playing = false;
		}
	}


};

