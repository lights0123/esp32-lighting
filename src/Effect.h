#pragma once
#include <FastLED.h>
FASTLED_USING_NAMESPACE
#include "ArduinoJson.h"
#include "Lighting.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <StreamUtils.h>
#include <functional>
#include <initializer_list>
#include <map>
#include <strict_variant/variant.hpp>

using EffectConfigValue = strict_variant::variant<std::string, double, uint32_t, uintptr_t, boolean>;
using EffectConfigData = std::map<uintptr_t, EffectConfigValue>;
namespace EffectConfig {
enum class DataType { String, Number, Select, Boolean, Json, Color };

struct String {
	static constexpr DataType type = DataType::String;
	constexpr String(int minLength, int maxLength, const char *defaultValue = "", bool required = false)
	: minLength(minLength), maxLength(maxLength), defaultValue(defaultValue), required(required) {
	}
	int minLength, maxLength;
	const char *defaultValue;
	bool required;
	void toJson(JsonObject &j) const {
		j["type"] = "string";
		j["minLength"] = minLength;
		j["maxLength"] = maxLength;
		j["defaultValue"] = defaultValue;
		j["required"] = required;
	}
};
struct Number {
	static constexpr DataType type = DataType::Number;
	constexpr Number(double min, double max, double stepBy = 1, double defaultValue = 0, bool required = false)
	: min(min), max(max), stepBy(stepBy), defaultValue(defaultValue), required(required) {
	}
	double min, max, stepBy;
	double defaultValue;
	bool required;
	void toJson(JsonObject &j) const {
		j["type"] = "number";
		j["min"] = min;
		j["max"] = max;
		j["stepBy"] = stepBy;
		j["defaultValue"] = defaultValue;
		j["required"] = required;
	}
};
struct Color {
	static constexpr DataType type = DataType::Color;
	constexpr Color(uint32_t defaultValue = 0, bool required = false)
	: defaultR((defaultValue >> 16) & 0xFF), defaultG((defaultValue >> 8) & 0xFF),
	  defaultB(defaultValue & 0xFF), useDefault(!((defaultValue >> 24) & 0xFF)), required(required) {
	}
	constexpr Color(uint8_t defaultR, uint8_t defaultG, uint8_t defaultB, bool useDefault = true, bool required = false)
	: defaultR(defaultR), defaultG(defaultG), defaultB(defaultB), useDefault(useDefault),
	  required(required) {
	}
	uint8_t defaultR, defaultG, defaultB;
	bool useDefault;
	bool required;
	void toJson(JsonObject &j) const {
		j["type"] = "color";
		if(useDefault) {
			j["defaultR"] = defaultR;
			j["defaultG"] = defaultG;
			j["defaultB"] = defaultB;
		}
		j["required"] = required;
	}
};
struct Select {
	static constexpr DataType type = DataType::Select;
	constexpr Select(const char *const *options, uintptr_t optionCount, uintptr_t defaultValue, bool required = false)
	: options(options), optionCount(optionCount), defaultValue(defaultValue), required(required) {
	}
	constexpr Select(std::initializer_list<const char *> options, uintptr_t defaultValue, bool required = false)
	: options(options.begin()), optionCount(options.size()), defaultValue(defaultValue),
	  required(required) {
	}
	const char *const *options;
	uintptr_t optionCount;
	uintptr_t defaultValue;
	bool required;
	void toJson(JsonObject &j) const {
		j["type"] = "select";
		auto jsonOptions = j.createNestedArray("options");
		for(auto i = 0; i < optionCount; i++) {
			jsonOptions.add(options[i]);
		}
		j["defaultValue"] = defaultValue;
		j["required"] = required;
	}
};
struct Boolean {
	static constexpr DataType type = DataType::Boolean;
	constexpr Boolean(bool defaultValue = false) : defaultValue(defaultValue) {
	}
	bool defaultValue;
	void toJson(JsonObject &j) const {
		j["type"] = "boolean";
		j["defaultValue"] = defaultValue;
	}
};
struct Json {
	static constexpr DataType type = DataType::Json;
	constexpr Json(const char *defaultValue, bool required = false)
	: defaultValue(defaultValue), required(required) {
	}
	const char *defaultValue;
	bool required;
	void toJson(JsonObject &j) const {
		j["type"] = "json";
		j["defaultValue"] = defaultValue;
		j["required"] = required;
	}
};
union Specs {
	constexpr Specs(String str) : str(str){};
	constexpr Specs(Number num) : num(num){};
	constexpr Specs(Color col) : col(col){};
	constexpr Specs(Select sel) : sel(sel){};
	constexpr Specs(Boolean boolean) : boolean(boolean){};
	constexpr Specs(Json json) : json(json){};
	String str;
	Number num;
	Color col;
	Select sel;
	Boolean boolean;
	Json json;
};
struct Configuration {
	const char *title;
	const char *description;
	DataType type;
	Specs specs;
	void toJson(JsonObject &j) const {
		j["title"] = title;
		j["description"] = description;
		if(type == String::type)
			specs.str.toJson(j);
		else if(type == Number::type)
			specs.num.toJson(j);
		else if(type == Color::type)
			specs.col.toJson(j);
		else if(type == Select::type)
			specs.sel.toJson(j);
		else if(type == Boolean::type)
			specs.boolean.toJson(j);
		else if(type == Json::type)
			specs.json.toJson(j);
	}
};
template <typename T>
constexpr Configuration create(const char *title, const char *description, T data) {
	return Configuration{ title, description, T::type, EffectConfig::Specs(data) };
}
} // namespace EffectConfig

struct Effect {
	CRGB *pixels;
	uintptr_t len;
	EffectConfigData configData;
	Effect(CRGB *pixels, uintptr_t len, EffectConfigData &configData)
	: pixels(pixels), len(len), configData(configData) {
	}
	virtual void updateConfig(EffectConfigData &configData) {
		this->configData = configData;
	}
	virtual void display() = 0;
	virtual ~Effect(){};
};

struct EffectCreator {
	std::function<Effect *(CRGB *pixels, uintptr_t len, EffectConfigData &config)> createFunction;
	Effect *create(CRGB *pixels, uintptr_t len, EffectConfigData &config) const {
		return createFunction(pixels, len, config);
	}
	Effect *create(const GenericLightStrip *strip, EffectConfigData &config) const {
		return create(strip->data, strip->len, config);
	}
	const char *name;
	const EffectConfig::Configuration *config;
	uintptr_t configLength;
};

template <typename T> EffectCreator addEffect() {
	EffectCreator e;
	e.createFunction = [](CRGB *pixels, uintptr_t len, EffectConfigData &config) {
		return new T(pixels, len, config);
	};
	e.name = T::name;
	e.config = T::config;

	e.configLength = sizeof(T::config) / sizeof(*T::config);
	return e;
}


struct RainbowEffect : Effect {
	static constexpr const char *const name = "Rainbow";
	static constexpr const EffectConfig::Configuration config[] = {
		EffectConfig::create("Speed", "Animation Speed", EffectConfig::Number(1, 50, 1, 1, true)),
	};
	RainbowEffect(CRGB *pixels, uintptr_t len, EffectConfigData &configData)
	: Effect(pixels, len, configData) {
	}
	void display() {
		for(auto i = 0; i < len; i++) {
			pixels[i] =
			CHSV(millis() / (unsigned long)(*strict_variant::get<double>(&configData[0])) % 256, 255, 255);
		}
	}
};
// https://stackoverflow.com/a/8016853/4471524
constexpr EffectConfig::Configuration RainbowEffect::config[];


struct Rainbow2Effect : Effect {
	static constexpr const char *const name = "Rainbow2";
	static constexpr const EffectConfig::Configuration config[] = {
		EffectConfig::create("Speed", "Animation Speed", EffectConfig::Number(1, 50, 1, 1, true)),
	};
	Rainbow2Effect(CRGB *pixels, uintptr_t len, EffectConfigData &configData)
	: Effect(pixels, len, configData) {
	}
	void display() {
		uint8_t hue = millis() / (unsigned long)(*strict_variant::get<double>(&configData[0])) % 256;
		for(auto i = 0; i < len; i++) {
			pixels[i] = CHSV(hue, 255, 255);
			hue += 10;
		}
	}
};
// https://stackoverflow.com/a/8016853/4471524
constexpr EffectConfig::Configuration Rainbow2Effect::config[];


struct SolidEffect : Effect {
	static constexpr const char *const name = "Solid";
	static constexpr const EffectConfig::Configuration config[] = {
		EffectConfig::create("Color", "Animation Speed", EffectConfig::Color(0xFF0000, true)),
	};
	SolidEffect(CRGB *pixels, uintptr_t len, EffectConfigData &configData)
	: Effect(pixels, len, configData) {
	}
	void display() {
		fill_solid(pixels, len, CRGB(*strict_variant::get<uint32_t>(&configData[0])));
	}
};
// https://stackoverflow.com/a/8016853/4471524
constexpr EffectConfig::Configuration SolidEffect::config[];


struct RedGreenEffect : Effect {
	static constexpr const char *const name = "Red and Green";
	static constexpr const EffectConfig::Configuration config[] = {
		EffectConfig::create("Speed", "Animation Speed", EffectConfig::Number(1, 50, 1, 1, true)),
	};
	RedGreenEffect(CRGB *pixels, uintptr_t len, EffectConfigData &configData)
	: Effect(pixels, len, configData) {
	}
	void display() {
		uintptr_t offset =
		255 - (millis() / ((unsigned long)(*strict_variant::get<double>(&configData[0])) * 100) % 256);
		for(auto i = 0; i < len; i++) {
			auto loc = (i + offset) % 16;
			auto color = CRGB::Black;
			if(loc < 5) {
				color = CRGB::Green;
			} else if(loc > 7 && loc < 13) {
				color = CRGB::Red;
			}
			pixels[i] = color;
		}
	}
};
// https://stackoverflow.com/a/8016853/4471524
constexpr EffectConfig::Configuration RedGreenEffect::config[];


struct BounceEffect : Effect {
	static constexpr const char *const name = "Bounce";
	static constexpr const EffectConfig::Configuration config[] = {
		EffectConfig::create("Speed", "Animation Speed", EffectConfig::Number(1, 50, 1, 1, true)),
	};
	uintptr_t loc = 0;
	BounceEffect(CRGB *pixels, uintptr_t len, EffectConfigData &configData)
	: Effect(pixels, len, configData) {
	}
	void display() {
		fill_solid(pixels, len, CRGB::Black);
		pixels[loc] = CRGB::White;
		loc++;
		if(loc >= len) loc = 0;
	}
};
// https://stackoverflow.com/a/8016853/4471524
constexpr EffectConfig::Configuration BounceEffect::config[];