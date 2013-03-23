
function BBThread(){
	this.running=false;
}

BBThread.prototype.Start=function(){
	this.running=true;
	this.Run__UNSAFE__();
}

BBThread.prototype.IsRunning=function(){
	return this.running;
}

BBThread.prototype.Run__UNSAFE__=function(){
	this.running=false;
}
