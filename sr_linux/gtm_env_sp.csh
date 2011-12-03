#################################################################
#								#
#	Copyright 2001, 2003 Sanchez Computer Associates, Inc.	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################
#
##########################################################################################
#
#	gtm_env_sp.csh - environment variable values and aliases specific to Linux
#
##########################################################################################
#


# Once the environment variables have been initialized, be careful not to
# re-initialize them for each subshell (i.e., don't undo any explicit changes that
# have been made since the last version change).

if ( $?gtm_version_change == "1" ) then

	# Archiver definitions:
	# GNU ar q equals r, S prevents generating the symbol table but
	#			requires ranlib before linking
	setenv	gt_ar_option_create	"qSv"		# quick, verbose
	setenv	gt_ar_option_update	"rv"		# replace, verbose
	setenv	gt_ar_use_ranlib	"yes"

	# Assembler definitions:

	# Until convert from MASM or get MASM running under DOSEMU or such
	# setenv	gt_as_use_prebuilt	"yes"

	# GNU as
	setenv gt_as_assembler		"as"
	# to avoid naming files with .S
	# smw 1999/12/04 setenv gt_as_options_common	"-c -x assembler-with-cpp"
	setenv gt_as_option_debug	"--gstabs"
	setenv gt_as_option_DDEBUG	""

	# C definitions:

	# generate position independent code
	setenv 	gt_cc_shl_fpic		"-fPIC"

#	setenv	gt_cc_options_common 	"-c -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64"
	# For gcc: _BSD_SOURCE for caddr_t, others
	#	   _XOPEN_SOURCE=500 should probably define POSIX 199309 and/or
	#		POSIX 199506 but doesnt so...
	#	   -fsigned-char for Linux390 but shouldn't hurt x86

#	setenv	gt_cc_options_common	"-c -ansi -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -D_POSIX_C_SOURCE=199506L -D_FILE_OFFSET_BITS=64 -DFULLBLOCKWRITES -fsigned-char"
#	_GNU_SOURCE includes _XOPEN_SOURCE=400, _BSD_SOURCE, and _POSIX_C_SOURCE-199506L among others
	setenv	gt_cc_options_common	"-c -ansi -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -fsigned-char"
	setenv gt_cc_options_common "$gt_cc_options_common $gt_cc_shl_fpic -Wimplicit -Wmissing-prototypes"
        set lversion=`uname -r`
        set ltemp_ver=`echo $lversion | sed 's/./& /g'`
        if ($ltemp_ver[3] == "2"  && $ltemp_ver[1] == "2") then
            setenv gt_cc_options_common "$gt_cc_options_common -DNeedInAddrPort"
        endif

	# Linux gcc optimizations cause problems so do without them for now.
	setenv	gt_cc_option_optimize	$gt_cc_option_nooptimize

	# -g	generate debugging information for dbx (no longer overrides -O)
	setenv	gt_cc_option_debug	"-g"

	# Linker definitions:
	setenv	gt_ld_linker		"$gt_cc_compiler" # redefine to use new C compiler definition

	# -M		generate link map onto standard output
	# To expose gtm_ci, we use -rdynamic option (man dlopen).
	# Unfortunately, -rdynamic exposes all symbols in mumps executable.
	# Until we figure out an option to expose symbols selectively, we want to disable this option.
	# This means Call ins won't be available on Linux. Malli Sep 28, 2001
	# setenv	gt_ld_options_common	"-Wl,-M -rdynamic"
	setenv	gt_ld_options_common	"-Wl,-M"

	setenv 	gt_ld_options_gtmshr	"-Wl,--version-script,gtmshr_symbols.export"

	# need to re-define these in terms of new gt_ld_options_common:
	setenv	gt_ld_options_bta	"$gt_ld_options_common"
	setenv	gt_ld_options_dbg	"$gt_ld_options_common"
	setenv	gt_ld_options_pro	"$gt_ld_options_common"


#	setenv	gt_ld_syslibs		"-lcurses -lm -lsocket -lnsl -ldl -lposix4"
	setenv	gt_ld_syslibs		"-lncurses -lm -ldl"
	# -lrt for async I/O in mupip recover/rollback
	setenv gt_ld_aio_syslib		"-lrt"

	# Shared library definition overrides:
	setenv	gt_cc_shl_options	"-c $gt_cc_shl_fpic"

	setenv	gt_ld_shl_linker	"cc"
	setenv	gt_ld_shl_options	"-shared"

	setenv	gt_ld_shl_suffix	".so"


	# lint definition overrides
	# setenv	gt_lint_linter		""

	setenv	gt_lint_options_library	"-x"
	setenv	gt_lint_options_common	""

endif

# Assembler definitions:
# Note: we need to specify the assembler output file name or it will write it to the source directory.
alias	gt_as_bta	'gt_as $gt_as_option_debug $gt_as_option_nooptimize -o `basename \!:1 .s`.o \!:1'
alias	gt_as_dbg	'gt_as $gt_as_option_DDEBUG $gt_as_option_debug $gt_as_option_nooptimize -o `basename \!:1 .s`.o \!:1'
alias	gt_as_pro	'gt_as $gt_as_option_optimize -o `basename \!:1 .s`.o \!:1'
