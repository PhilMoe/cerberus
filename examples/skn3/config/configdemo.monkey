
Import mojo
Import config

Function Main()
	New App
	Local c1 := LoadConfig("<options><option>one</option><option>two</option><option>three</option></options>")
	Local c2 := LoadConfig("<items><item id=apple x=0 y=0 width=128 height=128 /><item id=orange x=128 y=0 width=128 height=128 /><item id=pear x=256 y=0 width=128 height=128 /></items>")
	
	'parse c1
	Print "config1:"
	Local nodes := c1.FindNodesByPath("options/option")
	For Local node := Eachin nodes
		Print " -> "+node.GetName()+" = "+node.GetValue()
	Next
	
	'parse c2
	Print "config2:"
	nodes = c2.FindNodesByPath("items/item")
	For Local node := Eachin nodes
		Print " -> "+node.GetName()+"(x="+node.GetAttribute("x")+" y="+node.GetAttribute("y")+" width="+node.GetAttribute("width")+" height="+node.GetAttribute("height")+" )"
	Next
End
