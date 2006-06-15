#!/usr/bin/perl -w
#
# postanalyze gnome-vfs HTTP debug logs
#
# mfleming 11/17/00
#
# To use, enable the ANALYZE_HTTP messages in http-method.c
#
#
# Format of the message:
#
# HTTP: [0xa2740685] [0x0002bc03] ==> +create_handle
#	  ^^^^ LSW of gettimeofday     
#			^^^^  threadid
#                                    ^^^ + for entering a context - for leaving                              
while (<>) {

	chomp;
	
	next if ! /^HTTP: \[0x([0-9a-fA-F]+)\] \[0x([0-9a-fA-F]+)\](.*)$/;

	$timestamp = hex ($1);
	$threadid = hex ($2);
	$message = $3;

	if (!exists($messages{$threadid})) {
		if (!defined($basetime)) {
			$basetime = $timestamp;
		}

		$messages{$threadid} = "";
		$times{$threadid} = $timestamp;
		$context{$threadid} = [];
	} 


	$timediff = ($timestamp - $times{$threadid})/1000;
	$times{$threadid} = $timestamp;


	if ($message=~/==> \+(.*)/) {
		push @{$context{$threadid}}, [$timestamp, $1];

		$messages{$threadid} .= sprintf "[%08lu] (d%08lu ms)%s%s\n", $timestamp - $basetime, $timediff, "\t"x(@{$context{$threadid}} - 1) ,  $message;

	} elsif ($message=~/==> \-(.*)/) {
		$method = $1;
		
		do {
			$cur_context = pop @{$context{$threadid}};
		} until ( !defined($cur_context) || ${$cur_context}[1] eq $method);

		$messages{$threadid} .= sprintf "[%08lu] (d%08lu ms)%s%s (d%08lu ms)\n", $timestamp - $basetime, $timediff, "\t"x(@{$context{$threadid}}),  $message, ($timestamp - ${$cur_context}[0])/1000;

		if (exists($methods{$method})) {
			push @{$methods{$method}}, ($timestamp - ${$cur_context}[0]);
		} else {
			$methods{$method} = [($timestamp - ${$cur_context}[0])];
		}

	} else {
		$messages{$threadid} .= sprintf "[%08lu] (d%08lu ms)%s%s\n", $timestamp - $basetime, $timediff, "\t"x(@{$context{$threadid}}) ,  $message;
	}
}

@methods_list=[];

foreach $method (keys (%methods)) {
	@sorted = sort {$a <=> $b} @{$methods{$method}};

	push @methods_list, [$sorted[0], $sorted[scalar(@sorted)-1], &avg(@sorted), scalar(@sorted), $method]; 
}

print "   Min    Max    Avg    Num\n";
foreach $method ( reverse sort { ${$a}[2] <=> ${$b}[2] } @methods_list) {
	printf "%6u %6u %6u %6u : %s\n", ${$method}[0]/1000, ${$method}[1]/1000, ${$method}[2]/1000,${$method}[3], ${$method}[4];
}

foreach $thread (keys (%messages)) {
	$thread_string = sprintf "0x%08x", $thread; 
	print "="x10 . " $thread_string " . "="x10 . "\n";
	print $messages{$thread};
}

sub avg {
	my ($sum) = 0;
	my ($num);

	foreach $num (@_) {
		$sum += $num;
	}

	return scalar(@_) ?  $sum / @_ : 0 ;
}
