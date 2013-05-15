
Import microgui

Class Skin

	Method New()
		Local face:="calibri2"	'"meriyo"	'"droid"	'verdana"	'"arial"
'		Local face:="verdana"
		_bigFont=New Font( face,24 )
		_smallFont=New Font( face,16 )
		_defaultFont=New Font( face,20 )
		_controlFont=_defaultFont
	End
	
	Method BigFont:Font() Property
		Return _bigFont
	End
	
	Method SmallFont:Font() Property
		Return _smallFont
	End

	Method DefaultFont:Font() Property
		Return _defaultFont
	End
	
	Method DefaultColor:Float[]() Property
		Return _defaultColor
	End
	
	'Label
	Method MeasureLabel:Void( label:Label )
		label.SetMeasuredSize label.Font.Width( label.Text ),label.Font.Height
	End
	
	Method RenderLabel:Void( gc:GraphicsContext,label:Label )
		gc.DrawText label.Text,0,0
	End
	
	'Divider
	Method MeasureDivider:Void( divider:Divider )
		divider.SetMeasuredSize 1,1
	End
	
	Method RenderDivider:Void( gc:GraphicsContext,divider:Divider )
		gc.Color=[.7,.7,.7,1.0]
		gc.DrawRect 0,0,divider.Width,divider.Height
	End
	
	'Button
	Method MeasureButton:Void( button:Button )
		button.SetMeasuredSize button.Font.Width( button.Text )+8,button.Font.Height+8
	End
	
	Method RenderButton:Void( gc:GraphicsContext,button:Button )
		Local c:=gc.Color
		gc.Color=[.9,.9,.9,1.0]
		gc.DrawRect 0,0,button.Width,button.Height
		gc.Color=[.7,.7,.7,1.0]
		gc.DrawRect 0,0,button.Width,button.Height,False
		gc.Color=button.Color
		gc.Color=c
		gc.DrawText button.Text,4,4
	End
	
	'CheckBox
	Method MeasureCheckBox:Void( checkbox:CheckBox )
		Local h:=checkbox.Font.Height
		Local w:=checkbox.Font.Width( checkbox.Text )+h
		If checkbox.Text w+=4
		checkbox.SetMeasuredSize w,h
	End
	
	Method RenderCheckBox:Void( gc:GraphicsContext,checkbox:CheckBox )
		Local sz=checkbox.Font.Height
		gc.DrawRect 0,0,sz,sz,False
		If checkbox.IsChecked gc.DrawRect 2,2,sz-4,sz-4
		If checkbox.Text gc.DrawText checkbox.Text,sz+4,0
	End

	Function DefaultSkin:Skin()
		If Not _defaultSkin _defaultSkin=New Skin
		Return _defaultSkin
	End

	Private
	
	Global _defaultSkin:Skin
	
	Field _bigFont:Font
	Field _smallFont:Font
	Field _defaultFont:Font
	Field _controlFont:Font
	Field _defaultColor:=[.01,.01,.01,1.0]
		
End
	