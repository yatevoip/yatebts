<?php
$url = (isset($_SERVER['HTTPS']) ? "https://" : "http://") . $_SERVER['SERVER_NAME'] . "/lmi/";
header ("Location: $url");
?><html>
<head>
<title>Redirecting...</title>
</head>
<body>
<a href="<?php print $url; ?>">Redirecting...</a>
</body>
</html>