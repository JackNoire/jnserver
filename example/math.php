<html>
<form method="get">
输入一个数：<input type="text" name="num"><br><br>
<input type="submit" value="计算"><br>
</form>

<?php
if (isset($_GET['num'])) {
    $num = (integer)$_GET['num'];
    if ($num >= 2 && $num < 500000) {
        echo $num;
        echo " = ";
        for ($i = 2; $i < (integer)$_GET['num'] && $num !== 1; $i++) {
            while ($num % $i === 0) {
                echo $i;
                if ($num !== $i) {
                    echo " × ";
                }
                $num /= $i;
            }
        }
    }
}

echo "<br>";
var_dump($_GET);
?>

</html>