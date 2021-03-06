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

# Commands to build GT.M downloaded from SourceForge
# 1. 'cd' to the GT.M directory where sr_* directories are copied to.
# 2. Define an environment variable gtm_curpro to point to the full path of the prior GT.M installation.
#    (download and install GT.M binary distribution from SourceForge if you do not have
#    GT.M installed already).
# 3. To build debug version with no compiler optimzations -
# 		gmake -f sr_unix/comlist.mk -I./sr_unix -I./sr_linux buildtypes=dbg
#    To build a version enabling optimizations -
#    		gmake -f sr_unix/comlist.mk -I./sr_unix -I./sr_linux buildtypes=pro
#

# get_lib_dirs.mk must be in the same directory as this makefile
verbose ?= 0
include get_lib_dirs.mk

CURDIR=$(shell pwd)

ifeq ($(MAKELEVEL),0)
#the first-level make invocation - rules to create & clean directories and build utilities selectively.

ifndef buildtypes
buildtypes=pro
endif

ifndef gtm_ver
gtm_ver=$(CURDIR)
endif

gt_ar_archiver=ar
gt_ar_options=rv

# top level make - builds directory structure, calls make for each build type,
# and creates package

VPATH=$(addprefix $(gtm_ver)/, $(gt_src_list))
exe_list=mumps dse geteuid gtm_dmna gtmsecshr lke mupip gtcm_server gtcm_gnp_server gtcm_play gtcm_pkdisp gtcm_shmclean semstat2 ftok
make_i_flags=$(addprefix -I$(gtm_ver)/, $(gt_src_list))

export #export all variables defined here to sub-make

# Build the complete suit for packaging - all executables, % utilities, help files etc.
all: dirs $(addsuffix _all, $(buildtypes)) ;

dirs: 	$(addprefix $(gtm_ver)/, $(addsuffix /obj, $(buildtypes))) \
	$(addprefix $(gtm_ver)/, $(addsuffix /map, $(buildtypes))) ;

# Compile and archive all modules and stop.
compile: dirs $(addsuffix _compile, $(buildtypes)) ;

# Build all executables and stop.
links: dirs $(addsuffix _links, $(buildtypes)) ;

# Rules to make executables selectively (eg. make mumps, make dse etc..)
$(exe_list):%: dirs $(addsuffix _%, $(buildtypes)) ;

clean: $(addsuffix _clean, $(buildtypes))
	rm -f idtemp ostemp

# Package GT.M installation kit.
package: $(addsuffix _tar, $(buildtypes))

%_tar: release_name.h
	@echo "packaging GT.M..."
	grep RELEASE_NAME $< | awk '{print $$4}' | sed 's/[\.]//' | sed 's/-//' > idtemp
	grep RELEASE_NAME $< | awk '{print $$5}' | tr '[:upper:]' '[:lower:]' > ostemp
	@tar -zcvf gtm_`cat idtemp`_`cat ostemp`_$*.tar.gz -C $* $(filter-out obj map, $(notdir $(wildcard $*/*)))
	rm -f idtemp ostemp

%_clean:
	rm -rf $(gtm_ver)/$*
	rm -f $(gtm_ver)/*$*.tar.gz
%/obj:
	mkdir -p $@
%/map:
	mkdir -p $@

dbg_%:comlist.mk
	$(MAKE) -C $(gtm_ver)/dbg/obj -I$(gtm_ver)/dbg/obj $(make_i_flags) -f $< CURRENT_BUILDTYPE=dbg $*
pro_%:comlist.mk
	$(MAKE) -C $(gtm_ver)/pro/obj -I$(gtm_ver)/pro/obj $(make_i_flags) -f $< CURRENT_BUILDTYPE=pro $*
bta_%:comlist.mk
	$(MAKE) -C $(gtm_ver)/bta/obj -I$(gtm_ver)/bta/obj $(make_i_flags) -f $< CURRENT_BUILDTYPE=bta $*

else
# Second-level make invocation: compute dependencies, compile, archive, link and test.

# gt_src_list is the list of all source (sr_*) directories. allfiles_list is the superset of all
# GT.M files (.c, .s, .m, .list, etc. etc.) present in all sr_* directories.
allfiles_list:=$(sort $(notdir $(foreach d,$(gt_src_list),$(wildcard $(gtm_ver)/$(d)/*))))

# allfiles_list computation should precede this include, since os390:gtm_env_sp.mk requires $(allfiles_list)
include gtm_env_sp.mk

# the list of all GT.M executables
exe_list:=libgtmshr$(gt_ld_shl_suffix) $(exe_list) $(gt_svc_exe)


# In the following code, various categories of source files are filtered from allfiles_list into
# separate variables based on the file extention.

# m file stuff.  These list builds go to great pain to insure that either post cms_load
# forms and pre-cms load forms work.
mfile_list:=$(filter-out _%.m, $(filter %.m, $(allfiles_list)))
mptfile_list:=$(sort $(basename $(filter %.mpt, $(allfiles_list))) $(basename $(patsubst _%.m, %, $(filter _%.m, $(allfiles_list)))))
mfile_targets:=$(addsuffix .m,$(foreach f,$(basename $(mfile_list)), $(shell echo $(f) | tr '[:lower:]' '[:upper:]')))
mptfile_targets:=$(addprefix _,$(addsuffix .m, $(foreach f,$(mptfile_list), $(shell echo $(f) | tr '[:lower:]' '[:upper:]'))))
cfile_list:=$(filter %.c, $(allfiles_list))

ifdef gt_as_src_from_suffix
#
# DUX requires .m64 to be gawk'ed and assembled as well
#
sfile_list:=$(filter %$(gt_as_src_suffix) %$(gt_as_src_from_suffix), $(allfiles_list))
else
sfile_list:=$(filter %$(gt_as_src_suffix), $(allfiles_list))
endif

helpfile_list:=$(filter %.hlp, $(allfiles_list))
sh_list:=$(filter %.sh, $(allfiles_list))
gtc_list:=$(filter %.gtc, $(allfiles_list))
csh_files:=$(filter lower%.csh upper%.csh, $(allfiles_list))
list_files:=$(filter %.list, $(allfiles_list))
msgfile_list:=$(filter %.msg, $(allfiles_list))

hfile_list := gtm_stdio.h gtm_stdlib.h gtm_string.h gtmxc_types.h $(hfile_list_sp)
sh_targets:=$(basename $(sh_list))

msgcfile_list=$(addsuffix _ctl.c,$(basename $(msgfile_list)))
msgofile_list=$(addsuffix .o,$(basename $(msgcfile_list)))
list_file_libs=$(addsuffix .a,$(basename $(list_files)))


# object files
# NOTE: sort/basename weeds out .s, .c duplication and
#       rules giving %.s priority over %.c cause the %.s
#       version to always be used
ofile_list:=$(addsuffix .o,$(sort $(basename $(cfile_list)) $(basename $(sfile_list))))

#
# dynamic depend list - weed out .s based .o's
#
dep_list:=$(addsuffix .d,$(filter-out $(basename $(sfile_list)),$(basename $(cfile_list))))

# objects on link command lines
mumps_obj=gtm.o
gtmshr_obj=mumps_clitab.o gtm_main.o
lke_obj=lke.o lke_cmd.o
dse_obj=dse.o dse_cmd.o
mupip_obj=mupip.o mupip_cmd.o
gtm_dmna_obj=daemon.o
gtmsecshr_obj=gtmsecshr.o
geteuid_obj=geteuid.o
semstat2_obj=semstat2.o
ftok_obj=ftok.o
gtcm_server_obj=gtcm_main.o omi_srvc_xct.o
gtcm_gnp_server_obj=gtcm_gnp_server.o gtcm_gnp_clitab.o
gtcm_play_obj=gtcm_play.o omi_sx_play.o
gtcm_pkdisp_obj=gtcm_pkdisp.o
gtcm_shmclean_obj=gtcm_shmclean.o
dtgbldir_obj=dtgbldir.o

# exclude .o's in .list files, .o's used in ld's below, plus dtgblkdir.o (which doesn't appear to be
# used anywhere!
non_mumps_objs:=$(addsuffix .o,$(shell cat $(foreach d,$(gt_src_list),$(wildcard $(gtm_ver)/$(d)/*.list))))
exclude_list:= \
	$(non_mumps_objs) \
	$(mumps_obj) \
	$(gtmshr_obj) \
	$(gtm_svc_obj) \
	$(lke_obj) \
	$(dse_obj) \
	$(mupip_obj) \
	$(gtm_dmna_obj) \
	$(gtmsecshr_obj) \
	$(geteuid_obj) \
	$(semstat2_obj) \
	$(ftok_obj) \
	$(gtcm_server_obj) \
	$(gtcm_gnp_server_obj) \
	$(gtcm_play_obj) \
	$(gtcm_pkdisp_obj) \
	$(gtcm_shmclean_obj) \
	$(dtgbldir_obj)

# retain_list contains the modules listed in a .list file that also need to be
# included in libmumps.a (eg. getmaxfds, sleep in sr_sun:libgtmrpc.list)
libmumps_obj:=$(sort $(filter-out $(exclude_list),$(ofile_list)) $(msgofile_list) $(retain_list))

# rules, lists, variables specific to each type of build

#ifndef gtm_dist
gtm_dist=$(gtm_ver)/$(CURRENT_BUILDTYPE)
#endif

gt_cc_option_I:=$(gt_cc_option_I) $(addprefix -I$(gtm_ver)/, $(gt_src_list)) -I$(CURDIR)
gt_cc_option_DDEBUG=-DDEBUG
ifeq ($(CURRENT_BUILDTYPE), pro)
gt_cc_options=$(gt_cc_option_optimize) $(gt_cc_options_common)
gt_as_options=$(gt_as_option_optimize) $(gt_as_options_common)
gt_ld_options_buildsp=$(gt_ld_options_pro)
endif
ifeq ($(CURRENT_BUILDTYPE), bta)
gt_cc_options=$(gt_cc_option_DDEBUG) $(gt_cc_option_optimize) $(gt_cc_options_common)
gt_as_options=$(gt_as_option_DDEBUG) $(gt_as_option_optimize) $(gt_as_options_common)
gt_ld_options_buildsp=$(gt_ld_options_bta)
endif
ifeq ($(CURRENT_BUILDTYPE), dbg)
gt_cc_options=$(gt_cc_option_DDEBUG) $(gt_cc_option_debug) $(gt_cc_options_common)
gt_as_options=$(gt_as_option_DDEBUG) $(gt_as_option_debug) $(gt_as_options_common)
gt_ld_options_buildsp=$(gt_ld_options_dbg)
endif
gt_cc_options += $(gt_cc_option_I)
gt_as_options += $(gt_cc_option_I)

# gt_ld_options should be set with '=' to allow lazy evaluation of gt_ld_options_loadmap
gt_ld_options=$(gt_ld_options_common) $(gt_ld_options_buildsp) $(gt_ld_options_loadmap) -L$(CURDIR)

gt_cpus ?= 2

ifdef gt_ar_gtmrpc_name
gt_ar_gtmrpc_name_target=../lib$(gt_ar_gtmrpc_name).a
endif

postbuild=$(gt_ar_gtmrpc_name_target) dotcsh dotsh helpfiles hfiles gtcmconfig \
	../mumps.gld ../gtmhelp.dat ../gdehelp.dat

all:	links mfiles mcompiles testit $(postbuild)

compile:libmumps.a $(list_file_libs) $(filter-out $(non_mumps_objs), $(exclude_list))

%.export:%.exp
	$(gt-export)

testit:
	echo $(postbuild)

links: $(exe_list)
$(exe_list):%: $(prebuild) ../% ;

vars:
	echo MAKECMDGOALS $(MAKECMDGOALS)

../mumps.gld:
	cd ..;gtm_dist=$(gtm_dist);export gtm_dist;gtmgbldir=./$(notdir $@);export gtmgbldir;\
		echo exit | ./mumps -run GDE

define compile-help
cd ..;gtm_dist=$(gtm_dist);export gtm_dist;gtmgbldir=$(gtm_dist)/$(notdir $(basename $@));export gtmgbldir; \
	echo Change -segment DEFAULT -block=2048 -file=$(gtm_dist)/$(notdir $@) > hctemp;  \
	echo Change -region DEFAULT -record=1020 -key=255 >> hctemp; \
	echo exit >> hctemp; \
	cat hctemp | ./mumps -run GDE; \
	./mupip create; \
	echo "Do ^GTMHLPLD" > hctemp; \
	echo $(gtm_dist)/$(notdir $^) >> hctemp; \
	echo Halt >> hctemp; \
	cat hctemp | ./mumps -direct; \
	rm -f hctemp
endef
../gtmhelp.dat: ../mumps.hlp
	$(compile-help)
../gdehelp.dat: ../gde.hlp
	$(compile-help)

mcompiles:
	cd ..;gtm_dist=$(dir $(CURDIR));export gtm_dist;gtmgbldir=$(notdir $@);export gtmgbldir; ./mumps *.m

dotcsh: $(csh_files)
	cp -f $^ ..
	cd ..;chmod +x $(notdir $^)

dotsh: $(sh_targets)
	cp -f $^ ..

helpfiles: $(helpfile_list)
	cp -pf $^ ..

hfiles: $(hfile_list)
	cp -f $^ ..

mfiles: $(addprefix ../, $(mfile_targets) $(mptfile_targets))

$(list_file_libs): $(list_files)

ifdef gt_ar_gtmrpc_name_target
$(gt_ar_gtmrpc_name_target): lib$(gt_ar_gtmrpc_name).a
	cp $< $@
endif

# executables
define gt-ld
rm -f $@
@echo "linking $(notdir $@)..."
@echo $(gt_ld_linker) $(gt_ld_options) -o $@ $(gt_ld_sysrtns) $+ $(gt_ld_syslibs) > ../map/$(notdir $@).map 2>&1
@$(gt_ld_linker) $(gt_ld_options) -o $@ $(gt_ld_sysrtns) $+ $(gt_ld_syslibs) >> ../map/$(notdir $@).map 2>&1
endef

ifdef gt_svc_exe
# Note: gtm_svc should link with gtm_dal_svc.o before gtm_mumps_call_clnt.o(libgtmrpc.a) to
#       resolve conflicting symbols (gtm_init_1, gtm_halt_1 etc..) appropriately.
../$(gt_svc_exe): $(gtm_svc_obj) $(gtmshr_obj) libmumps.a libgnpclient.a libcmisockettcp.a $(gt_ld_gtmrpc_library_option)
	$(gt-ld)
endif

../mumps: $(mumps_obj)
	$(gt-ld)

../dse: $(dse_obj) libdse.a libmumps.a libstub.a
	$(gt-ld)

../geteuid: $(geteuid_obj) libmumps.a
	$(gt-ld)

../gtm_dmna: $(gtm_dmna_obj) libmumps.a
	$(gt-ld)

../gtmsecshr: $(gtmsecshr_obj) libmumps.a
	$(gt-ld)

../lke: $(lke_obj) liblke.a libmumps.a libgnpclient.a libmumps.a libgnpclient.a libcmisockettcp.a
	$(gt-ld)

../mupip: $(mupip_obj) libmupip.a libmumps.a libstub.a $(gt_ld_aio_syslib)
	$(gt-ld)

../gtcm_server: $(gtcm_server_obj) libgtcm.a libmumps.a libstub.a
	$(gt-ld)

../gtcm_gnp_server: $(gtcm_gnp_server_obj) libgnpserver.a liblke.a libmumps.a libcmisockettcp.a libstub.a
	$(gt-ld)

../gtcm_play: $(gtcm_play_obj) libgtcm.a libmumps.a libstub.a
	$(gt-ld)

../gtcm_pkdisp: $(gtcm_pkdisp_obj) libgtcm.a libmumps.a libstub.a
	$(gt-ld)

../gtcm_shmclean: $(gtcm_shmclean_obj) libgtcm.a libmumps.a libstub.a
	$(gt-ld)

../semstat2: $(semstat2_obj)
	$(gt-ld)

../ftok: $(ftok_obj)
	$(gt-ld)

# build GT.M shared library(libgtmshr) from PIC-compiled .o files
../libgtmshr$(gt_ld_shl_suffix): gtmshr_symbols.export $(gtmshr_obj) libmumps.a libgnpclient.a libcmisockettcp.a
	rm -f $@
	@echo "linking $(notdir $@)..."
	@echo $(gt_ld_linker) $(gt_ld_options) $(gt_ld_shl_options) $(gt_ld_options_gtmshr) -o $@ $(gtmshr_obj) -lmumps -lgnpclient -lcmisockettcp $(gt_ld_syslibs) > ../map/$(notdir $@).map 2>&1
	@$(gt_ld_linker) $(gt_ld_options) $(gt_ld_shl_options) $(gt_ld_options_gtmshr) -o $@ $(gtmshr_obj) -lmumps -lgnpclient -lcmisockettcp $(gt_ld_syslibs) >> ../map/$(notdir $@).map 2>&1

gtcmconfig: $(gtc_list) gtcm_gcore
	cp -f $^ ..
	cd ..;chmod a-wx $(notdir $^);mv -f configure.gtc configure
	cd ..;touch gtmhelp.dmp;chmod a+rw gtmhelp.dmp

test_type:
ifndef gt_cc_options
	$(error CURRENT_BUILDTYPE not properly defined)
endif

# no need to keep the archived object files
.INTERMEDIATE: $(libmumps_obj) $(non_mumps_objs)

#
# autodepend files for C files
#
-include $(dep_list)
#
# autodepend files for M files
#
-include $(mfile_list:.m=.mdep)
#
# autodepend files for mpt files
#
-include $(mptfile_list:=.mptdep)
#
# autodepend files for .a files
#
-include $(list_files:.list=.ldep)

# Overriding the implicit archive rule a(m):m to accumulate all changed .o files
# in a temporary dependency (.ardep) file that will be used by ar to archive
# all files in a single command.
# This enhancement [of accumulating in a temporary .ardep file instead of updating
# the library righaway] improves the full building time. However for incremental
# builds the object file is updated into the archive immediately.
(%):%
ifeq ($(incremental),1)
	@$(gt_ar_archiver) $(gt_ar_options) $@ $<
else
	@echo $< >> $(basename $@).ardep
endif

# Since ecode_set.c includes merrors_ansi.h, merrors.msg should be precompiled.
ecode_set.d:merrors_ctl.c

%.d:%.c
ifeq ($(verbose),1)
	@echo generating $@...
endif
	$(gt-dep)

ifeq ($(incremental),1)
%.ldep:%.list
	@echo $*.a\:$*.a\($$\(addsuffix .o,$$\(shell cat $<\)\)\) > $@
	@echo "\t@ranlib "$$\@ >> $@
else
%.ldep:%.list
	@echo $*.a\:$*.a\($$\(addsuffix .o,$$\(shell cat $<\)\)\) $*.ardep > $@
	@echo "\t@echo Processing "$$\@ "; cp -f $*.ardep _$*.ardep; echo >$*.ardep" >> $@
	@echo "\t@cat _$*.ardep | xargs $(gt_ar_archiver) $(gt_ar_options) "$$\@ >>$@
	@echo "\t@rm -f _$*.ardep" >> $@
endif

%.mdep:%.m
	@echo ../$(shell echo $* | tr '[:lower:]' '[:upper:]').m: $< > $@
	@echo "\t"cp -f $$\< $$\@ >> $@
%.mptdep:_%.m
	@echo ../_$(shell echo $* | tr '[:lower:]' '[:upper:]').m: $< > $@
	@echo "\t"cp -f $$\< $$\@ >> $@
%.mptdep:%.mpt
	@echo ../_$(shell echo $* | tr '[:lower:]' '[:upper:]').m: $< > $@
	@echo "\t"cp -f $$\< $$\@ >> $@

# By setting gtm_curpro to point to a prior installed GT.M directory (if available), the following
# rule automatically generates *_ctl.c from *.msg.
ifdef gtm_curpro
%_ctl.c:%.msg msg.m
	gtm_dist=$(gtm_curpro);export gtm_dist;\
		$(gtm_curpro)/mumps $(filter-out $<, $^);\
		$(gtm_curpro)/mumps -run msg $< unix
	@rm -f msg.o
endif

# By default [since the rule %.o:%.s precedes %.o:%.c], the .s files take precedence over
# .c files if both versions exist for a module. The following rule allows us to reverse
# this behavior for a special set of modules (eg. compswap for sparcv8 etc.) by assigning
# them to a variable gt_cc_before_as [in gtm_env_sp.mk].
# gt_cc_before_as should be defined to the list of .o files for which both .c and .s exist
# but need to be compiled from .c instead of from .s files.
ifdef gt_cc_before_as
$(gt_cc_before_as):%.o:%.c	#override rules for gt_cc_before_as modules ONLY
ifeq ($(verbose),1)
	$(gt_cc_compiler) -c $(gt_cc_options) -o $@ $<
else
	@echo "$< ----> $(CURDIR)/$@"
	@$(gt_cc_compiler) -c $(gt_cc_options) -o $@ $<
endif
endif

ifdef gt_as_src_from_suffix
%.o:%$(gt_as_src_from_suffix)
ifeq ($(verbose),1)
	$(gt-as-convert)
else
	@echo "$< ----> $(CURDIR)/$@"
	@$(gt-as-convert)
endif
endif
%.o:%$(gt_as_src_suffix)
ifeq ($(verbose),1)
	$(gt-as)
else
	@echo "$< ----> $(CURDIR)/$@"
	@$(gt-as)
endif

%.o:%.c
ifeq ($(verbose),1)
	$(gt_cc_compiler) -c $(gt_cc_options) -o $@ $<
else
	@echo "$< ----> $(CURDIR)/$@"
	@$(gt_cc_compiler) -c $(gt_cc_options) -o $@ $<
endif

omi_sx_play.c: omi_srvc_xct.c
	@cp $< $@

ifeq ($(incremental),1)
libmumps.a: libmumps.a($(msgofile_list) $(libmumps_obj))
	@ranlib $@
else
libmumps.a: libmumps.a($(msgofile_list) $(libmumps_obj)) libmumps.ardep
	@echo Processing $@ ;cp -f libmumps.ardep _libmumps.ardep; echo "">libmumps.ardep
	@cat _libmumps.ardep | xargs $(gt_ar_archiver) $(gt_ar_options) $@
	@rm -f _libmumps.ardep
endif

endif #second-level make invocation
