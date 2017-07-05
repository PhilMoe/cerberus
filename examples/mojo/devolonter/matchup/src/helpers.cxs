Strict

Class ArrayHelper<T>
	
	Function Shuffle:Void(arr:T[], howManyTimes:Int)
		Local i:Int 
		Local index1:Int
		Local index2:Int
		Local object:T
		
		While(i < howManyTimes)
			index1 = Rnd() * arr.Length()
			index2 = Rnd() * arr.Length()
			object = arr[index2]
			arr[index2] = arr[index1]
			arr[index1] = object
			i += 1	
		Wend
	End Function

End Class