TESTS = \
    test_cpy \
    test_ref

CFLAGS = -Wall -Werror -g

# Control the build verbosity
ifeq ("$(VERBOSE)","1")
    Q :=
    VECHO = @true
else
    Q := @
    VECHO = @printf
endif

GIT_HOOKS := .git/hooks/applied

.PHONY: all clean

all: $(GIT_HOOKS) $(TESTS)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

OBJS_LIB = \
    tst.o \
    bench.o

OBJS := \
    $(OBJS_LIB) \
    test_cpy.o \
    test_ref.o

deps := $(OBJS:%.o=.%.o.d)

test_%: test_%.o $(OBJS_LIB)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -c -MMD -MF .$@.d $<

bench:
	perf stat --repeat 100 \
		-e cache-misses,cache-references,instructions,cycles \
		sudo chrt -f 99 taskset -c 0 ./test_cpy --bench
	perf stat --repeat 100 \
		-e cache-misses,cache-references,instructions,cycles \
		sudo chrt -f 99 taskset -c 0 ./test_ref --bench

plot:
	gnuplot ./scripts/insert.gp
	gnuplot ./scripts/search.gp

clean:
	$(RM) $(TESTS) $(OBJS)
	$(RM) $(deps)

-include $(deps)
