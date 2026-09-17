SHELL := /bin/bash

# Project-owned C++ directories, derived rather than hardcoded so that adding
# one does not also require remembering to edit the format and lint targets
CPP_DIRS         := $(wildcard src app test)
CPP_LINT_DIRS    := $(wildcard src app)

CMAKE_BUILD_TYPE := Release
MPQCLI_BUILD_APP ?= ON
JOBS             ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Prefer the versioned tool, fall back to the plain name. Falling back rather
# than failing keeps the error legible: "clang-format: command not found"
# beats a make-level complaint about an empty variable.
CLANG_VERSION    ?= 18
CLANG_FORMAT ?= $(shell command -v clang-format-$(CLANG_VERSION) 2>/dev/null || echo clang-format)
CLANG_TIDY   ?= $(shell command -v clang-tidy-$(CLANG_VERSION) 2>/dev/null || echo clang-tidy)

# clang-tidy resolves headers through the compiler that produced
# compile_commands.json. Pointed at a GCC build it cannot find libstdc++ and
# emits confident diagnostics from a broken AST, so it gets its own tree.
LINT_DIR         ?= build-lint
GCC_INSTALL_DIR  := $(shell dirname "$(shell gcc -print-libgcc-file-name)")

VERSION          := $(shell awk '/project\(MPQCLI VERSION/ {gsub(/\)/, "", $$3); print $$3}' CMakeLists.txt)
README           := README.md
PACKAGE_URL      := https://github.com/thegraydot/mpqcli/pkgs/container/mpqcli
TAG              ?= $(shell git describe --tags --abbrev=0 2>/dev/null)

.PHONY: help
help: ## Show this help message
	@awk 'BEGIN {FS = ":.*?## "} /^##@ / {printf "\n%s\n", substr($$0, 5)} \
		/^[a-zA-Z_-]+:.*## / {printf "  %-22s %s\n", $$1, $$2}' $(MAKEFILE_LIST)

##@ BUILD
.PHONY: install_clang_tools
install_clang_tools: ## Install clang lint dependencies
	sudo apt-get install -y clang-format-$(CLANG_VERSION) clang-tidy-$(CLANG_VERSION)

.PHONY: configure
configure: ## Configure cmake build (debug, with compile_commands.json)
	cmake -B build \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DMPQCLI_BUILD_APP=$(MPQCLI_BUILD_APP)

.PHONY: build
build: ## Build via cmake
	cmake --build build --parallel $(JOBS)

.PHONY: build_linux
build_linux: ## Build for Linux using cmake
	cmake -B build \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DMPQCLI_BUILD_APP=$(MPQCLI_BUILD_APP)
	cmake --build build --parallel $(JOBS)

.PHONY: build_windows
build_windows: ## Build for Windows using cmake
	cmake -B build \
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) \
		-DMPQCLI_BUILD_APP=$(MPQCLI_BUILD_APP)
	cmake --build build --config $(CMAKE_BUILD_TYPE) --parallel $(JOBS)

##@ DOCKER
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

##@ DOCS
.PHONY: docs_mermaid
docs_mermaid: ## Fetch mermaid.min.js and mermaid-init.js into the repo root (gitignored)
	mdbook-mermaid install .

.PHONY: docs_build
docs_build: docs_mermaid ## Build the documentation site into book/
	mdbook build

.PHONY: docs_serve
docs_serve: docs_mermaid ## Serve the docs locally with live reload
	mdbook serve --open

.PHONY: docs_clean
docs_clean: ## Remove the generated docs site and mermaid assets
	rm -rf book mermaid.min.js mermaid-init.js

##@ TEST
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

##@ LINT
.PHONY: check_format
check_format: ## Check C++ formatting with clang-format
	find $(CPP_DIRS) \( -name "*.cpp" -o -name "*.h" \) \
	| xargs $(CLANG_FORMAT) --dry-run --Werror

.PHONY: format
format: ## Auto-fix C++ formatting with clang-format
	find $(CPP_DIRS) \( -name "*.cpp" -o -name "*.h" \) \
	| xargs $(CLANG_FORMAT) -i

.PHONY: configure_lint
configure_lint: ## Configure $(LINT_DIR) with clang++, so clang-tidy can parse the sources
	cmake -B $(LINT_DIR) \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DCMAKE_CXX_COMPILER=clang++-$(CLANG_VERSION) \
		-DCMAKE_CXX_FLAGS="--gcc-install-dir=$(GCC_INSTALL_DIR)"

.PHONY: check_lint
check_lint: configure_lint ## Run clang-tidy static analysis
	$(CLANG_TIDY) --quiet -p $(LINT_DIR) \
	--header-filter="$(CURDIR)/(src|app)/.*" $$(find $(CPP_LINT_DIRS) -name "*.cpp") 2>&1 \
	| grep -v " warnings generated"; \
	exit $${PIPESTATUS[0]}

.PHONY: check_all
check_all: check_format check_lint ## Run every static check

.PHONY: ci
ci: configure build check_all test ## Run all CI checks locally

##@ CLEAN
.PHONY: clean
clean: test_clean docs_clean ## Remove all build, test, and docs artifacts
	rm -rf build $(LINT_DIR)

##@ GET
.PHONY: get_version
get_version: ## Print the project version from CMakeLists.txt
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

##@ RELEASE
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
