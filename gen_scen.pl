#!/usr/bin/perl -w

my $num_nodes = 0;
my $one_day = 60*60*24; # Seconds in a day
my $end_time = 60*60; # one hour
my $max_speed = 10; # m/s

my $grid_x = 1000; #
my $grid_y = 500; #
my $time = 0;
my $num_traffic_sources = 20;
my $max_packets_per_hour = 100;
my $random_mobility = 0;

my @nodes = ();
my @nodes_traffic = ();
my %packets = ();



while (@ARGV) {
    if ($ARGV[0] eq "-n" || $ARGV[0] eq "--nodes") {
	defined($ARGV[1]) || die "Missing argument\n";
	$num_nodes = $ARGV[1];
	shift;
    } elsif ($ARGV[0] eq "-g" || $ARGV[0] eq "--grid") {
	defined($ARGV[1]) || die "Missing argument\n";

	$ARGV[1] =~ /(\d+)x(\d+)/;

	(defined($1) && defined($2)) || die "Grid argument should be <x-size>x<y-size>\n";
	$grid_x = $1;
	$grid_y = $2;
	shift;
    } elsif ($ARGV[0] eq "-s" || $ARGV[0] eq "--speed") {
	defined($ARGV[1]) || die "Missing argument\n";
	$max_speed = $ARGV[1];
	shift;
    } elsif ($ARGV[0] eq "-t" || $ARGV[0] eq "--time") {
	defined($ARGV[1]) || die "Missing argument\n";
	$end_time = $ARGV[1];
	shift;
    } elsif ($ARGV[0] eq "-ts" || $ARGV[0] eq "--traffic-sources") {
	defined($ARGV[1]) || die "Missing argument\n";
	$num_traffic_sources = $ARGV[1];
	shift;
    } elsif ($ARGV[0] eq "-tr" || $ARGV[0] eq "--traffic-rate") {
	defined($ARGV[1]) || die "Missing argument\n";
	$max_packets_per_hour = $ARGV[1];
	shift;
    }  elsif ($ARGV[0] eq "-rm" || $ARGV[0] eq "--random-mobility") {
	$random_mobility = 1;
    } elsif ($ARGV[0] eq "-h" || $ARGV[0] eq "--help") {
	print_help();
	exit;
    } 
    shift;
}

sub print_help {
    print "Usage: $0 [ OPTIONS [ ARGUMENT ] ] [ < CONTACTS_FILE ]\n";
    print "Where OPTIONS are:\n";
    print "\t-n, --nodes\t\t set the number of nodes.\n";
    print "\t-g, --grid\t\t set the grid size (format \'<x-size>x<y-size>\').\n";
    print "\t-s, --speed\t\t set the MAX speed of nodes.\n";
    print "\t-t, --time\t\t set the end time of the simulation in seconds.\n";
    print "\t-ts, --traffic-sources\t set number of traffic sources.\n";
    print "\t-tr, --traffic-rate\t set the traffic rate (pkts/hour).\n";
    print "\t-rm, --random-mobility\t generate scenario with random mobility.\n";
    print "\t-h, --help\t\t show help (this information).\n";
    print "\n";
    print "CONTACTS_FILE is a trace file with node contacts in the format:\n";
    print "<recording node#> <seen node#> <contact start time> <contact end time>\n";
    print "\n";       
}

sub distance {
    my ($x1, $y1, $x2, $y2) = @_;

    my $dx = $x1 - $x2;
    my $dy = $y1 - $y2;

    return sqrt($dx*$dx + $dy*$dy)
}

if ($random_mobility) {
    goto print_header;
}
# We read these from the trace file instead:
undef $end_time;
undef $num_nodes;
undef $start_time;

my $tmp_file = "/tmp/gen_scen.dat";

open TMP, ">$tmp_file";

while(<STDIN>) {
    my @line = split();
    
    if ($line[2] > 99) {
	next;
    }
    if (!defined($start_time) || $line[0] < $start_time) {
	$start_time = $line[0];
    }     
    if (!defined($num_nodes) || $num_nodes < $line[1]) {
	$num_nodes = $line[1];
    } 
    if (!defined($end_time) || $end_time < $line[3]) {
	$end_time = $line[3];
    } 
    
    print TMP "$line[1] $line[2] $line[0] $line[3]\n";
}

$end_time -= $start_time;

close TMP;

print_header:

if ($num_nodes < 2) {
    die "Number of nodes not specified or too small!\n";
}

# Print the header that initializes the simulator.
print "# num_nodes=$num_nodes\n";
print "# traffic sources=$num_traffic_sources\n";
print "# traffic rate=$max_packets_per_hour\n";
print "# max speed=$max_speed\n";
print "# x=$grid_x y=$grid_y\n";

# These are read by the simulator
print "num_nodes=$num_nodes\n";
print "end_time=$end_time\n";

for (my $i = 0; $i < $num_traffic_sources; $i++) {
    $nodes_traffic[$i]{'id'} = int(rand($num_nodes));
    $nodes_traffic[$i]{'pkts'} = 0;
} 

if (!$random_mobility) {
    print "# Contacts; c <node_rec> <node_seen> <start time> <end time>\n";
    
    open TMP, "<$tmp_file";
    
    while(<TMP>) {
	my @line = split();

	printf("c %d %d %f %f\n", $line[0], $line[1], $line[2] - $start_time, $line[3] - $start_time);	
	#printf("c %d %d %f %f\n", $line[1], $line[2], $line[2] - $start_time, $line[3] - $start_time);
    }
    close TMP;
    goto traffic;
}

print "# Set initial node pos; p <node> <x-pos> <y-pos>\n";

for (my $i = 0; $i < $num_nodes; $i++) {
    my $x = rand($grid_x);
    my $y = rand($grid_y);
    print"p $i $x $y\n";
    $nodes[$i]{'id'} = $i;
    $nodes[$i]{'pos_x'} = $x;
    $nodes[$i]{'pos_y'} = $y;
    $nodes[$i]->{'time'} = 0.0;
} 

print "# Mobility; m <time> <node> <x-dest> <y-dest> <speed>\n";

while ($time < $end_time) {
    
    my $new_time = 0;

    for (my $i = 0; $i < $num_nodes; $i++) {
	
	
	if ($nodes[$i]->{'time'} <= $time) {
	    my $x = rand($grid_x);
	    my $y = rand($grid_y);
	    my $speed = rand($max_speed);
	    
	    my $d = distance($nodes[$i]->{'pos_x'}, $nodes[$i]->{'pos_y'}, $x, $y);
	    
	    $nodes[$i]->{'time'} = $time + ($d / $speed);
	    
	    print "m $time $i $x $y $speed\n";
	}
	
	if ($i == 0) {
	    $new_time = $nodes[$i]->{'time'};
	   # print "next time is $new_time\n";
	} elsif ($nodes[$i]->{'time'} < $new_time) {
	    $new_time = $nodes[$i]->{'time'};
	   # print "next time is $new_time\n";
	} 
    }
    $time = $new_time;
}

traffic:

foreach my $node (@nodes_traffic) {
    
    while ($node->{'pkts'} < $max_packets_per_hour) {
	my $dst = $node->{'id'};
	
	while ($dst == $node->{'id'}) {
	    $dst = int(rand($num_nodes));
	}
	
	my $time = rand($end_time);
	$node->{'pkts'}++;
	
	$packets{$time} = { 'src' => $node->{'id'}, 'dst' => $dst, 'time' => $time }
    }
}

print "# Traffic; t <time> <type> <src> <dst> <size>\n";

foreach my $time (sort {$a <=> $b} keys (%packets)) {
    printf("t %f e %d %d 1500\n", $time, $packets{$time}->{'src'}, $packets{$time}->{'dst'});

}
