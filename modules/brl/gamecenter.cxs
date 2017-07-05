
#If TARGET<>"ios"
#Error "The GameCenter module is only available for the ios target"
#End

Import "native/gamecenter.ios.cpp"

#LIBS+="GameKit.framework"

Extern 

Class GameCenter Extends Null="BBGameCenter"

	Function GetGameCenter:GameCenter()
	
	Method GameCenterAvail:Bool()
	
	Method StartGameCenter:Int()
	
	Method GameCenterState:Int()
	
	Method ShowLeaderboard:Void( leaderboard_ID:String )
	
	Method ReportScore:Void( value:Int,leaderboard_ID:String )
	
	Method ShowAchievements:Void()
	
	Method ReportAchievement:Void( percent:Float,achievement_ID:String )
	
	Method GetAchievementPercent:Float( achievement_ID:String )
	
End
