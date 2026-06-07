SHELL := /bin/bash

CMAKE_BUILD_TYPE := Release
BUILD_MPQCLI     := ON
CLANG_VERSION    := 18
VERSION          := $(shell awk '/project\(MPQCLI VERSION/ {gsub(/\)/, "", $$3); print $$3}' CMakeLists.txt)
README           := README.md
PACKAGE_URL      := https://github.com/thegraydot/mpqcli/pkgs/container/mpqcli
GCC_INSTALL_DIR  := $(shell dirname "$(shell gcc -print-libgcc-file-name)")
TAG              ?= $(shell git describe --tags --abbrev=0 2>/dev/null)

.PHONY: help
help: ## Show this help message
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) \
		| awk 'BEGIN {FS = ":.*?## "}; {printf "  %-22s %s\n", $$1, $$2}'

# BUILD
.PHONY: install_clang_tools
install_clang_tools: ## Install clang lint dependencies
	sudo apt-get install -y clang-format-$(CLANG_VERSION) clang-tidy-$(CLANG_VERSION)

.PHONY: configure
configure: ## Configure cmake build (debug, with compile_commands.json)
	cmake -B build \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DBUILD_MPQCLI=$(BUILD_MPQCLI) \
		-DCMAKE_CXX_COMPILER=clang++-$(CLANG_VERSION) \
		-DCMAKE_CXX_FLAGS="--gcc-install-dir=$(GCC_INSTALL_DIR)"

.PHONY: build
build: ## Build via cmake
	cmake --build build

.PHONY: build_linux
build_linux: ## Build for Linux using cmake
	cmake -B build \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DBUILD_MPQCLI=$(BUILD_MPQCLI)
	cmake --build build

.PHONY: build_windows
build_windows: ## Build for Windows using cmake
	cmake -B build \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DBUILD_MPQCLI=$(BUILD_MPQCLI)
	cmake --build build --config $(CMAKE_BUILD_TYPE)

.PHONY: build_clean
build_clean: ## Remove cmake build directory
	rm -rf build

# DOCKER
.PHONY: docker_musl_build
docker_musl_build: ## Build Docker image using musl
	docker build -t mpqcli:$(VERSION) -f Dockerfile.musl .

.PHONY: docker_musl_run
docker_musl_run: ## Run the musl Docker image
	@docker run -it mpqcli:$(VERSION) version

.PHONY: docker_glibc_build
docker_glibc_build: ## Build Docker image using glibc
	docker build -t mpqcli:$(VERSION) -f Dockerfile.glibc .

.PHONY: docker_glibc_run
docker_glibc_run: ## Run the glibc Docker image
	@docker run -it mpqcli:$(VERSION) version

# TEST
.PHONY: test
test: build test_mpqcli ## Run test suite (builds binary first)

.PHONY: test_create_venv
test_create_venv: ## Create Python venv and install test dependencies
	python3 -m venv ./.venv
	. ./.venv/bin/activate && \
	pip3 install -r test/requirements.txt

.PHONY: test_mpqcli
test_mpqcli: ## Run pytest test suite
	. ./.venv/bin/activate && \
	python3 -m pytest test -s

.PHONY: test_clean
test_clean: ## Remove test data directory
	rm -rf test/data

.PHONY: test_lint
test_lint: ## Run ruff linter on test directory
	. ./.venv/bin/activate && \
	ruff check ./test

# LINT
.PHONY: fmt_check
fmt_check: ## Check C++ formatting with clang-format
	find src \( -name "*.cpp" -o -name "*.h" \) \
	| xargs clang-format-$(CLANG_VERSION) --dry-run --Werror

.PHONY: fmt
fmt: ## Auto-fix C++ formatting with clang-format
	find src \( -name "*.cpp" -o -name "*.h" \) \
	| xargs clang-format-$(CLANG_VERSION) -i

.PHONY: lint_cpp
lint_cpp: ## Run clang-tidy static analysis (requires: make configure)
	clang-tidy-$(CLANG_VERSION) --quiet -p build \
	--header-filter="$(CURDIR)/src/.*" src/*.cpp 2>&1 \
	| grep -v " warnings generated"; \
	exit $${PIPESTATUS[0]}

.PHONY: lint
lint: fmt_check lint_cpp ## Run all C++ linters

.PHONY: ci
ci: configure build fmt_check lint_cpp test ## Run all CI checks locally

# CLEAN
.PHONY: clean
clean: build_clean test_clean ## Remove all build and test artifacts

# GET
.PHONY: get_project_version
get_project_version: ## Print the project version from CMakeLists.txt
	@grep -oE 'VERSION [0-9]+\.[0-9]+\.[0-9]+' CMakeLists.txt | grep -oE '[0-9]+\.[0-9]+\.[0-9]+'

.PHONY: get_changelog
get_changelog: ## Print release notes for TAG to stdout (default: latest tag; override with TAG=v1.0.0)
	@if [ -z "$(TAG)" ]; then \
		echo "Error: no tag resolved. Create a git tag or pass TAG=v1.0.0" >&2; \
		exit 1; \
	fi
	@notes=$$(awk -v tag="$(TAG)" \
		'/^## /{if(found)exit; if(index($$0,"## "tag" ")==1 || $$0=="## "tag)found=1; next} found{print}' \
		CHANGELOG.md); \
	if [ -z "$$notes" ]; then \
		echo "Error: no CHANGELOG entry found for $(TAG)" >&2; \
		exit 1; \
	fi; \
	echo "$$notes"

# RELEASE
.PHONY: fetch_downloads
fetch_downloads: ## Fetch package downloads and update README.md badge
	@DOWNLOADS=$$(curl -s "$(PACKAGE_URL)" \
		| grep -A2 "Total downloads" \
		| grep -o '<h3 title="[0-9]*">[0-9]*</h3>' \
		| grep -o 'title="[0-9]*"' \
		| grep -o '[0-9]*' \
		| head -1); \
	sed -i "s/package_downloads-[0-9]*-green/package_downloads-$$DOWNLOADS-green/" $(README); \
	echo "[*] Updated package downloads badge: $$DOWNLOADS"
