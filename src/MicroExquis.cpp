/*

Microtonal Exquis 

Makes the Exquis into a freely configurable isomorphic keyboard 
with microtonal tuning for any regular scale.

The Exquis is an affordable and award-winning MPE controller 
by Intuitive Instruments. The PitchGrid integration with VCV Rack allows
to explore 

*/


#include "plugin.hpp"
#include "midi.hpp"

#include "pitchgrid_exquis.hpp"
#include "datalink.hpp"

using simd::float_4;
using simd::int32_4;


struct MicroExquis : Module {
	enum ParamIds {
		TUNING_OCTAVE_PARAM,
		TUNING_PITCHANGLE_PARAM,
		COLORING_SCHEME_PARAM,
		COLORING_MODE_PARAM, // 0=scale, 1=pitch
		SCALE_STEPS_A_PARAM, 
		SCALE_STEPS_B_PARAM, 
		SCALE_MODE_PARAM, // 1=major
		NUM_PARAMS
	};
	enum InputIds {
		VOCT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		MVOCT_OUTPUT,
		TUNING_DATA_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		COLORING_MODE_LIGHT,
		SCALE_MODE_LIGHT,
		NUM_LIGHTS
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
		TUNING_EXQUIS = 8
	};


	TuningPresets tuningPreset = TuningPresets::TUNING_12TET;
	ConsistentTuning tuning = ConsistentTuning({2, 5}, 2.f, {1, 3}, pow(2.f, 7.f/12.f)); // 12TET

	std::string message = "-";
	rack::dsp::Timer timer;
	int cnt = 0;
	PitchGridExquis exquis;

	bool did_set_values = false;
	int write_iterations = 5;

	uint8_t last_scaleStepsA = 2;
	uint8_t last_scaleStepsB = 5;
	uint8_t last_scaleMode = 1;
	float last_tuningOctave = 2.f;
	float last_tuningPitchAngle = 5.f;

	dsp::ClockDivider lightDivider;

	dsp::ClockDivider badlyImplementedValueUpdateDividerTODOMakeProperly;


	float exquis_tuningOctaveParam;
	float exquis_tuningPitchAngleParam;
	float exquis_coloringSchemeParam;
	float exquis_coloringModeParam;
	float exquis_scaleStepsAParam;
	float exquis_scaleStepsBParam;
	float exquis_scaleModeParam;

	float tuningOctaveParam;
	float tuningPitchAngleParam;
	float coloringSchemeParam;
	float coloringModeParam;
	float scaleStepsAParam;
	float scaleStepsBParam;
	float scaleModeParam;

	bool initialized = false;

	std::string tuning_info_string = "manual tuning";

	TuningDataSender tuningDataSender = TuningDataSender();

	MicroExquis() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(TUNING_OCTAVE_PARAM, 1.08f, 4.f, 2.f, "Octave Frequency Ratio", "");
		configParam(TUNING_PITCHANGLE_PARAM, -3.9f, 3.9f, 0.40965f, "Pitch Angle (45º/V)", "º", 0.f, 45.f); // 45 degrees / Volt. 12TET 2:1 = (45-arctan2(1,2)/(2*3.1415)*360)/45 = 0.40965
		configParam(COLORING_SCHEME_PARAM, 0.f, 1.f, 0.f, "Coloring Scheme", "V", 0.f, 2.f);
		configSwitch(COLORING_MODE_PARAM, 0.f, 1.f, 0.f, "Coloring Mode", {"Scale", "Pitch"});
		configParam(SCALE_STEPS_A_PARAM, 1.f, 7.f, 2.f, "Scale Steps A", "V");
		configParam(SCALE_STEPS_B_PARAM, 1.f, 7.f, 5.f, "Scale Steps B", "V");
		configParam(SCALE_MODE_PARAM, 0.f, 1.f, 1.f, "Scale Mode", "", 0.f, 7.f);

		configInput(VOCT_INPUT, "1V/octave pitch");
		configOutput(MVOCT_OUTPUT, "Microtonal Interface to the Exquis by Intuitive Instruments");
		configOutput(TUNING_DATA_OUTPUT, "Tuning Data");

		//TuningDataSender tuningDataSender(&outputs[TUNING_DATA_OUTPUT]);
		tuningDataSender.addTuningData(&tuning, &exquis.scaleMapper.scale);

		timer.reset();
		lightDivider.setDivision(16);
		badlyImplementedValueUpdateDividerTODOMakeProperly.setDivision(24000);

		exquis.tuning = &tuning;

		

	}

	int getTuningPreset() {
		return static_cast<int>(tuningPreset);
	}
	void setTuningPreset(int t) {
		tuningPreset = (TuningPresets)t;

		switch (tuningPreset) {
			case TuningPresets::TUNING_12TET:
				tuning.setParams({2, 5}, 2.f, {1, 0}, pow(2.f, 1.f/12.f));
				//tuning.setParams({1, 3}, pow(2.f, 7.f/12.f), {0, 2}, pow(2.f, 4.f/12.f));
				break;
			case TuningPresets::TUNING_PYTHAGOREAN:
				tuning.setParams({2, 5}, 2.f, {1, 3}, 3.f/2.f);
				//tuning.setParams({1, 3}, 3.f/2.f, {0, 2}, 81.f/64.f);
				break;
			case TuningPresets::TUNING_QUARTERCOMMA_MEANTONE:
				tuning.setParams({2, 5}, 2.f, {0, 2}, 5.f/4.f);
				//tuning.setParams({2, 5}, 2.f, {0, 2}, 5.f/4.f);
				break;
			case TuningPresets::TUNING_THIRDCOMMA_MEANTONE:
				tuning.setParams({2, 5}, 2.f, {1, 1}, 6.f/5.f);
				break;
			case TuningPresets::TUNING_5LIMIT_CLEANTONE:
				tuning.setParams({0, 2}, 5.f/4.f, {1, 3}, 3.f/2.f);
				break;
			case TuningPresets::TUNING_7LIMIT_CLEANTONE:
				tuning.setParams({1, 1}, 7.f/6.f, {1, 3}, 3.f/2.f);
				break;
			case TuningPresets::TUNING_19TET:
				tuning.setParams({2, 5}, 2.f, {1, 0}, pow(2.f, 2.f/19.f));
				break;
			case TuningPresets::TUNING_31TET:
				tuning.setParams({2, 5}, 2.f, {1, 0}, pow(2.f, 3.f/31.f));
				break;
		}
		exquis.didManualRetune = false;

	}


	void setParams(
		int scaleStepsA, 
		int scaleStepsB, 
		float tuningOctave, 
		int scaleMode, 
		float tuningPitchAngle
	){
		params[TUNING_OCTAVE_PARAM].setValue(tuningOctave);
		params[TUNING_PITCHANGLE_PARAM].setValue(tuningPitchAngle);
		params[SCALE_STEPS_A_PARAM].setValue(scaleStepsA);
		params[SCALE_STEPS_B_PARAM].setValue(scaleStepsB);
		params[SCALE_MODE_PARAM].setValue(scaleMode);
	}

	void setTuningInfoString(){
		tuning_info_string = "(" + std::to_string(tuning.V1().x) + "," + std::to_string(tuning.V1().y) + ")=" 
							+ std::to_string(int(1200*log2(tuning.F1())+0.5f)) 
							+ "ct (" + std::to_string(tuning.V2().x) + "," + std::to_string(tuning.V2().y) + ")="
							+ std::to_string(int(1200*log2(tuning.F2())+0.5f)) + "ct";
	}


	void process(const ProcessArgs& args) override {
		//float fmParam = params[FM_PARAM].getValue();

		if (!initialized){
			tuningOctaveParam = params[TUNING_OCTAVE_PARAM].getValue();
			tuningPitchAngleParam = params[TUNING_PITCHANGLE_PARAM].getValue();
			scaleStepsAParam = params[SCALE_STEPS_A_PARAM].getValue();
			scaleStepsBParam = params[SCALE_STEPS_B_PARAM].getValue();
			scaleModeParam = params[SCALE_MODE_PARAM].getValue();		
			initialized = true;
		}
		

		if (badlyImplementedValueUpdateDividerTODOMakeProperly.process()){
			//params[TUNING_OCTAVE_PARAM].setValue(tuning.vecToFreqRatioNoOffset(exquis.scaleMapper.scale.scale_class));
			//params[TUNING_PITCHANGLE_PARAM].setValue(tuning.vecToFreqRatioNoOffset({1,0}));
			//params[SCALE_MODE_PARAM].setValue((float)exquis.scaleMapper.scale.mode/exquis.scaleMapper.scale.n);

			exquis_tuningOctaveParam = tuning.vecToFreqRatioNoOffset(exquis.scaleMapper.scale.scale_class);
			exquis_tuningPitchAngleParam = 45.0f / M_2_PI * atan2(tuning.vecToFreqRatioNoOffset({1,0}), tuning.vecToFreqRatioNoOffset({0,1}));
			exquis_scaleStepsAParam = exquis.scaleMapper.scale.scale_class.x;
			exquis_scaleStepsBParam = exquis.scaleMapper.scale.scale_class.y;
			exquis_scaleModeParam = (float)exquis.scaleMapper.scale.mode/exquis.scaleMapper.scale.n;

			if (
				exquis_tuningOctaveParam != tuningOctaveParam || 
				exquis_tuningPitchAngleParam != tuningPitchAngleParam || 
				exquis_scaleStepsAParam != scaleStepsAParam || 
				exquis_scaleStepsBParam != scaleStepsBParam || 
				exquis_scaleModeParam != scaleModeParam
			){
				setParams(
					exquis_scaleStepsAParam, 
					exquis_scaleStepsBParam, 
					exquis_tuningOctaveParam, 
					exquis_scaleModeParam, 
					exquis_tuningPitchAngleParam
				);
				tuningOctaveParam = exquis_tuningOctaveParam;
				tuningPitchAngleParam = exquis_tuningPitchAngleParam;
				scaleStepsAParam = exquis_scaleStepsAParam;
				scaleStepsBParam = exquis_scaleStepsBParam;
				scaleModeParam = exquis_scaleModeParam;

			}else{
				//tuningOctaveParam = params[TUNING_OCTAVE_PARAM].getValue();
				//tuningPitchAngleParam = params[TUNING_PITCHANGLE_PARAM].getValue();
				//scaleStepsAParam = params[SCALE_STEPS_A_PARAM].getValue();
				//scaleStepsBParam = params[SCALE_STEPS_B_PARAM].getValue();
				//scaleModeParam = params[SCALE_MODE_PARAM].getValue();				
			}

			if (exquis.didManualRetune){
				tuningPreset = MicroExquis::TuningPresets::TUNING_EXQUIS;
				setTuningInfoString();
				exquis.didManualRetune = false;
			}
			

		}


		tuningOctaveParam = params[TUNING_OCTAVE_PARAM].getValue();
		tuningPitchAngleParam = params[TUNING_PITCHANGLE_PARAM].getValue();
		coloringSchemeParam = params[COLORING_SCHEME_PARAM].getValue();
		coloringModeParam = params[COLORING_MODE_PARAM].getValue();
		scaleStepsAParam = params[SCALE_STEPS_A_PARAM].getValue();
		scaleStepsBParam = params[SCALE_STEPS_B_PARAM].getValue();
		scaleModeParam = params[SCALE_MODE_PARAM].getValue();

		uint8_t scaleStepsA = round(scaleStepsAParam);
		uint8_t scaleStepsB = round(scaleStepsBParam);
		uint8_t scaleMode = round(scaleModeParam * exquis.scaleMapper.scale.n);



		//if (
		//	scaleStepsA != last_scaleStepsA || 
		//	scaleStepsB != last_scaleStepsB || 
		//	scaleMode != last_scaleMode ||
		//	tuningOctaveParam != last_tuningOctave ||
		//	tuningPitchAngleParam != last_tuningPitchAngle
		//	
		//){
		//	tuning.setParams({scaleStepsA, scaleStepsB}, tuningOctaveParam, {1, 0}, pow(2.f, tuningPitchAngleParam/45.f));
		//	message = "A: " + std::to_string(scaleStepsA) + " B: " + std::to_string(scaleStepsB);
		//	exquis.scaleMapper.scale.setScaleClass({scaleStepsA, scaleStepsB});
		//	exquis.scaleMapper.scale.mode = scaleMode;
		//	exquis.showAllOctavesLayer();
		//
		//	tuning.setParams({scaleStepsA, scaleStepsB}, tuningOctaveParam, {1, 0}, pow(2.f, tuningPitchAngleParam/45.f));
		//
		//	last_scaleStepsA = scaleStepsA;
		//	last_scaleStepsB = scaleStepsB;
		//	last_scaleMode = scaleMode;
		//	last_tuningOctave = tuningOctaveParam;
		//	last_tuningPitchAngle = tuningPitchAngleParam;
		//
		//}


		float dt = timer.process(args.sampleTime);
		if (dt > 0.25f){
			
			timer.reset();
			cnt +=1;
			//message = std::to_string(cnt);

			if (cnt%10 == 1) {
				if (!did_set_values){
					message = "ready";
				}
				exquis.checkConnection();

			}
			if (exquis.connected){
				message = "Exquis connected";
				exquis.sendKeepaliveMessage();
			}else{
				message = "Exquis disconnected";
			}


			if (exquis.connected){
				if  (write_iterations > 0){
					exquis.setNoteMidinoteValues();
					exquis.setNoteColors();
					write_iterations -= 1;
				}else{
					did_set_values = true;
				}
			}else{
				write_iterations = 5;
			}
		}

		exquis.process(args);
		
		int channels = std::max(inputs[VOCT_INPUT].getChannels(), 1);

		for (int c = 0; c < channels; c += 4) {
			float_4 pitch = inputs[VOCT_INPUT].getPolyVoltageSimd<float_4>(c);
			float_4 voltage;

			for (int i = 0; i < 4; i++){
				ExquisNote* note = exquis.getNoteByVoltage(pitch[i]);
				voltage[i] = tuning.vecToVoltage(note->scaleCoord);
			}

			// Set output
			if (outputs[MVOCT_OUTPUT].isConnected()){
				outputs[MVOCT_OUTPUT].setVoltageSimd(voltage, c);
			}

		}

		outputs[MVOCT_OUTPUT].setChannels(channels);


		if (lightDivider.process()) {
			lights[COLORING_MODE_LIGHT].setBrightness(coloringModeParam);
			lights[SCALE_MODE_LIGHT].setBrightness(scaleModeParam);

			tuningDataSender.setTuningData(&tuning, &exquis.scaleMapper.scale);
		}
		tuningDataSender.processWithOutput(&outputs[TUNING_DATA_OUTPUT]);

	}


	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		int tuningPreset = getTuningPreset();
		json_object_set_new(rootJ, "tuningPreset", json_integer(tuningPreset));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {

		json_t* tuningPresetJ = json_object_get(rootJ, "tuningPreset");
		if (tuningPresetJ){
			int tuningPreset = json_integer_value(tuningPresetJ);
			setTuningPreset(tuningPreset);
		}
	}

};

struct MicroExquisTuningDisplay: TuningDisplay {
	MicroExquis* module;
	void step() override {
		text = "12-TET";
		if(module){
			text = module->tuningPreset == MicroExquis::TuningPresets::TUNING_12TET ? "12-TET" :
				module->tuningPreset == MicroExquis::TuningPresets::TUNING_PYTHAGOREAN ? "Pythagorean" :
				module->tuningPreset == MicroExquis::TuningPresets::TUNING_QUARTERCOMMA_MEANTONE ? "1/4-comma Meantone" :
				module->tuningPreset == MicroExquis::TuningPresets::TUNING_THIRDCOMMA_MEANTONE ? "1/3-comma Meantone" :
				module->tuningPreset == MicroExquis::TuningPresets::TUNING_5LIMIT_CLEANTONE ? "5-limit (Cleantone)" :
				module->tuningPreset == MicroExquis::TuningPresets::TUNING_7LIMIT_CLEANTONE ? "7-limit (m3=7/6 P5=3/2)" :
				module->tuningPreset == MicroExquis::TuningPresets::TUNING_19TET ? "19-TET" :
				module->tuningPreset == MicroExquis::TuningPresets::TUNING_31TET ? "31-TET" : 
				module->tuningPreset == MicroExquis::TuningPresets::TUNING_EXQUIS ? module->tuning_info_string : 
				"Unknown";
		}
	};
};

struct InfoDisplay: TuningDisplay {
	MicroExquis* module;
	void step() override {
		if (module) text = module->message;
	};
};


struct MicroExquisWidget : ModuleWidget {
	MicroExquisWidget(MicroExquis* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/MicroExquis.svg")));

		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


		addParam(createParamCentered<Trimpot>(mm2px(Vec(6.607, 48.091)), module, MicroExquis::TUNING_OCTAVE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(17.444, 48.091)), module, MicroExquis::TUNING_PITCHANGLE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(28.282, 48.091)), module, MicroExquis::SCALE_STEPS_A_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(39.15, 48.091)), module, MicroExquis::SCALE_STEPS_B_PARAM));


		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(6.607, 64.347)), module, MicroExquis::COLORING_MODE_PARAM, MicroExquis::COLORING_MODE_LIGHT));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(39.15, 64.347)), module, MicroExquis::SCALE_MODE_PARAM));

		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(6.607, 113.115)), module, MicroExquis::VOCT_INPUT));

		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(39.15, 113.115)), module, MicroExquis::MVOCT_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(39.15, 96.859)), module, MicroExquis::TUNING_DATA_OUTPUT));

		MicroExquisTuningDisplay* display = createWidget<MicroExquisTuningDisplay>(mm2px(Vec(2.0, 78.0)));
		display->box.size = mm2px(Vec(42, 7));
		display->module = module;
		addChild(display);

		InfoDisplay* infoDisplay = createWidget<InfoDisplay>(mm2px(Vec(2.0, 70.0)));
		infoDisplay->box.size = mm2px(Vec(42, 7));
		infoDisplay->module = module;
		addChild(infoDisplay);

	}

	void appendContextMenu(Menu* menu) override {
		MicroExquis* module = getModule<MicroExquis>();
		assert(module);

		menu->addChild(new MenuSeparator);

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


Model* modelMicroExquis = createModel<MicroExquis, MicroExquisWidget>("MicroExquis");
