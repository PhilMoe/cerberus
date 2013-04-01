
function createArrayBuffer( byteLength ){
	return new ArrayBuffer( byteLength );
}

function createDataView( buffer,byteOffset,byteLength ){
	return new DataView( buffer,byteOffset,byteLength );
}

function createInt8Array( buffer,byteOffset,length ){
	return new Int8Array( buffer,byteOffset,length );
}

function createInt16Array( buffer,byteOffset,length ){
	return new Int16Array( buffer,byteOffset,length );
}

function createInt32Array( buffer,byteOffset,length ){
	return new Int32Array( buffer,byteOffset,length );
}

function createFloat32Array( buffer,byteOffset,length ){
	return new Float32Array( buffer,byteOffset,length );
}

