<!-- This Source Code Form is subject to the terms of the Mozilla Public
   - License, v. 2.0. If a copy of the MPL was not distributed with this
   - file, You can obtain one at http://mozilla.org/MPL/2.0/. -->

<template>
	<el-container class="container">
		<el-main class="main" v-if="config">
			<el-form label-width="120px">
				<el-form-item label="Strip">
					<el-select v-model="activeStrip" placeholder="Select">
						<el-option
							v-for="strip in strips"
							:key="strip"
							:label="strip"
							:value="strip" />
					</el-select>
				</el-form-item>
			</el-form>
			<el-row :gutter="24" v-if="activeStrip">
				<el-col :xs="24" :sm="24" :md="12" :lg="12" :xl="6" v-for="effect in $ws.effectConfig" :key="effect.name">
					<effect-config :enabled.sync="config[activeStrip][effect.name].enabled" :name="effect.name"
					               :config="config[activeStrip][effect.name]" :strip-name="activeStrip" />
				</el-col>
			</el-row>
		</el-main>
		<el-main class="main loading" v-else v-loading="true" />
	</el-container>
</template>

<script lang="ts">
import { Component, Vue, Watch } from 'vue-property-decorator';
import { Col, Container, Form, FormItem, Loading, Main, Option, Row, Select } from 'element-ui';
import EffectConfig from '~/components/EffectConfig.vue';
import { ConfigMessage } from '~/plugins/ws';
import copy from '~/components/copy';

Vue.use(Loading.directive);

@Component({
	components: {
		EffectConfig,
		[Container.name]: Container,
		[Main.name]: Main,
		[Select.name]: Select,
		[Option.name]: Option,
		[Row.name]: Row,
		[Col.name]: Col,
		[Form.name]: Form,
		[FormItem.name]: FormItem,
	},
	head() {
		return {
			title: 'Home',
		};
	},
})
export default class Effects extends Vue {
	activeStrip: string | null = null;
	config: ConfigMessage | null = null;

	get strips() {
		return this.config ? Object.keys(this.config).filter(val => val !== 'type') : [];
	}

	@Watch('$ws.config', { immediate: true, deep: true })
	@Watch('$ws.effectConfig', { immediate: true, deep: true })
	updateConfig() {
		if (!this.$ws.config || !this.$ws.effectConfig) return;
		const configEmpty = !this.config;
		this.config = copy(this.$ws.config);
		if (configEmpty) {
			this.activeStrip = Object.keys(this.config!)[0];
		}
		Object.keys(this.config!).filter(strip => strip !== 'type').forEach(stripName => {
			const strip = this.config![stripName];
			Object.keys(strip).forEach(effectName => {
				this.$set(strip[effectName], 'enabled', true);
			});
			this.$ws.effectConfig && this.$ws.effectConfig.forEach(config => {
				if (!(config.name in strip)) {
					this.$set(strip, config.name, { enabled: false });
					config.config.forEach(configItem => {
						if (configItem.type === 'color') {
							if (configItem.defaultR !== undefined) {
								this.$set(strip[config.name], configItem.title, {
									r: configItem.defaultR,
									g: configItem.defaultG,
									b: configItem.defaultB,
								});
							}
						} else if (configItem.type === 'select') {
							if (configItem.defaultValue !== undefined && configItem.options[configItem.defaultValue] !== undefined) {
								this.$set(strip[config.name], configItem.title, configItem.options[configItem.defaultValue]);
							}
						} else {
							this.$set(strip[config.name], configItem.title, configItem.defaultValue);
						}
					});
				}
			});
		});
	}

	get tabsOpen() {
		const arr: string[] = [];
		return arr;
	}

	set tabsOpen(tabs) {
	}
}
</script>

<style scoped lang="scss">
.container {
	height: 100%;
	flex-grow: 100;
}

.main {
	padding-right: 0;
}

.el-row {
	width: 100%;
}
</style>
