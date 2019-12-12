<template>
	<el-card class="box-card" shadow="never">
		<div slot="header" class="clearfix">
			<span>{{strip}}</span>
		</div>
		<el-row :gutter="20">
			<el-col :xs="24" :sm="18" :md="16" :lg="18" :xl="15">
				<el-select v-model="value" clearable placeholder="Effect">
					<el-option
						v-for="(config, effect) in $ws.config[strip]"
						:key="effect"
						:label="effect"
						:value="effect">
					</el-option>
				</el-select>
			</el-col>
			<el-col :xs="24" :sm="6" :md="8" :lg="6" :xl="9">
				<el-button-group>
					<el-button icon="el-icon-arrow-left" />
					<el-button icon="el-icon-arrow-right" />
					<el-button icon="el-icon-close" />
				</el-button-group>
			</el-col>
			<el-col :xs="24" :sm="18" :md="16" :lg="18" :xl="15">
				<label class="el-form-item__label">Autoplay</label>
			</el-col>
			<el-col :xs="24" :sm="6" :md="8" :lg="6" :xl="9" class="switch">
				<el-switch
					v-model="on"
					:active-text="on ? 'On' : 'Off'">
				</el-switch>
			</el-col>
			<el-col :xs="24" :sm="18" :md="16" :lg="18" :xl="15">
				<label class="el-form-item__label">Autoplay Duration (sec.)</label>
			</el-col>
			<el-col :xs="24" :sm="6" :md="8" :lg="6" :xl="9">
				<number-input :step-by="0.1" />
			</el-col>
		</el-row>
	</el-card>
</template>

<script lang="ts">
import { Component, Prop, Vue } from 'vue-property-decorator';
import { Button, ButtonGroup, Card, Col, Form, FormItem, Option, Row, Select, Switch } from 'element-ui';
import NumberInput from '~/components/NumberInput.vue';

@Component({
	components: {
		NumberInput,
		[Card.name]: Card,
		[Select.name]: Select,
		[Option.name]: Option,
		[Button.name]: Button,
		[ButtonGroup.name]: ButtonGroup,
		[Row.name]: Row,
		[Col.name]: Col,
		[Form.name]: Form,
		[FormItem.name]: FormItem,
		[Switch.name]: Switch,
	},
})
export default class StripConfig extends Vue {
	@Prop({ type: String, required: true }) readonly strip!: string;
	value = '';
	on = true;
}
</script>
<style scoped lang="scss">
.el-button {
	padding: 12px 0;
	width: 100%;
}

.el-button-group {
	display: flex;
	height: 40px;
}

.el-select {
	width: 100%;
}

.el-button-group {
	width: 100%;
}

.el-col {
	.el-input-number {
		width: 100%;
	}

	.el-form-item__label {
		width: 100%;
		padding: 0;
	}

	.el-switch:nth-child(1) {
		height: 100%;
	}

	&.switch {
		height: 40px;
	}
}


</style>
