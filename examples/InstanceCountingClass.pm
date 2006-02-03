
package InstanceCountingClass;

use strict;
use warnings;

use Class::MOP 'meta';

our $VERSION = '0.01';

use base 'Class::MOP::Class';

__PACKAGE__->meta->add_attribute(
    Class::MOP::Attribute->new('$:count' => (
        reader  => 'get_count',
        default => 0
    ))
);

sub construct_instance {
    my ($class, %params) = @_;
    $class->{'$:count'}++;
    return $class->SUPER::construct_instance();
}

1;

__END__

=pod

=head1 NAME 

InstanceCountingClass - An example metaclass which counts instances

=head1 SYNOPSIS

  package Foo;
  
  sub meta { InstanceCountingClass->initialize($_[0]) }
  sub new  {
      my $class = shift;
      bless $class->meta->construct_instance() => $class;
  }

  # ... meanwhile, somewhere in the code

  my $foo = Foo->new();
  print Foo->meta->get_count(); # prints 1
  
  my $foo2 = Foo->new();
  print Foo->meta->get_count(); # prints 2  
  
  # ... etc etc etc

=head1 DESCRIPTION

This is a classic example of a metaclass which keeps a count of each 
instance which is created. 

=head1 AUTHOR

Stevan Little E<lt>stevan@iinteractive.comE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright 2006 by Infinity Interactive, Inc.

L<http://www.iinteractive.com>

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself. 

=cut