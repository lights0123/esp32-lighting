<!-- This Source Code Form is subject to the terms of the Mozilla Public
   - License, v. 2.0. If a copy of the MPL was not distributed with this
   - file, You can obtain one at http://mozilla.org/MPL/2.0/. -->

<template>
	<div
		@dragstart.prevent
		class="el-input-number">
		<span
			class="el-input-number__decrease"
			:class="{'is-disabled': shownValue - stepBy < min}"
			role="button"
			v-repeat-click="minus"
			@keydown.enter="minus">
      <i class="el-icon-minus" />
    </span>
		<span
			class="el-input-number__increase"
			:class="{'is-disabled': shownValue + stepBy > max}"
			role="button"
			v-repeat-click="plus"
			@keydown.enter="plus">
      <i class="el-icon-plus" />
    </span>
		<input class="el-input__inner"
		       v-model.lazy="shownValue"
		       type="number"
		       :min="min"
		       :max="max"
		       :step="stepBy.toFixed(3)"
		>
	</div>
</template>

<script lang="ts">
import { Component, Emit, Prop, Vue, Watch } from 'vue-property-decorator';
import 'element-ui/lib/theme-chalk/input-number.css';
import RepeatClick from 'element-ui/lib/directives/repeat-click';

@Component({
	directives: {
		RepeatClick,
	}
})
export default class NumberInput extends Vue {
	@Prop({ type: Number, default: 1 }) public min!: number;
	@Prop({ type: Number, default: 100 }) public max!: number;
	@Prop({ type: Number, default: 1 }) public stepBy!: number;
	@Prop({ type: Boolean, default: false }) public required!: boolean;
	@Prop(Number) public value!: number;

	public shownValue = 1;

	@Emit()
	public input(value: number) { }

	@Watch('shownValue')
	public onShownValue(rawValue: string | number, oldValue: number) {
		if (rawValue === '' || Number.isNaN(rawValue as number)) {
			if (this.required) {
				this.shownValue = oldValue;
			} else {
				this.input(NaN);
				this.shownValue = NaN;
			}
		} else {
			let value = Number(rawValue);
			value = Math.round((value - this.min) / this.stepBy) * this.stepBy + this.min;
			value = Math.min(value, this.max);
			value = Math.max(value, this.min);
			value = +value.toFixed(3);
			this.input(value);
			this.shownValue = value;
		}
	}

	@Watch('value', { immediate: true })
	public onValue(value: number) {
		this.shownValue = value;
	}

	public minus() {
		if (Number.isNaN(this.shownValue)) {
			this.shownValue = this.min;
		} else this.shownValue -= this.stepBy;
	}

	public plus() {
		if (Number.isNaN(this.shownValue)) {
			this.shownValue = this.min;
		}
		this.shownValue += this.stepBy;
	}
}
</script>

<style lang="scss" scoped>
.el-input__inner {
	-moz-appearance: textfield !important;

	&[type=number]::-webkit-inner-spin-button,
	&[type=number]::-webkit-outer-spin-button {
		-webkit-appearance: none !important;
		margin: 0 !important;
	}
}
</style>
