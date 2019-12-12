<!-- This Source Code Form is subject to the terms of the Mozilla Public
   - License, v. 2.0. If a copy of the MPL was not distributed with this
   - file, You can obtain one at http://mozilla.org/MPL/2.0/. -->

<template>
	<section class="container">
		<el-container class="container">
			<el-main class="main">
				<h1>File not found</h1>
			</el-main>
		</el-container>
	</section>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Container, Main } from 'element-ui';

@Component({
	components: {
		[Container.name]: Container,
		[Main.name]: Main,
	},
	head() {
		return {
			title: '404',
		};
	},
})
export default class Planner extends Vue {
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
</style>
