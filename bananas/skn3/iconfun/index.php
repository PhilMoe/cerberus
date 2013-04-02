<? 
	$iconSize = 32;
	$iconPath = 'data';

	//clean the mode var
	$mode = '';
	if (isset($_GET['mode'])) { $mode = preg_replace('/[^a-z]/','',$_GET['mode']); }
	
	//make sure data folder exists
	if (file_exists($iconPath) == false) { mkdir($iconPath); }
	
	//operate based on mode
	switch($mode) {
		case 'load':
			//return all the icons at once (potentially slow with lots of icons but meh)
			//get load timestamp
			$timestamp = false;
			if (isset($_GET['timestamp'])) { $timestamp = preg_replace('/[^0-9]/','',$_GET['timestamp']); }
			
			//get the files
			$files = array();
			$dir = opendir($iconPath);
        	while (($fileName = readdir($dir)) !== false) {
        		$filePath = $iconPath.'/'.$fileName;
        		if (is_file($filePath)) {
        			//get data
        			$time = filemtime($filePath);
        			
        			//add to list only if timestamp matches
        			if ($timestamp === false || $time >= $timestamp) {
        				$data = explode("\n",file_get_contents($filePath));
						$files[$time] = array('id' => $fileName,'author' => $data[0],'title' => $data[1],'data' => $data[2]);
					}
       			}
      		}
      		closedir($dir);
      		
      		//order the files by created time
      		ksort($files,SORT_NUMERIC );
      		
      		//output files (if there are any)
      		foreach($files as $key => $file) {
      			echo "<ICON>\n".$file['id']."\n".$file['author']."\n".$file['title']."\n".$file['data']."\n".$key."\n";
      			$index++;
      		}
      		
			break;
			
		case 'save':
			//cleanup incoming data
			$author = '';
			$title = '';
			$data = '';
			
			if (isset($_GET['author'])) { $author = preg_replace('/[^a-zA-Z0-9\s\_\-]/','',$_GET['author']); }
			if (isset($_GET['title'])) { $title = preg_replace('/[^a-zA-Z0-9\s\_\-]/','',$_GET['title']); }
			if (isset($_GET['data'])) { $data = preg_replace('/[^0-9]/','',$_GET['data']); }
			
			//fix missing info
			if (strlen($author) == 0) { $author = 'anonymous'; }
			if (strlen($title) == 0) { $title = 'no title entered'; }
			
			//fix incorrect data
			$length = strlen($data);
			$count = $iconSize*$iconSize;
			if ($length < $count) {
				$data = str_pad($data,$count,'0');
			} elseif ($length > $count) {
				$data = substr($data,0,$count);
			}
			
			//create a new icon file
			file_put_contents(find_available_filename($author),$author."\n".$title."\n".$data);
			
			//return success
			echo "<SAVED>\n";
	}
	
	function find_available_filename($prefix) {
		//helper to find available filename
		global $iconPath;
		
		$count = '1';
		$file = $iconPath.'/'.$prefix.'_'.$count.'.txt';
		while (file_exists($file)){
			$count++;
			$file = $iconPath.'/'.$prefix.'_'.$count.'.txt';
		}
		return $file;
	}
?>