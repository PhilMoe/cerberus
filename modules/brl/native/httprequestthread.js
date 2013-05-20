
function BBHttpRequestThread(){
	BBThread.call(this);

	this.response = {
		text: '',
		status: -1,
		length: 0
	}
}

BBHttpRequestThread.prototype=extend_class( BBThread );

BBHttpRequestThread.prototype.Init=function( requestMethod, url ){
	if ( !this.xhr ) this.xhr=new XMLHttpRequest();

	//IE9
	if (window.XDomainRequest) {
		var location = document.createElement('a');
		location.href = url;

		if ( location.hostname !== window.location.hostname){
			if ( !('withCredentials' in this.xhr) && !(this.xhr instanceof XDomainRequest) ){
				this.xhr=new XDomainRequest();
			}
		} else if (this.xhr instanceof XDomainRequest) {
			this.xhr=new XMLHttpRequest();
		}
	}

	var request = this;

	if ( !this.xhr.onload ){
		this.xhr.onload=function(e){
			request.response.status=(e.target.status) ? e.target.status : 200;
			request.response.text=e.target.responseText;

			if ( request.response.length===0 ) {
				request.response.length=e.target.responseText.length;
			}
			
			request.running=false;
		}
	}

	if ( !this.xhr.onprogress ){
		this.xhr.onprogress=function(e){
			if (e.lengthComputable) request.response.length = e.loaded;
		}
	}

	if ( !this.xhr.onerror ){
		this.xhr.onerror=function(e){
			request.response.status=(e.target.status) ? e.target.status : 0;
			request.running=false;
		}
	}

	this.response.text='';
	this.response.status=-1;
	this.response.length=0;

	this.xhr.open( requestMethod, url );
}

BBHttpRequestThread.prototype.Discard=function(){
	if ( this.xhr ) this.xhr.abort();
	this.response=null;
	this.xhr=null;
}

BBHttpRequestThread.prototype.SendRequest=function( data, mimeType  ){
	if ( data.length===0 ) data=null;
	if ( mimeType.length!==0 ) this.SetHeader( 'Content-Type', mimeType );

	this.data=data;
	this.Start();
}

BBHttpRequestThread.prototype.Run__UNSAFE__=function(){
	if ( this.xhr ){
		this.xhr.send( this.data );
	} else{
		this.running = false;
	}
}

BBHttpRequestThread.prototype.SetHeader=function( name, value ){
	if ( this.xhr && this.xhr.setRequestHeader ) this.xhr.setRequestHeader( name, value );
}

BBHttpRequestThread.prototype.BytesReceived=function(){
	return this.response.length;
}

BBHttpRequestThread.prototype.ResponseText=function(){
	return this.response.text;
}

BBHttpRequestThread.prototype.Status=function(){
	return this.response.status;
}
