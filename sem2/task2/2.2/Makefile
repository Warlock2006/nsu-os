CC = gcc
CFLAGS = -g -Wall -pthread
RM = rm -f
INCLUDE = -I.

IMPLS = spinlock mutex condvar semaphore
TESTS = queue-threads queue-threads-usleep

ifeq ($(QUEUE_IMPL),)
ACTIVE = $(IMPLS)
else
ACTIVE = $(QUEUE_IMPL)
endif

all: $(foreach impl,$(ACTIVE),$(foreach t,$(TESTS),$(t)-$(impl)))

define RULE
$(1)-$(2): queue-$(2).c $(1).c queue.h
	$(CC) $(CFLAGS) $(INCLUDE) $$^ -o $$@
endef

$(foreach impl,$(ACTIVE), \
  $(foreach t,$(TESTS), \
    $(eval $(call RULE,$(t),$(impl))) \
  ) \
)

clean:
ifeq ($(QUEUE_IMPL),)
	$(RM) $(foreach impl,$(IMPLS),$(foreach t,$(TESTS),$(t)-$(impl)))
else
	$(RM) $(foreach t,$(TESTS),$(t)-$(QUEUE_IMPL))
endif
