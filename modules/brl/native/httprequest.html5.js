
function BBHttpRequest(){
	this.response = {
		text: '',
		status: -1,
		length: 0
	}
}

BBHttpRequest.prototype.Open=function( requestMethod, url ){
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

BBHttpRequest.prototype.Discard=function(){
	if ( this.xhr ) this.xhr.abort();
	this.response=null;
	this.xhr=null;
}

BBHttpRequest.prototype.SetHeader=function( name, value ){
	if ( this.xhr && this.xhr.setRequestHeader ) this.xhr.setRequestHeader( name, value );
}

BBHttpRequest.prototype.Send=function(){
	this.data=this.encoding=null;
	this.Start();
}

BBHttpRequest.prototype.SendText=function( data, encoding ){
	this.data=data;
	this.encoding=encoding;
	this.Start();
}

BBHttpRequest.prototype.Start=function(){
	if ( this.xhr ){
		this.running=true;
		this.xhr.send( this.data );
	}
}

BBHttpRequest.prototype.BytesReceived=function(){
	return this.response.length;
}

BBHttpRequest.prototype.ResponseText=function(){
	return this.response.text;
}

BBHttpRequest.prototype.Status=function(){
	return this.response.status;
}

BBHttpRequest.prototype.IsRunning=function(){
	return this.running;
}
