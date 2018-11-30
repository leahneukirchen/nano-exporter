# enabled collectors

COLLECTORS =
COLLECTORS += cpu
COLLECTORS += diskstats
COLLECTORS += filesystem
COLLECTORS += hwmon
COLLECTORS += meminfo
COLLECTORS += network
COLLECTORS += textfile
COLLECTORS += uname

# compile settings

CFLAGS = -std=c11 -Os
LDFLAGS = -Os -s

# build rules

PROG = prometheus-nano-exporter
SRCS = main.c scrape.c util.c $(foreach c,$(COLLECTORS),$(c).c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o : %.c
%.o : %.c $(DEPDIR)/%.d
	$(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

# make clean

.PHONY: clean
clean:
	$(RM) $(PROG) $(OBJS)
	$(RM) -r $(DEPDIR)

# dependencies

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))