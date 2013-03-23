
function BBXMLHttpRequest( req,url,result ){
	var xhr=new XMLHttpRequest();
	xhr.onload=function(){
		result[1]=xhr.responseText;
		result[0]=xhr.status;
	}
	xhr.onerror=function(){
		result[1]="OOPS!";
		result[0]=-1;
	}
	xhr.open( req,url,true );
	xhr.send();
}
