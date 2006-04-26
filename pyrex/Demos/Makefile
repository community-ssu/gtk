all:
	python2.4 Setup.py build_ext --inplace

test:	all
	python2.4 run_primes.py 20
	python2.4 run_numeric_demo.py
	python2.4 run_spam.py
	cd callback; $(MAKE) test

clean:
	@echo Cleaning Demos
	@rm -f *.c *.o *.so *~ core
	@rm -rf build
	@cd callback; $(MAKE) clean
	@cd embed; $(MAKE) clean
