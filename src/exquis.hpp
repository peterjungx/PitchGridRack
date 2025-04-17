#include <rack.hpp>
using namespace rack;

#include "integer_linalg.hpp"

typedef IntegerVector ExquisVector;


struct Color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	Color(){
		this->r = 0;
		this->g = 0;
		this->b = 0;
	}
	Color(uint8_t r, uint8_t g, uint8_t b){
		this->r = r;
		this->g = g;
		this->b = b;
	}
	Color fullBrightnessColor(){
		if (r == 0 && g == 0 && b == 0){
			return {0,0,0};
		}
		uint8_t m = r>g ? r>b ? r : b : g>b ? g : b;
		float scale_factor = 127.f / m;
		
		return {uint8_t(r * scale_factor),uint8_t(g * scale_factor),uint8_t(b * scale_factor)};
	}
	void toFullBrightness(){
		Color c = fullBrightnessColor();
		r = c.r;
		g = c.g;
		b = c.b;
	}
	Color operator*(float brightness){
		return {uint8_t(r*brightness), uint8_t(g*brightness), uint8_t(b*brightness)};
	}
	bool operator==(const Color& c) const {
		return r == c.r && g == c.g && b == c.b;
	}
};

Color XQ_COLOR_WHITE = {127,127,127};
Color XQ_COLOR_RED = {127,0,0};
Color XQ_COLOR_GREEN = {0,127,0};
Color XQ_COLOR_BLUE = {0,0,127};
Color XQ_COLOR_YELLOW = {127,127,0};
Color XQ_COLOR_CYAN = {0,127,127};
Color XQ_COLOR_MAGENTA = {127,0,127};
Color XQ_COLOR_BLACK = {0,0,0};

Color XQ_COLOR_EXQUIS_YELLOW = {127,75,5};
Color XQ_COLOR_EXQUIS_YELLOW_DIMMED = {63,37,2};
Color XQ_COLOR_EXQUIS_BLUE = {14,113,127};
Color XQ_COLOR_EXQUIS_BLUE_DIMMED = {7,56,85};


struct ExquisNote {
	uint8_t noteId;
	Color color;
	float brightness;
	ExquisVector coord;
	ExquisVector scaleCoord;
	uint8_t midinote;
	int8_t scaleSeqNr;
	Color shownColor;
	Color functionColor;
	bool functionEnabled = false;
	bool playing = false;
	ExquisNote(uint8_t noteId, Color color){
		this->noteId = noteId;
		this->color = color;
		this->coord = idToCoord();
		this->midinote = 36 + noteId;
		this->scaleSeqNr = -1;
		this->brightness = 1.f;
	}
	std::string canonicalName(){
		return std::to_string(scaleSeqNr + 1);
	}

	ExquisVector idToCoord(){
		ExquisVector c;
		int u = noteId % 11;
		int v = noteId / 11;
		c.y = v + u%6 + u/6;
		c.x = 2*v + u/6;
		return c;
	}
	uint8_t coordToId(){
		return uint8_t(coord.y + 6*coord.x - 2*(coord.x/2) - coord.x%2);
	}
	//void sendSetMidinoteMessage(midi::Output* midi_output){
	//	midi::Message msg;
	//	// F0 00 21 7E 04 noteId midinote F7
	//	msg.bytes = {0xf0, 0x00, 0x21, 0x7e, 0x04, noteId, midinote, 0xf7};
	//	midi_output->sendMessage(msg);
	//}
	void sendSetColorMessage(midi::Output* midi_output, Color color){
		midi::Message msg;
		// `F0 00 21 7E 7F 04 start_id color(0) [... color(N)] F7`
		shownColor = color;
		msg.bytes = {0xF0, 0x00, 0x21, 0x7E, 0x7F, 0x04, noteId, color.r, color.g, color.b, 0x00, 0xF7};
		midi_output->sendMessage(msg);
	}

	void sendSetBreathingBrightMessage(midi::Output* midi_output){
		midi::Message msg;
		msg.bytes = {0xAF, noteId, 0x7F};
		midi_output->sendMessage(msg);
	}
	void sendSetBreathingDarkMessage(midi::Output* midi_output){
		midi::Message msg;
		msg.bytes = {0xAF, noteId, 0x3F};
		midi_output->sendMessage(msg);
	}
	void sendSetBreathingOffMessage(midi::Output* midi_output){
		midi::Message msg;
		msg.bytes = {0xAF, noteId, 0x00};
		midi_output->sendMessage(msg);
	}
};

struct Exquis;

struct ExquisBreathingNote {
    ExquisNote* note = NULL;
    midi::Output* midi_output;
    bool bright = false;
    bool active = false;
	dsp::ClockDivider clockDivider;
	float period = 1.f;
	float phase = 0.f;

    ExquisBreathingNote(){
		this->note = NULL;
		this->midi_output = NULL;
		this->bright = false;
		this->active = false;
		this->clockDivider.setDivision(48000);

    }
    void config(midi::Output* midi_output, bool bright){
        this->midi_output = midi_output;
		this->bright = bright;
    }

    void process(){
		
        if (!active || note == NULL){
            return;
        }
        if (clockDivider.process()){
            phase += 1.f / period;
            if (phase > 1.f){
                phase -= 1.f;
            }
            float brightness = fabs(2.f * phase - 1.f);
            //note->sendSetColorMessage(midi_output, note->color * brightness);
			
            
        }
    }

    void startWithNote(ExquisNote* note){
        if (active){
            stop();
        }
        this->note = note;
        active = true;
		if (bright){
			note->functionColor = XQ_COLOR_WHITE;
			note->functionEnabled = true;
			//note->sendSetBreathingBrightMessage(midi_output);
		}else{
			note->functionColor = XQ_COLOR_RED;
			note->functionEnabled = true;
			//note->sendSetBreathingDarkMessage(midi_output);
		}
		// needs update


    }
    void stop(){
        if(note != NULL){
			// set back
			//note->shownColor = note->color * note->brightness;
			//note->sendSetBreathingOffMessage(midi_output);
			note->functionEnabled = false;
        }
        active = false;
        note = NULL;
    }
};


struct Exquis {
	std::vector<ExquisNote> notes;
	midi::Output midi_output;
	midi::InputQueue midi_input;
	bool connected = false;
    bool needsNoteDisplayUpdate = true;
    bool needsMidinoteValuesUpdate = true;

    dsp::ClockDivider xqNoteDisplayUpdateClock;

    ExquisBreathingNote tuningRetuneNote;
    ExquisBreathingNote tuningConstantNote;
    ExquisBreathingNote selectedScaleNote;

	
	Exquis(){
		for (int noteId = 0; noteId < 61; noteId++){
			ExquisNote note = ExquisNote(noteId, {0,0,0});
			notes.push_back(note);
		}
        xqNoteDisplayUpdateClock.setDivision(4800); // 10FPS at 48kHz

        tuningRetuneNote.config(&midi_output, true);
        tuningConstantNote.config(&midi_output, false);
        selectedScaleNote.config(&midi_output, true);
	}

    virtual void processMidiMessage(midi::Message msg){
        // override this
    }

    void process(const rack::engine::Module::ProcessArgs& args){
        midi::Message msg;
        while (midi_input.tryPop(&msg, args.frame)) {
            processMidiMessage(msg);
        }

        if (xqNoteDisplayUpdateClock.process()){
            if (needsNoteDisplayUpdate){
                needsNoteDisplayUpdate = false;
                setNoteColors();
            }
        }
        if (tuningRetuneNote.active){
            tuningRetuneNote.process();
        }
        if (tuningConstantNote.active){
            tuningConstantNote.process();
        }
        if (selectedScaleNote.active){
            selectedScaleNote.process();
        }

    }

	void setNoteColors(){
        if (!connected){
            needsNoteDisplayUpdate = true;
            //return;
        }
		sendCustomSnapshotMessage();
		//for (ExquisNote& note : notes){
        //    note.sendSetColorMessage(&midi_output, note.color * note.brightness);
		//}
	}
	void setNoteMidinoteValues(){
        if (!connected){
            needsMidinoteValuesUpdate = true;
            return;
        }
		sendCustomSnapshotMessage();
	}

	void sendCustomSnapshotMessage(){
	
		// 17 + 61 * 4 + 1 = 262
		uint8_t msg[262];
		// header (17 bytes):  F0 00 21 7E 7F 09 00 01 01 0E 00 00 01 01 00 00 00

		// Meaning
		//                                                                     |PBRange
		//                                                                     |
		uint8_t msg_start[] = {0xF0, 0x00, 0x21, 0x7E, 0x7F, 0x09, 0x00, 0x01, 0x00, 0x0E, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00};
		memcpy(msg, msg_start, sizeof(msg_start));
		for (int i = 0; i < 61; i++){
			ExquisNote* note = &notes[i];
			msg[17 + 4*i] = note->midinote;
			note->shownColor = note->functionEnabled ? note->functionColor : (note->color * note->brightness);
			msg[18 + 4*i] = note->shownColor.r;
			msg[19 + 4*i] = note->shownColor.g;
			msg[20 + 4*i] = note->shownColor.b;
			
		}
		// end byte: F7
		msg[261] = 0xF7;
		
		midi::Message midi_msg;
		//midi_msg.bytes.resize(262);
		//memcpy(midi_msg.bytes.data(), msg, 262);

		midi_msg.bytes = std::vector<uint8_t>(msg, msg + 262);
		midi_output.sendMessage(midi_msg);
	}

	void checkConnection(){
		for (int driver_id : midi::getDriverIds()){
			midi::Driver* driver = midi::getDriver(driver_id);
			for (int device_id : driver->getOutputDeviceIds()){
				if (driver->getOutputDeviceName(device_id).rfind( "Exquis", 0)==0){
					midi_output.setDriverId(driver_id);
					midi_output.setDeviceId(device_id);
				}
			}
			for (int device_id : driver->getInputDeviceIds()){
				if (driver->getInputDeviceName(device_id).rfind( "Exquis", 0)==0){
					midi_input.setDriverId(driver_id);
					midi_input.setDeviceId(device_id);
					connected = true;
					return;
				}
			}
		}
		connected = false;
	}
	//void sendKeepaliveMessage() {
	//	midi::Message msg;
	//	// F0 00 21 7E F7
	//	msg.bytes = {0xf0, 0x00, 0x21, 0x7e, 0xf7};
	//	midi_output.sendMessage(msg);
	//}
	void sendEnterDevModeMessage(){
		midi::Message msg;
		msg.bytes = {0xF0, 0x00, 0x21, 0x7E, 0x7F, 0x00, 0x3A, 0xF7}; // 0x3A is a bitmask, 0x02 pads, 0x08 up/down buttons, 0x10 settings&sound buttons, 0x20 other buttons
		midi_output.sendMessage(msg);
	}
	void sendExitDevModeMessage(){
		midi::Message msg;
		msg.bytes = {0xF0, 0x00, 0x21, 0x7E, 0x7F, 0x00, 0x00, 0xF7};
		midi_output.sendMessage(msg);
	}
	void sendSetColorMessage(uint8_t noteId, Color color){
		midi::Message msg;
		msg.bytes = {0xF0, 0x00, 0x21, 0x7E, 0x7F, 0x04, noteId, color.r, color.g, color.b, 0x00, 0xF7};
		midi_output.sendMessage(msg);
	}


	ExquisNote* getNoteByMidinote(uint8_t midinote){
		return &notes[midinote - 36];
	}
	ExquisNote* getNoteByVoltage(float voltage){
		// expect V in [-5v, +5v], map 1V/octave and 0v = middle C (C4=midinote 60)
		uint8_t midinote = (uint8_t)(12*(voltage+5.f) + 0.5f);
		return getNoteByMidinote(midinote);
	}

};


