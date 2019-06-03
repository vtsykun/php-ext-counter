--TEST--
Check for counterlock presence
--SKIPIF--
<?php if (!function_exists("counter_create")) print "skip"; ?>
--FILE--
<?php 
$res = counter_create(1863);
echo counter_value($res) . PHP_EOL;
counter_increment($res);
echo counter_value($res) . PHP_EOL;
counter_increment($res);
echo counter_value($res) . PHP_EOL;
counter_decrement($res);
echo counter_value($res) . PHP_EOL;
counter_remove($res);

/*
	you can add regression tests for your extension here

  the output of your test code has to be equal to the
  text in the --EXPECT-- section below for the tests
  to pass, differences between the output and the
  expected text are interpreted as failure

	see php7/README.TESTING for further information on
  writing regression tests
*/
?>
--EXPECT--
0
1
2
1
