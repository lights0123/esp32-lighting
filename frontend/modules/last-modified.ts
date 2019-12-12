import { Module } from '@nuxt/types';

const modifiedModule: Module = function () {
	const date = new Date().toUTCString();

	this.options.build && this.options.build.plugins && this.options.build.plugins.push({
		apply(compiler) {
			compiler.plugin('emit', (compilation, cb) => {
				compilation.assets['last-modified.txt'] = { source: () => date, size: () => date.length };

				cb();
			});
		},
	});
};

export default modifiedModule;
