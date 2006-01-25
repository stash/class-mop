#!/usr/bin/perl

use strict;
use warnings;

use Test::More no_plan => 1;
use Test::Exception;

BEGIN {
    use_ok('Class::MOP::Class');        
}


my $meta = Class::MOP::Class->initialize('Class::MOP::Class');
isa_ok($meta, 'Class::MOP::Class');

foreach my $method_name (qw(
    initialize create
    name version
    superclasses class_precedence_list
    has_method get_method add_method remove_method 
    get_method_list compute_all_applicable_methods find_all_methods_by_name
    )) {
    ok($meta->has_method($method_name), '... Class::MOP::Class->has_method(' . $method_name . ')');
    {
        no strict 'refs';
        is($meta->get_method($method_name), 
           \&{'Class::MOP::Class::' . $method_name},
           '... Class::MOP::Class->get_method(' . $method_name . ') == &Class::MOP::Class::' . $method_name);        
    }
}

is($meta->name, 'Class::MOP::Class', '... Class::MOP::Class->name');
is($meta->version, $Class::MOP::Class::VERSION, '... Class::MOP::Class->version');

is_deeply(
    [ $meta->superclasses ], 
    [], 
    '... Class::MOP::Class->superclasses == []');
    
is_deeply(
    [ $meta->class_precedence_list ], 
    [ 'Class::MOP::Class' ], 
    '... Class::MOP::Class->class_precedence_list == []');

