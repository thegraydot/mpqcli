SHELL := /bin/bash

# Project-owned C++ directories, derived rather than hardcoded so that adding
# one does not also require remembering to edit the format and lint targets
CPP_DIRS         := $(wildcard src app test)
CPP_LINT_DIRS    := $(wildcard src app)

JOBS             ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
BUILD_TYPE       ?= Debug
CMAKE_ARGS       ?=
MPQCLI_BUILD_APP ?= ON

# Prefer the versioned tool, fall back to the plain name. Falling back rather
# than failing keeps the error legible: "clang-format: command not found"
# beats a make-level complaint about an empty variable.
CLANG_VERSION    ?= 18
CLANG_CC         ?= $(shell command -v clang-$(CLANG_VERSION) || echo clang)
CLANG_CXX        ?= $(shell command -v clang++-$(CLANG_VERSION) || echo clang++)
CLANG_FORMAT     ?= $(shell command -v clang-format-$(CLANG_VERSION) || echo clang-format)
CLANG_TIDY       ?= $(shell command -v clang-tidy-$(CLANG_VERSION) || echo clang-tidy)

# clang-tidy resolves headers through the compiler that produced
# compile_commands.json. Pointed at a GCC build it cannot find libstdc++ and
# emits confident diagnostics from a broken AST, so it gets its own tree.
LINT_DIR         ?= build/lint

# Empty on a machine with no GCC, and configure_lint then passes no
# --gcc-install-dir at all. The && is what keeps it empty: dirname of an empty
# string is ".", which clang would take as the install directory.
GCC_INSTALL_DIR  := $(shell f=$$(gcc -print-libgcc-file-name 2>/dev/null) && dirname "$$f")

README           := README.md
PACKAGE_URL      := https://github.com/thegraydot/mpqcli/pkgs/container/mpqcli

# The version is declared once, in project(... VERSION X.Y.Z). The
# cmake_minimum_required line also says VERSION and comes first, so it is skipped.
VERSION_AWK      := /cmake_minimum_required/ { next } \
  match($$0, /VERSION[ \t]+[0-9]+\.[0-9]+\.[0-9]+/) { \
    v = substr($$0, RSTART, RLENGTH); sub(/VERSION[ \t]+/, "", v); \
    print v; found = 1; exit } \
  END { if (!found) exit 1 }
VERSION           = $(shell awk '$(VERSION_AWK)' CMakeLists.txt)

.PHONY: help
help: ## Show this help message
	@awk 'BEGIN {FS = ":.*?## "} /^##@ / {printf "\n%s\n", substr($$0, 5)} \
		/^[a-zA-Z_-]+:.*## / {printf "  %-22s %s\n", $$1, $$2}' $(MAKEFILE_LIST)

##@ BUILD

.PHONY: configure
configure: ## Configure the cmake build
	cmake -B build/dev \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DMPQCLI_BUILD_APP=$(MPQCLI_BUILD_APP) \
		$(CMAKE_ARGS)

.PHONY: build
build: configure ## Build the project
	cmake --build build/dev --parallel $(JOBS)

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

.PHONY: format
format: ## Format all source files with clang-format
	find $(CPP_DIRS) \( -name "*.cpp" -o -name "*.h" \) \
	| xargs $(CLANG_FORMAT) -i

.PHONY: check_format
check_format: ## Check formatting without modifying files
	find $(CPP_DIRS) \( -name "*.cpp" -o -name "*.h" \) \
	| xargs $(CLANG_FORMAT) --dry-run --Werror

.PHONY: configure_lint
configure_lint: ## Configure $(LINT_DIR) with clang, so clang-tidy can parse the sources
	cmake -B $(LINT_DIR) \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DCMAKE_C_COMPILER=$(CLANG_CC) \
		-DCMAKE_CXX_COMPILER=$(CLANG_CXX) \
		$(if $(GCC_INSTALL_DIR),-DCMAKE_CXX_FLAGS="--gcc-install-dir=$(GCC_INSTALL_DIR)") \
		$(CMAKE_ARGS)

.PHONY: check_lint
check_lint: configure_lint ## Run clang-tidy static analysis
	$(CLANG_TIDY) --quiet -p $(LINT_DIR) \
	--header-filter="$(CURDIR)/(src|app)/.*" $$(find $(CPP_LINT_DIRS) -name "*.cpp") 2>&1 \
	| grep -v " warnings generated"; \
	exit $${PIPESTATUS[0]}

.PHONY: check_embed
check_embed: ## Syntax-check the embedded completion scripts with every shell present
	bash -n completion/mpqcli.bash
	@if command -v zsh >/dev/null; then zsh -n completion/mpqcli.zsh; \
	  else echo "[*] zsh not installed, skipped"; fi
	@if command -v fish >/dev/null; then fish --no-execute completion/mpqcli.fish; \
	  else echo "[*] fish not installed, skipped"; fi
	@if command -v pwsh >/dev/null; then pwsh -NoProfile -Command \
	  '[scriptblock]::Create((Get-Content -Raw completion/mpqcli.ps1)) | Out-Null'; \
	  else echo "[*] pwsh not installed, skipped"; fi

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

##@ DOCKER

.PHONY: docker_build
docker_build: ## Build the Docker image, tagged with the project version
	docker build -t mpqcli:$(VERSION) .

.PHONY: docker_run
docker_run: ## Run the Docker image
	@docker run -it mpqcli:$(VERSION) version

##@ GENERATE

# The docs site builds from the committed copy, so this is run deliberately and the
# result committed rather than being made a prerequisite of docs_build
.PHONY: gen_docs_changelog
gen_docs_changelog: ## Copy CHANGELOG.md into the docs site
	@cp CHANGELOG.md docs/changelog.md
	@echo "[*] Updated docs/changelog.md"

.PHONY: gen_downloads
gen_downloads: ## Update the package downloads badge in README.md from GitHub
	@DOWNLOADS=$$(curl -s "$(PACKAGE_URL)" \
		| grep -A2 "Total downloads" \
		| grep -o '<h3 title="[0-9]*">[0-9]*</h3>' \
		| grep -o 'title="[0-9]*"' \
		| grep -o '[0-9]*' \
		| head -1); \
	sed -i "s/package_downloads-[0-9]*-green/package_downloads-$$DOWNLOADS-green/" $(README); \
	echo "[*] Updated package downloads badge: $$DOWNLOADS"

##@ GET

.PHONY: get_version
get_version: ## Print the project version from CMakeLists.txt (fails if absent)
	@awk '$(VERSION_AWK)' CMakeLists.txt

.PHONY: get_changelog
get_changelog: ## Print release notes for TAG to stdout (TAG=v1.0.0)
	@tag="$(TAG)"; tag="$${tag#v}"; \
	if [[ -z "$$tag" ]]; then \
	  printf 'get_changelog: TAG is empty; pass TAG=v1.0.0\n' >&2; \
	  exit 1; \
	fi; \
	notes="$$(awk -v tag="$$tag" ' \
	  /^## / { if (found) exit; if (index($$0,"## "tag" ")==1 || $$0=="## "tag) found=1; next } \
	  found { lines[n++]=$$0 } \
	  END { \
	    s=0; while (s<n && lines[s]~/^[[:space:]]*$$/) s++; \
	    e=n-1; while (e>=s && lines[e]~/^[[:space:]]*$$/) e--; \
	    for (i=s;i<=e;i++) print lines[i] \
	  }' CHANGELOG.md)"; \
	if [[ -z "$$notes" ]]; then \
	  printf 'get_changelog: no CHANGELOG entry for %s\n' "$$tag" >&2; \
	  exit 1; \
	fi; \
	printf '%s\n' "$$notes"

##@ CI

.PHONY: check_all
check_all: check_format check_lint check_embed ## Run every static check

.PHONY: ci
ci: check_all test ## Run the checks CI runs

.PHONY: clean
clean: test_clean docs_clean ## Remove all build, test, docs and release artefacts
	rm -rf build dist install.sh install.ps1 checksums.txt checksums.txt.sigstore.json
