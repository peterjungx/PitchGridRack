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
Color XQ_COLOR_EXQUIS_BLUE = {14,113,127};


struct ExquisNote {
	uint8_t noteId;
	Color color;
	float brightness;
	ExquisVector coord;
	ExquisVector scaleCoord;
	uint8_t midinote;
	int8_t scaleSeqNr;
	Color shownColor;
	bool playing = false;
	ExquisNote(uint8_t noteId, Color color){
		this->noteId = noteId;
		this->color = color;
		this->coord = idToCoord();
		this->midinote = 36 + noteId;
		this->scaleSeqNr = -1;
		this->brightness = 1.f;
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
	void sendSetMidinoteMessage(midi::Output* midi_output){
		midi::Message msg;
		// F0 00 21 7E 04 noteId midinote F7
		msg.bytes = {0xf0, 0x00, 0x21, 0x7e, 0x04, noteId, midinote, 0xf7};
		midi_output->sendMessage(msg);
	}
	void sendSetColorMessage(midi::Output* midi_output, Color color){
		midi::Message msg;
		// F0 00 21 7E 03 noteId r g b F7
		shownColor = color;
		msg.bytes = {0xf0, 0x00, 0x21, 0x7e, 0x03, noteId, color.r, color.g, color.b, 0xf7};
		midi_output->sendMessage(msg);
	}
};

struct Exquis;

struct ExquisBreathingNote {
    ExquisNote* note = NULL;
    midi::Output* midi_output;
    float period;
    float phase;
    bool active = false;

    dsp::ClockDivider clockDivider;
    ExquisBreathingNote(){
        this->phase = 0.f;
        clockDivider.setDivision(1500); // 32Hz at 48kHz
    }
    void config(midi::Output* midi_output, float period){
        this->midi_output = midi_output;
        this->period = 32.f * period;
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
            note->sendSetColorMessage(midi_output, note->color * brightness);
            
        }
    }

    void startWithNote(ExquisNote* note){
        if (active){
            stop();
        }
        this->note = note;
        active = true;
        phase = 0.f;

    }
    void stop(){
        if(note != NULL){
            note->sendSetColorMessage(midi_output, note->color * note->brightness);
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

        tuningRetuneNote.config(&midi_output, .3f);
        tuningConstantNote.config(&midi_output, 1.5f);
        selectedScaleNote.config(&midi_output, .7f);
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
            return;
        }
		for (ExquisNote& note : notes){
            note.sendSetColorMessage(&midi_output, note.color * note.brightness);
		}
	}
	void setNoteMidinoteValues(){
        if (!connected){
            needsMidinoteValuesUpdate = true;
            return;
        }
		for (ExquisNote& note : notes){
			note.sendSetMidinoteMessage(&midi_output);
		}
	}

	void checkConnection(){
		for (int driver_id : midi::getDriverIds()){
			midi::Driver* driver = midi::getDriver(driver_id);
			for (int device_id : driver->getOutputDeviceIds()){
				if (driver->getOutputDeviceName(device_id) == "Exquis"){
					midi_output.setDriverId(driver_id);
					midi_output.setDeviceId(device_id);
				}
			}
			for (int device_id : driver->getInputDeviceIds()){
				if (driver->getInputDeviceName(device_id) == "Exquis"){
					midi_input.setDriverId(driver_id);
					midi_input.setDeviceId(device_id);
					connected = true;
					return;
				}
			}
		}
		connected = false;
	}
	void sendKeepaliveMessage() {
		midi::Message msg;
		// F0 00 21 7E F7
		msg.bytes = {0xf0, 0x00, 0x21, 0x7e, 0xf7};
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


