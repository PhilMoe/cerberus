
// Actionscript Cerberus runtime.
//
// Placed into the public domain 24/02/2011.
// No warranty implied; use at your own risk.

//***** ActionScript Runtime *****

import flash.display.*;
import flash.external.ExternalInterface;

//Consts for radians<->degrees conversions
var D2R:Number=0.017453292519943295;
var R2D:Number=57.29577951308232;

//private
var _errInfo:String="?<?>";
var _errStack:Array=[];

var dbg_index:int=0;

function pushErr():void{
	_errStack.push( _errInfo );
}

function popErr():void{
	_errInfo=_errStack.pop();
}

function stackTrace():String{
	if( !_errInfo.length ) return "";
	var str:String=_errInfo+"\n";
	for( var i:int=_errStack.length-1;i>0;--i ){
		str+=_errStack[i]+"\n";
	}
	return str;
}

function print( str:String ):int{
	try{
		if( ExternalInterface.available ) ExternalInterface.call( "cerberus_print",str );
	}catch( ex:Error ){
	}
	return 0;
}

function error( err:String ):int{
	throw err;
}

function debugLog( str:String ):int{
	print( str );
	return 0;
}

function debugStop():int{
	error( "STOP" );
	return 0;
}

function dbg_object( obj:Object ):Object{
	if( obj ) return obj;
	error( "Null object access" );
	return obj;
}

function dbg_array( arr:Array,index:int ):Array{
	if( index<0 || index>=arr.length ) error( "Array index out of range" );
	dbg_index=index;
	return arr;
}

function dbg_charCodeAt( str:String,index:int ):int{
	if( index<0 || index>=str.length ) error( "Character index out of range" );
	return str.charCodeAt( index );
}

function new_bool_array( len:int ):Array{
	var arr:Array=new Array( len )
	for( var i:int=0;i<len;++i ) arr[i]=false;
	return arr;
}

function new_number_array( len:int ):Array{
	var arr:Array=new Array( len )
	for( var i:int=0;i<len;++i ) arr[i]=0;
	return arr;
}

function new_string_array( len:int ):Array{
	var arr:Array=new Array( len );
	for( var i:int=0;i<len;++i ) arr[i]='';
	return arr;
}

function new_array_array( len:int ):Array{
	var arr:Array=new Array( len );
	for( var i:int=0;i<len;++i ) arr[i]=[];
	return arr;
}

function new_object_array( len:int ):Array{
	var arr:Array=new Array( len );
	for( var i:int=0;i<len;++i ) arr[i]=null;
	return arr;
}

function resize_bool_array( arr:Array,len:int ):Array{
	var i:int=arr.length;
	arr=arr.slice(0,len);
	if( len<=i ) return arr;
	arr.length=len;
	while( i<len ) arr[i++]=false;
	return arr;
}

function resize_number_array( arr:Array,len:int ):Array{
	var i:int=arr.length;
	arr=arr.slice(0,len);
	if( len<=i ) return arr;
	arr.length=len;
	while( i<len ) arr[i++]=0;
	return arr;
}

function resize_string_array( arr:Array,len:int ):Array{
	var i:int=arr.length;
	arr=arr.slice(0,len);
	if( len<=i ) return arr;
	arr.length=len;
	while( i<len ) arr[i++]="";
	return arr;
}

function resize_array_array( arr:Array,len:int ):Array{
	var i:int=arr.length;
	arr=arr.slice(0,len);
	if( len<=i ) return arr;
	arr.length=len;
	while( i<len ) arr[i++]=[];
	return arr;
}

function resize_object_array( arr:Array,len:int ):Array{
	var i:int=arr.length;
	arr=arr.slice(0,len);
	if( len<=i ) return arr;
	arr.length=len;
	while( i<len ) arr[i++]=null;
	return arr;
}

function string_compare( lhs:String,rhs:String ):int{
	var n:int=Math.min( lhs.length,rhs.length ),i:int,t:int;
	for( i=0;i<n;++i ){
		t=lhs.charCodeAt(i)-rhs.charCodeAt(i);
		if( t ) return t;
	}
	return lhs.length-rhs.length;
}

function string_replace( str:String,find:String,rep:String ):String{	//no unregex replace all?!?
	var i:int=0;
	for(;;){
		i=str.indexOf( find,i );
		if( i==-1 ) return str;
		str=str.substring( 0,i )+rep+str.substring( i+find.length );
		i+=rep.length;
	}
	return str;
}

function string_trim( str:String ):String{
	var i:int=0,i2:int=str.length;
	while( i<i2 && str.charCodeAt(i)<=32 ) i+=1;
	while( i2>i && str.charCodeAt(i2-1)<=32 ) i2-=1;
	return str.slice( i,i2 );
}

function string_tochars( str:String ):Array{
	var arr:Array=new Array( str.length );
	for( var i:int=0;i<str.length;++i ) arr[i]=str.charCodeAt(i);
	return arr;	
}

function string_startswith( str:String,sub:String ):Boolean{
	return sub.length<=str.length && str.slice(0,sub.length)==sub;
}

function string_endswith( str:String,sub:String ):Boolean{
	return sub.length<=str.length && str.slice(str.length-sub.length,str.length)==sub;
}

function string_fromchars( chars:Array ):String{
	var str:String="",i:int;
	for( i=0;i<chars.length;++i ){
		str+=String.fromCharCode( chars[i] );
	}
	return str;
}

class ThrowableObject{
	internal function toString():String{
		return "Uncaught Cerberus Exception";
	}
}
