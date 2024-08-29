#include "plugin.hpp"


Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	p->addModel(modelMicroHammond);
	p->addModel(modelMicroVOctMapper);

}


MenuItem* createRangeItem(std::string label, float* gain, float* offset) {
	struct Range {
		float gain;
		float offset;

		bool operator==(const Range& other) const {
			return gain == other.gain && offset == other.offset;
		}
	};

	static const std::vector<Range> ranges = {
		{10.f, 0.f},
		{5.f, 0.f},
		{1.f, 0.f},
		{20.f, -10.f},
		{10.f, -5.f},
		{2.f, -1.f},
	};
	static std::vector<std::string> labels;
	if (labels.empty()) {
		for (const Range& range : ranges) {
			labels.push_back(string::f("%gV to %gV", range.offset, range.offset + range.gain));
		}
	}

	return createIndexSubmenuItem(label, labels,
		[=]() {
			auto it = std::find(ranges.begin(), ranges.end(), Range{*gain, *offset});
			return std::distance(ranges.begin(), it);
		},
		[=](int i) {
			*gain = ranges[i].gain;
			*offset = ranges[i].offset;
		}
	);
}