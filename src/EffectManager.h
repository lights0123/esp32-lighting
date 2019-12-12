#pragma once
#include "Configuration.h"
#include "Effect.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <cmath>
#include <map>
#include <memory>
#include <strict_variant/variant.hpp>
#include <string>
#include <vector>

boolean verifyNumber(double num, EffectConfig::Number &numberConfig) {
	return num >= numberConfig.min && num <= numberConfig.max &&
	       std::fmod(num - numberConfig.min, numberConfig.stepBy) == 0 && !std::isnan(num);
}
uint32_t packColor(uint8_t r, uint8_t g, uint8_t b) {
	return (r << 16) | (g << 8) | b;
}
class EffectManager {
	FS &fs;
	//      Strip index        Effect Index   Effect Data
	std::map<uintptr_t, std::map<uintptr_t, EffectConfigData>> stripEffectConfig;
	std::map<uintptr_t, std::map<uintptr_t, std::unique_ptr<Effect>>> effects;
	AsyncWebSocketMessageBuffer serializedConfig;

public:
	EffectManager() : fs(SPIFFS) {
	}
	AsyncWebSocketMessageBuffer *getSerializedConfig() {
		return &serializedConfig;
	};
	void begin() {
		SPIFFS.begin();
		auto storageSize = 2048;
		DynamicJsonDocument doc(storageSize);
		DeserializationError err = DeserializationError::InvalidInput;
		for(auto &strip : Configuration::strips) {
			stripEffectConfig[&strip - &Configuration::strips[0]];
			effects[&strip - &Configuration::strips[0]];
		}
		do {
			File file = fs.open("/effects.msgpack");
			if(!file || file.isDirectory()) {
				break;
			}
			ReadBufferingStream bufferingStream(file, 64);
			err = deserializeMsgPack(doc, bufferingStream);
			if(err == DeserializationError::NoMemory) {
				storageSize *= 2;
				if(storageSize > 1048576) {
					break;
				}
				doc = DynamicJsonDocument(storageSize);
				if(!doc.size()) {
					break;
				}
			}
		} while(err == DeserializationError::NoMemory);
		if(err == DeserializationError::Ok) {
			for(auto &strip : Configuration::strips) {
				auto stripConfig = doc[strip.name];
				if(!stripConfig.is<JsonObject>()) continue;
				for(auto &effect : Configuration::effects) {
					auto i = &effect - &Configuration::effects[0];
					auto effectConfig = stripConfig[effect.name];
					if(!effectConfig.is<JsonObject>()) continue;
					updateEffectConfig(&strip - &Configuration::strips[0], i, effectConfig.as<JsonObject>());
				}
			}
		}
		serializeConfig();
	}
	void removeEffectConfig(uintptr_t stripIndex, uintptr_t effectIndex) {
		auto strip = stripEffectConfig.find(stripIndex);
		if(strip == stripEffectConfig.end()) return;
		effects[stripIndex].erase(effectIndex);
		strip->second.erase(effectIndex);
	}
	bool updateEffectConfig(uintptr_t stripIndex, uintptr_t effectIndex, JsonObjectConst effectConfig) {
		auto effect = Configuration::effects[effectIndex];
		EffectConfigData config;
		bool ok = true;
		for(auto iConfig = 0; iConfig < effect.configLength; iConfig++) {
			using EffectConfig::DataType;
			auto &effectConfiguration = effect.config[iConfig];
			auto incomingValue = effectConfig[effectConfiguration.title];
			auto type = effectConfiguration.type;
			if(type == DataType::String) {
				auto stringConfig = effectConfiguration.specs.str;
				if(incomingValue.is<const char *>()) {
					auto str = incomingValue.as<const char *>();
					auto len = strlen(str);
					if(len >= stringConfig.minLength && len <= stringConfig.maxLength) {
						config[iConfig] = str;
						continue;
					}
				}
				if(!stringConfig.required) {
					continue;
				} else {
					auto defaultLength = strlen(stringConfig.defaultValue);
					if(defaultLength >= stringConfig.minLength && defaultLength <= stringConfig.maxLength) {
						config[iConfig] = stringConfig.defaultValue;
						continue;
					}
				}
			} else if(type == DataType::Number) {
				auto numberConfig = effectConfiguration.specs.num;
				if(incomingValue.is<double>()) {
					double num = incomingValue.as<double>();
					if(verifyNumber(num, numberConfig)) {
						config[iConfig] = num;
						continue;
					}
				}
				if(!numberConfig.required) {
					continue;
				} else {
					auto defaultValue = numberConfig.defaultValue;
					if(verifyNumber(defaultValue, numberConfig)) {
						config[iConfig] = defaultValue;
						continue;
					}
				}
			} else if(type == DataType::Color) {
				auto colorConfig = effectConfiguration.specs.col;
				if(incomingValue.is<JsonObject>()) {
					if(incomingValue["r"].is<uint8_t>() && incomingValue["g"].is<uint8_t>() &&
					   incomingValue["b"].is<uint8_t>()) {
						config[iConfig] =
						packColor(incomingValue["r"].as<uint8_t>(), incomingValue["g"].as<uint8_t>(),
						          incomingValue["b"].as<uint8_t>());
						continue;
					}
				}
				if(colorConfig.required) {
					config[iConfig] =
					packColor(colorConfig.defaultR, colorConfig.defaultG, colorConfig.defaultB);
				}
				continue;
			} else if(type == DataType::Select) {
				auto selectConfig = effectConfiguration.specs.sel;
				if(incomingValue.is<const char *>()) {
					auto str = incomingValue.as<const char *>();
					uintptr_t num = UINTPTR_MAX;
					for(auto i = 0; i < selectConfig.optionCount; i++) {
						if(strcmp(str, selectConfig.options[i]) == 0) {
							num = i;
							break;
						}
					}
					if(num < UINTPTR_MAX) {
						config[iConfig] = num;
						continue;
					}
				}
				if(!selectConfig.required) {
					continue;
				} else {
					auto defaultValue = selectConfig.defaultValue;
					if(defaultValue < selectConfig.optionCount) {
						config[iConfig] = defaultValue;
						continue;
					}
				}
			} else if(type == DataType::Boolean) {
				auto boolConfig = effectConfiguration.specs.boolean;
				if(incomingValue.is<bool>()) {
					config[iConfig] = incomingValue.as<bool>();
				} else {
					config[iConfig] = boolConfig.defaultValue;
				}
				continue;
			} else if(type == DataType::Json) {
				auto stringConfig = effectConfiguration.specs.json;
				if(incomingValue.is<const char *>()) {
					config[iConfig] = incomingValue.as<const char *>();
					continue;
				}
				if(!stringConfig.required) {
					continue;
				} else {
					if(*stringConfig.defaultValue) {
						config[iConfig] = stringConfig.defaultValue;
						continue;
					}
				}
			}
			ok = false;
			break;
		}
		if(ok) {
			stripEffectConfig[stripIndex][effectIndex] = config;
			auto effect = effects[stripIndex].find(effectIndex);
			Serial.println(__LINE__);
			if(effect == effects[stripIndex].end()) {
				Serial.println("create new");
				effects[stripIndex][effectIndex] = std::move(std::unique_ptr<Effect>(
				Configuration::effects[effectIndex].create(&Configuration::strips[stripIndex], config)));
			} else {
				effect->second->updateConfig(config);
			}
		} else {
			removeEffectConfig(stripIndex, effectIndex);
		}
		return ok;
	}
	void serializeConfig() {
		using namespace strict_variant;
		DynamicJsonDocument doc(2048);
		for(std::pair<uintptr_t, std::map<uintptr_t, EffectConfigData>> strip : stripEffectConfig) {
			auto stripConfig = doc.createNestedObject(Configuration::strips[strip.first].name);
			for(std::pair<uintptr_t, EffectConfigData> effect : strip.second) {
				auto effectConfig =
				stripConfig.createNestedObject(Configuration::effects[effect.first].name);
				for(std::pair<uintptr_t, EffectConfigValue> config : effect.second) {
					auto *data = &config.second;
					auto configSettings = Configuration::effects[effect.first].config[config.first];
					if(configSettings.type == EffectConfig::DataType::String) {
						effectConfig[configSettings.title] = (char *)(get<std::string>(data)->c_str());
					} else if(configSettings.type == EffectConfig::DataType::Number) {
						effectConfig[configSettings.title] = *get<double>(data);
					} else if(configSettings.type == EffectConfig::DataType::Color) {
						auto color = effectConfig.createNestedObject(configSettings.title);
						auto packedColor = *get<uint32_t>(data);
						color["r"] = (packedColor >> 16) & 0xFF;
						color["g"] = (packedColor >> 8) & 0xFF;
						color["b"] = packedColor & 0xFF;
					} else if(configSettings.type == EffectConfig::DataType::Select) {
						auto index = *get<uintptr_t>(data);
						effectConfig[configSettings.title] = configSettings.specs.sel.options[index];
					} else if(configSettings.type == EffectConfig::DataType::Boolean) {
						effectConfig[configSettings.title] = *get<bool>(data);
					} else if(configSettings.type == EffectConfig::DataType::Json) {
						effectConfig[configSettings.title] = (char *)(get<std::string>(data)->c_str());
					}
				}
			}
		}
		doc["type"] = "config";
		size_t len = measureMsgPack(doc);
		if(serializedConfig.reserve(len)) {
			serializeMsgPack(doc, (char *)serializedConfig.get(), len + 1);
		}
	}
	void saveConfig() {
		File file = fs.open("/effects.msgpack", FILE_WRITE);
		WriteBufferingStream bufferedFile(file, 64);
		for(auto i = 0; i < serializedConfig.length(); i++) {
			bufferedFile.write(serializedConfig.get()[i]);
		}
		bufferedFile.flush();
	}
	void run() {
		for(auto &strip : Configuration::strips) {
			auto stripIndex = &strip - &Configuration::strips[0];
			auto firstEffect = effects[stripIndex].begin();
			if(firstEffect!=effects[stripIndex].end()) {
				firstEffect->second->display();
			} else {
				fill_solid(strip.data, strip.len, CRGB::Black);
			}
		}
	}
};