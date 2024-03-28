<?php
if(isset($_POST['stringData']))
	{
    //name of get_included_files
    $fileName = "currentText.json";
    //the current array of strings to be saved
    $inPhp = $_POST['stringData'];
    //format as a json formatted key value pair
    $dataSubmitted= array('line'=>$inPhp);
    //get the current file contents
    $data_results = file_get_contents($fileName);
    //decode if WE WANT TO KEEP RESULTS (uncomment)
   //$tempArray = json_decode($data_results);
    //append additional data to json file
    $tempArray[]=$dataSubmitted;
		//reencode
    $jsonData = json_encode($tempArray);
		//write to file
    file_put_contents($fileName, $jsonData);
}
?>
