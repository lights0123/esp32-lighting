import webpack from 'webpack';
import { Configuration } from '@nuxt/types';
import * as pkg from './package.json';

const config: Configuration = {

	/*
  ** Headers of the page
  */
	head: {
		titleTemplate: '%s | ESP32 Lighting',
		meta: [
			{ charset: 'utf-8' },
			{ name: 'viewport', content: 'width=device-width, initial-scale=1' },
			{ hid: 'description', name: 'description', content: pkg.description },
		],
		link: [
			{ rel: 'icon', type: 'image/x-icon', href: '/favicon.ico' },
		],
	},

	/*
  ** Customize the progress-bar color
  */
	loading: { color: '#37cbfc' },

	/*
  ** Global CSS
  */
	css: [
		'@/assets/main.css',
	],

	/*
  ** Plugins to load before mounting the App
  */
	plugins: ['~/plugins/ws',],

	/*
  ** Nuxt.js modules
  */
	modules: [
		'modules/commit',
	],

	/*
  ** Build configuration
  */
	build: {
		extend(config, { isClient }) {
			// Extend only webpack config for client-bundle
			if (isClient && process.env.NODE_ENV !== 'production') {
				config.devtool = '#source-map';
			}
		},
		transpile: [/^vue-awesome/],
		babel: {
			plugins: ['@babel/plugin-syntax-dynamic-import',
				[
					'component',
					{
						'libraryName': 'element-ui',
						'styleLibraryName': 'theme-chalk',
					},
				],
			],
		},
		plugins: [
			new webpack.NormalModuleReplacementPlugin(/element-ui[\/\\]lib[\/\\]locale[\/\\]lang[\/\\]zh-CN/, 'element-ui/lib/locale/lang/en'),
		],
		filenames: {
			app: ({ isDev }) => isDev ? '[name].js' : '[chunkhash:14].js',
			chunk: ({ isDev }) => isDev ? '[name].js' : '[chunkhash:14].js',
			css: ({ isDev }) => isDev ? '[name].css' : '[contenthash:13].css',
			img: ({ isDev }) => isDev ? '[path][name].[ext]' : '[hash:7].[ext]',
			font: ({ isDev }) => isDev ? '[path][name].[ext]' : '[hash:7].[ext]',
			video: ({ isDev }) => isDev ? '[path][name].[ext]' : '[hash:7].[ext]',
		},
		extractCSS: true,
	},

	generate: {
		dir: '../data/www/',
		fallback: true,
	},
	buildModules: ['@nuxt/typescript-build', 'modules/last-modified', 'nuxt-compress'],
	'nuxt-compress': {
		gzip: {
			cache: true,
		},
		brotli: {
			threshold: Infinity,
		},
	},
};
export default config;
