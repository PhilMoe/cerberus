#rem
[March 11 2011]
 - added support for xml standard single and double quotes. Single quote must end with single and double with double
 - added support for comments <!-- comment -->
 - added destructor .Free() to clean up xml data (needs to be called manually)
 - tweaked parser to remove some unnecessary steps
#end

Class ConfigError
    Field message:String
    Field context:String
    Field line:Int
    Field column:Int
    Field data:String
    
    Method New(message:String,context:String="",data:String="",line:Int=0,column:Int=0)
        Self.message = message
        Self.context = context
        Self.data = data
        Self.line = line
        Self.column = column
    End
    
    Method ToString:String()
        Local error := ""
        If context.Length() > 0
            error += "Error In '"+context+"': "+message
        Else
            error += "Error: "+message
        Endif
        If line > 0
            error += " (line:"+line+")"
            If column > 0 error += " (column: "+column+")"
        Endif
        If data.Length() > 0 error += "~nData: "+data
        Return error
    End
End

Class ConfigNode
	Field name:String
	Field value:String
	Field parent:ConfigNode
	Field attributes:List<ConfigAttribute>
	Field children:List<ConfigNode>
	Field line:Int
	Field column:Int
	Field scriptOffset:Int

	Method New(name:String)
		Self.name = name
	End

	Method Free:Void()
		' --- free resources ---
		name = ""
		value = ""
		parent = Null
		line = 0
		column = 0
		scriptOffset = 0
		
		'free attributes
		If attributes
			For Local attribute := EachIn attributes
				attribute.Free()
			Next
			attributes.Clear()
		EndIf
		
		'free children
		If children
			For Local node := EachIn children
				node.Free()
			Next
			children.Clear()
		EndIf
	End
	
	Method ToString:String()
		' --- create debuggable print output ---
		Local build := "<"+name
		If attributes
			For Local attribute := Eachin attributes
				build += " "+attribute.name+"=~q"+attribute.value+"~q"
			Next
		Endif
		build += ">"
		Return build
	End

	Method FindValue:String(path:String,defaultValue:String="")
		Local configNode := FindNodeByPath(path)
		If configNode Return configNode.GetValue()
	End

	Method FindNodeByPath:ConfigNode(path:String)
		' --- returns first node with given path ---
		'return self if no path is given
		If path.Length() = 0 Return Self
		
		'skip if no children
		If children = Null Return Null

		'split the path into bits
		Local pathParts := path.ToLower().Split("/")
		Local nodeFound := False
		Local pointerNode:ConfigNode = Self

		'scan the path
		For Local pathIndex := 0 Until pathParts.Length()
			'validate all bits of the path
			If pathParts[pathIndex].Length() = 0 Return Null

			'check children of this pointer
			nodeFound = False
			If pointerNode.children
				For Local childNode := Eachin pointerNode.children
					If childNode.name = pathParts[pathIndex]
						pointerNode = childNode
						nodeFound = True
						Exit
					Endif
				Next
			Endif

			'check for node not found
			If nodeFound = False Return Null
		Next

		'return
		Return pointerNode
	End
	
	Method FindNodesByPath:List<ConfigNode>(path:String)
		' --- this will find a path within the children ---
		Local pointerNodes:List<ConfigNode>
		
		'return self if no path is given
		If path.Length() = 0
			pointerNodes = New List<ConfigNode>
			pointerNodes.AddLast(Self)
			Return pointerNodes
		Endif
		
		'skip if no children
		If children = Null Return Null

		'split the path into bits
		Local pathParts := path.ToLower().Split("/")
		pointerNodes = New List<ConfigNode>
		Local newPointerNodes := New List<ConfigNode>

		'add self as root
		pointerNodes.AddLast(Self)

		'scan the path
		For Local pathIndex := 0 Until pathParts.Length()
			'validate all bits of the path
			If pathParts[pathIndex].Length() = 0 Return Null

			'scan children of each pointer
			For Local pointerNode := Eachin pointerNodes
				'check children of this pointer
				If pointerNode.children
					For Local childNode := Eachin pointerNode.children
						If childNode.name = pathParts[pathIndex] newPointerNodes.AddLast(childNode)
					Next
				Endif
			Next
			pointerNodes.Clear()

			'add new pointers to scan
			For Local pointerNode := Eachin newPointerNodes
				pointerNodes.AddLast(pointerNode)
			Next
			newPointerNodes.Clear()
		Next

		'return
		If pointerNodes.IsEmpty() Return Null
		Return pointerNodes
	End
	
	Method FindNodesByName:List<ConfigNode>(name:String)
		' --- find all nodes of a given type ---
		If children = Null Or children.IsEmpty() Return Null
		
		Local returnNodes := New List<ConfigNode>
		Local stackNodes := New List<ConfigNode>
		Local stackNode:ConfigNode
		
		stackNodes.AddLast(Self)
		
		While stackNodes.IsEmpty() = False
			stackNode = stackNodes.RemoveLast()
			For Local scanNode := Eachin stackNode.children
				If scanNode.name = name returnNodes.AddLast(scanNode)
				If scanNode.children And scanNode.children.IsEmpty() = False stackNodes.AddLast(scanNode)
			Next
		Wend
		
		If returnNodes.IsEmpty() Return Null
		Return returnNodes
	End

	Method AddChild(childNode:ConfigNode)
		' --- add child to this node ---
		'set the parent of the child being added
		childNode.parent = Self

		'create the children list
		If children = Null children = New List<ConfigNode>

		'add the child
		children.AddLast(childNode)
	End

	Method GetAttribute:String(name:String,defaultValue:String="")
		' --- get an attribute value ---
		Local attribute := GetConfigAttribute(name)
		If attribute = Null Return defaultValue
		Return attribute.value
	End
	
	Method GetAttributes:List<ConfigAttribute>()
		' --- return all of thr attributes ---
		Return attributes
	End

	Method GetValue:String()
		Return value
	End

	Method GetName:String()
		Return name
	End

	Method GetChildren:List<ConfigNode>()
		Return children
	End

	Method GetConfigAttribute:ConfigAttribute(name:String)
		' --- find a property ---
		'make sure there are attributes
		If attributes = Null Return Null

		'fix casing of attribute
		name = name.ToLower()

		'search for matching attribute
		For Local attribute := Eachin attributes
			If name = attribute.name Return attribute
		Next

		Return Null
	End

	Method SetAttribute(name:String,value:String)
		' --- add a new property to this node ---
		'create properties list for first time
		If attributes = Null attributes = New List<ConfigAttribute>

		'find existing property
		Local attribute := GetConfigAttribute(name)

		'add / update attribute
		If attribute
			'update (overwrite)
			attribute.SetValue(value)
		Else
			'add
			attributes.AddLast(New ConfigAttribute(name,value))
		Endif
	End

	Method Remove()
		If parent parent.children.Remove(Self)
	End
End

Class ConfigAttribute
	Field name:String
	Field value:String
	Field segments:String[]
	
	Method New(name:String,value:String)
		' --- setup new attribute ---
		SetName(name.ToLower())
		SetValue(value)
	End
	
	Method Free:Void()
		' --- free resources ---
		name = ""
		value = ""
		segments = []
	End
	
	Private
	Method UpdateValue()
		Local length = segments.Length()
		
		If length = 0
			value = ""
			Return
		Endif
		
		If length = 1
			value = segments[0]
			Return
		Endif
		
		Local build := ""
		value = ""
		For Local index := 0 Until length
			value += segments[index]
		Next
	End
	Public
	
	Method IsSegmented:Bool()
		Return segments.Length() > 1
	End
	
	Method SetName(name:String)
		Self.name = name
	End
	
	Method GetSegments:String[]()
		Return segments
	End
	
	Method GetName:String()
		Return name
	End
	
	Method SetValue(value:String)
		segments = [value]
		UpdateValue()
	End
		
	Method AddSegment(segment:String)
		Local index := segments.Length()
		segments = segments.Resize(index+1)
		segments[index] = segment
		UpdateValue()
	End

	Method GetBoolString:String()
		If value = "1" Or value = "true"
			Return "1"
		Else
			Return "0"
		Endif
	End
	
	Method GetIntString:String()
		Return Int(value)
	End
	
	Method GetFloatString:String()
		Return Float(value)
	End
	
	Method GetInt:Int()
		Return Int(value)
	End
		
	Method GetFloat:Float()
		Return Float(value)
	End
	
	Method GetBool:Bool()
		If value = "1" Or value = "true" Return True
	End

	Method GetString:String()
		Return value
	End
	
	Method IsBool:Bool()
		'returns true if the value is a valid true/false definition
		Return value = "1" Or value = "0" Or value = "true" Or value = "false"
	End
	
	Method IsInt:Bool()
		' --- make sure this is a valid int ---
		Local asc:Int
		If value = "-" Return False
		For Local index := 0 Until value.Length()
			asc = value[index]
			If asc <> 45
				If asc < 48 Or value[index] > 57 Return False
			Else
				If index > 0 Return False
			Endif
		Next
		Return True
	End
	
	Method IsFloat:Bool()
		' --- make sure this is a valid float ---
		Local asc:Int
		Local countPeriod := 0
		If value = "-" Return False
		For Local index := 0 Until value.Length()
			asc = value[index]
			If asc <> 45
				If asc = 46
					If countPeriod > 0 Return False
					countPeriod += 1
				Else
					If (asc < 48 Or value[index] > 57) Return False
				Endif
			Else
				If index > 0 Return False
			Endif
		Next
		Return True
	End
End

Class Config Extends ConfigNode
	Field path:String
	Field script:String
	Field error:ConfigError
	
	Private	
	Method Parse:Bool(scriptRaw:String)
		' --- parse the string and fill the data objects ---
		'setup root item
		name = "root"
		
		'fix the raw script
		script = scriptRaw.Replace("~r~n","~n")
		
		'setup the stack
		Local stack := New List<ConfigNode>
		stack.AddLast(Self)
		
		'setup some parsing values (quite a few)
		Local stackTag:ConfigNode

		Local scriptLength = script.Length()
		Local scriptIndex := 0
		
		Local findTagQuote := -1
		Local findTagStart := -1
		Local findTagStartSize := 0
		Local findTagTemp := -1
		Local findTagEnd := -1
		Local findTagEndSize := 0
		Local findTag:ConfigNode
		Local findAttribute:ConfigAttribute
		Local findQuoteCharacter:Int
		Local findTagLine := 0
		Local findTagColumn := 0
		Local findName := ""
		Local findOperator := ""
		Local findSegment := ""
		Local findWhitespace := ""
		Local findLine := 1
		Local findLineStart := 0
		Local findHasPair := False
		Local findHasClose := False
		Local findHasFirst := False
		Local findHasStart := False
		Local findHasName := False
		Local findHasNameEnd := False
		Local findHasEquals := False
		Local findHasValue := False
		Local findHasComment := False
		Local findHasQuotes := False
		Local findHasEnd := False
		Local findHasSegment := False
		Local findHasSegmentEnd := False
		Local findHasOperator := False
		
		'parse the script
		While scriptIndex < scriptLength-1
			'parse looking for a complete bracket! (also skip comments)
			For Local findIndex := scriptIndex Until scriptLength
				'test for new line
				If script[findIndex] = 10
					findLine += 1
					findLineStart = findIndex + 1
				EndIf
				
				'not in comment
				If findHasStart = False
					'check to see if in comment
					If findHasComment = False
						'looking for <
						If script[findIndex] = 60
							'bracket starting
							'check if comment
							If findIndex < scriptLength-4 And script[findIndex..findIndex+4] = "<!--"
								'comment
								findHasComment = True
								findIndex += 3
							Else
								'process looking for start
								If findIndex < scriptLength-1 And script[findIndex+1] = 47
									findHasClose = True
									findTagStartSize = 2
								Else
									findTagStartSize = 1
								EndIf
								findHasStart = True
								findTagStart = findIndex
								findTagLine = findLine
								findTagColumn = (findIndex-findLineStart)+1
							EndIf
						Else
							findWhitespace += String.FromChar(script[findIndex])
						EndIf
					Else
						'in comment look for end of comment
						If script[findIndex] = 45 And findIndex + 3 < scriptLength And script[findIndex..findIndex+3] = "-->"
							findHasComment = False
							findIndex += 2
						EndIf
					EndIf
				Else
					'looking for >
					If findHasQuotes = False
						'not in quotes
						If script[findIndex] = 34
							'quote (double)
							findHasQuotes = True
							findQuoteCharacter = 34
						Else If script[findIndex] = 39
							'quote (single)
							findHasQuotes = True
							findQuoteCharacter = 39
						Else If script[findIndex] = 60
							'illegal open bracket
							Return SetError("unexpected opening bracket",findLine,(findIndex-findLineStart)+1)
						Else If script[findIndex] = 62
							'bracket ending
							If findIndex > 0 And script[findIndex-1] = 47
								findTagEndSize = 2
								findHasClose = True
								findTagEnd = findIndex - 1
							Else
								findHasPair = True
								findTagEndSize = 1
								findTagEnd = findIndex
							EndIf
							findHasEnd = True
							Exit
						EndIf
					Else
						'in quotes, can we get out please?
						If script[findIndex] = findQuoteCharacter findHasQuotes = False
					EndIf
				EndIf
			Next
			
			'check for errors
			If findHasQuotes Return SetError("expecting ending quote",findLine,scriptLength-findLineStart)
			If findHasStart And findHasEnd = False Return SetError("expecting ending bracket",findLine,scriptLength-findLineStart)

			'perform further parse on tag if found
			If findHasEnd = False
				'there are no more tags to retrieve from the string
				'check to see if any items waiting on the stack (other than root)
				If stack.Count() > 1 Return SetError("unexpected end of file",findLine,scriptLength-findLineStart)
				
				'dont add white space to root
				'Self.value += script[scriptIndex..scriptLength]

				Exit
			Else
				'reset
				findName = ""
				findOperator = ""
				findSegment = ""
				findHasEnd = False
				findHasName = False
				findHasNameEnd = False
				findHasEquals = False
				findHasValue = False
				findHasQuotes = False
				findHasSegment = False
				findHasSegmentEnd = False
				findHasOperator = False

				'add the white space
				stackTag = stack.Last()
				stackTag.value += findWhitespace'script[scriptIndex..findTagStart]

				'move the script index
				scriptIndex = findTagEnd+findTagEndSize

				'we now have an opening node so parse the node data!
				findTag = New ConfigNode()
				findTag.line = findTagLine
				findTag.column = findTagColumn

				For Local findIndex := findTagStart+findTagStartSize Until findTagEnd
					If findHasName = False
						'scanning for name
						If script[findIndex] = 32
							'space
							If findName findHasNameEnd = True
							
						Else If script[findIndex] = 10
							'new line
							If findName findHasNameEnd = True
							findLine += 1
							findLineStart = findIndex + 1
							
						Else If script[findIndex] = 61
							'=
							If findName
								findIndex -= 1
								findHasNameEnd = True
							Else
								Return SetError("expecting tag or attribute name",findLine,(findIndex-findLineStart)+1)
							EndIf
							
						Else If script[findIndex] = 95 or (script[findIndex] >= 48 and script[findIndex] <= 57) or (script[findIndex] >= 65 and script[findIndex] <= 90) or (script[findIndex] >= 97 and script[findIndex] <= 122)
							'_ a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9
							findName += String.FromChar(script[findIndex])
							
						Else
							'illegal character
							Return SetError("illegal character asc='"+script[findIndex]+"' chr='"+script[findIndex..findIndex+1]+"'",findLine,(findIndex-findLineStart)+1)
							
						EndIf
					Else
						'check for what to scan for next
						If findHasEquals = False
							'searching for equals so can start the value!
							If script[findIndex] = 32
								'space
								
							Else If script[findIndex] = 10
								'new line
								findLine += 1
								findLineStart = findIndex + 1
								
							Else If script[findIndex] = 61
								'=
								findHasEquals = True
								
							Else If script[findIndex] = 95 or (script[findIndex] >= 48 and script[findIndex] <= 57) or (script[findIndex] >= 65 and script[findIndex] <= 90) or (script[findIndex] >= 97 and script[findIndex] <= 122)
								'_ a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9
								'alpha numerics
								findHasEnd = True
								findIndex -= 1
								
							Else
								'illegal character
								Return SetError("illegal character asc='"+script[findIndex]+"' chr='"+script[findIndex..findIndex+1]+"'",findLine,(findIndex-findLineStart)+1)
								
							EndIf
						Else
							'scan through the value until an end is found!
							If findHasQuotes = False
								'look to see if there is a waiting segment to continue processing
								If findHasSegment = False
									'value hasn't completed a segment yet
									If script[findIndex] = 32
										'space
										If findSegment
											findHasSegment = True
											findHasSegmentEnd = True
										EndIf
										
									Else If script[findIndex] = 10
										'new line
										If findSegment
											findHasSegment = True
											findHasSegmentEnd = True
										EndIf
										
										findLine += 1
										findLineStart = findIndex + 1
										
									Else If script[findIndex] = 34 Or script[findIndex] = 39
										'"
										'looking for start quote
										If findSegment
											'unexepcted quote
											Return SetError("unexpected opening quote",findLine,(findIndex-findLineStart)+1)
										Else
											'start quotes
											findHasQuotes = True
											findHasValue = True
											findQuoteCharacter = script[findIndex]
										EndIf
										
									Else If script[findIndex] = 45
										'-
										'looking for negative value or operator
										If findSegment
											'operator
											findHasSegment = True
											findHasSegmentEnd = True
											findHasOperator = True
											findOperator = String.FromChar(script[findIndex])
										Else
											'negative value
											findSegment += String.FromChar(script[findIndex])
											findHasValue = True
										EndIf
										
									Else If script[findIndex] = 42 or script[findIndex] = 43 or script[findIndex] = 47
										'+
										'looking for (other non -) operators
										If findHasValue = False
											'unexpected oeprator
											Return SetError("unexpected operator asc='"+script[findIndex]+"' chr='"+script[findIndex..findIndex+1]+"'",findLine,(findIndex-findLineStart)+1)
										Else
											'end of value
											findHasSegmentEnd = True
											findHasOperator = True
											findOperator = String.FromChar(script[findIndex])
										EndIf
										
									Else If script[findIndex] = 36 or script[findIndex] = 46 or script[findIndex] = 95 or (script[findIndex] >= 48 and script[findIndex] <= 57) or (script[findIndex] >= 65 and script[findIndex] <= 90) or (script[findIndex] >= 97 and script[findIndex] <= 122)
										'$ . _ a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9
										'looking for unquoted attribute value
										findSegment += String.FromChar(script[findIndex])
										findHasValue = True
										
									Else
										'ilegal character
										Return SetError("illegal character asc='"+script[findIndex]+"' chr='"+script[findIndex..findIndex+1]+"'",findLine,(findIndex-findLineStart)+1)
									EndIf
									
								Else
									'value already has segment so need to look for end or operator
									If script[findIndex] = 32
										'space
										
									Else If script[findIndex] = 10
										'new line
										findLine += 1
										findLineStart = findIndex + 1
										
									Else If script[findIndex] = 95 or (script[findIndex] >= 48 and script[findIndex] <= 57) or (script[findIndex] >= 65 and script[findIndex] <= 90) or (script[findIndex] >= 97 and script[findIndex] <= 122)
										'_ a b c d e f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9
										'looking for new attribute so must rescan this character
										findHasSegment = False
										findIndex -= 1
										findHasEnd = True
										
									Else If script[findIndex] = 42 Or script[findIndex] = 43 or script[findIndex] = 45 or script[findIndex] = 47
										'+ -
										'looking for operators
										findHasSegment = False
										findHasOperator = True
										findOperator = String.FromChar(script[findIndex])
										
									Else
										Return SetError("illegal character asc='"+script[findIndex]+"' chr='"+script[findIndex..findIndex+1]+"'",findLine,(findIndex-findLineStart)+1)
									EndIf
								EndIf
							Else
								'quotes mode (anything goes) (possibly need to do escape quote character at some point)
								If script[findIndex] = 10
									'new line
									findLine += 1
									findLineStart = findIndex + 1
									
								Else If script[findIndex] = findQuoteCharacter
									'looking for end of quotes
									findHasSegment = True
									findHasSegmentEnd = True
									findHasQuotes = False
									findQuoteCharacter = 0
								Else
									'any character add to value
									findSegment += String.FromChar(script[findIndex])
									
								EndIf
							EndIf
						EndIf
					EndIf

					'look for forced end (eol)
					If findIndex = findTagEnd-1
						findHasEnd = True
						
						If findHasQuotes
							'unclosed "
							Return SetError("exepcting closing quote",findLine,(findIndex-findLineStart)+1)
						Else
							If findHasName = False
								If findName findHasNameEnd = True
							Else
								If findHasEquals = True
									If findHasValue = False And findSegment = ""
										'nothing after =
										Return SetError("expecting attribute value",findLine,(findIndex-findLineStart)+1)
									Else
										If findHasOperator
											'nothing after operator
											Return SetError("unexcpected end of attribute",findLine,(findIndex-findLineStart)+1)
										Else
											If findSegment <> "" findHasSegmentEnd = True
										EndIf
									EndIf
								EndIf
							EndIf
						EndIf
					EndIf
					
					'look at setting up tag name
					If findHasNameEnd
						'tag name						
						If findHasFirst = False
							findHasFirst = True
							findTag.name = findName.ToLower()
						Else
							findHasStart = True
						EndIf
						
						'say that name is found
						findHasName = True
						
						'reset
						findHasNameEnd = False
					EndIf
					
					If findHasName
						'look at adding segment to value
						If findHasSegmentEnd = True
							'look for appending existing attribute
							findAttribute = findTag.GetConfigAttribute(findName)
							If findAttribute = Null
								findTag.SetAttribute(findName,findSegment)
							Else
								findAttribute.AddSegment(findSegment)
							EndIf
							
							'reset
							findHasSegmentEnd = False
							findSegment = ""
						EndIf
						
						'look for adding operator to value
						If findHasOperator = True
							'look for appending existing attribute
							findAttribute = findTag.GetConfigAttribute(findName)
							If findAttribute = Null
								findTag.SetAttribute(findName,findOperator)
							Else
								findAttribute.AddSegment(findOperator)
							EndIf
							
							'reset
							findHasOperator = False
							findOperator = ""
						EndIf
					EndIf
					
					If findHasEnd
						'reset
						findName = ""
						findOperator = ""
						findSegment = ""
						findHasEnd = False
						findHasName = False
						findHasNameEnd = False
						findHasEquals = False
						findHasValue = False
						findHasQuotes = False
						findHasSegment = False
						findHasSegmentEnd = False
						findHasOperator = False
					EndIf
				Next

				'node is complete - what next
				If findHasClose
					If findHasPair
						'close the node from the stack
						stackTag = stack.RemoveLast()
						If stackTag <> Null
							If stackTag.name = findTag.name
								'close the node and add to parent
								stackTag.parent.AddChild(stackTag)
							Else
								'mis matched node
								Return SetError("open tag name '"+stackTag.name+"' doesn't match closing tag name '"+findTag.name+"'",findTag.line,findTag.column)
							EndIf
						Else
							'error - closing unopened node
							Return SetError("unexpected closing tag '"+findTag.name+"'",findTag.line,findTag.column)
						EndIf
					Else
						'close the node by itself and add to parent!
						findTag.parent = stack.Last()
						findTag.parent.AddChild(findTag)
					EndIf
				Else
					'add the node to the stack!
					'set parent first
					findTag.parent = stack.Last()
					stack.AddLast(findTag)
				EndIf

				'reset flags/values
				findHasStart = False
				findHasClose = False
				findHasPair = False
				findHasFirst = False
				findWhitespace = ""
			EndIf
		Wend
		
		'return parse ok
		Return True
	End
	
	Method SetError:Bool(message:String,line:Int=0,column:Int=0,data:String="")
		' --- error in parse ---
		error = New ConfigError(message,path,data,line,column)
		If attributes attributes.Clear()
		If children children.Clear()
		Return False
	End
	Public
	
	Method Free:Void()
		' --- free resources ---
		'call chain
		Super.Free()
		
		'free own resources
		path = ""
		script = ""
		error = Null
	End
	
	Method HasError:Bool()
		Return error <> Null
	End
End

Function LoadConfig:Config(script:String)
	'create the config
	Local config := New Config
	config.path = ""
		
	'parse the script
	config.Parse(script)
	Return config	
End
