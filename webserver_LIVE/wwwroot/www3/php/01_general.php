<?php
session_start();

if(!empty($_GET['reset']))
{
    $_SESSION['counter'] = 0;
    header("Location: 01_general.php");
    exit; 
}
if(!empty($_GET['init_cookie']))
{
    SetCookie("cookie_counter", 1);
    header("Location: 01_general.php");
    exit; 
}
    

if(empty($_SESSION['counter']))
    $_SESSION['counter'] = 0;
$_SESSION['counter'] ++;


if(empty($_COOKIE['cookie_counter']))
{
    $cookieValue = "(none have been set yet)";
}
else
{
    $cookieValue = $_COOKIE['cookie_counter'];
    SetCookie("cookie_counter", $_COOKIE['cookie_counter'] + 1);


    
}


?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="style.css" />
    <title>PHP test</title>
</head>
<body>
    <div class="container" style="text-align:right">
    session counter = <?php echo $_SESSION['counter'];?>
    <button value="Reset Session" onclick="document.location.assign('01_general.php?reset=1')" >Reset Counter</button>
</div>
<div class="container" style="text-align:right">
    cookie counter = <?php echo $cookieValue;?>
    <button value="Reset Session" onclick="document.location.assign('01_general.php?init_cookie=1')" >Start / Reset counter cookie</button>
</div>

<div class="container">

    <h1>PHP Stuff</h1> 
    <form method="post" action="01_general.php?key=check" enctype="multipart/form-data">
        <div>name: <input type="text" name="name" value="<?php echo empty($_POST['name']) ? "Random Dude": $_POST['name'];?>"> </div>
        <div>age: <input type="number" name="age" value="<?php echo empty($_POST['age']) ? "42": $_POST['age'];?>"> </div>
        <div>file: <input type="file" name="file1"></div>
        
        <div class="">
            <input type="submit">
        </div>


    </form>
</div>
    <div class="container">    
<?php


if(count($_POST)>0)
{
    echo '<h2>$_GET</h2>'; 
    echo "<pre>";
    print_r($_GET);
    echo "</pre>";


    echo '<h2>$_POST</h2>'; 
    echo "<pre>";
    print_r($_POST);
    echo "</pre>";

    echo '<h2>$_FILES</h2>'; 
    echo "<pre>";
    print_r($_FILES);
    echo "</pre>";

    echo '<h2>$_COOKIE</h2>'; 
    echo "<pre>";    
    print_r($_COOKIE);
    echo "</pre>";

    echo '<h2>$_SESSION</h2>'; 
    echo "<pre>";
    print_r($_SESSION);
    echo "</pre>";

}
?>
</div>

</body>
</html>