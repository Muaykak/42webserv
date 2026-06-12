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
body { font-family: Arial, sans-serif; background-color: #121212; color: #f5f5f5; margin: 0; padding: 0; }
.container { max-width: 800px; margin: 2rem auto; padding: 1rem; background-color: #1e1e1e; border-radius: 8px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); }
h1 { font-size: 2rem; text-align: center; margin-bottom: 1rem; color: #f5f5f5; }
ul { list-style: none; padding: 0; }
ul li { padding: 0.5rem 1rem; border-bottom: 1px solid #333; transition: background-color 0.3s ease; }
ul li:last-child { border-bottom: none; }
ul li a { text-decoration: none; color: #4dabf7; font-weight: bold; transition: color 0.3s ease; }
ul li a:hover { color: #82caff; }
ul li:hover { background-color: #2a2a2a; }
span.red { color:#DD0000; }
.ital
{ text-align:right; font-size:0.8rem ; color: #999999; font-style:italic;}
</style>
</head><body>
<div class="container">
<h1 style="color:red">Delete files from /uploads/</h1>
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
<script type="text/javascript">
    const confirmDelete = async (target) => {

        if(confirm("Are you sure want to remove " + target + "?"))
        {
            try 
    		{
    			const response = await fetch('/uploads/' + target , {
    				method: 'DELETE',
    			})
    			if(response.status == 204)
    			{
    				document.getElementById("divOutput").innerHTML = "Deleted"
    				if(confirm("Deleted OK, reload the page?"))
    					location.reload();
    			}
    			else
    				document.getElementById("divOutput").innerHTML = text
    			
    		}
    		catch (error) {
                console.log(error.message)
    			alert("Fail, exception was thrown")
    		}
        }
    }
</script> 
</body></html>
