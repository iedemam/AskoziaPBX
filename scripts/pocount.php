<?php
//==========  HELP message ========== 
function print_help_msg(){
	echo("Usage:\n");
	echo("\tpocount.php -f <[path/]file> -l <language>\n");
	echo("\tExample:\n");
	echo("\tpocount -f /home/myuser/myfile.po -l en-en\n");
	return;
}

//==========  check if PO files exist ==========
function check_pofile_existant($pofile){
	
	// check if PO file exists
	if ((file_exists($pofile) && is_file($pofile))){
		return(true);
	}//if
	else{
		die ("---> error: file '".$pofile."' does not exist.\n"); 
	}
}	

//==========  calculate percentage completed of PO file ==========
function calc_percentage_completed($pocontent){

	for($i = 0; $i < count($pocontent); $i++){
		if(preg_match('/^(msgid)./', $pocontent[$i])){
			$number_of_strings++;
			if(preg_match('/^(msgstr[[:space:]]\"\")/', $pocontent[$i+1])){
				$number_of_untranslated_strings++;
			}
		}
	}
	return(round((100 - (($number_of_untranslated_strings / $number_of_strings)*100)), 0));	
}

//============================================================================================================================================

//============================================================================
//		handle command line parameters
//============================================================================	

$opts  = "f:";	// PO file to check for percentage completed
$opts .= "l:";	// language
 
$options = getopt($opts);

// if a command line option is missing, quit
if (empty($options["f"]) && empty($options["l"])){
	print_help_msg(); die();
}//if

if (!empty($options["f"])){
	$pofile = $options["f"];	
}//if
else {echo("Specify PO files using -f <[path/]file>\n"); die();} 

//if (!empty($options["l"])){
//	$language = $options["l"];	
//}//if
//else {echo("Specify language of PO file using -l <language>\n"); die();} 
 

//============================================================================
//		read PO file and print out percentage completed of translation	
//============================================================================	
//$nl = chr(10).chr(13); 		// newline	
$nl = chr(10); 					// newline
if (check_pofile_existant($pofile)){
	$pocontent = explode($nl, file_get_contents($pofile));
	echo(calc_percentage_completed($pocontent));
}
?>


