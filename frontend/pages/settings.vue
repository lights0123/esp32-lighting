<!-- This Source Code Form is subject to the terms of the Mozilla Public
   - License, v. 2.0. If a copy of the MPL was not distributed with this
   - file, You can obtain one at http://mozilla.org/MPL/2.0/. -->

<template>
	<el-container class="container">
		<el-main class="main">
			<div>
				<el-collapse v-model="tabsOpen">
					<network-config name="station" title="Connect to a Network" :value.sync="form.station"
					                :scan="true" :verify-pass="false" />
					<network-config name="ap" title="Create a Network" :value.sync="form.ap" />
				</el-collapse>
				<el-form :model="form" status-icon :rules="rules" ref="ruleForm" label-width="120px"
				         class="demo-ruleForm">
					<el-form-item>
						<el-button type="primary" @click="submitForm('ruleForm')">Submit</el-button>
						<el-button @click="form('ruleForm')">Reset</el-button>
					</el-form-item>
				</el-form>
			</div>
		</el-main>
	</el-container>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Button, Collapse, Container, Form, FormItem, Input, Main } from 'element-ui';
import NetworkConfig from '~/components/NetworkConfig.vue';

@Component({
	components: {
		[Container.name]: Container,
		[Main.name]: Main,
		[Form.name]: Form,
		[FormItem.name]: FormItem,
		[Input.name]: Input,
		[Button.name]: Button,
		[Collapse.name]: Collapse,
		NetworkConfig,
	},
	head() {
		return {
			title: 'Home',
		};
	},
})
export default class Planner extends Vue {
	activeTabs = {
		station: true,
		ap: true,
	};
	form = {
		station: {
			enabled: true,
			ssid: '',
			pass: '',
		},
		ap: {
			enabled: true,
			ssid: '',
			pass: '',
		},
	};
	rules = {
		'ap.pass': [
			{
				validator: (rule, value, callback) => {
					const length = new Blob([value]).size;
					console.log(value, length);
					if (length === 0) callback();
					else if (length < 8) callback(new Error('Specify at least 8 characters'));
					else if (length > 63) callback(new Error('Specify no more than 63 characters'));
					else callback();
				},
				trigger: 'change',
			},
		],
	};

	get tabsOpen() {
		const arr: string[] = [];
		if (this.form.station.enabled) arr.push('station');
		if (this.form.ap.enabled) arr.push('ap');
		return arr;
	}

	set tabsOpen(tabs) {
		this.form.station.enabled = tabs.includes('station');
		this.form.ap.enabled = tabs.includes('ap');
	}

	submitForm() {}
}
</script>

<style scoped lang="scss">
.container {
	height: 100%;
	/*flex-grow: 100;*/
}
</style>
