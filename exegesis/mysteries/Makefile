# To run a given experiment, just run "make <experiment_dir>", e.g:
#   make read_modify_write


# Configuration
PERF=perf
COMPUTATION_EVENTS_1=uops_executed_port:port_4:u,ild_stall.lcp:u,unhalted_core_cycles:u
COMPUTATION_EVENTS_2=uops_executed_port:port_2:u,uops_executed_port:port_3:u,uops_executed_port:port_7:u
COMPUTATION_EVENTS_3=uops_executed_port:port_0:u,uops_executed_port:port_1:u,uops_executed_port:port_5:u,uops_executed_port:port_6:u
# Number of outer loop iterations.
LOOP_ITERATIONS=1000000
# Number of inner block repetitions.
INNER_REPEATS=1000


# List of experiments.
EXPERIMENTS=read_modify_write alu_16bits


# Disable builtin rules.
.SUFFIXES:


# Experiments are directory names but we should still execute the rule.
.PHONY: $(EXPERIMENTS) clean


# Generate one rule for each experiment: Collect all cc files for the experiment
# directory and depend on a '.run' rule for each of these cc files.
$(foreach experiment,$(EXPERIMENTS),$(eval \
  $(experiment): $(experiment).readme \
                 $(sort $(patsubst %.cc,%.run,$(wildcard $(experiment)/*.cc))) \
))


# Prints the README for and experiment.
%.readme: %/README.md
	@cat $<
	@echo -e "\n\n\n"


# Runs one example in an experiment: Prints comments, builds and runs perf,
# displays results.
%.run: %.comments %.perf
	@cat $*.comments
	@cat $*.perf | awk -F , '{if($$3 != "cycles") printf "%9.3f %s\n", $$1/($(INNER_REPEATS)*$(LOOP_ITERATIONS)), $$3;}' | sort -k2
	@echo -e "\n\n\n"


# Prints the top comments in a source file.
%.comments: %.cc
	@echo -e "----------------------------------------------------------" >> $@
	@echo -e "$*" >> $@
	@echo -e "----------------------------------------------------------\n" >> $@
	@cat $< | awk '{if(/\/\//)print;else { printf "\n"; exit } }' >> $@


# Produces a binary.
%.bin: %.cc
	@$(CC) -std=c++11 -DLOOP_ITERATIONS=$(LOOP_ITERATIONS) -o $@ $< -static


# Runs a binary in perf. Note that we measure batches of 4 events because perf
# can only handle 4 events at a time, and our code might not run for long enough
# that they can be effectively multiplexed.
%.perf: %.bin
	@$(PERF) stat -x, -e cycles --pfm-events=$(COMPUTATION_EVENTS_1) $< 2>> $@
	@$(PERF) stat -x, -e cycles --pfm-events=$(COMPUTATION_EVENTS_2) $< 2>> $@
	@$(PERF) stat -x, -e cycles --pfm-events=$(COMPUTATION_EVENTS_3) $< 2>> $@


clean:
	rm -f $(addsuffix /*.bin, $(EXPERIMENTS))
	rm -f $(addsuffix /*.perf, $(EXPERIMENTS))
