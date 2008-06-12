#!/usr/bin/env perl
use strict;
use warnings;
use Test::More tests => 11;
use Test::Exception;

require Class::MOP;
use lib 't/lib';

ok(!Class::MOP::is_class_loaded(), "is_class_loaded with no argument returns false");
ok(!Class::MOP::is_class_loaded(''), "can't load the empty class");
ok(!Class::MOP::is_class_loaded(\"foo"), "can't load a class name reference??");

throws_ok { Class::MOP::load_class()       } qr/Invalid class name \(undef\)/;
throws_ok { Class::MOP::load_class('')     } qr/Invalid class name \(\)/;
throws_ok { Class::MOP::load_class(\"foo") } qr/Invalid class name \(SCALAR\(\w+\)\)/;

ok(Class::MOP::load_class('BinaryTree'));
can_ok('BinaryTree' => 'traverse');

do {
    package Class;
    sub method {}
};

ok(Class::MOP::load_class('Class'), "this should not die!");

throws_ok {
    Class::MOP::load_class('FakeClassOhNo');
} qr/Can't locate /;

throws_ok {
    Class::MOP::load_class('SyntaxError');
} qr/Missing right curly/;
