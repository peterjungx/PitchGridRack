#include "plugin.hpp"
#include "pitchgrid.hpp"
#include "datalink.hpp"

using simd::float_4;

// Accurate only on [0, 1]
template <typename T>
T sin2pi_pade_05_7_6(T x) {
	x -= 0.5f;
	return (T(-6.28319) * x + T(35.353) * simd::pow(x, 3) - T(44.9043) * simd::pow(x, 5) + T(16.0951) * simd::pow(x, 7))
	       / (1 + T(0.953136) * simd::pow(x, 2) + T(0.430238) * simd::pow(x, 4) + T(0.0981408) * simd::pow(x, 6));
}

template <typename T>
T sin2pi_pade_05_5_4(T x) {
	x -= 0.5f;
	return (T(-6.283185307) * x + T(33.19863968) * simd::pow(x, 3) - T(32.44191367) * simd::pow(x, 5))
	       / (1 + T(1.296008659) * simd::pow(x, 2) + T(0.7028072946) * simd::pow(x, 4));
}

template <typename T>
T expCurve(T x) {
	return (3 + x * (-13 + 5 * x)) / (3 + 2 * x);
}


template <int OVERSAMPLE, int QUALITY, typename T>
struct VoltageControlledSinOsc {
	bool analog = false;
	bool soft = false;
	bool syncEnabled = false;
	// For optimizing in serial code
	int channels = 0;

	T lastSyncValue = 0.f;
	T phase = 0.f;
	T freq = 0.f;
	T syncDirection = 1.f;

	dsp::TRCFilter<T> sqrFilter;

	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sinMinBlep;

	T sinValue = 0.f;

	void process(float deltaTime, T syncValue) {
		// Advance phase
		T deltaPhase = simd::clamp(freq * deltaTime, 0.f, 0.35f);
		if (soft) {
			// Reverse direction
			deltaPhase *= syncDirection;
		}
		else {
			// Reset back to forward
			syncDirection = 1.f;
		}
		phase += deltaPhase;
		// Wrap phase
		phase -= simd::floor(phase);


		// Detect sync
		// Might be NAN or outside of [0, 1) range
		if (syncEnabled) {
			T deltaSync = syncValue - lastSyncValue;
			T syncCrossing = -lastSyncValue / deltaSync;
			lastSyncValue = syncValue;
			T sync = (0.f < syncCrossing) & (syncCrossing <= 1.f) & (syncValue >= 0.f);
			int syncMask = simd::movemask(sync);
			if (syncMask) {
				if (soft) {
					syncDirection = simd::ifelse(sync, -syncDirection, syncDirection);
				}
				else {
					T newPhase = simd::ifelse(sync, (1.f - syncCrossing) * deltaPhase, phase);
					// Insert minBLEP for sync
					for (int i = 0; i < channels; i++) {
						if (syncMask & (1 << i)) {
							T mask = simd::movemaskInverse<T>(1 << i);
							float p = syncCrossing[i] - 1.f;
							T x;
							x = mask & (sin(newPhase) - sin(phase));
							sinMinBlep.insertDiscontinuity(p, x);
						}
					}
					phase = newPhase;
				}
			}
		}


		// Sin
		sinValue = sin(phase);
		sinValue += sinMinBlep.process();
	}

	T sin(T phase) {
		T v;
		if (analog) {
			// Quadratic approximation of sine, slightly richer harmonics
			T halfPhase = (phase < 0.5f);
			T x = phase - simd::ifelse(halfPhase, 0.25f, 0.75f);
			v = 1.f - 16.f * simd::pow(x, 2);
			v *= simd::ifelse(halfPhase, 1.f, -1.f);
		}
		else {
			v = sin2pi_pade_05_5_4(phase);
			// v = sin2pi_pade_05_7_6(phase);
			// v = simd::sin(2 * T(M_PI) * phase);
		}
		return v;
	}
	T sin() {
		return sinValue;
	}

	T light() {
		return simd::sin(2 * T(M_PI) * phase);
	}
};



//class ConsistentTuning {
//	int a1, b1;
//	float f1;
//	int a2, b2;
//	float f2;
//	float det;
//public:
//	ConsistentTuning(int a1, int b1, float f1, int a2, int b2, float f2) {
//		this->setParams(a1, b1, f1, a2, b2, f2);
//	};
//	void setParams(int a1, int b1, float f1, int a2, int b2, float f2) {
//		this->a1 = a1;
//		this->b1 = b1;
//		this->f1 = f1;
//		this->a2 = a2;
//		this->b2 = b2;
//		this->f2 = f2;
//		this->det = a1 * b2 - a2 * b1;
//		assert(this->det != 0.f);
//	};
//	float vecToFreqRatio(int a, int b){
//		float z1 = (- this->a2 * b + this->b2 * a) / this->det;
//		float z2 = (this->a1 * b - this->b1 * a)  / this->det;
//		float f = pow(this->f1, z1) * pow(this->f2, z2);
//		return f;
//	};
//};

const int NUM_OSCILLATORS = 9;

struct VCOMH : Module {
	enum ParamIds {
		MODE_PARAM, // removed
		SYNC_PARAM,
		FREQ_PARAM,
		FINE_PARAM, // removed
		FM_PARAM,
		LINEAR_PARAM,
		// Hammond
		AMP1_PARAM,
		AMP2_PARAM,
		AMP3_PARAM,
		AMP4_PARAM,
		AMP5_PARAM,
		AMP6_PARAM,
		AMP7_PARAM,
		AMP8_PARAM,
		AMP9_PARAM,
		RELFREQ1_PARAM,
		RELFREQ2_PARAM,
		RELFREQ4_PARAM,
		RELFREQ5_PARAM,
		RELFREQ6_PARAM,
		RELFREQ7_PARAM,
		RELFREQ8_PARAM,
		RELFREQ9_PARAM,

		NUM_PARAMS
	};
	enum InputIds {
		PITCH_INPUT,
		FM_INPUT,
		SYNC_INPUT,
		TUNING_DATA_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SIN_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(PHASE_LIGHT, 3),
		LINEAR_LIGHT,
		SOFT_LIGHT,
		NUM_LIGHTS
	};

	VoltageControlledSinOsc<16, 16, float_4> oscillators[4*NUM_OSCILLATORS];
	dsp::ClockDivider lightDivider;
	dsp::ClockDivider onceASecDivider;

	enum class TuningPresets : int {
		TUNING_12TET = 0,
		TUNING_PYTHAGOREAN = 1,
		TUNING_QUARTERCOMMA_MEANTONE = 2,
		TUNING_THIRDCOMMA_MEANTONE = 3,
		TUNING_5LIMIT_CLEANTONE = 4,
		TUNING_7LIMIT_CLEANTONE = 5,
		TUNING_19TET = 6,
		TUNING_31TET = 7,
		TUNING_HARMONIC = 8,
		TUNING_SYNCED = 9
	};

	TuningPresets tuningPreset = TuningPresets::TUNING_12TET;
	ConsistentTuning tuning = ConsistentTuning({2, 5}, 2.f, {1, 3}, pow(2.f, 7.f/12.f)); // 12TET
	RegularScale scale = RegularScale({2, 5}, 1);

	TuningDataReceiver tuningDataReceiver;


	VCOMH() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configSwitch(LINEAR_PARAM, 0.f, 1.f, 0.f, "FM mode", {"1V/octave", "Linear"});
		configSwitch(SYNC_PARAM, 0.f, 1.f, 1.f, "Sync mode", {"Soft", "Hard"});
		configParam(FREQ_PARAM, -54.f, 54.f, 0.f, "Frequency", "Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);

		configParam(RELFREQ1_PARAM, .25f, .75f, .5f, "Sub Freq", "ratio", 0.f, 100.f);
		configParam(RELFREQ2_PARAM, .375f, 1.125f, .75f, "Sub3 Freq", "ratio", 0.f, 100.f);
		configParam(RELFREQ4_PARAM, 1.f, 3.f, 2.f, "Oct Freq", "ratio", 0.f, 100.f);
		configParam(RELFREQ5_PARAM, 1.5f, 4.5f, 3.f, "Harm3 Freq", "ratio", 0.f, 100.f);
		configParam(RELFREQ6_PARAM, 2.f, 6.f, 4.f, "Oct2 Freq", "ratio", 0.f, 100.f);
		configParam(RELFREQ7_PARAM, 2.5f, 7.5f, 5.f, "Harm5 Freq", "ratio", 0.f, 100.f);
		configParam(RELFREQ8_PARAM, 3.f, 9.f, 6.f, "Harm6 Freq", "ratio", 0.f, 100.f);
		configParam(RELFREQ9_PARAM, 4.f, 12.f, 8.f, "Oct3 Freq", "ratio", 0.f, 100.f);

		configParam(AMP1_PARAM, 0.f, 1.f, 1.f, "Sub Freq Amp", "0-1", 0.f, 100.f);
		configParam(AMP2_PARAM, 0.f, 1.f, 1.f, "Sub3 Freq Amp", "0-1", 0.f, 100.f);
		configParam(AMP3_PARAM, 0.f, 1.f, 1.f, "Base Freq Amp", "0-1", 0.f, 100.f);
		configParam(AMP4_PARAM, 0.f, 1.f, 1.f, "Oct Freq Amp", "0-1", 0.f, 100.f);
		configParam(AMP5_PARAM, 0.f, 1.f, 1.f, "Harm3 Freq Amp", "0-1", 0.f, 100.f);
		configParam(AMP6_PARAM, 0.f, 1.f, 1.f, "Oct2 Freq Amp", "0-1", 0.f, 100.f);
		configParam(AMP7_PARAM, 0.f, 1.f, 1.f, "Harm5 Freq Amp", "0-1", 0.f, 100.f);
		configParam(AMP8_PARAM, 0.f, 1.f, 1.f, "Harm6 Freq Amp", "0-1", 0.f, 100.f);
		configParam(AMP9_PARAM, 0.f, 1.f, 1.f, "Oct3 Freq Amp", "0-1", 0.f, 100.f);


		configParam(FM_PARAM, -1.f, 1.f, 0.f, "Frequency modulation", "%", 0.f, 100.f);
		getParamQuantity(FM_PARAM)->randomizeEnabled = false;

		configInput(PITCH_INPUT, "1V/octave pitch");
		configInput(FM_INPUT, "Frequency modulation");
		configInput(SYNC_INPUT, "Sync");
		configInput(TUNING_DATA_INPUT, "Tuning Data");

		configOutput(SIN_OUTPUT, "Sine");

		lightDivider.setDivision(16);
		onceASecDivider.setDivision(4800);

		tuningDataReceiver.initialize();


	}

	int getTuningPreset() {
		return static_cast<int>(tuningPreset);
	}
	void setTuningPreset(int t) {
		tuningPreset = (TuningPresets)t;

		switch (tuningPreset) {
			case TuningPresets::TUNING_12TET:
				tuning.setParams({2, 5}, 2.f, {1, 0}, pow(2.f, 1.f/12.f));
				break;
			case TuningPresets::TUNING_PYTHAGOREAN:
				tuning.setParams({2, 5}, 2.f, {1, 3}, 3.f/2.f);
				break;
			case TuningPresets::TUNING_QUARTERCOMMA_MEANTONE:
				tuning.setParams({2, 5}, 2.f, {0, 2}, 5.f/4.f);
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
	
		if (tuningPreset == TuningPresets::TUNING_HARMONIC) {
			params[RELFREQ1_PARAM].setValue(0.5f);
			params[RELFREQ2_PARAM].setValue(1.5f);
			params[RELFREQ4_PARAM].setValue(2.f);
			params[RELFREQ5_PARAM].setValue(3.f);
			params[RELFREQ6_PARAM].setValue(4.f);
			params[RELFREQ7_PARAM].setValue(5.f);
			params[RELFREQ8_PARAM].setValue(6.f);
			params[RELFREQ9_PARAM].setValue(8.f);			
		}else{
			int harm5_a = tuningPreset == TuningPresets::TUNING_THIRDCOMMA_MEANTONE ? 5 : 4;
			int harm5_b = tuningPreset == TuningPresets::TUNING_THIRDCOMMA_MEANTONE ? 11 : 12;
			params[RELFREQ1_PARAM].setValue(tuning.vecToFreqRatio({-2, -5}));
			params[RELFREQ2_PARAM].setValue(tuning.vecToFreqRatio({1, 3}));
			params[RELFREQ4_PARAM].setValue(tuning.vecToFreqRatio({2, 5}));
			params[RELFREQ5_PARAM].setValue(tuning.vecToFreqRatio({3, 8}));
			params[RELFREQ6_PARAM].setValue(tuning.vecToFreqRatio({4, 10}));
			params[RELFREQ7_PARAM].setValue(tuning.vecToFreqRatio({harm5_a, harm5_b}));
			params[RELFREQ8_PARAM].setValue(tuning.vecToFreqRatio({5, 13}));
			params[RELFREQ9_PARAM].setValue(tuning.vecToFreqRatio({6, 15}));
		}
	}

	void process(const ProcessArgs& args) override {
		float freqParam = params[FREQ_PARAM].getValue() / 12.f;
		float fmParam = params[FM_PARAM].getValue();
		bool linear = params[LINEAR_PARAM].getValue() > 0.f;
		bool soft = params[SYNC_PARAM].getValue() <= 0.f;

		// Get relative frequencies
		float relFreqs[9];
		relFreqs[0] = params[RELFREQ1_PARAM].getValue();
		relFreqs[1] = params[RELFREQ2_PARAM].getValue();
		relFreqs[2] = 1.f;
		relFreqs[3] = params[RELFREQ4_PARAM].getValue();
		relFreqs[4] = params[RELFREQ5_PARAM].getValue();
		relFreqs[5] = params[RELFREQ6_PARAM].getValue();
		relFreqs[6] = params[RELFREQ7_PARAM].getValue();
		relFreqs[7] = params[RELFREQ8_PARAM].getValue();
		relFreqs[8] = params[RELFREQ9_PARAM].getValue();

		// Get amplitudes
		float amps[9];
		amps[0] = params[AMP1_PARAM].getValue();
		amps[1] = params[AMP2_PARAM].getValue();
		amps[2] = params[AMP3_PARAM].getValue();
		amps[3] = params[AMP4_PARAM].getValue();
		amps[4] = params[AMP5_PARAM].getValue();
		amps[5] = params[AMP6_PARAM].getValue();
		amps[6] = params[AMP7_PARAM].getValue();
		amps[7] = params[AMP8_PARAM].getValue();
		amps[8] = params[AMP9_PARAM].getValue();

		int channels = std::max(inputs[PITCH_INPUT].getChannels(), 1);

		for (int c = 0; c < channels; c += 4) {
			float_4 signal = 0.f;
			for (int osc=0; osc<NUM_OSCILLATORS; osc++){

				auto& oscillator = oscillators[(c / 4)*9+osc];
				oscillator.channels = std::min(channels - c, 4);
				// removed
				oscillator.analog = true;
				oscillator.soft = soft;

				// Get frequency
				float_4 pitch = freqParam + inputs[PITCH_INPUT].getPolyVoltageSimd<float_4>(c);
				float_4 freq;
				if (!linear) {
					pitch += inputs[FM_INPUT].getPolyVoltageSimd<float_4>(c) * fmParam;
					freq = dsp::FREQ_C4 * dsp::exp2_taylor5(pitch);
				}
				else {
					freq = dsp::FREQ_C4 * dsp::exp2_taylor5(pitch);
					freq += dsp::FREQ_C4 * inputs[FM_INPUT].getPolyVoltageSimd<float_4>(c) * fmParam;
				}

				freq *= relFreqs[osc];
				freq = clamp(freq, 0.f, args.sampleRate / 2.f);
				oscillator.freq = freq;

				oscillator.syncEnabled = inputs[SYNC_INPUT].isConnected();
				float_4 sync = inputs[SYNC_INPUT].getPolyVoltageSimd<float_4>(c);
				oscillator.process(args.sampleTime, sync);

				signal += amps[osc] * oscillator.sin();
	
			}
			// Set output
			if (outputs[SIN_OUTPUT].isConnected())
				outputs[SIN_OUTPUT].setVoltageSimd(.11111f * signal, c);

		}

		outputs[SIN_OUTPUT].setChannels(channels);

		// Light
		if (lightDivider.process()) {
			if (channels == 1) {
				float lightValue = oscillators[0].light()[0];
				lights[PHASE_LIGHT + 0].setSmoothBrightness(-lightValue, args.sampleTime * lightDivider.getDivision());
				lights[PHASE_LIGHT + 1].setSmoothBrightness(lightValue, args.sampleTime * lightDivider.getDivision());
				lights[PHASE_LIGHT + 2].setBrightness(0.f);
			}
			else {
				lights[PHASE_LIGHT + 0].setBrightness(0.f);
				lights[PHASE_LIGHT + 1].setBrightness(0.f);
				lights[PHASE_LIGHT + 2].setBrightness(1.f);
			}
			lights[LINEAR_LIGHT].setBrightness(linear);
			lights[SOFT_LIGHT].setBrightness(soft);

			//tuningDataReceiver.getTuningData(&tuning, &scale);
		}
		if (onceASecDivider.process()) {

			if (inputs[TUNING_DATA_INPUT].isConnected()) {
				tuningPreset = TuningPresets::TUNING_SYNCED;
				tuningDataReceiver.getTuningData(&tuning, &scale);



				// TODO: Use scale to set params



				params[RELFREQ1_PARAM].setValue(tuning.vecToFreqRatio(-scale.scale_class));
				params[RELFREQ2_PARAM].setValue(tuning.vecToFreqRatio(tuning.V1()));
				params[RELFREQ4_PARAM].setValue(tuning.vecToFreqRatio(scale.scale_class));
				params[RELFREQ5_PARAM].setValue(tuning.vecToFreqRatio(tuning.V1()+scale.scale_class));
				params[RELFREQ6_PARAM].setValue(tuning.vecToFreqRatio(scale.scale_class*2));
				params[RELFREQ7_PARAM].setValue(tuning.vecToFreqRatio(tuning.V2()+scale.scale_class*2));
				params[RELFREQ8_PARAM].setValue(tuning.vecToFreqRatio(tuning.V1()+scale.scale_class*2));
				params[RELFREQ9_PARAM].setValue(tuning.vecToFreqRatio({3*scale.scale_class.x, 3*scale.scale_class.y}));


			}
		}
		if (inputs[TUNING_DATA_INPUT].isConnected()) {
			tuningDataReceiver.processWithInput(&inputs[TUNING_DATA_INPUT]);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "tuningPreset", json_integer((int)tuningPreset));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* tuningPresetJ = json_object_get(rootJ, "tuningPreset");
		if (tuningPresetJ)
			tuningPreset = (TuningPresets)json_integer_value(tuningPresetJ);
	}
};


struct MHTuningDisplay: TuningDisplay {
	VCOMH* module;
	void step() override {
		text = "12-TET";
		if(module){
			text = module->tuningPreset == VCOMH::TuningPresets::TUNING_12TET ? "12-TET" :
				module->tuningPreset == VCOMH::TuningPresets::TUNING_PYTHAGOREAN ? "Pythagorean" :
				module->tuningPreset == VCOMH::TuningPresets::TUNING_QUARTERCOMMA_MEANTONE ? "1/4-comma Meantone" :
				module->tuningPreset == VCOMH::TuningPresets::TUNING_THIRDCOMMA_MEANTONE ? "1/3-comma Meantone" :
				module->tuningPreset == VCOMH::TuningPresets::TUNING_5LIMIT_CLEANTONE ? "5-limit (Cleantone)" :
				module->tuningPreset == VCOMH::TuningPresets::TUNING_7LIMIT_CLEANTONE ? "7-limit (m3=7/6 P5=3/2)" :
				module->tuningPreset == VCOMH::TuningPresets::TUNING_19TET ? "19-TET" :
				module->tuningPreset == VCOMH::TuningPresets::TUNING_31TET ? "31-TET" :
				module->tuningPreset == VCOMH::TuningPresets::TUNING_HARMONIC ? "Harmonic" :
				module->tuningPreset == VCOMH::TuningPresets::TUNING_SYNCED ? "SYNC" : 
				"Unknown";
		}
	};
};

struct VCOMHWidget : ModuleWidget {
	VCOMHWidget(VCOMH* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/McHammond.svg")));

		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(22.905, 29.808)), module, VCOMH::FREQ_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(6.607, 96.859)), module, VCOMH::FM_PARAM));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(17.444, 96.859)), module, VCOMH::LINEAR_PARAM, VCOMH::LINEAR_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(28.282, 96.859)), module, VCOMH::SYNC_PARAM, VCOMH::SOFT_LIGHT));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(6.607, 48.091)), module, VCOMH::AMP1_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(17.444, 48.091)), module, VCOMH::AMP2_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(28.282, 48.091)), module, VCOMH::AMP4_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(39.15, 48.091)), module, VCOMH::AMP5_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(6.607, 64.347)), module, VCOMH::AMP6_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(17.444, 64.347)), module, VCOMH::AMP7_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(28.282, 64.347)), module, VCOMH::AMP8_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(39.15, 64.347)), module, VCOMH::AMP9_PARAM));

		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(6.607, 113.115)), module, VCOMH::FM_INPUT));
		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(17.444, 113.115)), module, VCOMH::PITCH_INPUT));
		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(28.282, 113.115)), module, VCOMH::SYNC_INPUT));
		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(39.15, 96.859)), module, VCOMH::TUNING_DATA_INPUT));

		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(39.15, 113.115)), module, VCOMH::SIN_OUTPUT));

		addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(mm2px(Vec(31.089, 16.428)), module, VCOMH::PHASE_LIGHT));


		MHTuningDisplay* display = createWidget<MHTuningDisplay>(mm2px(Vec(2.0, 78.0)));
		display->box.size = mm2px(Vec(42, 7));
		display->module = module;
		addChild(display);
	}

	void appendContextMenu(Menu* menu) override {
		VCOMH* module = getModule<VCOMH>();
		assert(module);

		menu->addChild(new MenuSeparator);

		menu->addChild(createIndexSubmenuItem("Tuning",
			{
				"Original Hammond (12-TET)", 
				"Pythagorean",
				"1/4-comma Meantone", 
				"1/3-comma Meantone", 
				"5-limit (Cleantone)", 
				"7-limit (m3=7/6 P5=3/2)",
				"19-TET",
				"31-TET",
				"Harmonic",
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


Model* modelMicroHammond = createModel<VCOMH, VCOMHWidget>("MicroHammond");
