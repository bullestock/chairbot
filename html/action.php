<html>
<head>
<script type="text/javascript">
setTimeout(function() {
    window.location.href = "http://<?php echo $_SERVER['SERVER_ADDR']; ?>/index.html";
  }, 3000);
</script>
</head>
<body>
<?php
if (isset($_POST['stop']))
{
    echo "STOP!";
}
elseif (isset($_POST['powerup']))
{
    echo "Yay!";
}
else
{
    echo "Huh?";
}
?>
</body>
</html>
