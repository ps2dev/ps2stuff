#! /usr/bin/perl -w

#
# globals
#

%Dependencies = ( );
$CurTopFile = "";

%Includes = ( );
$CurIncludeVar = "";

@IncludePaths = ( "" );

$bMutateVclFilenames = 1;

#
# prototypes
#

sub addDep;
sub addInclude;
sub getVSMDepsInFile;
sub getCPPDepsInFile;
sub getFile;
sub getDir;

#
# begin code
#

$cppPath = "";

# parse and remove any -I directives
$cppincludes = 1;
for ( my $i = 0; $i < scalar(@ARGV); $i++ ) {
    $remove = 0;
#    print STDERR "adding $ARGV[$i]\n";
    # is this a cpp search path (-I<dir>)?
    if ( $ARGV[$i] =~ m/^-I(.+)/ ) {
	if ( $cppincludes ) {
#	    print STDERR "adding $1 to cppPath\n";
	    $cppPath = $cppPath . "-I$1" . " ";
	}
	else {
#	    print STDERR "adding $1 to IncludePaths\n";
	    push @IncludePaths, "$1/";
	}
	$remove = 1;
    }
    # is this "-endcppincludes"?
    if ( $ARGV[$i] =~ m/\-endcppincludes/ ) {
#	print STDERR "got endcppincludes\n";
	$cppincludes = 0;
	$remove = 1;
    }

    # remove this from the command line?
    if ( $remove ) {
        # remove this from the array
        splice @ARGV, $i, 1;
        # adjust the index since we've removed one
        $i--;
    }
}

# print STDERR "cpp include path:\n";
# print STDERR "$cppPath\n";
# 
# print STDERR "include path:\n";
# foreach $path (@IncludePaths) {
#     print STDERR "$path\n";
# }
 
# get dependencies for each file left on the command line
foreach $arg (@ARGV) {
    $target = $arg;
    $target =~ s/\.vsm/\.vo/;
    $target =~ s/\.dsm/\.do/;
    $target =~ s/\.vcl/\.vl/;
    $CurTopFile = $target;

    # first get vsm .included dependencies
    getVSMDepsInFile $arg;

    # now get cpp #included dependencies
    $CurIncludeVar = getStem($arg)."_INCLUDES";
    getCPPDepsInFile $arg;
}

# print the dependency rule(s)
foreach $target (keys %Dependencies) {
    $target_file = getFile $target;
    print "$target_file: ";
    foreach $i (0 .. $#{ $Dependencies{$target} }) {
	if ( ($i+1) % 8 == 0 ) {
	    print "\\\n\t";
	}

	$dep = $Dependencies{$target}[$i];
	print "$dep ";
    }
    print "\n";
}

# print the #included files in a variable for the makefile
foreach $var (keys %Includes) {
    print "$var=";
    foreach $i (0 .. $#{ $Includes{$var} }) {
	if ( ($i+1) % 8 == 0 ) {
	    print "\\\n\t";
	}

	$file = $Includes{$var}[$i];
	print "$file ";
    }
    print "\n";
}

#
# subroutines
#

sub addDep {
    push @{ $Dependencies{$CurTopFile} }, ($_[0]);
}

sub addInclude {
    push @{ $Includes{$CurIncludeVar} }, ($_[0]);
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

    #print "path = $path\n";
    #print "file = $filename\n";

    return $filename;
}

sub getStem {
    my $path = $_[0];
    my $filename = getFile($path);
    $filename =~ s/(.+)\..+/$1/;
    return $filename;
}

sub getDir {
    my $path = $_[0];
    my $baseDir = "";

    if ( $path =~ m/(.*\/)([^\/]+)/ ) {
	$baseDir = $1;
    }

    # print "path = $path\n";
    # print "dir = $baseDir\n";

    return $baseDir;
}


sub getVSMDepsInFile {
    my $fileName = $_[0];
    local *fileHandle;

    # add this file as a dependency

    my $depName = $fileName;
    # if this is a vcl file, the object actually depends on the generated vsm file
    if ( $bMutateVclFilenames && $fileName =~ /\.vcl$/ ) {
	$depName =~ s/(.+)\.vcl/$1_vcl\.vsm/;  # turn filename.vcl to filename_vcl.vsm
	addDep getFile($depName);
    }
    else {
	addDep $depName;
    }

    # if this file is a vcl file
    # instead of recursing and adding dependencies to this object we need to 
    # output a new dependency rule for the generated file (filename_vcl.vsm)
    my $restoreState = 0;
    my $savedTopFile;
    if ( $bMutateVclFilenames && $fileName =~ /\.vcl$/ ) {
	$bMutateVclFilenames = 0;  # don't mutate filenames when creating dependencies for this file
	$savedTopFile = $CurTopFile; # save state
	$CurTopFile = $depName;
	addDep $fileName;
	$restoreState = 1; # return to the saved state after recursing
    }

    # recurse through other .includes
    open fileHandle, $fileName or die "Couldn't open file $fileName: $!\n";
    my $thisLine;
    my $newfile;
    my $dirname = getDir($fileName);
    while ( defined(fileHandle) and $thisLine = <fileHandle> ) {
	# skip comments
	if ( $thisLine =~ m/^\s*;/ ) {
	    # print STDERR "skipping: $thisLine";
	}
	else {
	    if ( $thisLine =~ m/^\s*\.include\s+"(\S+)"\s*$/ ) {
		$newfile = $1;
		my $filefound = 0;
		foreach $path (@IncludePaths) {
		    my $fullpath = "$dirname$path$newfile";
		    # print STDERR "testing $fullpath\n";
		    if ( -e $fullpath ) {
			getVSMDepsInFile $fullpath;
			$filefound = 1;
			last;
		    }
		}
		if ( ! $filefound ) {
		    print STDERR "WARNING: Could not find file $newfile.\n";
		}
	    }
	}
    }

    # restore state if necessary
    if ( $restoreState ) {
	$CurTopFile = $savedTopFile;
	$bMutateVclFilenames = 1;
    }
}

sub getCPPDepsInFile {
    my $fileName = $_[0];
    local *fileHandle;

    my $thisLine;

    # if this file is a vcl file
    # instead of recursing and adding dependencies to this object we need to 
    # output a new dependency rule for the generated file (filename_vcl.vsm)
    my $restoreState = 0;
    my $savedTopFile;
    if ( $bMutateVclFilenames && $fileName =~ /\.vcl$/ ) {
	$bMutateVclFilenames = 0;  # don't mutate filenames when creating dependencies for this file

	# save top file and replace with generated file
	$savedTopFile = $CurTopFile; # save state
	my $newName = $fileName;
	$newName =~ s/(.+)\.vcl/$1_vcl\.vsm/;  # turn filename.vcl to filename_vcl.vsm
	$CurTopFile = $newName;

	$CurIncludeVar = getStem($CurTopFile)."_INCLUDES";

	$restoreState = 1; # return to the saved state after recursing
    }

    # run cpp over this file to generate a Makefile rule, then strip out and
    # add the relevant dependencies
    # cpp doesn't like assembly comments (that begin with ';') so strip them out first
    my $includeDir = "";
    $includeDir = getDir $fileName;
    if ( $includeDir ) {
	$includeDir = "-I" . $includeDir;
    }
    open fileHandle, "sed 's/;.*/\\n/g' $fileName | gcc -MM -E $cppPath $includeDir - |";
    while ( $thisLine = <fileHandle> ) {
        chomp $thisLine;

        # remove the beginning "-: "
        $thisLine =~ s/^-://;
        # remove any continuation backslashes
        $thisLine =~ s/\s+\\\s*$//;
        # remove leading whitespace
        $thisLine =~ s/^\s*//;

        my $dep;
        foreach $dep (split /\s+/, $thisLine) {
	    addDep $dep;
	    addInclude $dep;
        }
    }
    close fileHandle;

    # recurse through other .includes
    open fileHandle, $fileName or die "Couldn't open file $fileName: $!\n";
    my $newfile;
    my $dirname = getDir($fileName);
    while ( defined(fileHandle) and $thisLine = <fileHandle> ) {
        if ( $thisLine =~ m/^\s*\.include\s+"(\S+)"\s*$/ ) {
	    $newfile = $1;
	    my $filefound = 0;
	    foreach $path (@IncludePaths) {
		my $fullpath = "$dirname$path$newfile";
		# print STDERR "testing $fullpath\n";
		if ( -e $fullpath ) {
		    getCPPDepsInFile $fullpath;
		    $filefound = 1;
		    last;
		}
	    }
	    if ( ! $filefound ) {
		print STDERR "WARNING: Could not find file $newfile.\n";
	    }
        }
    }

    # restore state if necessary
    if ( $restoreState ) {
	$CurTopFile = $savedTopFile;
	$CurIncludeVar = "";
	$bMutateVclFilenames = 1;
    }
}
