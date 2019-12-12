<!-- This Source Code Form is subject to the terms of the Mozilla Public
   - License, v. 2.0. If a copy of the MPL was not distributed with this
   - file, You can obtain one at http://mozilla.org/MPL/2.0/. -->

<template>
	<el-card class="box-card" shadow="never">
		<div slot="header" class="clearfix">
			<span>{{name}}</span>
			<el-switch style="float: right; padding: 3px 0"
			           :value="config.enabled" @input="configUpdated($event)"
			           :disabled="!(config.enabled || isConfigValid)">
			</el-switch>
		</div>
		<el-form :model="config" status-icon :rules="rules" label-width="120px" ref="form">
			<el-form-item v-for="item in configSettings" :key="item.title" :label="item.title"
			              :class="{ color: item.type === 'color', boolean: item.type === 'boolean', required: item.required }"
			              :prop="item.title">
				<el-input v-if="item.type === 'string'"
				          type="text"
				          v-model="config[item.title]"
				          :minlength="item.minLength"
				          :maxlength="item.maxLength"
				          show-word-limit
				/>
				<number-input v-else-if="item.type === 'number'" v-model="config[item.title]" :max="item.max"
				              :min="item.min"
				              :step-by="item.stepBy"
				              :required="item.required" />
				<el-color-picker v-else-if="item.type==='color'" :value="formatColor(config[item.title])"
				                 color-format="hex"
				                 :predefine="[`rgb(${item.defaultR},${item.defaultG},${item.defaultB})`]"
				                 @input="setColor(item.title, $event)" />
				<el-select v-else-if="item.type==='select'" v-model="config[item.title]" placeholder="Select"
				           :clearable="!item.required">
					<el-option
						v-for="strip in item.options"
						:key="strip"
						:label="strip"
						:value="strip" />
				</el-select>
				<el-switch v-else-if="item.type==='boolean'"
				           v-model="config[item.title]" />
				<el-input v-if="item.type === 'json'"
				          type="textarea"
				          :rows="4"
				          placeholder="Please input"
				          v-model="config[item.title]"
				/>
				<pre class="description">{{item.description}}</pre>
			</el-form-item>
			<el-form-item>
				<el-button type="primary" v-if="configChanged" :disabled="!isConfigValid"
				           @click="configUpdated(true)">Update {{ config.enabled ? '' : 'and Enable' }}
				</el-button>
			</el-form-item>
		</el-form>
	</el-card>
</template>

<script lang="ts">
import { Component, Prop, Ref, Vue, Watch } from 'vue-property-decorator';
import { Button, Card, ColorPicker, Form, FormItem, Input, Option, Select, Switch } from 'element-ui';
import NumberInput from '~/components/NumberInput.vue';
import { ColorValue } from '~/plugins/ws';

import copy from '~/components/copy';

@Component({
	components: {
		NumberInput,
		[Form.name]: Form,
		[FormItem.name]: FormItem,
		[Input.name]: Input,
		[Button.name]: Button,
		[Switch.name]: Switch,
		[ColorPicker.name]: ColorPicker,
		[Select.name]: Select,
		[Option.name]: Option,
		[Card.name]: Card,
	},
})
export default class EffectConfig extends Vue {
	@Prop({ type: String, required: true }) readonly name!: string;
	@Prop({ type: String, required: true }) readonly stripName!: string;
	@Prop({ type: Object, required: true }) config!: { [configName: string]: string | number | boolean | ColorValue };
	@Ref() readonly form!: Form;

	get rules() {
		const rules = {};
		if (this.$ws.effectConfig) {
			this.configSettings.forEach(config => {
				rules[config.title] = [{
					validator: (rule, value, callback) => {
						if (config.type === 'string') {
							const length = new Blob([value]).size;
							if (config.required && !value) callback(new Error('Required'));
							else if (length > 0 && length < config.minLength) callback(new Error(`Input more than ${config.minLength} characters`));
							else if (length > config.maxLength) callback(new Error(`Input no more than ${config.maxLength} characters`));
							else callback();
						} else if (config.type === 'number') {
							if (Number.isFinite(value)) {
								if (value < config.min) callback(new Error(`Input must be more than ${config.min}`));
								else if (value > config.max) callback(new Error(`Input must be no more than ${config.max}`));
								else if (Math.abs((value - config.min) % config.stepBy) > Number.EPSILON) callback(new Error(`Input must be a multiple of ${config.stepBy}`));
								else callback();
							} else if (config.required) callback(new Error('Required'));
							else callback();
						} else if (config.type === 'color') {
							if (config.required && !value) callback(new Error('Required'));
							else callback();
						} else if (config.type === 'select') {
							if (!value) {
								if (config.required) callback(new Error('Required'));
								else callback();
							} else if (!config.options.includes(value)) callback(new Error('Select an option'));
							else callback();
						} else if (config.type === 'json') {
							if (!value) {
								if (config.required) callback(new Error('Required'));
								else callback();
							} else {
								try {
									JSON.parse(value);
									callback();
								} catch (e) {
									callback(new Error('Input valid JSON'));
								}
							}
						} else callback();
					},
					trigger: 'change',
				}];
			});
		}
		return rules;
	}

	formatColor(color?: ColorValue) {
		if (color) {
			return `rgb(${color.r},${color.g},${color.b})`;
		} else {
			return null;
		}
	}

	setColor(name: string, color: string) {
		if (color) {
			this.$set(this.config, name, {
				r: parseInt(color.substring(1, 3), 16),
				g: parseInt(color.substring(3, 5), 16),
				b: parseInt(color.substring(5, 7), 16),
			});
		} else {
			if (this.$ws.effectConfig) {
				const configSetting = this.configSettings.find(({ title }) => title === name);
				if (!configSetting || configSetting.type !== 'color') return;
				if (configSetting.required) {
					this.$set(this.config, name, {
						r: configSetting.defaultR,
						g: configSetting.defaultG,
						b: configSetting.defaultB,
					});
				} else {
					this.$delete(this.config, name);
				}
			}
		}
	}

	get configSettings() {
		if (!this.$ws.effectConfig) return [];
		const settings = this.$ws.effectConfig.find(({ name }) => name === this.name);
		return settings ? settings.config : [];
	}

	get formattedConfig(): { [configName: string]: string | number | boolean | ColorValue } | null {
		const newConfig = {};
		Object.keys(this.config).forEach(configName => {
			if (configName === 'enabled') return;
			const config = this.config[configName];
			const type = this.configSettings.find(({ title }) => title === configName)!.type;
			if (type === 'color') {
				newConfig[configName] = copy(config);
			} else if (type === 'json') {
				try {
					if (config) newConfig[configName] = JSON.stringify(JSON.parse(config as string));
				} catch (e) {
				}
			} else {
				newConfig[configName] = config;
			}
		});
		return newConfig;
	}

	get configChanged() {
		if (!this.formattedConfig || !this.$ws.config || !this.$ws.config![this.stripName][this.name]) return true;
		return !Object.keys(this.formattedConfig).every(configName => {
			const currentConfig = this.formattedConfig![configName];
			const configSettings = this.configSettings.find(({ title }) => title === configName)!;
			const config = this.$ws.config![this.stripName][this.name][configName] as string | number | boolean | ColorValue | undefined;
			if (configSettings.type === 'string' || configSettings.type === 'json' || configSettings.type === 'select') {
				if (!!currentConfig === !!config) return true;
				return currentConfig === config;
			} else if (configSettings.type === 'number') {
				if (!Number.isFinite(currentConfig as number) && !Number.isFinite(config as number)) return true;
				return Math.abs(currentConfig as number - (config as number)) <= Number.EPSILON;
			} else if (configSettings.type === 'color') {
				if (config && 'r' in (config as ColorValue) && 'r' in (currentConfig as ColorValue)) {
					return (config as ColorValue).r === (currentConfig as ColorValue).r && (config as ColorValue).g === (currentConfig as ColorValue).g && (config as ColorValue).b === (currentConfig as ColorValue).b;
				} else return false;
			} else return config === currentConfig;
		});
	}

	isConfigValid = true;

	@Watch('config', { immediate: true, deep: true })
	async updateConfigValid() {
		console.log(this.form);
		if (!this.form) return;
		try {
			await this.form.validate();
			this.isConfigValid = true;
		} catch (e) {
			this.isConfigValid = false;
			console.log(e);
		}
	}

	configUpdated(enabled: boolean) {
		console.log(enabled);
		if (enabled) {
			this.$ws.send({
				type: 'updateEffect',
				strip: this.stripName,
				effect: this.name,
				config: this.config,
			});
		} else {
			this.$ws.send({
				type: 'removeEffect',
				strip: this.stripName,
				effect: this.name,
			});
		}
		this.config.enabled = enabled;
	}

	mounted() {
		this.updateConfigValid();
	}
}
</script>

<style scoped lang="scss">
.el-input {
	width: auto;

	/deep/ input {
		padding-right: 2em;
	}
}

.description {
	display: block;
	margin-top: 0.5em;
	line-height: normal;
	font-size: 12px;
	font-family: 'Open Sans', sans-serif;
}

.color .description {
	margin-top: -8px;
}

.boolean .description {
	margin-top: -2px;
}

.el-textarea /deep/ textarea {
	font-family: monospace;
}

.clearfix:before,
.clearfix:after {
	display: table;
	content: "";
}

.clearfix:after {
	clear: both
}

.el-form-item {
	margin-bottom: 10px;

	/deep/ .el-form-item__error {
		position: relative;
		top: -1em;
	}
}

.required /deep/ .el-form-item__label::before {
	content: '*';
	color: #F56C6C;
	margin-right: 4px;
}
</style>
