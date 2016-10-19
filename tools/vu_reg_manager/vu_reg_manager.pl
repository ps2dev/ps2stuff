#! /usr/bin/perl -w

################################################################
### Copyright (c) 1999-2001, Sony Computer Entertainment America Inc.
### All rights reserved.
###
### A simple script to manage register use on the vus.
###
### Tyler Daniel
################################################################

#
# function prototypes
#

sub process_file;
sub process_line;
sub main;

sub printWarning;
sub printError;
sub printFreeStacks;
sub debugPrint;
sub getFile;
sub getDir;

sub handle_get;
sub handle_free;
sub handle_free_all;
sub handle_rename;
sub handle_set;
sub handle_init_free;
sub handle_init_free_all;
sub handle_include;
sub handle_status;
sub handle_replace_text;

#
# globals
#

# modify the string on the left to change the name of a directive
%DirectiveHandlers = ( "get" => \&handle_get,

		       "free" => \&handle_free,
		       "end" => \&handle_free,

		       "free_all" => \&handle_free_all,

		       "ren" => \&handle_rename,
		       "rename" => \&handle_rename,

		       "set" => \&handle_set,

		       "init" => \&handle_init_free,
		       "init_free" => \&handle_init_free,

		       "init_free_all" => \&handle_init_free_all,

		       "include" => \&handle_include,

		       "status" => \&handle_status,
		       "list" => \&handle_status,

		       "replace_text" => \&handle_replace_text
		       );

%VfNameAssignments = ( );
%ViNameAssignments = ( );

@VfFreeStack = ( );
@ViFreeStack = ( );

%FreeStacks = ( "vf" => \@VfFreeStack,
		"vi" => \@ViFreeStack );
%NameAssignments = ( "vf" => \%VfNameAssignments,
		     "vi" => \%ViNameAssignments );

@IDirectives = ( "DMAcnt", "DMAnext", "DMAref", "DMArefe", "DMArefs", "DMAcall", "DMAret", "DMAend",
		 "GIFpacked",
		 "BASE", "DIRECTHL", "DIRECT", "FLUSH", "FLUSHA", "FLUSHE", "ITOP", "MARK", "MPG", "MSCAL", "MSCALF",
		 "MSCNT", "MSKPATH3", "OFFSET", "STCOL", "STCYCL", "STMASK", "STMOD", "STROW", "UNPACK", "VIFNOP",
		 "dmacnt", "dmanext", "dmaref", "dmarefe", "dmarefs", "dmacall", "dmaret", "dmaend",
		 "gifpacked",
		 "base", "directhl", "direct", "flush", "flusha", "flushe", "itop", "mark", "mpg", "mscal", "mscalf",
		 "mscnt", "mskpath3", "offset", "stcol", "stcycl", "stmask", "stmod", "strow", "unpack", "vifnop" );

$CurLine = 1;
$CurFile = "";
$bDebug = 0;
$bDoInclude = 1;
$bAnalyze = 0;

$MaxViRegsUsed = 0;
$MaxVfRegsUsed = 0;

$TextToReplace = "";
$ReplacementText = "";

#
# code
#

# Start by calling main().	Who's a C programmer?
main;

sub main {
    my $arg;
    my $fileName = "";

    foreach $arg (@ARGV) {
	if ( $arg eq '-d' or $arg eq '--debug' ) { $bDebug = 1; }
	elsif ( $arg eq '-a' or $arg eq '--analyze' ) {
	    handle_init_free( "init_free", "vi", " vi01, vi02, vi03, vi04, vi05, vi06, vi07, vi08, vi09, vi10, vi11, vi12, vi13, vi14, vi15\n" );
	    handle_init_free( "init_free", "vf", " vf01, vf02, vf03, vf04, vf05, vf06, vf07, vf08, vf09, vf10, vf11, vf12, vf13, vf14, vf15,																vf16, vf17, vf18, vf19, vf20, vf21, vf22, vf23, vf24, vf25, vf26, vf27, vf28, vf29, vf30, vf31\n" );
	    $bAnalyze = 1;
	}
	elsif ( $arg eq '-n' or $arg eq '--no_includes' ) { $bDoInclude = 0; }
	else { $fileName = $arg; }
    }
    if ( !$fileName ) { $fileName = "-"; }

    $CurFile = $fileName;
    process_file $fileName;

    if ( $bAnalyze ) {
	print "Max vf registers used:\t $MaxVfRegsUsed\n";
	print "Max vi registers used:\t $MaxViRegsUsed\n";
    }
}

sub process_file {
    my $fileName = $_[0];
    local *fileHandle;

    open fileHandle, $fileName or die "Couldn't open file $fileName.\n";

    my $inCComment = 0;
    my $thisLine; my $nextLine;
    my $thisComments; my $nextComments;
    debugPrint "\t**** Processing file $fileName ****\n";
    if (defined(fileHandle) and $thisLine = <fileHandle>) {	 # defined is to get -w off my back
	# strip off any comments
	if ( $thisLine =~ s/ (;.*) //x ) { $thisComments = $1; }
	else { $thisComments = ""; }

	# start processing
	while ($thisLine) {
	    if ($nextLine = <fileHandle>) {
		# strip off any comments
		if ( $nextLine =~ s/ (;.*) //x ) { $nextComments = $1; }
		else { $nextComments = ""; }
		# check to see if lines need to be concatenated
		if ( $nextLine =~ s/ ^\+ //x ) {
		    chomp $thisLine;
		    chomp $nextLine;
		    $thisLine .= $nextLine . $thisComments . $nextComments . "\n";
		    # get the real next line and comments
		    $nextLine = <fileHandle>;
		    if ($nextLine && $nextLine =~ s/ (;.*) //x) { $nextComments = $1; }
		    else { $nextComments = ""; }
		}
		else {
		    chomp $thisLine;
		    $thisLine .= $thisComments . "\n";
		}
	    }
	    # we're at the end of the file
	    else {
		chomp $thisLine;
		$thisLine .= $thisComments . "\n";
	    }
	    # process this line
	    debugPrint "Line $CurLine input: $thisLine";
	    # strip out c comments.... I don't think they should be there, but what can you do?
	    # are we already in a multi-line c comment?
	    if ( $inCComment ) {
		# first try removing everything til the end of the comment
		if ( $thisLine =~ s/.*\*\/// ) {
		    $inCComment = 0;
		}
		else {
		    # there's no end on this line, so just nuke it
		    $thisLine = "";
		}
	    }
	    else {
		# first try stripping out any c comments that are completely on this line
		$thisLine =~ s/\/\*.*\*\///;
		# now try stripping out c comments that begin on this line
		if ( $thisLine =~ s/\/\*.*// ) {
		    $inCComment = 1;
		}
	    }
	    my $outputLine = process_line($thisLine);
	    debugPrint "Line $CurLine output: $outputLine\n";
	    # if we're doing an analysis, check the number of free regs
	    if ( $bAnalyze ) {
		if ( scalar(keys %VfNameAssignments) > $MaxVfRegsUsed ) {
		    $MaxVfRegsUsed = keys %VfNameAssignments;
		}
		if ( scalar(keys %ViNameAssignments) > $MaxViRegsUsed ) {
		    $MaxViRegsUsed = keys %ViNameAssignments;
		}
	    }
	    # send the line to stdout
	    if ( !$bAnalyze ) {
		print "$outputLine";
	    }
	    
	    $thisLine = $nextLine;
	    $thisComments = $nextComments;
	    $CurLine++;
	}
    }
}

sub process_line {
    my $thisLine = $_[0];
    my $instruction;
    my $args;
    my $outputLine;
    my $comments;

    # strip any whitespace off the end of the line
    $thisLine =~ s/\s*$//;

    # strip out any comments
    if ( $thisLine =~ s/ (;.*) //x ) { $comments = $1; }
    else { $comments = ""; }

    # skip the stupid "non-dot" asm directives (like DMAcnt, flush, etc..)
    foreach $idir (@IDirectives) {
	if ( $thisLine =~ m/ ^\s*$idir /x ) {
	    debugPrint "Found idirective $idir.\n";
	    return "$thisLine\n";
	}
    }

    # leave cpp # directives alone
    if ( $thisLine =~ m/ (\#.+) /x ) {
	$outputLine = "$thisLine\n";
    }
    # process any directives we know about and skip any others
    # first look for directives of the form .<directive>_<vf|vi>
    elsif ( $thisLine =~ m/ ^\s*\.(\S+)_(vf|vi)\s*(.*)\s*$ /xi ) {
	my $directive = $1; my $regType = $2; my $argList = $3;
	debugPrint "directive $directive, regType $regType\n";
	if (exists $DirectiveHandlers{$directive}) {
	    # can't get it to work without the extra "$handler"
	    my $handler = $DirectiveHandlers{$directive};
	    if ( &$handler($directive, $regType, $argList) ) { $outputLine = "\n"; }
	    else {
		debugPrint "Couldn't process directive; leaving it alone.\n";
		$outputLine = $thisLine;
	    }

	}
    }
    # now check for non-register-specific directives
    elsif ( $thisLine =~ m/ ^\s*\.(\S+)\s*(.*)\s*$ /xi ) {
	my $directive = $1;
	if (exists $DirectiveHandlers{$directive}) {
	    debugPrint "Processing directive $directive.\n";
	    # can't get it to work without the extra "$handler"
	    my $handler = $DirectiveHandlers{$directive};
	    if ( &$handler($thisLine) ) { $outputLine = "\n"; }
	    else {
		debugPrint "Couldn't process directive; leaving it alone.\n";
		$outputLine = $thisLine;
	    }
	}
	elsif ($directive) {
	    # it's a directive we don't know, so just leave this line alone
	    debugPrint "Skipping directive $directive.\n";
	    $outputLine = "$thisLine\n";
	}
    }
    # does it contain any cpp macros that look like function calls?
    elsif ( $thisLine =~ m/^\s*\w+ ?\(.*\)\s*$/ ) {
	$outputLine = "$thisLine\n"
	}
    # a line with a directive on it shouldn't contain instructions or labels, so skip the rest
    # skip lines that are just whitespace
    elsif ( !($thisLine =~ m/^\s$/) ) {
	$outputLine = "";

	# if we are replacing text because neither the cpp or gasp can be convinced to
	# do so, do it now
	if ( $TextToReplace ) {
	    $thisLine =~ s/$TextToReplace/$ReplacementText/;
	}
	$outputLine = $thisLine;
	# get any +s at the beginning of the line (continuation chars)
#	 if ( $thisLine =~ s/ ^\+ //x ) { $outputLine .= '+ '; }
#
#	 # if there's a label on this line, get it:
#	 if ( $thisLine =~ s/ \w+\: //x ) {
#	     my $label = $&;
#	     debugPrint "Lbl: $label ";
#	     $outputLine .= "$label ";
#	 }
#	 else { debugPrint "Lbl: <none> "; }
#
#	 # now get any instructions and arguments and filter the arguments
#	 while( $thisLine =~ / (?:\s*((?:nop|waitq|waitp)(?:\[[edtEDT]+\])?)) | (?:\s*(\S+)\s+ ([^\s,]+ (?:,\s*[^\s,]+)*)) /igx ) {
#	     # if there is an instruction left, get it:
#	     if ($1) { $instruction = $1; }
#	     elsif ($2) { $instruction = $2; }
#	     else { printError "I can't find any instructions!\n"; }
#	     if ($instruction) { debugPrint "Inst: $instruction "; } else { debugPrint "Inst: <none> "; }
#	     $outputLine .= "$instruction ";
#
#	     # if the instruction has arguments, process them:
#	     if ($args = ($3)) {
#		 my @arglist = split /\s*,\s*/, $args;
#		 for ( my $i = 0; $i < scalar(@arglist); $i++ ) {
#		     # get any .* masks
#		     my $mask;
#		     if ( $arglist[$i] =~ s/ \.\S+ //x ) { $mask = $&; } else { $mask = ""; }
#		     # get any [xyzw] broadcast modifiers
#		     my $bc;
#		     if ( $arglist[$i] =~ s/ \[([xyzw]+)\] //xi ) { $bc = $1; } else { $bc = ""; }
#		     # lookup the name
#		     if (exists $VfNameAssignments{$arglist[$i]}) {
#			 $arglist[$i] = $VfNameAssignments{$arglist[$i]};
#		     }
#		     elsif (exists $ViNameAssignments{$arglist[$i]}) { 
#			 $arglist[$i] = $ViNameAssignments{$arglist[$i]};
#		     }
#		     # if it wasn't found above, it could still be a load instruction with the reg in parens
#		     else {
#			 while ( $arglist[$i] =~ m/ \(\s*(\S+)\s*\) /xg ) {
#			     my $reg = $1;
#			     # strip off any increments or decrements
#			     $reg =~ s/ ((?:\+\+) | (?:\-\-)) (\w+) /$2/x;
#			     $reg =~ s/ (\S+)((?:\+\+) | (?:\-\-)) /$1/x;
#
#			     if (exists $ViNameAssignments{$reg}) {
#				 $arglist[$i] =~ s/\($reg/\($ViNameAssignments{$reg}/;
#				 $arglist[$i] =~ s/$reg\)/$ViNameAssignments{$reg}\)/;
#			     }
#			 }
#		     }
#		     debugPrint "Arg: $arglist[$i]/$bc/$mask ";
#		     $arglist[$i] .= $bc . $mask;
#		 }
#		 $args = join ', ', @arglist;
#		 $outputLine .= "$args ";
#	     }
#	     else { debugPrint "Args: <none> "; }
#	 }
	$outputLine .= "$comments\n";
	debugPrint "Cmts: $comments\n";
    }
    # the line is just whitespace, keep it so the error line #'s match up
    else { $outputLine = $thisLine; }

    return $outputLine;
}

sub printError {
    print STDERR "Error!  $CurFile, line $CurLine:	$_[0]";
}

sub printWarning {
    print STDERR "Warning! $CurFile, line $CurLine:	 $_[0]";
}

sub printFreeStacks {
    my $el;

    debugPrint "Free vf registers:	";
    foreach $el (@VfFreeStack) {
	debugPrint "$el ";
    }
    debugPrint "\nFree vi registers:  ";
    foreach $el (@ViFreeStack) {
	debugPrint "$el ";
    }
    debugPrint "\n";
}

sub debugPrint {
    if ($bDebug) { print STDERR $_[0]; }
}

sub getFile {
    my $path = $_[0];
    my $filename;

    if ( $path =~ m/(.+\/)([^\/]+)/ ) {
	$filename = $2;
    }
    else {
	$filename = $path;
    }

    return $filename;
}

sub getDir {
    my $path = $_[0];
    my $baseDir = "";

    if ( $path =~ m/(.*\/)([^\/]+)/ ) {
	$baseDir = $1;
    }

    return $baseDir;
}

#
# directive handlers
#

sub handle_include {
    my $thisLine = $_[0];

    if ( $bDoInclude ) {
	$thisLine =~ / ^\s*\.(\S+)\s+(\S*) /x;
	my $directive = $1; my $args = $2;

	$args =~ s/"(\S+)"/$1/;

	# change filename.vcl to filename_vcl.vsm (the generated file)
	$args =~ s/\.vcl/_vcl\.vsm/;

	# if the file isn't in the current dir, try the dir of the file
	# currently being processed
	my $newfile;
	if (-e $args) {
	    $newfile = $args;
	}
	else {
	    if (-e (getDir $CurFile) . $args) {
		$newfile = (getDir $CurFile) . $args;
	    }
	    else {
		printError "Couldn't find file $args to include!";
		die;
	    }
	}

	debugPrint "Including file $newfile\n";
	# oooh, ugly hack!
	my $curLineSave = $CurLine;
	my $curFileSave = $CurFile;
	$CurLine = 1;
	$CurFile = $newfile;
	&process_file( $newfile );
	$CurLine = $curLineSave;
	$CurFile = $curFileSave;

	return 1;
    }
    else { return 0; }
}

sub handle_set {
    my ($directive, $regType, $args) = @_;

    my $nameAssignmentsRef = $NameAssignments{$regType};
    my $freeStackRef = $FreeStacks{$regType};
    if ( $args =~ m/ \s*(\S+)\s*,\s* (\S+) /x ) {
	my $name = $1; my $reg = $2;
	if (exists $$nameAssignmentsRef{$name}) {
	    printError "$name already exists!\n";
	}
	else {
	    # remove from the free stack if present
	    my $freereg; my $i;
	    my $freeStackLen = scalar(@$freeStackRef);
	    for ( $i = 0; $i < $freeStackLen; $i++ ) {
		$freereg = shift @$freeStackRef;
		if ( $freereg eq $reg ) { debugPrint "Removing $reg from free stack.\n"; }
		else { push @$freeStackRef, $freereg; }
	    }
	    printFreeStacks;
	    # check to see if the reg is assigned to something else
	    my $key;
	    foreach $key (keys %$nameAssigmentsRef) {
		if ( $$nameAssigmentsRef{$key} eq $reg ) {
		    printError "$reg is already assigned to $$nameAssigmentsRef{$key}!\n";
		    $reg = '<error>';
		    last;
		}
	    }

	    debugPrint "Assigning $reg to $name.\n";
	    # ${$NameAssignments{$regType}}{$name} = $reg;
	    $$nameAssignmentsRef{$name} = $reg;
	}
    }
    else {
	printError "Syntax error in $directive.\n";
    }

    return 1;
}

sub handle_get {
    my ($directive, $regType, $args) = @_;

    my @arglist = split /\s*,\s*/, $args;
    my $newName; my @newArgs;
    # translate names for arrays of registers
    foreach $newName (@arglist) {
	if ( $newName =~ m/ (\S+)\[([0-9])\] /x ) {
	    debugPrint( "Creating array of $regType regs \($1\[0\-$2\]\)\n" );
	    my $j;
	    for ( $j = 0; $j < $2; $j++ ) {
		push @newArgs, ("$1\[$j\]");
	    }
	}
	else { push @newArgs, ($newName); }
    }
    my $freeStackRef = $FreeStacks{$regType};
    my $nameAssignmentsRef = $NameAssignments{$regType};
    # allocate each name
    foreach $newName (@newArgs) {
	if ( (exists $VfNameAssignments{$newName}) ||
	     (exists $ViNameAssignments{$newName}) ) {
	    printError "$newName already exists!\n";
	}
	else {
	    my $nextReg = shift @$freeStackRef;
	    if ($nextReg) {
		$$nameAssignmentsRef{$newName} = $nextReg;
		debugPrint "$$nameAssignmentsRef{$newName} assigned to $newName\n";
		printFreeStacks;
	    }
	    else { printError "No more free registers for $newName\n"; }
	}
    }

    return 1;
}

sub handle_free{
    my ($directive, $regType, $args) = @_;

    my $freeName;
    my @arglist = split /\s*,\s*/, $args;
    # translate names for arrays of registers
    my @newArgs;
    foreach $freeName (@arglist) {
	if ( $freeName =~ m/ (\S+)\[([0-9])\] /x ) {
	    debugPrint( "Freeing array of $regType regs \($1\[0\-$2\]\)\n" );
	    my $j;
	    for ( $j = 0; $j < $2; $j++ ) {
		push @newArgs, ("$1\[$j\]");
	    }
	}
	else { push @newArgs, ($freeName); }
    }
    my $nameAssignmentsRef = $NameAssignments{$regType};
    my $freeStackRef = $FreeStacks{$regType};
    # free each name
    foreach $freeName (@newArgs) {
	if (exists $$nameAssignmentsRef{$freeName}) {
	    debugPrint "Releasing $freeName\n";
	    unshift @$freeStackRef, delete $$nameAssignmentsRef{$freeName};
	    printFreeStacks;
	}
	else {
	    printWarning "I can't free $freeName if it isn't allocated!\n";
	}
    }

    return 1;
}

sub handle_free_all {
    my ($directive, $regType, $args) = @_;

    my $nameAssignmentsRef = $NameAssignments{$regType};
    my $freeStackRef = $FreeStacks{$regType};

    my $name;
    foreach $name (keys %$nameAssignmentsRef) {
	unshift @$freeStackRef, (delete $$nameAssignmentsRef{$name});
    }
    printFreeStacks;

    return 1;
}

sub handle_rename{
    my ($directive, $regType, $args) = @_;

    my $nameAssignmentsRef = $NameAssignments{$regType};
    if ( $args =~ m/ \s*(\S+)\s*,\s* (\S+) /x ) {
	my $oldName = $1; my $newName = $2;
	# if (exists ${$NameAssignments{$regType}}{$oldName}) {
	if (exists $$nameAssignmentsRef{$oldName}) {
	    if (exists $$nameAssigmentsRef{$newName}) {
		printError "$newName already exists!\n";
	    }
	    else {
		debugPrint "renaming $oldName to $newName\n";
		$$nameAssignmentsRef{$newName} = delete $$nameAssignmentsRef{$oldName};
	    }
	}
	else {
	    printError "Can't rename $oldName -- it doesn't exist!\n";
	}
    }
    else {
	printError "Syntax error in $directive.\n";
    }

    return 1;
}

sub handle_init_free{
    my ($directive, $regType, $args) = @_;

    if ( !$bAnalyze ) {
	debugPrint "regType = $regType\n";
	my $freeStackRef = $FreeStacks{$regType};
	
	# clear the stack of free registers
	while ( scalar(@$freeStackRef) ) { shift @$freeStackRef; }
	# strip any whitespace off the end of the line
	$args =~ s/\s*$//;
	$args =~ s/^\s*//;
	# add registers
	my @arglist = split /\s*,\s*/, $args;
	push @$freeStackRef, @arglist;

	debugPrint "Initting $regType free stack::\n";
	printFreeStacks;
    }

    return 1;
}

sub handle_init_free_all {
    my ($directive, $regType, $args) = @_;

    if ( $regType eq "vi" ) {
	handle_init_free( "init_free", "vi", "vi01, vi02, vi03, vi04, vi05, vi06, vi07, vi08, vi09, vi10, vi11, vi12, vi13, vi14, vi15\n" );
    }
    elsif ( $regType eq "vf" ) {
	handle_init_free( "init_free", "vf", "vf01, vf02, vf03, vf04, vf05, vf06, vf07, vf08, vf09, vf10, vf11, vf12, vf13, vf14, vf15,																   vf16, vf17, vf18, vf19, vf20, vf21, vf22, vf23, vf24, vf25, vf26, vf27, vf28, vf29, vf30, vf31\n" );
    }
    else {
	printError "Unknown register type!\n";
    }
}

sub handle_status {
    my ($directive, $regType, $args) = @_;

    my $nameAssignmentsRef = $NameAssignments{$regType};
    my $freeStackRef = $FreeStacks{$regType};

    my @names = keys( %{$NameAssignments{$regType}} );
    my $i = scalar(@names);
    
    print STDERR "Status at $CurFile line $CurLine:	 $i $regType registers allocated:\n";
    for ( $i = 0; $i < scalar(@names); $i++ ) {
	print STDERR "$names[$i] = \t $$nameAssignmentsRef{$names[$i]}\n";
    }
    print STDERR "Free $regType registers:\n";
    foreach $i (@$freeStackRef) { print STDERR "$i "; };
    print STDERR "\n\n";

    return 1;
}

sub handle_replace_text {
    my $thisLine = $_[0];

    $thisLine =~ / ^\s*\.(\S+)\s+(\S+),\s*(\S+)\s*$/x;
    my $directive = $1; my $text = $2; my $repl_text = $3;

    $TextToReplace = $text;
    $ReplacementText = $repl_text;

    debugPrint "Replacing $text with $repl_text\n";

    return 1;
}
