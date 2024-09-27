/*

Microtonal Exquis 

Makes the Exquis into a freely configurable isomorphic keyboard 
with microtonal tuning for any regular scale.

The Exquis is an affordable and award-winning MPE controller 
by Intuitive Instruments. The PitchGrid integration with VCV Rack allows
to explore 

*/

#include <iomanip>

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

	ConsistentTuning tuning = ConsistentTuning({2, 5}, 2.f, {1, 3}, pow(2.f, 7.f/12.f)); // 12TET

	rack::dsp::Timer timer;
	int cnt = 0;
	PitchGridExquis exquis;

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

	std::string tuning_info_string1 = "";
	std::string tuning_info_string2 = "";

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
		
		std::stringstream ss1;
		ss1 << std::fixed << std::setprecision(1) << 1200*log2(tuning.F1()) << "ct"
			<< " (" <<  exquis.contFracDisplay(tuning.F1()) << ")";
		tuning_info_string1 = "(" + std::to_string(tuning.V1().y) + "," + std::to_string(tuning.V1().x) + ")=" + ss1.str();

		std::stringstream ss2;
		ss2 << std::fixed << std::setprecision(1) << 1200*log2(tuning.F2()) << "ct"
			<< " (" <<  exquis.contFracDisplay(tuning.F2()) << ")";
		tuning_info_string2 = "(" + std::to_string(tuning.V2().y) + "," + std::to_string(tuning.V2().x) + ")=" + ss2.str();

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
				//tuningPreset = MicroExquis::TuningPresets::TUNING_EXQUIS;
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

			if (cnt%10 == 1) {
				exquis.checkConnection();
			}

			if (exquis.connected){
				exquis.sendKeepaliveMessage();
				if  (write_iterations > 0){
					exquis.setNoteMidinoteValues();
					exquis.setNoteColors();
					write_iterations -= 1;
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
		//int tuningPreset = getTuningPreset();
		//json_object_set_new(rootJ, "tuningPreset", json_integer(tuningPreset));

		json_t* tuningJ = json_object();
		json_object_set_new(rootJ, "tuning", tuningJ);
		json_t* vec1J = json_array();
		json_array_append_new(vec1J, json_integer(tuning.V1().x));
		json_array_append_new(vec1J, json_integer(tuning.V1().y));
		json_t* vec2J = json_array();
		json_array_append_new(vec2J, json_integer(tuning.V2().x));
		json_array_append_new(vec2J, json_integer(tuning.V2().y));

		json_object_set_new(tuningJ, "vec1", vec1J);
		json_object_set_new(tuningJ, "f1", json_real(tuning.F1()));
		json_object_set_new(tuningJ, "vec2", vec2J);
		json_object_set_new(tuningJ, "f2", json_real(tuning.F2()));
		json_object_set_new(tuningJ, "offset", json_real(tuning.Offset()));

		json_t* scaleJ = json_object();
		json_object_set_new(rootJ, "scale", scaleJ);
		json_t* scaleClassJ = json_array();
		json_array_append_new(scaleClassJ, json_integer(exquis.scaleMapper.scale.scale_class.x));
		json_array_append_new(scaleClassJ, json_integer(exquis.scaleMapper.scale.scale_class.y));
		json_object_set_new(scaleJ, "class", scaleClassJ);
		json_object_set_new(scaleJ, "mode", json_integer(exquis.scaleMapper.scale.mode));

		json_t* layoutJ = json_object();
		json_object_set_new(rootJ, "layout", layoutJ);
		json_t* layoutBaseJ = json_array();
		json_array_append_new(layoutBaseJ, json_integer(exquis.scaleMapper.exquis_base.x));
		json_array_append_new(layoutBaseJ, json_integer(exquis.scaleMapper.exquis_base.y));
		json_t* layoutInterval1J = json_array();
		json_array_append_new(layoutInterval1J, json_integer(exquis.scaleMapper.exquis_interval1.x));
		json_array_append_new(layoutInterval1J, json_integer(exquis.scaleMapper.exquis_interval1.y));
		json_t* layoutInterval2J = json_array();
		json_array_append_new(layoutInterval2J, json_integer(exquis.scaleMapper.exquis_interval2.x));
		json_array_append_new(layoutInterval2J, json_integer(exquis.scaleMapper.exquis_interval2.y));

		json_object_set_new(layoutJ, "base", layoutBaseJ);
		json_object_set_new(layoutJ, "interval1", layoutInterval1J);
		json_object_set_new(layoutJ, "interval2", layoutInterval2J);

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {

		//json_t* tuningPresetJ = json_object_get(rootJ, "tuningPreset");
		//if (tuningPresetJ){
		//	int tuningPreset = json_integer_value(tuningPresetJ);
		//	setTuningPreset(tuningPreset);
		//}
		json_t* tuningJ = json_object_get(rootJ, "tuning");
		if (tuningJ){
			json_t* vec1J = json_object_get(tuningJ, "vec1");
			json_t* f1J = json_object_get(tuningJ, "f1");
			json_t* vec2J = json_object_get(tuningJ, "vec2");
			json_t* f2J = json_object_get(tuningJ, "f2");
			json_t* offsetJ = json_object_get(tuningJ, "offset");

			if (
				vec1J && json_is_array(vec1J) && json_array_size(vec1J) == 2 && json_is_integer(json_array_get(vec1J, 0)) && json_is_integer(json_array_get(vec1J, 1)) &&
				f1J && json_is_number(f1J) &&
				vec2J && json_is_array(vec2J) && json_array_size(vec2J) == 2 && json_is_integer(json_array_get(vec2J, 0)) && json_is_integer(json_array_get(vec2J, 1)) &&
				f2J && json_is_number(f2J) 
			){
				int v1x = json_integer_value(json_array_get(vec1J, 0));
				int v1y = json_integer_value(json_array_get(vec1J, 1));
				float f1 = json_number_value(f1J);
				int v2x = json_integer_value(json_array_get(vec2J, 0));
				int v2y = json_integer_value(json_array_get(vec2J, 1));
				float f2 = json_number_value(f2J);
				tuning.setParams({v1x, v1y}, f1, {v2x, v2y}, f2);
				setTuningInfoString();
				INFO("Tuning loaded: %d %d %f %d %d %f", v1x, v1y, f1, v2x, v2y, f2);
			}
			if (offsetJ && json_is_number(offsetJ)){
				float offset = json_number_value(offsetJ);
				tuning.setOffset(offset);
				INFO("Tuning offset loaded: %f", offset);
			}
		}
		json_t* scaleJ = json_object_get(rootJ, "scale");
		if (scaleJ && json_is_object(scaleJ)){
			json_t* scaleClassJ = json_object_get(scaleJ, "class");
			json_t* modeJ = json_object_get(scaleJ, "mode");
			if (
				scaleClassJ && json_is_array(scaleClassJ) && json_array_size(scaleClassJ) == 2 && 
				json_is_integer(json_array_get(scaleClassJ, 0)) && json_is_integer(json_array_get(scaleClassJ, 1)) && 
				modeJ && json_is_integer(modeJ)
			){
				int x = json_integer_value(json_array_get(scaleClassJ, 0));
				int y = json_integer_value(json_array_get(scaleClassJ, 1));
				int mode = json_integer_value(modeJ);
				exquis.scaleMapper.scale.setScaleClass({x, y});
				exquis.scaleMapper.scale.mode = mode;
				INFO("Scale loaded: %d %d %d", x, y, mode);
			}
		}

		json_t* layoutJ = json_object_get(rootJ, "layout");
		if (layoutJ && json_is_object(layoutJ)){
			json_t* layoutBaseJ = json_object_get(layoutJ, "base");
			json_t* layoutInterval1J = json_object_get(layoutJ, "interval1");
			json_t* layoutInterval2J = json_object_get(layoutJ, "interval2");
			if (
				layoutBaseJ && json_is_array(layoutBaseJ) && json_array_size(layoutBaseJ) == 2 && 
				json_is_integer(json_array_get(layoutBaseJ, 0)) && json_is_integer(json_array_get(layoutBaseJ, 1)) && 
				layoutInterval1J && json_is_array(layoutInterval1J) && json_array_size(layoutInterval1J) == 2 && 
				json_is_integer(json_array_get(layoutInterval1J, 0)) && json_is_integer(json_array_get(layoutInterval1J, 1)) && 
				layoutInterval2J && json_is_array(layoutInterval2J) && json_array_size(layoutInterval2J) == 2 && 
				json_is_integer(json_array_get(layoutInterval2J, 0)) && json_is_integer(json_array_get(layoutInterval2J, 1))
			){
				int baseX = json_integer_value(json_array_get(layoutBaseJ, 0));
				int baseY = json_integer_value(json_array_get(layoutBaseJ, 1));
				int interval1X = json_integer_value(json_array_get(layoutInterval1J, 0));
				int interval1Y = json_integer_value(json_array_get(layoutInterval1J, 1));
				int interval2X = json_integer_value(json_array_get(layoutInterval2J, 0));
				int interval2Y = json_integer_value(json_array_get(layoutInterval2J, 1));
				exquis.scaleMapper.exquis_base = {baseX, baseY};
				exquis.scaleMapper.exquis_interval1 = {interval1X, interval1Y};
				exquis.scaleMapper.exquis_interval2 = {interval2X, interval2Y};
				exquis.scaleMapper.calcTransforms();
				INFO("Layout loaded: %d %d %d %d %d %d", baseX, baseY, interval1X, interval1Y, interval2X, interval2Y);
			}
		}
		exquis.showAllOctavesLayer();
	}

};

struct MicroExquisDisplay: ExquisDisplay {
	MicroExquis* module;
	void step() override {
		if (module){
			text1 = "scale=(" + std::to_string(module->exquis.scaleMapper.scale.scale_class.y) + "," + std::to_string(module->exquis.scaleMapper.scale.scale_class.x) + ") mode=c" + std::to_string(module->exquis.scaleMapper.scale.mode+1);
			text2 = module->tuning_info_string1;
			text3 = module->tuning_info_string2;
			std::stringstream ss;
			ss << "base: " 
				<< std::fixed << std::setprecision(3) 
				<< module->tuning.Offset()
				<< "V "
				<< std::setprecision(2) 
				<< module->tuning.OffsetAsStandardFreq()
				<< "Hz";
			text4 = ss.str();
		}
	};
};

struct ExquisHexDisplay : Widget {
	MicroExquis* module;
	float hexsz = 5.8;

	void draw(const DrawArgs& args) override {
		drawBackground(args);
	}
	void drawBackground(const DrawArgs& args) {
		// Background
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
		nvgFillColor(args.vg, nvgRGB(0x19, 0x19, 0x19));
		nvgFill(args.vg);
	}

	float i2x(int i){
		return 9.5 + (i % 11) * 2*hexsz*1.1 - ((i%11)/6) * 11*hexsz*1.1;
	}
	float i2y(int i){
		return 10 + 20*hexsz - (i / 11) * 4*hexsz - ((i % 11)/6) * 2*hexsz;
	}

	void drawExquisLayout(const DrawArgs& args){
		if (module->exquis.connected){
			for (int i = 0; i < 61; i++){
				drawHexAt(args, i2x(i), i2y(i), i);
			}
		}else{
			drawBackground(args);
		}
	}

	void drawHexAt(const DrawArgs& args, float x, float y, int index) {
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, x, y-hexsz);
		nvgLineTo(args.vg, x+hexsz*sqrt(1-.25), y-hexsz*.5);
		nvgLineTo(args.vg, x+hexsz*sqrt(1-.25), y+hexsz*.5);
		nvgLineTo(args.vg, x, y+hexsz);
		nvgLineTo(args.vg, x-hexsz*sqrt(1-.25), y+hexsz*.5);
		nvgLineTo(args.vg, x-hexsz*sqrt(1-.25), y-hexsz*.5);
		nvgClosePath(args.vg);
		if (module->exquis.notes[index].playing){
			nvgFillColor(args.vg, nvgRGB(0xf9, 0xf9, 0xf9));
		}else{
			Color col = module->exquis.notes[index].shownColor;
			nvgFillColor(args.vg, nvgRGB(0x79 + col.r, 0x79 + col.g, 0x79 + col.b));
		}
		nvgFill(args.vg);		

	}

	void drawScalePath(const DrawArgs& args){
		nvgBeginPath(args.vg);

		//nvgMoveTo(args.vg, x, y-hexsz);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			drawExquisLayout(args);

		}
		Widget::drawLayer(args, layer);
	}	

};

struct MicroExquisWidget : ModuleWidget {
	MicroExquisWidget(MicroExquis* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/MicroExquis.svg")));

		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// removed controls. re-add later, with patching
		//addParam(createParamCentered<Trimpot>(mm2px(Vec(6.607, 48.091)), module, MicroExquis::TUNING_OCTAVE_PARAM));
		//addParam(createParamCentered<Trimpot>(mm2px(Vec(17.444, 48.091)), module, MicroExquis::TUNING_PITCHANGLE_PARAM));
		//addParam(createParamCentered<Trimpot>(mm2px(Vec(28.282, 48.091)), module, MicroExquis::SCALE_STEPS_A_PARAM));
		//addParam(createParamCentered<Trimpot>(mm2px(Vec(39.15, 48.091)), module, MicroExquis::SCALE_STEPS_B_PARAM));
		//addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(6.607, 64.347)), module, MicroExquis::COLORING_MODE_PARAM, MicroExquis::COLORING_MODE_LIGHT));
		//addParam(createParamCentered<Trimpot>(mm2px(Vec(39.15, 64.347)), module, MicroExquis::SCALE_MODE_PARAM));

		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(6.607, 113.115)), module, MicroExquis::VOCT_INPUT));

		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(39.15, 113.115)), module, MicroExquis::MVOCT_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(39.15, 96.859)), module, MicroExquis::TUNING_DATA_OUTPUT));

		MicroExquisDisplay* display = createWidget<MicroExquisDisplay>(mm2px(Vec(2.0, 67.0)));
		display->box.size = mm2px(Vec(42, 20));
		display->module = module;
		addChild(display);

		ExquisHexDisplay* hexDisplay = createWidget<ExquisHexDisplay>(mm2px(Vec(9.0, 17.0)));
		hexDisplay->box.size = mm2px(Vec(28, 46));
		hexDisplay->module = module;
		addChild(hexDisplay);

	}

};


Model* modelMicroExquis = createModel<MicroExquis, MicroExquisWidget>("MicroExquis");
