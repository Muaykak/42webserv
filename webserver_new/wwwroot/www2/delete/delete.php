<?php

$targetFolderName = "uploads" ;
$dirHandler = opendir(sprintf("../%s" , $targetFolderName));
if(empty($dirHandler))
{
    echo "<h1>Cannot open target directory</h1>"; 
    exit (0);
}


?>
<!DOCTYPE html>
<html><head><title>Directory Listing</title>
<style>
:root {
    --bg-color: #0d1117;
    --text-main: #c9d1d9;
    --btn-bg: #21262d;
    --btn-hover: #30363d;
    --btn-border: rgba(240, 246, 252, 0.1);
    --accent-delete: #f85149;
    --accent-home: #3fb950;
    }

body {
    margin: 0;
    padding: 20px;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    background-color: var(--bg-color);
    color: var(--text-main);
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 90vh;
    }

.container {
        background-color: var(--btn-bg);
        border: 1px solid var(--btn-border);
        border-top: 4px solid var(--accent-delete);
        border-radius: 12px;
        padding: 30px 40px;
        width: 100%;
        max-width: 600px;
        box-shadow: 0 8px 24px rgba(0, 0, 0, 0.5);
      }

h1 {
        font-size: 1.8rem;
        text-align: center;
        margin-top: 0;
        margin-bottom: 25px;
        color: #ffffff;
        letter-spacing: 1px;
    }

ul {
        list-style: none;
        padding: 0;
        margin: 0;
    }

ul li {
        border-bottom: 1px solid var(--btn-border);
        transition: background-color 0.2s ease;
        border-radius: 6px;
    }


ul li:last-child {
        border-bottom: none;
    }

ul li:hover {
        background-color: var(--btn-hover);
      }

ul li a {
        text-decoration: none;
        color: var(--text-main);
        font-weight: 500;
        display: flex;
        align-items: center;
        padding: 12px 16px;
        transition: color 0.2s ease;
      }

ul li a:hover {
        color: #ffffff;
    }

span.red {
        color: var(--accent-delete);
        margin-right: 15px;
        font-size: 1.2rem;
        transition: transform 0.2s ease;
      }

ul li a:hover span.red {
        transform: scale(1.2);
      }

 .ital {
        text-align: center;
        font-size: 0.85rem;
        color: var(--text-muted);
        font-style: italic;
        margin-top: 25px;
      }

</style>
</head><body>
<div class="container">
      <h1>Delete files from /uploads/</h1>
<ul>

<?php 
while (($entry = readdir($dirHandler)) !== false)
{
    if (is_dir(sprintf("../%s/%s" , $targetFolderName , $entry)))
        continue; 

    printf("<li><a href=\"#\" data-target=\"%s\" onclick=\"confirmDelete('%s')\"><span class=\"red\">&#x2716</span> %s</a></li>", $entry , $entry, $entry);

}
?>

</ul>
</div>
<div class="container ital">
	list generated on <?php echo date("Y-m-d H:i:s") ?> by PHP
</div>
<div id="divOutput"></div>
<script src="deletefile.js" type="text/javascript"></script> 
</body></html>
