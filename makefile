fastml.so:
	CC='gcc -std=c99' python3 setup.py build_ext --inplace
	# python3 setup.py install

test:
	python3 test/test_logit.py
