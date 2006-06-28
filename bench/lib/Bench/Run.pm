#!/usr/bin/perl

package Bench::Run;
use Moose;

use Benchmark qw/:hireswallclock :all/;

has classes => (
    isa => "ArrayRef",
    is  => "rw",
    auto_deref => 1,
);

has benchmarks => (
    isa => "ArrayRef",
    is  => "rw",
    auto_deref => 1,
);

has min_time => (
    isa => "Num",
    is  => "rw",
    default => 5,
);

sub run {
    my $self = shift;

    foreach my $bench ( $self->benchmarks ) {
        my $bench_class = $bench->{class};
        my @bench_args  = ( (ref($bench->{args}) eq "ARRAY") ? @{ $bench->{args} } : %{ $bench->{args} } );

        eval "require $bench_class";
        die $@ if $@;

        my %res;

        foreach my $class ( $self->classes ) {
            eval "require $class";
            die $@ if $@;

            my $b = $bench_class->new( @bench_args, class => $class );
            $res{$class} = countit( $self->min_time, $b->code );
        }

        print "$bench_class:\n";
        cmpthese( \%res );
        print "\n";
    }
}

__PACKAGE__;

__END__
