#pragma once
#include <rack.hpp>

using namespace rack;

//extern Plugin* pluginInstance;

struct ExquisDisplay : Widget {
	//std::string fontPath;
	std::string scalesystem_label = "Scale System";
	std::string scalesystem_text = "5;2";
	std::string scalemode_label = "Scale Mode";
	std::string scalemode_text = "c2";	
	std::string tuningvector1_label =  "Tuning Vector 1";
	std::string tuningvector1_coord_text = "5;2";
	std::string tuningvector1_fr_text = "1200.0ct (2/1)";
	std::string tuningvector2_label =  "Tuning Vector 2";
	std::string tuningvector2_coord_text = "3;1";
	std::string tuningvector2_fr_text = "702.0ct (3/2)";
	std::string tuningbase_label = "Tuning Base";
	std::string tuningbase_text = "261.63Hz (0.000V)";

	std::string lastnote_label = "Last Note";
	std::string lastnote_name_text = "";
	std::string lastnote_text = "";

	float fontSize;
	float textOffsetY;
	NVGcolor bgColor = nvgRGB(0x19, 0x19, 0x19);
	NVGcolor labelColor = SCHEME_YELLOW;
	NVGcolor textColor = nvgRGB(127+14, 127+113, 127+127); // {14,113,127}
	Vec labelPos1;

	ExquisDisplay() {
		//fontPath = asset::plugin(pluginInstance, "res/fonts/PTSans.ttc");
		fontSize = 14;
		textOffsetY = 27;
		labelPos1 = Vec(3, 17);
	}

	void prepareFont(const DrawArgs& args) {
		// Get font
		//std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		//if (!font) return;
		//nvgFontFaceId(args.vg, font->handle);
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

	void prepareFontRight(const DrawArgs& args){
		prepareFont(args);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
	}

	void draw(const DrawArgs& args) override {
		// Background
		nvgBeginPath(args.vg);
		//nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgFillColor(args.vg, bgColor);
		nvgFill(args.vg);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			// Foreground text
			
			prepareFontLeft(args);
			nvgFillColor(args.vg, labelColor);
			nvgText(args.vg, labelPos1.x, labelPos1.y, scalesystem_label.c_str(), NULL);
			nvgText(args.vg, labelPos1.x, labelPos1.y + 1*textOffsetY, scalemode_label.c_str(), NULL);
			nvgText(args.vg, labelPos1.x, labelPos1.y + 2*textOffsetY, tuningvector1_label.c_str(), NULL);
			nvgText(args.vg, labelPos1.x, labelPos1.y + 3.7*textOffsetY, tuningvector2_label.c_str(), NULL);
			nvgText(args.vg, labelPos1.x, labelPos1.y + 5.4*textOffsetY, tuningbase_label.c_str(), NULL);
			nvgText(args.vg, labelPos1.x, labelPos1.y + 7.1*textOffsetY, lastnote_label.c_str(), NULL);

			nvgFillColor(args.vg, textColor);

			nvgText(args.vg, labelPos1.x + 4, labelPos1.y + 2.7*textOffsetY , tuningvector1_fr_text.c_str(), NULL);
			nvgText(args.vg, labelPos1.x + 4, labelPos1.y + 4.4*textOffsetY , tuningvector2_fr_text.c_str(), NULL);
			nvgText(args.vg, labelPos1.x + 4, labelPos1.y + 6.1*textOffsetY , tuningbase_text.c_str(), NULL);
			nvgText(args.vg, labelPos1.x + 4, labelPos1.y + 7.8*textOffsetY , lastnote_text.c_str(), NULL);
			
			prepareFontRight(args);
			nvgText(args.vg, box.size.x - labelPos1.x, labelPos1.y + 0*textOffsetY, scalesystem_text.c_str(), NULL);
			nvgText(args.vg, box.size.x - labelPos1.x, labelPos1.y + 1*textOffsetY, scalemode_text.c_str(), NULL);
			nvgText(args.vg, box.size.x - labelPos1.x, labelPos1.y + 2*textOffsetY, tuningvector1_coord_text.c_str(), NULL);
			nvgText(args.vg, box.size.x - labelPos1.x, labelPos1.y + 3.7*textOffsetY, tuningvector2_coord_text.c_str(), NULL);
			nvgText(args.vg, box.size.x - labelPos1.x, labelPos1.y + 7.1*textOffsetY, lastnote_name_text.c_str(), NULL);

		}
		Widget::drawLayer(args, layer);
	}
};



