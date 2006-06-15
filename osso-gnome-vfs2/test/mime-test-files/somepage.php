<?php 
echo "<html>\n<body>\n";

$a = array(
     "apple"  => array(
          "color"  => "red",
          "taste"  => "sweet",
          "shape"  => "round"
     ),
     "orange"  => array(
          "color"  => "orange",
          "taste"  => "tart",
          "shape"  => "round"
     ),
     "banana"  => array(
          "color"  => "yellow",
          "taste"  => "paste-y",
          "shape"  => "banana-shaped"
     )
);

echo $a[apple][taste] . "<br>\n";
echo $a["apple"]["shape"] . "\n";

echo "</body>\n</html>\n";

?>