.PHONY: build
build:
	@mkdir -p build
	@cd build && cmake .. && cmake --build . -j

.PHONY: test
test: build
	@cd build && ctest --output-on-failure

.phony: clean
clean:
	@rm -rf build
