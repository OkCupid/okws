
SUBDIRS = system regtest unit

.PHONY: test tameclean

tameclean: 
	for dir in system unit; do \
		(cd $$dir && $(MAKE) tameclean ) ; \
	done

test:
	for dir in system pub; do \
		(cd $$dir && $(MAKE) test ) ; \
	done
