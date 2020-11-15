
class BBCerberusGame extends BBFlashGame{

	internal static var _cerberusGame:BBCerberusGame;
	
	public function BBCerberusGame( root:DisplayObjectContainer ){
		super( root );
	}
	
	public static function Main( root:DisplayObjectContainer ):void{
		
		_cerberusGame=new BBCerberusGame( root );

		try{
		
			bbInit();
			bbMain();
			
		}catch( ex:Object ){
		
			_cerberusGame.Die( ex );
			return;
		}
		
		if( !_cerberusGame.Delegate() ) return;
		
		_cerberusGame.Run();
	}
}
