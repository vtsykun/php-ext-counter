<?php

$res = counter_create(1863);
echo counter_value($res) . PHP_EOL;
counter_increment($res);
echo counter_value($res) . PHP_EOL;; 
counter_decrement($res);
