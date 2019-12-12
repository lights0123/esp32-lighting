#pragma once
#include "Effect.h"
#include "Lighting.h"
#include <FastLED.h>
namespace Configuration {
CRGB leds[840];

const GenericLightStrip strips[] = { LightStrip<840>("default", [](CRGB *leds, uintptr_t count) {
	FastLED.addLeds<WS2812B, 12, BGR>(leds, count);
}) };

const EffectCreator effects[] = { addEffect<RainbowEffect>(), addEffect<Rainbow2Effect>(),
	                              addEffect<SolidEffect>(), addEffect<RedGreenEffect>(), addEffect<BounceEffect>() };
} // namespace Configuration