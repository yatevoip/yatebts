<?php
$url = (isset($_SERVER['HTTPS']) ? "https://" : "http://")
    . $_SERVER['SERVER_NAME'] . ":" . $_SERVER['SERVER_PORT'] . "/lmi/";
header ("Location: $url");
?><html>
<head>
<title>Redirecting...</title>
</head>
<body>
<a href="<?php print $url; ?>">Redirecting...</a>
</body>
</html>