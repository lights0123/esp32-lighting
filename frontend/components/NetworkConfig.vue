<!-- This Source Code Form is subject to the terms of the Mozilla Public
   - License, v. 2.0. If a copy of the MPL was not distributed with this
   - file, You can obtain one at http://mozilla.org/MPL/2.0/. -->

<template>
	<el-collapse-item :name="name">
		<template slot="title">
			<el-switch
				:value="config.enabled"
				:active-text="title">
			</el-switch>
		</template>
		<el-form :model="config" status-icon :rules="rules" label-width="120px">
			<el-form-item label="Name" prop="ssid">
				<el-autocomplete
					v-if="scan"
					popper-class="my-autocomplete"
					v-model="config.ssid"
					:fetch-suggestions="querySearch"
					placeholder="Network Name">
					<template slot-scope="{ item }">
						<div class="autocomplete">
							<span class="value">{{ item.value }}</span>
							<i class="icon" :class="item.enc ? 'el-icon-lock' : 'el-icon-unlock'"></i>
						</div>
					</template>
				</el-autocomplete>
				<el-input v-else v-model="config.ssid" autocomplete="off" placeholder="Network Name" />
			</el-form-item>
			<el-form-item label="Password" :prop="verifyPass ? 'pass' : undefined">
				<el-input type="password" v-model="config.pass" autocomplete="off" placeholder="Password"></el-input>
			</el-form-item>
		</el-form>
	</el-collapse-item>
</template>

<script lang="ts">
import { Component, Prop, PropSync, Vue } from 'vue-property-decorator';
import { Autocomplete, Button, CollapseItem, Form, FormItem, Input, Switch } from 'element-ui';
import { Network } from '~/plugins/ws';

interface Config {
	enabled: boolean,
	ssid: string,
	pass: string,
}

@Component({
	components: {
		[Form.name]: Form,
		[FormItem.name]: FormItem,
		[Input.name]: Input,
		[Button.name]: Button,
		[CollapseItem.name]: CollapseItem,
		[Switch.name]: Switch,
		[Autocomplete.name]: Autocomplete,
	},
})
export default class Planner extends Vue {
	@PropSync('value', { type: Object, required: true }) config!: Config;
	@Prop({ type: String, required: true }) readonly title!: string;
	@Prop({ type: String, required: true }) readonly name!: string;
	@Prop({ type: Boolean, default: false }) readonly scan!: boolean;
	@Prop({ type: Boolean, default: true }) readonly verifyPass!: boolean;

	rules = {
		'pass': [
			{
				validator: (rule, value, callback) => {
					const length = new Blob([value]).size;
					if (length === 0) callback();
					else if (length < 8) callback(new Error('Specify at least 8 characters'));
					else if (length > 63) callback(new Error('Specify no more than 63 characters'));
					else callback();
				},
				trigger: 'change',
			},
		],
		'ssid': [
			{
				validator: (rule, value, callback) => {
					const length = new Blob([value]).size;
					if (length < 1) callback(new Error('Specify a name'));
					else if (length > 32) callback(new Error('Specify no more than 32 characters'));
					else callback();
				},
				trigger: 'change',
			},
		],
	};

	querySearch(queryString, cb) {
		const networks = [] as Network[];
		this.$ws.networks
			.filter(network => network.ssid && network.rssi)
			.sort((a, b) => b.rssi - a.rssi)
			.forEach(network => {
				if (!networks.some(other => other.ssid === network.ssid && other.enc === network.enc)) {
					networks.push(network);
				}
			});
		const results = networks.filter(network => network.ssid.toUpperCase().includes(queryString.toUpperCase()));
		cb(results.sort((a, b) => {
			const nameA = a.ssid.toUpperCase();
			const nameB = b.ssid.toUpperCase();
			if (nameA < nameB) return -1;
			if (nameA > nameB) return 1;
			return 0;
		}).map(({ ssid, enc, rssi }) => ({
			value: ssid,
			rssi,
			enc,
		})));
	}
}
</script>

<style scoped lang="scss">
.el-input, .el-autocomplete /deep/ .el-input {
	width: auto;

	/deep/ input {
		padding-right: 2em;
	}
}

.autocomplete {
	display: flex;
	flex-direction: row;
}

.icon {
	align-self: center;
}

.value {
	flex-grow: 1;
}
</style>
