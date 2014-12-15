
SUBDIRS = system regtest unit pub bson
.PHONY: test tameclean

tameclean:
	for dir in system unit bson; do \
		(cd $$dir && $(MAKE) tameclean ) ; \
	done

test:
	for dir in system; do \
		(cd $$dir && $(MAKE) test ) ; \
	done
