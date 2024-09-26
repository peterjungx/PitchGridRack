#include <rack.hpp>


using namespace rack;


extern Plugin* pluginInstance;

extern Model* modelMicroHammond;
extern Model* modelMicroVOctMapper;
extern Model* modelMicroExquis;

struct DigitalDisplay : Widget {
	std::string fontPath;
	std::string bgText;
	std::string text;
	float fontSize;
	NVGcolor bgColor = nvgRGB(0x46,0x46, 0x46);
	NVGcolor fgColor = SCHEME_YELLOW;
	Vec textPos;

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

	void draw(const DrawArgs& args) override {
		// Background
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
		nvgFillColor(args.vg, nvgRGB(0x19, 0x19, 0x19));
		nvgFill(args.vg);

		prepareFont(args);

		// Background text
		nvgFillColor(args.vg, bgColor);
		nvgText(args.vg, textPos.x, textPos.y, bgText.c_str(), NULL);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			prepareFont(args);

			// Foreground text
			nvgFillColor(args.vg, fgColor);
			nvgText(args.vg, textPos.x, textPos.y, text.c_str(), NULL);
		}
		Widget::drawLayer(args, layer);
	}
};


struct TuningDisplay : DigitalDisplay {
	TuningDisplay() {
		fontPath = asset::plugin(pluginInstance, "res/fonts/PTSans.ttc");
		textPos = Vec(62, 10);
		bgText = "18";
		fontSize = 14;
	}
};

struct DigitalFourlineDisplay : Widget {
	std::string fontPath;
	std::string text1;
	std::string text2;
	std::string text3;
	std::string text4;
	std::string text5;
	float fontSize;
	NVGcolor bgColor = nvgRGB(0x46,0x46, 0x46);
	NVGcolor fgColor = SCHEME_YELLOW;
	Vec textPos1;
	Vec textPos2;
	Vec textPos3;
	Vec textPos4;
	Vec textPos5;

	void prepareFont(const DrawArgs& args) {
		// Get font
		std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		if (!font)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, fontSize);
		nvgTextLetterSpacing(args.vg, -0.7);
	}

	void prepareFontCentered(const DrawArgs& args){
		prepareFont(args);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	}
	void prepareFontLeft(const DrawArgs& args){
		prepareFont(args);
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	}


	void draw(const DrawArgs& args) override {
		// Background
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
		nvgFillColor(args.vg, nvgRGB(0x19, 0x19, 0x19));
		nvgFill(args.vg);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {

			// Foreground text
			nvgFillColor(args.vg, fgColor);
			prepareFontCentered(args);
			nvgText(args.vg, textPos1.x, textPos1.y, text1.c_str(), NULL);
			prepareFontLeft(args);
			nvgText(args.vg, textPos2.x, textPos2.y, text2.c_str(), NULL);
			nvgText(args.vg, textPos3.x, textPos3.y, text3.c_str(), NULL);
			nvgText(args.vg, textPos4.x, textPos4.y, text4.c_str(), NULL);
			nvgText(args.vg, textPos5.x, textPos5.y, text5.c_str(), NULL);
		}
		Widget::drawLayer(args, layer);
	}
};

struct ExquisDisplay : DigitalFourlineDisplay {
	ExquisDisplay() {
		fontPath = asset::plugin(pluginInstance, "res/fonts/PTSans.ttc");
		textPos1 = Vec(62, 8);
		textPos2 = Vec(5, 22);
		textPos3 = Vec(5, 36);
		textPos4 = Vec(5, 50);
		textPos5 = Vec(5, 64);
		fontSize = 14;
	}
};



template <typename TBase = GrayModuleLightWidget>
struct YellowRedLight : TBase {
	YellowRedLight() {
		this->addBaseColor(SCHEME_YELLOW);
		this->addBaseColor(SCHEME_RED);
	}
};


template <typename TBase = GrayModuleLightWidget>
struct YellowBlueLight : TBase {
	YellowBlueLight() {
		this->addBaseColor(SCHEME_YELLOW);
		this->addBaseColor(SCHEME_BLUE);
	}
};


struct VCVBezelBig : app::SvgSwitch {
	VCVBezelBig() {
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/VCVBezelBig.svg")));
	}
};


template <typename TBase>
struct VCVBezelLightBig : TBase {
	VCVBezelLightBig() {
		this->borderColor = color::BLACK_TRANSPARENT;
		this->bgColor = color::BLACK_TRANSPARENT;
		this->box.size = mm2px(math::Vec(11.1936, 11.1936));
	}
};


MenuItem* createRangeItem(std::string label, float* gain, float* offset);
