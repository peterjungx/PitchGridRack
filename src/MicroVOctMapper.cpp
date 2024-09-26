#include "plugin.hpp"


using simd::float_4;
using simd::int32_4;

class ConsistentTuning {
	int a1, b1;
	float f1, log2f1;
	int a2, b2;
	float f2, log2f2;
	float det;
public:
	ConsistentTuning(int a1, int b1, float f1, int a2, int b2, float f2) {
		this->setParams(a1, b1, f1, a2, b2, f2);
	};
	void setParams(int a1, int b1, float f1, int a2, int b2, float f2) {
		this->a1 = a1;
		this->b1 = b1;
		this->f1 = f1;
		this->a2 = a2;
		this->b2 = b2;
		this->f2 = f2;
		this->det = a1 * b2 - a2 * b1;
		assert(this->det != 0.f);
		this->log2f1 = log2(f1);
		this->log2f2 = log2(f2);
	};
	float vecToFreqRatio(int a, int b){
		float z1 = (- this->a2 * b + this->b2 * a) / this->det;
		float z2 = (this->a1 * b - this->b1 * a)  / this->det;
		float f = pow(this->f1, z1) * pow(this->f2, z2);
		return f;
	};
	float vecToVoltage(int a, int b){
		float z1 = (- this->a2 * b + this->b2 * a) / this->det;
		float z2 = (this->a1 * b - this->b1 * a)  / this->det;
		float v = z1 * this->log2f1 + z2 * this->log2f2;
		return v;
	};


};



struct VOctMapper : Module {
	enum ParamIds {
		TUNING_OCT_PARAM,
		TUNING_FIFTH_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		VOCT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		MVOCT_OUTPUT,
		NUM_OUTPUTS
	};

	enum class TuningPresets : int {
		TUNING_12TET = 0,
		TUNING_PYTHAGOREAN = 1,
		TUNING_QUARTERCOMMA_MEANTONE = 2,
		TUNING_THIRDCOMMA_MEANTONE = 3,
		TUNING_5LIMIT_CLEANTONE = 4,
		TUNING_7LIMIT_CLEANTONE = 5,
		TUNING_19TET = 6,
		TUNING_31TET = 7,
	};

	enum class BlackKeyMapPresets : int {
		BLACKKEY_ALLFLAT = 0,
		BLACKKEY_FSHARP = 1,
		BLACKKEY_CSHARP = 2,
		BLACKKEY_GSHARP = 3,
		BLACKKEY_DSHARP = 4,
		BLACKKEY_ALLSHARP = 5
	};

	int blackkey_diatonic_step_sequences[6][12] = {
	//   C  Db D  Eb E  F  Gb G  Ab A  Bb B
	//   C  Db D  Eb E  F  F# G  Ab A  Bb B
	//   C  C# D  Eb E  F  F# G  Ab A  Bb B
	//   C  C# D  Eb E  F  F# G  G# A  Bb B
	//   C  C# D  D# E  F  F# G  G# A  Bb B
	//   C  C# D  D# E  F  F# G  G# A  A# B
		{0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6},
		{0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6},
		{0, 0, 1, 2, 2, 3, 3, 4, 5, 5, 6, 6},
		{0, 0, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6},
		{0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 6, 6},
		{0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6},
	};

	TuningPresets tuningPreset = TuningPresets::TUNING_12TET;
	BlackKeyMapPresets blackKeyMapPreset = BlackKeyMapPresets::BLACKKEY_FSHARP;
	ConsistentTuning tuning = ConsistentTuning(2, 5, 2.f, 1, 3, pow(2.f, 7.f/12.f)); // 12TET


	VOctMapper() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, 0);

		configParam(TUNING_OCT_PARAM, .25f, .75f, .5f, "Sub Freq", "ratio", 0.f, 100.f);
		configParam(TUNING_FIFTH_PARAM, .375f, 1.125f, .75f, "Sub3 Freq", "ratio", 0.f, 100.f);

		configInput(VOCT_INPUT, "1V/octave pitch");

		configOutput(MVOCT_OUTPUT, "Microtonally adjusted V/OCT pitch");


	}

	int getBlackKeyMapPreset() {
		return static_cast<int>(blackKeyMapPreset);
	}
	void setBlackKeyMapPreset(int b){
		blackKeyMapPreset = (BlackKeyMapPresets)b;

		// TODO: implement black key mapping
	}

	int getTuningPreset() {
		return static_cast<int>(tuningPreset);
	}
	void setTuningPreset(int t) {
		tuningPreset = (TuningPresets)t;

		switch (tuningPreset) {
			case TuningPresets::TUNING_12TET:
				tuning.setParams(2, 5, 2.f, 1, 0, pow(2.f, 1.f/12.f));
				break;
			case TuningPresets::TUNING_PYTHAGOREAN:
				tuning.setParams(2, 5, 2.f, 1, 3, 3.f/2.f);
				break;
			case TuningPresets::TUNING_QUARTERCOMMA_MEANTONE:
				tuning.setParams(2, 5, 2.f, 0, 2, 5.f/4.f);
				break;
			case TuningPresets::TUNING_THIRDCOMMA_MEANTONE:
				tuning.setParams(2, 5, 2.f, 1, 1, 6.f/5.f);
				break;
			case TuningPresets::TUNING_5LIMIT_CLEANTONE:
				tuning.setParams(0, 2, 5.f/4.f, 1, 3, 3.f/2.f);
				break;
			case TuningPresets::TUNING_7LIMIT_CLEANTONE:
				tuning.setParams(1, 1, 7.f/6.f, 1, 3, 3.f/2.f);
				break;
			case TuningPresets::TUNING_19TET:
				tuning.setParams(2, 5, 2.f, 1, 0, pow(2.f, 2.f/19.f));
				break;
			case TuningPresets::TUNING_31TET:
				tuning.setParams(2, 5, 2.f, 1, 0, pow(2.f, 3.f/31.f));
				break;
		}

	}

	void process(const ProcessArgs& args) override {
		//float fmParam = params[FM_PARAM].getValue();
		
		int channels = std::max(inputs[VOCT_INPUT].getChannels(), 1);

		for (int c = 0; c < channels; c += 4) {
			float_4 standard_pitch = inputs[VOCT_INPUT].getPolyVoltageSimd<float_4>(c) + 5.f; // + 5 octaves
			/*
			def voltage2coordinates(v,r):
				a=ceil(7*v-(r+5.5)/12)
				b=floor(5*v+(r+5.5)/12)
				a -= b
				return a,b 
			*/
			int r = (int)blackKeyMapPreset;

			int32_4 a = (int32_4)( ceil(7.f * standard_pitch - (r + 5.5f) / 12.f ) + .5f );
			int32_4 b = (int32_4)( floor(5.f * standard_pitch + (r + 5.5f) / 12.f ) + .5f );
			a -= b;

			// - 5 octaves
			a -= 10;
			b -= 25;

			float_4 voltage;
			for (int i = 0; i < 4; i++){
			 	voltage[i] = tuning.vecToVoltage(a[i], b[i]);
			}

			// Set output
			if (outputs[MVOCT_OUTPUT].isConnected()){
				outputs[MVOCT_OUTPUT].setVoltageSimd(voltage, c);
			}

		}

		outputs[MVOCT_OUTPUT].setChannels(channels);

	}


	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "blackKeyMapPreset", json_integer((int)blackKeyMapPreset));
		json_object_set_new(rootJ, "tuningPreset", json_integer((int)tuningPreset));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* blackKeyMapPresetJ = json_object_get(rootJ, "blackKeyMapPreset");
		if (blackKeyMapPresetJ)
			blackKeyMapPreset = (BlackKeyMapPresets)json_integer_value(blackKeyMapPresetJ);
		json_t* tuningPresetJ = json_object_get(rootJ, "tuningPreset");
		if (tuningPresetJ)
			tuningPreset = (TuningPresets)json_integer_value(tuningPresetJ);
	}

};

struct VOctTuningDisplay: TuningDisplay {
	VOctMapper* module;
	void step() override {
		text = "12-TET";
		if(module){
			text = module->tuningPreset == VOctMapper::TuningPresets::TUNING_12TET ? "12-TET" :
				module->tuningPreset == VOctMapper::TuningPresets::TUNING_PYTHAGOREAN ? "Pythagorean" :
				module->tuningPreset == VOctMapper::TuningPresets::TUNING_QUARTERCOMMA_MEANTONE ? "1/4-comma Meantone" :
				module->tuningPreset == VOctMapper::TuningPresets::TUNING_THIRDCOMMA_MEANTONE ? "1/3-comma Meantone" :
				module->tuningPreset == VOctMapper::TuningPresets::TUNING_5LIMIT_CLEANTONE ? "5-limit (Cleantone)" :
				module->tuningPreset == VOctMapper::TuningPresets::TUNING_7LIMIT_CLEANTONE ? "7-limit (m3=7/6 P5=3/2)" :
				module->tuningPreset == VOctMapper::TuningPresets::TUNING_19TET ? "19-TET" :
				module->tuningPreset == VOctMapper::TuningPresets::TUNING_31TET ? "31-TET" : "Unknown";
		}
	};
};

struct VOctMapperKeyboardDisplay : Widget {
	std::string fontPath;
	std::string text[12] = { "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
	float fontSize;
	NVGcolor whiteKeyFontColor = SCHEME_BLACK;
	NVGcolor blackKeyFontColor = SCHEME_YELLOW;
	Vec textPos[12];

	VOctMapper* module;

	VOctMapperKeyboardDisplay() {
		fontPath = asset::plugin(pluginInstance, "res/fonts/PTSans.ttc");
		float key_x_offset = 16.8f;
		float key_x_base = 11.f;
		textPos[0] = Vec(key_x_base, 20);
		textPos[1] = Vec(18, 0); //Db
		textPos[2] = Vec(key_x_base + key_x_offset, 20);
		textPos[3] = Vec(21 + key_x_offset, 0); //Eb
		textPos[4] = Vec(key_x_base + 2 * key_x_offset, 20);
		textPos[5] = Vec(key_x_base + 3 * key_x_offset, 20);
		textPos[6] = Vec(16 + 3 * key_x_offset, 0); //Gb
		textPos[7] = Vec(key_x_base + 4 * key_x_offset, 20);
		textPos[8] = Vec(19 + 4 * key_x_offset, 0); //Ab
		textPos[9] = Vec(key_x_base + 5 * key_x_offset, 20);
		textPos[10] = Vec(22 +5 * key_x_offset, 0); //Bb
		textPos[11] = Vec(key_x_base + 6 * key_x_offset, 20);
		
		fontSize = 14;
	}

	void step() override {
		if (module) {
			int r = (int)module->getBlackKeyMapPreset();
			text[1] = r < 2 ? "Db" : "C#";
			text[3] = r < 4 ? "Eb" : "D#";
			text[6] = r < 1 ? "Gb" : "F#";
			text[8] = r < 3 ? "Ab" : "G#";
			text[10] = r < 5 ? "Bb" : "A#";
		}
	};
	

	void prepareFont(const DrawArgs& args) {
		// Get font
		std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		if (!font)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, fontSize);
		nvgTextLetterSpacing(args.vg, -0.7);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	}

	//void draw(const DrawArgs& args) override {
	//	// Background
	//	//nvgBeginPath(args.vg);
	//	//nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
	//	//nvgFillColor(args.vg, nvgRGB(0x19, 0x19, 0x19));
	//	//nvgFill(args.vg);
	//
	//	//prepareFont(args);
	//}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			prepareFont(args);

			// Foreground text
			//nvgFillColor(args.vg, fgColor);
			for (int i = 0; i < 12; i++){
				nvgFillColor(args.vg, i == 1 || i == 3|| i == 6|| i == 8|| i == 10 ? blackKeyFontColor : whiteKeyFontColor);
				nvgText(args.vg, textPos[i].x, textPos[i].y, text[i].c_str(), NULL);
			}
		}
		Widget::drawLayer(args, layer);
	}
};


struct VOctMapperWidget : ModuleWidget {
	VOctMapperWidget(VOctMapper* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/McVOctMapper.svg")));

		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(6.607, 113.115)), module, VOctMapper::VOCT_INPUT));

		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(39.15, 113.115)), module, VOctMapper::MVOCT_OUTPUT));

		VOctTuningDisplay* display = createWidget<VOctTuningDisplay>(mm2px(Vec(2.0, 80.0)));
		display->box.size = mm2px(Vec(42, 7));
		display->module = module;
		addChild(display);

		VOctMapperKeyboardDisplay* keyboardDisplay = createWidget<VOctMapperKeyboardDisplay>(mm2px(Vec(2.0, 27.0)));
		keyboardDisplay->box.size = mm2px(Vec(42, 7));
		keyboardDisplay->module = module;
		addChild(keyboardDisplay);

	}

	void appendContextMenu(Menu* menu) override {
		VOctMapper* module = getModule<VOctMapper>();
		assert(module);

		menu->addChild(new MenuSeparator);

		menu->addChild(createIndexSubmenuItem("Black key mapping",
			{
				"All flat (Gb)",
				"F# (default)",
				"C#",
				"G#",
				"D#",
				"All sharp (A#)",
			},
			[=]() {
				return module->getBlackKeyMapPreset();
			},
			[=](int blackkeymap) {
				module->setBlackKeyMapPreset(blackkeymap);
			}
		));

		menu->addChild(createIndexSubmenuItem("Tuning",
			{
				"12-TET", 
				"Pythagorean",
				"1/4-comma Meantone", 
				"1/3-comma Meantone", 
				"5-limit (Cleantone)", 
				"7-limit (m3=7/6 P5=3/2)",
				"19-TET",
				"31-TET"
			},
			[=]() {
				return module->getTuningPreset();
			},
			[=](int tuning) {
				module->setTuningPreset(tuning);
			}
		));
	}
};


Model* modelMicroVOctMapper = createModel<VOctMapper, VOctMapperWidget>("MicroVOctMapper");
