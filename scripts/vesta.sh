#!/bin/tcsh -f
#----------------------------------------------------------
# Static timing analysis script using vesta
#----------------------------------------------------------
# Tim Edwards, 10/29/13, for Open Circuit Design
#----------------------------------------------------------

if ($#argv < 2) then
   echo Usage:  vesta.sh [options] <project_path> <source_name>
   exit 1
endif

# Split out options from the main arguments
set argline=(`getopt "nr" $argv[1-]`)

set options=`echo "$argline" | awk 'BEGIN {FS = "-- "} END {print $1}'`
set cmdargs=`echo "$argline" | awk 'BEGIN {FS = "-- "} END {print $2}'`
set argc=`echo $cmdargs | wc -w`

if ($argc == 2) then
   set argv1=`echo $cmdargs | cut -d' ' -f1`
   set argv2=`echo $cmdargs | cut -d' ' -f2`
else
   echo Usage:  vesta.sh [options] <project_path> <source_name>
   echo   where
   echo       <project_path> is the name of the project directory containing
   echo                 a file called qflow_vars.sh.
   echo       <source_name> is the root name of the verilog file
   exit 1
endif

set projectpath=$argv1
set sourcename=$argv2
set rootname=${sourcename:h}

# This script is called with the first argument <project_path>, which should
# have file "qflow_vars.sh".  Get all of our standard variable definitions
# from the qflow_vars.sh file.

if (! -f ${projectpath}/qflow_vars.sh ) then
   echo "Error:  Cannot find file qflow_vars.sh in path ${projectpath}"
   exit 1
endif

source ${projectpath}/qflow_vars.sh
source ${techdir}/${techname}.sh
cd ${projectpath}

if (! ${?vesta_options} ) then
   set vesta_options = ""
endif

#----------------------------------------------------------
# Done with initialization
#----------------------------------------------------------

cd ${synthdir}

#------------------------------------------------------------------
# Generate the static timing analysis results
#------------------------------------------------------------------

echo ""
echo "Running vesta static timing analysis"
echo ""
${bindir}/vesta ${vesta_options} ${rootname}.rtlnopwr.v \
		${techdir}/${libertyfile} |& tee -a ${synthlog}
echo ""

#------------------------------------------------------------
# Done!
#------------------------------------------------------------