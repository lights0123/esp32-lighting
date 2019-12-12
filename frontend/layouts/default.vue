<!-- This Source Code Form is subject to the terms of the Mozilla Public
   - License, v. 2.0. If a copy of the MPL was not distributed with this
   - file, You can obtain one at http://mozilla.org/MPL/2.0/. -->

<template>
	<div class="outer-container">
		<el-menu :default-active="active" mode="horizontal">
			<el-menu-item v-for="(title, path) in routes" :key="path" :index="path" class="link">
				<nuxt-link :to="path">{{ title }}</nuxt-link>
			</el-menu-item>
			<el-menu-item class="connection-status">
				<div class="wrapper" v-if="$ws.connected">
					<i class="el-icon-check icon"></i>
					Connected
				</div>
				<div class="wrapper" v-else>
					<i class="el-icon-warning-outline icon"></i>
					Disconnected
				</div>
			</el-menu-item>
		</el-menu>
		<nuxt />
	</div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import { Menu, MenuItem } from 'element-ui';

@Component({
	components: {
		[Menu.name]: Menu,
		[MenuItem.name]: MenuItem,
	},
})
export default class Default extends Vue {
	routes = { '/': 'Home','/effects': 'Effects', '/settings': 'Settings' };

	get active() {
		return this.$route.path;
	}
}
</script>

<style scoped lang="scss">
.link {
	padding: 0;
	border-bottom: 0;
	a {
		display: block;
		height: 100%;
		padding-left: 20px;
		padding-right: 20px;
		text-decoration: none;
	}
}

.outer-container {
	min-height: 100vh;
	display: flex;
	flex-direction: column;
}

.connection-status {
	.icon {
		&.el-icon-check {
			color: #0bbd87;
		}
		&.el-icon-warning-outline {
			color: #bc0b0b;
		}
		align-self: center;
		margin-right: 0.2em;
	}

	.wrapper {
		display: flex;
	}

	pointer-events: none;
	float: right;
}
</style>
