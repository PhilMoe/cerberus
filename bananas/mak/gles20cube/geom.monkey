
Import vec3
Import mat4

#rem

The geom classes use a 'temporaries' system to avoid having to call New.

Basically, all methods/functions in this module that return a geom object return a temporary, or 'tmp', object.

A tmp object will last until you call ClearTmps or PopTmps. After this, tmps get reused so you must assign them to your own
objects using 'Set' if you want to retain them.

Each geom object constructor has a correspond Tmp() function - use these to create your own tmp objects.

Basically, if you follow these rules you should be OK:

* Call ClearTmps at the start of every OnUpdate/OnRender.

* When assigning a geom object to a global or a field, use Set() (unless you are assigning an object created with New).

* When assigning a geom object to a local, use '='.

* Use PushTmps to create a new 'scope' for tmps. Tmps created after a PushTmps will exist until a corresponding call
to PopTmps. This can be useful if you do a lot of tmp allocation, particularly inside loops. Otherwise you can end up using
hundreds/thousands of tmps.

A quick example:

Class Entity

	Method SetPos( pos:Vec3 )
		_pos.Set pos
	End
	
	Method GetPos:Vec3()
		Return _pos
	End
	
	Method SetVel( vel:Vec3 )
		_vel.Set vel
	End
	
	Method GetVel:Vec3()
		Return _vel
	End
	
	Method Update()
		_pos.Set _pos.Plus( _vel )
	End
	
	Private
	
	Field _pos:=New Vec3	'could use your own object pool system here...
	Field _vel:=New Vec3	'ditto...
	
End

#end

Private

Global maxVec3Tmps
Global maxMat4Tmps
Global tmpStack:=New IntStack

Function UpdateMaxs()
	If Vec3.GetTmps()>maxVec3Tmps
		maxVec3Tmps=Vec3.GetTmps()
		Print "Max Vec3 tmps="+maxVec3Tmps
	Endif
	If Mat4.GetTmps()>maxMat4Tmps
		maxMat4Tmps=Mat4.GetTmps()
		Print "Max Mat4 tmps="+maxMat4Tmps
	Endif
End

Public

Function ClearTmps()
	UpdateMaxs
	tmpStack.Clear
	Mat4.SetTmps 0
	Vec3.SetTmps 0
End

Function PushTmps()
	tmpStack.Push Vec3.GetTmps()
	tmpStack.Push Mat4.GetTmps()
End

Function PopTmps()
	UpdateMaxs
	If tmpStack.IsEmpty()
		Mat4.SetTmps 0
		Vec3.SetTmps 0
	Else
		Mat4.SetTmps tmpStack.Pop()
		Vec3.SetTmps tmpStack.Pop()
	Endif
End

