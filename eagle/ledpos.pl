#!/usr/bin/perl
use Math::Trig;

$c = 0.8;
$r = 0.25;			# Radius per LED
$a0 = 270;			# Initial angle

sub get_radius($$) {
    my($r,$n) = @_;
    my $th = deg2rad(360/$n);
    my $ph = deg2rad(90-180/$n);
    return $r*sin($ph)/sin($th);
}

$rx = get_radius($r,3);
for ($i = 0; $i < 3; $i++) {
    $thd = (360/3)*$i;
    $th = deg2rad($thd);
    $name = sprintf('LW%d', $i+1);
    printf "MOVE %s (%.3f %.3f);\n", $name, $c+$rx*cos($th), $c+$rx*sin($th);
    printf "ROTATE =SR%.3f '%s';\n", ($thd+$a0) % 360, $name;
}

$rx += 0.075;
for ($i = 0; $i < 3; $i++) {
    $thd = (360/3)*($i+0.5);
    $th = deg2rad($thd);
    $name = sprintf('LW%d', $i+4);
    printf "MOVE %s (%.3f %.3f);\n", $name, $c+$rx*cos($th), $c+$rx*sin($th);
    printf "ROTATE =SR%.3f '%s';\n", ($thd+$a0) % 360, $name;
}

$rx += 0.125;
for ($i = 0; $i < 6; $i++) {
    $thd = (360/6)*($i+0.5);
    $th = deg2rad($thd);
    $name = sprintf('LY%d', $i+1);
    printf "MOVE %s (%.3f %.3f);\n", $name, $c+$rx*cos($th), $c+$rx*sin($th);
    printf "ROTATE =SR%.3f '%s';\n", ($thd+$a0) % 360, $name;
}

$rx += 0.1;
for ($i = 0; $i < 6; $i++) {
    $thd = (360/6)*$i;
    $th = deg2rad($thd);
    $name = sprintf('LR%d', $i+1);
    printf "MOVE %s (%.3f %.3f);\n", $name, $c+$rx*cos($th), $c+$rx*sin($th);
    printf "ROTATE =SR%.3f '%s';\n", ($thd+$a0) % 360, $name;
}
