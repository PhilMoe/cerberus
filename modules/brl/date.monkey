
Extern Private

Class BBDate
	Field _year
	Field _month
	Field _day
	Field _hours
	Field _minutes
	Field _seconds
End

Public 

Class Date Extends BBDate

	Method Year:Int()
		Return _year
	End
	
	Method Month:Int()
		Return _month
	End
	
	Method Day:Int()
		Return _day
	End
	
	Method Hours:Int()
		Return _hours
	End
	
	Method Minutes:Int()
		Return _minutes
	End
	
	Method Seconds:Int()
		Return _seconds
	End
	
	Method ToString:String()
		Local day:=("0"+date[2])[-2..]
		Local month:=["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"][date[1]]
		Local year:=""+date[0]
		Local hours:=("0"+date[3])[-2..]
		Local mins:=("0"+date[4])[-2..]
		Local secs:=("0"+date[5])[-2..]
		Return day+" "+month+" "+year+" "+hours+":"+mins+":"+secs
	End
End
