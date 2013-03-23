
class BBMonkeyGame extends BBFlashGame{

	internal static var _monkeyGame:BBMonkeyGame;
	
	public function BBMonkeyGame( root:DisplayObjectContainer ){
		super( root );
	}
	
	public static function Main( root:DisplayObjectContainer ):void{
		
		_monkeyGame=new BBMonkeyGame( root );

		try{
		
			bbInit();
			bbMain();
			
		}catch( ex:Object ){
		
			_monkeyGame.Die( ex );
			return;
		}
		
		if( !_monkeyGame.Delegate() ) return;
		
		_monkeyGame.Run();
	}
}
