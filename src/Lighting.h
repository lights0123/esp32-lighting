#pragma once
#include <FastLED.h>
#include <functional>
struct GenericLightStrip {
	const char *name;
	CRGB *data;
	uintptr_t len;

	std::function<void(CRGB *, uintptr_t)> initFunc;
	void init() const {
		initFunc(data, len);
	}

	GenericLightStrip(const char *name, uintptr_t len, std::function<void(CRGB *, uintptr_t)> initFunc)
	: name(name), len(len), initFunc(initFunc) {
	}
};
template <unsigned int N> struct LightStrip : GenericLightStrip {
	CRGB internalData[N];
	LightStrip(const char *name, std::function<void(CRGB *, uintptr_t)> initFunc)
	: GenericLightStrip(name, N, initFunc) {
		data = internalData;
	}
};
