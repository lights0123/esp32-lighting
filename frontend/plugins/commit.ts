if (console) {
	const hash = '<%= options.hash %>';
	const url = `https://github.com/lights0123/finals/commit/${hash}`;
	console.log(`%cGit commit ${hash.substring(0, 7)}`, 'color: black; background-image: linear-gradient(to right, #00FFA1, #00FFFF)');
	console.log(`%c${url}`, 'color: black; background-image: linear-gradient(to right, #D585FF, #00FFEE)');
}
