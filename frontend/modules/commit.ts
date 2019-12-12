import { resolve } from 'path';
import fs from 'mz/fs';
const { readFile } = fs;

async function getHeadCommit() {
	const rev = (await readFile(resolve('.git/HEAD'))).toString();
	if (rev.includes(':')) {
		return rev.trim();
	}
	return (await readFile(resolve(`.git/${rev.substring(5).trim()}`))).toString().trim();
}

export default async function commit(moduleOptions) {
	const hash = await getHeadCommit();
	this.addPlugin({
		src: resolve(__dirname, '../plugins/commit.ts'),
		options: {
			hash,
		},
	});
}
