<?php
if (!isset($_GET['action']))
{
    echo "Error: No action";
    die();
}
$action = $_GET['action'];
if ($action == 'stop')
{
    echo "STOP\n";
    $f = fopen("files/STOP", "w");
    if (!$f)
    {
        echo "fopen failed: ";
    }
}
else if ($action == 'resume')
{
    echo "Resume";
    unlink("files/STOP");
}
else if ($action == 'powerup')
{
    echo "Go!";
    $f = fopen("files/POWERUP", "w");
}
?>

