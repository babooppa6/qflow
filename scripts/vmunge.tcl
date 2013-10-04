#!/usr/bin/tclsh8.5
#-------------------------------------------------------------------------
# vmunge --- Hack for working around an Odin-II bug
#
# (1) Read the ".clk" file generated by vpreproc and record all
#     clock signals marked "internal wire".
#
# (2) Read the associated "_tmp.v" file generated by vpreproc.
#
# (3) For each internal wire clock signal named <signal>:
#
#   (3a) Add a new module input called "xloopback_in_<signal>"
#
#   (3b) Add a new module output called "xloopback_out_<signal>"
#
#   (3c) Convert all occurrences of <signal> in "always" statements
#	 to "xloopback_in_<signal>"
#
#   (3d) Convert all other occurrences of <signal> to "xloopback_out_<signal>"
#
# (4) For each internal wire reset signal names <signal>:
#
#   (4a) Add a new module output called "xreset_out_<signal>"
#
#   (4b) Convert all occurrences of <signal> in the file to "xreset_out_<signal>"
#
# This procedure ensures that clock "<signal>" is synthesized properly.
# A post-processing step needs to remove the loopback input and output
# from the netlist, and restore the name "<signal>".
#
# This procedure also ensures that any assigned reset signal is not
# optimized out of the synthesis, and is availble to the post-processor
# for determining reset states of flops.  The post-processor needs to
# remove the output and restore the name "<signal>".
#
#-------------------------------------------------------------------------
# Written by Tim Edwards July 25, 2013
# Open Circuit Design
# Expanded October 4, 2013, to handle assigned reset signals
#-------------------------------------------------------------------------
namespace path {::tcl::mathop ::tcl::mathfunc}

set verilogfile [lindex $argv 0]
set cellname [file rootname $verilogfile]
if {"$cellname" == "$verilogfile"} {
   set verilogfile ${cellname}.v
}

#-------------------------------------------------------------
# Open files for read and write

if [catch {open ${cellname}_tmp.v r} vnet] {
   puts stderr "Error: can't open file ${cellname}_tmp.v for reading!"
   exit 0
}

if [catch {open ${cellname}.clk r} ctmp] {
   puts stderr "Error: can't open file ${cellname}.clk for reading!"
   exit 0
}

if [catch {open ${cellname}.init r} cinit] {
   puts stderr "Error: can't open file ${cellname}.init for reading!"
   exit 0
}

if [catch {open ${cellname}_munge.v w} vtmp] {
   puts stderr "Error: can't open file ${cellname}_munge.v for writing!"
   exit 0
}

#-------------------------------------------------------------

# Use an array for quick checks on existance (maybe quicker than lsearch?)

set intclocks {}
while {[gets $ctmp line] >= 0} {
   if [regexp {([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)} $line lmatch clksig loc type] {
      if [string match $loc "internal"] {
	 if [string match $type "wire"] {
	    lappend intclocks $clksig
	 }
      }
   }
}

set intresets {}
while {[gets $cinit line] >= 0} {
   if [regexp {([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)} $line lmatch resetsig loc type] {
      if [string match $loc "internal"] {
	 if [string match $type "wire"] {
	    lappend intresets $resetsig
	 }
      }
   }
}


# If there are no internally-defined clocks or resets in the system,
# then just copy the input file to the output file and we're done.

if {([llength intclocks] == 0) && ([llength intresets] == 0)} {
   close $vnet
   close $ctmp
   close $cinit
   close $vtmp
   file copy -force ${cellname}_tmp.v ${cellname}_munge.v
   exit
}

set inmodule 0
while {[gets $vnet line] >= 0} {

    puts $vtmp $line

    # Find the "module" line and look for the open parens starting the
    # I/O list

    if [regexp {^[ \t]*module} $line lmatch] {
	set inmodule 1
    }

    if {$inmodule == 1} {

	if [regexp {\(} $line lmatch] {
	    foreach clock $intclocks {
		puts $vtmp "\txloopback_in_$clock,"
		puts $vtmp "\txloopback_out_$clock,"
	    }
	    foreach reset $intresets {
		puts $vtmp "\txreset_out_$reset,"
	    }
	} elseif [regexp {;} $line lmatch] {
	    puts $vtmp ""
	    foreach clock $intclocks {
		puts $vtmp "   input\txloopback_in_$clock;"
		puts $vtmp "   output\txloopback_out_$clock;"
	    }
	    foreach reset $intresets {
		puts $vtmp "   output\txreset_out_$reset;"
	    }
	    break;
	}
    }
}

# Parse the remainder of the file, looking for instances of the clock
# signals and replacing them with the loopback names

while {[gets $vnet line] >= 0} {

   if [regexp {[ \t]*always[ \t]+@} $line lmatch] {
      foreach clock $intclocks {
	 set RE [subst {\\m$clock\\M}]
	 regsub -all $RE $line xloopback_in_$clock line
      }
   } else {
      foreach clock $intclocks {
	 set RE [subst {\\m$clock\\M}]
	 regsub -all $RE $line xloopback_out_$clock line
      }
   }
   foreach reset $intresets {
      set RE [subst {\\m$reset\\M}]
      regsub -all $RE $line xreset_out_$reset line
   }
   puts $vtmp $line
}

close $vtmp
