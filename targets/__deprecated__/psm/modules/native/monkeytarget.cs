
public class BBMonkeyGame : BBPsmGame{

	public static void Main( String[] args ){
	
		BBMonkeyGame game=new BBMonkeyGame();
		
		try{
		
			bb_.bbInit();
			bb_.bbMain();
			
		}catch( Exception ex ){

			if( game.Die( ex ) ) throw;
			return;
		}
		
		if( game.Delegate()==null ) return;
		
		game.Run();
	}
}
