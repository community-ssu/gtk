all:
	python2.5 Setup.py build_ext --inplace

test:	all
	python2.5 run_cheese.py

clean:
	@echo Cleaning Demos/callback
	@rm -f cheese.c *.o *.so *~ core
	@rm -rf build
