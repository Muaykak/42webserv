#!/usr/bin/php-cgi
<?php
// Note: The php-cgi executable will automatically prepend the necessary 
// HTTP headers (like 'Content-Type: text/html\r\n\r\n') to this output.

echo "<!DOCTYPE html>\n";
echo "<html lang='en'>\n";
echo "<head>\n";
echo "    <meta charset='UTF-8'>\n";
echo "    <title>Webserv CGI Test Result</title>\n";
echo "    <style>\n";
echo "        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background-color: #f4f4f9; color: #333; max-width: 800px; margin: 40px auto; padding: 20px; }\n";
echo "        .container { background-color: white; border-radius: 8px; padding: 30px; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); }\n";
echo "        h1 { color: #2c3e50; border-bottom: 2px solid #eee; padding-bottom: 10px; margin-top: 0; }\n";
echo "        h2 { color: #34495e; font-size: 1.2em; margin-top: 0; }\n";
echo "        .block { background: #f8f9fa; border: 1px solid #e9ecef; border-left: 4px solid #3498db; padding: 15px; margin-bottom: 20px; border-radius: 4px; }\n";
echo "        .block.post { border-left-color: #2ecc71; }\n";
echo "        .block.env { border-left-color: #9b59b6; }\n";
echo "        pre { background: #272822; color: #f8f8f2; padding: 15px; border-radius: 5px; overflow-x: auto; }\n";
echo "        .highlight { color: #e74c3c; font-weight: bold; }\n";
echo "    </style>\n";
echo "</head>\n";
echo "<body>\n";
echo "<div class='container'>\n";
echo "    <h1>PHP-CGI Response</h1>\n";

echo "    <div class='block'>\n";
echo "        <h2>📍 Request Info</h2>\n";
$method = isset($_SERVER['REQUEST_METHOD']) ? $_SERVER['REQUEST_METHOD'] : 'UNKNOWN';
echo "        <strong>Method:</strong> <span class='highlight'>" . htmlspecialchars($method) . "</span><br>\n";
echo "        <strong>Query String:</strong> " . htmlspecialchars(isset($_SERVER['QUERY_STRING']) ? $_SERVER['QUERY_STRING'] : '') . "<br>\n";
echo "    </div>\n";

echo "    <div class='block'>\n";
echo "        <h2>📥 GET Data (\$_GET)</h2>\n";
echo "        <pre>";
if (empty($_GET)) {
    echo "No GET data found.";
} else {
    print_r($_GET);
}
echo "        </pre>\n";
echo "    </div>\n";

echo "    <div class='block post'>\n";
echo "        <h2>📤 POST Data (\$_POST)</h2>\n";
echo "        <p><em>If this is empty during a POST request, check if your webserv is setting <strong>CONTENT_TYPE</strong> and <strong>CONTENT_LENGTH</strong> correctly!</em></p>\n";
echo "        <pre>";
if (empty($_POST)) {
    echo "No POST data found (or unable to parse).";
} else {
    print_r($_POST);
}
echo "        </pre>\n";
echo "    </div>\n";

echo "    <div class='block post'>\n";
echo "        <h2>📦 Raw Body (from stdin)</h2>\n";
echo "        <p><em>This shows exactly what your webserv piped to the CGI script's stdin.</em></p>\n";
$raw_body = file_get_contents('php://input');
echo "        <pre>";
if (empty($raw_body)) {
    echo "(Empty body)";
} else {
    echo htmlspecialchars($raw_body);
}
echo "        </pre>\n";
echo "    </div>\n";

echo "    <div class='block env'>\n";
echo "        <h2>⚙️ Essential CGI Environment Variables</h2>\n";
echo "        <p><em>These are the variables your webserv passed via execve().</em></p>\n";
echo "        <pre>";
$cgi_vars = [
    'SERVER_PROTOCOL', 'SERVER_PORT', 'REQUEST_METHOD', 'PATH_INFO', 
    'SCRIPT_NAME', 'QUERY_STRING', 'CONTENT_TYPE', 'CONTENT_LENGTH', 
    'HTTP_HOST', 'HTTP_USER_AGENT', 'HTTP_ACCEPT'
];

$found_vars = false;
foreach ($cgi_vars as $var) {
    if (isset($_SERVER[$var])) {
        echo str_pad($var, 20) . ": " . htmlspecialchars($_SERVER[$var]) . "\n";
        $found_vars = true;
    }
}
if (!$found_vars) {
    echo "No standard CGI variables found in \$_SERVER. Are you passing the envp array correctly in execve?";
}
echo "        </pre>\n";
echo "    </div>\n";

echo "</div>\n";
echo "</body>\n";
echo "</html>\n";
?>