/*
   Tree v2: https://github.com/evilgeniuslabs/tree-v2
   Copyright (C) 2016 Jason Coon

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include <Preferences.h>
FASTLED_USING_NAMESPACE

#include "esp_wifi.h"
#include "time.h"
#include <AsyncTCP.h>
#include <Dusk2Dawn.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include "ServeStatic.h"
#include "ArduinoJson.h"

#include "Configuration.h"
#include "EffectManager.h"

Dusk2Dawn sunTimes(41.481454, -81.566639, 0);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
#define MILLI_AMPS 80000
#define FRAMES_PER_SECOND 240


AsyncWebSocketMessageBuffer wifiList;
AsyncWebSocketMessageBuffer effectsList;
AsyncWebSocketMessageBuffer globalStats;


EffectManager effectManager;
Preferences prefs;

uint8_t brightness = 30;
enum class LightStat { OFF, ON, ON_INIT };
LightStat lightStat = LightStat::ON_INIT;
bool followSun = true;
void updateGlobalStats() {
	StaticJsonDocument<256> doc;
	doc["type"] = "globalStats";
	doc["brightness"] = brightness;
	doc["on"] = lightStat != LightStat::OFF;
	doc["followSun"] = followSun;
	prefs.putUChar("brightness", brightness);
	prefs.putBool("followSun", followSun);
	size_t len = measureMsgPack(doc);
	if(globalStats.reserve(len)) {
		serializeMsgPack(doc, (char *)globalStats.get(), len + 1);
		ws.binaryAll(&globalStats);
	}
}
void handleMessage(JsonObjectConst doc) {
	auto type = doc["type"].as<const char *>();
	if(!type) return;
	if(strcmp(type, "removeEffect") == 0) {
		auto stripName = doc["strip"].as<const char *>();
		auto effectName = doc["effect"].as<const char *>();
		if(!stripName || !effectName) return;
		for(auto &strip : Configuration::strips) {
			if(strcmp(strip.name, stripName) != 0) continue;
			for(auto &effect : Configuration::effects) {
				if(strcmp(effect.name, effectName) != 0) continue;
				effectManager.removeEffectConfig(&strip - &Configuration::strips[0],
				                                 &effect - &Configuration::effects[0]);
				effectManager.serializeConfig();
				effectManager.saveConfig();
				break;
			}
			break;
		}
	} else if(strcmp(type, "updateEffect") == 0) {
		if(!doc["config"].is<JsonObject>()) return;
		auto stripName = doc["strip"].as<const char *>();
		auto effectName = doc["effect"].as<const char *>();
		if(!stripName || !effectName) return;
		for(auto &strip : Configuration::strips) {
			if(strcmp(strip.name, stripName) != 0) continue;
			for(auto &effect : Configuration::effects) {
				if(strcmp(effect.name, effectName) != 0) continue;
				effectManager.updateEffectConfig(&strip - &Configuration::strips[0],
				                                 &effect - &Configuration::effects[0],
				                                 doc["config"].as<JsonObjectConst>());
				effectManager.serializeConfig();
				effectManager.saveConfig();
				return;
			}
		}
	} else if(strcmp(type, "updateGlobal") == 0) {
		brightness = doc["brightness"] | brightness;
		lightStat = (doc["on"] | (lightStat != LightStat::OFF)) ? LightStat::ON : LightStat::OFF;
		followSun = doc["followSun"] | followSun;
		updateGlobalStats();
	}
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
	if(type == WS_EVT_CONNECT) {
		Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
		client->printf("Hello Client %u :)", client->id());
		if(wifiList.length()) {
			client->binary(&wifiList);
		}
		if(globalStats.length()) {
			client->binary(&globalStats);
		}
		auto buf = effectManager.getSerializedConfig();
		if(buf->length()) {
			client->binary(buf);
		}
		// TODO
		DynamicJsonDocument doc(2048);
		doc["type"] = "effectConfig";
		JsonArray data = doc.createNestedArray("effects");
		for(auto effect : Configuration::effects) {
			auto network = data.createNestedObject();
			network["name"] = effect.name;
			auto config = network.createNestedArray("config");
			for(auto i = 0; i < effect.configLength; i++) {
				auto configuration = config.createNestedObject();
				effect.config[i].toJson(configuration);
			}
		}
		size_t len = measureMsgPack(doc);
		AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
		if(buffer) {
			serializeMsgPack(doc, (char *)buffer->get(), len + 1);
			client->binary(buffer);
		}
		client->ping();
	} else if(type == WS_EVT_DISCONNECT) {
		Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
	} else if(type == WS_EVT_ERROR) {
		Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg),
		              (char *)data);
	} else if(type == WS_EVT_PONG) {
		Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
	} else if(type == WS_EVT_DATA) {
		AwsFrameInfo *info = (AwsFrameInfo *)arg;
		String msg = "";
		if(info->final && info->index == 0 && info->len == len) {
			// the whole message is in a single frame and we got all of it's data
			Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(),
			              (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

			if(info->opcode == WS_TEXT) {
				for(size_t i = 0; i < info->len; i++) {
					Serial.print((char)data[i]);
				}
			} else {
				for(size_t i = 0; i < info->len; i++) {
					Serial.printf("%02x ", (uint8_t)data[i]);
				}
				auto storageSize = 2048;
				DynamicJsonDocument doc(storageSize);
				DeserializationError err = DeserializationError::InvalidInput;
				do {
					err = deserializeMsgPack(doc, data, info->len);
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
					handleMessage(doc.as<JsonObjectConst>());
				}
			}
			Serial.println();
		} else {
			// message is comprised of multiple frames or the frame is split into multiple packets
			if(info->index == 0) {
				if(info->num == 0)
					Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(),
					              (info->message_opcode == WS_TEXT) ? "text" : "binary");
				Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(),
				              info->num, info->len);
			}

			Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(),
			              info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary",
			              info->index, info->index + len);

			if(info->opcode == WS_TEXT) {
				for(size_t i = 0; i < len; i++) {
					msg += (char)data[i];
				}
			} else {
				char buff[3];
				for(size_t i = 0; i < len; i++) {
					sprintf(buff, "%02x ", (uint8_t)data[i]);
					msg += buff;
				}
			}
			Serial.printf("%s\n", msg.c_str());

			if((info->index + len) == info->len) {
				Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(),
				              info->num, info->len);
				if(info->final) {
					Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(),
					              (info->message_opcode == WS_TEXT) ? "text" : "binary");
					if(info->message_opcode == WS_TEXT) client->text("I got your text message");
				}
			}
		}
	}
}

void setup() {
	pinMode(14, OUTPUT);
	digitalWrite(14, HIGH);
	esp_wifi_set_ps(WIFI_PS_NONE);
	Serial.begin(115200);
	prefs.begin("esp32_lighting");
	brightness = prefs.getUChar("brightness", 30);
	followSun = prefs.getBool("followSun", true);
	updateGlobalStats();
	delay(100);

	for(const auto strip : Configuration::strips) {
		strip.init();
	}
	FastLED.setDither(true);
	FastLED.setCorrection(Typical8mmPixel);
	FastLED.setBrightness(brightness);
	// FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
	for(const auto strip : Configuration::strips) {
		fill_solid(strip.data, strip.len, CRGB::Black);
	}
	FastLED.show();
	effectManager.begin();
	effectManager.saveConfig();
	Serial.println();
	Serial.println();

	SPIFFS.begin();
	String lastModifiedTime;
	{
		auto file = SPIFFS.open("/www/_nuxt/last-modified.txt");
		if(file && !file.isDirectory()) {
			lastModifiedTime = file.readString();
		}
	}
	WiFi.onEvent(
	[](WiFiEvent_t event, WiFiEventInfo_t info) {
		const auto networkCount = WiFi.scanComplete();
		Serial.println(networkCount);
		if(networkCount < 0) return;
		DynamicJsonDocument doc(JSON_ARRAY_SIZE(networkCount) + JSON_OBJECT_SIZE(2) +
		                        networkCount * JSON_OBJECT_SIZE(3) + networkCount * 47);
		doc["type"] = "scan";
		JsonArray data = doc.createNestedArray("networks");
		for(auto i = 0; i < networkCount; i++) {
			auto network = data.createNestedObject();
			Serial.println(WiFi.SSID(i));
			network["ssid"] = WiFi.SSID(i);
			network["rssi"] = WiFi.RSSI(i);
			network["enc"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
		}
		size_t len = measureMsgPack(doc);
		if(wifiList.reserve(len)) {
			serializeMsgPack(doc, (char *)wifiList.get(), len + 1);
			ws.binaryAll(&wifiList);
		}
	},
	WiFiEvent_t::SYSTEM_EVENT_SCAN_DONE);
	WiFi.onEvent(
	[](WiFiEvent_t event, WiFiEventInfo_t info) {
		Serial.println("WiFi connected");
		Serial.println("IP address: ");
		Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
	},
	WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
	WiFi.begin("Budapest", "2167529621");
	ws.onEvent(onWsEvent);
	server.addHandler(&ws);
	{
		auto handler = new ServeStatic("/_nuxt/", SPIFFS, "/www/_nuxt/");
		handler->setCacheControl("public, max-age=31536000");
		server.addHandler(handler);
	}
	{
		auto handler = new ServeStatic("/", SPIFFS, "/www/");
		handler->setDefaultFile("index.html");
		handler->setLastModified(lastModifiedTime.c_str());
		server.addHandler(handler);
	}
	server.onNotFound([](AsyncWebServerRequest *request) {
		auto response = request->beginResponse(SPIFFS, "/www/404.html");
		response->setCode(404);
		request->send(response);
	});
	server.begin();
	WiFi.scanNetworks(true);
	configTime(0, 0, "pool.ntp.org");
	ArduinoOTA
	.onStart([]() {
		String type;
		if(ArduinoOTA.getCommand() == U_FLASH)
			type = "sketch";
		else // U_SPIFFS
			type = "filesystem";

		// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
		Serial.println("Start updating " + type);
	})
	.onEnd([]() { Serial.println("\nEnd"); })
	.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	})
	.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if(error == OTA_AUTH_ERROR)
			Serial.println("Auth Failed");
		else if(error == OTA_BEGIN_ERROR)
			Serial.println("Begin Failed");
		else if(error == OTA_CONNECT_ERROR)
			Serial.println("Connect Failed");
		else if(error == OTA_RECEIVE_ERROR)
			Serial.println("Receive Failed");
		else if(error == OTA_END_ERROR)
			Serial.println("End Failed");
	});

	ArduinoOTA.begin();
}

int getSunrise(tm &timeinfo) {
	return sunTimes.sunrise(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, false);
}
int getSunset(tm &timeinfo) {
	return sunTimes.sunset(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, false);
}
time_t minutesPastMidnight(int minutes, tm timeinfo) {
	timeinfo.tm_sec = 0;
	timeinfo.tm_min = minutes;
	timeinfo.tm_hour = 0;
	return mktime(&timeinfo);
}
void loop() {
	ArduinoOTA.handle();
	random16_add_entropy(random(65535));

	EVERY_N_SECONDS(10) {
		Serial.println(ESP.getFreeHeap());
	}
	EVERY_N_SECONDS(5) {
		time_t now;
		tm timeinfo;
		static time_t sunrise = 0;
		static time_t sunset = 0;
		time(&now);
		gmtime_r(&now, &timeinfo);
		Serial.println(timeinfo.tm_year);
		if(timeinfo.tm_year > (2016 - 1900)) {
			if(sunrise > 0 && now >= sunrise) {
				sunrise = 0;
				if(followSun) {
					lightStat = LightStat::OFF;
					updateGlobalStats();
				}
			} else if(sunset > 0 && now >= sunset) {
				sunset = 0;
				if(followSun) {
					lightStat = LightStat::ON;
					updateGlobalStats();
				}
			}
			if(sunrise == 0) {
				auto time = minutesPastMidnight(getSunrise(timeinfo), timeinfo);
				if(time <= now) {
					auto tomorrow = timeinfo;
					tomorrow.tm_mday += 1;
					time = minutesPastMidnight(getSunrise(tomorrow), tomorrow);
				}
				sunrise = time;
			}
			if(sunset == 0) {
				auto time = minutesPastMidnight(getSunset(timeinfo), timeinfo);
				if(time <= now) {
					auto tomorrow = timeinfo;
					tomorrow.tm_mday += 1;
					time = minutesPastMidnight(getSunset(tomorrow), tomorrow);
				}
				sunset = time;
			}
			Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
			Serial.println(sunTimes.sunset(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, false));
			Serial.print("Sunset ");
			Serial.println(sunset);
			Serial.print("Sunrise ");
			Serial.println(sunrise);
		}
	}
	FastLED.setBrightness(brightness);
	if(lightStat != LightStat::OFF) {
		effectManager.run();
	} else {
		for(const auto strip : Configuration::strips) {
			fill_solid(strip.data, strip.len, CRGB::Black);
		}
	}
	FastLED.show();

	// insert a delay to keep the framerate modest
	FastLED.delay(1000 / FRAMES_PER_SECOND);
	ws.cleanupClients();
}
