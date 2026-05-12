<?php
// Stop output buffering to prevent memory exhaustion and corrupted binary streams
if (ob_get_level()) {
    ob_end_clean();
}

// 1. RAW VIDEO STREAMING LOGIC & 206 PARTIAL CONTENT
if (isset($_GET['stream']) && $_GET['stream'] === 'true') {
    // FIXED: Look for the video in the same directory as the script
    $file = './test.mp4';
    
    if (!file_exists($file)) {
        header("Status: 404 Not Found");
        header("HTTP/1.1 404 Not Found");
        echo "Error: test_video.mp4 not found on the server.";
        exit;
    }

    $filesize = filesize($file);
    $start = 0;
    $end = $filesize - 1;
    $length = $filesize;

    // Tell the browser we support Range requests
    header('Accept-Ranges: bytes');
    header('Content-Type: video/mp4');

    // Check if the browser sent a Range header
    if (isset($_SERVER['HTTP_RANGE'])) {
        $range = $_SERVER['HTTP_RANGE'];
        
        if (strpos($range, 'bytes=') === 0) {
            $range = substr($range, 6);
            $range_parts = explode('-', $range);
            
            $c_start = $range_parts[0] === '' ? false : intval($range_parts[0]);
            $c_end = (isset($range_parts[1]) && $range_parts[1] !== '') ? intval($range_parts[1]) : $end;

            if ($c_start === false) {
                $c_start = $filesize - $c_end;
                $c_end = $end;
            }
            
            $c_end = min($c_end, $end);
            
            if ($c_start > $c_end || $c_start >= $filesize) {
                header('HTTP/1.1 416 Requested Range Not Satisfiable');
                header('Status: 416 Requested Range Not Satisfiable');
                header("Content-Range: bytes */$filesize");
                exit;
            }

            $start = $c_start;
            $end = $c_end;
            $length = $end - $start + 1;

            header('HTTP/1.1 206 Partial Content');
            header('Status: 206 Partial Content');
            header("Content-Range: bytes $start-$end/$filesize");
        }
    } else {
        header('HTTP/1.1 200 OK');
        header('Status: 200 OK');
    }

    header("Content-Length: $length");

    $fp = fopen($file, 'rb');
    fseek($fp, $start);

    // Read and output the requested chunk in 64KB pieces
    $buffer_size = 1024 * 64;
    $bytes_left = $length;
    
    // connection_status() == 0 ensures we stop if the browser disconnects
    while (!feof($fp) && $bytes_left > 0 && connection_status() == 0) {
        $read_size = min($buffer_size, $bytes_left);
        $data = fread($fp, $read_size);
        if ($data === false) break;
        echo $data;
        flush(); 
        $bytes_left -= strlen($data);
    }
    
    fclose($fp);
    exit;
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Video Stream Test</title>
</head>
<body>
    <div style="max-width: 800px; margin: 0 auto; font-family: sans-serif;">
        <h1>Webserv Video Streaming</h1>
        
        <div class="video-container">
            <?php
                $video_src = htmlspecialchars($_SERVER['PHP_SELF']) . "?stream=true"; 
            ?>
            <video width="100%" controls preload="metadata">
                <source src="<?php echo $video_src; ?>" type="video/mp4">
                Your browser does not support the video tag.
            </video>
        </div>

        <div class="info" style="margin-top: 20px; padding: 15px; background-color: #fcf8e3; border-left: 4px solid #f0ad4e;">
            <p><strong>Note for Webserv Project:</strong></p>
            <p>1. Make sure you place a valid <code>test_video.mp4</code> in the same directory as this script.</p>
            <p>2. <strong>CGI Range Handling:</strong> This script fully implements HTTP 206 Partial Content natively in PHP.</p>
        </div>
    </div>
</body>
</html>