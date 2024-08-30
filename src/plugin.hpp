#include <rack.hpp>


using namespace rack;


extern Plugin* pluginInstance;

extern Model* modelMicroHammond;
extern Model* modelMicroVOctMapper;

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
