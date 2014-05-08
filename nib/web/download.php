<?php

require_once("defaults.php");
global $pysim_csv,$yate_conf_dir;

$fp = @fopen($pysim_csv, 'rb');
$filename = str_replace($yate_conf_dir, "", $pysim_csv);
header('Content-Type: "application/octet-stream"');
header('Content-Disposition: attachment; filename="'.$filename.'"');
header("Content-Transfer-Encoding: binary");
header('Expires: 0');
header('Pragma: no-cache');
header("Content-Length: ".filesize($pysim_csv));

fpassthru($fp);
fclose($fp);
?>
