<?php
/**
 * Copyright (C) 2014 Null Team
 *
 * This software is distributed under multiple licenses;
 * see the COPYING file in the main directory for licensing
 * information for this specific distribution.
 *
 * This use of this software may be subject to additional restrictions.
 * See the LEGAL file in the main directory for details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */ 


#include ANSQL project
if (is_dir("../../ansql"))
	$ansql_path = "../../";
elseif (is_dir("../ansql/"))
	$ansql_path = "../";
else
	exit("Could not locate ansql library.");

set_include_path( get_include_path().PATH_SEPARATOR.$ansql_path);
require_once("ansql/lib_files.php");


JsObjFile::runUnitTests();

?>
