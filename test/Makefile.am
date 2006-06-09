
SUBDIRS = unit system

.PHONY: test

test:
	for dir in system; do \
		(cd $$dir && $(MAKE) test ) ; \
	done
