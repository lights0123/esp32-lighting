<!-- This Source Code Form is subject to the terms of the Mozilla Public
   - License, v. 2.0. If a copy of the MPL was not distributed with this
   - file, You can obtain one at http://mozilla.org/MPL/2.0/. -->

<template>
	<el-container class="container">
		<el-main class="main" v-if="$ws.config">
			<label class="label">Power</label>
			<el-switch
				:value="on"
				@input="setOnOff"
				active-text="On"
				inactive-text="Off">
			</el-switch>
			<br>
			<br>
			<label class="label">Brightness</label>
			<el-row :gutter="20">
				<el-col :xs="24" :sm="6" :md="4" :lg="4" :xl="2">
					<number-input :value="brightness" @input="setBrightness" :min="0" :max="255" required />
				</el-col>
				<el-col :xs="24" :sm="18" :md="8" :lg="8" :xl="4">
					<el-slider :value="brightness" @input="setBrightness" :max="255" :show-tooltip="false" />
				</el-col>
			</el-row>
			<br>
			<el-row :gutter="24">
				<el-col :xs="24" :sm="24" :md="12" :lg="12" :xl="6" v-for="(effects, strip) in config" :key="strip">
					<strip-config :strip="strip" />
				</el-col>
			</el-row>
		</el-main>
		<el-main class="main loading" v-else v-loading="true" />
	</el-container>
</template>

<script lang="ts">
import { Component, Vue, Watch } from 'vue-property-decorator';
import { Col, Container, Loading, Main, Row, Slider, Switch } from 'element-ui';
import StripConfig from '~/components/StripConfig.vue';
import NumberInput from '~/components/NumberInput.vue';
import { GlobalStatsMessage } from '~/plugins/ws';

Vue.use(Loading.directive);
@Component({
	components: {
		NumberInput,
		StripConfig,
		[Container.name]: Container,
		[Main.name]: Main,
		[Row.name]: Row,
		[Col.name]: Col,
		[Slider.name]: Slider,
		[Switch.name]: Switch,
	},
	head() {
		return {
			title: 'Home',
		};
	},
})
export default class Index extends Vue {
	brightness = 0;
	on = true;

	get config() {
		const config = {};
		this.$ws.config && Object.keys(this.$ws.config).forEach(stripName => {
			if (stripName !== 'type') config[stripName] = this.$ws.config![stripName];
		});
		return config;
	}

	@Watch('$ws.globalConfig', { deep: true, immediate: true })
	globalConfigUpdated(config?: GlobalStatsMessage) {
		if (config) {
			this.on = config.on;
			this.brightness = config.brightness;
		}
	}

	setBrightness(brightness: number) {
		this.$ws.send({ type: 'updateGlobal', brightness });
	}

	setOnOff(state: boolean) {
		this.$ws.send({ type: 'updateGlobal', on: state });
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
	padding-top: 10px;
}

.el-row {
	width: 100%;
}

.el-input-number {
	width: 100%;
}

.label {
	font-size: 14px;
	color: #606266;
	display: block;
}
</style>
