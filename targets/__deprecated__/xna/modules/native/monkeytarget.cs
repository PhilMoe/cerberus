
class BBMonkeyGame : BBXnaGame{

	public BBMonkeyGame( Game app ) : base( app ){
	}
}

public class MonkeyGame : Game{

	BBMonkeyGame _game;

	public MonkeyGame(){
		_game=new BBMonkeyGame( this );
	}
	
	protected override void LoadContent(){
	
		try{
		
			bb_.bbInit();
			bb_.bbMain();
			
		}catch( Exception ex ){
		
			if( _game.Die( ex ) ) throw;
			return;
		}
		
		if( _game.Delegate()==null ){
			Exit();
			return;
		}
		
		_game.Run();
	}
	
	protected override void Update( GameTime gameTime ){
		_game.Update( gameTime );
		base.Update( gameTime );
	}
	
	protected override bool BeginDraw(){
		return _game.BeginDraw() && base.BeginDraw();
	}

	protected override void Draw( GameTime gameTime ){
		_game.Draw( gameTime );
		base.Draw( gameTime );
	}
	
#if !WINDOWS_PHONE
	public static void Main(){
		new MonkeyGame().Run();
	}
#endif
}
