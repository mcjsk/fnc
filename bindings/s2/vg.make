# We keep these in a separate file to avoid a rebuild when changing
# these flags. Damn... some automatic rules foil us here.
########################################################################
# Valgrind sanity checks...
VG := $(call ShakeNMake.CALL.FIND_FILE,valgrind)
ifneq (,$(VG))
VG_REPORT := VG.report.csv
VG_FLAGS ?= --leak-check=full -v --show-reachable=yes --track-origins=yes
#SCRIPT_LIST := $(shell ls -1 test-[0-9]*.s2 | sort)
# Whether or not to collect massif-based stats...
RUN_MASSIF := 1
.PHONY: vg
# Reminder: don't use -A b/c it breaks output buffering tests!
VG_SHELL_FLAGS=--a -S
VG_UNIT_RUN_CMD = ./$(f-s2sh.BIN) $(VG_SHELL_FLAGS)
vg: $(f-s2sh.BIN) $(UNIT_GENERATED)
	@echo "Running: $(VG) $(VG_FLAGS) $(VG_UNIT_RUN_CMD) ..."; \
	export LD_LIBRARY_PATH="$(TOP_SRCDIR):$${LD_LIBRARY_PATH}"; \
	massif_tmp=tmp.massif; \
	for i in $(sort $(UNIT_SCRIPT_LIST)) $(UNIT_GENERATED); do \
		vgout=$$i.vg; \
		xtraargs="-o $$i._out -f $$i"; \
		cmd="$(VG) $(VG_FLAGS) $(VG_UNIT_RUN_CMD) $$xtraargs"; \
		echo -n "**** Valgrinding [$$i]: "; \
		$$cmd 2>&1 | tee $$vgout | grep 'total heap usage' || exit $$?; \
		grep 'ERROR SUMMARY' $$vgout | grep -v ' 0 errors' && echo "See $$vgout!"; \
		vgout=$$i.massif; \
		cmd="$(VG) --tool=massif --time-unit=B --massif-out-file=$$vgout --heap-admin=0  $(VG_UNIT_RUN_CMD) $$xtraargs"; \
		test x1 = x$(RUN_MASSIF) || continue; \
		echo -n "**** Massifing [$$i]: "; \
		$$cmd > $$massif_tmp 2>&1 || exit $$?; \
		echo -n "==> $$vgout Peak RAM: "; \
	    ms_print $$vgout | perl -n -e 'if(m/^(\s+)([KM]B)$$/){my $$x=$$2; $$_=<>; m/^(\d[^^]+)/; print $$1." ".$$x; exit 0;}'; \
		echo; \
	done; \
	rm -f $$massif_tmp
	@test x1 = x$(RUN_MASSIF) || exit 0; \
	echo "Done running through s2 scripts. Collecting stats..."; \
	echo 'script,allocs,frees,totalMemory,peakMemUsage,peakMemUsageUnit' > $(VG_REPORT); \
	for i in $$(ls -1 unit/*.s2.vg unit2/*.s2.vg | sort) $(patsubst %.s2,%.s2.vg,$(UNIT_GENERATED)); do \
		base=$${i%%.vg}; \
		echo -n "$$base,"; \
		grep 'total heap usage' $$i | sed -e 's/,//g' -e 's/^\.\///' | awk '{printf "%d,%d,%d,",$$5,$$7,$$9}'; \
		ms_print $${base}.massif | perl -n -e 'if(m/^(\s+)([KM]B)$$/){my $$x=$$2; $$_=<>; m/^(\d[^^]+)/; print $$1.",".$$x; exit 0;}'; \
		echo; \
	done >> $(VG_REPORT); \
	rm -f unit/ms_print.tmp.* unit2/ms_print.tmp.* ms_print.tmp.*; \
	echo "Stats are in $(VG_REPORT):"; \
	tr ',' '\t' < $(VG_REPORT)

CLEAN_FILES += $(wildcard \
	$(patsubst %.s2,%.s2.vg,$(UNIT_GENERATED)) \
	$(patsubst %,unit/%,*.vg *._out *.massif *.db *~ *.uncompressed *.z *.zip *.2) \
	$(patsubst %,unit2/%,*.vg *._out *.massif *.db *~ *.uncompressed *.z *.zip *.2) \
	)
# 'vg' proxies which tweak various s2/cwal-level optimizations...
#vgp: VG_SHELL_FLAGS+=--p
#vgp: VG_REPORT:=VG.report-p.csv
#vgp: vg
vgr: VG_SHELL_FLAGS+=--R -C -S
vgr: VG_REPORT:=VG.report-r.csv
vgr: vg
vgs: VG_SHELL_FLAGS+=--S -R -C
vgs: VG_REPORT:=VG.report-s.csv
vgs: vg
#vgt: VG_SHELL_FLAGS+=--t
#vgt: VG_REPORT:=VG.report-t.csv
#vgt: vg
vgrs: VG_SHELL_FLAGS+=--R -C --S
vgrs: VG_REPORT:=VG.report-rs.csv
vgrs: vg
vgrsc: VG_SHELL_FLAGS+=--R --C --S
vgrsc: VG_REPORT:=VG.report-rsc.csv
vgrsc: vg
# vgrst: VG_SHELL_FLAGS+=--r --s --t
# vgrst: VG_REPORT:=VG.report-rst.csv
# vgrst: vg
# vgprst: VG_SHELL_FLAGS+=--r --s --t --p
# vgprst: VG_REPORT:=VG.report-prst.csv
# vgprst: vg
VG_REPORT_MEGA := VG.report-mega.csv
CLEAN_FILES += $(VG_REPORT_MEGA)
VG_COMMANDS := \
	$(MAKE) vg \
	&& $(MAKE) vgr \
	&& $(MAKE) vgs \
	&& $(MAKE) vgrs \
	&& $(MAKE) vgrsc

vgall:
	@start=$$(date); \
	echo "Start time: $$start"; \
	$(VG_COMMANDS) || exit; \
	echo "Run time: $$start - $$(date)"; \
	rm -f $(VG_REPORT_MEGA); \
	for i in VG.report*.csv; do \
		test -s $$i || continue; \
		echo $$i | grep t-mega >/dev/null && continue; \
		echo "Report: $$i"; \
		cut -d, -f1,2,4,5 $$i | tr ',' '\t'; \
	done > $(VG_REPORT_MEGA); \
	echo "Consolidated valgrind report is in [$(VG_REPORT_MEGA)]."

endif
#/$(VG)
########################################################################
