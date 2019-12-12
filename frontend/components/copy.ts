export default function copy(aObject) {
	if (!aObject) {
		return aObject;
	}
	let bObject = Array.isArray(aObject) ? [] : {};
	Object.keys(aObject).forEach(k => {
		const v = aObject[k];
		bObject[k] = (typeof v === 'object') ? copy(v) : v;
	});

	return bObject;
}
