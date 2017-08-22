<?php
require_once("ansql/lib.php");
require_once("defaults.php");
global $pysim_csv,$yate_conf_dir;

if (getparam("file")) {
	$filename = getparam("file");
	$path_file = $yate_conf_dir .$filename;
	$fp = @fopen($path_file,'rb');
} else {
	$fp = @fopen($pysim_csv, 'rb');
	$filename = str_replace($yate_conf_dir, "", $pysim_csv);
}
header('Content-Type: "application/octet-stream"');
header('Content-Disposition: attachment; filename="'.$filename.'"');
header("Content-Transfer-Encoding: binary");
header('Expires: 0');
header('Pragma: no-cache');
if (getparam("file"))
	header("Content-Length: ".filesize($yate_conf_dir .getparam("file")));
else
	header("Content-Length: ".filesize($pysim_csv));

fpassthru($fp);
fclose($fp);
?>
