#!/usr/bin/tclsh
#-------------------------------------------------------------------------
# ybuffer --- post-process a mapped .blif file generated by yosys
#
# Note that this file handles mapped blif files ONLY.  The only statements
# allowed in the file are ".model", ".inputs", ".outputs", ".latch",
# ".gate" (or ".subckt"), and ".end".
#
# All output nets are recorded
# Buffers are inserted before each output
# The existing output name is appended with "_RAW" and becomes an
#	internal net.
#
#-------------------------------------------------------------------------
# Written by Tim Edwards October 25, 2013
# Open Circuit Design
#-------------------------------------------------------------------------

if {$argc < 3} {
   puts stderr \
	"Usage:  ybuffer.tcl input_blif_file output_blif_file variables_file"
   exit 1
}

puts stdout "Buffering all outputs of module"

set mbliffile [lindex $argv 0]
set cellname [file rootname $mbliffile]
if {"$cellname" == "$mbliffile"} {
   set mbliffile ${cellname}.blif
}

set rootname ${cellname}
set outfile [lindex $argv 1]
set varsfile [lindex $argv 2]

#-------------------------------------------------------------
# Open files for read and write

if [catch {open $mbliffile r} bnet] {
   puts stderr "Error: can't open file $mbliffile for reading!"
   exit 1
}

if [catch {open $varsfile r} vfd] {
   puts stderr "Error: can't open file $varsfile for reading!"
   exit 1
}

if [catch {open $outfile w} onet] {
   puts stderr "Error: can't open file $outfile for writing!"
   exit 1
}

#-------------------------------------------------------------
# The variables file is a UNIX tcsh script, but it can be
# processed like a Tcl script if we substitute space for '='
# in the "set" commands.  Then all the variables are in Tcl
# variable space.
#-------------------------------------------------------------

while {[gets $vfd line] >= 0} {
   set tcmd [string map {= \ } $line]
   eval $tcmd
}

#-------------------------------------------------------------
# Yosys first pass of the blif file
# Parse all outputs and remember names.
#-------------------------------------------------------------

set outputs {}
set mode none

while {[gets $bnet line] >= 0} {
   if [regexp {^.outputs[ \t]*(.*)$} $line lmatch rest] {
      set mode outputs
      set line $rest
   }

   if {$mode == "outputs"} {
      while {[regexp {^[ \t]*([^ \t]+)(.*)$} $line lmatch signame rest] > 0} {
         lappend outputs $signame
	 set line $rest
      }
      if {![regexp {^\\[ \n\t]*$} $line lmatch]} {
         set mode none
      }
   }
}
seek $bnet 0

#-------------------------------------------------------------
# Now post-process the blif file
# The main thing to remember is that internal signals will be
# outputs of flops, but external pin names have to be translated
# to their internal names by looking at the OUTPUT section.
#-------------------------------------------------------------

while {[gets $bnet line] >= 0} {
   if [regexp {^[ \t]*\.gate} $line lmatch] {
      break
   }
   puts $onet $line
}

# Add buffers

foreach signal $outputs {
   puts $onet " .gate ${bufcell} ${bufpin_in}=${signal}_RAW ${bufpin_out}=${signal}"
}

# Reorder the outputs in decreasing order, such that outputs that are substrings
# of other outputs come after those outputs.  That way it will always match to
# the longest matching output name.

set outputs [lsort -decreasing $outputs]

while {1} {

   # All references to any signal in the output list have "_RAW" appended to the name

   if [regexp {^[ \t]*\.gate} $line lmatch] {
      set fidx 0
      foreach signal $outputs {
	 while {1} {
	    set sidx [string first ${signal} $line $fidx]
	    if {$sidx < 0} {
		break
	    } else {
		set fidx [string first " " $line $sidx]
		if {$fidx < 0} {
		   if {[expr {[string length $line] - $sidx}] == [string length $signal]} {
		      set line "${line}_RAW"
		   }
		   break
		} else {
		   if {[expr {$fidx - $sidx}] == [string length $signal]} {
		      set line [string replace $line $fidx $fidx "_RAW "]
		   }
		}
	    }
	 }
      }
   }
	 
   puts $onet $line

   if [regexp {^[ \t]*.end} $line lmatch] break
   if {[gets $bnet line] < 0} break
}

close $bnet
close $onet
