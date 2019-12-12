import { Plugin } from '@nuxt/types';
import Vue from 'vue';
import { decode, encode } from '@msgpack/msgpack';

export interface Network {
	ssid: string,
	rssi: number,
	enc: boolean,
}

interface ScanMessage {
	type: 'scan',
	networks: Network[],
}

interface ConfigSetting {
	title: string,
	description: string,
}

interface StringConfig extends ConfigSetting {
	type: 'string',
	minLength: number,
	maxLength: number,
	defaultValue: string,
	required: boolean,
}

interface NumberConfig extends ConfigSetting {
	type: 'number',
	min: number,
	max: number,
	stepBy: number,
	defaultValue: string,
	required: boolean,
}

interface ColorConfig extends ConfigSetting {
	type: 'color',
	defaultR?: number,
	defaultG?: number,
	defaultB?: number,
	required: boolean,
}

interface SelectConfig extends ConfigSetting {
	type: 'select',
	options: string[],
	defaultValue: number,
	required: boolean,
}

interface BooleanConfig extends ConfigSetting {
	type: 'boolean',
	defaultValue: boolean,
}

interface JsonConfig extends ConfigSetting {
	type: 'json',
	defaultValue: string,
	required: boolean,
}

interface EffectConfig {
	name: string,
	config: (StringConfig | NumberConfig | ColorConfig | SelectConfig | BooleanConfig | JsonConfig)[],
}

interface EffectConfigMessage {
	type: 'effectConfig',
	effects: EffectConfig[],
}

export interface ColorValue {
	r: number,
	g: number,
	b: number,
}

export type ConfigMessage = {
	[stripName: string]: {
		[effectName: string]: {
			[configName: string]: string | number | boolean | ColorValue,
		}
	}
} & { type: 'config' }

export interface GlobalStatsMessage {
	type: 'globalStats',
	brightness: number,
	on: boolean,
	followSun: boolean,
}

type Message = ScanMessage | EffectConfigMessage | ConfigMessage | GlobalStatsMessage;
export namespace Outgoing {
	export interface UpdateEffectMessage {
		type: 'updateEffect',
		strip: string,
		effect: string,
		config: {
			[configName: string]: string | number | boolean | ColorValue,
		}
	}

	export interface RemoveEffectMessage {
		type: 'removeEffect',
		strip: string,
		effect: string,
	}

	export interface GlobalStatsMessage {
		type: 'updateGlobal',
		brightness?: number,
		on?: boolean,
		followSun?: boolean,
	}

	export type Message = UpdateEffectMessage | RemoveEffectMessage | GlobalStatsMessage;
}

interface Ws {
	connected: boolean,
	networks: Network[],
	effectConfig: EffectConfig[] | null,
	config: ConfigMessage | null,
	globalConfig: GlobalStatsMessage | null,
	send: (obj: Outgoing.Message) => boolean,
}

declare module 'vue/types/vue' {
	// 3. Declare augmentation for Vue
	interface Vue {
		$ws: Ws
	}
}

const myPlugin: Plugin = (context, inject) => {
	let ws: WebSocket | null = null;
	const state = Vue.observable({
		connected: false,
		networks: [],
		effectConfig: null,
		config: null,
		globalConfig: null,
		send(obj: Outgoing.Message) {
			if (!ws) return false;
			try {
				ws.send(encode(obj));
				return true;
			} catch (e) {
				return false;
			}
		},
	} as Ws);

	function connect() {
		ws = new WebSocket('ws://10.0.1.224/ws');
		ws.onopen = function () {
			// subscribe to some channels
			state.connected = true;
			ws && ws.send(JSON.stringify({
				//.... some message the I must send when I connect ....
			}));
		};

		ws.onmessage = function (e) {
			console.log('Message:', e.data);
			if (e.data instanceof Blob) {
				const fileReader = new FileReader();
				fileReader.onload = (event) => {
					// @ts-ignore
					const message = decode(event.target.result) as Message;
					console.log('Message:', message);
					if (message.type === 'scan') {
						state.networks = message.networks;
					} else if (message.type === 'effectConfig') {
						message.effects.forEach(effect => {
							effect.config.forEach(config => {
								if (config.type === 'number') {
									config.min = +config.min.toFixed(3);
									config.max = +config.max.toFixed(3);
									config.stepBy = +config.stepBy.toFixed(3);
								}
							});
						});
						state.effectConfig = message.effects;
					} else if (message.type === 'config') {
						state.config = message;
					} else if (message.type === 'globalStats') {
						state.globalConfig = message;
					}
				};
				fileReader.readAsArrayBuffer(e.data);
			}
		};

		ws.onclose = function (e) {
			state.connected = false;
			console.log('Socket is closed. Reconnect will be attempted in 1 second.', e.reason);
			setTimeout(function () {
				connect();
			}, 1000);
		};

		ws.onerror = function (err) {
			// @ts-ignore
			console.error('Socket encountered error: ', err.message, 'Closing socket');
			ws && ws.close();
		};
	}

	if (process.client) connect();

	inject('ws', state);
};

export default myPlugin;
