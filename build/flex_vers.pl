#
# usage:
#   flex_version.pl <flex-name> <flex-desired-version> <flex-v-output>
#

use strict;

sub usage()
{
	warn "$0 <flex-name> <flex-desired-version> <flex-v-output>\n";
	exit 1;
}

sub get_vers($)
{
	my ($in) = @_;
	return split /\./, $in  ;
}

#
# Check whether tuple 1 is greater than/equal to tuple 2
#
sub vers_ge ($$)
{
	my ($aa, $bb) = @_;
	my @a = @$aa;
	my @b = @$bb;
	while ($#a >= 0 || $#b >= 0) {
		return 0 if ($#a < 0 && $#b >= 0);
		return 1 if ($#a >= 0 && $#b < 0);
		return 1 if ($a[0] > $b[0]);
		return 0 if ($a[0] < $b[0]);
		shift @a;
		shift @b;
	}
	return 1;
}


usage() unless $#ARGV == 2;
my $flex_name = $ARGV[0];
my $flex_desired_version = $ARGV[1];
my $flex_v_output = $ARGV[2];

exit 2 unless ($flex_v_output =~ /^${flex_name}.*?((\d+)(\.(\d+))*)$/);
my @actual = get_vers ($1);
my @desired = get_vers ($flex_desired_version);

exit 0 if vers_ge (\@actual, \@desired);

exit 1;
