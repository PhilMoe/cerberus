
#If TARGET="xna" Or TARGET="android"
Import mojo
#End

Interface IBase
	Method Blah()
End

Interface Drawable Extends IBase
	Method Draw()
End

Interface Killable Extends IBase
	Method Kill()
End

Interface Bonkable Extends Drawable,Killable
	Method Bonk()
End

Class Actor Implements Drawable
	Method Blah()
		Print "Actor.Blah"
	End
	Method Draw()
		Print "Actor.Draw"
	End
End

Class Player Extends Actor Implements Bonkable
	Method Blah()
		Print "Player.Blah"
	End
	Method Draw()
		Print "Player.Draw"
	End
	Method Kill()
		Print "Player.Kill"
	End
	Method Bonk()
		Print "Player.Bonk"
	End
End

Function Test:Object( actor:Actor )
	actor.Draw
End

Function Draw:Object( drawable:Drawable )
	If Not drawable Return
	drawable.Draw
	Return drawable
End

Function Kill:Object( killable:Killable )
	If Not killable Return
	killable.Kill
	Return killable
End

Function Bonk:Object( bonkable:Bonkable )
	If Not bonkable Return 
	bonkable.Bonk
	Return bonkable
End

Function Main()

	Test New Player
	
	Print ""
	
	Local player:Player=New Player

	If Draw( player )<>player Error "ERR!"
	If Kill( player )<>player Error "ERR!"
	If Bonk( player )<>player Error "ERR!"
	
	Print ""
	
	Local list:=New List<Drawable>
	
	Local x:Drawable=New Player
	
	list.AddLast x'New Player
	
	For Local it:=Eachin list
	
		Local bonkable:=Bonkable( it )

		Local o:Object=bonkable

		bonkable.Blah		
		bonkable.Kill
	Next
	
End
