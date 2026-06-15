<?php
// Note: If you are using 'php-cgi', it automatically adds the 'Content-Type: text/html' header.
// If you are using 'php' (cli), you might need to manually echo headers first depending on your server logic.

$method = $_SERVER['REQUEST_METHOD'] ?? 'UNKNOWN';
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webserv CGI Test</title>
    <style>
        body { font-family: system-ui, sans-serif; background: #f4f4f9; color: #333; padding: 2rem; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 2rem; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.1); }
        h1 { color: #4f46e5; border-bottom: 2px solid #e5e7eb; padding-bottom: 10px; }
        h2 { color: #d97706; margin-top: 1.5rem; }
        table { width: 100%; border-collapse: collapse; margin-top: 10px; font-size: 0.9rem; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f8fafc; width: 30%; }
        code { background: #eee; padding: 2px 5px; border-radius: 4px; color: #d63384; }
        .method-badge { background: #4f46e5; color: white; padding: 4px 10px; border-radius: 4px; font-size: 0.9rem; }
    </style>
</head>
<body>

<div class="container">
    <h1>CGI GET Execution Successful! 🎉</h1>
    <p>If you are seeing this rendered as HTML, your server successfully executed the PHP binary and returned the output.</p>
    
    <p><strong>Request Method Received:</strong> <span class="method-badge"><?php echo htmlspecialchars($method); ?></span></p>

    <?php if ($method !== 'GET'): ?>
        <p style="color: #991b1b; background: #fee2e2; padding: 10px; border-radius: 5px; border: 1px solid #f87171;">
            ⚠️ <strong>Warning:</strong> This script is designed to test <code>GET</code> requests, but received a <code><?php echo htmlspecialchars($method); ?></code> request.
        </p>
    <?php endif; ?>

    <h2>1. GET Parameters (Query String)</h2>
    <p>Testing if your server correctly passed the <code>QUERY_STRING</code> environment variable.</p>
    <table>
        <tr><th>Key</th><th>Value</th></tr>
        <?php
        if (empty($_GET)) {
            echo "<tr><td colspan='2'><em>No GET parameters found. Try adding ?name=test&age=25 to the URL.</em></td></tr>";
        } else {
            foreach ($_GET as $key => $value) {
                echo "<tr><td><code>" . htmlspecialchars($key) . "</code></td><td>" . htmlspecialchars($value) . "</td></tr>";
            }
        }
        ?>
    </table>

    <h2>2. Server Environment Variables</h2>
    <p>Your C++ server must pass these to the <code>execve</code> function via the <code>envp</code> array.</p>
    <table>
        <tr><th>Variable Name</th><th>Value Set By Your Server</th></tr>
        <?php
        $required_vars = [
            'SERVER_PROTOCOL', 'REQUEST_METHOD', 'PATH_INFO', 'PATH_TRANSLATED', 
            'SCRIPT_NAME', 'QUERY_STRING', 'CONTENT_TYPE', 'CONTENT_LENGTH', 
            'SERVER_PORT', 'SERVER_NAME', 'HTTP_USER_AGENT', 'HTTP_COOKIE'
        ];
        
        foreach ($required_vars as $var) {
            $val = $_SERVER[$var] ?? '<span style="color:red">NOT SET</span>';
            echo "<tr><td><code>$var</code></td><td>$val</td></tr>";
        }
        ?>
    </table>
    
    <br>
    <hr>
    <p style="text-align:center; color:#666; font-size:0.8rem;">Parsed by PHP v<?php echo phpversion(); ?> via CGI</p>
</div>

</body>
</html>