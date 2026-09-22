CXX ?= g++
CPPFLAGS += -Iinclude -IGalsGenerated
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic

TARGET := compilador_sueco_go
BUILD_DIR := build
SOURCE_DIR := src
INCLUDE_DIR := include
GENERATED_DIR := GalsGenerated
DEFAULT_ZIP := compilador_sueco_go - C++.zip
ZIP ?= $(DEFAULT_ZIP)

SOURCES := $(wildcard $(SOURCE_DIR)/*.cpp) $(wildcard $(GENERATED_DIR)/*.cpp)
OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
DEPENDENCIES := $(OBJECTS:.o=.d)

.PHONY: all clean update-generated

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

update-generated:
	@set -eu; \
	zip_path="$(ZIP)"; \
	test -f "$$zip_path" || { echo "ZIP nao encontrado: $$zip_path" >&2; exit 1; }; \
	stage_dir=$$(mktemp -d ./.GalsGenerated.tmp.XXXXXX); \
	trap 'rm -rf "$$stage_dir"' EXIT INT TERM; \
	unzip -q "$$zip_path" -d "$$stage_dir"; \
	find "$$stage_dir" -exec touch {} +; \
	test -f "$$stage_dir/Lexico.h" && test -f "$$stage_dir/Lexico.cpp" && \
	test -f "$$stage_dir/Sintatico.h" && test -f "$$stage_dir/Sintatico.cpp" || \
	{ echo "ZIP invalido: arquivos gerados esperados nao foram encontrados na raiz" >&2; exit 1; }; \
	rm -rf "$(GENERATED_DIR)"; \
	mkdir -p "$(GENERATED_DIR)"; \
	find "$$stage_dir" -maxdepth 1 -mindepth 1 -not -name Semantico.cpp -not -name Semantico.h -exec mv '{}' "$(GENERATED_DIR)" \; ;\
	rm -rf "$$stage_dir"; \
	trap - EXIT INT TERM; \
	echo "Conteudo de $(GENERATED_DIR)/ atualizado a partir de $$zip_path"

clean:
	rm -rf "$(BUILD_DIR)" "$(TARGET)"

-include $(DEPENDENCIES)
